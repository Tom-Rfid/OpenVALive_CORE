/*
 * OpenVALive - Live Core
 * clock.c : the master clock, see clock.h.
 */
#include "clock.h"
#include "live_types.h"

#define MIN_BPM 20.0f
#define MAX_BPM 300.0f

static float ticks_per_sample(float bpm, float sample_rate)
{
    return bpm * (float)OLS_PPQN / (60.0f * sample_rate);
}

void clock_init(ols_clock_t *c, float sample_rate, float bpm)
{
    ols_zero(c, sizeof *c);
    c->sample_rate = sample_rate;
    clock_set_bpm(c, bpm);
    c->measured_bpm = c->bpm;
}

void clock_set_bpm(ols_clock_t *c, float bpm)
{
    if (bpm < MIN_BPM) {
        bpm = MIN_BPM;
    }
    if (bpm > MAX_BPM) {
        bpm = MAX_BPM;
    }
    c->bpm = bpm;
    if (!c->external) {
        c->ticks_per_sample = ticks_per_sample(bpm, c->sample_rate);
    }
}

void clock_set_external(ols_clock_t *c, int on)
{
    c->external = (uint8_t)(on != 0);
    c->running = 0;
    c->ext_started = 0;
    c->have_pulse = 0;
    if (!c->external) {
        c->ticks_per_sample = ticks_per_sample(c->bpm, c->sample_rate);
    }
}

void clock_start(ols_clock_t *c)
{
    c->tick = 0;
    c->frac = 0.0f;
    c->running = 1;
    c->pending = 1;              /* tick 0 plays on the very first sample */
}

void clock_continue(ols_clock_t *c)
{
    c->running = 1;
}

void clock_halt(ols_clock_t *c)
{
    c->running = 0;
    c->pending = 0;
}

void clock_locate(ols_clock_t *c, uint32_t tick, float frac)
{
    c->tick = tick;
    c->frac = frac;
    c->pending = 0;
}

float clock_bpm(const ols_clock_t *c)
{
    return c->external ? c->measured_bpm : c->bpm;
}

/* External clock: may the transport move on to the next tick yet? */
static int next_tick_allowed(const ols_clock_t *c)
{
    return !c->external || (c->ext_started && c->tick + 1u < c->granted);
}

uint32_t clock_samples_to_tick(const ols_clock_t *c, uint32_t limit)
{
    if (!c->running || !next_tick_allowed(c) || c->ticks_per_sample <= 0.0f) {
        return limit;
    }
    float need = (1.0f - c->frac) / c->ticks_per_sample;
    uint32_t n = (uint32_t)need;
    if ((float)n < need) {
        n++;                     /* round up: the tick starts on the sample after */
    }
    if (n < 1u) {
        n = 1u;
    }
    return n < limit ? n : limit;
}

static void advance_free(ols_clock_t *c, uint32_t n)
{
    float rate = c->external ? c->ticks_per_sample : ticks_per_sample(c->bpm, c->sample_rate);
    c->free_frac += (float)n * rate;
    while (c->free_frac >= 1.0f) {
        c->free_frac -= 1.0f;
        c->free_tick++;
    }
}

int clock_advance(ols_clock_t *c, uint32_t n)
{
    advance_free(c, n);
    if (c->external) {
        c->samples_since_pulse += n;
    }
    if (!c->running) {
        return 0;
    }

    c->frac += (float)n * c->ticks_per_sample;

    if (!next_tick_allowed(c)) {
        /* waiting for the next MIDI clock pulse: hold just before the tick */
        if (c->frac > 0.999f) {
            c->frac = 0.999f;
        }
        return 0;
    }
    /* clock_samples_to_tick rounds up, so a tick that was due is reached;
     * the tolerance absorbs float rounding */
    if (c->frac >= 1.0f - 1e-4f) {
        c->frac -= 1.0f;
        if (c->frac < 0.0f) {
            c->frac = 0.0f;
        }
        c->tick++;
        c->pending = 1;
        return 1;
    }
    return 0;
}

/* One MIDI clock pulse: 4 more ticks allowed, and a new tempo measure.
 * `late`: it arrived that many samples before now. */
static void midi_clock_pulse(ols_clock_t *c, uint32_t late)
{
    /* time between the two pulses as they arrived, not as they were read */
    uint32_t span = c->samples_since_pulse + c->pulse_late;
    uint32_t interval = span > late ? span - late : 0u;
    int measurable = c->have_pulse && interval > 0u;
    c->pulse_late = late;
    c->have_pulse = 1;

    if (measurable && interval < (uint32_t)c->sample_rate) {
        float rate = (float)OLS_TICKS_PER_MIDI_CLOCK / (float)interval;
        /* average over a few pulses: USB timing jitters by a millisecond or so */
        c->ticks_per_sample += 0.25f * (rate - c->ticks_per_sample);
        c->measured_bpm = c->ticks_per_sample * 60.0f * c->sample_rate / (float)OLS_PPQN;
    }
    c->samples_since_pulse = 0;

    if (!c->running) {
        return;
    }
    if (!c->ext_started) {
        /* the first pulse after START is the downbeat */
        c->ext_started = 1;
        c->tick = 0;
        c->frac = 0.0f;
        c->granted = OLS_TICKS_PER_MIDI_CLOCK;
        c->pending = 1;
        return;
    }
    c->granted += OLS_TICKS_PER_MIDI_CLOCK;

    /* more than a pulse behind (a tempo jump): catch up at once */
    if (c->tick + OLS_TICKS_PER_MIDI_CLOCK < c->granted - OLS_TICKS_PER_MIDI_CLOCK) {
        c->tick = c->granted - OLS_TICKS_PER_MIDI_CLOCK;
        c->frac = 0.0f;
        c->pending = 1;
    }
}

void clock_midi_realtime(ols_clock_t *c, uint8_t byte)
{
    clock_midi_realtime_late(c, byte, 0u);
}

void clock_midi_realtime_late(ols_clock_t *c, uint8_t byte, uint32_t late)
{
    if (!c->external) {
        return;
    }
    switch (byte) {
    case 0xF8:
        midi_clock_pulse(c, late);
        break;
    case 0xFA:                   /* start: wait for the first pulse */
        c->running = 1;
        c->ext_started = 0;
        c->pending = 0;
        break;
    case 0xFB:                   /* continue from the current position */
        c->running = 1;
        c->ext_started = 1;
        c->granted = c->tick + 1u;
        break;
    case 0xFC:
        clock_halt(c);
        break;
    default:
        break;
    }
}
