/*
 * OpenVALive - DSP Core
 * dsp_common.h : shared constants, sample-rate handling, fast math helpers.
 *
 * Rules for everything in core/dsp:
 *   - no hardware access (no screen, buttons, MIDI or DAC)
 *   - no dynamic memory allocation
 *   - 32-bit float only (the FM-1's pi32v2 CPU has a single-precision FPU)
 *   - expensive functions (tan, exp, pow) only at control rate, never per sample
 */
#ifndef DSP_COMMON_H
#define DSP_COMMON_H

#include <stdint.h>

/* ------------------------------------------------------------------------
 * Math backend
 *
 * The FM-1 firmware has no C library and no libm: with OLS_FASTMATH defined,
 * we use fastmath.h from fm1-base (fm_* functions). On a PC we use libm.
 * Always call the dsp_* names below, never tanf/expf/... directly.
 * ---------------------------------------------------------------------- */
#ifdef OLS_FASTMATH
#include "fastmath.h"
#define dsp_floorf fm_floorf
#define dsp_fabsf  fm_fabsf
#define dsp_expf   fm_expf
#define dsp_powf   fm_powf
#define dsp_tanf   fm_tanf
#else
#include <math.h>
#define dsp_floorf floorf
#define dsp_fabsf  fabsf
#define dsp_expf   expf
#define dsp_powf   powf
#define dsp_tanf   tanf
#endif

/* ------------------------------------------------------------------------
 * Sample rate, two modes
 *
 *   FIXED at compile time: define DSP_SR_F (or DSP_SAMPLE_RATE). Every
 *   conversion becomes a constant. This is the FM-1 mode: its audio clock is
 *   24 MHz / 544 = 44117.647 Hz (see felucca-bsp docs/BOARD_IO.md).
 *
 *   SET AT RUN TIME (default): call dsp_set_sample_rate() BEFORE initialising
 *   an engine (plugins at 44.1/48/96 kHz, Daisy, Teensy...). Requires
 *   core/dsp/dsp_runtime.c in the build. One sample rate per program.
 *
 * In both modes the sample rate is only used by control-rate code
 * (coefficients, times), never inside the per-sample loops.
 * ---------------------------------------------------------------------- */
#if !defined(DSP_SR_F) && defined(DSP_SAMPLE_RATE)
#define DSP_SR_F ((float)DSP_SAMPLE_RATE)
#endif

#ifdef DSP_SR_F
#define DSP_SR     (DSP_SR_F)
#define DSP_INV_SR (1.0f / (DSP_SR_F))
#else
extern float dsp_sr;
extern float dsp_inv_sr;
#define DSP_SR     dsp_sr
#define DSP_INV_SR dsp_inv_sr
void dsp_set_sample_rate(float sample_rate_hz);
#endif

/* Control block: modulations (LFOs, pitch, cutoff) are recomputed every
 * DSP_BLOCK samples (32 samples = 0.67 ms at 48 kHz). */
#ifndef DSP_BLOCK
#define DSP_BLOCK 32
#endif

#define DSP_CTRL_RATE (DSP_SR / (float)DSP_BLOCK)
#define DSP_PI        3.14159265358979f
#define DSP_TWO_PI    6.28318530717959f

/* ------------------------------------------------------------------------
 * Small helpers
 * ---------------------------------------------------------------------- */

static inline float dsp_clampf(float x, float lo, float hi)
{
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

static inline float dsp_lerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}

/* Rational tanh (Pade 3/2). Continuous, saturates exactly at +/-1 beyond |3|.
 * Used for drive and soft clipping: about 6 flops, no libm call. */
static inline float dsp_tanh(float x)
{
    if (x > 3.0f) {
        return 1.0f;
    }
    if (x < -3.0f) {
        return -1.0f;
    }
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

/* Fast 2^x: IEEE exponent bits + degree-5 polynomial for the fraction.
 * Error below 0.3 cent, good enough for pitch. */
static inline float dsp_exp2(float x)
{
    if (x < -120.0f) {
        x = -120.0f;
    }
    if (x > 120.0f) {
        x = 120.0f;
    }

    float whole = dsp_floorf(x);
    float frac = x - whole;

    float poly = 1.0f + frac * (0.69314718f
                      + frac * (0.24022651f
                      + frac * (0.05550411f
                      + frac * (0.00961813f
                      + frac * 0.00133336f))));

    union {
        float f;
        int32_t i;
    } bits;
    bits.i = ((int32_t)whole + 127) << 23;

    return poly * bits.f;
}

/* MIDI note number (may be fractional) to frequency in Hz. A4 = 69 = 440 Hz. */
static inline float dsp_mtof(float note)
{
    return 440.0f * dsp_exp2((note - 69.0f) * (1.0f / 12.0f));
}

/* xorshift32 pseudo-random generator, returns a value in [-1, 1]. */
static inline float dsp_noise(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return (float)(int32_t)x * (1.0f / 2147483648.0f);
}

/* Maps a normalised 0..1 parameter to an exponential range [lo, hi].
 * Used for times and frequencies, so the knob feels even across the range. */
static inline float dsp_expmap(float value01, float lo, float hi)
{
    return lo * dsp_powf(hi / lo, dsp_clampf(value01, 0.0f, 1.0f));
}

#endif /* DSP_COMMON_H */
