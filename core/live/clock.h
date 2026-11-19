/*
 * OpenVALive - Live Core
 * clock.h : the master clock. One clock drives every track.
 *
 * It counts ticks (96 per quarter note) from the audio sample count, so it
 * never drifts from the audio. The renderer asks "how many samples until the
 * next tick?" and splits its block there: every sequenced event lands on its
 * exact sample.
 *
 * Two sources:
 *   internal  ticks come from the tempo (20..300 BPM);
 *   external  ticks follow MIDI clock (24 pulses per quarter = 4 ticks each),
 *             played smoothly between pulses at the measured rate.
 *
 * A free-running tick counter keeps counting while the transport is stopped:
 * Capture uses it to place notes that were played before PLAY.
 */
#ifndef OLS_CLOCK_H
#define OLS_CLOCK_H

#include <stdint.h>

#define OLS_TICKS_PER_MIDI_CLOCK 4u        /* 96 PPQN / 24 PPQN */

typedef struct {
    float sample_rate;
    float bpm;                  /* internal tempo */
    float ticks_per_sample;     /* from bpm (internal) or measured (external) */

    uint8_t running;            /* transport playing */
    uint8_t external;           /* follow MIDI clock instead of the tempo */

    uint32_t tick;              /* the tick playing now, counted from PLAY */
    float frac;                 /* 0..1 inside it */
    uint8_t pending;            /* `tick` has started and its events are not played yet */

    uint32_t free_tick;         /* always counting, at the internal tempo */
    float free_frac;

    /* external clock */
    uint8_t ext_started;        /* the first pulse after START has come */
    uint32_t granted;           /* ticks allowed by the MIDI clock pulses received */
    uint32_t samples_since_pulse;
    uint32_t pulse_late;        /* how long before it was read the last pulse arrived, samples */
    uint8_t have_pulse;         /* a previous pulse exists to measure the tempo from */
    float measured_bpm;         /* tempo seen on the MIDI clock, for the screen */
} ols_clock_t;

void clock_init(ols_clock_t *c, float sample_rate, float bpm);
void clock_set_bpm(ols_clock_t *c, float bpm);
void clock_set_external(ols_clock_t *c, int on);

void clock_start(ols_clock_t *c);       /* from the beginning */
void clock_continue(ols_clock_t *c);    /* from where it stopped */
void clock_halt(ols_clock_t *c);        /* stop, keep the position */

/* Moves the transport so that the next tick played is `tick` (Capture). */
void clock_locate(ols_clock_t *c, uint32_t tick, float frac);

/* Samples until the next transport tick (>= 1), or `limit` if none comes sooner. */
uint32_t clock_samples_to_tick(const ols_clock_t *c, uint32_t limit);

/* Advances by n samples (n <= clock_samples_to_tick). Returns 1 when the
 * next transport tick starts right after these samples (`pending` is set). */
int clock_advance(ols_clock_t *c, uint32_t n);

/* MIDI real-time bytes: 0xF8 clock, 0xFA start, 0xFB continue, 0xFC stop. */
void clock_midi_realtime(ols_clock_t *c, uint8_t byte);

/* The same, for a byte that arrived `late` samples ago (a platform that
 * time-stamps incoming MIDI): the tempo is measured between the real arrival
 * times, so a clock read once per audio block does not jitter. */
void clock_midi_realtime_late(ols_clock_t *c, uint8_t byte, uint32_t late);

/* Current tempo: the internal one, or the measured MIDI clock tempo. */
float clock_bpm(const ols_clock_t *c);

#endif /* OLS_CLOCK_H */
