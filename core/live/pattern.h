/*
 * OpenVALive - Live Core
 * pattern.h : editing a pattern (a loop of notes sorted by start tick).
 */
#ifndef OLS_PATTERN_H
#define OLS_PATTERN_H

#include "live_types.h"

/* Empties the pattern and sets its length (ticks, rounded to whole steps, 1 step .. 8 bars). */
void pattern_clear(ols_pattern_t *p, uint32_t length);

/* Changes the length; notes past the new end are removed. */
void pattern_set_length(ols_pattern_t *p, uint32_t length);
void pattern_repeat(ols_pattern_t *p, uint32_t old_length);

/* Adds a note, kept in order. A note with the same start and pitch is replaced.
 * Returns its index, or -1 if the pattern is full. */
int pattern_add(ols_pattern_t *p, uint32_t tick, uint32_t len, uint8_t note, uint8_t vel);

void pattern_remove(ols_pattern_t *p, int index);

/* Restores the order after notes were moved (insertion sort: patterns are short). */
void pattern_sort(ols_pattern_t *p);

void pattern_copy(ols_pattern_t *dst, const ols_pattern_t *src);

/* Index of the note starting at tick with this pitch, or -1. */
int pattern_find(const ols_pattern_t *p, uint32_t tick, uint8_t note);

/* Wraps any tick (even negative) into 0 .. length - 1. */
uint32_t pattern_wrap(const ols_pattern_t *p, int32_t tick);

/* Clamps a length to the allowed range and rounds it to whole steps. */
uint32_t pattern_valid_length(uint32_t length);

#endif /* OLS_PATTERN_H */
