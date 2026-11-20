/* SPDX-License-Identifier: MIT
 * OpenVALive - CLIPS: short phrases kept in slots and played back on a tap (a target with pads: APP_CLIPS).
 *
 * A clip is notes, not sound: what was played on a line (a track or A), from its first note, with that line's sound
 * then. Played back, it goes to the same line with the sound it has now, looping at its length (whole beats, at
 * most CLIP_MAX_BARS). Everything here runs on the audio side, from events (EV_CLIP): the UI arms a slot, ends the
 * take, starts and stops clips, and only reads `clips` / `crec` to draw.
 *   CLIP_ARM      b = slot, target = the line: the next note on that line starts the take
 *   CLIP_END      the take ends now (its length rounded to the beat); a take not started is given up
 *   CLIP_PLAY     b = slot: from its first note, now (a = CLIP_PLAY) or on the next beat (a = CLIP_PLAY_SYNC)
 *   CLIP_STOP     b = slot (its notes let go)
 *   CLIP_DELETE   b = slot
 * The time is the clock's free tick (always counting at the tempo), so clips play with the transport stopped too. */

#define CLIP_SLOTS 14u
#define CLIP_NOTES 48u
#define CLIP_MAX_BARS 4u
#define CLIP_MAX_TICKS (CLIP_MAX_BARS * OLS_TICKS_PER_BAR)
#define CLIP_SOUNDING 8u

enum { CLIP_ARM, CLIP_END, CLIP_PLAY, CLIP_PLAY_SYNC, CLIP_STOP, CLIP_DELETE };

typedef struct {
    uint16_t tick, len;
    uint8_t note, vel;
} clip_note_t;

typedef struct {
    uint8_t used, line, count, playing;
    uint8_t sound;                              /* the line's sound when it was taken (its colour) */
    uint16_t length;
    uint32_t origin;                            /* free tick of its first note's first play */
    uint32_t done;                              /* how far it has played (ticks since origin) */
    clip_note_t n[CLIP_NOTES];
    uint8_t on_note[CLIP_SOUNDING];             /* its notes sounding now, and when each ends */
    uint32_t on_end[CLIP_SOUNDING];
    uint8_t on_count;
} clip_t;

static clip_t clips[CLIP_SLOTS];
static struct {
    int8_t slot;                                /* armed or recording, -1 none */
    uint8_t started, line;
    uint32_t start;
} crec = { -1, 0, 0, 0 };

static uint32_t clip_now(void) { return live.clock.free_tick; }

static void clip_engine_note(uint8_t line, uint8_t note, uint8_t vel)
{
    if (line == T_LIVE) {
        if (vel) {
            engine_va_note_on(&lv, note, vel);
        } else {
            engine_va_note_off(&lv, note);
        }
    } else if (line < TRACKS) {
        const ols_engine_api_t *api = live.track[line].engine.api;
        if (vel && live.track[line].mute) {
            return;                             /* a muted track: its clips are silent too */
        }
        if (vel) {
            api->note_on(live.track[line].engine.ctx, note, vel);
        } else {
            api->note_off(live.track[line].engine.ctx, note);
        }
    }
}

static void clip_silence(clip_t *c)
{
    for (uint32_t i = 0; i < c->on_count; i++) {
        clip_engine_note(c->line, c->on_note[i], 0);
    }
    c->on_count = 0;
}

/* the take: a note seen on the armed line (audio side, as the event is applied) */
static void clip_note_seen(uint8_t line, uint8_t note, uint8_t vel)
{
    if (crec.slot < 0 || line != crec.line) {
        return;
    }
    clip_t *c = &clips[crec.slot];
    uint32_t now = clip_now();
    if (!crec.started) {
        if (!vel) {
            return;
        }
        clip_silence(c);
        c->playing = 0;
        c->count = 0;
        c->used = 0;
        c->line = line;
        c->sound = line == T_LIVE ? set.live_sound : set.preset[line];
        crec.started = 1;
        crec.start = now;                       /* the take starts at its first note */
    }
    uint32_t at = now - crec.start;
    if (vel) {
        if (c->count < CLIP_NOTES && at < CLIP_MAX_TICKS) {
            clip_note_t *n = &c->n[c->count++];
            n->tick = (uint16_t)at;
            n->len = 0;                         /* (still held) */
            n->note = note;
            n->vel = vel;
        }
        return;
    }
    for (uint32_t i = c->count; i-- > 0;) {     /* the latest held one of that note */
        if (c->n[i].note == note && c->n[i].len == 0u) {
            uint32_t len = at - c->n[i].tick;
            c->n[i].len = (uint16_t)(len < 6u ? 6u : len);
            break;
        }
    }
}

static void clip_end_take(void)
{
    if (crec.slot < 0) {
        return;
    }
    clip_t *c = &clips[crec.slot];
    if (crec.started && c->count) {
        uint32_t len = clip_now() - crec.start;
        len = (len + OLS_PPQN / 2u) / OLS_PPQN * OLS_PPQN;      /* to the beat */
        len = len < OLS_PPQN ? OLS_PPQN : (len > CLIP_MAX_TICKS ? CLIP_MAX_TICKS : len);
        uint32_t kept = 0;
        for (uint32_t i = 0; i < c->count; i++) {
            if (c->n[i].tick >= len) {
                continue;                       /* past the rounded end */
            }
            c->n[kept] = c->n[i];
            if (c->n[kept].len == 0u || c->n[kept].tick + c->n[kept].len > len) {
                c->n[kept].len = (uint16_t)(len - c->n[kept].tick);   /* held to the end */
            }
            kept++;
        }
        c->count = (uint8_t)kept;
        c->length = (uint16_t)len;
        c->used = kept > 0u;
    }
    crec.slot = -1;
    crec.started = 0;
}

static void clip_event(const ols_event_t *e)
{
    uint8_t s = e->b;
    if (s >= CLIP_SLOTS && e->a != CLIP_END) {
        return;
    }
    clip_t *c = &clips[s < CLIP_SLOTS ? s : 0];
    switch (e->a) {
    case CLIP_ARM:
        clip_end_take();
        crec.slot = (int8_t)s;
        crec.started = 0;
        crec.line = e->target;
        break;
    case CLIP_END:
        clip_end_take();
        break;
    case CLIP_PLAY:
    case CLIP_PLAY_SYNC:
        if (!c->used) {
            break;
        }
        clip_silence(c);
        c->origin = clip_now();
        if (e->a == CLIP_PLAY_SYNC) {           /* on the next beat */
            c->origin = (c->origin / OLS_PPQN + 1u) * OLS_PPQN;
        }
        c->done = 0;
        c->playing = 1;
        break;
    case CLIP_STOP:
        clip_silence(c);
        c->playing = 0;
        break;
    default:                                    /* CLIP_DELETE */
        clip_silence(c);
        c->playing = 0;
        c->used = 0;
        c->count = 0;
        break;
    }
}

/* once a block: each playing clip's notes due by now start, the ones whose time is up end */
static void clips_advance(void)
{
    uint32_t now = clip_now();
    if (crec.slot >= 0 && crec.started && now - crec.start >= CLIP_MAX_TICKS) {
        clip_end_take();                        /* the longest take: it ends by itself */
    }
    for (uint32_t s = 0; s < CLIP_SLOTS; s++) {
        clip_t *c = &clips[s];
        if (!c->playing) {
            continue;
        }
        for (uint32_t i = 0; i < c->on_count;) {          /* the ends due */
            if ((int32_t)(now - c->on_end[i]) >= 0) {
                clip_engine_note(c->line, c->on_note[i], 0);
                c->on_note[i] = c->on_note[c->on_count - 1u];
                c->on_end[i] = c->on_end[c->on_count - 1u];
                c->on_count--;
            } else {
                i++;
            }
        }
        if ((int32_t)(now - c->origin) < 0) {
            continue;                           /* waiting for its beat */
        }
        uint32_t upto = now - c->origin + 1u;   /* ticks played once this block is done */
        while (c->done < upto) {                /* every note whose time comes in (done, upto] */
            uint32_t pos = c->done % c->length;
            for (uint32_t i = 0; i < c->count; i++) {
                if (c->n[i].tick == pos && c->on_count < CLIP_SOUNDING) {
                    clip_engine_note(c->line, c->n[i].note, c->n[i].vel);
                    c->on_note[c->on_count] = c->n[i].note;
                    c->on_end[c->on_count] = c->origin + c->done + c->n[i].len;
                    c->on_count++;
                }
            }
            c->done++;
        }
    }
}
