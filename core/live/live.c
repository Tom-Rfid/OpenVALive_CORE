/*
 * OpenVALive - Live Core
 * live.c : tracks, transport, the sequencer and the mix (see live.h).
 *
 * The sequencer runs inside the render: the block is cut at every clock
 * tick, the notes of that tick are started or stopped, then rendering goes
 * on. So a sequenced note starts on its exact sample, whatever the block
 * size of the platform.
 */
#include "live_internal.h"
#include "../dsp/dsp_common.h"

#define SWING_MAX_TICKS 16.0f      /* full swing: an off-beat 16th moves by 2/3 of a step */

/* ---------------------------------------------------------------- set-up */

void ols_live_init(ols_live_t *L, const ols_engine_t *engines, int count, float sample_rate)
{
    ols_zero(L, sizeof *L);

    if (count > OLS_TRACKS) {
        count = OLS_TRACKS;
    }
    L->track_count = (uint8_t)count;
    for (int t = 0; t < count; t++) {
        ols_track_t *tr = &L->track[t];
        tr->engine = engines[t];
        tr->gain = 1.0f;
        tr->suppress_note = 0xFF;
        tr->last_note = 60;
        tr->last_vel = 100;
        pattern_clear(&tr->pat, OLS_TICKS_PER_BAR);
    }

    clock_init(&L->clock, sample_rate, 120.0f);
    L->master = 0.8f;
    L->rng = 0x2545F491u;
    L->send_notes = 1;
    L->morph_b = 1;
    L->scene_queued = -1;
    for (int t = 0; t < OLS_TRACKS; t++) {
        for (int m = 0; m < OLS_MACRO_COUNT; m++) {
            L->macro[t][m] = 0.5f;
        }
    }
    L->duck_kind = 0xFF;                     /* no sidechain source until the host says which kind */
    L->duck_gain = 1.0f;
    L->duck_release = dsp_expf(-1.0f / (0.12f * sample_rate));   /* the pump comes back in ~120 ms */
    L->chain_bars = 4;
    L->click_coef = dsp_expf(-1.0f / (0.012f * sample_rate));   /* a 12 ms tick */
}

void ols_live_set_engine(ols_live_t *L, int track, ols_engine_t engine, uint8_t kind)
{
    if (track < 0 || track >= L->track_count) {
        return;
    }
    ols_track_t *tr = &L->track[track];
    tr->engine.api->all_notes_off(tr->engine.ctx);
    tr->active_count = 0;
    tr->held_count = 0;
    tr->engine = engine;
    tr->kind = kind;
    /* the track's macros apply to the new engine too */
    for (int m = 0; m < OLS_MACRO_COUNT; m++) {
        if (L->macro[track][m] != 0.5f) {
            ols_live_macro(L, track, m, L->macro[track][m]);
            break;
        }
    }
    /* a held override belonged to the old engine's parameter */
    for (int i = 0; i < OLS_OVERRIDES; i++) {
        if (L->ovr[i].track == track) {
            L->ovr[i].active = 0;
        }
    }
    L->changes++;
}

void ols_live_set_midi_out(ols_live_t *L, void (*fn)(void *, uint8_t, uint8_t, uint8_t), void *ctx)
{
    L->midi_out_ctx = ctx;
    L->midi_out = fn;
}

/* ---------------------------------------------------------------- sequenced notes */

static void track_note_off(ols_live_t *L, int t, uint8_t note)
{
    ols_track_t *tr = &L->track[t];
    tr->engine.api->note_off(tr->engine.ctx, note);
    if (L->send_notes) {
        live_midi_out(L, (uint8_t)(0x80u | (uint32_t)t), note, 0);
    }
}

static void remove_active(ols_track_t *tr, int i)
{
    tr->active[i] = tr->active[tr->active_count - 1];
    tr->active_count--;
}

void live_sequencer_notes_off(ols_live_t *L, ols_track_t *tr)
{
    int t = (int)(tr - L->track);
    while (tr->active_count > 0) {
        track_note_off(L, t, tr->active[tr->active_count - 1].note);
        tr->active_count--;
    }
}

/* Starts a sequenced note and remembers when to stop it. */
static void start_note(ols_live_t *L, int t, uint8_t note, uint8_t vel, uint32_t off_tick)
{
    ols_track_t *tr = &L->track[t];

    /* the same note still sounding (a long note, a ratchet): restart it cleanly */
    for (int i = 0; i < tr->active_count; i++) {
        if (tr->active[i].note == note) {
            track_note_off(L, t, note);
            remove_active(tr, i);
            break;
        }
    }
    /* no room: the oldest sequenced note gives way */
    if (tr->active_count >= OLS_ACTIVE_NOTES) {
        track_note_off(L, t, tr->active[0].note);
        remove_active(tr, 0);
    }

    tr->engine.api->note_on(tr->engine.ctx, note, vel);
    if (L->send_notes) {
        live_midi_out(L, (uint8_t)(0x90u | (uint32_t)t), note, vel);
    }
    tr->active[tr->active_count].note = note;
    tr->active[tr->active_count].off_tick = off_tick;
    tr->active_count++;
}

static void stop_due_notes(ols_live_t *L, int t, uint32_t now)
{
    ols_track_t *tr = &L->track[t];
    int i = 0;
    while (i < tr->active_count) {
        if (tr->active[i].off_tick <= now) {
            track_note_off(L, t, tr->active[i].note);
            remove_active(tr, i);
        } else {
            i++;
        }
    }
}

/* Swing moves the off-beat 16ths later. */
static uint32_t swung_start(const ols_live_t *L, const ols_note_t *n)
{
    if (L->swing > 0.0f && n->tick % OLS_TICKS_PER_STEP == 0u
        && (n->tick / OLS_TICKS_PER_STEP) % 2u == 1u) {
        return n->tick + (uint32_t)(L->swing * SWING_MAX_TICKS + 0.5f);
    }
    return n->tick;
}

static int bit_of(const uint8_t *bits, int i)
{
    return (bits[i >> 3] >> (i & 7)) & 1;
}

static void set_bit(uint8_t *bits, int i, int on)
{
    uint8_t mask = (uint8_t)(1u << (i & 7));
    bits[i >> 3] = on ? (uint8_t)(bits[i >> 3] | mask) : (uint8_t)(bits[i >> 3] & ~mask);
}

static int skip_bit(const ols_track_t *tr, int i)
{
    return bit_of(tr->skip, i);
}

static void set_skip_bit(ols_track_t *tr, int i, int on)
{
    set_bit(tr->skip, i, on);
}

/* The notes of one track that start (or re-hit, for ratchets) at this tick. */
static void play_track_tick(ols_live_t *L, int t, uint32_t now)
{
    ols_track_t *tr = &L->track[t];
    const ols_pattern_t *p = &tr->pat;
    uint32_t pos = live_track_pos(L, tr, now);

    for (int i = 0; i < (int)p->count; i++) {
        const ols_note_t *n = &p->n[i];
        uint32_t start = swung_start(L, n);
        uint32_t into = (pos + p->length - (start % p->length)) % p->length;
        if (into >= n->len) {
            continue;
        }

        uint32_t hits = n->ratchet > 1u ? n->ratchet : (bit_of(tr->dense, i) ? 2u : 1u);
        uint32_t spacing = n->len / hits;
        if (spacing < 1u) {
            spacing = 1u;
        }
        if (into % spacing != 0u || into / spacing >= hits) {
            continue;
        }

        if (into == 0u) {
            /* a new pass of this note: roll its chance once for all its hits */
            int plays = n->chance >= 100u || ols_rand_below(&L->rng, 100) < n->chance;
            /* DENSITY below 0 thins the off-beats; above 0 doubles some notes */
            int off_beat = n->tick % OLS_PPQN != 0u;
            if (L->density[t] < 0.0f && off_beat && ols_rand01(&L->rng) < -L->density[t] * 0.8f) {
                plays = 0;
            }
            set_bit(tr->dense, i, L->density[t] > 0.0f && n->len >= 12u
                                  && ols_rand01(&L->rng) < L->density[t] * 0.5f);
            set_skip_bit(tr, i, !plays);

            if (tr->suppress_note == n->note && tr->suppress_tick == n->tick) {
                tr->suppress_note = 0xFF;  /* recorded a moment ago, already sounding */
                set_skip_bit(tr, i, 1);
            }
        }
        if (skip_bit(tr, i) || ols_live_track_silent(L, t)) {
            continue;
        }

        uint32_t length = hits > 1u ? (spacing * 3u + 3u) / 4u : n->len;
        uint8_t vel = n->vel;
        if (into > 0u && vel > 20u) {
            vel = (uint8_t)(vel - 12u);    /* repeats a little softer than the first hit */
        }
        start_note(L, t, n->note, vel, now + (length > 0u ? length : 1u));
    }
}

/* The metronome: a short tick on each beat, higher on the bar. It also
 * plays during a count-in, metronome on or not. */
static void metronome_tick(ols_live_t *L, uint32_t now)
{
    int counting_in = now < L->rec_from;
    if ((!L->metronome && !counting_in) || now % OLS_PPQN != 0u) {
        return;
    }
    int bar = now % OLS_TICKS_PER_BAR == 0u;
    L->click_amp = bar ? 0.5f : 0.3f;
    L->click_phase = 0.0f;
    L->click_inc = (bar ? 1760.0f : 1320.0f) / L->clock.sample_rate;
}

static void render_click(ols_live_t *L, float *out_l, float *out_r, uint32_t n)
{
    if (L->click_amp < 0.0005f) {
        return;
    }
    for (uint32_t i = 0; i < n; i++) {
        L->click_phase += L->click_inc;
        if (L->click_phase >= 1.0f) {
            L->click_phase -= 1.0f;
        }
        /* a triangle is enough for a click, and costs nothing */
        float tri = 1.0f - 4.0f * dsp_fabsf(L->click_phase - 0.5f);
        float x = tri * L->click_amp;
        out_l[i] += x;
        out_r[i] += x;
        L->click_amp *= L->click_coef;
    }
}

int ols_live_track_silent(const ols_live_t *L, int track)
{
    return L->track[track].mute || ((L->hold_mute >> track) & 1u);
}

/* Things that wait for the downbeat: the end of a break, a queued scene. */
static void bar_start(ols_live_t *L, uint32_t now)
{
    if (L->hold_release) {
        L->hold_mute = 0;
        L->hold_release = 0;
    }
    if (L->chain_on && now >= L->chain_next) {
        /* the next stored scene of the chain, cut or cross-faded like any scene change */
        for (int k = 1; k <= OLS_SCENES; k++) {
            int slot = (L->chain_pos + k) % OLS_SCENES;
            if (L->scene[slot].valid) {
                L->chain_pos = (uint8_t)slot;
                L->scene_queued = (int8_t)slot;
                break;
            }
        }
        L->chain_next = now + (uint32_t)L->chain_bars * OLS_TICKS_PER_BAR;
    }
    if (L->scene_queued >= 0) {
        int slot = L->scene_queued;
        L->scene_queued = -1;
        if (L->xfade_bars == 0u) {
            ols_live_scene_recall(L, slot);
        } else {
            live_xfade_begin(L, slot, now);
        }
    }
}

/* A punch replaces: the selected track's old notes go as the playhead reaches them (for one loop length; the
 * note just recorded ahead of the playhead stays) */
static void punch_erase(ols_live_t *L, uint32_t now)
{
    ols_track_t *tr = &L->track[L->current];
    if (now - L->punch_start >= tr->pat.length) {
        return;
    }
    uint32_t pos = live_track_pos(L, tr, now);
    for (int i = (int)tr->pat.count - 1; i >= 0; i--) {
        const ols_note_t *n = &tr->pat.n[i];
        if (n->tick == pos && !(n->note == tr->suppress_note && n->tick == tr->suppress_tick)) {
            pattern_remove(&tr->pat, i);
            L->changes++;
        }
    }
}

static void sequencer_tick(ols_live_t *L, uint32_t now)
{
    if (L->stutter_end != 0u && now >= L->stutter_end) {
        live_stutter_end(L);              /* the slice ended: back where the music is, in time */
    }
    metronome_tick(L, now);
    if (now % OLS_TICKS_PER_BAR == 0u) {
        bar_start(L, now);
    }
    live_xfade_step(L, now);
    if (L->send_clock && !L->clock.external && now % OLS_TICKS_PER_MIDI_CLOCK == 0u) {
        live_midi_out(L, 0xF8, 0, 0);
    }
    if (L->punch) {
        punch_erase(L, now);
    }
    for (int t = 0; t < L->track_count; t++) {
        stop_due_notes(L, t, now);        /* stops first: a note may restart on the same tick */
        play_track_tick(L, t, now);
    }
}

/* ---------------------------------------------------------------- render */

/* The sidechain source: the first track of the host's drum kind, or -1. */
static int duck_source(const ols_live_t *L)
{
    if (L->pump <= 0.0f) {
        return -1;
    }
    for (int t = 0; t < L->track_count; t++) {
        if (L->track[t].kind == L->duck_kind) {
            return t;
        }
    }
    return -1;
}

/* Adds one track into the mix, its gain ramping from g0 to g1 over the chunk (no zipper). */
static void mix_track(ols_live_t *L, ols_track_t *tr, float *out_l, float *out_r, uint32_t n, float g0, float g1)
{
    tr->engine.api->render(tr->engine.ctx, L->tmp_l, L->tmp_r, (int)n);
    float step = (g1 - g0) / (float)n;
    float g = g0;
    if (tr->pan == 0.0f && tr->pan_now == 0.0f) {
        for (uint32_t i = 0; i < n; i++) {
            g += step;
            out_l[i] += L->tmp_l[i] * g;
            out_r[i] += L->tmp_r[i] * g;
        }
        return;
    }
    /* panned: the far side turned down, the pan ramping to where it is asked over the chunk (no zipper) */
    float p = tr->pan_now, pstep = (tr->pan - tr->pan_now) / (float)n;
    for (uint32_t i = 0; i < n; i++) {
        g += step;
        p += pstep;
        float gl = p > 0.0f ? 1.0f - p : 1.0f, gr = p < 0.0f ? 1.0f + p : 1.0f;
        out_l[i] += L->tmp_l[i] * g * gl;
        out_r[i] += L->tmp_r[i] * g * gr;
    }
    tr->pan_now = tr->pan;
}

/* Follows the source's peaks: instant up, ~120 ms down. Returns the ducked gain for the others. */
static float follow_source(ols_live_t *L, uint32_t n)
{
    float peak = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        float a = dsp_fabsf(L->tmp_l[i]);
        peak = a > peak ? a : peak;
    }
    float decayed = L->duck_env;
    for (uint32_t i = 0; i < n; i++) {
        decayed *= L->duck_release;
    }
    L->duck_env = peak > decayed ? peak : decayed;
    float amount = dsp_clampf(L->duck_env * 2.5f, 0.0f, 1.0f);   /* a kick at ~0.4 ducks fully */
    return 1.0f - L->pump * 0.85f * amount;
}

/* the slicer: how far track t is cut at the end of this chunk (the second half of every slice; chunks end on ticks,
 * so the edges fall in time), moved a part of the way each chunk: ~2 ms an edge */
static float slice_cut_next(ols_live_t *L, int t, uint32_t n)
{
    uint32_t tick = L->clock.running ? L->clock.tick : L->clock.free_tick, len = L->slice_len ? L->slice_len : 24u;
    float target = ((L->slice_mask >> t) & 1u) && (tick % len) >= len / 2u ? 0.95f : 0.0f;
    float k = (float)n * 0.012f;
    float c = L->slice_cut[t];
    c += (target - c) * (k > 1.0f ? 1.0f : k);
    return c < 0.0005f && target == 0.0f ? 0.0f : c;
}

static void render_tracks(ols_live_t *L, float *out_l, float *out_r, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        out_l[i] = 0.0f;
        out_r[i] = 0.0f;
    }
    int source = duck_source(L);
    float duck0 = L->duck_gain, duck1 = 1.0f;
    if (source >= 0) {
        ols_track_t *tr = &L->track[source];
        float g = tr->gain * L->master, c0 = L->slice_cut[source], c1 = slice_cut_next(L, source, n);
        L->slice_cut[source] = c1;
        mix_track(L, tr, out_l, out_r, n, g * (1.0f - c0), g * (1.0f - c1));
        duck1 = follow_source(L, n);
    }
    for (int t = 0; t < L->track_count; t++) {
        if (t == source) {
            continue;
        }
        ols_track_t *tr = &L->track[t];
        float g = tr->gain * L->master, c0 = L->slice_cut[t], c1 = slice_cut_next(L, t, n);
        L->slice_cut[t] = c1;
        mix_track(L, tr, out_l, out_r, n, g * duck0 * (1.0f - c0), g * duck1 * (1.0f - c1));
    }
    L->duck_gain = duck1;
}

/* Four engines can add up. Below the knee the sum passes untouched (and
 * costs nothing); above it, a tanh curve bends it smoothly towards 1. */
#define LIMIT_KNEE 0.6f

static void soft_limit(float *x, uint32_t n)
{
    float peak = 0.0f;
    for (uint32_t i = 0; i < n; i++) {
        float a = dsp_fabsf(x[i]);
        peak = a > peak ? a : peak;
    }
    if (peak <= LIMIT_KNEE) {
        return;
    }
    const float room = 1.0f - LIMIT_KNEE;
    for (uint32_t i = 0; i < n; i++) {
        float a = dsp_fabsf(x[i]);
        if (a > LIMIT_KNEE) {
            float bent = LIMIT_KNEE + room * dsp_tanh((a - LIMIT_KNEE) / room);
            x[i] = x[i] < 0.0f ? -bent : bent;
        }
    }
}

/* The engines' clock (engine.h clock): the tempo and the beat, from PLAY, or free-running when stopped.
 * Counted modulo 48 beats (12 bars, a multiple of every tempo-locked LFO length): a float keeps its precision
 * however long the set runs. */
#define CLOCK_WRAP_TICKS (48u * OLS_PPQN)

static void tell_engines_clock(ols_live_t *L)
{
    const ols_clock_t *c = &L->clock;
    uint32_t tick = c->running ? c->tick : c->free_tick;
    float frac = c->running ? c->frac : c->free_frac;
    float beats = ((float)(tick % CLOCK_WRAP_TICKS) + frac) * (1.0f / (float)OLS_PPQN);
    float bpm = clock_bpm(c);
    for (int t = 0; t < L->track_count; t++) {
        const ols_engine_t *en = &L->track[t].engine;
        if (en->api->clock != 0) {
            en->api->clock(en->ctx, bpm, beats);
        }
    }
}

void ols_live_render(ols_live_t *L, float *out_l, float *out_r, int n)
{
    uint32_t offset = 0;
    uint32_t total = n > 0 ? (uint32_t)n : 0u;

    while (offset < total) {
        if (L->clock.pending) {
            L->clock.pending = 0;
            sequencer_tick(L, L->clock.tick);
        }
        uint32_t room = total - offset;
        if (room > OLS_RENDER_CHUNK) {
            room = OLS_RENDER_CHUNK;
        }
        uint32_t count = clock_samples_to_tick(&L->clock, room);
        tell_engines_clock(L);
        render_tracks(L, out_l + offset, out_r + offset, count);
        render_click(L, out_l + offset, out_r + offset, count);
        clock_advance(&L->clock, count);
        offset += count;
    }

    soft_limit(out_l, total);
    soft_limit(out_r, total);
    L->samples += total;
}

void ols_live_hold_mute(ols_live_t *L, uint8_t mask, int hold)
{
    if (hold) {
        /* out now: a break hits at once */
        L->hold_mute = (uint8_t)(L->hold_mute | mask);
        L->hold_release = 0;
        for (int t = 0; t < L->track_count; t++) {
            if ((mask >> t) & 1u) {
                live_sequencer_notes_off(L, &L->track[t]);
            }
        }
    } else if (L->clock.running) {
        L->hold_release = 1;                 /* back on the one */
    } else {
        L->hold_mute = 0;
    }
}

/* The macros, as offsets by role on every track (engine.h role_offset) and a density. */
void ols_live_macro(ols_live_t *L, int track, int macro, float value)
{
    if (macro < 0 || macro >= OLS_MACRO_COUNT) {
        return;
    }
    for (int t = 0; t < L->track_count; t++) {
        if (track != (int)OLS_TRACK_ALL && t != track) {
            continue;
        }
        float *m = L->macro[t];
        m[macro] = dsp_clampf(value, 0.0f, 1.0f);
        float energy = 2.0f * (m[OLS_MACRO_ENERGY] - 0.5f);
        float space = 2.0f * (m[OLS_MACRO_SPACE] - 0.5f);
        float tension = 2.0f * (m[OLS_MACRO_TENSION] - 0.5f);
        float density = 2.0f * (m[OLS_MACRO_DENSITY] - 0.5f);
        float off[OLS_ROLE_COUNT];
        for (int r = 0; r < OLS_ROLE_COUNT; r++) {
            off[r] = 0.0f;
        }
        off[OLS_ROLE_CUTOFF] = 0.22f * energy;
        off[OLS_ROLE_DRIVE] = 0.35f * energy;
        off[OLS_ROLE_DECAY] = -0.12f * energy + 0.12f * space;
        off[OLS_ROLE_RELEASE] = 0.3f * space;
        off[OLS_ROLE_DELAY_MIX] = 0.4f * space;
        off[OLS_ROLE_CHORUS] = 0.3f * space;
        off[OLS_ROLE_RESO] = 0.35f * tension;
        off[OLS_ROLE_FENV] = 0.2f * tension;
        off[OLS_ROLE_CRUSH] = tension > 0.4f ? 0.6f * (tension - 0.4f) : 0.0f;
        L->density[t] = dsp_clampf(density + 0.5f * energy, -1.0f, 1.0f);
        ols_track_t *tr = &L->track[t];
        if (tr->engine.api->role_offset == 0) {
            continue;
        }
        for (uint16_t r = 0; r < OLS_ROLE_COUNT; r++) {
            tr->engine.api->role_offset(tr->engine.ctx, r, off[r]);
        }
    }
}

void ols_live_chain(ols_live_t *L, int on, uint32_t bars)
{
    L->chain_on = (uint8_t)(on != 0);
    if (bars >= 1u && bars <= 32u) {
        L->chain_bars = (uint8_t)bars;
    }
    if (L->chain_on) {
        /* the first change comes after `bars` bars from the next downbeat */
        uint32_t now = L->clock.tick;
        uint32_t next_bar = (now / OLS_TICKS_PER_BAR + 1u) * OLS_TICKS_PER_BAR;
        L->chain_next = L->clock.running ? next_bar + ((uint32_t)L->chain_bars - 1u) * OLS_TICKS_PER_BAR : 0u;
        L->chain_pos = L->scene_current;
    }
}

void ols_live_tap(ols_live_t *L)
{
    uint32_t interval = L->samples - L->tap_last;
    float sr = L->clock.sample_rate;
    L->tap_last = L->samples;
    if ((float)interval < 0.2f * sr || (float)interval > 2.0f * sr) {
        L->tap_interval = 0.0f;              /* a first tap, or a pause: start again */
        return;
    }
    /* average the taps, so one late tap does not throw the tempo */
    L->tap_interval = L->tap_interval > 0.0f ? 0.6f * L->tap_interval + 0.4f * (float)interval : (float)interval;
    clock_set_bpm(&L->clock, 60.0f * sr / L->tap_interval);
}

/* ---------------------------------------------------------------- live notes */

void ols_live_note_on(ols_live_t *L, int track, uint8_t note, uint8_t vel)
{
    int t = live_track_index(L, track);
    if (t < 0) {
        return;
    }
    ols_track_t *tr = &L->track[t];
    tr->engine.api->note_on(tr->engine.ctx, note, vel);
    tr->last_note = note;
    tr->last_vel = vel;
    live_record_note_on(L, t, note, vel);
}

void ols_live_note_off(ols_live_t *L, int track, uint8_t note)
{
    int t = live_track_index(L, track);
    if (t < 0) {
        return;
    }
    ols_track_t *tr = &L->track[t];
    tr->engine.api->note_off(tr->engine.ctx, note);
    live_record_note_off(L, t, note);
}

void ols_live_all_notes_off(ols_live_t *L)
{
    for (int t = 0; t < L->track_count; t++) {
        ols_track_t *tr = &L->track[t];
        tr->active_count = 0;
        tr->held_count = 0;
        tr->engine.api->all_notes_off(tr->engine.ctx);
    }
}

/* ---------------------------------------------------------------- transport */

void ols_live_play(ols_live_t *L)
{
    if (L->clock.external) {
        return;                  /* the MIDI clock decides */
    }
    clock_start(&L->clock);
    for (int t = 0; t < L->track_count; t++) {
        ols_zero(L->track[t].skip, sizeof L->track[t].skip);
    }
    if (L->send_clock) {
        live_midi_out(L, 0xFA, 0, 0);
    }
}

void ols_live_stop(ols_live_t *L)
{
    if (!L->clock.external) {
        clock_halt(&L->clock);
        if (L->send_clock) {
            live_midi_out(L, 0xFC, 0, 0);
        }
    }
    for (int t = 0; t < L->track_count; t++) {
        live_sequencer_notes_off(L, &L->track[t]);
        L->track[t].held_count = 0;
    }
    L->recording = 0;
    L->punch = 0;
    live_stutter_end(L);
    L->rec_from = 0;
    /* what waited for a bar happens now */
    L->hold_mute = L->hold_release ? 0u : L->hold_mute;
    L->hold_release = 0;
    if (L->scene_queued >= 0) {
        ols_live_scene_recall(L, L->scene_queued);
        L->scene_queued = -1;
    }
    if (L->xfade_active) {
        L->xfade_active = 0;
        ols_live_scene_recall(L, L->xfade_to);
    }
}

void ols_live_select(ols_live_t *L, int track)
{
    /* notes held on the old track are released on the old track: the UI
     * and ols_live_input() send note-offs to the track of the note-on */
    if (track >= 0 && track < L->track_count && track != L->current) {
        L->current = (uint8_t)track;
        if (L->recording) {
            ols_live_undo_push(L, track);   /* the take goes on, on this track */
        }
    }
}

void ols_live_mute(ols_live_t *L, int track, int mute)
{
    int t = live_track_index(L, track);
    if (t < 0) {
        return;
    }
    L->track[t].mute = (uint8_t)(mute != 0);
    if (mute) {
        live_sequencer_notes_off(L, &L->track[t]);
    }
    /* (not a change to save: a mute is played, as the track chosen, the transport, a hold) */
}

void ols_live_midi_realtime(void *live, uint8_t byte)
{
    ols_live_t *L = (ols_live_t *)live;
    int was_running = L->clock.running;
    clock_midi_realtime_late(&L->clock, byte, L->midi_late);
    if (was_running && !L->clock.running) {
        for (int t = 0; t < L->track_count; t++) {
            live_sequencer_notes_off(L, &L->track[t]);
        }
    }
}

void ols_live_midi_program(void *live, uint8_t program)
{
    ols_live_t *L = (ols_live_t *)live;
    if (program < OLS_SCENES && L->scene[program].valid) {
        ols_live_scene_recall(L, program);
    }
}

/* ---------------------------------------------------------------- state */

uint32_t ols_live_step(const ols_live_t *L, int track)
{
    int t = live_track_index(L, track);
    if (t < 0) {
        return 0;
    }
    const ols_track_t *tr = &L->track[t];
    return live_track_pos(L, tr, L->clock.tick) / OLS_TICKS_PER_STEP;
}
