/*
 * OpenVALive desktop - bridge between the threads and the Live Core.
 *
 * The audio thread is the only one that changes the engines and the Live
 * Core. The others send events (main thread) or MIDI bytes (MIDI thread),
 * applied at the start of every audio callback. The main thread only reads
 * the Live Core, to show it.
 */
#include <stdio.h>
#include "desktop.h"
#include "../../core/live/project.h"

desktop_t app;

static int16_t delay_ram[TRACKS][DESKTOP_DELAY_LEN];

const char *const KNOB_NAME[KNOB_COUNT] = { "CUTOFF", "RESO", "ENV", "FX", "MORPH", "SWING", "TEMPO", "LEVEL",
                                            "ENERGY", "SPACE", "TENSION", "DENSITY", "PUMP" };

/* a band ready to play: keys, bass, lead, drums (the FM-1 starts the same way) */
static const int DEFAULT_PRESET[TRACKS] = { VA_PRESET_INIT, VA_PRESET_BASS, VA_PRESET_LEAD,
                                            OLS_SOUND_FIRST_DRUM + DR_KIT_808 };

const char *knob_name(int knob)
{
    const ols_engine_api_t *api = app.live.track[app.track].engine.api;
    return knob < OLS_MACROS && api->macro_names ? api->macro_names[knob] : KNOB_NAME[knob];
}

void desktop_init_synth(float sample_rate)
{
    ols_engine_t engines[TRACKS];

    app.sample_rate = sample_rate;
    evq_init(&app.events);
    for (int t = 0; t < TRACKS; t++) {
        engine_va_init(&app.va[t], delay_ram[t], DESKTOP_DELAY_LEN);
        engine_drum_init(&app.drum[t]);
        app.slots[t].va = &app.va[t];
        app.slots[t].drum = &app.drum[t];
        engines[t] = ols_slot_engine(&app.slots[t], OLS_KIND_VA);
    }
    ols_live_init(&app.live, engines, TRACKS, sample_rate);
    ols_bank_attach(&app.live, app.slots);
    app.live.duck_kind = OLS_KIND_DRUM;              /* PUMP: the drum track ducks the others */
    for (int t = 0; t < TRACKS; t++) {
        app.preset[t] = DEFAULT_PRESET[t];
        if (t == 0 && app.start_preset >= 0) {
            app.preset[t] = app.start_preset;           /* --preset */
        }
        ols_bank_select(&app.live, app.slots, t, app.preset[t]);
    }

    app.live_input = ols_live_input(&app.live);
    midi_in_init(&app.midi, &app.live_input, MIDI_OMNI);
    midi_in_on_realtime(&app.midi, ols_live_midi_realtime, &app.live);
    midi_in_on_program(&app.midi, ols_live_midi_program, &app.live);   /* program change n = scene n + 1 */
    midi_in_map_cc(&app.midi, 74, VA_P_CUTOFF);
    midi_in_map_cc(&app.midi, 71, VA_P_RESO);
    midi_in_map_cc(&app.midi, 73, VA_P_A_ATTACK);
    midi_in_map_cc(&app.midi, 72, VA_P_A_RELEASE);
    midi_in_map_cc(&app.midi, 5, VA_P_GLIDE);
    midi_in_map_cc(&app.midi, 1, VA_P_LFO1_PITCH);
    midi_in_map_cc(&app.midi, 93, VA_P_CHORUS_MIX);
    midi_in_map_cc(&app.midi, 94, VA_P_DELAY_MIX);

    app.bpm = 120.0f;
    app.euclid_hits = 3;
    app.scene_prev = app.scene_last = -1;
}

/* Any thread except audio: queue an event for the audio thread. */
void desktop_send(uint8_t type, uint8_t target, uint8_t a, uint8_t b, uint16_t id, float v)
{
    ols_event_t e;
    e.at = app.frames_rendered;
    e.type = type;
    e.target = target;
    e.a = a;
    e.b = b;
    e.id = id;
    e.reserved = 0;
    e.v = v;
    evq_push(&app.events, &e);
}

void desktop_live(ols_live_cmd_t cmd, uint8_t b, uint16_t id, float v)
{
    desktop_send(EV_LIVE, (uint8_t)app.track, (uint8_t)cmd, b, id, v);
}

static void apply_event(const ols_event_t *e)
{
    if (e->type == EV_PRESET) {
        int t = e->target < TRACKS ? e->target : app.live.current;
        ols_bank_select(&app.live, app.slots, t, e->a);
        return;
    }
    ols_live_apply_event(&app.live, e);
}

static int count_voices(void)
{
    int n = 0;
    for (int t = 0; t < TRACKS; t++) {
        for (int i = 0; i < VA_VOICES; i++) {
            if (app.va[t].voice[i].amp.stage != ENV_IDLE) {
                n++;
            }
        }
        n += engine_drum_active(&app.drum[t]);
    }
    return n;
}

/* Audio thread: events, MIDI, then the Live Core, written as interleaved stereo. */
void desktop_render(float *interleaved, int frames)
{
    ols_event_t e;
    while (evq_pop(&app.events, &e)) {
        apply_event(&e);
    }

    uint8_t byte;
    while (byte_ring_pop(&app.midi_bytes, &byte)) {
        midi_in_byte(&app.midi, byte);
    }

    static float left[DESKTOP_MAX_BLOCK];
    static float right[DESKTOP_MAX_BLOCK];

    int done = 0;
    while (done < frames) {
        int n = frames - done;
        if (n > DESKTOP_MAX_BLOCK) {
            n = DESKTOP_MAX_BLOCK;
        }
        ols_live_render(&app.live, left, right, n);
        for (int i = 0; i < n; i++) {
            interleaved[2 * (done + i)] = left[i];
            interleaved[2 * (done + i) + 1] = right[i];
        }
        done += n;
    }

    app.voices_sounding = count_voices();
    app.frames_rendered += (uint32_t)frames;
}

float desktop_knob_value(int knob)
{
    const ols_live_t *L = &app.live;
    const ols_track_t *tr = &L->track[app.track];
    switch (knob) {
    case 0: case 1: case 2: case 3:
        return tr->engine.api->get_param(tr->engine.ctx, tr->engine.api->macros[knob]);
    case 4:
        return L->morph;
    case 5:
        return L->swing;
    case 6:
        return (L->clock.bpm - 20.0f) / 280.0f;
    case 7:
        return L->track[app.track].gain / 1.5f;
    case 8: case 9: case 10: case 11:
        return L->macro[L->current][knob - 8];
    default:
        return L->pump;
    }
}

/* ------------------------------------------------------------ live set files */

static uint8_t set_buf[OLS_PROJECT_MAX];

int desktop_save_set(const char *path)
{
    /* the audio thread may change the set during the copy: retry until a quiet copy */
    uint32_t size = 0;
    for (int attempt = 0; attempt < 8 && size == 0; attempt++) {
        uint32_t before = app.live.changes;
        size = ols_project_save(&app.live, set_buf, sizeof set_buf);
        if (app.live.changes != before) {
            size = 0;
        }
    }
    FILE *f = size ? fopen(path, "wb") : 0;
    if (!f) {
        return 0;
    }
    int ok = fwrite(set_buf, 1, size, f) == size;
    return fclose(f) == 0 && ok;
}

int desktop_load_set(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return 0;
    }
    uint32_t size = (uint32_t)fread(set_buf, 1, sizeof set_buf, f);
    fclose(f);
    if (!ols_project_load(&app.live, set_buf, size)) {
        return 0;
    }
    for (int t = 0; t < TRACKS; t++) {
        engine_va_snap(&app.va[t]);
    }
    app.track = app.live.current;
    app.bpm = app.live.clock.bpm;
    return 1;
}
