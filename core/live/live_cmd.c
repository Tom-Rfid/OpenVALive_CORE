/*
 * OpenVALive - Live Core
 * live_cmd.c : what the audio side does with the events of the queue, and
 * the engine-shaped front door used by the MIDI parser.
 */
#include "live_internal.h"

/* LIVE_NOTE_EDIT: one note of a track's loop changed, taken out or added; the loop kept sorted by start */
OLS_COLD static void live_note_edit(ols_live_t *L, uint8_t target, uint8_t op, uint16_t id, float v)
{
    int t = live_track_index(L, target);
    if (t < 0) {
        return;
    }
    ols_pattern_t *p = &L->track[t].pat;
    int32_t x = (int32_t)(v + 0.5f);
    if (op == NOTE_EDIT_BEGIN) {
        ols_live_undo_push(L, t);
        return;
    }
    if (!p->length) {
        return;
    }
    if (op == NOTE_EDIT_ADD) {
        if (p->count >= OLS_PATTERN_NOTES) {
            return;
        }
        ols_note_t *n = &p->n[p->count++];
        n->tick = (uint16_t)(x < 0 ? 0 : x % p->length);
        n->len = OLS_PPQN / 4u;
        n->note = (uint8_t)(id & 127u);
        n->vel = 100;
        n->chance = 100;
        n->ratchet = 1;
        id = (uint16_t)(p->count - 1u);
    } else if (id >= p->count) {
        return;
    }
    ols_note_t *n = &p->n[id];
    switch (op) {
    case NOTE_EDIT_TICK:
        n->tick = (uint16_t)(((x % (int32_t)p->length) + (int32_t)p->length) % (int32_t)p->length);
        break;
    case NOTE_EDIT_LEN:
        n->len = (uint16_t)(x < 1 ? 1 : (x > (int32_t)p->length ? p->length : x));
        break;
    case NOTE_EDIT_VEL:
        n->vel = (uint8_t)(x < 1 ? 1 : (x > 127 ? 127 : x));
        break;
    case NOTE_EDIT_PITCH:
        n->note = (uint8_t)(x < 0 ? 0 : (x > 127 ? 127 : x));
        break;
    case NOTE_EDIT_DELETE:
        for (uint32_t i = id; i + 1u < p->count; i++) {
            p->n[i] = p->n[i + 1u];
        }
        p->count--;
        return;
    default:
        break;
    }
    for (uint32_t i = 1; i < p->count; i++) {    /* sorted again (one note moved) */
        ols_note_t k = p->n[i];
        uint32_t j = i;
        while (j > 0 && p->n[j - 1u].tick > k.tick) {
            p->n[j] = p->n[j - 1u];
            j--;
        }
        p->n[j] = k;
    }
}

/* recording into a muted track: its old loop goes (undoable) and it plays again, so the new take is heard */
static void record_on_muted(ols_live_t *L)
{
    ols_track_t *tr = &L->track[L->current];
    if (tr->mute) {
        live_sequencer_notes_off(L, tr);
        tr->pat.count = 0;
        tr->mute = 0;
    }
}

OLS_COLD void ols_live_command(ols_live_t *L, ols_live_cmd_t cmd, uint8_t b, uint16_t id, float v, uint8_t target)
{
    switch (cmd) {
    case LIVE_PLAY_TOGGLE:
        if (L->clock.running) {
            ols_live_stop(L);
        } else {
            ols_live_play(L);
        }
        break;
    case LIVE_PLAY:
        ols_live_play(L);
        break;
    case LIVE_STOP:
        ols_live_stop(L);
        break;
    case LIVE_RECORD_TOGGLE:
        L->recording = (uint8_t)!L->recording;
        if (L->recording) {
            ols_live_undo_push(L, L->current);  /* the whole take is one undo step */
            record_on_muted(L);
            if (!L->clock.running) {
                ols_live_play(L);
                /* metronome on: one bar to count in, notes are taken from bar 2 */
                L->rec_from = (L->metronome && !L->clock.external) ? OLS_TICKS_PER_BAR : 0u;
            }
        }
        break;
    case LIVE_CAPTURE:
        if (b) {
            ols_live_capture_take(L);
        } else {
            ols_live_capture(L);
        }
        break;
    case LIVE_CAPTURE_ARM:
        ols_live_capture_arm(L);
        break;
    case LIVE_PAN: {
        int t = live_track_index(L, target);
        if (t >= 0) {
            L->track[t].pan = v < -1.0f ? -1.0f : (v > 1.0f ? 1.0f : v);
        }
        break;
    }
    case LIVE_NOTE_EDIT:
        live_note_edit(L, target, b, id, v);
        break;
    case LIVE_RESTART:
        for (int i = 0; i < L->track_count; i++) {
            live_sequencer_notes_off(L, &L->track[i]);
        }
        if (L->clock.running && !L->clock.external) {
            ols_live_play(L);                   /* tick 0 again; MIDI START to what follows us */
        }
        break;
    case LIVE_PUNCH:
        if (b && !L->recording && L->clock.running) {
            ols_live_undo_push(L, L->current);  /* the punch is one undo step */
            record_on_muted(L);
            L->recording = 1;
            L->punch = 1;
            L->punch_start = L->clock.tick;
            L->rec_from = 0;
        } else if (!b && L->punch) {
            L->recording = 0;
            L->punch = 0;
        }
        break;
    case LIVE_UNDO:
        ols_live_undo_track(L, target);
        break;
    case LIVE_REDO:
        ols_live_redo_track(L, target);
        break;
    case LIVE_CLEAR:
        ols_live_clear(L, target);
        break;
    case LIVE_SELECT:
        ols_live_select(L, b);
        break;
    case LIVE_MUTE_TOGGLE: {
        int t = live_track_index(L, b);
        if (t >= 0) {
            ols_live_mute(L, t, !L->track[t].mute);
        }
        break;
    }
    case LIVE_CREATIVE:
        ols_live_creative(L, target, (ols_creative_op_t)b, v, (int32_t)(int16_t)id);
        break;
    case LIVE_EUCLID:
        ols_live_euclid(L, target, b, id);
        break;
    case LIVE_LENGTH: {
        int t = live_track_index(L, target);
        if (t >= 0) {
            ols_live_undo_push(L, t);
            live_sequencer_notes_off(L, &L->track[t]);
            uint32_t old = L->track[t].pat.length;
            pattern_set_length(&L->track[t].pat, id);
            pattern_repeat(&L->track[t].pat, old);     /* bars added: the loop again, not silence */
            L->changes++;
        }
        break;
    }
    case LIVE_TEMPO:
        clock_set_bpm(&L->clock, v);
        break;
    case LIVE_SWING:
        L->swing = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        break;
    case LIVE_SCENE_STORE:
        ols_live_scene_store(L, b);
        break;
    case LIVE_SCENE_RECALL:
        ols_live_scene_go(L, b);            /* on the next bar when playing */
        break;
    case LIVE_MORPH:
        ols_live_morph(L, b, id, v);
        break;
    case LIVE_STUTTER:
        ols_live_stutter_track(L, b, b ? id : 0u, target, !b && id == 1u);
        break;
    case LIVE_OVERRIDE:
        ols_live_override(L, target, id, v, b);
        break;
    case LIVE_QUANTIZE:
        L->quantize = (uint8_t)(b != 0);
        break;
    case LIVE_EXTERNAL_CLOCK:
        ols_live_stop(L);
        clock_set_external(&L->clock, b);
        break;
    case LIVE_SEND_CLOCK:
        L->send_clock = (uint8_t)(b != 0);
        break;
    case LIVE_HOLD_MUTE:
        ols_live_hold_mute(L, (uint8_t)id, b);
        break;
    case LIVE_XFADE:
        L->xfade_bars = (uint8_t)(b > 8u ? 8u : b);
        break;
    case LIVE_MACRO:
        ols_live_macro(L, b, v);
        break;
    case LIVE_PUMP:
        L->pump = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        break;
    case LIVE_CHAIN:
        ols_live_chain(L, b, id);
        break;
    case LIVE_METRONOME:
        L->metronome = (uint8_t)(b != 0);
        break;
    case LIVE_TAP:
        ols_live_tap(L);
        break;
    case LIVE_GAIN: {
        int t = live_track_index(L, target);
        if (t >= 0) {
            L->track[t].gain = v < 0.0f ? 0.0f : (v > 2.0f ? 2.0f : v);
        }
        break;
    }
    default:
        break;
    }
}

int ols_live_apply_event(ols_live_t *L, const ols_event_t *e)
{
    int t = live_track_index(L, e->target);

    switch (e->type) {
    case EV_NOTE_ON:
        ols_live_note_on(L, e->target, e->a, e->b);
        return 1;
    case EV_NOTE_OFF:
        ols_live_note_off(L, e->target, e->a);
        return 1;
    case EV_PARAM:
        if (t >= 0 && e->id < L->track[t].engine.api->param_count) {
            live_set_param(&L->track[t], e->id, e->v);
        }
        return 1;
    case EV_ALL_OFF:
        ols_live_all_notes_off(L);
        return 1;
    case EV_BEND:
        if (t >= 0) {
            L->track[t].engine.api->pitch_bend(L->track[t].engine.ctx, e->v);
        }
        return 1;
    case EV_LIVE:
        ols_live_command(L, (ols_live_cmd_t)e->a, e->b, e->id, e->v, e->target);
        return 1;
    case EV_PRESSURE:
        if (t >= 0 && L->track[t].engine.api->pressure != 0) {
            L->track[t].engine.api->pressure(L->track[t].engine.ctx, e->v);
        }
        return 1;
    default:
        return 0;
    }
}

/* ---------------------------------------------------------------- MIDI front door */

static void input_note_on(void *ctx, uint8_t note, uint8_t velocity)
{
    ols_live_t *L = (ols_live_t *)ctx;
    note &= 0x7Fu;
    L->input_track[note] = L->current;
    ols_live_note_on(L, L->current, note, velocity);
}

static void input_note_off(void *ctx, uint8_t note)
{
    ols_live_t *L = (ols_live_t *)ctx;
    note &= 0x7Fu;
    ols_live_note_off(L, L->input_track[note], note);
}

static void input_all_notes_off(void *ctx)
{
    ols_live_all_notes_off((ols_live_t *)ctx);
}

static void input_set_param(void *ctx, uint16_t id, float value)
{
    ols_live_t *L = (ols_live_t *)ctx;
    ols_track_t *tr = &L->track[L->current];
    if (id < tr->engine.api->param_count) {
        live_set_param(tr, id, value);
    }
}

static float input_get_param(void *ctx, uint16_t id)
{
    ols_live_t *L = (ols_live_t *)ctx;
    ols_track_t *tr = &L->track[L->current];
    return id < tr->engine.api->param_count ? live_param(tr, id) : 0.0f;
}

static void input_pitch_bend(void *ctx, float semitones)
{
    ols_live_t *L = (ols_live_t *)ctx;
    ols_track_t *tr = &L->track[L->current];
    tr->engine.api->pitch_bend(tr->engine.ctx, semitones);
}

static void input_pressure(void *ctx, float amount)
{
    ols_live_t *L = (ols_live_t *)ctx;
    ols_track_t *tr = &L->track[L->current];
    if (tr->engine.api->pressure != 0) {
        tr->engine.api->pressure(tr->engine.ctx, amount);
    }
}

static void input_render(void *ctx, float *out_l, float *out_r, int n)
{
    (void)ctx;
    for (int i = 0; i < n; i++) {
        out_l[i] = 0.0f;
        out_r[i] = 0.0f;
    }
}

static const ols_engine_api_t LIVE_INPUT_API = {
    .name = "live-input",
    .param_count = OLS_MAX_PARAMS,
    .param_names = 0,
    .param_kinds = 0,
    .note_on = input_note_on,
    .note_off = input_note_off,
    .all_notes_off = input_all_notes_off,
    .set_param = input_set_param,
    .get_param = input_get_param,
    .pitch_bend = input_pitch_bend,
    .render = input_render,
    .roles = 0,
    .macros = 0,
    .macro_names = 0,
    .pressure = input_pressure,
};

ols_engine_t ols_live_input(ols_live_t *L)
{
    ols_engine_t e;
    e.api = &LIVE_INPUT_API;
    e.ctx = L;
    return e;
}
