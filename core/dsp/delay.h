/*
 * OpenVALive - DSP Core
 * delay.h : mono delay line with filtered feedback.
 *
 * The circular buffer stores int16 samples: half the RAM of a float buffer.
 * The buffer is provided by the caller, so each platform decides how much
 * memory it gives (16384 samples = 371 ms at 44.1 kHz = 32 KB).
 * The delay time glides smoothly (no clicks while turning the knob) and a
 * low-pass filter inside the feedback loop darkens each repeat, like tape.
 */
#ifndef DSP_DELAY_H
#define DSP_DELAY_H

#include "dsp_common.h"

typedef struct {
    int16_t *buf;
    uint32_t size;
    uint32_t write_pos;
    float time_smp;          /* current delay, in samples (smoothed) */
    float time_target;       /* requested delay, in samples */
    float feedback;
    float tone_state;        /* feedback low-pass state */
    float tone_coef;
} dsp_delay_t;

static inline void delay_init(dsp_delay_t *d, int16_t *buf, uint32_t size)
{
    d->buf = buf;
    d->size = size;
    d->write_pos = 0;
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = 0;
    }
    d->time_smp = (float)(size / 2);
    d->time_target = d->time_smp;
    d->feedback = 0.35f;
    d->tone_state = 0.0f;
    d->tone_coef = 0.35f;
}

/* time in seconds, feedback 0..0.95, tone 0 (dark) .. 1 (bright). */
static inline void delay_set(dsp_delay_t *d, float time_s, float feedback, float tone01)
{
    float max_samples = (float)(d->size - 2);
    d->time_target = dsp_clampf(time_s * DSP_SR, 1.0f, max_samples);
    d->feedback = dsp_clampf(feedback, 0.0f, 0.95f);
    d->tone_coef = 0.05f + 0.9f * dsp_clampf(tone01, 0.0f, 1.0f);
}

/* Linear interpolation between two stored samples. */
static inline float delay_read(const dsp_delay_t *d, float delay_samples)
{
    float read_pos = (float)d->write_pos - delay_samples;
    if (read_pos < 0.0f) {
        read_pos += (float)d->size;
    }

    uint32_t i0 = (uint32_t)read_pos;
    uint32_t i1 = i0 + 1;
    if (i1 >= d->size) {
        i1 = 0;
    }
    float frac = read_pos - (float)i0;

    float a = (float)d->buf[i0];
    float b = (float)d->buf[i1];
    return (a + frac * (b - a)) * (1.0f / 32767.0f);
}

/* Processes one sample. Returns only the delayed ("wet") signal. */
static inline float delay_process(dsp_delay_t *d, float input)
{
    /* glide towards the requested time */
    d->time_smp += 0.0005f * (d->time_target - d->time_smp);

    float wet = delay_read(d, d->time_smp);

    /* darker repeats */
    d->tone_state += d->tone_coef * (wet - d->tone_state);

    /* soft-clip what we write back, so high feedback never explodes */
    float to_write = dsp_tanh(input + d->tone_state * d->feedback);
    d->buf[d->write_pos] = (int16_t)(to_write * 32767.0f);

    d->write_pos++;
    if (d->write_pos >= d->size) {
        d->write_pos = 0;
    }
    return wet;
}

#endif /* DSP_DELAY_H */
