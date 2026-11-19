/*
 * OpenVALive - DSP core
 * phaser.h : a classic 4-stage phaser (think MXR Phase 90 / Small Stone).
 *
 * Four first-order all-pass filters in a row shift the phase of the sound;
 * mixed with the dry sound, they dig moving notches. A little feedback makes
 * the sweep sing. The LFO moves the all-pass corner between ~150 Hz and a
 * few kHz.
 *
 * Cost: the corner (one tan) is set at control rate by phaser_set_corner;
 * per sample: 4 multiply-adds and the feedback. Mono in, mono out.
 */
#ifndef DSP_PHASER_H
#define DSP_PHASER_H

#include "dsp_common.h"

#define PHASER_STAGES 4

typedef struct {
    float s[PHASER_STAGES];     /* all-pass states */
    float a;                    /* all-pass coefficient, from the corner frequency */
    float last;                 /* last wet output, for the feedback */
    float feedback;             /* 0..0.9 */
} dsp_phaser_t;

static inline void phaser_init(dsp_phaser_t *p)
{
    for (int i = 0; i < PHASER_STAGES; i++) {
        p->s[i] = 0.0f;
    }
    p->a = 0.0f;
    p->last = 0.0f;
    p->feedback = 0.6f;
}

/* Corner frequency of the all-pass stages (control rate: uses tan). */
static inline void phaser_set_corner(dsp_phaser_t *p, float hz)
{
    float t = dsp_tanf(DSP_PI * dsp_clampf(hz, 20.0f, 0.4f / DSP_INV_SR) * DSP_INV_SR);
    p->a = (t - 1.0f) / (t + 1.0f);
}

/* Returns the wet (phase-shifted) signal; mix it with the dry one. */
static inline float phaser_process(dsp_phaser_t *p, float x)
{
    float y = x + p->last * p->feedback;
    for (int i = 0; i < PHASER_STAGES; i++) {
        /* first-order all-pass: y = a*x + x[n-1] - a*y[n-1], with one state */
        float out = p->a * y + p->s[i];
        p->s[i] = y - p->a * out;
        y = out;
    }
    p->last = y;
    return y;
}

#endif /* DSP_PHASER_H */
