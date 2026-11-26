/*
 * OpenVALive - VA engine: parameters.
 *
 * Every parameter arrives normalised to 0..1. This file converts it once
 * into the unit the DSP needs (Hz, seconds, semitones, gain) and stores the
 * result in e->d, so the audio loop never has to convert anything.
 */
#include "engine_va_internal.h"

const char *const VA_PARAM_NAMES[VA_P_COUNT] = {
    "Osc1Wav", "Osc2Wav", "Osc2Crs", "Osc2Fin", "PW",
    "MixOsc1", "MixOsc2", "MixSub", "MixNois",
    "Cutoff", "Reso", "FltMode", "FEnvAmt", "KeyTrk", "Drive",
    "F.Atk", "F.Dec", "F.Sus", "F.Rel",
    "A.Atk", "A.Dec", "A.Sus", "A.Rel",
    "L1Rate", "L1Wave", "L1>Pitc", "L1>PW",
    "L2Rate", "L2Wave", "L2>Cut",
    "Glide", "Mono", "VelSens",
    "DlyTime", "DlyFb", "DlyTone", "DlyMix",
    "ChoMode", "ChoMix", "Volume",
    "PhsRate", "PhsDpth", "PhsMix", "Crush", "VelFilt",
    "L1Sync", "L1Trig", "L2Sync", "L2Trig",
    "M1Src", "M1Dst", "M1Amt", "M2Src", "M2Dst", "M2Amt",
    "M3Src", "M3Dst", "M3Amt", "M4Src", "M4Dst", "M4Amt"
};

/* Waveforms, modes and switches jump; everything else is interpolated. */
const uint8_t VA_PARAM_KINDS[VA_P_COUNT] = {
    [VA_P_OSC1_WAVE] = OLS_PARAM_DISCRETE,
    [VA_P_OSC2_WAVE] = OLS_PARAM_DISCRETE,
    [VA_P_OSC2_COARSE] = OLS_PARAM_DISCRETE,
    [VA_P_FILTER_MODE] = OLS_PARAM_DISCRETE,
    [VA_P_LFO1_WAVE] = OLS_PARAM_DISCRETE,
    [VA_P_LFO2_WAVE] = OLS_PARAM_DISCRETE,
    [VA_P_MONO] = OLS_PARAM_DISCRETE,
    [VA_P_CHORUS_MODE] = OLS_PARAM_DISCRETE,
    [VA_P_LFO1_SYNC] = OLS_PARAM_DISCRETE, [VA_P_LFO1_TRIG] = OLS_PARAM_DISCRETE,
    [VA_P_LFO2_SYNC] = OLS_PARAM_DISCRETE, [VA_P_LFO2_TRIG] = OLS_PARAM_DISCRETE,
    [VA_P_MOD1_SRC] = OLS_PARAM_DISCRETE, [VA_P_MOD1_DST] = OLS_PARAM_DISCRETE,
    [VA_P_MOD2_SRC] = OLS_PARAM_DISCRETE, [VA_P_MOD2_DST] = OLS_PARAM_DISCRETE,
    [VA_P_MOD3_SRC] = OLS_PARAM_DISCRETE, [VA_P_MOD3_DST] = OLS_PARAM_DISCRETE,
    [VA_P_MOD4_SRC] = OLS_PARAM_DISCRETE, [VA_P_MOD4_DST] = OLS_PARAM_DISCRETE,
};

/* ---------------------------------------------------------------- performance offsets */

/* The role a parameter plays for the macros, or -1. */
static int role_of(uint16_t id)
{
    switch (id) {
    case VA_P_CUTOFF:      return OLS_ROLE_CUTOFF;
    case VA_P_RESO:        return OLS_ROLE_RESO;
    case VA_P_DRIVE:       return OLS_ROLE_DRIVE;
    case VA_P_DELAY_MIX:   return OLS_ROLE_DELAY_MIX;
    case VA_P_CRUSH:       return OLS_ROLE_CRUSH;
    case VA_P_A_DECAY:
    case VA_P_F_DECAY:     return OLS_ROLE_DECAY;
    case VA_P_A_RELEASE:   return OLS_ROLE_RELEASE;
    case VA_P_FENV_AMT:    return OLS_ROLE_FENV;
    case VA_P_CHORUS_MIX:  return OLS_ROLE_CHORUS;
    default:               return -1;
    }
}

/* The value the sound uses: the stored one plus the macros' offset. */
float va_effective(const engine_va_t *e, uint16_t id)
{
    int role = role_of(id);
    float v = e->p[id];
    if (role >= 0) {
        v += e->role_off[role];
    }
    return dsp_clampf(v, 0.0f, 1.0f);
}

void engine_va_role_offset(void *ctx, uint16_t role, float offset)
{
    engine_va_t *e = (engine_va_t *)ctx;
    if (role >= OLS_ROLE_COUNT || e->role_off[role] == offset) {
        return;
    }
    e->role_off[role] = offset;
    /* recompute what depends on it, from the stored values */
    for (uint16_t id = 0; id < VA_P_COUNT; id++) {
        if (role_of(id) == (int)role) {
            engine_va_set_param(e, id, e->p[id]);
        }
    }
}

/* ---------------------------------------------------------------- helpers */

static float squared(float x)
{
    return x * x;
}

/* 0..1 -> 1 ms .. 8 s, exponential */
static float envelope_time(float value01)
{
    return dsp_expmap(value01, 0.001f, 8.0f);
}

/* 0..1 -> 0.05 .. 30 Hz, exponential */
static float lfo_rate(float value01)
{
    return dsp_expmap(value01, 0.05f, 30.0f);
}

/* 0..1 -> index 0..count-1 (for wave or mode selectors) */
static int choice(float value01, int count)
{
    int i = (int)(value01 * (float)count);
    if (i >= count) {
        i = count - 1;
    }
    return i;
}

void va_update_envelopes(engine_va_t *e)
{
    float f_a = envelope_time(e->p[VA_P_F_ATTACK]);
    float f_d = envelope_time(va_effective(e, VA_P_F_DECAY));
    float f_r = envelope_time(e->p[VA_P_F_RELEASE]);
    float a_a = envelope_time(e->p[VA_P_A_ATTACK]);
    float a_d = envelope_time(va_effective(e, VA_P_A_DECAY));
    float a_r = envelope_time(va_effective(e, VA_P_A_RELEASE));

    for (int i = 0; i < VA_VOICES; i++) {
        adsr_set(&e->voice[i].fenv, f_a, f_d, e->p[VA_P_F_SUSTAIN], f_r);
        adsr_set(&e->voice[i].amp, a_a, a_d, e->p[VA_P_A_SUSTAIN], a_r);
    }
}

void va_update_delay(engine_va_t *e)
{
    if (e->delay.buf == 0) {
        return;
    }
    /* the knob's span is the same whatever the buffer (16382 samples, 0.37 s at 44.1 kHz): a target with a shorter
     * buffer (the SMK-37, for RAM) only loses the longest times, clamped by delay_set; the sounds stay the same */
    float max_seconds = 16382.0f * DSP_INV_SR;
    float time = dsp_expmap(e->p[VA_P_DELAY_TIME], 0.02f, max_seconds);
    float feedback = e->p[VA_P_DELAY_FB] * 0.9f;
    delay_set(&e->delay, time, feedback, e->p[VA_P_DELAY_TONE]);
}

static void update_osc2_offset(engine_va_t *e)
{
    float coarse = dsp_floorf(-24.0f + 48.0f * e->p[VA_P_OSC2_COARSE] + 0.5f);
    float fine = e->p[VA_P_OSC2_FINE] - 0.5f;          /* +/- 0.5 semitone */
    e->d.osc2_offset = coarse + fine;
}

static void update_drive(engine_va_t *e, float value01)
{
    e->d.drive_gain = 1.0f + 15.0f * squared(value01);
}

static void update_glide(engine_va_t *e, float value01)
{
    if (value01 < 0.01f) {
        e->d.glide_coef = 1.0f;                        /* no glide */
        return;
    }
    float seconds = dsp_expmap(value01, 0.005f, 2.0f);
    e->d.glide_coef = 1.0f - dsp_expf(-1.0f / (seconds * DSP_CTRL_RATE));
}

static void update_mono(engine_va_t *e, float value01)
{
    int mono = value01 >= 0.5f;
    if (mono != e->d.mono) {
        engine_va_all_notes_off(e);                    /* avoid stuck notes when switching */
        e->d.mono = mono;
    }
}

/* ------------------------------------------------------------ set_param */

void engine_va_set_param(void *ctx, uint16_t id, float value)
{
    engine_va_t *e = (engine_va_t *)ctx;
    if (id >= VA_P_COUNT) {
        return;
    }
    value = dsp_clampf(value, 0.0f, 1.0f);
    e->p[id] = value;
    value = va_effective(e, id);          /* the macros ride on top (0 offset: the same value) */

    switch ((va_param_t)id) {
    /* oscillators */
    case VA_P_OSC1_WAVE:   e->d.wave1 = (osc_wave_t)choice(value, OSC_WAVE_COUNT); break;
    case VA_P_OSC2_WAVE:   e->d.wave2 = (osc_wave_t)choice(value, OSC_WAVE_COUNT); break;
    case VA_P_OSC2_COARSE:
    case VA_P_OSC2_FINE:   update_osc2_offset(e); break;
    case VA_P_PW:          e->d.pw = 0.5f + 0.9f * (value - 0.5f); break;

    /* mixer: squared for a natural volume curve */
    case VA_P_MIX_OSC1:    e->d.gain_osc1 = squared(value); break;
    case VA_P_MIX_OSC2:    e->d.gain_osc2 = squared(value); break;
    case VA_P_MIX_SUB:     e->d.gain_sub = squared(value); break;
    case VA_P_MIX_NOISE:   e->d.gain_noise = squared(value); break;

    /* filter */
    case VA_P_CUTOFF:      e->d.cutoff_note = 20.0f + 115.0f * value; break;   /* ~26 Hz .. 20 kHz */
    case VA_P_RESO:        e->d.reso = value; break;
    case VA_P_FILTER_MODE: e->d.filter_mode = (svf_mode_t)choice(value, SVF_MODE_COUNT); break;
    case VA_P_FENV_AMT:    e->d.fenv_amount = (value - 0.5f) * 2.0f * 72.0f; break;   /* +/- 6 octaves */
    case VA_P_KEYTRACK:    e->d.keytrack = value; break;
    case VA_P_DRIVE:       update_drive(e, value); break;

    /* envelopes */
    case VA_P_F_ATTACK: case VA_P_F_DECAY: case VA_P_F_SUSTAIN: case VA_P_F_RELEASE:
    case VA_P_A_ATTACK: case VA_P_A_DECAY: case VA_P_A_SUSTAIN: case VA_P_A_RELEASE:
        va_update_envelopes(e);
        break;

    /* LFOs */
    case VA_P_LFO1_RATE:   lfo_set_rate(&e->lfo1, lfo_rate(value)); e->lfo_inc[0] = e->lfo1.inc; break;
    case VA_P_LFO1_WAVE:   e->lfo1.wave = (lfo_wave_t)choice(value, LFO_WAVE_COUNT); break;
    case VA_P_LFO1_PITCH:  e->d.lfo1_to_pitch = squared(value) * 12.0f; break;
    case VA_P_LFO1_PW:     e->d.lfo1_to_pw = value * 0.4f; break;
    case VA_P_LFO2_RATE:   lfo_set_rate(&e->lfo2, lfo_rate(value)); e->lfo_inc[1] = e->lfo2.inc; break;
    case VA_P_LFO2_WAVE:   e->lfo2.wave = (lfo_wave_t)choice(value, LFO_WAVE_COUNT); break;
    case VA_P_LFO2_CUTOFF: e->d.lfo2_to_cutoff = squared(value) * 48.0f; break;

    /* playing */
    case VA_P_GLIDE:       update_glide(e, value); break;
    case VA_P_MONO:        update_mono(e, value); break;
    case VA_P_VEL_SENS:    e->d.vel_sens = value; break;

    /* effects */
    case VA_P_DELAY_TIME: case VA_P_DELAY_FB: case VA_P_DELAY_TONE:
        va_update_delay(e);
        break;
    case VA_P_DELAY_MIX:   e->d.delay_mix = value; break;
    case VA_P_CHORUS_MODE: chorus_set_mode(&e->chorus, (chorus_mode_t)choice(value, CHORUS_MODE_COUNT)); break;
    case VA_P_CHORUS_MIX:  e->d.chorus_mix = value; break;
    case VA_P_VOLUME:      e->d.volume = 2.0f * squared(value); break;
    case VA_P_PHASER_RATE: lfo_set_rate(&e->lfo_phaser, dsp_expmap(value, 0.03f, 6.0f)); break;
    case VA_P_PHASER_DEPTH:
        e->d.phaser_span = 1.0f + 5.0f * value;              /* 1..6 octaves above 150 Hz */
        e->phaser.feedback = 0.3f + 0.45f * value;
        break;
    case VA_P_PHASER_MIX:  e->d.phaser_mix = value; break;
    case VA_P_VEL_FILTER:  e->d.vel_filter = 36.0f * value; break;
    case VA_P_LFO1_SYNC: case VA_P_LFO1_TRIG: case VA_P_LFO2_SYNC: case VA_P_LFO2_TRIG:
    case VA_P_MOD1_SRC: case VA_P_MOD1_DST: case VA_P_MOD1_AMT:
    case VA_P_MOD2_SRC: case VA_P_MOD2_DST: case VA_P_MOD2_AMT:
    case VA_P_MOD3_SRC: case VA_P_MOD3_DST: case VA_P_MOD3_AMT:
    case VA_P_MOD4_SRC: case VA_P_MOD4_DST: case VA_P_MOD4_AMT:
        va_mod_update(e);
        break;
    case VA_P_CRUSH:
        if (value < 0.01f) {
            e->d.crush_levels = 0.0f;                       /* off: no cost */
        } else {
            e->d.crush_levels = dsp_exp2(15.0f - 11.0f * value);   /* 15 bits .. 4 bits */
            e->d.crush_hold = 1 + (int)(15.0f * value * value);    /* full rate .. 1/16 */
        }
        break;

    default:
        break;
    }
}
