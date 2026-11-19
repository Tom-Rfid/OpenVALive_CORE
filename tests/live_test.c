/*
 * OpenVALive - Live Core unit tests.
 *
 *   sh tests/run_tests.sh    (or see the "live core" step there for the cc line)
 *
 * A "probe" engine records the exact sample of every note it receives, so
 * the tests can check timing to the sample. The VA engine is used for the
 * parameter features (scenes, morph, overrides, projects).
 */
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../core/live/live.h"
#include "../core/live/pattern.h"
#include "../core/live/creative.h"
#include "../core/live/project.h"
#include "../core/engines/va/engine_va.h"
#include "../core/engines/drum/engine_drum.h"
#include "../core/engines/drum/engine_drum_internal.h"   /* dr_render_voice: one voice heard alone */
#include "../core/engines/bank.h"
#include "../core/live/midi_in.h"

#define SR 48000.0f          /* 120 BPM at 48 kHz: exactly 250 samples per tick */

static int failures;
static int checks;

#define CHECK(cond, ...) do { \
    checks++; \
    if (!(cond)) { failures++; printf("  FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } \
} while (0)

/* ---------------------------------------------------------------- probe engine */

typedef struct {
    uint32_t sample;               /* samples rendered so far */
    int on_count, off_count;
    uint32_t on_at[64];
    uint8_t on_note[64], on_vel[64];
    uint32_t off_at[64];
    float params[8];
} probe_t;

static void probe_on(void *c, uint8_t note, uint8_t vel)
{
    probe_t *p = c;
    if (p->on_count < 64) {
        p->on_at[p->on_count] = p->sample;
        p->on_note[p->on_count] = note;
        p->on_vel[p->on_count] = vel;
    }
    p->on_count++;
}
static void probe_off(void *c, uint8_t note)
{
    probe_t *p = c;
    (void)note;
    if (p->off_count < 64) {
        p->off_at[p->off_count] = p->sample;
    }
    p->off_count++;
}
static void probe_all_off(void *c) { (void)c; }
static void probe_set(void *c, uint16_t id, float v) { ((probe_t *)c)->params[id % 8] = v; }
static float probe_get(void *c, uint16_t id) { return ((probe_t *)c)->params[id % 8]; }
static void probe_bend(void *c, float s) { (void)c; (void)s; }
static void probe_render(void *c, float *l, float *r, int n)
{
    probe_t *p = c;
    for (int i = 0; i < n; i++) {
        l[i] = r[i] = 0.0f;
    }
    p->sample += (uint32_t)n;
}

static const ols_engine_api_t PROBE_API = {
    "probe", 8, 0, 0, probe_on, probe_off, probe_all_off, probe_set, probe_get, probe_bend, probe_render, 0, 0, 0, 0, 0, 0, 0,
};

static probe_t probe[OLS_TRACKS];
static ols_live_t live;
static float bl[4096], br[4096];

static void setup_probes(void)
{
    ols_engine_t eng[OLS_TRACKS];
    memset(probe, 0, sizeof probe);
    for (int t = 0; t < OLS_TRACKS; t++) {
        eng[t].api = &PROBE_API;
        eng[t].ctx = &probe[t];
    }
    ols_live_init(&live, eng, OLS_TRACKS, SR);
}

/* Renders `samples` in uneven blocks, like a real audio callback would. */
static void run(uint32_t samples)
{
    static const int sizes[] = { 256, 100, 33, 512, 7, 128 };
    int k = 0;
    while (samples > 0) {
        uint32_t n = (uint32_t)sizes[k++ % 6];
        if (n > samples) {
            n = samples;
        }
        ols_live_render(&live, bl, br, (int)n);
        samples -= n;
    }
}

#define TICK 250u            /* samples per tick at 120 BPM, 48 kHz */

/* ---------------------------------------------------------------- tests */

static void test_sample_accurate_notes(void)
{
    setup_probes();
    ols_pattern_t *p = &live.track[0].pat;
    pattern_add(p, 0, 48, 60, 100);
    pattern_add(p, 96, 24, 62, 90);
    pattern_add(p, 200, 10, 64, 80);          /* off the grid: humanised timing */
    ols_live_play(&live);
    run(OLS_TICKS_PER_BAR * TICK + 10);

    CHECK(probe[0].on_count == 4, "4 note-ons in one bar + 1 tick (got %d)", probe[0].on_count);
    CHECK(probe[0].on_at[0] == 0, "first note at sample 0 (got %u)", probe[0].on_at[0]);
    CHECK(probe[0].on_at[1] == 96 * TICK, "beat 2 at sample %u (got %u)", 96 * TICK, probe[0].on_at[1]);
    CHECK(probe[0].on_at[2] == 200 * TICK, "tick 200 at sample %u (got %u)", 200 * TICK, probe[0].on_at[2]);
    CHECK(probe[0].on_at[3] == OLS_TICKS_PER_BAR * TICK, "loops at the bar (got %u)", probe[0].on_at[3]);
    CHECK(probe[0].off_at[0] == 48 * TICK, "first note stops after 48 ticks (got %u)", probe[0].off_at[0]);
}

static void test_ratchet_chance_mute_swing(void)
{
    setup_probes();
    int i = pattern_add(&live.track[0].pat, 0, 48, 60, 100);
    live.track[0].pat.n[i].ratchet = 4;
    i = pattern_add(&live.track[1].pat, 0, 24, 60, 100);
    live.track[1].pat.n[i].chance = 0;
    pattern_add(&live.track[2].pat, 0, 24, 60, 100);
    ols_live_mute(&live, 2, 1);
    pattern_add(&live.track[3].pat, 24, 12, 60, 100);
    live.swing = 1.0f;
    ols_live_play(&live);
    run(100 * TICK);

    CHECK(probe[0].on_count == 4, "ratchet 4 = 4 hits (got %d)", probe[0].on_count);
    CHECK(probe[0].on_at[1] == 12 * TICK && probe[0].on_at[3] == 36 * TICK, "ratchet spacing 12 ticks");
    CHECK(probe[0].on_vel[1] < probe[0].on_vel[0], "repeats are softer");
    CHECK(probe[1].on_count == 0, "chance 0 never plays (got %d)", probe[1].on_count);
    CHECK(probe[2].on_count == 0, "muted track is silent");
    CHECK(probe[3].on_count == 1 && probe[3].on_at[0] == 40 * TICK,
          "full swing moves the off-beat 16th by 16 ticks (at %u)", probe[3].on_at[0]);
}

static void test_record_and_undo(void)
{
    setup_probes();
    ols_live_command(&live, LIVE_RECORD_TOGGLE, 0, 0, 0.0f, 0);   /* REC also starts the transport */
    CHECK(live.clock.running && live.recording, "REC starts playing and recording");
    run(50 * TICK);
    ols_live_note_on(&live, 0, 67, 110);
    run(30 * TICK);
    ols_live_note_off(&live, 0, 67);
    CHECK(live.track[0].pat.count == 1, "one note recorded (got %u)", live.track[0].pat.count);
    const ols_note_t *n = &live.track[0].pat.n[0];
    CHECK(n->tick == 50 && n->len == 30 && n->note == 67 && n->vel == 110,
          "recorded as played: tick %u len %u", n->tick, n->len);

    live.quantize = 1;
    run(10 * TICK);                            /* now at tick 90 */
    ols_live_note_on(&live, 0, 69, 100);
    run(5 * TICK);
    ols_live_note_off(&live, 0, 69);
    CHECK(pattern_find(&live.track[0].pat, 96, 69) >= 0, "quantized to the 16th (tick 96)");

    int before = probe[0].on_count;
    run(10 * TICK);                            /* passes tick 96: must not double the held note */
    CHECK(probe[0].on_count == before, "a note quantized ahead is not played twice in the take");

    ols_live_command(&live, LIVE_RECORD_TOGGLE, 0, 0, 0.0f, 0);
    CHECK(ols_live_undo(&live) && live.track[0].pat.count == 0, "undo removes the whole take");
    CHECK(ols_live_redo(&live) && live.track[0].pat.count == 2, "redo brings it back");
}

/* bars added to a loop play it again (no silent hole) */
static void test_length_repeats(void)
{
    setup_probes();
    pattern_set_length(&live.track[0].pat, OLS_TICKS_PER_BAR);
    for (int st = 0; st < 4; st++) {
        pattern_add(&live.track[0].pat, (uint32_t)st * 96u, 48, (uint8_t)(60 + st), 100);
    }
    ols_live_command(&live, LIVE_LENGTH, 0, 2 * OLS_TICKS_PER_BAR, 0.0f, 0);
    const ols_pattern_t *p = &live.track[0].pat;
    int found = 0;
    for (uint16_t i = 0; i < p->count; i++) {
        found += p->n[i].tick == OLS_TICKS_PER_BAR + 96u && p->n[i].note == 61;
    }
    CHECK(p->length == 2 * OLS_TICKS_PER_BAR && p->count == 8 && found == 1, "2 bars from 1: the bar twice (%u notes)",
          p->count);
    ols_live_command(&live, LIVE_LENGTH, 0, OLS_TICKS_PER_BAR, 0.0f, 0);
    CHECK(p->count == 4, "back to 1 bar: 4 notes");
}

/* a punch replaces what the playhead passes, then stops; it is one undo step */
static void test_punch(void)
{
    setup_probes();
    ols_live_stop(&live);
    live.current = 0;
    pattern_set_length(&live.track[0].pat, OLS_TICKS_PER_BAR);
    live.track[0].pat.count = 0;
    for (int st = 0; st < 4; st++) {
        pattern_add(&live.track[0].pat, (uint32_t)st * 96u, 48, (uint8_t)(60 + st), 100);
    }
    ols_live_play(&live);
    run(10 * TICK);
    ols_live_command(&live, LIVE_PUNCH, 1, 0, 0.0f, 0);
    CHECK(live.recording && live.punch, "punch: recording");
    run(150 * TICK);                                            /* the playhead passes the note at 96 */
    ols_live_command(&live, LIVE_PUNCH, 0, 0, 0.0f, 0);
    int at96 = 0;
    for (uint16_t i = 0; i < live.track[0].pat.count; i++) {
        at96 += live.track[0].pat.n[i].tick == 96u;
    }
    CHECK(!live.recording && at96 == 0 && live.track[0].pat.count == 3, "punch: the note passed is replaced (%u left)",
          live.track[0].pat.count);
    ols_live_undo_track(&live, 0);
    CHECK(live.track[0].pat.count == 4, "punch: one undo puts it back");
    ols_live_stop(&live);
    /* recording into a muted track: the old loop goes, the track plays */
    live.track[0].mute = 1;
    ols_live_command(&live, LIVE_RECORD_TOGGLE, 0, 0, 0.0f, 0);
    CHECK(live.recording && !live.track[0].mute && live.track[0].pat.count == 0, "REC on a muted track: cleared, playing");
    ols_live_command(&live, LIVE_RECORD_TOGGLE, 0, 0, 0.0f, 0);
    ols_live_undo_track(&live, 0);
    CHECK(live.track[0].pat.count == 4, "...and undo brings the old loop back");
    ols_live_stop(&live);
}

static void test_capture_stopped(void)
{
    setup_probes();
    /* transport stopped: four notes, one per beat, then a little more time */
    for (int beat = 0; beat < 4; beat++) {
        ols_live_note_on(&live, 0, (uint8_t)(60 + beat), 100);
        run(48 * TICK);
        ols_live_note_off(&live, 0, (uint8_t)(60 + beat));
        run(48 * TICK);
    }
    uint32_t free_before = live.clock.free_tick;
    CHECK(ols_live_capture(&live), "capture makes a loop");
    const ols_pattern_t *p = &live.track[0].pat;
    CHECK(p->length == OLS_TICKS_PER_BAR, "one bar (got %u ticks)", p->length);
    CHECK(p->count == 4 && p->n[0].tick == 0 && p->n[1].tick == 96 && p->n[3].tick == 288,
          "notes on the beats");
    CHECK(p->n[0].len == 48, "lengths kept (%u)", p->n[0].len);
    CHECK(live.clock.running, "the transport starts by itself");
    CHECK(live.clock.tick == free_before - (free_before - 4 * 96), "and carries on in time (tick %u)",
          live.clock.tick);
    CHECK(ols_live_undo(&live) && live.track[0].pat.count == 0, "capture is undoable");
}

static void test_capture_running(void)
{
    setup_probes();
    ols_live_play(&live);
    run(OLS_TICKS_PER_BAR * TICK);             /* bar 1 empty */
    for (int i = 0; i < 6; i++) {              /* two bars of dotted 8ths in bars 2-3 */
        ols_live_note_on(&live, 0, 48, 100);
        run(20 * TICK);
        ols_live_note_off(&live, 0, 48);
        run(52 * TICK + (i == 5 ? 0 : 56 * TICK));
    }
    CHECK(ols_live_capture(&live), "capture while playing");
    const ols_pattern_t *p = &live.track[0].pat;
    CHECK(p->length == 2 * OLS_TICKS_PER_BAR, "two bars (got %u)", p->length);
    CHECK(p->count == 6 && p->n[0].tick == 0, "keeps its place in the bars (first at %u)", p->n[0].tick);
}

static void test_undo_depth(void)
{
    setup_probes();
    for (int i = 0; i < 10; i++) {
        ols_live_undo_push(&live, 0);
        pattern_add(&live.track[0].pat, (uint32_t)i * 24u, 12, 60, 100);
    }
    int undone = 0;
    while (ols_live_undo(&live)) {
        undone++;
    }
    CHECK(undone == OLS_UNDO_DEPTH, "%d undo steps (got %d)", OLS_UNDO_DEPTH, undone);
    CHECK(live.track[0].pat.count == 2, "back to the state 8 steps ago (%u notes)", live.track[0].pat.count);
}

static int sorted_and_valid(const ols_pattern_t *p)
{
    for (int i = 0; i < (int)p->count; i++) {
        if (p->n[i].tick >= p->length || p->n[i].len < 1 || p->n[i].vel < 1 || p->n[i].vel > 127) {
            return 0;
        }
        if (i > 0 && p->n[i - 1].tick > p->n[i].tick) {
            return 0;
        }
    }
    return 1;
}

static int same_pattern(const ols_pattern_t *a, const ols_pattern_t *b)
{
    if (a->length != b->length || a->count != b->count) {
        return 0;
    }
    for (int i = 0; i < (int)a->count; i++) {
        if (memcmp(&a->n[i], &b->n[i], sizeof a->n[i]) != 0) {
            return 0;
        }
    }
    return 1;
}

static void test_creative(void)
{
    ols_pattern_t base, p;
    uint32_t rng = 1234u;
    pattern_clear(&base, OLS_TICKS_PER_BAR);
    static const uint8_t notes[] = { 36, 48, 43, 46, 36, 51, 48, 43 };
    for (int s = 0; s < 8; s++) {
        pattern_add(&base, (uint32_t)s * 48u, 20, notes[s], (uint8_t)(80 + s * 5));
    }

    for (int op = 0; op < CREATIVE_COUNT; op++) {
        for (int k = 0; k < 50; k++) {
            pattern_copy(&p, &base);
            creative_apply(&p, (ols_creative_op_t)op, (float)(k % 11) / 10.0f, k % 7 - 3, &rng);
            if (!sorted_and_valid(&p)) {
                CHECK(0, "%s leaves a valid, sorted pattern", CREATIVE_NAMES[op]);
                break;
            }
        }
    }

    pattern_copy(&p, &base);
    creative_rotate(&p, 3);
    creative_rotate(&p, -3);
    CHECK(same_pattern(&p, &base), "rotate +3 then -3 = identity");

    pattern_copy(&p, &base);
    creative_reverse(&p);
    creative_reverse(&p);
    CHECK(same_pattern(&p, &base), "reverse twice = identity");

    pattern_copy(&p, &base);
    creative_expand(&p);
    CHECK(p.length == 2 * OLS_TICKS_PER_BAR && p.n[1].tick == 96, "expand doubles time");
    creative_contract(&p);
    CHECK(same_pattern(&p, &base), "expand then contract = identity");

    pattern_copy(&p, &base);
    creative_transpose(&p, 5);
    CHECK(p.n[0].note == 41, "transpose +5");

    pattern_copy(&p, &base);
    creative_simplify(&p, 1.0f);
    CHECK(p.count >= 1 && p.count <= 3 && pattern_find(&p, 0, 36) >= 0,
          "simplify keeps the downbeat (%u notes left)", p.count);

    pattern_copy(&p, &base);
    creative_fill(&p, &rng, 1.0f);
    CHECK(p.count > base.count, "fill adds notes (%u -> %u)", base.count, p.count);
    for (int i = 0; i < (int)p.count; i++) {
        int known = 0;
        for (int k = 0; k < 8; k++) {
            known |= p.n[i].note == notes[k];
        }
        CHECK(known, "fill only uses the pattern's pitches");
    }

    pattern_copy(&p, &base);
    creative_humanize(&p, &rng, 1.0f);
    int moved = 0;
    for (int i = 0; i < (int)p.count; i++) {
        int32_t d = (int32_t)p.n[i].tick % 48;
        if (d > 24) {
            d -= 48;
        }
        CHECK(d >= -6 && d <= 6, "humanize stays within 6 ticks");
        moved |= d != 0;
    }
    CHECK(moved, "humanize moves something");

    pattern_copy(&p, &base);
    creative_chance(&p, 1.0f);
    CHECK(p.n[0].chance == 100 && p.n[1].chance == 40, "chance spares the beats");

    creative_euclid(&p, 3, 8, 36, 100);
    CHECK(p.count == 3 && p.n[0].tick == 0 && p.n[1].tick == 72 && p.n[2].tick == 144 && p.length == 192,
          "euclid(3,8) = tresillo");
    creative_euclid(&p, 5, 16, 36, 100);
    CHECK(p.count == 5, "euclid(5,16) has 5 hits");
}

static engine_va_t va[OLS_TRACKS];

static void setup_va(void)
{
    ols_engine_t eng[OLS_TRACKS];
    for (int t = 0; t < OLS_TRACKS; t++) {
        engine_va_init(&va[t], 0, 0);
        eng[t].api = &ENGINE_VA_API;
        eng[t].ctx = &va[t];
    }
    ols_live_init(&live, eng, OLS_TRACKS, SR);
}

static void test_scenes_morph_override(void)
{
    setup_va();
    engine_va_set_param(&va[0], VA_P_CUTOFF, 0.2f);
    engine_va_set_param(&va[0], VA_P_OSC1_WAVE, 0.0f);
    ols_live_scene_store(&live, 0);
    engine_va_set_param(&va[0], VA_P_CUTOFF, 0.8f);
    engine_va_set_param(&va[0], VA_P_OSC1_WAVE, 1.0f);
    ols_live_mute(&live, 3, 1);
    ols_live_scene_store(&live, 1);

    ols_live_morph(&live, 0, 1, 0.25f);
    CHECK(fabsf(va[0].p[VA_P_CUTOFF] - 0.35f) < 0.001f, "morph 25%% interpolates (%.3f)", va[0].p[VA_P_CUTOFF]);
    CHECK(va[0].p[VA_P_OSC1_WAVE] < 0.01f && !live.track[3].mute, "switches stay on A below half-way");
    ols_live_morph(&live, 0, 1, 0.75f);
    CHECK(va[0].p[VA_P_OSC1_WAVE] > 0.99f && live.track[3].mute, "and flip to B past half-way");

    ols_live_scene_recall(&live, 0);
    CHECK(fabsf(va[0].p[VA_P_CUTOFF] - 0.2f) < 0.001f && !live.track[3].mute, "recall scene A");

    ols_live_override(&live, OLS_TRACK_ALL, OLS_ROLE_CUTOFF, 0.05f, 1);
    CHECK(va[0].p[VA_P_CUTOFF] < 0.06f && va[2].p[VA_P_CUTOFF] < 0.06f, "hold: filter shut on every track");
    ols_live_override(&live, OLS_TRACK_ALL, OLS_ROLE_CUTOFF, 0.05f, 0);
    CHECK(fabsf(va[0].p[VA_P_CUTOFF] - 0.2f) < 0.001f, "release: springs back");
}

static void test_project_round_trip(void)
{
    static uint8_t a[OLS_PROJECT_MAX], b[OLS_PROJECT_MAX];
    setup_va();
    uint32_t rng = 99u;
    for (int t = 0; t < OLS_TRACKS; t++) {
        creative_euclid(&live.track[t].pat, (uint32_t)(3 + t), 16, (uint8_t)(36 + 7 * t), 100);
        creative_ratchet(&live.track[t].pat, &rng, 1.0f);
        engine_va_set_param(&va[t], VA_P_CUTOFF, 0.1f * (float)(t + 1));
    }
    ols_live_scene_store(&live, 2);
    live.swing = 0.4f;
    clock_set_bpm(&live.clock, 97.5f);

    uint32_t size = ols_project_save(&live, a, sizeof a);
    CHECK(size > 0 && size <= OLS_PROJECT_MAX, "saved (%u bytes, max %u)", size, (unsigned)OLS_PROJECT_MAX);
    CHECK(ols_project_save(&live, a, 10) == 0, "too small a buffer is refused");
    size = ols_project_save(&live, a, sizeof a);

    a[40] ^= 0x10;
    CHECK(!ols_project_load(&live, a, size), "a damaged project is refused");
    a[40] ^= 0x10;

    setup_va();
    CHECK(ols_project_load(&live, a, size), "loads");
    CHECK(fabsf(live.clock.bpm - 97.5f) < 0.01f && live.scene[2].valid && !live.scene[1].valid,
          "tempo and scenes back");
    uint32_t size2 = ols_project_save(&live, b, sizeof b);
    CHECK(size2 == size && memcmp(a, b, size) == 0, "save -> load -> save gives the same bytes");
}

static void test_external_clock(void)
{
    setup_probes();
    pattern_add(&live.track[0].pat, 0, 24, 60, 100);
    pattern_add(&live.track[0].pat, 96, 24, 60, 100);
    ols_live_command(&live, LIVE_EXTERNAL_CLOCK, 1, 0, 0.0f, 0);
    ols_live_midi_realtime(&live, 0xFA);
    run(500);
    CHECK(probe[0].on_count == 0, "after START, waits for the first pulse");

    /* 120 BPM on the MIDI clock at 48 kHz: one pulse every 1000 samples */
    uint32_t first_pulse = probe[0].sample;
    for (int pulse = 0; pulse < 30; pulse++) {
        ols_live_midi_realtime(&live, 0xF8);
        run(1000);
    }
    CHECK(probe[0].on_count == 2, "plays beat 1 and beat 2 (%d notes)", probe[0].on_count);
    CHECK(probe[0].on_at[0] == first_pulse, "beat 1 on the first pulse");
    int32_t err = (int32_t)probe[0].on_at[1] - (int32_t)(first_pulse + 24000u);
    CHECK(err > -300 && err < 300, "beat 2 within a tick of the clock (error %d samples)", err);
    CHECK(fabsf(clock_bpm(&live.clock) - 120.0f) < 1.0f, "measured tempo %.1f", clock_bpm(&live.clock));

    ols_live_midi_realtime(&live, 0xFC);
    CHECK(!live.clock.running, "STOP stops");
}

static void test_stutter(void)
{
    setup_probes();
    for (int s = 0; s < 16; s++) {
        pattern_add(&live.track[0].pat, (uint32_t)s * 24u, 12, (uint8_t)(60 + s), 100);
    }
    ols_live_play(&live);
    run(4 * 24 * TICK + 10);                    /* steps 0..4 played */
    ols_live_stutter(&live, 1, 48);              /* repeat the current 8th: steps 4-5 */
    int before = probe[0].on_count;
    run(4 * 48 * TICK);
    int repeated = 1;
    for (int i = before; i < probe[0].on_count; i++) {
        repeated &= probe[0].on_note[i] == 64 || probe[0].on_note[i] == 65;
    }
    CHECK(probe[0].on_count > before && repeated, "stutter repeats steps 4-5 only");
    /* let go mid-slice: the loop plays to the end of its slice, then the music is back, in time */
    run(24 * TICK);                              /* half way into a slice */
    ols_live_stutter(&live, 0, 0);
    CHECK(live.stutter && live.stutter_end > live.clock.tick, "release waits for the end of the slice");
    run(24 * TICK + 10);
    CHECK(!live.stutter && live.stutter_end == 0u, "...then the loop stops, on the grid of its slice");
    run(24 * TICK);
    uint8_t last = probe[0].on_note[probe[0].on_count - 1];
    CHECK(last == 60 + (uint8_t)(live.clock.tick / 24 % 16), "release: back where the music is");

    /* one track only: the others play on */
    ols_live_stutter_track(&live, 1, 48, 1, 0);
    CHECK(live.stutter && !live.track[0].stutter && live.track[1].stutter, "a loop on one track only");
    ols_live_stutter_track(&live, 0, 0, OLS_TRACK_ALL, 1);
    CHECK(!live.stutter && !live.track[1].stutter, "panic: the loop stops at once");
}

static void test_undo_per_track(void)
{
    setup_probes();
    pattern_add(&live.track[0].pat, 0, 12, 60, 100);
    pattern_add(&live.track[1].pat, 0, 12, 62, 100);
    ols_live_clear(&live, 0);                    /* step 1: track 1 */
    ols_live_clear(&live, 1);                    /* step 2: track 2 */
    CHECK(ols_live_can_undo_track(&live, 0) && ols_live_can_undo_track(&live, 1), "both tracks can undo");
    CHECK(ols_live_undo_track(&live, 0) && live.track[0].pat.count == 1 && live.track[1].pat.count == 0,
          "track 1's undo leaves track 2 alone");
    CHECK(!ols_live_can_undo_track(&live, 0) && ols_live_can_redo_track(&live, 0), "track 1: nothing more, redo ready");
    CHECK(ols_live_undo(&live) && live.track[1].pat.count == 1, "ALL: the newest step left (track 2)");
    CHECK(ols_live_redo_track(&live, 0) && live.track[0].pat.count == 0 && live.track[1].pat.count == 1,
          "track 1's redo, track 2 untouched");
    pattern_add(&live.track[1].pat, 24, 12, 64, 100);
    ols_live_clear(&live, 1);                    /* a new gesture on track 2 forgets its redo */
    CHECK(!ols_live_can_redo_track(&live, 1) && live.track[1].pat.count == 0, "a new gesture forgets the redo");
    CHECK(ols_live_undo_track(&live, 1) && live.track[1].pat.count == 2, "...and undoes itself");
}

static void test_va_split_render(void)
{
    /* the VA engine sounds exactly the same whatever the render is cut into */
    static engine_va_t a, b;
    static float la[4096], ra[4096], lb[4096], rb[4096];
    engine_va_init(&a, 0, 0);
    engine_va_init(&b, 0, 0);
    engine_va_load_preset(&a, VA_PRESET_PAD);
    engine_va_load_preset(&b, VA_PRESET_PAD);
    engine_va_note_on(&a, 60, 100);
    engine_va_note_on(&b, 60, 100);
    engine_va_render(&a, la, ra, 4096);
    int pos = 0;
    static const int cuts[] = { 1, 31, 32, 33, 100, 7, 256, 3 };
    for (int k = 0; pos < 4096; k++) {
        int n = cuts[k % 8];
        if (pos + n > 4096) {
            n = 4096 - pos;
        }
        engine_va_render(&b, lb + pos, rb + pos, n);
        pos += n;
    }
    CHECK(memcmp(la, lb, sizeof la) == 0 && memcmp(ra, rb, sizeof ra) == 0,
          "VA render split anywhere = one render (bit-identical)");
}

/* Every drum: makes a sound, stays in range, ends by itself. Hats are bright, the kick is low. */
static void test_drums(void)
{
    static engine_drum_t d;
    static float l[256], r[256];
    for (uint8_t note = 36; note < 48; note++) {
        engine_drum_init(&d);
        engine_drum_note_on(&d, note, 127);
        float peak = 0.0f, energy = 0.0f;
        int crossings = 0, finite = 1;
        float prev = 0.0f;
        int blocks = 0;
        while (engine_drum_active(&d) && blocks < 1000) {          /* 5.3 s at most */
            engine_drum_render(&d, l, r, 256);
            for (int i = 0; i < 256; i++) {
                float a = fabsf(l[i]);
                finite &= isfinite(l[i]);
                peak = a > peak ? a : peak;
                energy += l[i] * l[i];
                crossings += (prev < 0.0f) != (l[i] < 0.0f);
                prev = l[i];
            }
            blocks++;
        }
        float zcr = (float)crossings / (float)(blocks * 256) * 48000.0f / 2.0f;   /* ~ dominant frequency */
        CHECK(finite && peak > 0.05f && peak < 1.1f && energy > 0.0f,          /* (the kick at 127: 1.03) */
              "drum note %u sounds within range (peak %.2f)", note, peak);
        CHECK(blocks < 1000, "drum note %u ends by itself", note);
        if (note == 36) {
            CHECK(zcr < 150.0f, "the kick is low (%.0f Hz)", zcr);
        }
        if (note == 42) {
            CHECK(zcr > 3000.0f, "the closed hat is bright (%.0f Hz)", zcr);
        }
    }
    engine_drum_init(&d);
    engine_drum_note_on(&d, 46, 100);
    engine_drum_note_on(&d, 42, 100);
    CHECK(!d.voice[DR_OH].active, "the closed hat chokes the open hat");
    CHECK(engine_drum_instrument(36) == DR_KICK && engine_drum_instrument(39) == DR_CLAP
          && engine_drum_instrument(41) == DR_FTOM && engine_drum_instrument(45) == DR_LTOM
          && engine_drum_instrument(48) == DR_MTOM && engine_drum_instrument(50) == DR_HTOM
          && engine_drum_instrument(56) == DR_COWBELL && engine_drum_instrument(81) == DR_TRIANGLE
          && engine_drum_instrument(44) == DR_CH && engine_drum_instrument(51) == DR_OH,
          "General MIDI's drum map (toms 41..50, cowbell 56, triangle 81, pedal hat, ride)");
    CHECK(engine_drum_instrument(60) == DR_KICK && engine_drum_instrument(62) == DR_SNARE,
          "a note GM does not name plays as its place in the GM octave (C4 kick, D4 snare)");
    {
        float buf[256] = { 0 };
        engine_drum_init(&d);
        engine_drum_note_on(&d, 81, 120);
        dr_render_voice(&d, DR_COWBELL, buf, 256);
        float peak = 0.0f;
        for (int i = 0; i < 256; i++) {
            peak = fabsf(buf[i]) > peak ? fabsf(buf[i]) : peak;
        }
        CHECK(peak > 0.05f && d.voice[DR_COWBELL].active, "the triangle rings on the cowbell's voice (peak %.2f)", (double)peak);
    }
}

static engine_drum_t drum[OLS_TRACKS];
static ols_slot_t slots[OLS_TRACKS];

static void setup_band(void)
{
    ols_engine_t eng[OLS_TRACKS];
    for (int t = 0; t < OLS_TRACKS; t++) {
        engine_va_init(&va[t], 0, 0);
        engine_drum_init(&drum[t]);
        slots[t].va = &va[t];
        slots[t].drum = &drum[t];
        eng[t] = ols_slot_engine(&slots[t], OLS_KIND_VA);
    }
    ols_live_init(&live, eng, OLS_TRACKS, SR);
    ols_bank_attach(&live, slots);
}

static void test_bank_and_engines(void)
{
    static uint8_t buf[OLS_PROJECT_MAX];
    setup_band();
    ols_live_scene_store(&live, 0);                              /* track 4 still VA */
    ols_bank_select(&live, slots, 3, OLS_SOUND_FIRST_DRUM + DR_KIT_TIGHT);
    CHECK(live.track[3].kind == OLS_KIND_DRUM && live.track[3].engine.api == &ENGINE_DRUM_API,
          "a drum sound plugs the drum engine into the track");
    CHECK(!strcmp(ols_sound_name(OLS_SOUND_FIRST_DRUM), "DRUMS 808") && !strcmp(ols_sound_name(VA_PRESET_ACID), "ACID 303")
          && !strcmp(ols_sound_name(OLS_SOUND_FIRST_DRUM + DR_KIT_909), "DRUMS 909"),
          "sound names");

    creative_euclid(&live.track[3].pat, 4, 16, 36, 120);
    ols_live_play(&live);
    for (int i = 0; i < 20; i++) {
        ols_live_render(&live, bl, br, 256);
    }
    CHECK(engine_drum_active(&drum[3]) > 0, "the loop plays the kick on the drum engine");

    float cutoff = drum[3].p[DR_P_CUTOFF];
    ols_live_override(&live, OLS_TRACK_ALL, OLS_ROLE_DELAY_MIX, 0.9f, 1);   /* drums have no delay */
    ols_live_override(&live, OLS_TRACK_ALL, OLS_ROLE_CUTOFF, 0.1f, 1);
    CHECK(fabsf(drum[3].p[DR_P_CUTOFF] - 0.1f) < 0.001f && fabsf(va[0].p[VA_P_CUTOFF] - 0.1f) < 0.001f,
          "a role override reaches the drums and the synths");
    ols_live_override(&live, OLS_TRACK_ALL, OLS_ROLE_CUTOFF, 0.1f, 0);
    ols_live_override(&live, OLS_TRACK_ALL, OLS_ROLE_DELAY_MIX, 0.9f, 0);
    CHECK(fabsf(drum[3].p[DR_P_CUTOFF] - cutoff) < 0.001f, "and springs back");

    engine_drum_set_param(&drum[3], DR_P_TUNE, 0.8f);
    ols_live_scene_recall(&live, 0);                             /* stored when track 4 was a synth */
    CHECK(fabsf(drum[3].p[DR_P_TUNE] - 0.8f) < 0.001f, "a scene from another engine leaves the drums alone");

    uint32_t size = ols_project_save(&live, buf, sizeof buf);
    setup_band();
    CHECK(ols_project_load(&live, buf, size), "a set with a drum track loads");
    CHECK(live.track[3].kind == OLS_KIND_DRUM && fabsf(drum[3].p[DR_P_TUNE] - 0.8f) < 0.001f,
          "the drum engine and its sound come back");
}

/* A drum machine as the master: its notes must not play our synth when we
 * only want its clock; its clock, read once per audio block, must not
 * jitter when the platform time-stamps it; a program change recalls a
 * scene; aftertouch reaches the engine. */
static void test_midi_follow(void)
{
    setup_band();
    midi_in_t m;
    ols_engine_t input = ols_live_input(&live);
    midi_in_init(&m, &input, MIDI_NONE);                      /* clock only */
    midi_in_on_realtime(&m, ols_live_midi_realtime, &live);
    ols_live_command(&live, LIVE_EXTERNAL_CLOCK, 1, 0, 0.0f, 0);

    static const uint8_t kick[3] = { 0x99, 36, 100 };          /* a kick on channel 10 */
    for (int i = 0; i < 3; i++) {
        midi_in_byte(&m, kick[i]);
    }
    CHECK(engine_drum_active(&drum[0]) == 0 && va[0].voice[0].amp.stage == ENV_IDLE,
          "MIDI_NONE: the master's notes do not play us");

    /* 120 BPM = a pulse every 1000 samples, read only at block starts (256 samples apart) */
    midi_in_byte(&m, 0xFA);
    uint32_t now = 0, next_pulse = 0;
    float worst = 0.0f;
    for (int block = 0; block < 400; block++) {
        while (next_pulse <= now) {
            live.midi_late = now - next_pulse;                  /* it came this long before we read it */
            midi_in_byte(&m, 0xF8);
            next_pulse += 1000u;
        }
        live.midi_late = 0;
        ols_live_render(&live, bl, br, 256);
        now += 256u;
        if (block > 100) {
            float err = fabsf(clock_bpm(&live.clock) - 120.0f);
            worst = err > worst ? err : worst;
        }
    }
    CHECK(worst < 0.05f, "time-stamped MIDI clock: steady tempo (worst error %.3f BPM)", worst);
    CHECK(live.clock.running && live.clock.tick > 380u, "and the loops follow it (tick %u)", live.clock.tick);

    engine_va_set_param(&va[0], VA_P_CUTOFF, 0.3f);
    ols_live_scene_store(&live, 2);
    engine_va_set_param(&va[0], VA_P_CUTOFF, 0.9f);
    midi_in_init(&m, &input, MIDI_OMNI);
    midi_in_on_program(&m, ols_live_midi_program, &live);
    midi_in_byte(&m, 0xCF);
    midi_in_byte(&m, 2);
    CHECK(fabsf(va[0].p[VA_P_CUTOFF] - 0.3f) < 0.001f, "program change 2 recalls scene 3");

    midi_in_byte(&m, 0xD0);
    midi_in_byte(&m, 127);
    CHECK(va[0].d.pressure > 0.99f, "channel pressure reaches the selected track's synth");
}

/* Count-in, metronome, tap tempo, user sounds, the bitcrusher. */
static void test_live_helpers(void)
{
    setup_probes();
    ols_live_command(&live, LIVE_METRONOME, 1, 0, 0.0f, 0);
    ols_live_command(&live, LIVE_RECORD_TOGGLE, 0, 0, 0.0f, 0);      /* from stop: one bar counted in */
    run(1000);
    float loud;
    CHECK(live.rec_from == OLS_TICKS_PER_BAR, "REC from stop with the metronome counts one bar in");
    run(100 * TICK);
    ols_live_note_on(&live, 0, 60, 100);
    run(10 * TICK);
    ols_live_note_off(&live, 0, 60);
    CHECK(live.track[0].pat.count == 0, "notes during the count-in are not recorded");
    run(OLS_TICKS_PER_BAR * TICK);
    ols_live_note_on(&live, 0, 62, 100);
    run(10 * TICK);
    ols_live_note_off(&live, 0, 62);
    CHECK(live.track[0].pat.count == 1, "after the count-in they are");

    /* the metronome is heard: render a beat with silent engines */
    setup_probes();
    ols_live_command(&live, LIVE_METRONOME, 1, 0, 0.0f, 0);
    ols_live_play(&live);
    ols_live_render(&live, bl, br, 256);
    loud = 0.0f;
    for (int i = 0; i < 256; i++) {
        float a = fabsf(bl[i]);
        loud = a > loud ? a : loud;
    }
    CHECK(loud > 0.1f, "the metronome ticks on the beat (%.2f)", loud);

    /* tap tempo: taps every 0.5 s = 120 BPM, from 100 */
    clock_set_bpm(&live.clock, 100.0f);
    for (int t = 0; t < 4; t++) {
        ols_live_command(&live, LIVE_TAP, 0, 0, 0.0f, 0);
        run(24000);
    }
    CHECK(fabsf(live.clock.bpm - 120.0f) < 0.5f, "tap tempo: %.1f BPM", live.clock.bpm);

    /* user sounds: keep track 1's sound, change it, bring it back, also onto another track */
    static ols_user_sound_t user;
    setup_band();
    ols_bank_select(&live, slots, 0, VA_PRESET_ACID);
    engine_va_set_param(&va[0], VA_P_CUTOFF, 0.123f);
    ols_bank_store_user(&live, 0, &user);
    ols_bank_select(&live, slots, 0, VA_PRESET_PAD);
    ols_bank_select(&live, slots, 3, OLS_SOUND_FIRST_DRUM);
    ols_bank_select_user(&live, slots, 3, &user);
    CHECK(live.track[3].kind == OLS_KIND_VA && fabsf(va[3].p[VA_P_CUTOFF] - 0.123f) < 0.001f
          && fabsf(va[3].p[VA_P_RESO] - 0.85f) < 0.001f,
          "a user sound (ACID, cutoff changed) comes back, on any track, with its engine");
    CHECK(!strcmp(ols_sound_name(OLS_SOUND_FIRST_USER + 2), "USER 3"), "user sound names");

    /* the bitcrusher quantises */
    static engine_va_t crushed;
    engine_va_init(&crushed, 0, 0);
    engine_va_set_param(&crushed, VA_P_CRUSH, 1.0f);
    CHECK(crushed.d.crush_hold == 16 && crushed.d.crush_levels > 15.0f && crushed.d.crush_levels < 17.0f,
          "crush 100%% = 4 bits, 1/16 of the rate");
}

/* DJ moves: a break comes back on the one, a scene waits for the bar, a cross-fade glides. */
static void test_transitions(void)
{
    setup_probes();
    pattern_add(&live.track[3].pat, 0, 12, 36, 120);
    pattern_add(&live.track[3].pat, 192, 12, 36, 120);
    ols_live_play(&live);
    run(10 * TICK);
    ols_live_hold_mute(&live, 1u << 3, 1);                     /* break: drums out now */
    int before = probe[3].on_count;
    run(300 * TICK);
    CHECK(probe[3].on_count == before, "break: the held-out track is silent");
    ols_live_hold_mute(&live, 1u << 3, 0);                     /* released mid-bar... */
    run(60 * TICK);                                            /* ...tick 370: still out */
    CHECK(probe[3].on_count == before && live.hold_mute != 0, "it waits for the next bar");
    run(20 * TICK);                                            /* past tick 384 */
    CHECK(probe[3].on_count == before + 1 && probe[3].on_at[before] == OLS_TICKS_PER_BAR * TICK,
          "and drops back exactly on the one");

    setup_va();
    engine_va_set_param(&va[0], VA_P_CUTOFF, 0.2f);
    ols_live_scene_store(&live, 0);
    engine_va_set_param(&va[0], VA_P_CUTOFF, 0.8f);
    ols_live_scene_store(&live, 1);
    ols_live_play(&live);
    for (int i = 0; i < 10; i++) {
        ols_live_render(&live, bl, br, 256);
    }
    ols_live_command(&live, LIVE_SCENE_RECALL, 0, 0, 0.0f, 0);   /* playing: waits for the bar */
    CHECK(fabsf(va[0].p[VA_P_CUTOFF] - 0.8f) < 0.001f, "a scene asked while playing waits");
    while (live.clock.tick < OLS_TICKS_PER_BAR + 2u) {
        ols_live_render(&live, bl, br, 256);
    }
    CHECK(fabsf(va[0].p[VA_P_CUTOFF] - 0.2f) < 0.001f, "and lands on the next bar (cut)");

    ols_live_command(&live, LIVE_XFADE, 2, 0, 0.0f, 0);           /* 2-bar cross-fade back to scene 2 */
    ols_live_command(&live, LIVE_SCENE_RECALL, 1, 0, 0.0f, 0);
    while (live.clock.tick < 3u * OLS_TICKS_PER_BAR) {             /* one bar into the fade */
        ols_live_render(&live, bl, br, 256);
    }
    float mid = va[0].p[VA_P_CUTOFF];
    CHECK(mid > 0.4f && mid < 0.6f, "cross-fade half-way after one of two bars (%.2f)", mid);
    while (live.clock.tick < 4u * OLS_TICKS_PER_BAR + 8u) {
        ols_live_render(&live, bl, br, 256);
    }
    CHECK(fabsf(va[0].p[VA_P_CUTOFF] - 0.8f) < 0.001f && !live.xfade_active, "and lands on the scene");

    /* the 1-bar loop roll */
    setup_probes();
    for (int st = 0; st < 32; st++) {
        pattern_add(&live.track[0].pat, (uint32_t)st * 24u, 12, (uint8_t)(40 + st), 100);
    }
    pattern_set_length(&live.track[0].pat, 2 * OLS_TICKS_PER_BAR);
    for (int st = 0; st < 32; st++) {
        pattern_add(&live.track[0].pat, (uint32_t)st * 24u, 12, (uint8_t)(40 + st), 100);
    }
    ols_live_play(&live);
    run(OLS_TICKS_PER_BAR * TICK + 10);
    ols_live_stutter(&live, 1, OLS_TICKS_PER_BAR);                 /* loop bar 2 */
    run(OLS_TICKS_PER_BAR * TICK);
    run(30 * TICK);
    uint8_t last = probe[0].on_note[probe[0].on_count - 1];
    CHECK(last == 56 || last == 57, "a 1-bar loop replays bar 2 (note %u)", last);
}

/* Instrument macros, sidechain, scene chain, acid accent. */
static void test_macros_pump_chain(void)
{
    setup_band();
    ols_bank_select(&live, slots, 3, OLS_SOUND_FIRST_DRUM);
    float cut0 = va[0].d.cutoff_note, drv0 = va[0].d.drive_gain, dec0 = drum[3].decay;
    ols_live_macro(&live, OLS_MACRO_ENERGY, 1.0f);
    CHECK(va[0].d.cutoff_note > cut0 + 10.0f && va[0].d.drive_gain > drv0 && drum[3].decay < dec0,
          "ENERGY: brighter, more drive, tighter drums");
    CHECK(fabsf(va[0].p[VA_P_CUTOFF] - 0.65f) < 0.001f, "the stored sound does not move (scenes, saves)");
    CHECK(live.density > 0.4f, "ENERGY also makes the loops denser");
    ols_live_macro(&live, OLS_MACRO_ENERGY, 0.5f);
    CHECK(fabsf(va[0].d.cutoff_note - cut0) < 0.001f && fabsf(drum[3].decay - dec0) < 0.0001f,
          "back to 50%%: exactly the sound again");
    ols_live_macro(&live, OLS_MACRO_SPACE, 1.0f);
    CHECK(va[0].d.delay_mix > 0.3f, "SPACE: more delay");
    ols_live_macro(&live, OLS_MACRO_SPACE, 0.5f);
    ols_live_macro(&live, OLS_MACRO_TENSION, 1.0f);
    CHECK(va[1].d.reso > 0.5f && va[1].d.crush_levels > 0.0f, "TENSION: resonance and grit");
    ols_live_macro(&live, OLS_MACRO_TENSION, 0.5f);

    /* DENSITY at 0: off-beats thinned */
    setup_probes();
    for (int st = 0; st < 16; st++) {
        pattern_add(&live.track[0].pat, (uint32_t)st * 24u, 12, 60, 100);
    }
    ols_live_macro(&live, OLS_MACRO_DENSITY, 0.0f);
    ols_live_play(&live);
    run(4 * OLS_TICKS_PER_BAR * TICK);
    CHECK(probe[0].on_count < 50 && probe[0].on_count >= 16, "DENSITY 0: fewer notes (%d of 64), beats kept",
          probe[0].on_count);

    /* PUMP: a kick ducks the pad */
    setup_band();
    ols_bank_select(&live, slots, 3, OLS_SOUND_FIRST_DRUM);
    live.duck_kind = OLS_KIND_DRUM;
    ols_bank_select(&live, slots, 0, VA_PRESET_ORGAN);
    engine_va_note_on(&va[0], 60, 100);
    for (int i = 0; i < 40; i++) {
        ols_live_render(&live, bl, br, 256);
    }
    float before = 0.0f;
    for (int i = 0; i < 256; i++) {
        before = fabsf(bl[i]) > before ? fabsf(bl[i]) : before;
    }
    live.pump = 1.0f;
    engine_drum_note_on(&drum[3], 36, 127);
    ols_live_render(&live, bl, br, 256);
    CHECK(live.duck_gain < 0.4f, "PUMP: the kick ducks the other tracks (gain %.2f)", live.duck_gain);
    for (int i = 0; i < 200; i++) {
        ols_live_render(&live, bl, br, 256);
    }
    CHECK(live.duck_gain > 0.95f && before > 0.0f, "and they come back after the kick (gain %.2f)", live.duck_gain);

    /* scene chain: 1 bar each, scenes 1 and 3 stored */
    setup_va();
    engine_va_set_param(&va[0], VA_P_CUTOFF, 0.2f);
    ols_live_scene_store(&live, 0);
    engine_va_set_param(&va[0], VA_P_CUTOFF, 0.9f);
    ols_live_scene_store(&live, 2);
    ols_live_scene_recall(&live, 0);
    ols_live_play(&live);
    ols_live_command(&live, LIVE_CHAIN, 1, 1, 0.0f, 0);
    while (live.clock.tick < OLS_TICKS_PER_BAR + 4u) {
        ols_live_render(&live, bl, br, 256);
    }
    CHECK(fabsf(va[0].p[VA_P_CUTOFF] - 0.9f) < 0.001f, "chain: bar 2 plays scene 3");
    while (live.clock.tick < 2u * OLS_TICKS_PER_BAR + 4u) {
        ols_live_render(&live, bl, br, 256);
    }
    CHECK(fabsf(va[0].p[VA_P_CUTOFF] - 0.2f) < 0.001f, "and bar 3 goes round to scene 1");

    /* acid accent: a hard note opens the filter more */
    static engine_va_t acid;
    engine_va_init(&acid, 0, 0);
    engine_va_load_preset(&acid, VA_PRESET_ACID);
    CHECK(acid.d.vel_filter > 20.0f, "ACID has accents (velocity opens the filter)");
}

/* ---- the modulation matrix and the LFO timing (VA) ---- */

static float rms_of(const float *x, int n)
{
    float e = 0.0f;
    for (int i = 0; i < n; i++) {
        e += x[i] * x[i];
    }
    return sqrtf(e / (float)n);
}

/* a sound with a slot whose source or destination is OFF sounds exactly as without the matrix */
static void test_mod_off_is_identical(void)
{
    static engine_va_t a, b;
    static float la[4096], ra[4096], lb[4096], rb[4096];
    engine_va_init(&a, 0, 0);
    engine_va_init(&b, 0, 0);
    engine_va_load_preset(&a, VA_PRESET_PAD);
    engine_va_load_preset(&b, VA_PRESET_PAD);
    engine_va_set_param(&b, VA_P_MOD1_SRC, 0.15f);               /* LFO1 ... */
    engine_va_set_param(&b, VA_P_MOD1_DST, 0.0f);                /* ... to OFF */
    engine_va_set_param(&b, VA_P_MOD1_AMT, 1.0f);
    engine_va_note_on(&a, 60, 100);
    engine_va_note_on(&b, 60, 100);
    engine_va_render(&a, la, ra, 4096);
    engine_va_render(&b, lb, rb, 4096);
    CHECK(memcmp(la, lb, sizeof la) == 0 && memcmp(ra, rb, sizeof ra) == 0,
          "a slot to OFF changes nothing (bit-identical)");
    CHECK(!b.d.mod_voice && !b.d.mod_engine, "...and costs nothing (no slot active)");
}

static float va_choice(int index, int count)   /* the middle of a choice, as a knob would land on it */
{
    return ((float)index + 0.5f) / (float)count;
}

static void test_mod_matrix(void)
{
    static engine_va_t e;
    static float l[2048], r[2048];

    /* VELOCITY -> LEVEL, negative: soft notes quieter than with no slot, hard ones as loud */
    engine_va_init(&e, 0, 0);
    engine_va_set_param(&e, VA_P_VEL_SENS, 0.0f);
    engine_va_note_on(&e, 60, 30);
    engine_va_render(&e, l, r, 2048);
    float plain = rms_of(l + 1024, 1024);
    engine_va_init(&e, 0, 0);
    engine_va_set_param(&e, VA_P_VEL_SENS, 0.0f);
    engine_va_set_param(&e, VA_P_MOD1_SRC, va_choice(VA_SRC_VELOCITY, VA_SRC_COUNT));
    engine_va_set_param(&e, VA_P_MOD1_DST, va_choice(VA_DST_LEVEL, VA_DST_COUNT));
    engine_va_set_param(&e, VA_P_MOD1_AMT, 0.0f);                /* -100 %: level x (1 - velocity) */
    CHECK(e.d.mod_voice && e.d.mod_src[0] == VA_SRC_VELOCITY && e.d.mod_dst[0] == VA_DST_LEVEL,
          "a slot reads its source and destination");
    engine_va_note_on(&e, 60, 30);
    engine_va_render(&e, l, r, 2048);
    float modded = rms_of(l + 1024, 1024);
    CHECK(modded < plain * 0.85f && modded > plain * 0.6f, "VELOCITY -> LEVEL lowers a soft note (%.3f vs %.3f)",
          modded, plain);

    /* RANDOM is drawn at each note */
    engine_va_note_on(&e, 62, 100);
    float r1 = e.voice[1].rnd;
    engine_va_note_on(&e, 64, 100);
    CHECK(e.voice[2].rnd != r1 && r1 >= -1.0f && r1 <= 1.0f, "RANDOM: a new value per note");

    /* KEY -> CUTOFF opens the filter on high notes */
    engine_va_init(&e, 0, 0);
    engine_va_set_param(&e, VA_P_KEYTRACK, 0.0f);
    engine_va_set_param(&e, VA_P_CUTOFF, 0.3f);
    engine_va_set_param(&e, VA_P_FENV_AMT, 0.5f);
    engine_va_set_param(&e, VA_P_MOD1_SRC, va_choice(VA_SRC_KEY, VA_SRC_COUNT));
    engine_va_set_param(&e, VA_P_MOD1_DST, va_choice(VA_DST_CUTOFF, VA_DST_COUNT));
    engine_va_set_param(&e, VA_P_MOD1_AMT, 1.0f);
    engine_va_note_on(&e, 96, 100);
    engine_va_render(&e, l, r, 64);
    float k_high = e.voice[0].svf.a1;
    engine_va_all_notes_off(&e);
    engine_va_init(&e, 0, 0);
    engine_va_set_param(&e, VA_P_KEYTRACK, 0.0f);
    engine_va_set_param(&e, VA_P_CUTOFF, 0.3f);
    engine_va_set_param(&e, VA_P_FENV_AMT, 0.5f);
    engine_va_note_on(&e, 96, 100);
    engine_va_render(&e, l, r, 64);
    CHECK(e.voice[0].svf.a1 > k_high, "KEY -> CUTOFF: a high note opens the filter");

    /* LFO1 -> DELAY (engine-wide) */
    static int16_t dbuf[8192];
    engine_va_init(&e, dbuf, 8192);
    engine_va_set_param(&e, VA_P_LFO1_WAVE, va_choice(LFO_SQUARE, LFO_WAVE_COUNT));
    engine_va_set_param(&e, VA_P_MOD2_SRC, va_choice(VA_SRC_LFO1, VA_SRC_COUNT));
    engine_va_set_param(&e, VA_P_MOD2_DST, va_choice(VA_DST_DELAY, VA_DST_COUNT));
    engine_va_set_param(&e, VA_P_MOD2_AMT, 1.0f);
    CHECK(e.d.mod_engine && !e.d.mod_voice, "a slot on the delay acts on the engine");
    engine_va_note_on(&e, 60, 100);
    engine_va_render(&e, l, r, 256);
    CHECK(e.mod_engine_now[VA_DST_DELAY] > 0.99f || e.mod_engine_now[VA_DST_DELAY] < -0.99f,
          "LFO1 square drives the delay send (%.2f)", e.mod_engine_now[VA_DST_DELAY]);
}

static void test_lfo_timing(void)
{
    static engine_va_t e;
    static float l[512], r[512];

    /* SYNC 1/4: the phase follows the beat, whatever the rate knob says */
    engine_va_init(&e, 0, 0);
    engine_va_set_param(&e, VA_P_LFO1_WAVE, va_choice(LFO_SQUARE, LFO_WAVE_COUNT));
    engine_va_set_param(&e, VA_P_LFO1_RATE, 0.0f);               /* slowest: free, it would not move */
    engine_va_set_param(&e, VA_P_LFO1_SYNC, va_choice(7, VA_LFO_SYNCS));
    CHECK(e.d.lfo_sync[0] == 1.0f, "SYNC 1/4 = one beat (%.3f)", e.d.lfo_sync[0]);
    engine_va_clock(&e, 120.0f, 0.1f);
    engine_va_render(&e, l, r, 64);
    float first = e.lfo1_now;
    engine_va_clock(&e, 120.0f, 0.6f);
    engine_va_render(&e, l, r, 64);
    CHECK(first > 0.99f && e.lfo1_now < -0.99f, "tempo-locked square: up on the beat, down half-way");
    engine_va_clock(&e, 120.0f, 7.1f);
    engine_va_render(&e, l, r, 64);
    CHECK(e.lfo1_now > 0.99f, "...on every beat");

    /* between two clock calls, the beat moves with the samples: 0.5 beat at 120 BPM = 0.25 s */
    engine_va_clock(&e, 120.0f, 0.0f);
    for (int i = 0; i < (int)(DSP_SR * 0.3f) / 64; i++) {
        engine_va_render(&e, l, r, 64);
    }
    CHECK(e.lfo1_now < -0.99f, "the beat advances between clock calls");

    /* RETRIG: the first key restarts the cycle */
    engine_va_init(&e, 0, 0);
    engine_va_set_param(&e, VA_P_LFO1_WAVE, va_choice(LFO_TRIANGLE, LFO_WAVE_COUNT));
    engine_va_set_param(&e, VA_P_LFO1_RATE, 0.5f);
    engine_va_render(&e, l, r, 512);
    engine_va_render(&e, l, r, 512);
    engine_va_set_param(&e, VA_P_LFO1_TRIG, va_choice(VA_TRIG_RETRIG, VA_TRIG_COUNT));
    engine_va_note_on(&e, 60, 100);
    CHECK(e.lfo1.phase == 0.0f, "RETRIG: the first key restarts the LFO");
    engine_va_render(&e, l, r, 512);
    float phase = e.lfo1.phase;
    engine_va_note_on(&e, 64, 100);                              /* a second key held: no restart */
    CHECK(e.lfo1.phase == phase, "...a key added to a chord does not");

    /* ONE: one cycle, then it holds */
    engine_va_init(&e, 0, 0);
    engine_va_set_param(&e, VA_P_LFO1_WAVE, va_choice(LFO_TRIANGLE, LFO_WAVE_COUNT));
    engine_va_set_param(&e, VA_P_LFO1_RATE, 1.0f);               /* 30 Hz: a cycle is 33 ms */
    engine_va_set_param(&e, VA_P_LFO1_TRIG, va_choice(VA_TRIG_ONESHOT, VA_TRIG_COUNT));
    engine_va_note_on(&e, 60, 100);
    for (int i = 0; i < 40; i++) {
        engine_va_render(&e, l, r, 512);
    }
    float held = e.lfo1_now;
    engine_va_render(&e, l, r, 512);
    CHECK(e.lfo_done[0] && e.lfo1_now == held, "ONE: after its cycle the LFO holds");

    /* the Live Core gives every engine its clock */
    setup_va();
    clock_set_bpm(&live.clock, 100.0f);
    ols_live_play(&live);
    static float ll[4800], lr[4800];
    ols_live_render(&live, ll, lr, 4800);                        /* 0.1 s */
    CHECK(fabsf(va[0].bpm - 100.0f) < 0.01f && va[2].beats > 0.1f && va[2].beats < 0.2f,
          "the Live Core tells the engines the tempo and the beat (%.1f BPM, %.3f beats)", va[0].bpm, va[2].beats);
}

/* parameters past the 0.3 ones (the matrix) go into projects and come back */
static void test_mod_in_projects(void)
{
    setup_va();
    engine_va_set_param(&va[1], VA_P_MOD4_AMT, 0.8f);
    engine_va_set_param(&va[1], VA_P_LFO2_SYNC, 0.5f);
    static uint8_t buf[OLS_PROJECT_MAX];
    uint32_t size = ols_project_save(&live, buf, sizeof buf);
    CHECK(size > 0 && size <= OLS_PROJECT_MAX, "a project with the matrix fits (%u bytes)", size);
    setup_va();
    int loaded = ols_project_load(&live, buf, size);
    CHECK(loaded && fabsf(va[1].p[VA_P_MOD4_AMT] - 0.8f) < 0.001f && fabsf(va[1].p[VA_P_LFO2_SYNC] - 0.5f) < 0.001f,
          "...and comes back (%d, %.3f, %.3f)", loaded, va[1].p[VA_P_MOD4_AMT], va[1].p[VA_P_LFO2_SYNC]);
}

int main(void)
{
    test_sample_accurate_notes();
    test_ratchet_chance_mute_swing();
    test_record_and_undo();
    test_capture_stopped();
    test_length_repeats();
    test_punch();
    test_capture_running();
    test_undo_depth();
    test_creative();
    test_scenes_morph_override();
    test_project_round_trip();
    test_external_clock();
    test_stutter();
    test_undo_per_track();
    test_va_split_render();
    test_drums();
    test_bank_and_engines();
    test_midi_follow();
    test_live_helpers();
    test_transitions();
    test_macros_pump_chain();
    test_mod_off_is_identical();
    test_mod_matrix();
    test_lfo_timing();
    test_mod_in_projects();

    printf("live core: %d checks, %d failed\n", checks, failures);
    return failures ? 1 : 0;
}
