/*
 * OpenVALive - Live Core
 * pattern.c : pattern editing, see pattern.h.
 */
#include "pattern.h"

uint32_t pattern_valid_length(uint32_t length)
{
    uint32_t steps = (length + OLS_TICKS_PER_STEP / 2u) / OLS_TICKS_PER_STEP;
    if (steps < 1u) {
        steps = 1u;
    }
    if (steps > OLS_MAX_BARS * OLS_STEPS_PER_BAR) {
        steps = OLS_MAX_BARS * OLS_STEPS_PER_BAR;
    }
    return steps * OLS_TICKS_PER_STEP;
}

void pattern_clear(ols_pattern_t *p, uint32_t length)
{
    p->length = (uint16_t)pattern_valid_length(length);
    p->count = 0;
}

void pattern_set_length(ols_pattern_t *p, uint32_t length)
{
    p->length = (uint16_t)pattern_valid_length(length);

    /* drop the notes that now start after the end, keep the order */
    uint16_t kept = 0;
    for (uint16_t i = 0; i < p->count; i++) {
        if (p->n[i].tick < p->length) {
            p->n[kept++] = p->n[i];
        }
    }
    p->count = kept;
}

/* a pattern made longer plays its old length again in the new bars (no hole), as far as the notes go */
void pattern_repeat(ols_pattern_t *p, uint32_t old_length)
{
    uint16_t n = p->count;
    if (old_length == 0u || p->length <= old_length) {
        return;
    }
    for (uint32_t off = old_length; off < p->length; off += old_length) {
        for (uint16_t i = 0; i < n && p->count < OLS_PATTERN_NOTES; i++) {
            uint32_t tick = p->n[i].tick + off;
            if (tick >= p->length) {
                continue;
            }
            p->n[p->count] = p->n[i];
            p->n[p->count].tick = (uint16_t)tick;
            p->count++;
        }
    }
}

uint32_t pattern_wrap(const ols_pattern_t *p, int32_t tick)
{
    int32_t len = (int32_t)p->length;
    int32_t t = tick % len;
    return (uint32_t)(t < 0 ? t + len : t);
}

int pattern_find(const ols_pattern_t *p, uint32_t tick, uint8_t note)
{
    for (uint16_t i = 0; i < p->count; i++) {
        if (p->n[i].tick == tick && p->n[i].note == note) {
            return (int)i;
        }
    }
    return -1;
}

int pattern_add(ols_pattern_t *p, uint32_t tick, uint32_t len, uint8_t note, uint8_t vel)
{
    tick = pattern_wrap(p, (int32_t)tick);
    if (len < 1u) {
        len = 1u;
    }
    if (len > p->length) {
        len = p->length;
    }
    if (vel < 1u) {
        vel = 1u;
    }

    ols_note_t n;
    n.tick = (uint16_t)tick;
    n.len = (uint16_t)len;
    n.note = note & 0x7Fu;
    n.vel = vel > 127u ? 127u : vel;
    n.chance = 100;
    n.ratchet = 1;

    int same = pattern_find(p, tick, n.note);
    if (same >= 0) {
        p->n[same] = n;
        return same;
    }
    if (p->count >= OLS_PATTERN_NOTES) {
        return -1;
    }

    /* insertion: shift the later notes up by one */
    int i = (int)p->count;
    while (i > 0 && p->n[i - 1].tick > n.tick) {
        p->n[i] = p->n[i - 1];
        i--;
    }
    p->n[i] = n;
    p->count++;
    return i;
}

void pattern_remove(ols_pattern_t *p, int index)
{
    if (index < 0 || index >= (int)p->count) {
        return;
    }
    for (int i = index; i + 1 < (int)p->count; i++) {
        p->n[i] = p->n[i + 1];
    }
    p->count--;
}

void pattern_sort(ols_pattern_t *p)
{
    for (int i = 1; i < (int)p->count; i++) {
        ols_note_t n = p->n[i];
        int j = i;
        while (j > 0 && (p->n[j - 1].tick > n.tick
                         || (p->n[j - 1].tick == n.tick && p->n[j - 1].note > n.note))) {
            p->n[j] = p->n[j - 1];
            j--;
        }
        p->n[j] = n;
    }
}

void pattern_copy(ols_pattern_t *dst, const ols_pattern_t *src)
{
    dst->length = src->length;
    dst->count = src->count;
    for (uint16_t i = 0; i < src->count; i++) {
        dst->n[i] = src->n[i];
    }
}
