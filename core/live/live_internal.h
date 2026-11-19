/*
 * OpenVALive - Live Core
 * live_internal.h : helpers shared by the live_*.c files (not a public API).
 */
#ifndef OLS_LIVE_INTERNAL_H
#define OLS_LIVE_INTERNAL_H

#include "live.h"
#include "pattern.h"

/* A track number from a command: OLS_TRACK_CURRENT means the selected one. Returns -1 if invalid. */
static inline int live_track_index(const ols_live_t *L, int track)
{
    if (track == (int)OLS_TRACK_CURRENT) {
        return L->current;
    }
    return (track >= 0 && track < L->track_count) ? track : -1;
}

/* Where a track's pattern is at transport tick t (stutter included). */
static inline uint32_t live_track_pos(const ols_live_t *L, const ols_track_t *tr, uint32_t t)
{
    uint32_t length = tr->pat.length;
    if (L->stutter && tr->stutter && t >= L->stutter_start) {
        uint32_t into = (t - L->stutter_start) % L->stutter_len;
        return (tr->stutter_from + into) % length;
    }
    return t % length;
}

static inline void live_midi_out(ols_live_t *L, uint8_t status, uint8_t d1, uint8_t d2)
{
    if (L->midi_out != 0) {
        L->midi_out(L->midi_out_ctx, status, d1, d2);
    }
}

static inline float live_param(const ols_track_t *tr, uint16_t id)
{
    return tr->engine.api->get_param(tr->engine.ctx, id);
}

static inline void live_set_param(ols_track_t *tr, uint16_t id, float value)
{
    tr->engine.api->set_param(tr->engine.ctx, id, value);
}

static inline uint16_t live_param_count(const ols_track_t *tr)
{
    uint16_t n = tr->engine.api->param_count;
    return n < OLS_MAX_PARAMS ? n : (uint16_t)OLS_MAX_PARAMS;
}

/* live.c */
void live_sequencer_notes_off(ols_live_t *L, ols_track_t *tr);

/* live_scenes.c */
void live_stutter_end(ols_live_t *L);
void live_xfade_begin(ols_live_t *L, int slot, uint32_t now);
void live_xfade_step(ols_live_t *L, uint32_t now);

/* live_record.c */
void live_record_note_on(ols_live_t *L, int track, uint8_t note, uint8_t vel);
void live_record_note_off(ols_live_t *L, int track, uint8_t note);

#endif /* OLS_LIVE_INTERNAL_H */
