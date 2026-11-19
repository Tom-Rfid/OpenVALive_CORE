/*
 * OpenVALive - FM-1 app: keys, buttons and encoders (main loop).
 *
 *   keys             notes on the selected track (velocity), also out over USB-MIDI
 *   SELECT (turn)    track 1..4                ALGO (turn)    tempo
 *   PRESET (turn)    sound of the track (12 synths, 3 drum kits)    K1..K4   the knob page
 *   SELECT past 4    A: the live line (keys play A's own sound, live; gestures act on every track)
 *   FX               next knob page (on release): SOUND (the track's 4 macros) / MACRO (ENERGY SPACE TENSION DENSITY,
 *                    on every track) / LIVE (MORPH SWING PUMP LEVEL) / SETUP (MIDI IN, XFADE, QUANTIZE, BARS) / USER
 *   GLO + PLAY       scene chain on / off (the stored scenes in turn)   GLO + ALGO   bars per scene 4 / 8 / 16
 *   GLO + SEL        MAP CONTROL: the pages, what the knobs of MACRO LIVE EQ USER control
 *   ARP + SEL        SOUND EDIT: every parameter of the track's sound, the modulation matrix (src/sound_edit.c)
 *   ARP tapped (A)   A's arpeggio: OFF, UP, DOWN, UP/DOWN, RANDOM
 *   PLAY             play / stop               REC            record into the track (starts playing)
 *   EDIT tapped      CAPTURE: what you just played becomes the loop (on release)
 *   EDIT held + keys PUNCH (playing): records over the track, replacing what the playhead passes; EDIT let go stops
 *   ENV / LFO        UNDO / REDO of the selected track (lit when there is one; none on A)   SAVE   save (on release)
 *   SAVE + white key 1..8   keep the track's sound as USER 1..8 (then on PRESET, after the factory sounds)
 *   SEL + PLAY       tap tempo                 SEL + REC      metronome on / off (REC from stop then counts in)
 *   SEL pressed      takes a sound waiting (PRESET on a track whose loop plays: red in the header, +SEL)
 *   SEL + SELECT     points at a line (violet); SEL let go selects it (from A, SEL lit: A swaps with that track)
 *   HOME held while switching on: safe start (the saved set is not loaded)
 *   OCT- / OCT+      octave of the line; a loop running: twice as long / half as long   HOME   panic
 *
 * Hold a layer button and play the white keys (the 16 white keys, F3 to G5). On a track, ARP / SEQ / GLO + key only
 * names the gesture ("LET GO .."): the button let go first takes it (mint), the key first gives it up (orange).
 *   ARP  creative    MUTATE HUMANIZE FILL SIMPLIFY ROT< ROT> REVERSE TRANSP- TRANSP+ EXPAND
 *                    CONTRACT CHANCE RATCHET OCT- OCT+   (the harder you hit, the stronger)
 *   GLO  scenes      keys 1-8 recall scene 1-8 (playing: on the next bar, cut or cross-fade: SETUP page),
 *                    keys 9-16 store scene 1-8
 *   SEQ  perform     LOOP 1 BAR 1/2 1/4 1/8 1/16 1/32, FILTER SHUT, FILTER OPEN, DELAY THROW, DRIVE, CRUSH,
 *                    BREAK (drums out), SOLO, DRUMS ONLY, ALL OUT, ALL FX OFF. Taken: on, latched (again: off);
 *                    on A: hold the key = while held, tap = latched. SEQ tapped: the loop lets go (A: every loop).
 *                    Tracks taken out come back on the next bar, on the one.
 *   SEL  tracks      MUTE 1 2 3 4, (free), CLEAR (A: every track), QUANTIZE, CLOCK IN,
 *                    CLOCK OUT, EUCLID 3..8 hits, CONTROLLER; black keys: 1st MUTE ALL 4 (A plays on), 4th A VOL
 *                    (the levels brought together over the longest loop)
 *
 * Hold SEL and turn PRESET: MIDI IN channel (ALL, 1..16, CLOCK only: follow a drum machine
 * without playing its notes). Key pressure (aftertouch) opens the filter and deepens the vibrato.
 *
 * CONTROLLER mode ("local off"): the keys and knobs only send MIDI (channel =
 * track number, knobs = CC 74 71 75 94); the loops keep playing inside.
 */

#define FIRST_KEY_NOTE 53           /* key 0 is F3 */
#define ENCODER_STEP (1.0f / 64.0f) /* one detent = 1/64 of the range */
#define WHITE_KEYS 16

/* the white keys of the FM-1 (F3 .. G5), as key numbers */
static const uint8_t WHITE_KEY[WHITE_KEYS] = { 0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26 };
/* its black keys, the top row from the left (F#3 G#3 A#3 C#4 ...): a target maps its own on these */
#define BLACK_KEYS 11
static const uint8_t BLACK_KEY[BLACK_KEYS] = { 1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25 };
#define KEY_MUTE_ALL 1u                         /* SEL + the first black key (F#3): every track out / back */
#define KEY_AVOL 8u                             /* SEL + the fourth (C#4): A VOL, the levels brought together */

/* the Creative Engine on the white keys (ARP held): operation and argument */
static const struct {
    uint8_t op;
    int8_t arg;
    const char *name;
} CREATIVE_KEY[WHITE_KEYS] = {
    { CREATIVE_MUTATE, 0, "MUTATE" },       { CREATIVE_HUMANIZE, 0, "HUMANIZE" },
    { CREATIVE_FILL, 0, "FILL" },           { CREATIVE_SIMPLIFY, 0, "SIMPLIFY" },
    { CREATIVE_ROTATE, -1, "ROTATE <" },    { CREATIVE_ROTATE, 1, "ROTATE >" },
    { CREATIVE_REVERSE, 0, "REVERSE" },     { CREATIVE_TRANSPOSE, -1, "TRANSP -1" },
    { CREATIVE_TRANSPOSE, 1, "TRANSP +1" }, { CREATIVE_EXPAND, 0, "EXPAND" },
    { CREATIVE_CONTRACT, 0, "CONTRACT" },   { CREATIVE_CHANCE, 0, "CHANCE" },
    { CREATIVE_RATCHET, 0, "RATCHET" },     { CREATIVE_TRANSPOSE, -12, "OCTAVE -" },
    { CREATIVE_TRANSPOSE, 12, "OCTAVE +" }, { CREATIVE_COUNT, 0, "" },
};

/* The perform effects (FX held + white key). */
enum { FX_LOOP, FX_ROLE, FX_MUTE, FX_ALL_OFF };
enum { MUTE_DRUMS, MUTE_SOLO, MUTE_SYNTHS, MUTE_ALL };

static const struct {
    uint8_t type;
    uint16_t arg;           /* loop: ticks; role: OLS_ROLE_*; mute: MUTE_* */
    float value;            /* role: the value held */
    const char *name;
} PERFORM_KEY[FX_KEYS] = {
    { FX_LOOP, 384, 0.0f, "LOOP 1 BAR" },
    { FX_LOOP, 192, 0.0f, "LOOP 1/2" },
    { FX_LOOP, 96, 0.0f, "LOOP 1/4" },
    { FX_LOOP, 48, 0.0f, "LOOP 1/8" },
    { FX_LOOP, 24, 0.0f, "LOOP 1/16" },
    { FX_LOOP, 12, 0.0f, "LOOP 1/32" },
    { FX_ROLE, OLS_ROLE_CUTOFF, 0.04f, "FILTER SHUT" },
    { FX_ROLE, OLS_ROLE_CUTOFF, 1.0f, "FILTER OPEN" },
    { FX_ROLE, OLS_ROLE_DELAY_MIX, 0.9f, "DELAY THROW" },
    { FX_ROLE, OLS_ROLE_DRIVE, 1.0f, "DRIVE" },
    { FX_ROLE, OLS_ROLE_CRUSH, 0.7f, "CRUSH" },
    { FX_MUTE, MUTE_DRUMS, 0.0f, "BREAK" },
    { FX_MUTE, MUTE_SOLO, 0.0f, "SOLO" },
    { FX_MUTE, MUTE_SYNTHS, 0.0f, "DRUMS ONLY" },
    { FX_MUTE, MUTE_ALL, 0.0f, "ALL OUT" },
    { FX_ALL_OFF, 0, 0.0f, "ALL FX OFF" },
};

/* UI mirrors of Live Core settings (the UI sends changes, the audio side applies them) */
static struct {
    uint8_t track;
    float bpm;
    uint8_t quantize, clock_in, clock_out;
    int8_t scene_prev, scene_last;               /* the two last recalled scenes: Morph goes between them */
} mirror = { 0, 120.0f, 0, 0, 0, -1, -1 };

static void select_line(int32_t to);           /* (encoders, below) */

static int white_index(uint32_t k)
{
    for (int w = 0; w < WHITE_KEYS; w++) {
        if (WHITE_KEY[w] == k) {
            return w;
        }
    }
    return -1;
}

static int button_held(uint32_t buttons, uint32_t b)
{
    return (buttons >> b) & 1u;
}

/* The engines also change through MIDI, scenes and presets: the UI re-reads
 * them before an encoder turn that follows a pause. */
static const ols_engine_api_t *track_api(int t)
{
    return live.track[t].engine.api;
}

static float track_param(int t, uint16_t id)
{
    return track_api(t)->get_param(live.track[t].engine.ctx, id);
}

static void resync_shadow(void)
{
    for (int t = 0; t < TRACKS; t++) {
        uint16_t count = track_api(t)->param_count;
        for (uint16_t i = 0; i < count && i < OLS_MAX_PARAMS; i++) {
            ui.shadow[t][i] = track_param(t, i);
        }
        ui.level[t] = live.track[t].gain;
    }
    ui.morph = live.morph;
    ui.swing = live.swing;
}

static void mark_dirty(void)
{
    set_dirty = 1;
}

static void say_int(const char *label, int32_t value, const char *unit)
{
    char t[16];
    int_text(value, t);
    text_cat(t, unit);
    app_say(label, t);
}

/* ------------------------------------------------------------ layers on the white keys */

static void creative_gesture(int w, uint32_t velocity)
{
    if (CREATIVE_KEY[w].op >= CREATIVE_COUNT) {
        return;
    }
    float amount = (float)velocity * (1.0f / 127.0f);
    for (uint8_t t = 0; t < TRACKS; t++) {      /* the selected track, or every track with A selected */
        if (ui.on_a || t == mirror.track) {
            send_live(LIVE_CREATIVE, CREATIVE_KEY[w].op, (uint16_t)(int16_t)CREATIVE_KEY[w].arg, amount, t);
        }
    }
    say_int(CREATIVE_KEY[w].name, (int32_t)(amount * 100.0f + 0.5f), "%");
}

/* On a track the keys of ARP and SEQ only show what they do: the key held, ARP / SEQ let go first applies it;
 * the key let go first gives it up. On A they act at once (A is played live). */
/* the name of an armed key's gesture */
static const char *armed_name(uint8_t role, int w)
{
    static char t[20];
    if (role == KEY_ARMED_ARP) {                /* its name and the % it would take: "RANDOM 64%" */
        copy_text(t, CREATIVE_KEY[w].name, sizeof t);
        text_cat(t, " ");
        char p[6];
        int_text((int32_t)((uint32_t)ui.arp_vel * 100u / 127u), p);
        text_cat(t, p);
        text_cat(t, "%");
        return t;
    }
    if (role == KEY_ARMED_SEQ) {
        return PERFORM_KEY[w].name;
    }
    if (role == KEY_ARMED_HOME) {
        return w == 0 ? "PANIC" : (w == 1 ? "RESTART" : (w == 2 ? "STOP ALL" : (w == 13 ? "SOUNDS" : (w == 14 ? "LOOP EDIT" : "SETTINGS"))));
    }
    if (role == KEY_ARMED_FX && w == 13) {
        return set.wheel_pan ? "WHEEL BEND" : "WHEEL PAN";   /* what letting go switches the wheel to */
    }
    if (role == KEY_ARMED_FX) {
        return w == 14 ? "DRUMS ORIGINAL" : (w == 15 ? "DRUMS GM" : (ui.on_a ? "A RESET" : "TRACK RESET"));
    }
    copy_text(t, w >= 8 ? "STORE " : "SCENE ", sizeof t);
    int_text(w % 8 + 1, t + 6);
    return t;
}

static void arm_key(uint32_t k, int w, uint8_t role)
{
    ui.key_role[k] = role;
    ui.key_fx[k] = (uint8_t)w;
    ui.key_note[k] = (uint8_t)touch_vel(k);     /* (the strength of a creative gesture) */
    ui.arp_vel = ui.key_note[k];
    if (role == KEY_ARMED_ARP) {
        ui.arp_pend = 0;                        /* a new tap replaces the one chosen before */
    }
    static const char *const LET_GO[] = { "ARP", "SEQ", "GLO", "HOME", "FX" };   /* drawn after an arrow: let go */
    app_say(armed_name(role, w), LET_GO[role - KEY_ARMED_ARP]);
    ui.msg_style = MSG_ARMED;
}

static void creative_key(uint32_t k, int w, int armable)
{
    if (CREATIVE_KEY[w].op >= CREATIVE_COUNT) {
        return;
    }
    if (!ui.on_a && armable) {
        arm_key(k, w, KEY_ARMED_ARP);
        return;
    }
    creative_gesture(w, touch_vel(k));
}

static void scene_key(int w)
{
    char t[12] = "SCENE ";
    int slot = w % 8;
    int_text(slot + 1, t + 6);

    if (w >= 8) {
        send_live(LIVE_SCENE_STORE, (uint8_t)slot, 0, 0.0f, 0);
        app_say("STORED", t);
        return;
    }
    if (!live.scene[slot].valid) {
        app_say(t, "EMPTY");
        return;
    }
    send_live(LIVE_SCENE_RECALL, (uint8_t)slot, 0, 0.0f, 0);
    if (mirror.scene_last != slot) {
        mirror.scene_prev = mirror.scene_last;
        mirror.scene_last = (int8_t)slot;
    }
    ui.morph = 1.0f;                            /* Morph knob: fully on the scene just recalled */
    app_say(live.clock.running ? "NEXT BAR" : "RECALL", t);
}

/* Which tracks a break-type effect takes out. */
static uint8_t mute_mask_for(uint16_t which)
{
    uint8_t mask = 0;
    for (int t = 0; t < TRACKS; t++) {
        int drums = live.track[t].kind == OLS_KIND_DRUM;
        int out = which == MUTE_ALL
               || (which == MUTE_DRUMS && drums)
               || (which == MUTE_SYNTHS && !drums)
               || (which == MUTE_SOLO && t != mirror.track);
        if (out) {
            mask = (uint8_t)(mask | (1u << t));
        }
    }
    return mask;
}

/* the track the gestures act on: the selected one, or every track with A selected */
static uint8_t gesture_target(void)
{
    return ui.on_a ? OLS_TRACK_ALL : mirror.track;
}

static void fx_engage(int w)
{
    uint8_t target = gesture_target();
    ui.fx_target[w] = target;
    switch (PERFORM_KEY[w].type) {
    case FX_LOOP:
        /* one loop at a time: a new length replaces the running one */
        for (int i = 0; i < FX_KEYS; i++) {
            if (PERFORM_KEY[i].type == FX_LOOP) {
                ui.fx_on[i] = 0;
                ui.fx_latched[i] = 0;
            }
        }
        send_live(LIVE_STUTTER, 1, PERFORM_KEY[w].arg, 0.0f, target);
        break;
    case FX_ROLE:
        send_live(LIVE_OVERRIDE, 1, PERFORM_KEY[w].arg, PERFORM_KEY[w].value, target);
        break;
    case FX_MUTE:
        send_live(LIVE_HOLD_MUTE, 1, mute_mask_for(PERFORM_KEY[w].arg), 0.0f, 0);
        break;
    default:
        break;
    }
    ui.fx_on[w] = 1;
}

static void fx_release(int w)
{
    if (!ui.fx_on[w]) {
        return;
    }
    ui.fx_on[w] = 0;
    ui.fx_latched[w] = 0;
    switch (PERFORM_KEY[w].type) {
    case FX_LOOP:
        send_live(LIVE_STUTTER, 0, 0, 0.0f, OLS_TRACK_ALL);     /* it ends with its slice: in time */
        break;
    case FX_ROLE:
        send_live(LIVE_OVERRIDE, 0, PERFORM_KEY[w].arg, 0.0f, ui.fx_target[w]);
        break;
    case FX_MUTE: {
        /* the tracks come back together, on the next bar: every break key lets go */
        for (int i = 0; i < FX_KEYS; i++) {
            if (PERFORM_KEY[i].type == FX_MUTE) {
                ui.fx_on[i] = 0;
                ui.fx_latched[i] = 0;
            }
        }
        send_live(LIVE_HOLD_MUTE, 0, 0, 0.0f, 0);
        break;
    }
    default:
        break;
    }
}

static void fx_all_off(void)
{
    for (int w = 0; w < FX_KEYS; w++) {
        fx_release(w);
    }
    send_live(LIVE_STUTTER, 0, 1, 0.0f, OLS_TRACK_ALL);         /* id 1: at once */
}

static void perform_key(uint32_t k, int w, int armable)
{
    if (!ui.on_a && armable) {
        arm_key(k, w, KEY_ARMED_SEQ);
        return;
    }
    if (PERFORM_KEY[w].type == FX_ALL_OFF) {
        fx_all_off();
        app_say("ALL FX", "OFF");
        return;
    }
    if (ui.fx_latched[w]) {
        fx_release(w);                          /* tapped again: stop it */
        app_say(PERFORM_KEY[w].name, "OFF");
        return;
    }
    fx_engage(w);
    ui.key_role[k] = KEY_FX;
    ui.key_fx[k] = (uint8_t)w;
    ui.key_down_at[k] = plat_ms();
    app_say(PERFORM_KEY[w].name, "");
}

/* SEQ let go over an armed key: the effect on, latched (the same gesture again: off) */
static void perform_confirm(int w)
{
    if (PERFORM_KEY[w].type == FX_ALL_OFF) {
        fx_all_off();
        app_say("ALL FX", "OFF");
    } else if (ui.fx_latched[w]) {
        fx_release(w);
        app_say(PERFORM_KEY[w].name, "OFF");
    } else {
        fx_engage(w);
        ui.fx_latched[w] = 1;
        ui.fx_flash_until = plat_ms() + 500u;   /* the bottom line: white on green, 0.5 s */
        app_say(PERFORM_KEY[w].name, "ON");
    }
}

static int8_t *line_octave(void);

/* HOME + white key 1 / 2 / 3: PANIC (every note, effect and loop off; the music goes on), RESTART (every track
 * from its start, now; with a MIDI clock in, the clock keeps them), STOP ALL */
static void home_key(int w)
{
    if (w == 13) {
        fav_open();                             /* SOUNDS: which sounds PRESET offers */
        return;
    }
    if (w == 14) {
        ledit_open();                           /* LOOP EDIT: the selected track's notes */
        return;
    }
    if (w == 15) {
        map_open_settings();                    /* OPTION CONTROL */
        return;
    }
    if (w == 1) {
        send_live(LIVE_RESTART, 0, 0, 0.0f, 0);
        app_say("RESTART", live.clock.external ? "ON THE CLOCK IN" : "ALL TRACKS");
        return;
    }
    fx_all_off();
    send_all_off();
    if (w == 2) {
        send_live(LIVE_STOP, 0, 0, 0.0f, 0);
        app_say("STOP", "ALL");
    } else {
        app_say("PANIC", "ALL OFF");
    }
}

/* FX + white key 1: the selected line plays as it was made: its effects off, unmuted, octave 0, level at unity */
static void line_reset(void)
{
    uint8_t t = mirror.track;
    for (int w = 0; w < FX_KEYS; w++) {
        if (ui.fx_on[w] && (ui.on_a || ui.fx_target[w] == t)) {
            fx_release(w);
        }
    }
    *line_octave() = 0;
    mark_dirty();
    if (ui.on_a) {
        ui.lv_level = 1.0f;
        lv_gain = 1.0f;
        return;
    }
    send_live(LIVE_STUTTER, 0, 1, 0.0f, t);     /* its loop off at once */
    if (live.track[t].mute) {
        send_live(LIVE_MUTE_TOGGLE, t, 0, 0.0f, 0);
    }
    ui.level[t] = 1.0f;
    send_live(LIVE_GAIN, 0, 0, 1.0f, t);
}

/* ARP or SEQ let go: the armed keys still held apply their gesture */
static void layer_released(uint8_t role)
{
    for (uint32_t k = 0; k < NKEYS; k++) {
        if (ui.key_role[k] != role) {
            continue;
        }
        ui.key_role[k] = KEY_NONE;              /* (the key, still held, then does nothing more) */
        if (role == KEY_ARMED_ARP) {
            creative_gesture(ui.key_fx[k], ui.key_note[k]);
        } else if (role == KEY_ARMED_SEQ) {
            perform_confirm(ui.key_fx[k]);
        } else if (role == KEY_ARMED_GLO) {
            scene_key(ui.key_fx[k]);
        } else if (role == KEY_ARMED_HOME) {
            home_key(ui.key_fx[k]);
        } else if (ui.key_fx[k] == 0) {
            line_reset();
        } else if (ui.key_fx[k] == 13) {
            set.wheel_pan = (uint8_t)!set.wheel_pan;   /* the pitch wheel: bend, or pan the selected track */
            settings_keep();
        } else {
            set.drum_map = ui.key_fx[k] == 15 ? DRUM_MAP_GM : DRUM_MAP_ORIGINAL;
            settings_keep();                    /* kept at once, past a power cycle (no SAVE needed) */
        }
        ui.msg_style = MSG_TAKEN;               /* taken: the message turns mint, shown 0.5 s (unless another comes) */
        ui.msg_until = plat_ms() + 500u;
        ui.given_up = 0;
        ui.arp_pend = 0;
    }
    if (role == KEY_ARMED_ARP && ui.arp_pend) { /* ARP: the last key tapped, already let go, is taken */
        ui.arp_pend = 0;
        creative_gesture(ui.arp_pend_w, ui.arp_pend_vel);
        ui.msg_style = MSG_TAKEN;
        ui.msg_until = plat_ms() + 500u;
        ui.given_up = 0;
    }
    if (ui.given_up == role) {                  /* a key let go first, nothing taken: said now, its name red for 1 s */
        app_say(armed_name(role, ui.given_up_w), "");
        ui.msg_style = MSG_NOT_TAKEN;
        ui.msg_until = plat_ms() + 1000u;
    }
    ui.given_up = 0;
}

/* the running loop (its perform key), or -1 */
static int running_loop(void)
{
    for (int w = 0; w < FX_KEYS; w++) {
        if (ui.fx_on[w] && PERFORM_KEY[w].type == FX_LOOP) {
            return w;
        }
    }
    return -1;
}

/* SEQ tapped (no key, no OCT) while a loop runs: on a track, its loop lets go (none there: every loop); on A, every
 * loop */
static void seq_released(void)
{
    if (ui.seq_used) {
        return;
    }
    int any = 0;
    for (int w = 0; w < FX_KEYS; w++) {
        if (ui.fx_on[w] && PERFORM_KEY[w].type == FX_LOOP
            && (ui.on_a || ui.fx_target[w] == mirror.track || ui.fx_target[w] == OLS_TRACK_ALL)) {
            fx_release(w);
            any = 1;
        }
    }
    if (!any) {                                 /* none on this line (a loop left running on another track, then
                                                 * another line selected): SEQ tapped still lets every loop go */
        for (int w = 0; w < FX_KEYS; w++) {
            if (ui.fx_on[w] && PERFORM_KEY[w].type == FX_LOOP) {
                fx_release(w);
                any = 2;
            }
        }
    }
    if (any) {
        app_say("LOOP", ui.on_a || any == 2 ? "ALL LET GO" : "LET GO");
    }
}

/* SEQ held while a loop runs: OCT- doubles it (towards 1 BAR), OCT+ halves it (towards 1/32) */
static void loop_resize(int dir)
{
    int w = running_loop();
    int to = w + dir;
    if (w < 0 || to < 0 || to >= FX_KEYS || PERFORM_KEY[to].type != FX_LOOP) {
        return;
    }
    uint8_t latched = ui.fx_latched[w];
    for (uint32_t k = 0; k < NKEYS; k++) {
        if (ui.key_role[k] == KEY_FX && ui.key_fx[k] == w) {
            ui.key_fx[k] = (uint8_t)to;         /* a key holding it now holds the new length */
        }
    }
    fx_engage(to);
    ui.fx_latched[to] = latched;
    app_say(PERFORM_KEY[to].name, "");
}

/* A perform key let go: a short tap latches the effect, a hold ends it. */
static void perform_key_released(uint32_t k)
{
    int w = ui.key_fx[k];
    if (plat_ms() - ui.key_down_at[k] < FX_TAP_MS) {
        ui.fx_latched[w] = 1;
        app_say(PERFORM_KEY[w].name, "LATCHED");
    } else {
        fx_release(w);
    }
}

/* A VOL (average volume): the playing tracks' levels (and A's) go to one level, step by step over the longest loop
 * playing (4 bars: 4 bars), then it stops. The level: with 4 tracks the mean of the 2 middle ones, with 2 or 3 the
 * mean of the 2 loudest (a quiet one comes up, a loud one down; the outliers do not pull it). Again: stopped. */
static struct {
    uint8_t on, n;
    uint8_t t[TRACKS];
    float from[TRACKS], a_from, target;
    uint32_t start, ms, sent;
} avol;

static void avol_key(void)
{
    if (avol.on) {
        avol.on = 0;
        app_say("A VOL", "STOPPED");
        return;
    }
    float lv[TRACKS];
    uint32_t longest = 0, n = 0;
    for (int t = 0; t < TRACKS; t++) {
        if (!live.track[t].mute && live.track[t].pat.count) {
            avol.t[n] = (uint8_t)t;
            avol.from[n] = ui.level[t];
            lv[n++] = ui.level[t];
            longest = live.track[t].pat.length > longest ? live.track[t].pat.length : longest;
        }
    }
    if (n == 0u) {
        app_say("A VOL", "NO LOOP");
        return;
    }
    for (uint32_t i = 1; i < n; i++) {          /* loudest first */
        for (uint32_t j = i; j > 0 && lv[j] > lv[j - 1]; j--) {
            float x = lv[j];
            lv[j] = lv[j - 1];
            lv[j - 1] = x;
        }
    }
    avol.target = n >= 4u ? 0.5f * (lv[1] + lv[2]) : (n >= 2u ? 0.5f * (lv[0] + lv[1]) : lv[0]);
    uint32_t bars = (longest + OLS_TICKS_PER_BAR - 1u) / OLS_TICKS_PER_BAR;
    bars = bars < 1u ? 1u : bars;
    float bpm = clock_bpm(&live.clock);
    bpm = bpm < 20.0f ? 120.0f : bpm;
    avol.n = (uint8_t)n;
    avol.a_from = ui.lv_level;
    avol.ms = (uint32_t)((float)bars * 240000.0f / bpm);   /* 4 beats a bar */
    avol.start = avol.sent = plat_ms();
    avol.on = 1;
    char b[12];
    int_text((int32_t)bars, b);
    text_cat(b, bars > 1u ? " BARS" : " BAR");
    app_say("A VOL", b);
}

/* each frame while A VOL runs: the levels a step nearer (sent every 40 ms), the LIVE page shows them */
static void avol_tick(void)
{
    if (!avol.on) {
        return;
    }
    uint32_t now = plat_ms(), gone = now - avol.start;
    int done = gone >= avol.ms;
    if (!done && now - avol.sent < 40u) {
        return;
    }
    avol.sent = now;
    float p = done ? 1.0f : (float)gone / (float)avol.ms;
    for (uint32_t i = 0; i < avol.n; i++) {
        uint8_t t = avol.t[i];
        ui.level[t] = avol.from[i] + (avol.target - avol.from[i]) * p;
        send_live(LIVE_GAIN, 0, 0, ui.level[t], t);
    }
    ui.lv_level = avol.a_from + (avol.target - avol.a_from) * p;
    lv_gain = ui.lv_level;
    ui.redraw = 1;
    if (done) {
        avol.on = 0;
        mark_dirty();
        app_say("A VOL", "DONE");
    }
}

/* the four tracks muted, or back (A plays on); A is let go, so a key pressed next does not sound by mistake */
static void mute_all_key(void)
{
    int any_playing = 0;
    for (int t = 0; t < TRACKS; t++) {
        any_playing |= !live.track[t].mute;
    }
    for (int t = 0; t < TRACKS; t++) {
        if ((live.track[t].mute != 0) != any_playing) {
            send_live(LIVE_MUTE_TOGGLE, (uint8_t)t, 0, 0.0f, 0);
        }
    }
    if (ui.on_a) {
        ui.on_a = 0;
        ui.redraw = 1;
    }
    app_say(any_playing ? "MUTE" : "UNMUTE", "ALL 4");
}

static void track_key(int w)
{
    if (w < 4) {
        send_live(LIVE_MUTE_TOGGLE, (uint8_t)w, 0, 0.0f, 0);
        char t[4] = "1";
        t[0] = (char)('1' + w);
        app_say(live.track[w].mute ? "UNMUTE" : "MUTE", t);   /* state before the toggle */
    } else if (w == 4) {
        /* free: MUTE ALL is on the first black key now */
    } else if (w == 5) {
        if (ui.on_a) {
            for (uint8_t t = 0; t < TRACKS; t++) {
                send_live(LIVE_CLEAR, 0, 0, 0.0f, t);
            }
            app_say("CLEAR ALL", "UNDO: ENV");
        } else {
            send_live(LIVE_CLEAR, 0, 0, 0.0f, mirror.track);
            app_say("CLEAR", "UNDO: ENV");
        }
    } else if (w == 6) {
        mirror.quantize = (uint8_t)!mirror.quantize;
        send_live(LIVE_QUANTIZE, mirror.quantize, 0, 0.0f, 0);
        app_say("QUANTIZE", mirror.quantize ? "1/16" : "OFF");
    } else if (w == 7) {
        mirror.clock_in = (uint8_t)!mirror.clock_in;
        send_live(LIVE_EXTERNAL_CLOCK, mirror.clock_in, 0, 0.0f, 0);
        app_say("CLOCK IN", mirror.clock_in ? "MIDI" : "INTERNAL");
    } else if (w == 8) {
        mirror.clock_out = (uint8_t)!mirror.clock_out;
        send_live(LIVE_SEND_CLOCK, mirror.clock_out, 0, 0.0f, 0);
        app_say("CLOCK OUT", mirror.clock_out ? "ON" : "OFF");
    } else if (w < 15 && ui.on_a) {
        app_say("EUCLID", "SELECT A TRACK");
    } else if (w < 15) {
        uint8_t hits = (uint8_t)(w - 6);        /* 3 .. 8 hits over 16 steps */
        send_live(LIVE_EUCLID, hits, 16, 0.0f, mirror.track);
        say_int("EUCLID", hits, "/16");
    } else {
        set.controller = (uint8_t)!set.controller;
        mark_dirty();
        app_say("CONTROLLER", set.controller ? "MIDI ONLY" : "OFF");
    }
}

/* ------------------------------------------------------------ keys */

static int swap_a(int t);

/* the selected track's last gesture was a swap with A, nothing newer since (redo: 1, after an undo of it) */
static int swap_undoable(int redo)
{
    return !ui.on_a && ui.swap_track == mirror.track + 1u && ui.swap_undone == (uint8_t)redo
           && live.undo_top == ui.swap_mark;
}

/* the selected line's octave: track 1..4, or A */
static int8_t *line_octave(void)
{
    return &set.octave[ui.on_a ? TRACKS : mirror.track];
}

/* a D# or G# key of the keyboard (the line's octave taken off: the key itself): its PERC setting, 0 = D#3, 1 = G#3,
 * 2 = D#4 .. 5 = G#5 (the FM-1 has G#3 .. D#5, the SMK-37 all six); lower and higher ones take the nearest */
static uint32_t perc_key(uint32_t note)
{
    int32_t key = (int32_t)note - 12 * *line_octave();
    int32_t j = (key - 51) / 12 * 2 + (key % 12 == 8);
    j = key < 51 ? (key % 12 == 8) : j;
    return (uint32_t)clamp(j, (key % 12 == 8) ? 1 : 0, (key % 12 == 8) ? (int32_t)PERC_KEYS - 1 : (int32_t)PERC_KEYS - 2);
}

/* A key on a drum track: the General MIDI note of its instrument in the chosen map (every octave the same), so the
 * loop, MIDI OUT and a computer all see the right drum. Other tracks: the note itself. */
static uint8_t drum_key_note(uint32_t note)
{
    static const uint8_t ORIGINAL[12] = { 36, 37, 38, 0, 40, 50, 42, 48, 0, 45, 46, 41 };   /* toms high to low */
    uint32_t n = note % 12u;
    if (set.drum_map == DRUM_MAP_GM) {
        return (uint8_t)(36u + n);
    }
    if (n == 3u || n == 8u) {
        return DRUM_PERC[set.perc[perc_key(note)] % DRUM_PERCS].note;
    }
    return ORIGINAL[n];
}

/* the note a key of the selected line plays (a target whose keys skip key_pressed uses it too: the SMK-37) */
static uint8_t app_play_note(uint32_t note)
{
    return !ui.on_a && live.track[mirror.track].kind == OLS_KIND_DRUM ? drum_key_note(note) : (uint8_t)note;
}

/* a user sound brings back the octave it was kept with (a factory sound keeps the line's) */
static void sound_octave(int line, uint32_t sound)
{
    if (sound >= OLS_SOUND_FIRST_USER) {
        int8_t o = USER_OCTAVE(&user_sounds[(sound - OLS_SOUND_FIRST_USER) % OLS_USER_SOUNDS]);
        set.octave[line] = (int8_t)(o < -3 ? -3 : (o > 3 ? 3 : o));
    }
}

/* a sound is being chosen: one waits for SEL, or PRESET was turned in the last 2 s */
static int choosing_sound(void)
{
    return (ui.pend_sound >= 0 && !ui.on_a) || (ui.sound_at && plat_ms() - ui.sound_at < 2000u);
}

static int32_t family_step(int32_t sound, int32_t step, int for_a);
static const char *sound_label(int32_t sound);
static void preset_turn(int32_t step, int family);

static void key_pressed(uint32_t k, uint32_t buttons)
{
    int w = white_index(k);
    uint32_t held = buttons;
    ui.sound_at = 0;                            /* a key played: OCT is the octave again */                    /* the buttons really held (a pad's own layer is not: it acts at once) */
    ui.key_role[k] = KEY_NONE;
#ifdef APP_KEY_LAYER
    {                               /* a target's pads stand for keys with a layer of their own (SMK-37): that layer
                                     * alone, at once, whatever button is held */
        uint32_t own = APP_KEY_LAYER(k);
        if (own) {
            buttons = own;
            held = 0;
        }
    }
#endif

    if (button_held(buttons, B_SAVE)) {
        if (w >= 0 && w < OLS_USER_SOUNDS) {
            push_event(EV_PRESET, mirror.track, (uint8_t)w, 1, 0, (float)set.octave[mirror.track]);   /* b = 1: keep as user sound w, with the octave */
            set.preset[mirror.track] = (uint8_t)(OLS_SOUND_FIRST_USER + w);
            ui.save_used = 1;
            mark_dirty();
            char t[8] = "USER ";
            int_text(w + 1, t + 5);
            app_say("SOUND KEPT", t);
        }
        return;
    }

    if (button_held(buttons, B_ARP)) {
        ui.arp_used = 1;                        /* a gesture, not a tap: A's arpeggio stays */
        if (w >= 0) {
            creative_key(k, w, button_held(held, B_ARP));
        }
        return;
    }
    if (button_held(buttons, B_GLO)) {
        if (w >= 0) {
            if (button_held(held, B_GLO)) {
                arm_key(k, w, KEY_ARMED_GLO);   /* GLO let go first takes it */
            } else {
                scene_key(w);
            }
        }
        return;
    }
    if (button_held(buttons, BTN_PERFORM)) {
        ui.seq_used = 1;
        if (w >= 0) {
            perform_key(k, w, button_held(held, BTN_PERFORM));
        }
        return;
    }
    if (button_held(buttons, BTN_TRACKS)) {
        if (w >= 0) {
            track_key(w);
        } else if (k == KEY_MUTE_ALL) {
            mute_all_key();
        } else if (k == KEY_AVOL) {
            avol_key();
        }
        return;
    }
    if (button_held(held, B_HOME)) {            /* HOME + white key 1 / 2 / 3 / 16, HOME let go first: PANIC / RESTART /
                                                 * STOP / the settings */
        if ((w >= 0 && w < 3) || (w >= 13 && w <= 15)) {
            arm_key(k, w, KEY_ARMED_HOME);
        }
        return;
    }
    if (button_held(held, BTN_PAGES)) {         /* FX + white key 1 / 15 / 16, FX let go first: the line's reset,
                                                 * the drum keys original / GM */
        ui.pages_used = 1;
        if (w == 0 || w == 14 || w == 15 || (APP_WHEEL && w == 13)) {
            arm_key(k, w, KEY_ARMED_FX);
        }
        return;
    }

    int32_t note = app_play_note((uint32_t)clamp(FIRST_KEY_NOTE + (int32_t)k + 12 * *line_octave(), 0, 127));
    uint32_t velocity = touch_vel(k);
    if (velocity < 1u) {
        velocity = 1u;
    }
    uint8_t target = ui.on_a ? T_LIVE : mirror.track;       /* A: its own voice, live, not recorded */
    if (button_held(buttons, B_EDIT) && !ui.on_a && !ui.punch && live.clock.running && !live.recording) {
        send_live(LIVE_PUNCH, 1, 0, 0.0f, 0);   /* EDIT held + a note: record over, until EDIT is let go */
        ui.punch = 1;
        app_say("PUNCH", "LET GO EDIT: STOP");
        ui.msg_style = MSG_ARMED;
    }
    ui.key_role[k] = KEY_NOTE;
    ui.key_note[k] = (uint8_t)note;
    ui.key_track[k] = target;
    if (!set.controller) {
        send_note_on(target, (uint8_t)note, (uint8_t)velocity);
    }
    midi_out_note_on(target == T_LIVE ? TRACKS : target, (uint32_t)note, velocity);   /* A: MIDI channel 5 */
}

static void key_released(uint32_t k)
{
    switch (ui.key_role[k]) {
    case KEY_NOTE:
        /* the note it started, on the track it started on */
        send_note_off(ui.key_track[k], ui.key_note[k]);     /* harmless in controller mode */
        midi_out_note_off(ui.key_track[k] == T_LIVE ? TRACKS : ui.key_track[k], ui.key_note[k]);
        break;
    case KEY_FX:
        perform_key_released(k);
        break;
    case KEY_ARMED_ARP:                         /* ARP: a tap chooses; the % stays shown, ARP let go takes it */
        ui.arp_pend = 1;
        ui.arp_pend_w = ui.key_fx[k];
        ui.arp_pend_vel = ui.key_note[k];
        ui.key_role[k] = KEY_NONE;
        if (ui.msg_style == MSG_ARMED) {
            ui.msg_until = plat_ms() + 1200u;
        }
        return;
    case KEY_ARMED_SEQ:
    case KEY_ARMED_GLO:
    case KEY_ARMED_HOME:
    case KEY_ARMED_FX:
        ui.given_up = ui.key_role[k];           /* the key let go first: nothing done (said when the button is let go) */
        ui.given_up_w = ui.key_fx[k];
        break;
    default:
        break;
    }
    ui.key_role[k] = KEY_NONE;
}

static void scan_keys(uint32_t keys, uint32_t buttons)
{
    for (uint32_t k = 0; k < NKEYS; k++) {
        uint32_t bit = 1u << k;
        int down_now = (keys & bit) != 0;
        int down_before = (ui.keys & bit) != 0;

        if (down_now && !down_before) {
            key_pressed(k, buttons);
        } else if (!down_now && down_before) {
            key_released(k);
        }
    }
}

/* ------------------------------------------------------------ buttons */

/* OCT-/OCT+ move the selected line's octave only (on A: A's own) */
static void change_octave(int32_t delta)
{
    int8_t *o = line_octave();
    int32_t octave = clamp(*o + delta, -3, 3);
    if (octave == *o) {
        return;
    }
    char t[4];
    *o = (int8_t)octave;
    mark_dirty();
    octave_text(octave, t);
    app_say("OCTAVE", t);
}

static void button_pressed(uint32_t b, uint32_t buttons)
{
    if (button_held(buttons, B_GLO) && b == BTN_VALIDATE) {
        map_open();
        return;
    }
    if (button_held(buttons, B_ARP) && b == BTN_VALIDATE) {
        ui.arp_used = 1;
        sedit_open();
        return;
    }
    if (b == BTN_VALIDATE) {                    /* SEL: takes a waiting sound; held + SELECT: points at a line */
        ui.hover = ui.on_a ? TRACKS : mirror.track;
        ui.hovering = 0;
        if (ui.pend_sound >= 0) {
            set.preset[mirror.track] = (uint8_t)ui.pend_sound;
            sound_octave(mirror.track, (uint32_t)ui.pend_sound);
            ui.pend_sound = -1;
            mark_dirty();
            send_preset(mirror.track, set.preset[mirror.track]);
            ui.turned_at = 0;
            new_sound_page(mirror.track, set.preset[mirror.track]);
            app_say("SOUND", ols_sound_name(set.preset[mirror.track]));
            ui.msg_style = MSG_TAKEN;
            return;
        }
    }
    if (button_held(buttons, B_GLO) && b == B_PLAY) {
        uint8_t on = (uint8_t)!live.chain_on;
        send_live(LIVE_CHAIN, on, CHAIN_BARS[ui.chain_bars], 0.0f, 0);
        if (on) {
            say_int("SCENE CHAIN", CHAIN_BARS[ui.chain_bars], " BARS");
        } else {
            app_say("SCENE CHAIN", "OFF");
        }
        return;
    }
    if (button_held(buttons, BTN_TRACKS) && b == B_PLAY) {
        send_live(LIVE_TAP, 0, 0, 0.0f, 0);
        app_say("TAP", "TEMPO");
        return;
    }
    if (button_held(buttons, BTN_TRACKS) && b == B_REC) {
        ui.metronome = (uint8_t)!ui.metronome;
        send_live(LIVE_METRONOME, ui.metronome, 0, 0.0f, 0);
        app_say("METRONOME", ui.metronome ? "ON" : "OFF");
        return;
    }
    switch (b) {
    case B_OCTDN:
        if (choosing_sound()) {
            preset_turn(-1, 1);                 /* a sound being chosen: the previous family */
        } else if (running_loop() >= 0) {
            loop_resize(-1);                    /* a loop runs: OCT- makes it twice as long */
            ui.seq_used = 1;
        } else {
            change_octave(-1);
        }
        break;
    case B_OCTUP:
        if (choosing_sound()) {
            preset_turn(1, 1);                  /* ...the next one */
        } else if (running_loop() >= 0) {
            loop_resize(1);                     /* ...OCT+ half as long */
            ui.seq_used = 1;
        } else {
            change_octave(1);
        }
        break;
    case B_PLAY:
        send_live(LIVE_PLAY_TOGGLE, 0, 0, 0.0f, 0);
        if (mirror.clock_in) {
            app_say("CLOCK IN", "MIDI");        /* the MIDI clock starts and stops */
        }
        break;
    case B_REC:
        send_live(LIVE_RECORD_TOGGLE, 0, 0, 0.0f, 0);
        break;
    case B_EDIT:                                /* on release: CAPTURE, or the end of a punch */
        ui.punch = 0;
        break;
    case B_ENV:
        if (ui.on_a) {
            app_say("A LIVE", "NO UNDO");       /* A is played live: nothing to undo */
            break;
        }
        if (swap_undoable(0)) {                 /* the last thing on this track was a swap with A: swap back */
            swap_a(mirror.track);
            ui.swap_undone = 1;
            app_say("UNDO", "SWAP A");
            break;
        }
        send_live(LIVE_UNDO, 0, 0, 0.0f, gesture_target());
        app_say("UNDO", ols_live_can_undo_track(&live, gesture_target()) ? "" : "NOTHING");
        break;
    case B_LFO:
        if (ui.on_a) {
            app_say("A LIVE", "NO REDO");
            break;
        }
        if (swap_undoable(1)) {
            swap_a(mirror.track);
            ui.swap_undone = 0;
            app_say("REDO", "SWAP A");
            break;
        }
        send_live(LIVE_REDO, 0, 0, 0.0f, gesture_target());
        app_say("REDO", ols_live_can_redo_track(&live, gesture_target()) ? "" : "NOTHING");
        break;
    case BTN_PAGES:                             /* FX: the page turns on release */
        ui.pages_used = 0;
        break;
    case B_HOME:
        app_say("1 PANIC 2 SYNC", "3 STOP");     /* HOME + white key, HOME let go first */
        ui.msg_style = MSG_ARMED;
        break;
    case B_SAVE:
        ui.save_used = 0;                       /* the set is saved on release, unless a key kept a sound */
        break;
    case B_ARP:
        ui.arp_down_at = plat_ms();
        ui.arp_used = 0;
        ui.arp_pend = 0;
        app_say("CREATIVE", ui.on_a ? "HOLD + KEY, TAP: ARP" : "HOLD + KEY");
        break;
    case B_GLO:
        app_say("SCENES", "HOLD + KEY");
        break;
    case BTN_PERFORM:
        ui.seq_used = 0;
        app_say("PERFORM", running_loop() >= 0 ? "TAP: LOOP OFF" : "HOLD + KEY");
        break;
    case BTN_TRACKS:
        app_say("MUTE", "1 2 3 4 ALL");
        ui.msg_right = "A VOL";
        break;
    default:
        break;
    }
}

static void save_released(void)
{
    if (ui.save_used) {
        return;
    }
    if (settings_save()) {
        app_say("SAVED", "LIVE SET");
    } else {
        app_say("SAVE", "FAILED");
    }
}

/* ARP tapped with A selected (no key, short): A's arpeggio, OFF UP DOWN UP/DOWN RANDOM in turn */
static void arp_released(void)
{
    if (!ui.on_a || ui.arp_used || plat_ms() - ui.arp_down_at > 400u) {
        return;
    }
    set.arp_mode = (uint8_t)((set.arp_mode + 1u) % ARP_MODES);
    arp_mode = set.arp_mode;
    mark_dirty();
    app_say("ARP", ARP_NAME[set.arp_mode]);
}

/* A <-> track t: the sounds (every parameter, effects included) and the octaves change place; the loop stays */
static int swap_a(int t)
{
    if (live.track[t].kind != OLS_KIND_VA) {
        app_say("NO SWAP", "DRUMS");            /* A plays synth sounds only */
        return 0;
    }
    push_event(EV_PRESET, (uint8_t)t, 0, 2, 0, 0.0f);
    uint8_t s = set.preset[t];
    set.preset[t] = set.live_sound;
    set.live_sound = s;
    int8_t o = set.octave[t];
    set.octave[t] = set.octave[TRACKS];
    set.octave[TRACKS] = o;
    ui.pend_sound = -1;
    ui.turned_at = 0;
    mark_dirty();
    return 1;
}

/* EDIT let go: the end of a punch, else CAPTURE */
static void edit_released(void)
{
    if (ui.punch) {
        ui.punch = 0;
        send_live(LIVE_PUNCH, 0, 0, 0.0f, 0);
        app_say("PUNCH", "DONE");
        ui.msg_style = MSG_TAKEN;
        ui.msg_until = plat_ms() + 500u;
        return;
    }
    send_live(LIVE_CAPTURE, 0, 0, 0.0f, 0);
    app_say("CAPTURE", "LOOP");
}

/* the page after `p`: DRUM EQ only when the selected line is a drum track */
static int drum_line(void)
{
    return !ui.on_a && live.track[mirror.track].kind == OLS_KIND_DRUM;
}


/* FX let go: the next knob page (DRUM EQ only on a drum line) */
static void pages_released(void)
{
    if (ui.pages_used) {
        return;
    }
    set.page = (uint8_t)page_after(set.page);
    mark_dirty();
    ui.redraw = 1;                              /* no header message: the colours and the name bottom right show it */
    if (ann_on()) {
        ann_say(PAGE_NAME[set.page], 0, ANN_NOW);
    }
}

/* SEL let go: the line pointed at is selected (from A: A swaps with it) */
static void validate_released(void)
{
    if (ui.hovering) {
        ui.hovering = 0;
        if (ui.on_a && ui.hover < TRACKS) {
            if (swap_a(ui.hover)) {
                ui.swap_track = (uint8_t)(ui.hover + 1u);
                ui.swap_undone = 0;
                ui.swap_mark = live.undo_top;
                char t[4] = "T";
                int_text(ui.hover + 1, t + 1);
                app_say("SWAP A", t);
            }
            return;
        }
        select_line(ui.hover);
    }
}

static void scan_buttons(uint32_t buttons)
{
    for (uint32_t b = 0; b < NB; b++) {
        uint32_t bit = 1u << b;
        if ((buttons & bit) && !(ui.buttons & bit)) {
            button_pressed(b, buttons);
        } else if (!(buttons & bit) && (ui.buttons & bit) && b == B_SAVE) {
            save_released();
        } else if (!(buttons & bit) && (ui.buttons & bit) && b == B_ARP) {
            layer_released(KEY_ARMED_ARP);
            arp_released();
        } else if (!(buttons & bit) && (ui.buttons & bit) && b == BTN_PERFORM) {
            layer_released(KEY_ARMED_SEQ);
            seq_released();
        } else if (!(buttons & bit) && (ui.buttons & bit) && b == B_GLO) {
            layer_released(KEY_ARMED_GLO);
        } else if (!(buttons & bit) && (ui.buttons & bit) && b == B_EDIT) {
            edit_released();
        } else if (!(buttons & bit) && (ui.buttons & bit) && b == BTN_VALIDATE) {
            validate_released();
        } else if (!(buttons & bit) && (ui.buttons & bit) && b == BTN_PAGES) {
            layer_released(KEY_ARMED_FX);
            pages_released();
        } else if (!(buttons & bit) && (ui.buttons & bit) && b == B_HOME) {
            layer_released(KEY_ARMED_HOME);
        }
    }
}

/* ------------------------------------------------------------ encoders */

static int32_t detents(int role)
{
    int32_t d = plat_enc(role);
    if (d != 0) {
        ui.activity = 1;
    }
    return d;
}

/* the line selected: a track, or A (TRACKS) */
static void select_line(int32_t to)
{
    ui.pend_sound = -1;                         /* a sound waiting for SEL is given up */
    ui.turned_at = 0;
    ui.redraw = 1;
    if (to == TRACKS) {
        ui.on_a = 1;
        app_say("A LIVE", ols_sound_name(set.live_sound));
        return;
    }
    ui.on_a = 0;
    int32_t track = to;
    if (track == mirror.track) {
        return;
    }
    mirror.track = (uint8_t)track;
    send_live(LIVE_SELECT, (uint8_t)track, 0, 0.0f, 0);
    ui.turned_at = 0;
    char t[4] = "T";                            /* T1..T4, as in the header: the line does not jump */
    int_text(track + 1, t + 1);
    app_say(t, ols_sound_name(set.preset[track]));
}

static void scan_track_encoder(uint32_t buttons)
{
    int32_t turn = detents(EN_SELECT);
    if (turn == 0) {
        return;
    }
    ui.sound_at = 0;                            /* (another track: OCT is the octave again) */
    /* 1 2 3 4 A: A is the line past track 4. SEL held: point at a line (violet), SEL let go selects it */
    int32_t at = button_held(buttons, BTN_VALIDATE) ? ui.hover : (ui.on_a ? TRACKS : (int32_t)mirror.track);
    int32_t to = clamp(at + (turn > 0 ? 1 : -1), 0, TRACKS);
    if (to == at) {
        return;
    }
    if (button_held(buttons, BTN_VALIDATE)) {
        ui.hover = (uint8_t)to;
        ui.hovering = 1;
        ui.redraw = 1;
        return;
    }
    select_line(to);
}

static void scan_tempo_encoder(uint32_t buttons)
{
    int32_t turn = detents(EN_ALGO);
    if (turn == 0) {
        return;
    }
    if (button_held(buttons, B_GLO)) {
        /* bars per scene of the chain */
        ui.chain_bars = (uint8_t)clamp((int32_t)ui.chain_bars + (turn > 0 ? 1 : -1), 0, 2);
        send_live(LIVE_CHAIN, live.chain_on, CHAIN_BARS[ui.chain_bars], 0.0f, 0);
        say_int("CHAIN", CHAIN_BARS[ui.chain_bars], " BARS");
        return;
    }
    mirror.bpm = fm_clampf(live.clock.bpm + (float)turn, 20.0f, 300.0f);   /* from the real tempo (tap tempo) */
    send_live(LIVE_TEMPO, 0, 0, mirror.bpm, 0);
    mark_dirty();
    say_int("TEMPO", (int32_t)mirror.bpm, " BPM");
}

static void midi_channel_turn(int32_t turn)
{
    /* ALL, 1..16, CLOCK, around and around */
    int32_t count = (int32_t)MIDI_CH_CLOCK + 1;
    int32_t ch = ((int32_t)set.midi_ch + (turn > 0 ? 1 : -1) + count) % count;
    set.midi_ch = (uint8_t)ch;
    apply_midi_channel();
    mark_dirty();
    if (ch == 0) {
        app_say("MIDI IN", "ALL CH");
    } else if (ch == (int32_t)MIDI_CH_CLOCK) {
        app_say("MIDI IN", "CLOCK ONLY");
    } else {
        say_int("MIDI IN", ch, " CH");
    }
}

/* a sound's family, 1..FAMILIES (its place in PRESET's order) */
static int sound_family(int32_t sound)
{
    if (sound >= OLS_SOUND_FIRST_USER) {
        return FAMILIES;
    }
    int pos = 0;
    while (pos < OLS_SOUND_COUNT && SOUND_ORDER[pos] != sound) {
        pos++;
    }
    int f = 0;
    while (f + 1 < (int)sizeof FAMILY_AT && pos >= FAMILY_AT[f + 1]) {
        f++;
    }
    return f + 1;
}

/* ---- the sounds PRESET offers: in its order, the hidden ones (SOUNDS screen) and the empty USER slots left out */

static int sound_hidden(int32_t sound)
{
    return sound >= 0 && sound < OLS_SOUND_TOTAL && ((set.hidden[sound >> 3] >> (sound & 7)) & 1u);
}

static int32_t sound_at(int32_t pos)            /* PRESET's order: SOUND_ORDER, then USER 1..8 */
{
    return pos < OLS_SOUND_COUNT ? SOUND_ORDER[pos] : pos;
}

static int32_t sound_pos(int32_t sound)
{
    if (sound >= OLS_SOUND_COUNT) {
        return sound;
    }
    for (int32_t p = 0; p < OLS_SOUND_COUNT; p++) {
        if (SOUND_ORDER[p] == sound) {
            return p;
        }
    }
    return 0;
}

/* a sound PRESET may land on: for A, the synth sounds and the user sounds holding a synth sound; for a track, every
 * factory sound and the user sounds holding something; `shown`: not hidden too */
static int sound_offered(int32_t sound, int for_a, int shown)
{
    if (shown && sound_hidden(sound)) {
        return 0;
    }
    if (sound >= OLS_SOUND_FIRST_USER) {
        const ols_user_sound_t *u = &user_sounds[sound - OLS_SOUND_FIRST_USER];
        return u->valid && (!for_a || u->kind == OLS_KIND_VA);
    }
    return !for_a || live_sound_ok((uint32_t)sound);
}

/* the next sound offered `step` (+1 / -1) away; with everything hidden, the hidden ones are offered again */
static int32_t offered_step(int32_t sound, int32_t step, int for_a)
{
    for (int shown = 1; shown >= 0; shown--) {
        int32_t pos = sound_pos(sound);
        for (int32_t i = 0; i < OLS_SOUND_TOTAL; i++) {
            pos = (pos + OLS_SOUND_TOTAL + step) % OLS_SOUND_TOTAL;
            if (sound_offered(sound_at(pos), for_a, shown)) {
                return sound_at(pos);
            }
        }
    }
    return sound;
}

/* the first sound offered of family f (or -1) */
static int32_t family_first(int f, int for_a, int shown)
{
    for (int32_t pos = 0; pos < OLS_SOUND_TOTAL; pos++) {
        int32_t s = sound_at(pos);
        if (sound_family(s) == f && sound_offered(s, for_a, shown)) {
            return s;
        }
    }
    return -1;
}

/* OCT+: the first sound of the next family that offers one; OCT-: back to this family's first, or from there the
 * previous family's first. A family with nothing offered is skipped. */
static int32_t family_step(int32_t sound, int32_t step, int for_a)
{
    for (int shown = 1; shown >= 0; shown--) {
        int f = sound_family(sound);
        int32_t here = family_first(f, for_a, shown);
        if (step < 0 && here >= 0 && here != sound && sound_pos(here) < sound_pos(sound)) {
            return here;
        }
        for (int i = 1; i <= FAMILIES; i++) {
            int g = (int)((f - 1 + (step > 0 ? i : FAMILIES - i)) % FAMILIES) + 1;
            int32_t first = family_first(g, for_a, shown);
            if (first >= 0) {
                return first;
            }
        }
    }
    return sound;
}

/* the sound's name with its family: "F1 ACID 303" */
static const char *sound_label(int32_t sound)
{
    static char t[24];
    t[0] = 'F';
    t[1] = (char)('0' + sound_family(sound));
    t[2] = ' ';
    copy_text(t + 3, ols_sound_name((uint32_t)sound), sizeof t - 3u);
    return t;
}

static void preset_turn(int32_t step, int family);

static void scan_preset_encoder(uint32_t buttons)
{
    int32_t turn = detents(EN_PRESET);
    if (turn == 0) {
        return;
    }
    if (button_held(buttons, BTN_TRACKS)) {
        midi_channel_turn(turn);
        return;
    }
    preset_turn(turn > 0 ? 1 : -1, 0);
}

/* a new sound taken on a line (0..3, TRACKS = A): the page OPTION CONTROL chose for it (KEEP: none), so the same
 * routine follows each new sound (DRUM EQ only for a drum sound) */
static void new_sound_page(uint32_t line, int32_t sound)
{
    uint8_t p = set.new_page[line];
    if (!p) {
        return;
    }
    int kind = sound >= OLS_SOUND_FIRST_USER ? user_sounds[(sound - OLS_SOUND_FIRST_USER) % OLS_USER_SOUNDS].kind
                                              : ols_sound_kind(sound);
    if (p - 1u == PAGE_DRUM && (kind != OLS_KIND_DRUM || line == TRACKS)) {
        return;
    }
    set.page = (uint8_t)(p - 1u);
    ui.redraw = 1;
}

/* PRESET turned (family 0), or OCT- / OCT+ while a sound is chosen (family 1: the next family's first sound) */
static void preset_turn(int32_t step, int family)
{
    ui.sound_at = plat_ms();
    if (ui.on_a) {                              /* A's sound: the synth sounds, then the user sounds holding one */
        int32_t sound = family ? family_step(set.live_sound, step, 1) : offered_step(set.live_sound, step, 1);
        set.live_sound = (uint8_t)sound;
        sound_octave(TRACKS, (uint32_t)sound);
        mark_dirty();
        send_preset(T_LIVE, set.live_sound);
        ui.turned_at = 0;
        new_sound_page(TRACKS, sound);
        app_say("A SOUND", sound_label(set.live_sound));
        return;
    }
    uint8_t t = mirror.track;
    /* factory sounds, then the user sounds that hold something */
    int32_t sound = ui.pend_sound >= 0 ? ui.pend_sound : set.preset[t];
    sound = family ? family_step(sound, step, 0) : offered_step(sound, step, 0);
    if (live.track[t].pat.count > 0u) {
        /* the loop plays: the sound waits, red in the header, until SEL (another track gives it up) */
        ui.pend_sound = (int16_t)(sound == set.preset[t] ? -1 : sound);
        ui.redraw = 1;
        return;
    }
    set.preset[t] = (uint8_t)sound;
    sound_octave(t, (uint32_t)sound);
    mark_dirty();
    send_preset(t, set.preset[t]);
    ui.turned_at = 0;                           /* next knob turn re-reads the new sound */
    new_sound_page(t, sound);
    app_say("SOUND", sound_label(set.preset[t]));
}

static void live_sound_knob(uint32_t k, int32_t turn)
{
    uint16_t id = ENGINE_VA_API.macros[k];
    float value = fm_clampf(engine_va_get_param(&lv, id) + (float)turn * ENCODER_STEP, 0.0f, 1.0f);
    send_param(T_LIVE, id, value);
    say_int(ENGINE_VA_API.macro_names[k], (int32_t)(value * 100.0f + 0.5f), "");
}

static void sound_knob(uint32_t k, int32_t turn)
{
    if (ui.on_a) {
        live_sound_knob(k, turn);
        return;
    }
    uint8_t t = mirror.track;
    const ols_engine_api_t *api = track_api(t);
    uint16_t id = api->macros[k];
    float value = fm_clampf(ui.shadow[t][id] + (float)turn * ENCODER_STEP, 0.0f, 1.0f);
    ui.shadow[t][id] = value;
    send_param(t, id, value);
    say_int(api->macro_names[k], (int32_t)(value * 100.0f + 0.5f), "");
}

/* Controller mode: the knob sends a CC on the track's channel, the sound inside does not move. */
static void controller_knob(uint32_t k, int32_t turn)
{
    int32_t value = clamp((int32_t)ui.cc_value[k] + turn * 2, 0, 127);
    ui.cc_value[k] = (uint8_t)value;
    midi_out_cc(mirror.track, CONTROLLER_CC[k], (uint32_t)value);
    char label[8] = "CC ";
    int_text(CONTROLLER_CC[k], label + 3);
    say_int(label, value, "");
}

static void live_knob(uint32_t k, int32_t turn)
{
    uint8_t t = mirror.track;
    switch (k) {
    case 0:                                     /* MORPH between the two last recalled scenes */
        if (mirror.scene_prev < 0) {
            app_say("MORPH", "RECALL 2 SCENES");
            return;
        }
        ui.morph = fm_clampf(ui.morph + (float)turn * ENCODER_STEP, 0.0f, 1.0f);
        send_live(LIVE_MORPH, (uint8_t)mirror.scene_prev, (uint16_t)mirror.scene_last, ui.morph, 0);
        say_int("MORPH", (int32_t)(ui.morph * 100.0f + 0.5f), "%");
        break;
    case 1:
        ui.swing = fm_clampf(ui.swing + (float)turn * ENCODER_STEP, 0.0f, 1.0f);
        send_live(LIVE_SWING, 0, 0, ui.swing, 0);
        say_int("SWING", (int32_t)(ui.swing * 100.0f + 0.5f), "%");
        break;
    case 2:                                     /* PUMP: the drums duck the other tracks */
        ui.pump = fm_clampf(ui.pump + (float)turn * ENCODER_STEP, 0.0f, 1.0f);
        send_live(LIVE_PUMP, 0, 0, ui.pump, 0);
        say_int("PUMP", (int32_t)(ui.pump * 100.0f + 0.5f), "%");
        break;
    default:
        if (ui.on_a) {                          /* A's level */
            ui.lv_level = fm_clampf(ui.lv_level + (float)turn * ENCODER_STEP, 0.0f, 1.5f);
            lv_gain = ui.lv_level;
            say_int("A LEVEL", (int32_t)(ui.lv_level * (100.0f / 1.5f) + 0.5f), "%");     /* 100 % = the top (x1.5) */
            break;
        }
        avol.on = 0;                            /* a level turned by hand: A VOL lets go */
        ui.level[t] = fm_clampf(ui.level[t] + (float)turn * ENCODER_STEP, 0.0f, 1.5f);
        send_live(LIVE_GAIN, 0, 0, ui.level[t], t);
        say_int("LEVEL", (int32_t)(ui.level[t] * (100.0f / 1.5f) + 0.5f), "%");
        break;
    }
    mark_dirty();
}

/* SETUP page: one detent = one choice */
static void setup_knob(uint32_t k, int32_t turn)
{
    int32_t step = turn > 0 ? 1 : -1;
    switch (k) {
    case 0:
        midi_channel_turn(turn);
        break;
    case 1: {
        ui.xfade = (uint8_t)clamp((int32_t)ui.xfade + step, 0, 3);
        send_live(LIVE_XFADE, XFADE_BARS[ui.xfade], 0, 0.0f, 0);
        if (ui.xfade == 0u) {
            app_say("SCENES", "CUT ON THE BAR");
        } else {
            say_int("XFADE BARS", XFADE_BARS[ui.xfade], "");
        }
        break;
    }
    case 2:
        mirror.quantize = step > 0;
        send_live(LIVE_QUANTIZE, mirror.quantize, 0, 0.0f, 0);
        app_say("QUANTIZE", mirror.quantize ? "1/16" : "OFF");
        break;
    default: {                                  /* loop length of the selected track, in bars (A has no loop) */
        if (ui.on_a) {
            app_say("BARS", "A HAS NO LOOP");
            break;
        }
        uint8_t t = mirror.track;
        int32_t bars = (int32_t)(live.track[t].pat.length / OLS_TICKS_PER_BAR);
        bars = clamp(bars + step, 1, (int32_t)OLS_MAX_BARS);
        send_live(LIVE_LENGTH, 0, (uint16_t)(bars * (int32_t)OLS_TICKS_PER_BAR), 0.0f, t);
        say_int("BARS", bars, "");
        break;
    }
    }
}

/* DRUM EQ page (a drum line): BASS, TREBLE, the kick's and the hats' levels */
static const uint16_t DRUM_EQ_ID[KNOB_COUNT] = { DR_P_BASS, DR_P_TREBLE, DR_P_LEVEL_KICK, DR_P_LEVEL_HAT };
static const char *const DRUM_EQ_NAME[KNOB_COUNT] = { "BASS", "TREBLE", "KICK", "HATS" };

static void drum_eq_knob(uint32_t k, int32_t turn)
{
    uint8_t t = mirror.track;
    uint16_t id = DRUM_EQ_ID[k];
    float v = fm_clampf(ui.shadow[t][id] + (float)turn * ENCODER_STEP, 0.0f, 1.0f);
    ui.shadow[t][id] = v;
    send_param(t, id, v);
    say_int(DRUM_EQ_NAME[k], (int32_t)(v * 100.0f + 0.5f), "%");
}

/* MACRO page: ENERGY, SPACE, TENSION, DENSITY on every track at once. */
static void macro_knob(uint32_t k, int32_t turn)
{
    ui.macro[k] = fm_clampf(ui.macro[k] + (float)turn * ENCODER_STEP, 0.0f, 1.0f);
    send_live(LIVE_MACRO, (uint8_t)k, 0, ui.macro[k], 0);
    say_int(MACRO_NAME[k], (int32_t)(ui.macro[k] * 100.0f + 0.5f), "%");
}

static void scan_knobs(void)
{
    for (uint32_t k = 0; k < KNOB_COUNT; k++) {
        int32_t turn = detents((int)(EN_K1 + k));
        if (turn == 0) {
            continue;
        }
        if (ui.turned_at == 0 || plat_ms() - ui.turned_at > 400u) {
            resync_shadow();
        }
        ui.turned_at = plat_ms();
        mark_dirty();

        if (set.page == PAGE_SOUND && set.controller) {
            controller_knob(k, turn);
        } else if (set.page == PAGE_SOUND) {
            sound_knob(k, turn);
        } else if (set.page == PAGE_SETUP) {
            setup_knob(k, turn);
        } else if (page_slot(set.page, k) >= 0 && (set.page == PAGE_USER || set.page == PAGE_EQ
                                                   || !slot_is_default((uint32_t)page_slot(set.page, k)))) {
            map_turn((uint32_t)page_slot(set.page, k), turn);   /* a knob MAP CONTROL changed (EQ, USER: always) */
        } else if (set.page == PAGE_MACRO) {
            macro_knob(k, turn);
        } else if (set.page == PAGE_DRUM) {
            drum_eq_knob(k, turn);
        } else {
            live_knob(k, turn);
        }
    }
}

/* ------------------------------------------------------------ key pressure */

/* The hardest-pressed held note key is the aftertouch of the selected track
 * (and goes out as MIDI channel pressure). Sent only when it moves. */
static void scan_pressure(uint32_t keys)
{
    uint32_t most = 0;
    for (uint32_t k = 0; k < NKEYS; k++) {
        if ((keys >> k) & 1u && ui.key_role[k] == KEY_NOTE) {
            uint32_t p = touch_pressure(k);
            most = p > most ? p : most;
        }
    }
    int32_t moved = (int32_t)most - (int32_t)ui.pressure_sent;
    if (moved > 2 || moved < -2 || (most == 0u && ui.pressure_sent != 0u)) {
        ui.pressure_sent = (uint8_t)most;
        if (!set.controller) {
            push_event(EV_PRESSURE, ui.on_a ? T_LIVE : mirror.track, 0, 0, 0, (float)most * (1.0f / 127.0f));
        }
        midi_out_pressure(mirror.track, most);
    }
}

/* ------------------------------------------------------------ lights */

/* Two levels: dim = this key or button does something now, lit = it is on (selected, running). While a layer button is
 * held, the white keys show what the layer offers. A latched perform effect blinks (it runs without a hand on it). */

static uint32_t white_mask(int from, int to)
{
    uint32_t m = 0;
    for (int w = from; w < to; w++) {
        m |= 1u << WHITE_KEY[w];
    }
    return m;
}

/* the keys' lights: lit, dim (it does something now), and which of them blink (*blink, may be 0) */
static void key_lights(uint32_t keys, uint32_t buttons, uint32_t *lit, uint32_t *dim, uint32_t *blink_mask)
{
    uint32_t no_blink;
    uint32_t *bl = blink_mask ? blink_mask : &no_blink;
    *bl = 0;
    ui.keys_conf = 0;
    *lit = keys;
    *dim = 0;
    if (button_held(buttons, B_GLO)) {
        *dim = white_mask(0, 16);
        for (int s = 0; s < 8; s++) {
            if (live.scene[s].valid) {
                *lit |= 1u << WHITE_KEY[s];
            }
        }
    } else if (button_held(buttons, BTN_TRACKS)) {
        int all_out = 1;
        *dim = (white_mask(0, 16) & ~(1u << WHITE_KEY[4])) | (1u << KEY_MUTE_ALL) | (1u << KEY_AVOL);
        for (int t = 0; t < TRACKS; t++) {
            if (!live.track[t].mute) {
                *lit |= 1u << WHITE_KEY[t];
                all_out = 0;
            }
        }
        if (all_out) {                          /* MUTE ALL: fast blink while every track is out */
            *lit |= ((plat_ms() / 125u) & 1u) << KEY_MUTE_ALL;
            *bl |= 1u << KEY_MUTE_ALL;
        }
        if (avol.on) {                          /* A VOL running: a slow 2 s swell, lit first (lit, dim, off, dim) */
            uint32_t ph = (plat_ms() - avol.start) % 2000u;
            if (ph < 800u) {
                *lit |= 1u << KEY_AVOL;
            } else if (ph >= 1000u && ph < 1800u) {
                *dim &= ~(1u << KEY_AVOL);
            }
        }
        if (live.quantize) {
            *lit |= 1u << WHITE_KEY[6];
        }
        if (live.clock.external) {
            *lit |= 1u << WHITE_KEY[7];
        }
        if (live.send_clock) {
            *lit |= 1u << WHITE_KEY[8];
        }
        if (set.controller) {
            *lit |= 1u << WHITE_KEY[15];
        }
    } else if (button_held(buttons, BTN_PERFORM)) {
        int blink = (plat_ms() / 125u) & 1u;
        *dim = white_mask(0, FX_KEYS);
        for (int w = 0; w < FX_KEYS; w++) {
            if (ui.fx_on[w] && (!ui.fx_latched[w] || blink)) {
                *lit |= 1u << WHITE_KEY[w];
            }
            if (ui.fx_on[w] && ui.fx_latched[w]) {
                *bl |= 1u << WHITE_KEY[w];
            }
        }
    } else if (button_held(buttons, B_HOME)) {
        *dim = white_mask(0, 3) | white_mask(13, 16);       /* PANIC, RESTART, STOP; SOUNDS, LOOP EDIT, the settings */
        ui.keys_conf = (1u << WHITE_KEY[13]) | (1u << WHITE_KEY[15]);   /* the set-up ones: slow blink / red on screen */
        if ((plat_ms() / 500u) & 1u) {
            *dim &= ~ui.keys_conf;
        }
        *bl |= ui.keys_conf;
    } else if (button_held(buttons, BTN_PAGES)) {
        *dim = white_mask(0, 1) | white_mask(APP_WHEEL ? 13 : 14, 16);   /* the line's reset; the wheel; the drum keys */
        *lit |= 1u << WHITE_KEY[set.drum_map == DRUM_MAP_GM ? 15 : 14];
    } else if (button_held(buttons, B_ARP)) {
        *dim = white_mask(0, 15);
    } else if (!ui.on_a && live.track[mirror.track].kind == OLS_KIND_DRUM) {
        /* a drum track: kicks (C) and open hats (A#) blink every half second, snares (D, E) and the closed hat
         * (F#) lit */
        int blink = (plat_ms() / 500u) & 1u;
        for (uint32_t k = 0; k < NKEYS; k++) {
            uint32_t n = drum_key_note((uint32_t)(FIRST_KEY_NOTE + (int32_t)k + 12 * *line_octave()));
            if (n == 36u || n == 46u) {
                *bl |= 1u << k;
            }
            if (((n == 36u || n == 46u) && blink) || n == 38u || n == 40u || n == 42u) {
                *lit |= 1u << k;
            }
        }
    }
}

static uint32_t button_lit(uint32_t buttons)
{
    uint32_t lit = buttons;
    if (set_dirty || live.changes != saved_changes) {
        lit |= 1u << B_SAVE;
    }
    if (live.clock.running) {
        lit |= 1u << B_PLAY;
    }
    if (live.recording) {
        lit |= 1u << B_REC;
        lit = (plat_ms() / 60u) & 1u ? lit | (1u << B_EDIT) : lit & ~(1u << B_EDIT);   /* recording: EDIT flickers */
    }
    if (ui.on_a) {
        lit |= 1u << BTN_VALIDATE;              /* on A, SEL held + SELECT then let go swaps A with a track */
    }
    if (!ui.on_a && (ols_live_can_undo_track(&live, mirror.track) || swap_undoable(0))) {
        lit |= 1u << B_ENV;                     /* something to undo: ENV lit; to redo: LFO lit */
    }
    if (!ui.on_a && (ols_live_can_redo_track(&live, mirror.track) || swap_undoable(1))) {
        lit |= 1u << B_LFO;
    }
    if (ui.on_a && set.arp_mode != ARP_OFF) {
        lit |= 1u << B_ARP;                     /* A arpeggiates */
    }
    for (int w = 0; w < FX_KEYS; w++) {
        if (ui.fx_on[w] && PERFORM_KEY[w].type == FX_LOOP) {
            lit = (plat_ms() / 60u) & 1u ? lit | (1u << BTN_PERFORM) : lit & ~(1u << BTN_PERFORM);
            break;                                /* a loop runs: SEQ flickers fast */
        }
        if (ui.fx_latched[w] && ((plat_ms() / 125u) & 1u)) {
            lit |= 1u << BTN_PERFORM;             /* something latched: SEQ (perform) blinks */
        }
    }
    return lit;
}

static uint32_t button_dim(void)
{
    uint32_t dim = (1u << B_ARP) | (1u << B_GLO) | (1u << BTN_PERFORM) | (1u << BTN_TRACKS) | (1u << BTN_PAGES) | (1u << B_PLAY)
                   | (1u << B_REC) | (1u << B_EDIT) | (1u << B_OCTDN) | (1u << B_OCTUP) | (1u << B_HOME) | (1u << B_SAVE);
    return dim;
}

static void show_lights(uint32_t keys, uint32_t buttons)
{
    uint32_t lit_k, dim_k, lit_b = button_lit(buttons), dim_b = button_dim() & ~lit_b;
    int loop = running_loop();
    if (loop >= 0) {                            /* a loop runs: OCT- / OCT+ = longer / shorter, lit when they can */
        uint32_t oct = (1u << B_OCTDN) | (1u << B_OCTUP);
        lit_b &= ~oct;
        dim_b &= ~oct;
        lit_b |= (loop > 0 ? 1u << B_OCTDN : 0u) | (loop + 1 < FX_KEYS && PERFORM_KEY[loop + 1].type == FX_LOOP ? 1u << B_OCTUP : 0u);
    }
    uint32_t play = 1u << B_PLAY;
    key_lights(keys, buttons, &lit_k, &dim_k, 0);
    if (!live.recording) {                      /* PLAY green; red while recording */
        lit_b = (lit_b & ~play) | (lit_b & play ? LED_PLAY_GREEN : 0u);
        dim_b = (dim_b & ~play) | (dim_b & play ? LED_PLAY_GREEN : 0u);
    }
    plat_play_red(live.recording);
    plat_lights(lit_b, lit_k, dim_b, dim_k & ~lit_k, 1);
}

/* ------------------------------------------------------------ all inputs */

static void input(void)
{
    if (map_is_open()) {                    /* MAP / OPTION CONTROL take the buttons and encoders */
        map_input();
        return;
    }
    if (sedit_is_open()) {                  /* SOUND EDIT too */
        sedit_input();
        return;
    }
    if (ledit_is_open()) {                  /* and LOOP EDIT */
        ledit_input();
        return;
    }
    if (fav_is_open()) {                    /* and SOUNDS */
        fav_input();
        return;
    }
    uint32_t buttons = plat_buttons();

    touch_poll();                           /* key velocity sensing (lib/touch.h) */
    uint32_t keys = touch_keys();

    scan_buttons(buttons);
    avol_tick();
    scan_keys(keys, buttons);
    scan_pressure(keys);
    scan_track_encoder(buttons);
    scan_tempo_encoder(buttons);
    scan_preset_encoder(buttons);
    if (set.page == PAGE_DRUM && !drum_line()) {   /* DRUM EQ shown on a synth line or A: back to SOUND */
        set.page = PAGE_SOUND;
        ui.redraw = 1;
    }
    scan_knobs();

    /* a layer button held: what its keys last said stays on top until the button is let go or another comes */
    if (ui.msg_until && plat_ms() < ui.msg_until && ui.msg_style != MSG_TAKEN && ui.msg_style != MSG_NOT_TAKEN
        && (buttons & ((1u << B_ARP) | (1u << BTN_PERFORM) | (1u << B_GLO) | (1u << B_HOME) | (1u << BTN_PAGES)
                       | (1u << BTN_TRACKS) | (1u << B_SAVE)))
        && ui.msg_until - plat_ms() < 400u) {
        ui.msg_until = plat_ms() + 400u;
    }

    if (keys != ui.keys || buttons != ui.buttons) {
        ui.redraw = 1;
        ui.activity = 1;
    }
    ui.keys = keys;
    ui.buttons = buttons;

    show_lights(keys, buttons);
}
