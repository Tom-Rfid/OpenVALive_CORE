/*
 * OpenVALive - DRUM engine: an analog-style drum machine (TR-808 family).
 *
 * Every sound is synthesised, nothing is sampled: no flash, no sample
 * licence, and every knob reshapes the sound live.
 *
 *   KICK     sine with a fast pitch drop, long decay, a click on top
 *   SNARE    two tuned sines + high-passed noise ("snap")
 *   CLAP     band-passed noise in three quick bursts, then a tail
 *   HATS     six square "metal" oscillators, high-passed; closed chokes open
 *   TOMS     low / mid / high sines with a small pitch drop
 *   RIM      short high click        COWBELL   two squares, band-passed
 *
 * Notes: the GM drum notes work (36 kick, 38 snare, 39 clap, 42 closed hat,
 * 46 open hat, 45/47/48 toms, 37 rim, 56 cowbell...), and every octave of
 * the keyboard repeats the same layout (note % 12), so the FM-1's keys
 * (F3..G5) play the whole kit:
 *
 *   C kick  C# rim  D snare  D# clap  E snare  F low tom  F# closed hat
 *   G mid tom  G# closed hat  A high tom  A# open hat  B cowbell
 *
 * Mono out (copied to both channels), one voice per instrument (a new hit
 * restarts it, like the original). A silent kit costs nothing.
 */
#ifndef ENGINE_DRUM_H
#define ENGINE_DRUM_H

#include <stdint.h>
#include "../../dsp/dsp_common.h"
#include "../../dsp/svf.h"
#include "../engine.h"

typedef enum {
    DR_KICK, DR_SNARE, DR_CLAP, DR_CH, DR_OH, DR_LTOM, DR_MTOM, DR_HTOM, DR_FTOM, DR_RIM, DR_COWBELL,
    DR_COUNT,
    DR_TRIANGLE = DR_COUNT    /* plays on the cowbell's voice (one cuts the other): no RAM for an 11th voice */
} dr_inst_t;

/* Parameters, normalised 0..1. The first four are the performance macros. */
typedef enum {
    DR_P_TUNE = 0,        /* whole kit, -12 .. +12 semitones */
    DR_P_DECAY,           /* whole kit, x0.25 .. x4 */
    DR_P_CUTOFF,          /* low-pass on the mix */
    DR_P_DRIVE,
    DR_P_KICK_PITCH, DR_P_KICK_DECAY, DR_P_KICK_CLICK, DR_P_KICK_SWEEP,
    DR_P_SNARE_TONE, DR_P_SNARE_SNAP, DR_P_SNARE_DECAY,
    DR_P_HAT_TONE, DR_P_HAT_DECAY,
    DR_P_CLAP_DECAY, DR_P_TOM_DECAY,
    DR_P_LEVEL_KICK, DR_P_LEVEL_SNARE, DR_P_LEVEL_HAT, DR_P_LEVEL_PERC,
    DR_P_RESO,
    DR_P_VOLUME,
    DR_P_BASS, DR_P_TREBLE,           /* a two-band EQ on the kit's output: 0.5 flat, 0 -12 dB, 1 +5 dB */
    DR_P_COUNT
} dr_param_t;

typedef enum {
    DR_KIT_808 = 0,       /* the classic: boomy kick, soft snare */
    DR_KIT_909,           /* punchy kick with its click, cracking snare, bright metallic hats */
    DR_KIT_TIGHT,         /* short and punchy, for fast patterns */
    DR_KIT_LOFI,          /* dark and driven */
    DR_KIT_IRON,          /* industrial: low, metallic, distorted, long rattling hats */
    DR_KIT_COUNT
} dr_kit_t;

/* One instrument's sound. Every envelope is exponential: value *= coef per sample. */
typedef struct {
    uint8_t active;
    float level;          /* velocity x instrument level */
    float phase[6];       /* oscillators (0..1) */
    float freq;           /* main pitch, Hz */
    float amp, amp_coef;  /* body */
    float sweep, sweep_coef;   /* pitch drop (kick, toms) */
    float noise, noise_coef;   /* noise or click layer */
    uint32_t age;         /* samples since the hit (clap bursts) */
    dsp_svf_t svf;        /* shapes the noise / metal */
} dr_voice_t;

typedef struct {
    float p[DR_P_COUNT];
    float role_off[OLS_ROLE_COUNT];   /* performance offsets (macros) */

    /* derived from the parameters */
    float tune;           /* frequency ratio */
    float decay;          /* time scale */
    float cutoff_hz, reso;
    float drive_gain, drive_comp;
    float volume;
    float bass_g, treble_g;           /* the EQ's gains on its low and high bands (1: flat) */
    float eq_lo, eq_hi;               /* its two one-pole low-passes (150 Hz, 5 kHz) */

    float cutoff_now;     /* smoothed, set into `out_filter` at control rate */
    dsp_svf_t out_filter;
    int ctrl_left;

    dr_voice_t voice[DR_COUNT];
    float hit_pitch;      /* the next hit's pitch ratio (a GM tom note sets it: six toms on four voices) */
    uint32_t rng;
} engine_drum_t;

void engine_drum_init(engine_drum_t *e);
void engine_drum_load_kit(engine_drum_t *e, dr_kit_t kit);

void  engine_drum_note_on(void *ctx, uint8_t note, uint8_t velocity);
void  engine_drum_note_off(void *ctx, uint8_t note);
void  engine_drum_all_notes_off(void *ctx);
void  engine_drum_set_param(void *ctx, uint16_t id, float value01);
float engine_drum_get_param(void *ctx, uint16_t id);
void  engine_drum_pitch_bend(void *ctx, float semitones);
void  engine_drum_render(void *ctx, float *out_l, float *out_r, int n);
void  engine_drum_role_offset(void *ctx, uint16_t role, float offset);

/* Instrument played by a MIDI note. */
dr_inst_t engine_drum_instrument(uint8_t note);
int engine_drum_active(const engine_drum_t *e);         /* instruments sounding */

extern const ols_engine_api_t ENGINE_DRUM_API;
extern const char *const DR_PARAM_NAMES[DR_P_COUNT];
extern const char *const DR_KIT_NAMES[DR_KIT_COUNT];

#endif /* ENGINE_DRUM_H */
