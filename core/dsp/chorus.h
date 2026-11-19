/*
 * OpenVALive - DSP Core
 * chorus.h : stereo chorus inspired by the Roland Juno-60 / Juno-106.
 *
 * Two delay lines modulated by a triangle LFO in opposite phase, one per side.
 * Modes (published Juno-60 values):
 *   I    : 0.513 Hz, delay 1.66 .. 5.35 ms
 *   II   : 0.863 Hz, delay 1.66 .. 5.35 ms
 *   I+II : 9.75 Hz,  delay 3.3 .. 3.7 ms   (fast vibrato)
 * RAM: 512 floats = 2 KB.
 */
#ifndef DSP_CHORUS_H
#define DSP_CHORUS_H

#include "dsp_common.h"

#define CHORUS_SIZE 512   /* power of two, >= 10 ms at 48 kHz */

typedef enum {
    CHORUS_OFF = 0,
    CHORUS_I,
    CHORUS_II,
    CHORUS_I_II,
    CHORUS_MODE_COUNT
} chorus_mode_t;

typedef struct {
    float buf[CHORUS_SIZE];
    uint32_t write_pos;
    float lfo_phase;
    float lfo_inc;
    float center;        /* centre delay, in samples */
    float depth;         /* modulation depth, in samples */
    float mix;           /* 0 = dry, 1 = wet */
    chorus_mode_t mode;
} dsp_chorus_t;

static inline void chorus_set_mode(dsp_chorus_t *c, chorus_mode_t mode)
{
    c->mode = mode;

    switch (mode) {
    case CHORUS_I:
        c->lfo_inc = 0.513f * DSP_INV_SR;
        c->center = 3.505e-3f * DSP_SR;
        c->depth = 1.845e-3f * DSP_SR;
        break;
    case CHORUS_II:
        c->lfo_inc = 0.863f * DSP_INV_SR;
        c->center = 3.505e-3f * DSP_SR;
        c->depth = 1.845e-3f * DSP_SR;
        break;
    case CHORUS_I_II:
        c->lfo_inc = 9.75f * DSP_INV_SR;
        c->center = 3.5e-3f * DSP_SR;
        c->depth = 0.2e-3f * DSP_SR;
        break;
    default:
        break;
    }

    /* Above ~90 kHz the 512-sample buffer is too short: shrink the delays. */
    float max_delay = (float)(CHORUS_SIZE - 3);
    if (c->center + c->depth > max_delay) {
        float k = max_delay / (c->center + c->depth);
        c->center *= k;
        c->depth *= k;
    }
}

static inline void chorus_init(dsp_chorus_t *c)
{
    for (int i = 0; i < CHORUS_SIZE; i++) {
        c->buf[i] = 0.0f;
    }
    c->write_pos = 0;
    c->lfo_phase = 0.0f;
    c->mix = 0.5f;
    chorus_set_mode(c, CHORUS_I);
}

static inline float chorus_read(const dsp_chorus_t *c, float delay_samples)
{
    float read_pos = (float)c->write_pos - delay_samples;
    if (read_pos < 0.0f) {
        read_pos += (float)CHORUS_SIZE;
    }
    uint32_t i0 = (uint32_t)read_pos & (CHORUS_SIZE - 1);
    uint32_t i1 = (i0 + 1) & (CHORUS_SIZE - 1);
    float frac = read_pos - dsp_floorf(read_pos);
    return c->buf[i0] + frac * (c->buf[i1] - c->buf[i0]);
}

/* Mono in, stereo out. */
static inline void chorus_process(dsp_chorus_t *c, float input, float *out_l, float *out_r)
{
    if (c->mode == CHORUS_OFF) {
        *out_l = input;
        *out_r = input;
        return;
    }

    c->buf[c->write_pos] = input;

    c->lfo_phase += c->lfo_inc;
    if (c->lfo_phase >= 1.0f) {
        c->lfo_phase -= 1.0f;
    }
    float tri = 1.0f - 4.0f * dsp_fabsf(c->lfo_phase - 0.5f);

    float wet_l = chorus_read(c, c->center + c->depth * tri);
    float wet_r = chorus_read(c, c->center - c->depth * tri);

    c->write_pos = (c->write_pos + 1) & (CHORUS_SIZE - 1);

    float dry = 1.0f - 0.5f * c->mix;
    *out_l = input * dry + wet_l * c->mix;
    *out_r = input * dry + wet_r * c->mix;
}

#endif /* DSP_CHORUS_H */
