/*
 * OpenVALive app, entry points (the platform contract: openvalive_app.h and fm1-base's plat.h).
 *
 * The app is built as ONE unit ("unity build"): the platform (fm1-base's device/unity.c, its simulator, or a
 * target's own, like the SMK-37's) includes the target's app.c, which includes this file, after the platform.
 * This file in turn includes the app's parts below, in order.
 *
 *   src/app_state.c     shared state (4 engines, Live Core, event queue, settings, UI)
 *   src/app_audio.c     audio interrupt: events, MIDI in, Live Core rendering, output
 *   src/app_events.c    UI -> audio events, USB-MIDI out
 *   src/app_settings.c  load / save in flash (settings + the live set)
 *   src/app_version.c   the version as text (V_0.0.03)
 *   src/ui_text.c       text helpers, on-screen messages
 *   src/ui_input.c      keys, buttons, encoders
 *   src/control_map.c   MAP CONTROL (pages, knobs of MACRO LIVE EQ USER, faders, wheels), OPTION CONTROL
 *   src/sound_edit.c    SOUND EDIT: every parameter of the selected track's sound (the modulation matrix too)
 *   src/ui_draw.c       screen
 *   src/splash.c        the boot screen (logo and version), for a target that defines APP_SPLASH
 *
 * The DSP and the Live Core (core/) are compiled separately at -O2: see ols_dsp.c.
 */
#include "ols_config.h"
#include "fastmath.h"
#include "../core/engines/va/engine_va.h"
#include "../core/engines/drum/engine_drum.h"
#include "../core/engines/bank.h"
#include "../core/live/midi_in.h"
#include "../core/live/event_queue.h"
#include "../core/live/live.h"
#include "../core/live/project.h"
#include "../core/dsp/eq3.h"

#include "src/app_state.c"
#ifdef APP_CLIPS
#include "src/clips.c"
#endif
#include "src/app_audio.c"
#include "src/app_events.c"
#include "src/app_settings.c"
#include "src/app_version.c"
#include "src/ui_text.c"
#include "src/ui_input.c"
#include "src/control_map.c"
#include "src/sound_edit.c"
#include "src/loop_edit.c"
#include "src/sound_fav.c"
#include "src/ui_draw.c"
#include "src/splash.c"

/* Boot: four engines, the Live Core, MIDI mapping, the saved live set. Audio is not running yet. */
void app_init(void)
{
    ols_engine_t engines[TRACKS];

    evq_init(&evq);
    for (int t = 0; t < TRACKS; t++) {
        engine_va_init(&va[t], delay_buf[t], DELAY_LEN);
        engine_va_set_voice_limit(&va[t], VOICE_LIMIT[t]);
        engine_drum_init(&drum[t]);
        slots[t].va = &va[t];
        slots[t].drum = &drum[t];
        engines[t] = ols_slot_engine(&slots[t], OLS_KIND_VA);
    }
    ols_live_init(&live, engines, TRACKS, DSP_SR_F);
    ols_bank_attach(&live, slots);
    live.duck_kind = OLS_KIND_DRUM;               /* PUMP: the drum track ducks the others */
    for (int t = 0; t < TRACKS; t++) {
        ols_bank_select(&live, slots, t, DEFAULT_SOUND[t]);   /* the band: keys, bass, lead, drums */
        engine_va_set_voice_limit(&va[t], VOICE_LIMIT[t]);
    }
    ols_live_set_midi_out(&live, usb_midi_out, 0);
    engine_va_init(&lv, 0, 0);                    /* A's voice: no delay */
    arp.rng = 0x1234567u;

    /* MIDI in plays the selected track (recorded and captured like the keys); clock follows */
    static ols_engine_t live_input;
    live_input = ols_live_input(&live);
    midi_in_init(&midi, &live_input, MIDI_OMNI);
    midi_in_on_realtime(&midi, ols_live_midi_realtime, &live);
    midi_in_on_program(&midi, ols_live_midi_program, &live);  /* program change n = scene n + 1 */
    svf_set(&speaker_cut[0], 130.0f, 0.0f);
    svf_set(&speaker_cut[1], 130.0f, 0.0f);
    midi_in_map_cc(&midi, 74, VA_P_CUTOFF);       /* standard "brightness" */
    midi_in_map_cc(&midi, 71, VA_P_RESO);         /* standard "timbre/harmonic" */
    midi_in_map_cc(&midi, 73, VA_P_A_ATTACK);
    midi_in_map_cc(&midi, 72, VA_P_A_RELEASE);
    midi_in_map_cc(&midi, 5, VA_P_GLIDE);
    midi_in_map_cc(&midi, 1, VA_P_LFO1_PITCH);    /* mod wheel = vibrato */
    midi_in_map_cc(&midi, 93, VA_P_CHORUS_MIX);
    midi_in_map_cc(&midi, 94, VA_P_DELAY_MIX);

    ui_started = 0;
    settings_load();
    apply_midi_channel();
    live_voice_sound(set.live_sound);             /* audio not running yet: straight in */
#ifdef APP_CLIPS
    engine_drum_init(&pad_drum);                  /* the pads' drum kit */
    engine_drum_load_kit(&pad_drum, (dr_kit_t)set.pad_kit);
#endif
    ui.pend_sound = -1;
    arp_mode = set.arp_mode;
    ui.lv_level = 1.0f;
}

/* The UI's copies of the Live Core's settings, from the Live Core itself. */
static void sync_mirror(void)
{
    mirror.track = live.current;
    mirror.bpm = live.clock.bpm;
    mirror.quantize = live.quantize;
    mirror.clock_in = live.clock.external;
    mirror.clock_out = live.send_clock;
    ui.metronome = live.metronome;
    for (int m = 0; m < KNOB_COUNT; m++) {
        ui.macro[m] = live.macro[m];
    }
    ui.pump = live.pump;
    ui.xfade = 0;
    for (uint8_t i = 0; i < 4u; i++) {
        if (XFADE_BARS[i] == live.xfade_bars) {
            ui.xfade = i;
        }
    }
    resync_shadow();
}

/* The first screen, after the splash. */
void app_ui_init(void)
{
    touch_on(1);                                  /* key velocity sensing on */
    if (!ui_started) {
        ui.splash_until = plat_ms() + 3000u;      /* the boot screen stays 3 s more, unless a hand moves */
        ui.help = !set.no_help;                   /* then the help pages, until a button (not SEL) or a knob */
    } else {
        lcd_fill(0, 0, 240, 240, C_BLACK);
    }
    /* the saved set: applied now, or skipped if HOME is held (safe start) */
    if (!ui_started) {
        ui_started = 1;
        if (project_pending == 2u && (plat_buttons() & (1u << B_HOME))) {
            project_pending = 0;
            app_say("SAFE START", "SET NOT LOADED");
        } else if (project_pending == 2u) {
            project_pending = 1;                  /* the audio side loads it at its next block */
            ui.sync_after_load = 1;               /* app_frame catches the UI up once it is in */
        }
    }
    sync_mirror();
    ui.redraw = 1;
}

/* Main loop, about 60 times a second. */
void app_frame(void)
{
    uint32_t now = plat_ms();

    on_speaker = plat_headphones() == 0;          /* 1: nothing in the jack, the speaker plays */
    if (ui.sync_after_load && project_pending == 0u) {
        ui.sync_after_load = 0;
        sync_mirror();
        ui.redraw = 1;
    }
    ui.activity = 0;
    input();
    keep_tick();                                  /* (a setting kept at once whose write was refused: again) */
    if (settings_save_tick()) {                   /* SAVE ended (it goes in steps) */
        app_say(saving.ok ? "SAVED" : "SAVE", saving.ok ? "LIVE SET" : "FAILED");
        if (saving.ok) {
            ui.msg_style = MSG_TAKEN;
        }
    }
    if (ui.splash_until) {                        /* the boot screen: until its time, or the first touch */
        ui.touched |= ui.activity;
        if (now < ui.splash_until && (!ui.touched || ui.splash_until - now > 2500u)) {   /* (the first 0.5 s: noise;
                                                 * a touch then still ends it right after) */
            return;
        }
        if (ui.touched) {
            ui.help = 0;                          /* the boot screen cut short: no help either (it is for a quiet start) */
        }
        ui.splash_until = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.redraw = 1;
        ui.help_since = now;
        ui.help_shown = 0;
    }
    if (ui.help && now - ui.help_since >= 6u * 3000u) {
        ui.help = 0;                              /* three times round (six pages): they go by themselves */
    }
    if (ui.help) {                                /* the help pages: one every 3 s, until a button or a knob */
        help_draw(now);
        return;
    }
    if (ui.help_shown) {                          /* (just closed) */
        ui.help_shown = 0;
        lcd_fill(0, 0, 240, 240, C_BLACK);
        ui.redraw = 1;
    }

    if (ann_on() && ann_where()) {                /* a screen reader asks where we are */
        ann_say(APP_NAME, 0, ANN_NOW);
    }
    ann_tick(now);

    if (ui.msg_until && now >= ui.msg_until) {    /* the message has expired */
        ui.msg_until = 0;
        ui.redraw = 1;
    }

    if (map_is_open()) {                          /* MAP / OPTION CONTROL have the screen */
        map_draw();
        return;
    }
    if (sedit_is_open()) {                        /* so does SOUND EDIT */
        sedit_draw();
        return;
    }
    if (ledit_is_open()) {                        /* and LOOP EDIT */
        ledit_draw();
        return;
    }
    if (fav_is_open()) {                          /* and SOUNDS */
        fav_draw();
        return;
    }
    /* redraw on change, and at least 20 times a second for the oscilloscope */
    if (ui.redraw || now - ui.last_draw >= 50u) {
        ui.redraw = 0;
        ui.last_draw = now;
        draw();
    }
}

/* Between frames: input only. */
void app_input_only(void)
{
    input();
    ui.touched |= ui.activity;                    /* (a key played between frames ends the boot screen too) */
}

#ifdef PLAT_HOST
/* Values the simulator scripts can check with "expect NAME VALUE". */
int app_expect(const char *what, int *got)
{
    const ols_track_t *tr = &live.track[live.current];
    if (!strcmp(what, "voices")) {
        *got = voices_sounding();
    } else if (!strcmp(what, "voice_limit")) {
        *got = 0;
        for (int t = 0; t < TRACKS; t++) {
            *got += va[t].voice_limit;              /* the voice budget (no longer on screen) */
        }
    } else if (!strcmp(what, "slicer")) {
        *got = slicer_on ? 2 : (live.slice_mask != 0);   /* PERFORM SLICER: 1 on a track, 2 on the whole mix (A) */
    } else if (!strcmp(what, "help")) {
        *got = ui.help;                           /* the help pages up (after the boot screen) */
    } else if (!strcmp(what, "no_help")) {
        *got = set.no_help;                       /* HELP AT START off */
    } else if (!strcmp(what, "help_page")) {
        *got = ui.help_shown;                     /* the help page drawn, 1 or 2 (0: none) */
    } else if (!strcmp(what, "cap")) {
        *got = ui.cap_on;                         /* CAPTURE's green line shown */
    } else if (!strcmp(what, "preset")) {
        *got = set.preset[mirror.track];
    } else if (!strcmp(what, "octave")) {
        *got = set.octave[ui.on_a ? TRACKS : mirror.track];
    } else if (!strcmp(what, "dirty")) {
        *got = set_dirty || live.changes != saved_changes;
    } else if (!strcmp(what, "dropped")) {
        *got = (int)evq.dropped;
    } else if (!strcmp(what, "track")) {
        *got = live.current;
    } else if (!strcmp(what, "playing")) {
        *got = live.clock.running;
    } else if (!strcmp(what, "recording")) {
        *got = live.recording;
    } else if (!strcmp(what, "notes")) {
        *got = tr->pat.count;               /* notes in the selected track's loop */
    } else if (!strcmp(what, "bars")) {
        *got = (int)(tr->pat.length / OLS_TICKS_PER_BAR);
    } else if (!strcmp(what, "steps")) {
        *got = (int)(tr->pat.length / OLS_TICKS_PER_STEP);
    } else if (!strcmp(what, "muted")) {
        *got = (live.track[0].mute) | (live.track[1].mute << 1) | (live.track[2].mute << 2) | (live.track[3].mute << 3);
    } else if (!strcmp(what, "level")) {
        *got = (int)(ui.level[mirror.track] * 100.0f + 0.5f);   /* the selected track's level, % of unity */
    } else if (!strcmp(what, "a_level")) {
        *got = (int)(ui.lv_level * 100.0f + 0.5f);
    } else if (!strcmp(what, "avol")) {
        *got = avol.on;                         /* A VOL running */
    } else if (!strcmp(what, "scenes")) {
        int n = 0;
        for (int s = 0; s < OLS_SCENES; s++) {
            n += live.scene[s].valid;
        }
        *got = n;
    } else if (!strcmp(what, "undo")) {
        *got = ols_live_can_undo(&live);
    } else if (!strcmp(what, "undo_track")) {
        *got = ols_live_can_undo_track(&live, mirror.track);   /* the selected track's own history */
    } else if (!strcmp(what, "pending")) {
        *got = ui.pend_sound;                   /* a sound waiting for SEL, -1 none */
    } else if (!strcmp(what, "drum_map")) {
        *got = set.drum_map;                    /* DRUM_MAP_ORIGINAL 0, DRUM_MAP_GM 1 */
    } else if (!strcmp(what, "drum_key_d#")) {
        *got = drum_key_note(63);               /* D#4: the original map's chosen sound, GM's clap (39) */
    } else if (!strcmp(what, "loop")) {
        *got = running_loop();                  /* the perform key of the loop running (0 = 1 BAR .. 4 = 1/16, 16 = 1/32), -1 none */
    } else if (!strcmp(what, "swap")) {
        *got = ui.swap_undone ? -(int)ui.swap_track : (int)ui.swap_track;   /* last A swap: track + 1, < 0 undone */
    } else if (!strcmp(what, "on_a")) {
        *got = ui.on_a;
    } else if (!strcmp(what, "arp_mode")) {
        *got = arp_mode;
    } else if (!strcmp(what, "live_sound")) {
        *got = set.live_sound;
    } else if (!strcmp(what, "lv_voices")) {
        int n = 0;                              /* A's voice: notes sounding */
        for (int i = 0; i < VA_VOICES; i++) {
            n += lv.voice[i].amp.stage != ENV_IDLE;
        }
        *got = n;
    } else if (!strcmp(what, "bpm")) {
        *got = (int)(live.clock.bpm + 0.5f);
    } else if (!strcmp(what, "kind")) {
        *got = tr->kind;                    /* 0 synth, 1 drums */
    } else if (!strcmp(what, "controller")) {
        *got = set.controller;
    } else if (!strcmp(what, "held_out")) {
        *got = live.hold_mute;              /* tracks taken out by a break */
    } else if (!strcmp(what, "looping")) {
        *got = live.stutter;
    } else if (!strcmp(what, "page")) {
        *got = set.page;                        /* PAGE_*: SOUND 0 MACRO 1 LIVE 2 SETUP 3 USER 4 DRUM EQ 5 EQ 6 */
    } else if (!strcmp(what, "eq_bass")) {
        *got = set.eq[0];                       /* 0..255, 128 flat */
    } else if (!strcmp(what, "perc_g#4")) {
        *got = drum_key_note(68);               /* G#4: PERC 3 on the FM-1 */
    } else if (!strcmp(what, "perc_g#3")) {
        *got = drum_key_note(56);
    } else if (!strcmp(what, "map_menu")) {
        *got = mapui.open ? mapui.screen : 0;   /* 1 MAP CONTROL, 2 OPTION CONTROL */
    } else if (!strcmp(what, "fav_open")) {
        *got = fav.open;
    } else if (!strcmp(what, "energy")) {
        *got = (int)(live.macro[OLS_MACRO_ENERGY] * 100.0f + 0.5f);
    } else if (!strcmp(what, "pump")) {
        *got = (int)(live.pump * 100.0f + 0.5f);
    } else if (!strcmp(what, "chain")) {
        *got = live.chain_on;
    } else if (!strcmp(what, "edit_open")) {
        *got = sedit_is_open();
    } else if (!strcmp(what, "edit_row")) {
        *got = sedit.row;
    } else if (!strcmp(what, "map_open")) {
        *got = map_is_open();
    } else if (!strcmp(what, "map_k1")) {
        *got = cmap.target[MAP_K1];
    } else if (!strcmp(what, "overrides")) {
        int n = 0;
        for (int i = 0; i < OLS_OVERRIDES; i++) {
            n += live.ovr[i].active;
        }
        *got = n;
    } else if (!strcmp(what, "xfade")) {
        *got = live.xfade_bars;
    } else if (!strcmp(what, "metronome")) {
        *got = live.metronome;
    } else if (!strcmp(what, "user")) {
        int n = 0;
        for (int u = 0; u < OLS_USER_SOUNDS; u++) {
            n += user_sounds[u].valid != 0u;
        }
        *got = n;                           /* user sounds kept */
    } else if (!strcmp(what, "midi_ch")) {
        *got = set.midi_ch;
    } else if (!strcmp(what, "pressure")) {
        *got = (int)(va[live.current].d.pressure * 100.0f + 0.5f);
    } else if (what[0] == 'p' && what[1] >= '0' && what[1] <= '9') {
        /* pN: parameter N of the selected track's engine x 100 */
        *got = (int)(tr->engine.api->get_param(tr->engine.ctx, (uint16_t)atoi(what + 1)) * 100.0f + 0.5f);
    } else {
        return 0;
    }
    return 1;
}
#endif
