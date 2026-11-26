/*
 * OpenVALive desktop - what each computer key does.
 *
 *   notes        a w s e d f t g y h u j k o l p ;   (QWERTY, like a tracker)
 *                q z s e d f t g y h u j k o l p m   (AZERTY: automatic on a French Windows, or --azerty)
 *   b / n        previous / next track            v      mute the track
 *   Space        play / stop                      r      record into the track
 *   c            CAPTURE what you just played     x / X  undo / redo
 *   i            quantize on / off                !      panic (all notes off)
 *   Up / Down    octave                           Left / Right   sound of the track (synths, drum kits)
 *   Tab          next control                     + / -  change it
 *                (macros, MORPH SWING TEMPO LEVEL ENERGY SPACE TENSION DENSITY PUMP)
 *   Creative     M mutate  H humanize  F fill  S simplify  R reverse  E expand  C contract
 *   (Shift)      P chance  T ratchet  < > rotate  U / D transpose  G euclid (one more hit each time)
 *   Scenes       W store a scene (next slot)      Q recall the next one (MORPH goes between the last two)
 *   L            metronome (REC from stop counts in) A   tap tempo
 *   K            save the live set (openvalive.set) Esc    quit
 *
 * Commands avoid every note key of both layouts, so they work the same in
 * QWERTY and AZERTY. A terminal reports key presses, not releases: a tapped
 * key plays for 0.7 s, a held key keeps playing with the auto-repeat.
 */
#include <stdio.h>
#include <string.h>
#include "desktop.h"

#define NOTE_KEYS 17
#define FIRST_NOTE 60                      /* C4 at octave 0 */
#define TAP_LENGTH 0.70                    /* seconds */
#define REPEAT_LENGTH 0.15                 /* extension on each auto-repeat */
#define KNOB_STEP (1.0f / 32.0f)
#define CREATIVE_AMOUNT 0.5f

static const char QWERTY_NOTES[NOTE_KEYS + 1] = "awsedftgyhujkolp;";
static const char AZERTY_NOTES[NOTE_KEYS + 1] = "qzsedftgyhujkolpm";

/* note currently playing for each note key, its track, and when it stops */
static struct {
    int note;                              /* -1 = silent */
    int track;
    double off_at;
} held[NOTE_KEYS];

static int initialised;
static char message[64];                   /* last command, shown by the status line */

const char *last_message(void)
{
    return message;
}

static void say(const char *text)
{
    snprintf(message, sizeof message, "%s", text);
}

static void init_held(void)
{
    for (int i = 0; i < NOTE_KEYS; i++) {
        held[i].note = -1;
    }
    initialised = 1;
}

static int note_index(int key)
{
    const char *layout = app.azerty ? AZERTY_NOTES : QWERTY_NOTES;
    const char *found = (key > 0 && key < 128) ? strchr(layout, key) : 0;
    if (found == 0) {
        return -1;
    }
    return (int)(found - layout);
}

static void play_key(int index)
{
    double now = time_now();

    if (held[index].note >= 0) {           /* auto-repeat: keep it sounding */
        double until = now + REPEAT_LENGTH;
        if (until > held[index].off_at) {
            held[index].off_at = until;
        }
        return;
    }

    int note = FIRST_NOTE + index + 12 * app.octave;
    if (note < 0 || note > 127) {
        return;
    }
    held[index].note = note;
    held[index].track = app.track;
    held[index].off_at = now + TAP_LENGTH;
    desktop_send(EV_NOTE_ON, (uint8_t)app.track, (uint8_t)note, 100, 0, 0.0f);
}

void update_note_timers(double now)
{
    if (!initialised) {
        init_held();
    }
    for (int i = 0; i < NOTE_KEYS; i++) {
        if (held[i].note >= 0 && now >= held[i].off_at) {
            desktop_send(EV_NOTE_OFF, (uint8_t)held[i].track, (uint8_t)held[i].note, 0, 0, 0.0f);
            held[i].note = -1;
        }
    }
}

/* ------------------------------------------------------------ controls */

static float clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

static void change_knob(float delta)
{
    int k = app.selected_knob;

    if (k < 4) {
        /* the four macros of the track's engine (synth: CUTOFF RESO ENV FX, drums: TUNE DECAY CUTOFF DRIVE) */
        uint16_t id = app.live.track[app.track].engine.api->macros[k];
        float value = clamp01(desktop_knob_value(k) + delta);           /* a float read: harmless race */
        desktop_send(EV_PARAM, (uint8_t)app.track, 0, 0, id, value);
    } else if (k == 4) {
        if (app.scene_prev < 0) {
            say("MORPH: recall two scenes first (W stores, Q recalls)");
            return;
        }
        app.morph = clamp01(app.morph + delta);
        desktop_live(LIVE_MORPH, (uint8_t)app.scene_prev, (uint16_t)app.scene_last, app.morph);
    } else if (k == 5) {
        app.swing = clamp01(app.swing + delta);
        desktop_live(LIVE_SWING, 0, 0, app.swing);
    } else if (k == 6) {
        app.bpm += delta > 0.0f ? 1.0f : -1.0f;
        app.bpm = app.bpm < 20.0f ? 20.0f : (app.bpm > 300.0f ? 300.0f : app.bpm);
        desktop_live(LIVE_TEMPO, 0, 0, app.bpm);
    } else if (k == 7) {
        float gain = app.live.track[app.track].gain + delta;
        desktop_live(LIVE_GAIN, 0, 0, gain < 0.0f ? 0.0f : (gain > 1.5f ? 1.5f : gain));
    } else if (k < 12) {
        desktop_live(LIVE_MACRO, (uint8_t)(k - 8), 0, clamp01(app.live.macro[app.track][k - 8] + delta));
    } else {
        app.pump = clamp01(app.pump + delta);
        desktop_live(LIVE_PUMP, 0, 0, app.pump);
    }
}

static void change_preset(int step)
{
    int t = app.track;
    app.preset[t] = (app.preset[t] + OLS_SOUND_COUNT + step) % OLS_SOUND_COUNT;
    desktop_send(EV_PRESET, (uint8_t)t, (uint8_t)app.preset[t], 0, 0, 0.0f);
    snprintf(message, sizeof message, "%s", ols_sound_name(app.preset[t]));
}

static void select_track(int step)
{
    app.track = (app.track + TRACKS + step) % TRACKS;
    desktop_live(LIVE_SELECT, (uint8_t)app.track, 0, 0.0f);
    snprintf(message, sizeof message, "track %d", app.track + 1);
}

static void creative(ols_creative_op_t op, int arg)
{
    desktop_live(LIVE_CREATIVE, (uint8_t)op, (uint16_t)(int16_t)arg, CREATIVE_AMOUNT);
    snprintf(message, sizeof message, "%s", CREATIVE_NAMES[op]);
}

static void scene_store(void)
{
    int slot = app.scene_next % OLS_SCENES;
    desktop_live(LIVE_SCENE_STORE, (uint8_t)slot, 0, 0.0f);
    app.scene_next = slot + 1;
    snprintf(message, sizeof message, "scene %d stored", slot + 1);
}

static void scene_recall_next(void)
{
    int count = app.scene_next < OLS_SCENES ? app.scene_next : OLS_SCENES;
    if (count == 0) {
        say("no scene yet: W stores one");
        return;
    }
    int slot = (app.scene_last + 1) % count;
    desktop_live(LIVE_SCENE_RECALL, (uint8_t)slot, 0, 0.0f);
    app.scene_prev = app.scene_last;
    app.scene_last = slot;
    app.morph = 1.0f;
    snprintf(message, sizeof message, "scene %d", slot + 1);
}

void print_help(void)
{
    printf("\n");
    printf("  Notes      %s   (C4 to E5)\n", app.azerty ? "q z s e d f t g y h u j k o l p m" : "a w s e d f t g y h u j k o l p ;");
    printf("  b / n      track -/+     v  mute        Space  play/stop   r  record    c  CAPTURE\n");
    printf("  x / X      undo / redo   i  quantize    !      panic       K  save the set\n");
    printf("  Up/Down    octave        Left/Right  sound: 17 synths, 4 drum kits (drums: C kick, D snare, F# hat...)\n");
    printf("  Tab        next control  + / -       change it (macros, MORPH SWING TEMPO LEVEL ENERGY SPACE TENSION DENSITY PUMP)\n");
    printf("  Creative   M mutate H humanize F fill S simplify R reverse E expand C contract\n");
    printf("             P chance T ratchet < > rotate U/D transpose G euclid\n");
    printf("  Scenes     W store   Q recall next (MORPH glides between the last two)\n");
    printf("  L          metronome (REC from stop counts one bar in)   A  tap tempo\n");
    printf("  h help     Esc quit  A MIDI keyboard plays the selected track (see --list and --midi).\n\n");
}

/* Shift + letter: the Creative Engine. Returns 1 if the key was one. */
static int creative_key(int key)
{
    switch (key) {
    case 'M': creative(CREATIVE_MUTATE, 0); return 1;
    case 'H': creative(CREATIVE_HUMANIZE, 0); return 1;
    case 'F': creative(CREATIVE_FILL, 0); return 1;
    case 'S': creative(CREATIVE_SIMPLIFY, 0); return 1;
    case 'R': creative(CREATIVE_REVERSE, 0); return 1;
    case 'E': creative(CREATIVE_EXPAND, 0); return 1;
    case 'C': creative(CREATIVE_CONTRACT, 0); return 1;
    case 'P': creative(CREATIVE_CHANCE, 0); return 1;
    case 'T': creative(CREATIVE_RATCHET, 0); return 1;
    case '<': creative(CREATIVE_ROTATE, -1); return 1;
    case '>': creative(CREATIVE_ROTATE, 1); return 1;
    case 'U': creative(CREATIVE_TRANSPOSE, 1); return 1;
    case 'D': creative(CREATIVE_TRANSPOSE, -1); return 1;
    case 'G':
        desktop_live(LIVE_EUCLID, (uint8_t)app.euclid_hits, 16, 0.0f);
        snprintf(message, sizeof message, "euclid %d/16", app.euclid_hits);
        app.euclid_hits = app.euclid_hits >= 9 ? 2 : app.euclid_hits + 1;
        return 1;
    default:
        return 0;
    }
}

int handle_key(int key)
{
    if (!initialised) {
        init_held();
    }

    int index = note_index(key);
    if (index >= 0) {
        play_key(index);
        return 1;
    }
    if (creative_key(key)) {
        return 1;
    }

    switch (key) {
    case KEY_ESC:
        return 0;
    case KEY_UP:
        if (app.octave < 3) app.octave++;
        break;
    case KEY_DOWN:
        if (app.octave > -3) app.octave--;
        break;
    case KEY_RIGHT:
        change_preset(1);
        break;
    case KEY_LEFT:
        change_preset(-1);
        break;
    case KEY_TAB:
        app.selected_knob = (app.selected_knob + 1) % KNOB_COUNT;
        break;
    case '+':
    case '=':
        change_knob(KNOB_STEP);
        break;
    case '-':
    case '_':
        change_knob(-KNOB_STEP);
        break;
    case 'b':
        select_track(-1);
        break;
    case 'n':
        select_track(1);
        break;
    case 'v':
        desktop_live(LIVE_MUTE_TOGGLE, (uint8_t)app.track, 0, 0.0f);
        say(app.live.track[app.track].mute ? "unmuted" : "muted");
        break;
    case ' ':
        desktop_live(LIVE_PLAY_TOGGLE, 0, 0, 0.0f);
        break;
    case 'r':
    case KEY_ENTER:
        desktop_live(LIVE_RECORD_TOGGLE, 0, 0, 0.0f);
        break;
    case 'c':
        desktop_live(LIVE_CAPTURE, 0, 0, 0.0f);
        say("CAPTURE");
        break;
    case 'x':
        desktop_send(EV_LIVE, OLS_TRACK_ALL, LIVE_UNDO, 0, 0, 0.0f);     /* the newest step, any track */
        say("undo");
        break;
    case 'X':
        desktop_send(EV_LIVE, OLS_TRACK_ALL, LIVE_REDO, 0, 0, 0.0f);
        say("redo");
        break;
    case 'i':
        desktop_live(LIVE_QUANTIZE, (uint8_t)!app.live.quantize, 0, 0.0f);
        say(app.live.quantize ? "quantize off" : "quantize 1/16");
        break;
    case '!':
        desktop_send(EV_LIVE, OLS_TRACK_ALL, LIVE_STUTTER, 0, 1, 0.0f);   /* id 1: at once */
        desktop_send(EV_ALL_OFF, 0, 0, 0, 0, 0.0f);
        say("panic");
        break;
    case 'W':
        scene_store();
        break;
    case 'Q':
        scene_recall_next();
        break;
    case 'K':
        say(desktop_save_set(app.set_path) ? "live set saved" : "could not save the set");
        break;
    case 'L':
        desktop_live(LIVE_METRONOME, (uint8_t)!app.live.metronome, 0, 0.0f);
        say(app.live.metronome ? "metronome off" : "metronome on");
        break;
    case 'A':
        desktop_live(LIVE_TAP, 0, 0, 0.0f);
        say("tap");
        break;
    case 'h':
    case '?':
        print_help();
        break;
    default:
        break;
    }
    return 1;
}
