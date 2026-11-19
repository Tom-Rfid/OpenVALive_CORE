/*
 * OpenVALive - Live Core
 * live_types.h : time units, notes and patterns shared by the Live Core.
 *
 * Time is counted in ticks: 96 per quarter note (PPQN), so a 16th note
 * ("step") is 24 ticks and a 4/4 bar is 384 ticks. At 120 BPM one tick is
 * 5.2 ms: fine enough for human timing, swing and ratchets.
 *
 * Everything here is plain data: no allocation, no C library, fixed sizes
 * known at compile time. A pattern is about 0.8 KB.
 */
#ifndef OLS_LIVE_TYPES_H
#define OLS_LIVE_TYPES_H

#include <stdint.h>

#define OLS_PPQN 96u                       /* ticks per quarter note */
#define OLS_TICKS_PER_STEP 24u             /* a 16th note */
#define OLS_STEPS_PER_BAR 16u
#define OLS_TICKS_PER_BAR 384u             /* 4/4 */
#define OLS_MAX_BARS 8u
#define OLS_MAX_TICKS (OLS_MAX_BARS * OLS_TICKS_PER_BAR)

#ifndef OLS_TRACKS
#define OLS_TRACKS 4                       /* tracks of the Live Core */
#endif
#ifndef OLS_PATTERN_NOTES
#define OLS_PATTERN_NOTES 96               /* notes per track pattern */
#endif
#ifndef OLS_MAX_PARAMS
#define OLS_MAX_PARAMS 61                  /* engine parameters kept by scenes and projects (the VA's 61;
                                            * a full live set must still fit two FM-1 flash objects) */
#endif

/* One note of a pattern: 8 bytes. */
typedef struct {
    uint16_t tick;      /* start, 0 .. length - 1 */
    uint16_t len;       /* duration in ticks, at least 1 */
    uint8_t note;       /* MIDI note */
    uint8_t vel;        /* velocity 1..127 */
    uint8_t chance;     /* probability to play, 0..100 % (100: always) */
    uint8_t ratchet;    /* hits inside the note: 1 (normal) .. 4 */
} ols_note_t;

/* A loop of notes, sorted by start tick. */
typedef struct {
    uint16_t length;                        /* ticks, a whole number of steps */
    uint16_t count;                         /* notes used */
    ols_note_t n[OLS_PATTERN_NOTES];
} ols_pattern_t;

/* Control-side code (commands, gestures, saving): built for size where the compiler can (clang's minsize), so the
 * small flashes keep room; never on what runs per sample or per tick. The results do not change. */
#if defined(__clang__)
#define OLS_COLD __attribute__((minsize))
#else
#define OLS_COLD
#endif

/* Sets n bytes to zero. A loop rather than memset: no C library on some targets. */
static inline void ols_zero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;
    for (uint32_t i = 0; i < n; i++) {
        b[i] = 0;
    }
}

/* Small fast random generator (xorshift32). The state must never be 0. */
static inline uint32_t ols_rand(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/* Random integer in [0, n). */
static inline uint32_t ols_rand_below(uint32_t *state, uint32_t n)
{
    return n ? ols_rand(state) % n : 0;
}

/* Random float in [0, 1). */
static inline float ols_rand01(uint32_t *state)
{
    return (float)(ols_rand(state) >> 8) * (1.0f / 16777216.0f);
}

#endif /* OLS_LIVE_TYPES_H */
