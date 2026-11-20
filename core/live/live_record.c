/*
 * OpenVALive - Live Core
 * live_record.c : recording into a track, and Capture.
 *
 * Recording (REC while playing): a note enters the pattern when its key is
 * released, with its real length. A take (REC on .. off) is one undo step. Timing is kept as played (96 PPQN), or
 * rounded to the 16th when quantize is on.
 *
 * Capture: every note played is kept in a ring of the last 128 notes,
 * recording or not, transport running or not. CAPTURE turns the last phrase
 * into the selected track's loop (a take: every note since the arm, the loop
 * lasting until it is taken):
 *   - transport running: the phrase keeps its place in the bars;
 *   - transport stopped: the first note becomes the downbeat, and the
 *     transport starts right where you are in the phrase, so the loop
 *     carries on in time with what you were playing.
 */
#include "live_internal.h"

#define PHRASE_GAP_TICKS (OLS_TICKS_PER_BAR + OLS_TICKS_PER_BAR / 2u)  /* a silence this long ends a phrase */

/* ---------------------------------------------------------------- the capture ring */

static uint32_t capture_now(const ols_live_t *L)
{
    return L->clock.running ? L->clock.tick : L->clock.free_tick;
}

static ols_capture_note_t *capture_at(ols_live_t *L, uint32_t age)
{
    /* age 0 = the newest note */
    uint32_t i = (L->cap_write + OLS_CAPTURE_NOTES - 1u - age) % OLS_CAPTURE_NOTES;
    return &L->cap[i];
}

static void capture_note_on(ols_live_t *L, int track, uint8_t note, uint8_t vel)
{
    ols_capture_note_t *c = &L->cap[L->cap_write];
    c->t = capture_now(L);
    c->len = 0;
    c->note = note;
    c->vel = vel;
    c->track = (uint8_t)track;
    c->on_grid = L->clock.running;
    L->cap_write = (uint16_t)((L->cap_write + 1u) % OLS_CAPTURE_NOTES);
    if (L->cap_count < OLS_CAPTURE_NOTES) {
        L->cap_count++;
    }
}

static void capture_note_off(ols_live_t *L, int track, uint8_t note)
{
    uint32_t now = capture_now(L);
    for (uint32_t age = 0; age < L->cap_count; age++) {
        ols_capture_note_t *c = capture_at(L, age);
        if (c->note == note && c->track == track && c->len == 0) {
            uint32_t len = now > c->t ? now - c->t : 1u;
            c->len = (uint16_t)(len > 0xFFFFu ? 0xFFFFu : len);
            return;
        }
    }
}

/* ---------------------------------------------------------------- recording */

static uint32_t quantized(const ols_pattern_t *p, uint32_t pos)
{
    uint32_t step = (pos + OLS_TICKS_PER_STEP / 2u) / OLS_TICKS_PER_STEP;
    return (step * OLS_TICKS_PER_STEP) % p->length;
}

void live_record_note_on(ols_live_t *L, int track, uint8_t note, uint8_t vel)
{
    capture_note_on(L, track, note, vel);

    ols_track_t *tr = &L->track[track];
    if (!L->recording || !L->clock.running || track != L->current
        || tr->held_count >= OLS_HELD_NOTES || L->clock.tick < L->rec_from) {
        return;                                 /* (still counting in) */
    }
    ols_held_note_t *h = &tr->held[tr->held_count++];
    h->note = note;
    h->vel = vel;
    h->start = L->clock.tick;
    h->pos = (uint16_t)live_track_pos(L, tr, L->clock.tick);
}

void live_record_note_off(ols_live_t *L, int track, uint8_t note)
{
    capture_note_off(L, track, note);

    ols_track_t *tr = &L->track[track];
    for (int i = 0; i < tr->held_count; i++) {
        ols_held_note_t h = tr->held[i];
        if (h.note != note) {
            continue;
        }
        tr->held[i] = tr->held[tr->held_count - 1];
        tr->held_count--;

        uint32_t len = L->clock.tick > h.start ? L->clock.tick - h.start : 1u;
        uint32_t pos = L->quantize ? quantized(&tr->pat, h.pos) : h.pos;

        pattern_add(&tr->pat, pos, len, note, h.vel);

        /* quantized ahead of the playhead: it would sound again right now */
        uint32_t here = live_track_pos(L, tr, L->clock.tick);
        if (pos > h.pos && pos > here) {
            tr->suppress_note = note;
            tr->suppress_tick = (uint16_t)pos;
        }
        L->changes++;
        return;
    }
}

/* ---------------------------------------------------------------- capture */

static uint32_t bars_for(uint32_t ticks)
{
    uint32_t bars = (ticks + OLS_TICKS_PER_BAR - 1u) / OLS_TICKS_PER_BAR;
    if (bars < 1u) {
        bars = 1u;
    }
    /* musicians play in 1, 2, 4 or 8 bars: 3 becomes 4, 5..7 become 8 */
    if (bars == 3u) {
        bars = 4u;
    } else if (bars > 4u) {
        bars = 8u;
    }
    return bars;
}

/* the selected track's notes in the ring, from the newest back: `take` = 0 the last phrase (a silence of 1.5 bars, or
 * a change between stopped and playing, ends it), 1 every note since ols_live_capture_arm (the loop runs to now) */
static int capture_notes(ols_live_t *L, int take)
{
    int track = L->current;

    uint32_t first = 0, count = 0;
    uint8_t grid = 0;
    uint32_t newest_t = 0, oldest_t = 0;
    for (uint32_t age = 0; age < L->cap_count; age++) {
        const ols_capture_note_t *c = capture_at(L, age);
        if (c->track != track) {
            continue;
        }
        if (count == 0) {
            grid = c->on_grid;
            newest_t = c->t;
        } else if (c->on_grid != grid || (!take && oldest_t - c->t > PHRASE_GAP_TICKS)
                   || newest_t - c->t >= OLS_MAX_TICKS) {
            break;
        }
        oldest_t = c->t;
        first = age;
        count++;
    }
    if (count == 0) {
        return 0;
    }

    uint32_t end = take ? capture_now(L) : newest_t;   /* a take: the loop lasts until it was stopped */
    if (end < newest_t || end - oldest_t >= OLS_MAX_TICKS) {
        end = newest_t;
    }
    uint32_t bars = bars_for(end - oldest_t + 1u);
    uint32_t length = bars * OLS_TICKS_PER_BAR;
    uint32_t origin = grid ? 0u : oldest_t;  /* stopped: the first note is the downbeat */

    ols_live_undo_push(L, track);
    ols_track_t *tr = &L->track[track];
    live_sequencer_notes_off(L, tr);
    pattern_clear(&tr->pat, length);

    for (uint32_t age = first + 1u; age-- > 0u;) {
        const ols_capture_note_t *c = capture_at(L, age);
        if (c->track != track || c->t < oldest_t) {
            continue;
        }
        uint32_t len = c->len ? c->len : OLS_TICKS_PER_STEP;
        pattern_add(&tr->pat, (c->t - origin) % length, len, c->note, c->vel);
    }

    if (!grid && !L->clock.external) {
        /* carry on in time: the transport starts where the phrase is now */
        uint32_t into = L->clock.free_tick - oldest_t;
        clock_locate(&L->clock, into, L->clock.free_frac);
        L->clock.running = 1;
        if (L->send_clock) {
            live_midi_out(L, 0xFA, 0, 0);
        }
    }

    L->cap_count = 0;            /* the phrase is used: the next capture starts afresh */
    L->changes++;
    return 1;
}

OLS_COLD int ols_live_capture(ols_live_t *L)
{
    return capture_notes(L, 0);
}

OLS_COLD void ols_live_capture_arm(ols_live_t *L)
{
    L->cap_count = 0;
}

OLS_COLD int ols_live_capture_take(ols_live_t *L)
{
    return capture_notes(L, 1);
}
