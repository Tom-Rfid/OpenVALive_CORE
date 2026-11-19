/*
 * OpenVALive - DSP Core
 * osc.h : band-limited oscillators (saw, square with PWM, triangle).
 *
 * PolyBLEP: a small polynomial correction applied around each discontinuity
 * of the naive waveform. It removes most of the aliasing for ~10 flops per
 * sample and no tables.
 * Reference: Valimaki & Huovilainen, "Antialiasing Oscillators in
 * Subtractive Synthesis", IEEE Signal Processing Magazine, 2007.
 *
 * Usage, per sample:   osc_advance(&o);  out = osc_value(&o, OSC_SAW);
 * Usage, per block:    osc_set_freq(&o, hz);
 */
#ifndef DSP_OSC_H
#define DSP_OSC_H

#include "dsp_common.h"

typedef enum {
    OSC_SAW = 0,
    OSC_SQUARE,
    OSC_TRIANGLE,
    OSC_WAVE_COUNT
} osc_wave_t;

typedef struct {
    float phase;   /* 0..1 */
    float inc;     /* phase increment per sample = frequency / sample rate */
    float pw;      /* pulse width of the square wave, 0.05..0.95 */
} dsp_osc_t;

static inline void osc_init(dsp_osc_t *o, float start_phase)
{
    o->phase = start_phase;
    o->inc = 0.0f;
    o->pw = 0.5f;
}

/* Call at control rate. The increment is limited below Nyquist. */
static inline void osc_set_freq(dsp_osc_t *o, float hz)
{
    o->inc = dsp_clampf(hz * DSP_INV_SR, 0.0f, 0.45f);
}

/* PolyBLEP residual. t = phase (0..1), dt = phase increment. */
static inline float dsp_polyblep(float t, float dt)
{
    if (t < dt) {
        /* just after the discontinuity */
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        /* just before the discontinuity */
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

/* Advances the phase by one sample.
 * Returns 1 when the phase wrapped (useful for hard sync or a derived sub). */
static inline int osc_advance(dsp_osc_t *o)
{
    o->phase += o->inc;
    if (o->phase >= 1.0f) {
        o->phase -= 1.0f;
        return 1;
    }
    return 0;
}

static inline float osc_saw(const dsp_osc_t *o)
{
    float naive = 2.0f * o->phase - 1.0f;
    return naive - dsp_polyblep(o->phase, o->inc);
}

static inline float osc_square(const dsp_osc_t *o)
{
    float naive = (o->phase < o->pw) ? 1.0f : -1.0f;

    /* the falling edge sits at phase = pw */
    float falling_phase = o->phase - o->pw;
    if (falling_phase < 0.0f) {
        falling_phase += 1.0f;
    }

    return naive
         + dsp_polyblep(o->phase, o->inc)
         - dsp_polyblep(falling_phase, o->inc);
}

/* Naive triangle: its harmonics fall as 1/n^2, aliasing is negligible. */
static inline float osc_triangle(const dsp_osc_t *o)
{
    return 1.0f - 4.0f * dsp_fabsf(o->phase - 0.5f);
}

/* Current value (call after osc_advance). Output roughly in [-1, 1]. */
static inline float osc_value(const dsp_osc_t *o, osc_wave_t wave)
{
    switch (wave) {
    case OSC_SAW:
        return osc_saw(o);
    case OSC_SQUARE:
        return osc_square(o);
    case OSC_TRIANGLE:
    default:
        return osc_triangle(o);
    }
}

#endif /* DSP_OSC_H */
