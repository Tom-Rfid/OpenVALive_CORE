/*
 * OpenVALive desktop - shared declarations.
 *
 * A small program that plays OpenVALive live on a computer (Windows,
 * Linux, macOS): the same Live Core as the FM-1 (four tracks, loops,
 * capture, undo, scenes, Creative Engine), sound through the default audio
 * device, notes from a MIDI keyboard (an SMK-37, an FM-1 over USB...) or
 * from the computer keyboard.
 *
 * Threads:
 *   main thread   keyboard, commands, status line    -> event queue  -> audio
 *   MIDI thread   MIDI input device                  -> byte ring    -> audio
 *   audio thread  miniaudio callback: applies both, renders the Live Core
 */
#ifndef OLS_DESKTOP_H
#define OLS_DESKTOP_H

/* desktop CPUs: the queue's barrier must be a real memory barrier */
#define OLS_BARRIER() __sync_synchronize()

#include "../../core/engines/va/engine_va.h"
#include "../../core/engines/drum/engine_drum.h"
#include "../../core/engines/bank.h"
#include "../../core/live/event_queue.h"
#include "../../core/live/live.h"
#include "../../core/live/midi_in.h"
#include "byte_ring.h"

#define DESKTOP_DELAY_LEN 32768u   /* 683 ms at 48 kHz */
#define DESKTOP_MAX_BLOCK 512      /* frames rendered per internal chunk */
#define TRACKS OLS_TRACKS
#define KNOB_COUNT 13              /* Tab cycles them, + / - change them */

typedef struct {
    /* audio side */
    engine_va_t va[TRACKS];
    engine_drum_t drum[TRACKS];
    ols_slot_t slots[TRACKS];      /* the engines each track can switch between (bank.h) */
    ols_live_t live;
    midi_in_t midi;
    ols_engine_t live_input;       /* MIDI in -> the selected track */
    ols_evq_t events;              /* main thread -> audio thread */
    byte_ring_t midi_bytes;        /* MIDI thread -> audio thread */
    float sample_rate;
    volatile float cpu_load;       /* 0..1, written by the audio thread */
    volatile int voices_sounding;  /* written by the audio thread */
    volatile uint32_t frames_rendered;

    /* user interface (main thread only) */
    int track;                     /* selected track 0..3 */
    int preset[TRACKS];            /* sound of each track (bank.h numbers) */
    int start_preset;              /* --preset for track 1, -1 = the default band */
    int octave;
    int selected_knob;
    int azerty;
    int euclid_hits;
    int scene_next, scene_prev, scene_last;
    float morph, swing, bpm, pump;
    const char *midi_name;
    const char *set_path;          /* the live set file (K saves it, loaded at start) */
} desktop_t;

extern desktop_t app;

extern const char *const KNOB_NAME[KNOB_COUNT];
const char *knob_name(int knob);                        /* the macro names of the track's engine */

/* engine_bridge.c */
void desktop_init_synth(float sample_rate);
void desktop_render(float *interleaved, int frames);   /* audio thread */
void desktop_send(uint8_t type, uint8_t target, uint8_t a, uint8_t b, uint16_t id, float v);
void desktop_live(ols_live_cmd_t cmd, uint8_t b, uint16_t id, float v);   /* on the selected track */
float desktop_knob_value(int knob);                     /* 0..1, for the status line */
int  desktop_save_set(const char *path);
int  desktop_load_set(const char *path);                /* before audio starts */

/* audio_device.c */
int  audio_device_start(float requested_rate);
void audio_device_stop(void);

/* render_offline.c */
int  render_to_wav(const char *path, float seconds, int with_demo);

/* midi_input_*.c (one per OS) */
void midi_input_list(void);
int  midi_input_open(const char *which);   /* 0 = first device; returns 1 if opened */
void midi_input_close(void);

/* keyboard_input.c */
enum {
    KEY_NONE = 0,
    KEY_ESC = 27,
    KEY_TAB = 9,
    KEY_ENTER = 13,
    KEY_UP = 1000,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT
};
void keyboard_begin(void);
void keyboard_end(void);
int  keyboard_poll(void);                  /* KEY_NONE when nothing was pressed */

/* key_commands.c */
int  handle_key(int key);                  /* returns 0 to quit */
void update_note_timers(double now);
void print_help(void);

/* status_line.c */
void status_line_print(void);

/* timing.c */
double time_now(void);                     /* seconds, monotonic */
void   sleep_ms(int ms);

#endif /* OLS_DESKTOP_H */
