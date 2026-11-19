/*
 * OpenVALive - DRUM engine: how each instrument sounds.
 *
 * dr_trigger() sets an instrument up from the parameters (the exp() and
 * tan() happen here, once per hit); dr_render_voice() then only multiplies
 * and adds. Envelopes are exponential: value *= coef every sample, the
 * shape of an analog decay.
 */
#include "engine_drum_internal.h"

#define SILENT 0.0001f            /* -80 dB: the instrument stops */

/* The six "metal" oscillators of the 808 hats and cymbal, in Hz. */
static const float METAL_HZ[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };

/* Multiplier that makes a value fall by 60 dB in `seconds`. */
static float decay_coef(float seconds)
{
    if (seconds < 0.001f) {
        seconds = 0.001f;
    }
    return dsp_expf(-6.9078f * DSP_INV_SR / seconds);
}

/* sin(2 pi phase) with the parabolic approximation (0.1 % error), phase 0..1. */
static float sine01(float phase)
{
    float x = 2.0f * phase - 1.0f;
    float y = 4.0f * x * (1.0f - dsp_fabsf(x));
    y = 0.225f * (y * dsp_fabsf(y) - y) + y;
    return -y;
}

static float wrap01(float phase)
{
    return phase >= 1.0f ? phase - 1.0f : phase;
}

/* ---------------------------------------------------------------- triggering */

static float param(const engine_drum_t *e, dr_param_t id)
{
    return e->p[id];
}

DR_COLD void dr_trigger(engine_drum_t *e, dr_inst_t inst, float velocity)
{
    int triangle = inst == DR_TRIANGLE;
    if (triangle) {
        inst = DR_COWBELL;                    /* its voice */
    }
    dr_voice_t *v = &e->voice[inst];
    const float tune = e->tune;
    const float decay = e->decay;

    v->active = 1;
    v->age = 0;
    v->amp = 1.0f;
    v->noise = 1.0f;
    v->sweep = 1.0f;
    for (int i = 0; i < 6; i++) {
        v->phase[i] = 0.0f;
    }
    svf_reset(&v->svf);

    switch (inst) {
    case DR_KICK:
        v->level = velocity * 1.4f * param(e, DR_P_LEVEL_KICK);
        v->freq = (38.0f + 40.0f * param(e, DR_P_KICK_PITCH)) * tune;
        v->amp_coef = decay_coef((0.15f + 1.2f * param(e, DR_P_KICK_DECAY)) * decay);
        v->sweep_coef = decay_coef(0.02f + 0.08f * param(e, DR_P_KICK_SWEEP));
        v->noise = param(e, DR_P_KICK_CLICK);                 /* the click: a 4 ms tick */
        v->noise_coef = decay_coef(0.004f);
        svf_set(&v->svf, 3000.0f, 0.2f);
        break;
    case DR_SNARE:
        v->level = velocity * param(e, DR_P_LEVEL_SNARE);
        v->freq = (150.0f + 90.0f * param(e, DR_P_SNARE_TONE)) * tune;
        v->amp_coef = decay_coef(0.12f * decay);
        v->noise = 0.3f + 0.9f * param(e, DR_P_SNARE_SNAP);
        v->noise_coef = decay_coef((0.08f + 0.4f * param(e, DR_P_SNARE_DECAY)) * decay);
        svf_set(&v->svf, (2000.0f + 4000.0f * param(e, DR_P_SNARE_TONE)) * tune, 0.0f);   /* wide band */
        break;
    case DR_CLAP:
        v->level = velocity * 1.04f * param(e, DR_P_LEVEL_PERC);   /* (-20 %: it stood out) */
        v->noise_coef = decay_coef((0.15f + 0.5f * param(e, DR_P_CLAP_DECAY)) * decay);
        v->amp_coef = decay_coef(0.012f);                      /* each burst */
        svf_set(&v->svf, 1100.0f * tune, 0.45f);
        break;
    case DR_CH:
    case DR_OH: {
        int open = inst == DR_OH;
        v->level = velocity * 2.2f * param(e, DR_P_LEVEL_HAT);
        v->freq = (0.8f + 0.5f * param(e, DR_P_HAT_TONE)) * tune; /* ratio for the metal */
        float t = open ? 0.15f + 0.9f * param(e, DR_P_HAT_DECAY) : 0.02f + 0.12f * param(e, DR_P_HAT_DECAY);
        v->amp_coef = decay_coef(t * decay);
        svf_set(&v->svf, (5000.0f + 4000.0f * param(e, DR_P_HAT_TONE)) * tune, 0.35f);
        break;
    }
    case DR_LTOM:
    case DR_MTOM:
    case DR_HTOM:
    case DR_FTOM: {
        static const float TOM_HZ[4] = { 90.0f, 130.0f, 185.0f, 64.0f };
        v->level = velocity * param(e, DR_P_LEVEL_PERC);
        v->freq = TOM_HZ[inst - DR_LTOM] * tune * (e->hit_pitch > 0.0f ? e->hit_pitch : 1.0f);
        v->amp_coef = decay_coef((0.15f + 0.6f * param(e, DR_P_TOM_DECAY)) * decay);
        v->sweep_coef = decay_coef(0.06f);
        v->noise = 0.15f;
        v->noise_coef = decay_coef(0.02f);
        svf_set(&v->svf, 800.0f, 0.1f);
        break;
    }
    case DR_RIM:
        v->level = velocity * 0.8f * param(e, DR_P_LEVEL_PERC);
        v->freq = 1700.0f * tune;
        v->amp_coef = decay_coef(0.025f);
        break;
    case DR_COWBELL:
    default:
        if (triangle) {                        /* a struck triangle: two high inharmonic partials, a long ring */
            v->level = velocity * 0.4f * param(e, DR_P_LEVEL_PERC);    /* (-20 %) */
            v->freq = tune;
            v->amp_coef = decay_coef(1.1f * decay);
            v->noise = 0.0f;
            v->sweep = 1.0f;                   /* (marks the triangle for the render) */
            break;
        }
        v->sweep = 0.0f;
        v->level = velocity * 0.96f * param(e, DR_P_LEVEL_PERC);   /* the cowbell (-20 %) */
        v->freq = tune;
        v->amp_coef = decay_coef(0.35f * decay);
        v->noise = 1.0f;                                       /* the bright first part */
        v->noise_coef = decay_coef(0.04f);
        svf_set(&v->svf, 2600.0f * tune, 0.5f);
        break;
    }
}

/* ---------------------------------------------------------------- sound */

static void render_kick(engine_drum_t *e, dr_voice_t *v, float *mix, int n)
{
    const float sweep_depth = 2.0f + 6.0f * e->p[DR_P_KICK_SWEEP];   /* starts 3..8x higher */
    for (int i = 0; i < n; i++) {
        float f = v->freq * (1.0f + sweep_depth * v->sweep);
        v->phase[0] = wrap01(v->phase[0] + f * DSP_INV_SR);
        float body = sine01(v->phase[0]) * v->amp;
        float click = svf_process(&v->svf, dsp_noise(&e->rng), SVF_HP) * v->noise;
        mix[i] += (body + 0.5f * click) * v->level;
        v->amp *= v->amp_coef;
        v->sweep *= v->sweep_coef;
        v->noise *= v->noise_coef;
    }
    v->active = v->amp > SILENT;
}

static void render_snare(engine_drum_t *e, dr_voice_t *v, float *mix, int n)
{
    const float f1 = v->freq * DSP_INV_SR;
    const float f2 = v->freq * 1.6f * DSP_INV_SR;
    for (int i = 0; i < n; i++) {
        v->phase[0] = wrap01(v->phase[0] + f1);
        v->phase[1] = wrap01(v->phase[1] + f2);
        float tone = (sine01(v->phase[0]) + 0.6f * sine01(v->phase[1])) * v->amp;
        float snap = svf_process(&v->svf, dsp_noise(&e->rng), SVF_BP) * v->noise;
        mix[i] += (0.6f * tone + 0.9f * snap) * v->level;
        v->amp *= v->amp_coef;
        v->noise *= v->noise_coef;
    }
    v->active = v->amp > SILENT || v->noise > SILENT;
}

static void render_clap(engine_drum_t *e, dr_voice_t *v, float *mix, int n)
{
    const uint32_t burst = (uint32_t)(0.010f / DSP_INV_SR);   /* 10 ms between the three hand claps */
    for (int i = 0; i < n; i++) {
        if (v->age == burst || v->age == 2u * burst) {
            v->amp = 1.0f;                                     /* the next clap */
        }
        float x = svf_process(&v->svf, dsp_noise(&e->rng), SVF_BP);
        float env = v->age < 3u * burst ? v->amp : v->noise;   /* bursts, then the tail */
        mix[i] += 2.0f * x * env * v->level;
        v->amp *= v->amp_coef;
        v->noise *= v->noise_coef;
        v->age++;
    }
    v->active = v->age < 3u * (uint32_t)(0.010f / DSP_INV_SR) || v->noise > SILENT;
}

static void render_hat(engine_drum_t *e, dr_voice_t *v, float *mix, int n)
{
    float inc[6];
    for (int k = 0; k < 6; k++) {
        inc[k] = METAL_HZ[k] * v->freq * DSP_INV_SR;
    }
    for (int i = 0; i < n; i++) {
        float metal = 0.0f;
        for (int k = 0; k < 6; k++) {
            v->phase[k] = wrap01(v->phase[k] + inc[k]);
            metal += v->phase[k] < 0.5f ? 1.0f : -1.0f;
        }
        float x = svf_process(&v->svf, metal * (1.0f / 6.0f) + 0.05f * dsp_noise(&e->rng), SVF_HP);
        mix[i] += x * v->amp * v->level;
        v->amp *= v->amp_coef;
    }
    v->active = v->amp > SILENT;
    (void)e;
}

static void render_tom(engine_drum_t *e, dr_voice_t *v, float *mix, int n)
{
    for (int i = 0; i < n; i++) {
        float f = v->freq * (1.0f + 0.4f * v->sweep);
        v->phase[0] = wrap01(v->phase[0] + f * DSP_INV_SR);
        float body = sine01(v->phase[0]) * v->amp;
        float skin = svf_process(&v->svf, dsp_noise(&e->rng), SVF_BP) * v->noise;
        mix[i] += (body + skin) * v->level;
        v->amp *= v->amp_coef;
        v->sweep *= v->sweep_coef;
        v->noise *= v->noise_coef;
    }
    v->active = v->amp > SILENT;
}

static void render_rim(engine_drum_t *e, dr_voice_t *v, float *mix, int n)
{
    const float inc = v->freq * DSP_INV_SR;
    for (int i = 0; i < n; i++) {
        v->phase[0] = wrap01(v->phase[0] + inc);
        v->phase[1] = wrap01(v->phase[1] + inc * 0.27f);
        float x = sine01(v->phase[0]) + 0.5f * sine01(v->phase[1]);
        mix[i] += x * v->amp * v->level;
        v->amp *= v->amp_coef;
    }
    v->active = v->amp > SILENT;
    (void)e;
}

static void render_triangle(dr_voice_t *v, float *mix, int n)
{
    const float a = 1980.0f * v->freq * DSP_INV_SR;
    const float b = 5460.0f * v->freq * DSP_INV_SR;
    for (int i = 0; i < n; i++) {
        v->phase[0] = wrap01(v->phase[0] + a);
        v->phase[1] = wrap01(v->phase[1] + b);
        mix[i] += (sine01(v->phase[0]) + 0.35f * sine01(v->phase[1])) * v->amp * v->level;
        v->amp *= v->amp_coef;
    }
    v->active = v->amp > SILENT;
}

static void render_cowbell(engine_drum_t *e, dr_voice_t *v, float *mix, int n)
{
    if (v->sweep > 0.5f) {
        render_triangle(v, mix, n);
        return;
    }
    const float a = 540.0f * v->freq * DSP_INV_SR;
    const float b = 800.0f * v->freq * DSP_INV_SR;
    for (int i = 0; i < n; i++) {
        v->phase[0] = wrap01(v->phase[0] + a);
        v->phase[1] = wrap01(v->phase[1] + b);
        float sq = (v->phase[0] < 0.5f ? 1.0f : -1.0f) + (v->phase[1] < 0.5f ? 1.0f : -1.0f);
        float x = svf_process(&v->svf, sq * 0.5f, SVF_BP);
        mix[i] += x * (0.6f * v->amp + 0.4f * v->noise) * v->level;
        v->amp *= v->amp_coef;
        v->noise *= v->noise_coef;
    }
    v->active = v->amp > SILENT;
    (void)e;
}

void dr_render_voice(engine_drum_t *e, dr_inst_t inst, float *mix, int n)
{
    dr_voice_t *v = &e->voice[inst];
    switch (inst) {
    case DR_KICK:    render_kick(e, v, mix, n); break;
    case DR_SNARE:   render_snare(e, v, mix, n); break;
    case DR_CLAP:    render_clap(e, v, mix, n); break;
    case DR_CH:
    case DR_OH:      render_hat(e, v, mix, n); break;
    case DR_LTOM:
    case DR_MTOM:
    case DR_HTOM:
    case DR_FTOM:    render_tom(e, v, mix, n); break;
    case DR_RIM:     render_rim(e, v, mix, n); break;
    case DR_COWBELL:
    default:         render_cowbell(e, v, mix, n); break;
    }
}
