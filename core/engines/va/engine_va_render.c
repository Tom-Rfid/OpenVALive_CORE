/*
 * OpenVALive - VA engine: audio rendering.
 *
 * Two rates:
 *   - control rate (once every DSP_BLOCK samples): LFOs, glide, pitch,
 *     cutoff and filter coefficients (the only place tan() is used);
 *   - audio rate (every sample): oscillators, filter, envelopes, effects.
 */
#include "engine_va_internal.h"

/* Mixer headroom: two full-scale oscillators should not hit the filter at 2.0 */
#define MIX_SCALE   0.5f
/* Sum of voices before the effects */
#define VOICE_SCALE 0.8f

/* One small step of every smoothed value towards its target (control rate).
 * A one-pole glide of ~5 ms: fast enough to feel immediate, slow enough
 * that a jump in cutoff, level or drive never clicks. */
static void glide(float *value, float target, float k)
{
    *value += k * (target - *value);
}

void va_smooth_step(engine_va_t *e)
{
    va_smoothed_t *s = &e->s;
    const va_derived_t *d = &e->d;
    const float k = e->smooth_coef;

    glide(&s->cutoff_note, d->cutoff_note, k);
    glide(&s->reso, d->reso, k);
    glide(&s->fenv_amount, d->fenv_amount, k);
    glide(&s->gain_osc1, d->gain_osc1, k);
    glide(&s->gain_osc2, d->gain_osc2, k);
    glide(&s->gain_sub, d->gain_sub, k);
    glide(&s->gain_noise, d->gain_noise, k);
    glide(&s->volume, d->volume, k);
    glide(&s->delay_mix, d->delay_mix, k);
    glide(&s->chorus_mix, d->chorus_mix, k);
    glide(&s->phaser_mix, d->phaser_mix, k);
    glide(&s->pressure, d->pressure, k);
    glide(&s->pw, d->pw, k);

    if (s->drive_gain != d->drive_gain) {
        glide(&s->drive_gain, d->drive_gain, k);
        /* keep a typical 0.6 level unchanged whatever the drive amount */
        s->drive_comp = 0.6f / dsp_tanh(0.6f * s->drive_gain);
    }
    e->chorus.mix = s->chorus_mix;
}

/* The same as voice_control below, with the modulation matrix's slots added (engine_va_mod.c). Kept apart so
 * a sound without modulation runs exactly the code it always ran. */
static void voice_control_modulated(engine_va_t *e, va_voice_t *v, float lfo1, float lfo2)
{
    const va_derived_t *d = &e->d;
    const va_smoothed_t *s = &e->s;
    float m[VA_DST_COUNT];
    va_mod_voice(e, v, m);

    float pitch = v->note_cur + e->bend + lfo1 * (d->lfo1_to_pitch + 0.4f * s->pressure) + 24.0f * m[VA_DST_PITCH];
    osc_set_freq(&v->osc1, dsp_mtof(pitch));
    osc_set_freq(&v->osc2, dsp_mtof(pitch + d->osc2_offset + 24.0f * m[VA_DST_OSC2]));
    osc_set_freq(&v->sub, dsp_mtof(pitch - 12.0f));

    float pw = dsp_clampf(s->pw + lfo1 * d->lfo1_to_pw + 0.45f * m[VA_DST_PW], 0.05f, 0.95f);
    v->osc1.pw = pw;
    v->osc2.pw = pw;

    float cutoff = s->cutoff_note
                 + d->keytrack * (v->note_cur - 60.0f)
                 + (s->fenv_amount + 48.0f * m[VA_DST_FENV]) * v->fenv.y
                 + lfo2 * d->lfo2_to_cutoff
                 + 24.0f * s->pressure
                 + d->vel_filter * v->accent
                 + 60.0f * m[VA_DST_CUTOFF];
    cutoff = dsp_clampf(cutoff, 0.0f, 140.0f);
    svf_set(&v->svf, dsp_mtof(cutoff), dsp_clampf(s->reso + m[VA_DST_RESO], 0.0f, 1.0f));

    /* LEVEL: x0 .. x2, ramped over the period (no zipper on a fast tremolo) */
    float gain = dsp_clampf(1.0f + m[VA_DST_LEVEL], 0.0f, 2.0f);
    v->gain_step = (gain - v->gain_cur) * (1.0f / (float)DSP_BLOCK);
    /* MIX: towards osc 1 (-) or osc 2 (+) */
    float mix = dsp_clampf(m[VA_DST_MIX], -1.0f, 1.0f);
    v->mix1 = mix > 0.0f ? 1.0f - mix : 1.0f;
    v->mix2 = mix < 0.0f ? 1.0f + mix : 1.0f;
}

/* Control-rate update of one voice: pitch, pulse width, filter. */
static void voice_control(engine_va_t *e, va_voice_t *v, float lfo1, float lfo2)
{
    const va_derived_t *d = &e->d;
    const va_smoothed_t *s = &e->s;

    /* glide */
    v->note_cur += d->glide_coef * (v->note_target - v->note_cur);

    if (d->mod_voice) {
        voice_control_modulated(e, v, lfo1, lfo2);
        return;
    }

    /* pitch: note + bend + vibrato */
    float pitch = v->note_cur + e->bend + lfo1 * (d->lfo1_to_pitch + 0.4f * s->pressure);
    osc_set_freq(&v->osc1, dsp_mtof(pitch));
    osc_set_freq(&v->osc2, dsp_mtof(pitch + d->osc2_offset));
    osc_set_freq(&v->sub, dsp_mtof(pitch - 12.0f));

    /* pulse width, modulated by LFO1 */
    float pw = dsp_clampf(s->pw + lfo1 * d->lfo1_to_pw, 0.05f, 0.95f);
    v->osc1.pw = pw;
    v->osc2.pw = pw;

    /* cutoff, in semitones: base + key tracking + envelope + LFO2 */
    float cutoff = s->cutoff_note
                 + d->keytrack * (v->note_cur - 60.0f)
                 + s->fenv_amount * v->fenv.y
                 + lfo2 * d->lfo2_to_cutoff
                 + 24.0f * s->pressure
                 + d->vel_filter * v->accent;
    cutoff = dsp_clampf(cutoff, 0.0f, 140.0f);
    svf_set(&v->svf, dsp_mtof(cutoff), s->reso);

    /* no LEVEL / MIX modulation: back to the mixer's gains (a ramp, if a slot was just switched off) */
    v->gain_step = (1.0f - v->gain_cur) * (1.0f / (float)DSP_BLOCK);
    v->mix1 = 1.0f;
    v->mix2 = 1.0f;
}

/* Adds n samples of one voice into mix[] (its control values are already set). */
void va_render_voice(engine_va_t *e, va_voice_t *v, float *mix, int n)
{
    /* copy what the loop needs into locals: the compiler keeps them in registers */
    const va_derived_t *d = &e->d;
    const va_smoothed_t *s = &e->s;
    const float g1 = s->gain_osc1 * MIX_SCALE * v->mix1;
    const float g2 = s->gain_osc2 * MIX_SCALE * v->mix2;
    const float g_sub = s->gain_sub * MIX_SCALE;
    const float g_noise = s->gain_noise * MIX_SCALE;
    const osc_wave_t w1 = d->wave1;
    const osc_wave_t w2 = d->wave2;
    const svf_mode_t mode = d->filter_mode;
    const int drive_on = s->drive_gain > 1.001f;
    const float drive_gain = s->drive_gain;
    const float drive_comp = s->drive_comp;
    const float velocity = v->velocity;
    const float gain_step = v->gain_step;
    float gain = v->gain_cur;

    for (int i = 0; i < n; i++) {
        /* oscillators */
        osc_advance(&v->osc1);
        osc_advance(&v->osc2);
        osc_advance(&v->sub);

        float s = g1 * osc_value(&v->osc1, w1)
                + g2 * osc_value(&v->osc2, w2);
        if (g_sub > 0.0f) {
            s += g_sub * osc_value(&v->sub, OSC_SQUARE);
        }
        if (g_noise > 0.0f) {
            s += g_noise * dsp_noise(&e->rng);
        }

        /* filter (its envelope runs every sample, used at control rate) */
        adsr_tick(&v->fenv);
        float y = svf_process(&v->svf, s, mode);

        /* drive */
        if (drive_on) {
            y = dsp_tanh(y * drive_gain) * drive_comp;
        }

        /* amplifier (gain: the LEVEL modulation, exactly 1 without it) */
        mix[i] += y * adsr_tick(&v->amp) * velocity * gain;
        gain += gain_step;
    }
    v->gain_cur = gain;
}

#define SILENCE 1e-5f            /* -100 dBFS: below a 16-bit step */

static float peak_of(float peak, float x)
{
    float a = dsp_fabsf(x);
    return a > peak ? a : peak;
}

/* Effects on the summed voices, then soft clip. Returns the loudest value
 * seen anywhere in the chain (input, delay line, outputs): the sleep test. */
static float render_effects(engine_va_t *e, const float *mix, float *out_l, float *out_r, int n)
{
    const int has_delay = e->delay.buf != 0;
    const float phaser_mix = e->s.phaser_mix;
    const int has_phaser = phaser_mix > 0.001f;
    const float crush_levels = e->d.crush_levels;
    const float delay_mix = e->d.mod_engine
        ? dsp_clampf(e->s.delay_mix + e->mod_engine_now[VA_DST_DELAY], 0.0f, 1.0f) : e->s.delay_mix;
    const int crush_on = crush_levels > 0.0f;
    float peak = 0.0f;

    for (int i = 0; i < n; i++) {
        float x = mix[i] * VOICE_SCALE;
        peak = peak_of(peak, x);

        if (crush_on) {
            /* hold one sample for `hold` samples, on a coarse grid: the sound of cheap samplers */
            if (--e->crush_count <= 0) {
                e->crush_count = e->d.crush_hold;
                e->crush_value = dsp_floorf(x * crush_levels + 0.5f) / crush_levels;
            }
            x = e->crush_value;
        }

        if (has_phaser) {
            /* dry + wet in equal parts digs the deepest notches */
            float wet = phaser_process(&e->phaser, x);
            x += phaser_mix * 0.5f * (wet - x);
        }

        if (has_delay) {
            float wet = delay_process(&e->delay, x);
            peak = peak_of(peak, wet);
            x += wet * delay_mix;
        }

        float l, r;
        chorus_process(&e->chorus, x, &l, &r);

        out_l[i] = dsp_tanh(l * e->s.volume);
        out_r[i] = dsp_tanh(r * e->s.volume);
        peak = peak_of(peak, out_l[i]);
        peak = peak_of(peak, out_r[i]);
    }
    return peak;
}

/* Once per control period (DSP_BLOCK samples): smoothing, LFOs, and the
 * pitch/filter of every voice that sounds. Decides which voices render
 * during the period. */
static void control_tick(engine_va_t *e)
{
    va_smooth_step(e);
    va_mod_engine_tick(e);                  /* the LFOs (free, tempo-locked, triggered) and engine-wide modulation */
    if (e->s.phaser_mix > 0.001f || e->d.phaser_mix > 0.001f) {
        /* triangle 0..1 -> 150 Hz .. 150 Hz x 2^span */
        float sweep = 0.5f + 0.5f * lfo_tick(&e->lfo_phaser);
        phaser_set_corner(&e->phaser, 150.0f * dsp_exp2(sweep * e->d.phaser_span));
    }

    for (int v = 0; v < VA_VOICES; v++) {
        va_voice_t *voice = &e->voice[v];
        voice->in_period = (uint8_t)adsr_active(&voice->amp);
        voice->needs_control = 0;
        if (voice->in_period) {
            voice_control(e, voice, e->lfo1_now, e->lfo2_now);
        }
    }
    e->ctrl_left = DSP_BLOCK;
}

/* Renders n samples. n may be anything: the host can split a block at the
 * exact sample of an event (sequencer, MIDI) and the sound stays the same
 * as one call, because the control period carries over between calls. */
void engine_va_render(void *ctx, float *out_l, float *out_r, int n)
{
    engine_va_t *e = (engine_va_t *)ctx;
    int offset = 0;

    while (offset < n) {
        if (e->ctrl_left <= 0) {
            control_tick(e);
        }
        int count = n - offset;
        if (count > e->ctrl_left) {
            count = e->ctrl_left;
        }

        /* voices */
        float *mix = e->mixbuf;
        int voices = 0;
        for (int i = 0; i < count; i++) {
            mix[i] = 0.0f;
        }
        for (int v = 0; v < VA_VOICES; v++) {
            va_voice_t *voice = &e->voice[v];
            if (!voice->in_period) {
                continue;
            }
            if (voice->needs_control) {
                voice->needs_control = 0;
                voice_control(e, voice, e->lfo1_now, e->lfo2_now);
            }
            va_render_voice(e, voice, mix, count);
            voices++;
        }

        if (voices == 0 && e->quiet >= e->sleep_after) {
            /* asleep: nothing to hear, nothing left in the delay line */
            for (int i = 0; i < count; i++) {
                out_l[offset + i] = 0.0f;
                out_r[offset + i] = 0.0f;
            }
        } else {
            float peak = render_effects(e, mix, out_l + offset, out_r + offset, count);
            if (voices == 0 && peak < SILENCE) {
                e->quiet += (uint32_t)count;
            } else {
                e->quiet = 0;
            }
        }

        e->ctrl_left -= count;
        e->since_clock += (uint32_t)count;
        offset += count;
    }
}
