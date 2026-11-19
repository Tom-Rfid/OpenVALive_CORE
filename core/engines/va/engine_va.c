/*
 * OpenVALive - VA engine: API table, initialisation, small accessors.
 *
 * Note: this code builds without a C library (the FM-1 firmware has none):
 * no <string.h>, no NULL, no malloc.
 */
#include "engine_va_internal.h"

static const uint16_t VA_ROLES[OLS_ROLE_COUNT] = {
    [OLS_ROLE_CUTOFF] = VA_P_CUTOFF,
    [OLS_ROLE_RESO] = VA_P_RESO,
    [OLS_ROLE_DRIVE] = VA_P_DRIVE,
    [OLS_ROLE_DELAY_MIX] = VA_P_DELAY_MIX,
    [OLS_ROLE_VOLUME] = VA_P_VOLUME,
    [OLS_ROLE_CRUSH] = VA_P_CRUSH,
    [OLS_ROLE_DECAY] = VA_P_A_DECAY,
    [OLS_ROLE_RELEASE] = VA_P_A_RELEASE,
    [OLS_ROLE_FENV] = VA_P_FENV_AMT,
    [OLS_ROLE_CHORUS] = VA_P_CHORUS_MIX,
};

static const uint16_t VA_MACROS[OLS_MACROS] = { VA_P_CUTOFF, VA_P_RESO, VA_P_FENV_AMT, VA_P_DELAY_MIX };
static const char *const VA_MACRO_NAMES[OLS_MACROS] = { "CUTOFF", "RESO", "ENV", "FX" };

const ols_engine_api_t ENGINE_VA_API = {
    .name = "VA",
    .param_count = VA_P_COUNT,
    .param_names = VA_PARAM_NAMES,
    .param_kinds = VA_PARAM_KINDS,
    .note_on = engine_va_note_on,
    .note_off = engine_va_note_off,
    .all_notes_off = engine_va_all_notes_off,
    .set_param = engine_va_set_param,
    .get_param = engine_va_get_param,
    .pitch_bend = engine_va_pitch_bend,
    .render = engine_va_render,
    .roles = VA_ROLES,
    .macros = VA_MACROS,
    .macro_names = VA_MACRO_NAMES,
    .pressure = engine_va_pressure,
    .role_offset = engine_va_role_offset,
    .clock = engine_va_clock,
    .motion = engine_va_motion,
};

void engine_va_init(engine_va_t *e, int16_t *delay_buf, uint32_t delay_len)
{
    /* zero the whole state without memset (a loop: a "static const zero" copy would cost its size in flash) */
    for (uint32_t i = 0; i < sizeof *e; i++) {
        ((uint8_t *)e)[i] = 0;
    }

    e->rng = 0x9E3779B9u;
    e->mod_rng = 0x2545F491u;
    e->bpm = 120.0f;
    e->voice_limit = VA_VOICES;
    va_voices_init(e);

    lfo_init(&e->lfo1, 0xA5A5A5A5u);
    lfo_init(&e->lfo2, 0x5A5A5A5Au);
    lfo_init(&e->lfo_phaser, 0x3C3C3C3Cu);
    e->lfo_phaser.wave = LFO_TRIANGLE;
    phaser_init(&e->phaser);

    if (delay_buf != 0 && delay_len > 64) {
        delay_init(&e->delay, delay_buf, delay_len);
    }
    chorus_init(&e->chorus);

    /* the delay line must have been rewritten with silence before sleeping */
    e->sleep_after = delay_len + 4096u;

    /* parameter smoothing: 5 ms time constant, applied once per control block */
    e->smooth_coef = 1.0f - dsp_expf(-1.0f / (0.005f * DSP_CTRL_RATE));

    /* -1 forces the mono/poly parameter to be applied by the first preset */
    e->d.mono = -1;
    engine_va_load_preset(e, VA_PRESET_INIT);
}

void engine_va_snap(engine_va_t *e)
{
    va_smoothed_t *s = &e->s;
    const va_derived_t *d = &e->d;
    s->cutoff_note = d->cutoff_note;
    s->reso = d->reso;
    s->fenv_amount = d->fenv_amount;
    s->gain_osc1 = d->gain_osc1;
    s->gain_osc2 = d->gain_osc2;
    s->gain_sub = d->gain_sub;
    s->gain_noise = d->gain_noise;
    s->drive_gain = d->drive_gain;
    s->drive_comp = 0.6f / dsp_tanh(0.6f * d->drive_gain);
    s->volume = d->volume;
    s->delay_mix = d->delay_mix;
    s->chorus_mix = d->chorus_mix;
    e->chorus.mix = d->chorus_mix;
    s->phaser_mix = d->phaser_mix;
    s->pw = d->pw;
}

float engine_va_get_param(void *ctx, uint16_t id)
{
    engine_va_t *e = (engine_va_t *)ctx;
    if (id >= VA_P_COUNT) {
        return 0.0f;
    }
    return e->p[id];
}

void engine_va_pitch_bend(void *ctx, float semitones)
{
    engine_va_t *e = (engine_va_t *)ctx;
    e->bend = semitones;
}

void engine_va_set_voice_limit(engine_va_t *e, int n)
{
    if (n < 1) {
        n = 1;
    }
    if (n > VA_VOICES) {
        n = VA_VOICES;
    }
    e->voice_limit = n;

    /* silence the voices that are no longer allowed, immediately */
    for (int i = n; i < VA_VOICES; i++) {
        va_voice_t *v = &e->voice[i];
        v->gate = 0;
        v->amp.y = 0.0f;
        v->amp.stage = ENV_IDLE;
        v->fenv.stage = ENV_IDLE;
    }
}

/* Aftertouch: pressing harder opens the filter (up to 2 octaves) and deepens
 * the vibrato. Smoothed like every parameter, so it never zips. */
void engine_va_pressure(void *ctx, float amount01)
{
    engine_va_t *e = (engine_va_t *)ctx;
    e->d.pressure = dsp_clampf(amount01, 0.0f, 1.0f);
}
