/*
 * OpenVALive - Live Core
 * live.h : the instrument around the engines. Four tracks, one clock.
 *
 *   tracks     each track = one engine + one looping pattern (its own length:
 *              polymeter for free), mute, gain
 *   clock      internal tempo or MIDI clock in, MIDI clock out, swing
 *   record     overdub what you play into the selected track, in time
 *   capture    play freely, press CAPTURE: the last phrase becomes the loop,
 *              already playing in time (even when the transport was stopped)
 *   undo/redo  every destructive gesture can be taken back (8 steps)
 *   scenes     8 snapshots of every sound and every mute
 *   morph      one knob glides between two scenes
 *   perform    hold-to-play effects: loops of 1 bar .. 1/32 (beat repeat) and
 *              parameter overrides (filter sweep, delay throw...) that spring
 *              back; breaks (tracks out now, back on the next bar)
 *   transitions scene changes wait for the next bar, and can cross-fade
 *              (auto-Morph) over 1, 2 or 4 bars
 *   creative   Mutate, Humanize, Fill, Simplify... (creative.h)
 *
 * Threading: every function that changes the Live Core runs on the AUDIO
 * side (audio interrupt or audio thread). The UI sends ols_event_t through
 * the event queue and the audio side calls ols_live_apply_event(). The UI
 * may READ the state to draw it (small values: a torn read only affects
 * one frame of the screen).
 *
 * Memory: about 15 KB plus the engines. No allocation, no C library.
 */
#ifndef OLS_LIVE_H
#define OLS_LIVE_H

#include <stdint.h>
#include "live_types.h"
#include "clock.h"
#include "creative.h"
#include "event_queue.h"
#include "../engines/engine.h"

#define OLS_UNDO_DEPTH 8
#define OLS_SCENES 8
#define OLS_CAPTURE_NOTES 128
#define OLS_ACTIVE_NOTES 16        /* sequenced notes sounding per track */
#define OLS_HELD_NOTES 10          /* keys held while recording */
#define OLS_OVERRIDES 8
#define OLS_RENDER_CHUNK 64        /* render granularity inside a block, samples */
#define OLS_TRACK_CURRENT 0xFFu    /* "the selected track" in commands */
#define OLS_TRACK_ALL 0xFEu        /* every track (overrides) */

/* The instrument macros: one gesture, many parameters on every track. 0.5 = neutral. */
enum {
    OLS_MACRO_ENERGY,      /* brighter, more drive, tighter decays, denser loops */
    OLS_MACRO_SPACE,       /* more delay, chorus, longer tails */
    OLS_MACRO_TENSION,     /* resonance, filter envelope, grit */
    OLS_MACRO_DENSITY,     /* fewer notes (thinned off-beats) .. more (doubled hits) */
    OLS_MACRO_COUNT
};

/* Commands carried by EV_LIVE events (field a). */
typedef enum {
    LIVE_PLAY_TOGGLE,
    LIVE_PLAY,
    LIVE_STOP,
    LIVE_RECORD_TOGGLE,
    LIVE_CAPTURE,
    LIVE_UNDO,                 /* target = a track (its own steps) or OLS_TRACK_ALL (the newest step) */
    LIVE_REDO,                 /* target: as LIVE_UNDO */
    LIVE_CLEAR,                /* empties the track (undoable) */
    LIVE_SELECT,               /* b = track */
    LIVE_MUTE_TOGGLE,          /* b = track */
    LIVE_CREATIVE,             /* b = ols_creative_op_t, v = amount, id = arg (int16) */
    LIVE_EUCLID,               /* b = hits, id = steps, uses the last note played */
    LIVE_LENGTH,               /* id = ticks */
    LIVE_TEMPO,                /* v = BPM */
    LIVE_SWING,                /* v = 0..1 */
    LIVE_SCENE_STORE,          /* b = slot */
    LIVE_SCENE_RECALL,         /* b = slot */
    LIVE_MORPH,                /* b = scene A, id = scene B, v = 0..1 */
    LIVE_STUTTER,              /* b = 1 on / 0 off, id = on: repeat length in ticks, off: 1 = now (else at the end of
                                * the slice playing: the music stays in time), target = a track or OLS_TRACK_ALL */
    LIVE_OVERRIDE,             /* b = 1 hold / 0 release, id = OLS_ROLE_*, v = value, target = track or OLS_TRACK_ALL */
    LIVE_QUANTIZE,             /* b = 0 off / 1 to the 16th */
    LIVE_EXTERNAL_CLOCK,       /* b = 0 internal / 1 follow MIDI clock */
    LIVE_SEND_CLOCK,           /* b = 0 / 1: MIDI clock and transport out */
    LIVE_GAIN,                 /* v = track gain 0..2, target = track */
    LIVE_METRONOME,            /* b = 0 off / 1 on (with it on, REC from stop counts one bar in) */
    LIVE_HOLD_MUTE,            /* b = 1: mute the tracks of mask id now / 0: bring them back on the next bar (breaks) */
    LIVE_XFADE,                /* b = bars a scene change takes (0 = a cut on the next bar) */
    LIVE_MACRO,                /* b = OLS_MACRO_*, v = 0..1 (0.5 neutral) */
    LIVE_PUMP,                 /* v = sidechain depth 0..1: the drum track ducks the others */
    LIVE_CHAIN,                /* b = 1 on / 0 off: the stored scenes in turn, id = bars each */
    LIVE_TAP,                  /* tap tempo: the tempo follows the last taps */
    LIVE_PUNCH,                /* b = 1: record over the selected track, replacing what the playhead passes; 0: stop */
    LIVE_RESTART,              /* every track back to its start, now (internal clock; external: the clock decides) */
    LIVE_NOTE_EDIT,            /* a loop's note: target = track, b = ols_note_edit_t, id = note index, v = the value */
    LIVE_PAN,                  /* target = track, v = -1 left .. 1 right (a spring-loaded wheel: back to 0) */
    LIVE_CMD_COUNT
} ols_live_cmd_t;

/* LIVE_NOTE_EDIT: BEGIN (one undo step for the edits that follow), the note's start (v = tick), length (ticks),
 * velocity, pitch; DELETE it; ADD one (id = pitch, v = tick; a 1/16, velocity 100). The loop stays sorted. */
typedef enum { NOTE_EDIT_BEGIN, NOTE_EDIT_TICK, NOTE_EDIT_LEN, NOTE_EDIT_VEL, NOTE_EDIT_PITCH, NOTE_EDIT_DELETE,
               NOTE_EDIT_ADD } ols_note_edit_t;

typedef struct {
    uint8_t note;
    uint32_t off_tick;         /* transport tick at which it stops */
} ols_active_note_t;

typedef struct {
    uint8_t note, vel;
    uint16_t pos;              /* pattern position at the key press */
    uint32_t start;            /* transport tick at the key press */
} ols_held_note_t;

typedef struct {
    ols_engine_t engine;
    uint8_t kind;              /* which kind of engine (the host's numbering: bank.h), saved in projects */
    ols_pattern_t pat;
    uint8_t mute;
    float gain;
    float pan, pan_now;        /* -1 left .. 0 centre .. 1 right (LIVE_PAN), and where the mix is (ramped) */

    ols_active_note_t active[OLS_ACTIVE_NOTES];
    uint8_t active_count;
    uint8_t skip[(OLS_PATTERN_NOTES + 7) / 8];  /* notes that lost their chance roll this pass */
    uint8_t dense[(OLS_PATTERN_NOTES + 7) / 8]; /* notes doubled by DENSITY this pass */

    ols_held_note_t held[OLS_HELD_NOTES];       /* recording: notes not released yet */
    uint8_t held_count;
    uint8_t suppress_note;                      /* just recorded ahead of the playhead: don't play twice */
    uint16_t suppress_tick;

    uint16_t stutter_from;                      /* pattern position where the stutter started */
    uint8_t stutter;                            /* 1: this track repeats the slice (L->stutter_len) */
    uint8_t last_note, last_vel;                /* last note played live (Euclid uses it) */
} ols_track_t;

typedef struct {
    uint32_t t;                /* transport tick (on_grid) or free tick */
    uint16_t len;              /* 0 while the key is held */
    uint8_t note, vel;
    uint8_t track;
    uint8_t on_grid;           /* played while the transport was running */
} ols_capture_note_t;

typedef struct {
    uint8_t track;             /* OLS_UNDO_GONE: a redo step a newer gesture made useless */
    uint8_t undone;            /* 1: undone (it then holds what redo puts back) */
    ols_pattern_t pat;
} ols_undo_step_t;
#define OLS_UNDO_GONE 0xFFu

typedef struct {
    uint8_t valid;
    uint8_t mute_mask;
    uint8_t kind[OLS_TRACKS];                   /* engine kind of each track when stored */
    uint16_t v[OLS_TRACKS][OLS_MAX_PARAMS];     /* parameters, 0..65535 */
} ols_scene_t;

typedef struct {
    uint8_t active;
    uint8_t track;
    uint16_t id;
    float base;                /* value to restore on release */
} ols_override_t;

typedef struct {
    ols_clock_t clock;
    ols_track_t track[OLS_TRACKS];
    uint8_t track_count;
    uint8_t current;           /* selected track: keys, MIDI in, record, creative */
    uint8_t recording;
    uint8_t punch;             /* 1: the recording replaces (LIVE_PUNCH): old notes go as the playhead passes them */
    uint8_t quantize;
    uint32_t punch_start;      /* clock tick of the punch: after one loop length it only adds */
    float swing;               /* 0..1: 1 pushes every off-beat 16th by two thirds of a step */
    float master;              /* mix gain */
    uint32_t rng;
    uint32_t changes;          /* counts every change of patterns, mutes or scenes (UI: redraw, save) */

    /* capture: the last notes played, always recorded */
    ols_capture_note_t cap[OLS_CAPTURE_NOTES];
    uint16_t cap_write;
    uint16_t cap_count;

    /* undo history, oldest to newest: [undo_base .. undo_top); each step knows its track, so a track can be undone
     * and redone on its own (the steps of the others stay) */
    ols_undo_step_t undo[OLS_UNDO_DEPTH];
    uint32_t undo_base, undo_top;
    ols_pattern_t scratch;

    ols_scene_t scene[OLS_SCENES];
    uint8_t scene_current;
    uint8_t morph_a, morph_b;
    float morph;

    ols_override_t ovr[OLS_OVERRIDES];

    uint8_t stutter;           /* 1: a loop runs on the tracks whose own flag is set */
    uint16_t stutter_len;
    uint32_t stutter_start;
    uint32_t stutter_end;      /* !0: the loop lets go at this tick (the end of the slice playing) */

    /* breaks: tracks held out of the mix, back on the next bar */
    uint8_t hold_mute;         /* mask of tracks held out */
    uint8_t hold_release;      /* 1: bring them back at the next bar */

    /* transitions: a scene asked for while playing waits for the next bar */
    int8_t scene_queued;       /* -1: none */
    uint8_t xfade_bars;        /* 0: cut; 1, 2, 4: auto-Morph over that many bars */
    uint8_t xfade_active;
    uint8_t xfade_to;
    uint32_t xfade_start, xfade_len;
    ols_scene_t xfade_from;    /* the sound as it was when the cross-fade began */

    /* scene chain (song mode): the stored scenes in order, `chain_bars` bars each */
    uint8_t chain_on;
    uint8_t chain_bars;
    uint8_t chain_pos;         /* scene playing in the chain */
    uint32_t chain_next;       /* transport tick of the next change */

    /* instrument macros */
    float macro[OLS_MACRO_COUNT];
    float density;             /* -1 .. +1, from DENSITY and ENERGY */

    /* sidechain ("pump"): the first track of kind duck_kind ducks the others */
    float pump;                /* depth 0..1 */
    uint8_t duck_kind;         /* set by the host: its drum kind */
    float duck_env;            /* the source's level, follows its peaks */
    float duck_gain;           /* the gain the others got last chunk */
    float duck_release;        /* per-sample release of the envelope */

    /* MIDI out (optional): sequenced notes on channels 1..4, clock and transport */
    void (*midi_out)(void *ctx, uint8_t status, uint8_t d1, uint8_t d2);
    void *midi_out_ctx;
    uint8_t send_notes, send_clock;

    uint8_t input_track[128];  /* ols_live_input(): the track each MIDI note started on */
    uint32_t midi_late;        /* set by the host before feeding MIDI: how many samples ago it arrived */

    /* metronome and count-in */
    uint8_t metronome;
    uint32_t rec_from;         /* REC takes notes from this transport tick on (after the count-in) */
    float click_amp, click_phase, click_inc, click_coef;

    /* tap tempo */
    uint32_t samples;          /* samples rendered since init */
    uint32_t tap_last;
    float tap_interval;        /* averaged, samples */

    /* optional: gives the engine of a kind for a track (a project saved with another kind) */
    ols_engine_t (*engine_for_kind)(void *ctx, int track, uint8_t kind);
    void *kind_ctx;

    float tmp_l[OLS_RENDER_CHUNK], tmp_r[OLS_RENDER_CHUNK];
} ols_live_t;

/* --- set-up (before audio starts) ------------------------------------ */

/* engines: one per track (1..OLS_TRACKS), already initialised. */
void ols_live_init(ols_live_t *L, const ols_engine_t *engines, int count, float sample_rate);
void ols_live_set_midi_out(ols_live_t *L, void (*fn)(void *, uint8_t, uint8_t, uint8_t), void *ctx);

/* Plugs another engine into a track (audio side, or before audio starts).
 * The old engine is silenced; the loop, mute and gain stay. */
void ols_live_set_engine(ols_live_t *L, int track, ols_engine_t engine, uint8_t kind);

/* --- audio side ------------------------------------------------------- */

void ols_live_render(ols_live_t *L, float *out_l, float *out_r, int n);

/* Applies an event from the queue: notes and parameters (target = track,
 * OLS_TRACK_CURRENT = the selected one) and EV_LIVE commands.
 * Returns 0 for an event it does not know (EV_PRESET: engine-specific). */
int ols_live_apply_event(ols_live_t *L, const ols_event_t *e);
void ols_live_command(ols_live_t *L, ols_live_cmd_t cmd, uint8_t b, uint16_t id, float v, uint8_t target);

void ols_live_note_on(ols_live_t *L, int track, uint8_t note, uint8_t vel);
void ols_live_note_off(ols_live_t *L, int track, uint8_t note);
void ols_live_all_notes_off(ols_live_t *L);

void ols_live_play(ols_live_t *L);
void ols_live_stop(ols_live_t *L);
void ols_live_select(ols_live_t *L, int track);
void ols_live_mute(ols_live_t *L, int track, int mute);

int  ols_live_capture(ols_live_t *L);           /* 1 = a loop was made */
void ols_live_undo_push(ols_live_t *L, int track);
int  ols_live_undo(ols_live_t *L);                  /* the newest step, whatever its track */
int  ols_live_redo(ols_live_t *L);
int  ols_live_undo_track(ols_live_t *L, int track);  /* a track's own newest step; OLS_TRACK_ALL: as ols_live_undo */
int  ols_live_redo_track(ols_live_t *L, int track);
void ols_live_clear(ols_live_t *L, int track);
void ols_live_creative(ols_live_t *L, int track, ols_creative_op_t op, float amount, int32_t arg);
void ols_live_euclid(ols_live_t *L, int track, uint32_t hits, uint32_t steps);

void ols_live_scene_store(ols_live_t *L, int slot);
void ols_live_scene_recall(ols_live_t *L, int slot);
void ols_live_morph(ols_live_t *L, int scene_a, int scene_b, float amount);
/* Holds the parameter playing `role` (OLS_ROLE_CUTOFF...) at value; hold = 0 releases it. */
void ols_live_override(ols_live_t *L, int track, uint16_t role, float value, int hold);
void ols_live_stutter(ols_live_t *L, int on, uint32_t ticks);     /* every track; off: at the end of the slice */
void ols_live_stutter_track(ols_live_t *L, int on, uint32_t ticks, int track, int now);
void ols_live_tap(ols_live_t *L);

/* A scene, DJ style: now when stopped; on the next bar when playing, cut or
 * cross-faded over L->xfade_bars bars. */
void ols_live_scene_go(ols_live_t *L, int slot);
void ols_live_hold_mute(ols_live_t *L, uint8_t mask, int hold);
void ols_live_macro(ols_live_t *L, int macro, float value);
void ols_live_chain(ols_live_t *L, int on, uint32_t bars);
int  ols_live_track_silent(const ols_live_t *L, int track);   /* muted or held out */

/* MIDI real-time bytes from midi_in (midi_in_on_realtime(m, ols_live_midi_realtime, L)).
 * A host with time-stamped MIDI sets L->midi_late first (samples since the byte arrived). */
void ols_live_midi_realtime(void *live, uint8_t byte);

/* MIDI program change n: recalls scene n + 1 if it is stored (midi_in_on_program). */
void ols_live_midi_program(void *live, uint8_t program);

/* An engine-shaped front door for midi_in: notes go to the selected track
 * (recorded and captured like the keys), CCs and pitch bend to its engine. */
ols_engine_t ols_live_input(ols_live_t *L);

/* --- reading the state (any side) ------------------------------------- */

/* Pattern position of a track in steps (0..), for the screen. */
uint32_t ols_live_step(const ols_live_t *L, int track);
int ols_live_can_undo(const ols_live_t *L);
int ols_live_can_redo(const ols_live_t *L);
int ols_live_can_undo_track(const ols_live_t *L, int track);
int ols_live_can_redo_track(const ols_live_t *L, int track);

#endif /* OLS_LIVE_H */
