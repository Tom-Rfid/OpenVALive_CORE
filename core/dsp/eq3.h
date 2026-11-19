/* SPDX-License-Identifier: MIT
 * OpenVALive - a three-band tone control: a low shelf (120 Hz), a peak in the middle (its frequency chosen,
 * 250 Hz .. 4 kHz) and a high shelf (8 kHz), +-12 dB each. Stereo, for the master mix.
 *
 * Biquads in transposed direct form II, the coefficients from the bilinear transform (Zoelzer's shelving and peak
 * formulas, K = tan(pi f / fs)). eq3_set() is control rate (tan, pow); eq3_process() is a few multiplies a sample,
 * and nothing at all while every band is flat.
 */
#ifndef OLS_EQ3_H
#define OLS_EQ3_H

#include "dsp_common.h"

typedef struct {
    float b0, b1, b2, a1, a2;
    float z1[2], z2[2];                         /* left, right */
} eq_biquad_t;

typedef struct {
    eq_biquad_t band[3];
    uint8_t flat;                               /* every gain 0 dB: the mix goes through untouched */
} eq3_t;

static inline void eq_biquad_norm(eq_biquad_t *f, float b0, float b1, float b2, float a0, float a1, float a2)
{
    float n = 1.0f / a0;
    f->b0 = b0 * n;
    f->b1 = b1 * n;
    f->b2 = b2 * n;
    f->a1 = a1 * n;
    f->a2 = a2 * n;
}

/* the square root of x > 0 (control rate only) */
static inline float eq_sqrt(float x)
{
    return dsp_powf(x, 0.5f);
}

/* a shelf (high = 0 low, 1 high) of `db` at `hz` */
static inline void eq_shelf(eq_biquad_t *f, int high, float hz, float db, float fs)
{
    float k = dsp_tanf(DSP_PI * hz / fs), kk = k * k;
    float v = dsp_powf(10.0f, dsp_fabsf(db) * 0.05f);
    float r2 = 1.41421356f, r2v = eq_sqrt(2.0f * v);
    if (!high && db >= 0.0f) {
        eq_biquad_norm(f, 1.0f + r2v * k + v * kk, 2.0f * (v * kk - 1.0f), 1.0f - r2v * k + v * kk,
                       1.0f + r2 * k + kk, 2.0f * (kk - 1.0f), 1.0f - r2 * k + kk);
    } else if (!high) {
        eq_biquad_norm(f, 1.0f + r2 * k + kk, 2.0f * (kk - 1.0f), 1.0f - r2 * k + kk,
                       1.0f + r2v * k + v * kk, 2.0f * (v * kk - 1.0f), 1.0f - r2v * k + v * kk);
    } else if (db >= 0.0f) {
        eq_biquad_norm(f, v + r2v * k + kk, 2.0f * (kk - v), v - r2v * k + kk,
                       1.0f + r2 * k + kk, 2.0f * (kk - 1.0f), 1.0f - r2 * k + kk);
    } else {
        eq_biquad_norm(f, 1.0f + r2 * k + kk, 2.0f * (kk - 1.0f), 1.0f - r2 * k + kk,
                       v + r2v * k + kk, 2.0f * (kk - v), v - r2v * k + kk);
    }
}

/* a peak of `db` at `hz`, width q */
static inline void eq_peak(eq_biquad_t *f, float hz, float db, float q, float fs)
{
    float k = dsp_tanf(DSP_PI * hz / fs), kk = k * k;
    float v = dsp_powf(10.0f, dsp_fabsf(db) * 0.05f);
    if (db >= 0.0f) {
        eq_biquad_norm(f, 1.0f + v * k / q + kk, 2.0f * (kk - 1.0f), 1.0f - v * k / q + kk,
                       1.0f + k / q + kk, 2.0f * (kk - 1.0f), 1.0f - k / q + kk);
    } else {
        eq_biquad_norm(f, 1.0f + k / q + kk, 2.0f * (kk - 1.0f), 1.0f - k / q + kk,
                       1.0f + v * k / q + kk, 2.0f * (kk - 1.0f), 1.0f - v * k / q + kk);
    }
}

static inline void eq3_init(eq3_t *e)
{
    for (int b = 0; b < 3; b++) {
        e->band[b].b0 = 1.0f;
        e->band[b].b1 = e->band[b].b2 = e->band[b].a1 = e->band[b].a2 = 0.0f;
        e->band[b].z1[0] = e->band[b].z1[1] = e->band[b].z2[0] = e->band[b].z2[1] = 0.0f;
    }
    e->flat = 1;
}

/* the knobs, 0..1 (0.5 flat): bass, mid, treble (+-12 dB), the mid's frequency (250 Hz .. 4 kHz) */
static inline void eq3_set(eq3_t *e, float bass, float mid, float treble, float mid_hz01, float fs)
{
    float db[3] = { (bass - 0.5f) * 24.0f, (mid - 0.5f) * 24.0f, (treble - 0.5f) * 24.0f };
    int flat = dsp_fabsf(db[0]) < 0.2f && dsp_fabsf(db[1]) < 0.2f && dsp_fabsf(db[2]) < 0.2f;
    if (flat && !e->flat) {
        eq3_init(e);                            /* back to flat: the filters' memory starts clean next time */
        return;
    }
    eq_shelf(&e->band[0], 0, 120.0f, db[0], fs);
    eq_peak(&e->band[1], dsp_expmap(mid_hz01, 250.0f, 4000.0f), db[1], 0.8f, fs);
    eq_shelf(&e->band[2], 1, 8000.0f, db[2], fs);
    e->flat = (uint8_t)flat;
}

static inline void eq3_process(eq3_t *e, float *l, float *r, uint32_t n)
{
    if (e->flat) {
        return;
    }
    for (int b = 0; b < 3; b++) {
        eq_biquad_t *f = &e->band[b];
        for (int c = 0; c < 2; c++) {
            float *x = c ? r : l, z1 = f->z1[c], z2 = f->z2[c];
            for (uint32_t i = 0; i < n; i++) {
                float in = x[i], out = f->b0 * in + z1;
                z1 = f->b1 * in - f->a1 * out + z2;
                z2 = f->b2 * in - f->a2 * out;
                x[i] = out;
            }
            f->z1[c] = z1;
            f->z2[c] = z2;
        }
    }
}

#endif /* OLS_EQ3_H */
