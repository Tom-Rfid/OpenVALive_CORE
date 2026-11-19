/*
 * OpenVALive - Live Core
 * creative.h : the Creative Engine. Musical operations on a pattern.
 *
 * The idea: never start from an empty grid. Play something, then push it
 * somewhere else with one gesture. Every operation keeps the musical
 * material of the pattern (its notes, its velocities, its groove) and only
 * reshapes it, so the result still sounds like you.
 *
 * `amount` is 0..1 ("a little" .. "a lot"). The operations are pure
 * functions on a pattern: the Live Core saves an undo step before calling
 * them (ols_live_creative), and the tests call them directly.
 *
 *   MUTATE     moves some notes to other pitches of the pattern, nudges velocities and steps
 *   HUMANIZE   small timing and velocity variations, like a player
 *   FILL       adds notes in the gaps, louder towards the end of the loop
 *   SIMPLIFY   removes the weakest notes first (quiet, off-beat), keeps the downbeats
 *   ROTATE     shifts the whole loop by steps
 *   REVERSE    plays the loop backwards
 *   TRANSPOSE  semitones up or down
 *   EXPAND     twice as long and twice as slow
 *   CONTRACT   half as long and twice as fast
 *   CHANCE     gives the off-beat notes a probability, so the loop varies by itself
 *   RATCHET    turns some notes into quick repeats (2..4 hits)
 *   EUCLID     a Euclidean rhythm: k hits spread as evenly as possible over n steps
 */
#ifndef OLS_CREATIVE_H
#define OLS_CREATIVE_H

#include "live_types.h"

typedef enum {
    CREATIVE_MUTATE,
    CREATIVE_HUMANIZE,
    CREATIVE_FILL,
    CREATIVE_SIMPLIFY,
    CREATIVE_ROTATE,         /* arg: steps, may be negative */
    CREATIVE_REVERSE,
    CREATIVE_TRANSPOSE,      /* arg: semitones */
    CREATIVE_EXPAND,
    CREATIVE_CONTRACT,
    CREATIVE_CHANCE,
    CREATIVE_RATCHET,
    CREATIVE_COUNT
} ols_creative_op_t;

extern const char *const CREATIVE_NAMES[CREATIVE_COUNT];

void creative_mutate(ols_pattern_t *p, uint32_t *rng, float amount);
void creative_humanize(ols_pattern_t *p, uint32_t *rng, float amount);
void creative_fill(ols_pattern_t *p, uint32_t *rng, float amount);
void creative_simplify(ols_pattern_t *p, float amount);
void creative_rotate(ols_pattern_t *p, int32_t steps);
void creative_reverse(ols_pattern_t *p);
void creative_transpose(ols_pattern_t *p, int32_t semitones);
void creative_expand(ols_pattern_t *p);
void creative_contract(ols_pattern_t *p);
void creative_chance(ols_pattern_t *p, float amount);
void creative_ratchet(ols_pattern_t *p, uint32_t *rng, float amount);

/* Replaces the pattern by a Euclidean rhythm of `hits` over `steps` 16ths, on one note. */
void creative_euclid(ols_pattern_t *p, uint32_t hits, uint32_t steps, uint8_t note, uint8_t vel);

/* Runs an operation by number: amount 0..1, arg for ROTATE and TRANSPOSE. */
void creative_apply(ols_pattern_t *p, ols_creative_op_t op, float amount, int32_t arg, uint32_t *rng);

#endif /* OLS_CREATIVE_H */
