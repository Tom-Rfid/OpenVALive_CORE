/*
 * OpenVALive - VA engine: modulation matrix and LFO timing.
 *
 * Four slots, each "source -> destination x amount", computed at control rate (every DSP_BLOCK samples), like
 * the LFOs: a slot costs a few multiply-adds per voice and per control period, nothing per sample. With every
 * slot off, nothing here runs and the sound is exactly the one without the matrix.
 *
 * LFO timing: FREE runs on its rate knob; a SYNC length locks the cycle to the tempo (the phase comes from the
 * clock's beats, so it stays on the bar); RETRIG restarts the cycle at the first key of a phrase; ONE plays one
 * cycle per first key and holds its last value (an extra envelope).
 */
#include "engine_va_internal.h"

const char *const VA_MOD_SRC_NAMES[VA_SRC_COUNT] = {
    "OFF", "LFO1", "LFO2", "F.ENV", "A.ENV", "VELO", "PRESS", "KEY", "RANDOM", "MOTION"
};
const char *const VA_MOD_DST_NAMES[VA_DST_COUNT] = {
    "OFF", "PITCH", "OSC2", "PW", "CUTOFF", "RESO", "LEVEL", "MIX", "F.ENV", "L1RATE", "L2RATE", "DELAY"
};
const char *const VA_LFO_SYNC_NAMES[VA_LFO_SYNCS] = {
    "FREE", "4 BARS", "2 BARS", "1 BAR", "1/2.", "1/2", "1/4.", "1/4", "1/8.", "1/8", "1/4T", "1/16", "1/8T", "1/32"
};
const char *const VA_LFO_TRIG_NAMES[VA_TRIG_COUNT] = { "FREE", "RETRIG", "ONE" };

/* beats per cycle for each SYNC choice */
static const float SYNC_BEATS[VA_LFO_SYNCS] = {
    0.0f, 16.0f, 8.0f, 4.0f, 3.0f, 2.0f, 1.5f, 1.0f, 0.75f, 0.5f, 2.0f / 3.0f, 0.25f, 1.0f / 3.0f, 0.125f
};

static int choice_of(float value01, int count)
{
    int i = (int)(value01 * (float)count);
    return i >= count ? count - 1 : i;
}

/* Reads the matrix and LFO timing parameters into e->d. */
void va_mod_update(engine_va_t *e)
{
    va_derived_t *d = &e->d;
    d->mod_voice = 0;
    d->mod_engine = 0;
    for (int s = 0; s < VA_MOD_SLOTS; s++) {
        uint16_t base = (uint16_t)(VA_P_MOD1_SRC + 3 * s);
        d->mod_src[s] = (uint8_t)choice_of(e->p[base], VA_SRC_COUNT);
        d->mod_dst[s] = (uint8_t)choice_of(e->p[base + 1], VA_DST_COUNT);
        d->mod_amt[s] = 2.0f * e->p[base + 2] - 1.0f;
        if (d->mod_src[s] == VA_SRC_OFF || d->mod_dst[s] == VA_DST_OFF || d->mod_amt[s] == 0.0f) {
            continue;
        }
        if (d->mod_dst[s] >= VA_DST_LFO1_RATE) {
            d->mod_engine = 1;
        } else {
            d->mod_voice = 1;
        }
    }
    for (int w = 0; w < 2; w++) {
        d->lfo_sync[w] = SYNC_BEATS[choice_of(e->p[w ? VA_P_LFO2_SYNC : VA_P_LFO1_SYNC], VA_LFO_SYNCS)];
        d->lfo_trig[w] = (uint8_t)choice_of(e->p[w ? VA_P_LFO2_TRIG : VA_P_LFO1_TRIG], VA_TRIG_COUNT);
    }
}

/* ------------------------------------------------------------ the clock and the motion lane */

void engine_va_clock(void *ctx, float bpm, float beats)
{
    engine_va_t *e = (engine_va_t *)ctx;
    e->bpm = bpm;
    e->beats = beats;
    e->since_clock = 0;
}

void engine_va_motion(void *ctx, float value01)
{
    engine_va_t *e = (engine_va_t *)ctx;
    e->motion = dsp_clampf(value01, 0.0f, 1.0f);
}

static float beats_now(const engine_va_t *e)
{
    return e->beats + e->bpm * (1.0f / 60.0f) * DSP_INV_SR * (float)e->since_clock;
}

/* The first key of a phrase: LFOs set to RETRIG or ONE start their cycle again. */
void va_lfo_trigger(engine_va_t *e)
{
    dsp_lfo_t *lfo[2] = { &e->lfo1, &e->lfo2 };
    for (int w = 0; w < 2; w++) {
        if (e->d.lfo_trig[w] == VA_TRIG_FREE) {
            continue;
        }
        lfo[w]->phase = 0.0f;
        lfo[w]->sh_value = dsp_noise(&lfo[w]->rng);
        e->lfo_origin[w] = beats_now(e);
        e->lfo_done[w] = 0;
    }
}

/* ------------------------------------------------------------ control rate */

/* The value of one global source (the per-voice ones are 0 here: they have no single value). */
static float global_source(const engine_va_t *e, int src)
{
    switch (src) {
    case VA_SRC_LFO1:     return e->lfo1_now;
    case VA_SRC_LFO2:     return e->lfo2_now;
    case VA_SRC_PRESSURE: return e->s.pressure;
    case VA_SRC_MOTION:   return e->motion;
    default:              return 0.0f;
    }
}

/* One LFO for this control period: free, tempo-locked, triggered or one cycle. */
static float lfo_step(engine_va_t *e, int w, float rate_mod)
{
    dsp_lfo_t *l = w ? &e->lfo2 : &e->lfo1;
    const float sync = e->d.lfo_sync[w];
    const uint8_t trig = e->d.lfo_trig[w];

    if (sync > 0.0f) {
        float pos = beats_now(e) - (trig == VA_TRIG_FREE ? 0.0f : e->lfo_origin[w]);
        if (pos < 0.0f) {
            pos += 48.0f;                        /* the clock's beats wrap every 48 (live.c) */
        }
        float cycles = pos / sync;
        if (trig == VA_TRIG_ONESHOT && (cycles >= 1.0f || e->lfo_done[w])) {
            e->lfo_done[w] = 1;
            return lfo_follow(l, 0.9999f);
        }
        return lfo_follow(l, cycles - dsp_floorf(cycles));
    }
    if (trig == VA_TRIG_ONESHOT) {
        if (!e->lfo_done[w]) {
            l->phase += e->lfo_inc[w] * rate_mod;
            if (l->phase >= 1.0f) {
                l->phase = 0.9999f;
                e->lfo_done[w] = 1;
            }
        }
        return lfo_shape(l);
    }
    if (rate_mod != 1.0f) {
        l->inc = e->lfo_inc[w] * rate_mod;
    }
    float v = lfo_tick(l);
    l->inc = e->lfo_inc[w];
    return v;
}

static int lfo_plain(const engine_va_t *e, int w)
{
    return e->d.lfo_sync[w] == 0.0f && e->d.lfo_trig[w] == VA_TRIG_FREE;
}

/* Once per control period, before the voices: the LFOs, then the engine-wide destinations. */
void va_mod_engine_tick(engine_va_t *e)
{
    const va_derived_t *d = &e->d;
    float rate[2] = { 1.0f, 1.0f };

    if (d->mod_engine) {
        /* the rate modulation uses the LFO values of the period before: one period late, inaudible */
        for (int i = VA_DST_LFO1_RATE; i < VA_DST_COUNT; i++) {
            e->mod_engine_now[i] = 0.0f;
        }
        for (int s = 0; s < VA_MOD_SLOTS; s++) {
            if (d->mod_dst[s] >= VA_DST_LFO1_RATE) {
                e->mod_engine_now[d->mod_dst[s]] += global_source(e, d->mod_src[s]) * d->mod_amt[s];
            }
        }
        rate[0] = dsp_exp2(3.0f * dsp_clampf(e->mod_engine_now[VA_DST_LFO1_RATE], -1.0f, 1.0f));
        rate[1] = dsp_exp2(3.0f * dsp_clampf(e->mod_engine_now[VA_DST_LFO2_RATE], -1.0f, 1.0f));
    }

    e->lfo1_now = lfo_plain(e, 0) && rate[0] == 1.0f ? lfo_tick(&e->lfo1) : lfo_step(e, 0, rate[0]);
    e->lfo2_now = lfo_plain(e, 1) && rate[1] == 1.0f ? lfo_tick(&e->lfo2) : lfo_step(e, 1, rate[1]);
}

/* The modulation of one voice for this period, by destination (zero where nothing aims). */
void va_mod_voice(engine_va_t *e, va_voice_t *v, float m[VA_DST_COUNT])
{
    const va_derived_t *d = &e->d;
    for (int i = 0; i < VA_DST_COUNT; i++) {
        m[i] = 0.0f;
    }
    for (int s = 0; s < VA_MOD_SLOTS; s++) {
        int dst = d->mod_dst[s];
        if (dst == VA_DST_OFF || dst >= VA_DST_LFO1_RATE) {
            continue;
        }
        float x;
        switch (d->mod_src[s]) {
        case VA_SRC_FENV:     x = v->fenv.y; break;
        case VA_SRC_AENV:     x = v->amp.y; break;
        case VA_SRC_VELOCITY: x = v->accent; break;
        case VA_SRC_KEY:      x = (v->note_cur - 60.0f) * (1.0f / 48.0f); break;
        case VA_SRC_RANDOM:   x = v->rnd; break;
        default:              x = global_source(e, d->mod_src[s]); break;
        }
        m[dst] += x * d->mod_amt[s];
    }
}
