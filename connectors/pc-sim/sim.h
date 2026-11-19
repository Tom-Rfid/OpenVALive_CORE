/*
 * OpenVALive - PC simulator, shared declarations.
 *
 * Plays the engine through the REAL path (MIDI bytes -> parser -> engine ->
 * DSP), writes WAV files and measures CPU and RAM, so the sound can be
 * developed and checked without flashing the FM-1.
 */
#ifndef OLS_SIM_H
#define OLS_SIM_H

#include "../../core/engines/va/engine_va.h"
#include "../../core/live/midi_in.h"

#define SIM_SR 48000        /* simulator sample rate (the DSP Core accepts any) */
#define SIM_DELAY_LEN 18000 /* 375 ms at 48 kHz, int16 = 36 KB */

/* wav.c */
int  wav_write(const char *path, const float *left, const float *right, int frames, int sample_rate);
void signal_report(const float *left, const float *right, int frames);

/* demo.c, sweep.c, bench.c */
int  sim_demo(const char *path);
int  sim_sweep(const char *path);
void sim_bench(void);

#endif
