/*
 * OpenVALive - Live Core
 * live_scenes.c : scenes, Morph, and the hold-to-play performance effects.
 *
 * A scene is a snapshot of every parameter of every track, plus the mutes.
 * Morph glides between two scenes with one value 0..1: continuous
 * parameters are interpolated, switches (waveform, mode) flip half-way.
 * The engines smooth every change (5 ms), so a morph never clicks.
 *
 * Overrides are "hold" gestures, named by role (cutoff, drive...) so they
 * work on any engine: hold a pad, a parameter jumps to a value
 * (filter shut, delay thrown); release it, the parameter springs back.
 * Stutter repeats the last fraction of a beat on every track while held,
 * then drops back to where the music would have been (like a beat repeat).
 */
#include "live_internal.h"

/* ---------------------------------------------------------------- scenes */

static uint16_t to_u16(float v)
{
    if (v <= 0.0f) {
        return 0;
    }
    if (v >= 1.0f) {
        return 65535u;
    }
    return (uint16_t)(v * 65535.0f + 0.5f);
}

static float from_u16(uint16_t v)
{
    return (float)v * (1.0f / 65535.0f);
}

static uint8_t mute_mask(const ols_live_t *L)
{
    uint8_t mask = 0;
    for (int t = 0; t < L->track_count; t++) {
        if (L->track[t].mute) {
            mask = (uint8_t)(mask | (1u << t));
        }
    }
    return mask;
}

static void apply_mutes(ols_live_t *L, uint8_t mask)
{
    for (int t = 0; t < L->track_count; t++) {
        int mute = (mask >> t) & 1;
        if (mute != L->track[t].mute) {
            ols_live_mute(L, t, mute);
        }
    }
}

OLS_COLD void ols_live_scene_store(ols_live_t *L, int slot)
{
    if (slot < 0 || slot >= OLS_SCENES) {
        return;
    }
    ols_scene_t *s = &L->scene[slot];
    for (int t = 0; t < L->track_count; t++) {
        const ols_track_t *tr = &L->track[t];
        s->kind[t] = tr->kind;
        for (uint16_t id = 0; id < live_param_count(tr); id++) {
            s->v[t][id] = to_u16(live_param(tr, id));
        }
    }
    s->mute_mask = mute_mask(L);
    s->valid = 1;
    L->scene_current = (uint8_t)slot;
    L->changes++;
}

/* Sets a parameter only when it really changes: set_param may do work. */
static void set_if_changed(ols_track_t *tr, uint16_t id, float value)
{
    float now = live_param(tr, id);
    float diff = now - value;
    if (diff > 0.0005f || diff < -0.0005f) {
        live_set_param(tr, id, value);
    }
}

void ols_live_scene_recall(ols_live_t *L, int slot)
{
    if (slot < 0 || slot >= OLS_SCENES || !L->scene[slot].valid) {
        return;
    }
    const ols_scene_t *s = &L->scene[slot];
    for (int t = 0; t < L->track_count; t++) {
        ols_track_t *tr = &L->track[t];
        if (s->kind[t] != tr->kind) {
            continue;                       /* stored with another engine: its numbers mean nothing here */
        }
        for (uint16_t id = 0; id < live_param_count(tr); id++) {
            set_if_changed(tr, id, from_u16(s->v[t][id]));
        }
    }
    apply_mutes(L, s->mute_mask);
    L->scene_current = (uint8_t)slot;
    L->morph = 1.0f;                        /* a Morph knob now stands fully on this scene */
    L->changes++;
}

/* ---------------------------------------------------------------- morph */

static int param_is_discrete(const ols_track_t *tr, uint16_t id)
{
    const uint8_t *kinds = tr->engine.api->param_kinds;
    return kinds != 0 && kinds[id] == OLS_PARAM_DISCRETE;
}

/* The sound between scenes a and b: continuous parameters interpolated, switches half-way. */
static void morph_between(ols_live_t *L, const ols_scene_t *a, const ols_scene_t *b, float amount)
{
    for (int t = 0; t < L->track_count; t++) {
        ols_track_t *tr = &L->track[t];
        if (a->kind[t] != tr->kind || b->kind[t] != tr->kind) {
            continue;
        }
        for (uint16_t id = 0; id < live_param_count(tr); id++) {
            float va = from_u16(a->v[t][id]);
            float vb = from_u16(b->v[t][id]);
            float value;
            if (param_is_discrete(tr, id)) {
                value = amount < 0.5f ? va : vb;
            } else {
                value = va + (vb - va) * amount;
            }
            set_if_changed(tr, id, value);
        }
    }
    apply_mutes(L, amount < 0.5f ? a->mute_mask : b->mute_mask);
}

void ols_live_morph(ols_live_t *L, int scene_a, int scene_b, float amount)
{
    if (scene_a < 0 || scene_a >= OLS_SCENES || scene_b < 0 || scene_b >= OLS_SCENES) {
        return;
    }
    const ols_scene_t *a = &L->scene[scene_a];
    const ols_scene_t *b = &L->scene[scene_b];
    if (!a->valid || !b->valid) {
        return;
    }
    amount = amount < 0.0f ? 0.0f : (amount > 1.0f ? 1.0f : amount);
    L->morph_a = (uint8_t)scene_a;
    L->morph_b = (uint8_t)scene_b;
    L->morph = amount;
    morph_between(L, a, b, amount);
}

/* ---------------------------------------------------------------- transitions */

/* Captures the sound as it is now (a scene that is not stored anywhere). */
static void snapshot_now(ols_live_t *L, ols_scene_t *s)
{
    for (int t = 0; t < L->track_count; t++) {
        const ols_track_t *tr = &L->track[t];
        s->kind[t] = tr->kind;
        for (uint16_t id = 0; id < live_param_count(tr); id++) {
            s->v[t][id] = to_u16(live_param(tr, id));
        }
    }
    s->mute_mask = mute_mask(L);
    s->valid = 1;
}

void ols_live_scene_go(ols_live_t *L, int slot)
{
    if (slot < 0 || slot >= OLS_SCENES || !L->scene[slot].valid) {
        return;
    }
    if (L->clock.running) {
        L->scene_queued = (int8_t)slot;     /* on the next bar (bar_start, live.c) */
    } else {
        ols_live_scene_recall(L, slot);
    }
}

void live_xfade_begin(ols_live_t *L, int slot, uint32_t now)
{
    snapshot_now(L, &L->xfade_from);
    L->xfade_to = (uint8_t)slot;
    L->xfade_start = now;
    L->xfade_len = (uint32_t)L->xfade_bars * OLS_TICKS_PER_BAR;
    L->xfade_active = 1;
}

/* Every 4 ticks (21 ms at 120 BPM): smooth enough, since the engines glide too. */
void live_xfade_step(ols_live_t *L, uint32_t now)
{
    if (!L->xfade_active || (now - L->xfade_start) % 4u != 0u) {
        return;
    }
    uint32_t done = now - L->xfade_start;
    if (done >= L->xfade_len) {
        L->xfade_active = 0;
        ols_live_scene_recall(L, L->xfade_to);   /* lands exactly on the scene */
        return;
    }
    morph_between(L, &L->xfade_from, &L->scene[L->xfade_to], (float)done / (float)L->xfade_len);
}

/* ---------------------------------------------------------------- overrides */

static void override_one(ols_live_t *L, int t, uint16_t role, float value, int hold)
{
    ols_track_t *tr = &L->track[t];
    const uint16_t *roles = tr->engine.api->roles;
    if (roles == 0 || role >= OLS_ROLE_COUNT || roles[role] == OLS_NO_PARAM) {
        return;                             /* this engine has no such parameter (drums: no delay) */
    }
    uint16_t id = roles[role];

    /* already held on this parameter? */
    int found = -1;
    for (int i = 0; i < OLS_OVERRIDES; i++) {
        if (L->ovr[i].active && L->ovr[i].track == t && L->ovr[i].id == id) {
            found = i;
        }
    }

    if (hold) {
        if (found < 0) {
            for (int i = 0; i < OLS_OVERRIDES && found < 0; i++) {
                if (!L->ovr[i].active) {
                    found = i;
                    L->ovr[i].active = 1;
                    L->ovr[i].track = (uint8_t)t;
                    L->ovr[i].id = id;
                    L->ovr[i].base = live_param(tr, id);
                }
            }
        }
        if (found >= 0) {
            live_set_param(tr, id, value);
        }
    } else if (found >= 0) {
        live_set_param(tr, id, L->ovr[found].base);
        L->ovr[found].active = 0;
    }
}

void ols_live_override(ols_live_t *L, int track, uint16_t role, float value, int hold)
{
    if (track == (int)OLS_TRACK_ALL) {
        for (int t = 0; t < L->track_count; t++) {
            override_one(L, t, role, value, hold);
        }
        return;
    }
    int t = live_track_index(L, track);
    if (t >= 0) {
        override_one(L, t, role, value, hold);
    }
}

/* ---------------------------------------------------------------- stutter */

/* Every track loops (or lets go). */
void ols_live_stutter(ols_live_t *L, int on, uint32_t ticks)
{
    ols_live_stutter_track(L, on, ticks, OLS_TRACK_ALL, 0);
}

/* The loop stops on every track, now. */
void live_stutter_end(ols_live_t *L)
{
    for (int t = 0; t < L->track_count; t++) {
        L->track[t].stutter = 0;
    }
    L->stutter = 0;
    L->stutter_end = 0;
}

/* One loop at a time: a new one replaces the running one, on `track` or every track (OLS_TRACK_ALL).
 * Letting go waits for the end of the slice playing, so the music comes back in time (now = 1: at once). */
void ols_live_stutter_track(ols_live_t *L, int on, uint32_t ticks, int track, int now)
{
    uint32_t tick = L->clock.tick;
    if (!on) {
        if (!L->stutter) {
            return;
        }
        if (now || !L->clock.running || L->stutter_len == 0u) {
            live_stutter_end(L);
        } else {
            uint32_t into = tick >= L->stutter_start ? tick - L->stutter_start : 0u;
            L->stutter_end = L->stutter_start + (into / L->stutter_len + 1u) * L->stutter_len;
        }
        return;
    }
    if (ticks < 1u) {
        ticks = OLS_TICKS_PER_STEP;
    }
    int only = track == (int)OLS_TRACK_ALL ? -1 : live_track_index(L, track);
    if (track != (int)OLS_TRACK_ALL && only < 0) {
        return;
    }
    live_stutter_end(L);
    for (int t = 0; t < L->track_count; t++) {
        ols_track_t *tr = &L->track[t];
        /* repeat from the start of the current slice, on the grid */
        uint32_t pos = tick % tr->pat.length;
        tr->stutter_from = (uint16_t)(pos - pos % ticks);
        tr->stutter = (uint8_t)(only < 0 || only == t);
    }
    L->stutter_start = tick - tick % ticks;
    L->stutter_len = (uint16_t)ticks;
    L->stutter = 1;
}
