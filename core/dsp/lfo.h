/*
 * OpenVALive - DSP Core
 * lfo.h : low-frequency oscillator, ticked once per control block.
 * Shapes: sine, triangle, square, sample & hold. Output in [-1, 1].
 */
#ifndef DSP_LFO_H
#define DSP_LFO_H

#include "dsp_common.h"

typedef enum {
    LFO_SINE = 0,
    LFO_TRIANGLE,
    LFO_SQUARE,
    LFO_SH,          /* sample & hold: a new random value each cycle */
    LFO_WAVE_COUNT
} lfo_wave_t;

typedef struct {
    float phase;
    float inc;           /* per control tick */
    float sh_value;
    uint32_t rng;
    lfo_wave_t wave;
} dsp_lfo_t;

static inline void lfo_init(dsp_lfo_t *l, uint32_t seed)
{
    l->phase = 0.0f;
    l->inc = 0.0f;
    l->sh_value = 0.0f;
    l->rng = seed ? seed : 0x1234567u;
    l->wave = LFO_SINE;
}

static inline void lfo_set_rate(dsp_lfo_t *l, float hz)
{
    l->inc = hz / DSP_CTRL_RATE;
}

static inline void lfo_reset(dsp_lfo_t *l)
{
    l->phase = 0.0f;
}

/* Parabolic sine approximation of sin(2*pi*phase), accurate to ~0.1 %. */
static inline float lfo_sine(float phase)
{
    float x = 2.0f * phase - 1.0f;                 /* -1..1 */
    float y = 4.0f * x * (1.0f - dsp_fabsf(x));    /* rough sin(pi*x) */
    y = 0.225f * (y * dsp_fabsf(y) - y) + y;       /* refinement */
    return -y;                                     /* sin(pi*x) = -sin(2*pi*phase) */
}

/* The value at the current phase. */
static inline float lfo_shape(const dsp_lfo_t *l)
{
    switch (l->wave) {
    case LFO_SINE:
        return lfo_sine(l->phase);
    case LFO_TRIANGLE:
        return 1.0f - 4.0f * dsp_fabsf(l->phase - 0.5f);
    case LFO_SQUARE:
        return (l->phase < 0.5f) ? 1.0f : -1.0f;
    case LFO_SH:
    default:
        return l->sh_value;
    }
}

/* Advances by one control tick and returns the new value. */
static inline float lfo_tick(dsp_lfo_t *l)
{
    l->phase += l->inc;
    if (l->phase >= 1.0f) {
        l->phase -= 1.0f;
        l->sh_value = dsp_noise(&l->rng);
    }
    return lfo_shape(l);
}

/* Jumps to a phase given from outside (an LFO locked to the tempo): a new cycle draws a new S&H value. */
static inline float lfo_follow(dsp_lfo_t *l, float phase)
{
    if (phase < l->phase) {
        l->sh_value = dsp_noise(&l->rng);
    }
    l->phase = phase;
    return lfo_shape(l);
}

#endif /* DSP_LFO_H */
