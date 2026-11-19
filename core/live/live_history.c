/*
 * OpenVALive - Live Core
 * live_history.c : undo / redo, and the gestures that change a whole pattern
 * (clear, Creative Engine, Euclid). Each of them saves an undo step first.
 *
 * The history is one ring of 8 steps, oldest to newest. Each step knows its track: a track's own undo takes its
 * newest step that is not undone, its redo the oldest one that is, so each track has a history of its own inside the
 * shared ring (the selected track's UNDO leaves the other tracks alone); OLS_TRACK_ALL takes any track's. Undo swaps
 * the saved pattern with the track's, so the same slot then holds what redo needs: no second buffer, 8 x 0.8 KB.
 */
#include "live_internal.h"

static ols_undo_step_t *slot(ols_live_t *L, uint32_t i)
{
    return &L->undo[i % OLS_UNDO_DEPTH];
}

static const ols_undo_step_t *slot_c(const ols_live_t *L, uint32_t i)
{
    return &L->undo[i % OLS_UNDO_DEPTH];
}

static void swap_patterns(ols_live_t *L, ols_pattern_t *a, ols_pattern_t *b)
{
    pattern_copy(&L->scratch, a);
    pattern_copy(a, b);
    pattern_copy(b, &L->scratch);
}

/* a step of `track` (OLS_TRACK_ALL: of any track) */
static int step_of(const ols_undo_step_t *s, int track)
{
    return s->track != OLS_UNDO_GONE && (track == (int)OLS_TRACK_ALL || s->track == track);
}

/* track: a track number, OLS_TRACK_CURRENT, or OLS_TRACK_ALL (kept as it is); -1 if none */
static int history_track(const ols_live_t *L, int track)
{
    return track == (int)OLS_TRACK_ALL ? track : live_track_index(L, track);
}

/* the newest step not undone (undo = 1) or the oldest undone (undo = 0) of the track, or -1 */
static int64_t find_step(const ols_live_t *L, int track, int undo)
{
    if (undo) {
        for (uint32_t i = L->undo_top; i > L->undo_base; i--) {
            const ols_undo_step_t *s = slot_c(L, i - 1u);
            if (step_of(s, track) && !s->undone) {
                return (int64_t)(i - 1u);
            }
        }
    } else {
        for (uint32_t i = L->undo_base; i < L->undo_top; i++) {
            const ols_undo_step_t *s = slot_c(L, i);
            if (step_of(s, track) && s->undone) {
                return (int64_t)i;
            }
        }
    }
    return -1;
}

void ols_live_undo_push(ols_live_t *L, int track)
{
    int t = live_track_index(L, track);
    if (t < 0) {
        return;
    }
    /* a new gesture forgets the track's redo steps */
    for (uint32_t i = L->undo_base; i < L->undo_top; i++) {
        ols_undo_step_t *old = slot(L, i);
        if (old->track == t && old->undone) {
            old->track = OLS_UNDO_GONE;
        }
    }
    ols_undo_step_t *s = slot(L, L->undo_top);
    s->track = (uint8_t)t;
    s->undone = 0;
    pattern_copy(&s->pat, &L->track[t].pat);
    L->undo_top++;
    if (L->undo_top - L->undo_base > OLS_UNDO_DEPTH) {
        L->undo_base++;                    /* the oldest step falls off */
    }
}

int ols_live_can_undo_track(const ols_live_t *L, int track)
{
    int t = history_track(L, track);
    return t >= 0 && find_step(L, t, 1) >= 0;
}

int ols_live_can_redo_track(const ols_live_t *L, int track)
{
    int t = history_track(L, track);
    return t >= 0 && find_step(L, t, 0) >= 0;
}

int ols_live_can_undo(const ols_live_t *L)
{
    return ols_live_can_undo_track(L, OLS_TRACK_ALL);
}

int ols_live_can_redo(const ols_live_t *L)
{
    return ols_live_can_redo_track(L, OLS_TRACK_ALL);
}

static void restore(ols_live_t *L, ols_undo_step_t *s)
{
    ols_track_t *tr = &L->track[s->track];
    live_sequencer_notes_off(L, tr);
    swap_patterns(L, &s->pat, &tr->pat);
    ols_zero(tr->skip, sizeof tr->skip);
    s->undone = (uint8_t)!s->undone;
    L->changes++;
}

OLS_COLD int ols_live_undo_track(ols_live_t *L, int track)
{
    int t = history_track(L, track);
    int64_t i = t >= 0 ? find_step(L, t, 1) : -1;
    if (i < 0) {
        return 0;
    }
    restore(L, slot(L, (uint32_t)i));
    return 1;
}

OLS_COLD int ols_live_redo_track(ols_live_t *L, int track)
{
    int t = history_track(L, track);
    int64_t i = t >= 0 ? find_step(L, t, 0) : -1;
    if (i < 0) {
        return 0;
    }
    restore(L, slot(L, (uint32_t)i));
    return 1;
}

int ols_live_undo(ols_live_t *L)
{
    return ols_live_undo_track(L, OLS_TRACK_ALL);
}

int ols_live_redo(ols_live_t *L)
{
    return ols_live_redo_track(L, OLS_TRACK_ALL);
}

void ols_live_clear(ols_live_t *L, int track)
{
    int t = live_track_index(L, track);
    if (t < 0) {
        return;
    }
    ols_live_undo_push(L, t);
    live_sequencer_notes_off(L, &L->track[t]);
    pattern_clear(&L->track[t].pat, L->track[t].pat.length);
    L->changes++;
}

OLS_COLD void ols_live_creative(ols_live_t *L, int track, ols_creative_op_t op, float amount, int32_t arg)
{
    int t = live_track_index(L, track);
    if (t < 0 || op >= CREATIVE_COUNT) {
        return;
    }
    if (amount < 0.0f) {
        amount = 0.0f;
    }
    if (amount > 1.0f) {
        amount = 1.0f;
    }
    ols_live_undo_push(L, t);
    live_sequencer_notes_off(L, &L->track[t]);
    creative_apply(&L->track[t].pat, op, amount, arg, &L->rng);
    ols_zero(L->track[t].skip, sizeof L->track[t].skip);
    L->changes++;
}

OLS_COLD void ols_live_euclid(ols_live_t *L, int track, uint32_t hits, uint32_t steps)
{
    int t = live_track_index(L, track);
    if (t < 0) {
        return;
    }
    ols_track_t *tr = &L->track[t];
    ols_live_undo_push(L, t);
    live_sequencer_notes_off(L, tr);
    creative_euclid(&tr->pat, hits, steps, tr->last_note, tr->last_vel);
    L->changes++;
}
