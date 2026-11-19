/*
 * OpenVALive - DSP Core
 * adsr.h : exponential "analog style" ADSR envelope.
 *
 * Each stage is a one-pole move towards a target:  y += coeff * (target - y).
 * The attack aims at 1.3 and switches to decay when it reaches 1.0, which
 * gives the concave, capacitor-like attack curve of analog synths.
 * Cost: ~3 flops per sample, 24 bytes of RAM.
 */
#ifndef DSP_ADSR_H
#define DSP_ADSR_H

#include "dsp_common.h"

typedef enum {
    ENV_IDLE = 0,
    ENV_ATTACK,
    ENV_DECAY,      /* also holds the sustain level once reached */
    ENV_RELEASE
} env_stage_t;

typedef struct {
    float y;                         /* current output, 0..1 */
    float a_coef, d_coef, r_coef;    /* per-stage coefficients */
    float sustain;
    env_stage_t stage;
} dsp_adsr_t;

#define ADSR_ATTACK_TARGET 1.3f

/* One-pole coefficient for a time constant tau, in seconds. */
static inline float adsr_coef(float tau_seconds)
{
    if (tau_seconds < 1e-5f) {
        return 1.0f;
    }
    return 1.0f - dsp_expf(-1.0f / (tau_seconds * DSP_SR));
}

static inline void adsr_init(dsp_adsr_t *e)
{
    e->y = 0.0f;
    e->stage = ENV_IDLE;
    e->sustain = 0.7f;
    e->a_coef = 0.01f;
    e->d_coef = 0.01f;
    e->r_coef = 0.01f;
}

/* Times in seconds.
 * attack: time to reach 1.0.  decay / release: time to fall by about 40 dB. */
static inline void adsr_set(dsp_adsr_t *e, float attack_s, float decay_s,
                            float sustain, float release_s)
{
    e->a_coef = adsr_coef(attack_s / 1.466f);   /* 1.466 = ln(1.3 / 0.3) */
    e->d_coef = adsr_coef(decay_s / 4.6f);      /* 4.6 = ln(100) */
    e->r_coef = adsr_coef(release_s / 4.6f);
    e->sustain = dsp_clampf(sustain, 0.0f, 1.0f);
}

/* Starts from the current level: retriggering never clicks. */
static inline void adsr_gate_on(dsp_adsr_t *e)
{
    e->stage = ENV_ATTACK;
}

static inline void adsr_gate_off(dsp_adsr_t *e)
{
    if (e->stage != ENV_IDLE) {
        e->stage = ENV_RELEASE;
    }
}

static inline int adsr_active(const dsp_adsr_t *e)
{
    return e->stage != ENV_IDLE;
}

/* Advances by one sample and returns the new level. */
static inline float adsr_tick(dsp_adsr_t *e)
{
    switch (e->stage) {
    case ENV_ATTACK:
        e->y += e->a_coef * (ADSR_ATTACK_TARGET - e->y);
        if (e->y >= 1.0f) {
            e->y = 1.0f;
            e->stage = ENV_DECAY;
        }
        break;

    case ENV_DECAY:
        e->y += e->d_coef * (e->sustain - e->y);
        break;

    case ENV_RELEASE:
        e->y += e->r_coef * (0.0f - e->y);
        if (e->y < 1e-4f) {
            e->y = 0.0f;
            e->stage = ENV_IDLE;
        }
        break;

    case ENV_IDLE:
    default:
        break;
    }
    return e->y;
}

#endif /* DSP_ADSR_H */
