/*
 * OpenVALive - simple C API (the universal "connector")
 *
 * The easiest way to use the OpenVALive synth in any project: a plugin
 * (VST/CLAP/JUCE), Daisy Seed, Teensy, RP2350, ESP32, a desktop app...
 * One header, no dynamic allocation, no hardware dependency.
 *
 *   static int16_t delay_ram[16384];
 *   static ols_synth_t synth;
 *
 *   ols_synth_init(&synth, 48000.0f, delay_ram, 16384);   // once
 *   ols_synth_midi(&synth, bytes, n);                       // incoming MIDI bytes
 *   ols_synth_render_f32(&synth, left, right, frames);      // in the audio callback
 *
 * Files to compile: core/api/ols_synth.c, core/engines/va/engine_va*.c,
 * core/live/midi_in.c, core/dsp/dsp_runtime.c (run-time sample rate).
 *
 * Threads: call every function from the SAME thread as the audio rendering
 * (or protect the calls yourself). To drive the synth from a UI thread, use
 * core/live/event_queue.h, as the FM-1 and desktop apps do.
 *
 * Current limit: one sample rate per program (a DSP Core global). Several
 * instances are fine if they all run at the same rate.
 */
#ifndef OLS_SYNTH_H
#define OLS_SYNTH_H

#include <stdint.h>
#include "../engines/va/engine_va.h"
#include "../live/midi_in.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OLS_API_VERSION 1

typedef struct {
    engine_va_t va;
    ols_engine_t engine;
    midi_in_t midi;
    float out_gain;
} ols_synth_t;

/* sample_rate: Hz (ignored when the DSP Core is built with a fixed rate).
 * delay_buf / delay_len: RAM for the delay (int16); 0 / 0 = no delay.
 * 16384 samples = 341 ms at 48 kHz = 32 KB. */
void ols_synth_init(ols_synth_t *s, float sample_rate, int16_t *delay_buf, uint32_t delay_len);

/* Raw MIDI bytes (DIN, UART, or already unpacked from USB). All channels. */
void ols_synth_midi(ols_synth_t *s, const uint8_t *bytes, int len);
/* 4-byte USB-MIDI event packet (CIN + 3 bytes). */
void ols_synth_usb_midi(ols_synth_t *s, const uint8_t packet[4]);

void ols_synth_note_on(ols_synth_t *s, uint8_t note, uint8_t velocity);
void ols_synth_note_off(ols_synth_t *s, uint8_t note);
void ols_synth_all_notes_off(ols_synth_t *s);

/* Parameters are normalised 0..1: see va_param_t in engine_va.h. */
void ols_synth_set_param(ols_synth_t *s, uint16_t id, float value01);
float ols_synth_get_param(const ols_synth_t *s, uint16_t id);
uint16_t ols_synth_param_count(void);
const char *ols_synth_param_name(uint16_t id);
void ols_synth_load_preset(ols_synth_t *s, va_preset_t preset);
/* Maps a MIDI CC to a parameter (74, 71, 73, 72, 5, 1, 93, 94 are mapped by default). */
void ols_synth_map_cc(ols_synth_t *s, uint8_t cc, uint16_t param_id);

/* Linear output gain (1.0 by default). */
void ols_synth_set_gain(ols_synth_t *s, float gain);

/* Renders n frames, output in [-1, 1]. */
void ols_synth_render_f32(ols_synth_t *s, float *left, float *right, int n);
/* Interleaved int16 rendering (L,R,L,R...) for common DACs / I2S.
 * scratch: 2*n floats of work memory provided by the caller. */
void ols_synth_render_i16(ols_synth_t *s, int16_t *interleaved, int n, float *scratch);

#ifdef __cplusplus
}
#endif

#endif /* OLS_SYNTH_H */
