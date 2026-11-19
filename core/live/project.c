/*
 * OpenVALive - Live Core
 * project.c : saving and loading a live set, see project.h.
 */
#include "project.h"
#include "live_internal.h"

#define PROJECT_VERSION 1u
#define HEADER_SIZE 16u

/* ---------------------------------------------------------------- CRC-32 */

uint32_t ols_crc32(const uint8_t *data, uint32_t len)
{
    /* bitwise CRC-32 (zlib polynomial): slow but tiny, and a project is a few KB */
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

/* ---------------------------------------------------------------- writer */

typedef struct {
    uint8_t *buf;
    uint32_t max, len;
    int overflow;
} writer_t;

static void put8(writer_t *w, uint32_t v)
{
    if (w->len >= w->max) {
        w->overflow = 1;
        return;
    }
    w->buf[w->len++] = (uint8_t)v;
}

static void put16(writer_t *w, uint32_t v)
{
    put8(w, v & 0xFFu);
    put8(w, (v >> 8) & 0xFFu);
}

static void put32(writer_t *w, uint32_t v)
{
    put16(w, v & 0xFFFFu);
    put16(w, v >> 16);
}

static uint16_t param_u16(const ols_track_t *tr, uint16_t id)
{
    float v = live_param(tr, id);
    if (v <= 0.0f) {
        return 0;
    }
    if (v >= 1.0f) {
        return 65535u;
    }
    return (uint16_t)(v * 65535.0f + 0.5f);
}

static void write_track(writer_t *w, const ols_track_t *tr)
{
    uint16_t params = live_param_count(tr);
    float gain = tr->gain * 100.0f + 0.5f;

    put8(w, tr->mute);
    put8(w, gain > 255.0f ? 255u : (uint32_t)gain);
    put8(w, params);
    put8(w, tr->kind);
    put16(w, tr->pat.length);
    put16(w, tr->pat.count);
    for (uint16_t id = 0; id < params; id++) {
        put16(w, param_u16(tr, id));
    }
    for (uint16_t i = 0; i < tr->pat.count; i++) {
        const ols_note_t *n = &tr->pat.n[i];
        put16(w, n->tick);
        put16(w, n->len);
        put8(w, n->note);
        put8(w, n->vel);
        put8(w, n->chance);
        put8(w, n->ratchet);
    }
}

/* The engine kinds of a scene's tracks, 2 bits each (up to 4 kinds). */
static uint32_t scene_kinds(const ols_scene_t *scene)
{
    uint32_t packed = 0;
    for (int t = 0; t < OLS_TRACKS && t < 4; t++) {
        packed |= (uint32_t)(scene->kind[t] & 3u) << (2 * t);
    }
    return packed;
}

OLS_COLD uint32_t ols_project_save(const ols_live_t *L, uint8_t *buf, uint32_t max)
{
    writer_t w = { buf, max, 0, 0 };
    float bpm10 = L->clock.bpm * 10.0f + 0.5f;

    put8(&w, 'O');
    put8(&w, 'L');
    put8(&w, 'S');
    put8(&w, 'P');
    put8(&w, PROJECT_VERSION);
    put8(&w, L->track_count);
    put8(&w, L->quantize ? 1u : 0u);
    put8(&w, L->current);
    put16(&w, (uint32_t)bpm10);
    put8(&w, (uint32_t)(L->swing * 255.0f + 0.5f));
    put8(&w, L->scene_current);
    put8(&w, L->xfade_bars);
    put8(&w, L->metronome);
    put16(&w, 0);                               /* reserved */

    for (int t = 0; t < L->track_count; t++) {
        write_track(&w, &L->track[t]);
    }

    uint8_t valid = 0;
    for (int s = 0; s < OLS_SCENES; s++) {
        if (L->scene[s].valid) {
            valid = (uint8_t)(valid | (1u << s));
        }
    }
    put8(&w, valid);
    for (int s = 0; s < OLS_SCENES; s++) {
        if (!L->scene[s].valid) {
            continue;
        }
        put8(&w, L->scene[s].mute_mask);
        put8(&w, scene_kinds(&L->scene[s]));
        for (int t = 0; t < L->track_count; t++) {
            for (uint16_t id = 0; id < live_param_count(&L->track[t]); id++) {
                put16(&w, L->scene[s].v[t][id]);
            }
        }
    }

    put32(&w, ols_crc32(buf, w.len));
    return w.overflow ? 0u : w.len;
}

/* ---------------------------------------------------------------- reader */

typedef struct {
    const uint8_t *buf;
    uint32_t len, pos;
    int bad;
} reader_t;

static uint32_t get8(reader_t *r)
{
    if (r->pos >= r->len) {
        r->bad = 1;
        return 0;
    }
    return r->buf[r->pos++];
}

static uint32_t get16(reader_t *r)
{
    uint32_t lo = get8(r);
    return lo | get8(r) << 8;
}

/* Reads (and, when L is not 0, applies) the whole project.
 * Called twice: first to check everything, then to apply. */
OLS_COLD static int parse(ols_live_t *L, const uint8_t *buf, uint32_t len)
{
    reader_t r = { buf, len - 4u, 0, 0 };

    r.pos = 4;                                  /* "OLSP", checked by the caller */
    if (get8(&r) != PROJECT_VERSION) {
        return 0;
    }
    uint32_t tracks = get8(&r);
    uint32_t flags = get8(&r);
    uint32_t current = get8(&r);
    uint32_t bpm10 = get16(&r);
    uint32_t swing = get8(&r);
    uint32_t scene_current = get8(&r);
    uint32_t xfade = get8(&r);
    uint32_t metronome = get8(&r);
    r.pos += 2;
    if (tracks > OLS_TRACKS || bpm10 < 200u || bpm10 > 3000u) {
        return 0;
    }
    if (L != 0) {
        clock_set_bpm(&L->clock, (float)bpm10 * 0.1f);
        L->quantize = (uint8_t)(flags & 1u);
        L->swing = (float)swing * (1.0f / 255.0f);
        L->current = (uint8_t)(current < L->track_count ? current : 0u);
        L->scene_current = (uint8_t)(scene_current < OLS_SCENES ? scene_current : 0u);
        L->xfade_bars = (uint8_t)(xfade <= 8u ? xfade : 0u);
        L->metronome = (uint8_t)(metronome != 0u);
    }

    uint8_t params_of[OLS_TRACKS];
    for (uint32_t t = 0; t < tracks; t++) {
        uint32_t mute = get8(&r);
        uint32_t gain = get8(&r);
        uint32_t params = get8(&r);
        uint32_t kind = get8(&r);
        uint32_t length = get16(&r);
        uint32_t count = get16(&r);
        params_of[t] = (uint8_t)params;
        if (params > OLS_MAX_PARAMS || count > OLS_PATTERN_NOTES
            || length != pattern_valid_length(length)) {
            return 0;
        }

        ols_track_t *tr = (L != 0 && t < L->track_count) ? &L->track[t] : 0;
        if (tr != 0 && kind != tr->kind) {
            if (L->engine_for_kind == 0) {
                tr = 0;                         /* no way to switch engines here: keep this track as it is */
            } else {
                ols_live_set_engine(L, (int)t, L->engine_for_kind(L->kind_ctx, (int)t, (uint8_t)kind),
                                    (uint8_t)kind);
            }
        }
        if (tr != 0) {
            live_sequencer_notes_off(L, tr);
            tr->mute = (uint8_t)(mute != 0);
            tr->gain = (float)gain * 0.01f;
            pattern_clear(&tr->pat, length);
        }
        for (uint32_t id = 0; id < params; id++) {
            uint32_t v = get16(&r);
            if (tr != 0 && id < live_param_count(tr)) {
                live_set_param(tr, (uint16_t)id, (float)v * (1.0f / 65535.0f));
            }
        }
        for (uint32_t i = 0; i < count; i++) {
            ols_note_t n;
            n.tick = (uint16_t)get16(&r);
            n.len = (uint16_t)get16(&r);
            n.note = (uint8_t)get8(&r);
            n.vel = (uint8_t)get8(&r);
            n.chance = (uint8_t)get8(&r);
            n.ratchet = (uint8_t)get8(&r);
            if (n.tick >= length || n.len < 1u || n.note > 127u || n.chance > 100u
                || n.ratchet < 1u || n.ratchet > 4u) {
                return 0;
            }
            if (tr != 0) {
                tr->pat.n[tr->pat.count++] = n;
            }
        }
        if (tr != 0) {
            pattern_sort(&tr->pat);
        }
    }

    uint32_t valid = get8(&r);
    for (int s = 0; s < OLS_SCENES; s++) {
        ols_scene_t *scene = (L != 0) ? &L->scene[s] : 0;
        if (scene != 0) {
            scene->valid = 0;
        }
        if (!((valid >> s) & 1u)) {
            continue;
        }
        uint32_t mutes = get8(&r);
        uint32_t kinds = get8(&r);
        for (uint32_t t = 0; t < tracks; t++) {
            for (uint32_t id = 0; id < params_of[t]; id++) {
                uint32_t v = get16(&r);
                if (scene != 0 && t < L->track_count && id < OLS_MAX_PARAMS) {
                    scene->v[t][id] = (uint16_t)v;
                }
            }
        }
        if (scene != 0) {
            scene->mute_mask = (uint8_t)mutes;
            for (int t = 0; t < OLS_TRACKS && t < 4; t++) {
                scene->kind[t] = (uint8_t)((kinds >> (2 * t)) & 3u);
            }
            scene->valid = 1;
        }
    }
    return !r.bad && r.pos == r.len;
}

OLS_COLD int ols_project_load(ols_live_t *L, const uint8_t *buf, uint32_t len)
{
    if (len < HEADER_SIZE + 5u || buf[0] != 'O' || buf[1] != 'L' || buf[2] != 'S' || buf[3] != 'P') {
        return 0;
    }
    uint32_t stored = (uint32_t)buf[len - 4u] | (uint32_t)buf[len - 3u] << 8
                    | (uint32_t)buf[len - 2u] << 16 | (uint32_t)buf[len - 1u] << 24;
    if (stored != ols_crc32(buf, len - 4u)) {
        return 0;
    }
    if (!parse(0, buf, len)) {                  /* check everything before touching anything */
        return 0;
    }
    parse(L, buf, len);
    /* a loaded set starts with a clean history */
    L->undo_base = L->undo_top = 0;
    L->changes++;
    return 1;
}
