/*
 * OpenVALive - DSP Core
 * svf.h : zero-delay-feedback state variable filter (LP / BP / HP / Notch).
 *
 * Trapezoidal-integration SVF as described by Andrew Simper (Cytomic,
 * "SvfLinearTrapOptimised2") and Vadim Zavalishin ("The Art of VA Filter
 * Design"). One algorithm gives all four responses, it stays stable under
 * fast cutoff modulation, and costs ~12 flops per sample.
 *
 * Usage, per block:   svf_set(&f, cutoff_hz, resonance01);   (uses tan: control rate only)
 * Usage, per sample:  y = svf_process(&f, x, SVF_LP);
 */
#ifndef DSP_SVF_H
#define DSP_SVF_H

#include "dsp_common.h"

typedef enum {
    SVF_LP = 0,
    SVF_BP,
    SVF_HP,
    SVF_NOTCH,
    SVF_MODE_COUNT
} svf_mode_t;

typedef struct {
    float ic1, ic2;      /* integrator states */
    float k;             /* damping = 1/Q */
    float a1, a2, a3;    /* precomputed coefficients */
} dsp_svf_t;

static inline void svf_reset(dsp_svf_t *f)
{
    f->ic1 = 0.0f;
    f->ic2 = 0.0f;
}

/* cutoff_hz is clamped to [16 Hz, 0.45 * sample rate].
 * resonance: 0 = no resonance, 1 = close to self-oscillation. */
static inline void svf_set(dsp_svf_t *f, float cutoff_hz, float resonance)
{
    float fc = dsp_clampf(cutoff_hz, 16.0f, 0.45f * DSP_SR);
    float g = dsp_tanf(DSP_PI * fc * DSP_INV_SR);

    f->k = 2.0f - 1.96f * dsp_clampf(resonance, 0.0f, 1.0f);   /* k from 2 down to 0.04 */
    f->a1 = 1.0f / (1.0f + g * (g + f->k));
    f->a2 = g * f->a1;
    f->a3 = g * f->a2;
}

static inline float svf_process(dsp_svf_t *f, float input, svf_mode_t mode)
{
    float v3 = input - f->ic2;
    float band = f->a1 * f->ic1 + f->a2 * v3;
    float low = f->ic2 + f->a2 * f->ic1 + f->a3 * v3;

    f->ic1 = 2.0f * band - f->ic1;
    f->ic2 = 2.0f * low - f->ic2;

    switch (mode) {
    case SVF_LP:
        return low;
    case SVF_BP:
        return band;
    case SVF_HP:
        return input - f->k * band - low;
    case SVF_NOTCH:
    default:
        return input - f->k * band;
    }
}

#endif /* DSP_SVF_H */
