/*
 * OpenVALive - Live Core
 * creative.c : the Creative Engine, see creative.h.
 */
#include "creative.h"
#include "pattern.h"

const char *const CREATIVE_NAMES[CREATIVE_COUNT] = {
    "MUTATE", "HUMANIZE", "FILL", "SIMPLIFY", "ROTATE", "REVERSE",
    "TRANSPOSE", "EXPAND", "CONTRACT", "CHANCE", "RATCHET",
};

#define MAX_PITCHES 16

/* ---------------------------------------------------------------- helpers */

/* How strong a position is in the bar: 4 bar, 3 beat, 2 eighth, 1 sixteenth, 0 in between. */
static int metric_weight(uint32_t tick)
{
    if (tick % OLS_TICKS_PER_BAR == 0u) {
        return 4;
    }
    if (tick % OLS_PPQN == 0u) {
        return 3;
    }
    if (tick % (OLS_PPQN / 2u) == 0u) {
        return 2;
    }
    if (tick % OLS_TICKS_PER_STEP == 0u) {
        return 1;
    }
    return 0;
}

/* The distinct pitches of the pattern, low to high: the material a
 * mutation or a fill is allowed to use. Returns how many. */
static int collect_pitches(const ols_pattern_t *p, uint8_t *pitches)
{
    int n = 0;
    for (uint16_t i = 0; i < p->count; i++) {
        uint8_t note = p->n[i].note;
        int known = 0;
        for (int k = 0; k < n; k++) {
            if (pitches[k] == note) {
                known = 1;
            }
        }
        if (!known && n < MAX_PITCHES) {
            int k = n++;
            while (k > 0 && pitches[k - 1] > note) {
                pitches[k] = pitches[k - 1];
                k--;
            }
            pitches[k] = note;
        }
    }
    return n;
}

static uint8_t clamp_vel(int32_t v)
{
    return (uint8_t)(v < 1 ? 1 : (v > 127 ? 127 : v));
}

static int32_t random_between(uint32_t *rng, int32_t lo, int32_t hi)
{
    return lo + (int32_t)ols_rand_below(rng, (uint32_t)(hi - lo + 1));
}

/* After notes moved: back in order, and two identical notes (same start,
 * same pitch) become one. */
static void tidy(ols_pattern_t *p)
{
    pattern_sort(p);
    int kept = 0;
    for (int i = 0; i < (int)p->count; i++) {
        if (kept > 0 && p->n[kept - 1].tick == p->n[i].tick && p->n[kept - 1].note == p->n[i].note) {
            continue;
        }
        p->n[kept++] = p->n[i];
    }
    p->count = (uint16_t)kept;
}

static int step_is_empty(const ols_pattern_t *p, uint32_t step)
{
    uint32_t from = step * OLS_TICKS_PER_STEP;
    uint32_t to = from + OLS_TICKS_PER_STEP;
    for (uint16_t i = 0; i < p->count; i++) {
        if (p->n[i].tick >= from && p->n[i].tick < to) {
            return 0;
        }
    }
    return 1;
}

static int average_velocity(const ols_pattern_t *p)
{
    if (p->count == 0) {
        return 96;
    }
    int32_t sum = 0;
    for (uint16_t i = 0; i < p->count; i++) {
        sum += p->n[i].vel;
    }
    return (int)(sum / (int32_t)p->count);
}

/* ---------------------------------------------------------------- operations */

void creative_mutate(ols_pattern_t *p, uint32_t *rng, float amount)
{
    uint8_t pitches[MAX_PITCHES];
    int npitch = collect_pitches(p, pitches);

    for (uint16_t i = 0; i < p->count; i++) {
        ols_note_t *n = &p->n[i];
        if (ols_rand01(rng) >= amount * 0.6f) {
            continue;                         /* this note stays as it is */
        }
        switch (ols_rand_below(rng, 3)) {
        case 0:                               /* another pitch of the pattern */
            if (npitch > 1) {
                n->note = pitches[ols_rand_below(rng, (uint32_t)npitch)];
            } else {
                static const int8_t leaps[4] = { 12, -12, 7, -5 };
                int32_t note = (int32_t)n->note + leaps[ols_rand_below(rng, 4)];
                n->note = (uint8_t)(note < 0 ? 0 : (note > 127 ? 127 : note));
            }
            break;
        case 1: {                             /* accent or ghost */
            int32_t spread = (int32_t)(30.0f * amount) + 1;
            n->vel = clamp_vel((int32_t)n->vel + random_between(rng, -spread, spread));
            break;
        }
        default:                              /* one step earlier or later, never the downbeat */
            if (metric_weight(n->tick) < 4) {
                int32_t move = (ols_rand(rng) & 1u) ? (int32_t)OLS_TICKS_PER_STEP : -(int32_t)OLS_TICKS_PER_STEP;
                n->tick = (uint16_t)pattern_wrap(p, (int32_t)n->tick + move);
            }
            break;
        }
    }
    tidy(p);
}

void creative_humanize(ols_pattern_t *p, uint32_t *rng, float amount)
{
    int32_t timing = (int32_t)(6.0f * amount + 0.5f);       /* up to +-6 ticks: +-31 ms at 120 BPM */
    int32_t velocity = (int32_t)(18.0f * amount + 0.5f);

    for (uint16_t i = 0; i < p->count; i++) {
        ols_note_t *n = &p->n[i];
        if (timing > 0) {
            n->tick = (uint16_t)pattern_wrap(p, (int32_t)n->tick + random_between(rng, -timing, timing));
        }
        if (velocity > 0) {
            n->vel = clamp_vel((int32_t)n->vel + random_between(rng, -velocity, velocity));
        }
    }
    tidy(p);
}

void creative_fill(ols_pattern_t *p, uint32_t *rng, float amount)
{
    uint8_t pitches[MAX_PITCHES];
    int npitch = collect_pitches(p, pitches);
    if (npitch == 0) {
        pitches[0] = 60;
        npitch = 1;
    }

    uint32_t steps = p->length / OLS_TICKS_PER_STEP;
    uint32_t empty = 0;
    for (uint32_t s = 0; s < steps; s++) {
        empty += (uint32_t)step_is_empty(p, s);
    }
    uint32_t wanted = (uint32_t)((float)empty * amount * 0.5f + 0.5f);
    if (wanted == 0 && amount > 0.0f && empty > 0) {
        wanted = 1;
    }

    int base_vel = average_velocity(p);
    uint32_t tries = steps * 4u;

    /* a fill grows towards the end of the loop: later steps are more likely */
    while (wanted > 0 && tries-- > 0) {
        uint32_t a = ols_rand_below(rng, steps);
        uint32_t b = ols_rand_below(rng, steps);
        uint32_t s = a > b ? a : b;
        if (!step_is_empty(p, s)) {
            continue;
        }
        float position = (float)(s + 1u) / (float)steps;
        int vel = (int)((float)base_vel * (0.55f + 0.45f * position));
        uint8_t note = pitches[ols_rand_below(rng, (uint32_t)npitch)];
        if (pattern_add(p, s * OLS_TICKS_PER_STEP, OLS_TICKS_PER_STEP - 4u, note, clamp_vel(vel)) < 0) {
            break;                            /* pattern full */
        }
        wanted--;
    }
}

void creative_simplify(ols_pattern_t *p, float amount)
{
    uint32_t remove = (uint32_t)((float)p->count * amount * 0.75f + 0.5f);
    if (remove >= p->count) {
        remove = p->count > 0 ? p->count - 1u : 0u;  /* never empty the loop */
    }

    while (remove-- > 0) {
        /* the weakest note: off the beat and quiet */
        int weakest = -1;
        int32_t weakest_score = 0x7FFFFFFF;
        for (uint16_t i = 0; i < p->count; i++) {
            int32_t score = (int32_t)metric_weight(p->n[i].tick) * 40 + (int32_t)p->n[i].vel;
            if (score < weakest_score) {
                weakest_score = score;
                weakest = (int)i;
            }
        }
        pattern_remove(p, weakest);
    }
}

void creative_rotate(ols_pattern_t *p, int32_t steps)
{
    int32_t shift = steps * (int32_t)OLS_TICKS_PER_STEP;
    for (uint16_t i = 0; i < p->count; i++) {
        p->n[i].tick = (uint16_t)pattern_wrap(p, (int32_t)p->n[i].tick + shift);
    }
    tidy(p);
}

void creative_reverse(ols_pattern_t *p)
{
    for (uint16_t i = 0; i < p->count; i++) {
        ols_note_t *n = &p->n[i];
        /* the note now ends where it used to start */
        n->tick = (uint16_t)pattern_wrap(p, (int32_t)p->length - (int32_t)n->tick - (int32_t)n->len);
    }
    tidy(p);
}

void creative_transpose(ols_pattern_t *p, int32_t semitones)
{
    for (uint16_t i = 0; i < p->count; i++) {
        int32_t note = (int32_t)p->n[i].note + semitones;
        p->n[i].note = (uint8_t)(note < 0 ? 0 : (note > 127 ? 127 : note));
    }
    tidy(p);
}

void creative_expand(ols_pattern_t *p)
{
    uint32_t length = pattern_valid_length((uint32_t)p->length * 2u);
    int kept = 0;
    for (int i = 0; i < (int)p->count; i++) {
        uint32_t tick = (uint32_t)p->n[i].tick * 2u;
        if (tick >= length) {
            continue;                         /* already at the 8-bar limit */
        }
        ols_note_t n = p->n[i];
        n.tick = (uint16_t)tick;
        n.len = (uint16_t)((uint32_t)n.len * 2u > length ? length : (uint32_t)n.len * 2u);
        p->n[kept++] = n;
    }
    p->count = (uint16_t)kept;
    p->length = (uint16_t)length;
}

void creative_contract(ols_pattern_t *p)
{
    p->length = (uint16_t)pattern_valid_length((uint32_t)p->length / 2u);
    for (uint16_t i = 0; i < p->count; i++) {
        ols_note_t *n = &p->n[i];
        n->tick = (uint16_t)pattern_wrap(p, (int32_t)(n->tick / 2u));
        n->len = (uint16_t)(n->len > 1u ? n->len / 2u : 1u);
    }
    tidy(p);
}

void creative_chance(ols_pattern_t *p, float amount)
{
    uint8_t chance = (uint8_t)(100.0f - 60.0f * amount + 0.5f);
    for (uint16_t i = 0; i < p->count; i++) {
        /* the beats always play, so the loop keeps its shape */
        p->n[i].chance = metric_weight(p->n[i].tick) >= 3 ? 100u : chance;
    }
}

void creative_ratchet(ols_pattern_t *p, uint32_t *rng, float amount)
{
    for (uint16_t i = 0; i < p->count; i++) {
        ols_note_t *n = &p->n[i];
        if (amount <= 0.0f) {
            n->ratchet = 1;
            continue;
        }
        if (ols_rand01(rng) < amount * 0.4f) {
            n->ratchet = (uint8_t)(2u + ols_rand_below(rng, 3));
            if (n->len < OLS_TICKS_PER_STEP) {
                n->len = OLS_TICKS_PER_STEP;  /* room for the repeats */
            }
        }
    }
}

void creative_euclid(ols_pattern_t *p, uint32_t hits, uint32_t steps, uint8_t note, uint8_t vel)
{
    if (steps < 1u) {
        steps = 1u;
    }
    if (steps > OLS_MAX_BARS * OLS_STEPS_PER_BAR) {
        steps = OLS_MAX_BARS * OLS_STEPS_PER_BAR;
    }
    if (hits > steps) {
        hits = steps;
    }
    pattern_clear(p, steps * OLS_TICKS_PER_STEP);

    /* Bresenham's way to spread k hits over n steps: the same rhythms as
     * Bjorklund's algorithm (rotated), with the first step always a hit */
    for (uint32_t s = 0; s < steps; s++) {
        if ((s * hits) % steps < hits) {
            uint8_t v = (s == 0u) ? vel : clamp_vel((int32_t)vel - 16);
            pattern_add(p, s * OLS_TICKS_PER_STEP, OLS_TICKS_PER_STEP - 4u, note, v);
        }
    }
}

OLS_COLD void creative_apply(ols_pattern_t *p, ols_creative_op_t op, float amount, int32_t arg, uint32_t *rng)
{
    switch (op) {
    case CREATIVE_MUTATE:    creative_mutate(p, rng, amount); break;
    case CREATIVE_HUMANIZE:  creative_humanize(p, rng, amount); break;
    case CREATIVE_FILL:      creative_fill(p, rng, amount); break;
    case CREATIVE_SIMPLIFY:  creative_simplify(p, amount); break;
    case CREATIVE_ROTATE:    creative_rotate(p, arg); break;
    case CREATIVE_REVERSE:   creative_reverse(p); break;
    case CREATIVE_TRANSPOSE: creative_transpose(p, arg); break;
    case CREATIVE_EXPAND:    creative_expand(p); break;
    case CREATIVE_CONTRACT:  creative_contract(p); break;
    case CREATIVE_CHANCE:    creative_chance(p, amount); break;
    case CREATIVE_RATCHET:   creative_ratchet(p, rng, amount); break;
    default: break;
    }
}
