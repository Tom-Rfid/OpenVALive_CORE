/*
 * OpenVALive - FM-1 app: the CONTROL MAP, controls the player assigns to what they want.
 *
 * Slots: the four knobs of the USER page (K1..K4) on every target; a target with more controls adds its own
 * (APP_MAP_SLOTS: the SMK-37's 4 faders in their USER bank, its pitch and mod wheels). Each slot has a target
 * (a level, a macro, an effect, a sound knob, the tempo, bend, aftertouch, a MIDI CC...) and an option (which
 * track, which CC).
 *
 * The menu: GLO + SEL (the SMK-37: a long press on its Bluetooth button too). SELECT chooses the row, PRESET the
 * target, ALGO the option; moving a mapped control jumps to its row. HOME, SEL or GLO closes it; the map is kept in
 * the flash when it closes (and with SAVE). Under the slots, the settings rows (HOME + white key 16 opens the menu on
 * them): the drum keys' map, the original map's D# and G#, the keys on screen; kept when the menu closes.
 *
 * Effects (FILTER, RESO, DRIVE, DELAY, CHORUS, CRUSH, DECAY) hold the role's value on the chosen tracks while the
 * control is up; all the way down (under 2 %), the sounds are back as they are stored.
 */

#ifndef APP_MAP_SLOTS
#define APP_MAP_SLOTS 4u            /* the FM-1: the USER page's four knobs */
#endif

enum { MAP_K1, MAP_K2, MAP_K3, MAP_K4, MAP_F1, MAP_F2, MAP_F3, MAP_F4, MAP_PITCH, MAP_MOD, MAP_ALL_SLOTS };
static const char *const MAP_SLOT_NAME[MAP_ALL_SLOTS] = {
    "K1", "K2", "K3", "K4", "FADER 1", "FADER 2", "FADER 3", "FADER 4", "PITCH", "MOD"
};

enum { ARG_NONE, ARG_TRACK, ARG_SCOPE, ARG_KNOB, ARG_CC };
enum {
    MAP_NONE, MAP_LEVEL, MAP_ENERGY, MAP_SPACE, MAP_TENSION, MAP_DENSITY, MAP_MORPH, MAP_SWING, MAP_PUMP,
    MAP_FILTER, MAP_RESO, MAP_DRIVE, MAP_DELAY, MAP_CHORUS, MAP_CRUSH, MAP_DECAY,
    MAP_SOUND, MAP_TEMPO, MAP_BEND, MAP_PRESSURE, MAP_CC, MAP_TARGETS
};
static const struct {
    const char *name;
    uint8_t arg;                    /* ARG_*: what the option chooses */
    uint8_t role;                   /* effects: OLS_ROLE_* */
} MAP_TARGET[MAP_TARGETS] = {
    { "--", ARG_NONE, 0 },
    { "LEVEL", ARG_TRACK, 0 },
    { "ENERGY", ARG_NONE, 0 },      { "SPACE", ARG_NONE, 0 },
    { "TENSION", ARG_NONE, 0 },     { "DENSITY", ARG_NONE, 0 },
    { "MORPH", ARG_NONE, 0 },       { "SWING", ARG_NONE, 0 },       { "PUMP", ARG_NONE, 0 },
    { "FILTER", ARG_SCOPE, OLS_ROLE_CUTOFF },   { "RESO", ARG_SCOPE, OLS_ROLE_RESO },
    { "DRIVE", ARG_SCOPE, OLS_ROLE_DRIVE },     { "DELAY", ARG_SCOPE, OLS_ROLE_DELAY_MIX },
    { "CHORUS", ARG_SCOPE, OLS_ROLE_CHORUS },   { "CRUSH", ARG_SCOPE, OLS_ROLE_CRUSH },
    { "DECAY", ARG_SCOPE, OLS_ROLE_DECAY },
    { "SOUND", ARG_KNOB, 0 },       { "TEMPO", ARG_NONE, 0 },
    { "BEND", ARG_NONE, 0 },        { "AFTER", ARG_NONE, 0 },       { "MIDI CC", ARG_CC, 0 },
};
#define SCOPE_ALL 0u                /* ARG_SCOPE: 0 every track, 1..4 one track, 5 the selected one */
#define SCOPE_SELECTED 5u

#define MAP_MAGIC 0x4F4C4D31u       /* "OLM1" */
typedef struct {
    uint32_t magic;
    uint8_t target[MAP_ALL_SLOTS];
    uint8_t arg[MAP_ALL_SLOTS];
} control_map_t;

static control_map_t cmap;
static float map_value[MAP_ALL_SLOTS];  /* where each control last put its target, 0..1 */
static struct {
    uint8_t open, row, top, dirty, conf_dirty, drawn;
    uint32_t buttons;
    uint8_t to_sounds;                          /* MENU row turned: SOUNDS next */
} mapui;

/* the settings rows, after the slots */
enum { CONF_DRUM_MAP, CONF_DRUM_DS, CONF_DRUM_GS, CONF_KEYS_SCREEN, CONF_MENU, CONF_ROWS };
static const char *const CONF_NAME[CONF_ROWS] = { "DRUM KEYS", "DRUM D#", "DRUM G#", "KEYS ON SCREEN", "MENU" };
/* the menu buttons: one pressed leaves a set-up screen (what it changed is kept) */
#define MENU_BUTTONS ((1u << B_HOME) | (1u << B_GLO) | (1u << BTN_PAGES) | (1u << B_ARP) | (1u << BTN_PERFORM) \
                      | (1u << B_SAVE))
#define MAP_ROWS (APP_MAP_SLOTS + CONF_ROWS)
#define MAP_VISIBLE 9u

static const char *conf_text(uint32_t c)
{
    switch (c) {
    case CONF_DRUM_MAP: return set.drum_map == DRUM_MAP_GM ? "GM" : "ORIGINAL";
    case CONF_DRUM_DS: return DRUM_PERC[set.drum_ds % DRUM_PERCS].name;
    case CONF_DRUM_GS: return DRUM_PERC[set.drum_gs % DRUM_PERCS].name;
    case CONF_MENU: return "SOUNDS >";           /* the other set-up screen: turn or OCT to go */
    default: return set.keys_screen ? "ON" : "OFF";
    }
}

static void conf_turn(uint32_t c, int32_t turn)
{
    int32_t step = turn > 0 ? 1 : -1;
    switch (c) {
    case CONF_DRUM_MAP: set.drum_map = (uint8_t)!set.drum_map; break;
    case CONF_DRUM_DS: set.drum_ds = (uint8_t)((set.drum_ds + DRUM_PERCS + step) % DRUM_PERCS); break;
    case CONF_DRUM_GS: set.drum_gs = (uint8_t)((set.drum_gs + DRUM_PERCS + step) % DRUM_PERCS); break;
    case CONF_MENU: mapui.to_sounds = 1; return;
    default: set.keys_screen = (uint8_t)!set.keys_screen; break;
    }
    mapui.conf_dirty = 1;
}

static void map_defaults(void)
{
    static const uint8_t TARGET[MAP_ALL_SLOTS] = {
        MAP_FILTER, MAP_RESO, MAP_DELAY, MAP_CHORUS,        /* USER knobs: the selected track's effects */
        MAP_FILTER, MAP_DELAY, MAP_DRIVE, MAP_CRUSH,        /* USER faders: effects on the whole mix */
        MAP_BEND, MAP_MORPH                                 /* the wheels */
    };
    static const uint8_t ARG[MAP_ALL_SLOTS] = {
        SCOPE_SELECTED, SCOPE_SELECTED, SCOPE_SELECTED, SCOPE_SELECTED, SCOPE_ALL, SCOPE_ALL, SCOPE_ALL, SCOPE_ALL, 0, 0
    };
    cmap.magic = MAP_MAGIC;
    for (uint32_t s = 0; s < MAP_ALL_SLOTS; s++) {
        cmap.target[s] = TARGET[s];
        cmap.arg[s] = ARG[s];
        map_value[s] = TARGET[s] == MAP_BEND ? 0.5f : 0.0f;
    }
}

static int map_valid(int size)
{
    if (size != (int)sizeof cmap || cmap.magic != MAP_MAGIC) {
        return 0;
    }
    for (uint32_t s = 0; s < MAP_ALL_SLOTS; s++) {
        if (cmap.target[s] >= MAP_TARGETS || cmap.arg[s] > 127u) {
            return 0;
        }
    }
    return 1;
}

static void map_load(void)
{
    int size = plat_store_load(OBJ_MAP, &cmap, sizeof cmap);
    if (!map_valid(size)) {
        map_defaults();
    }
    for (uint32_t s = 0; s < MAP_ALL_SLOTS; s++) {
        map_value[s] = cmap.target[s] == MAP_BEND ? 0.5f : 0.0f;
    }
}

static int map_save(void)
{
    if (plat_store_save(OBJ_MAP, &cmap, sizeof cmap) != 0) {
        return 0;
    }
    mapui.dirty = 0;
    return 1;
}

/* ------------------------------------------------------------ what a slot does */

static uint8_t map_track(uint32_t slot)
{
    uint8_t a = cmap.arg[slot];
    return (a >= 1u && a <= TRACKS) ? (uint8_t)(a - 1u) : mirror.track;
}

static uint8_t map_scope(uint32_t slot)
{
    uint8_t a = cmap.arg[slot];
    if (a == SCOPE_ALL) {
        return OLS_TRACK_ALL;
    }
    return (a >= 1u && a <= TRACKS) ? (uint8_t)(a - 1u) : mirror.track;
}

/* the option's text: "T2", "ALL", "SEL TRK", "CC 74", the sound knob's name */
static void map_arg_text(uint32_t slot, char *t)
{
    uint32_t kind = MAP_TARGET[cmap.target[slot]].arg;
    uint8_t a = cmap.arg[slot];
    t[0] = 0;
    if (kind == ARG_TRACK || kind == ARG_SCOPE) {
        if (kind == ARG_SCOPE && a == SCOPE_ALL) {
            copy_text(t, "ALL", 12);
        } else if (a >= 1u && a <= TRACKS) {
            copy_text(t, "T", 12);
            int_text(a, t + 1);
        } else {
            copy_text(t, "SEL TRK", 12);
        }
    } else if (kind == ARG_KNOB) {
        const ols_engine_api_t *api = track_api(mirror.track);
        copy_text(t, api->macro_names[(a - 1u) % KNOB_COUNT], 12);
    } else if (kind == ARG_CC) {
        copy_text(t, "CC ", 12);
        int_text(a, t + 3);
    }
}

/* where the target is now, 0..1 (a knob turns from there) */
static float map_current(uint32_t slot)
{
    switch (cmap.target[slot]) {
    case MAP_LEVEL:
        return live.track[map_track(slot)].gain / 1.25f;
    case MAP_ENERGY: case MAP_SPACE: case MAP_TENSION: case MAP_DENSITY:
        return live.macro[cmap.target[slot] - MAP_ENERGY];
    case MAP_SWING:
        return live.swing;
    case MAP_PUMP:
        return live.pump;
    case MAP_TEMPO:
        return (live.clock.bpm - 60.0f) * (1.0f / 120.0f);
    case MAP_SOUND: {
        const ols_engine_api_t *api = track_api(mirror.track);
        return ui.shadow[mirror.track][api->macros[(cmap.arg[slot] - 1u) % KNOB_COUNT]];
    }
    default:
        return map_value[slot];
    }
}

/* the slot's control moved to v (0..1): its target follows */
static void map_set(uint32_t slot, float v)
{
    if (slot >= APP_MAP_SLOTS) {
        return;
    }
    v = fm_clampf(v, 0.0f, 1.0f);
    map_value[slot] = v;
    uint8_t target = cmap.target[slot];
    const char *name = MAP_TARGET[target].name;
    int32_t pct = (int32_t)(v * 100.0f + 0.5f);
    switch (target) {
    case MAP_NONE:
        return;
    case MAP_LEVEL: {
        uint8_t t = map_track(slot);
        ui.level[t] = v * 1.25f;                /* unity at 80 % of the travel */
        send_live(LIVE_GAIN, 0, 0, ui.level[t], t);
        pct = (int32_t)(ui.level[t] * 100.0f + 0.5f);
        break;
    }
    case MAP_ENERGY: case MAP_SPACE: case MAP_TENSION: case MAP_DENSITY: {
        uint8_t m = (uint8_t)(target - MAP_ENERGY);
        ui.macro[m] = v;
        send_live(LIVE_MACRO, m, 0, v, 0);
        break;
    }
    case MAP_MORPH:
        if (mirror.scene_prev < 0) {            /* no two scenes yet: aftertouch, so the control still plays */
            push_event(EV_PRESSURE, mirror.track, 0, 0, 0, v);
            return;
        }
        ui.morph = v;
        send_live(LIVE_MORPH, (uint8_t)mirror.scene_prev, (uint16_t)mirror.scene_last, v, 0);
        break;
    case MAP_SWING:
        ui.swing = v;
        send_live(LIVE_SWING, 0, 0, v, 0);
        break;
    case MAP_PUMP:
        ui.pump = v;
        send_live(LIVE_PUMP, 0, 0, v, 0);
        break;
    case MAP_SOUND: {
        uint8_t t = mirror.track;
        const ols_engine_api_t *api = track_api(t);
        uint32_t k = (cmap.arg[slot] - 1u) % KNOB_COUNT;
        ui.shadow[t][api->macros[k]] = v;
        send_param(t, api->macros[k], v);
        name = api->macro_names[k];
        break;
    }
    case MAP_TEMPO: {
        float bpm = 60.0f + v * 120.0f;
        mirror.bpm = bpm;
        send_live(LIVE_TEMPO, 0, 0, bpm, 0);
        pct = (int32_t)(bpm + 0.5f);
        say_int(name, pct, " BPM");
        return;
    }
    case MAP_BEND: {
        push_event(EV_BEND, mirror.track, 0, 0, 0, (v - 0.5f) * 4.0f);   /* +-2 semitones */
        uint32_t bend = (uint32_t)clamp((int32_t)(v * 16383.0f + 0.5f), 0, 16383);
        plat_midi_send(0x0Eu | (0xE0u | mirror.track) << 8 | (bend & 127u) << 16 | (bend >> 7) << 24, TO_USB);
        return;                                 /* (no message: a wheel moves all the time) */
    }
    case MAP_PRESSURE:
        if (!set.controller) {
            push_event(EV_PRESSURE, mirror.track, 0, 0, 0, v);
        }
        midi_out_pressure(mirror.track, (uint32_t)(v * 127.0f + 0.5f));
        return;
    case MAP_CC: {
        midi_out_cc(mirror.track, cmap.arg[slot] & 127u, (uint32_t)(v * 127.0f + 0.5f));
        char t[12];
        map_arg_text(slot, t);
        say_int(t, (int32_t)(v * 127.0f + 0.5f), "");
        return;
    }
    default: {                                  /* the effects: hold the role, or let the sounds back */
        uint8_t role = MAP_TARGET[target].role;
        if (v < 0.02f) {
            send_live(LIVE_OVERRIDE, 0, role, 0.0f, map_scope(slot));
            app_say(name, "OFF");
            return;
        }
        send_live(LIVE_OVERRIDE, 1, role, v, map_scope(slot));
        break;
    }
    }
    say_int(name, pct, "%");
}

/* a knob of the USER page turned: from where its target is */
static void map_turn(uint32_t slot, int32_t turn)
{
    map_set(slot, map_current(slot) + (float)turn * ENCODER_STEP);
}

/* the USER page's knobs as drawn: the target's name and where it is */
static void map_knob_value(uint32_t k, float *bar, int32_t *shown, const char **label)
{
    uint8_t target = cmap.target[k];
    *label = MAP_TARGET[target].name;
    if (target == MAP_SOUND) {
        *label = track_api(mirror.track)->macro_names[(cmap.arg[k] - 1u) % KNOB_COUNT];
    }
    *bar = map_current(k);
    *shown = target == MAP_TEMPO ? (int32_t)(live.clock.bpm + 0.5f) : (int32_t)(*bar * 100.0f + 0.5f);
}

/* ------------------------------------------------------------ the menu */

static int map_is_open(void) { return mapui.open; }

static void map_open(void)
{
    mapui.open = 1;
    mapui.drawn = 0;
    mapui.buttons = plat_buttons();             /* what is held now does not count as a press */
    lcd_fill(0, 0, 240, 240, C_BLACK);
}

static void map_open_settings(void)
{
    map_open();
    mapui.row = APP_MAP_SLOTS;                  /* the first settings row */
}

static void map_close(void)
{
    mapui.open = 0;
    if (mapui.conf_dirty) {
        settings_keep();                        /* the settings: kept at once, past a power cycle */
        mapui.conf_dirty = 0;
    }
    if (mapui.dirty && !map_save()) {
        app_say("CONTROL MAP", "NOT SAVED");
    } else {
        app_say("CONTROL MAP", "KEPT");
    }
    ui.buttons = plat_buttons();                /* the app does not see the closing press again */
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = 1;
}

/* a mapped control moved while the menu is open: its row is chosen */
static void map_touch(uint32_t slot)
{
    if (mapui.open && slot < APP_MAP_SLOTS && mapui.row != slot) {
        mapui.row = (uint8_t)slot;
        mapui.drawn = 0;
    }
}

static void map_change_target(int32_t turn)
{
    uint32_t s = mapui.row;
    int32_t t = ((int32_t)cmap.target[s] + (turn > 0 ? 1 : -1) + MAP_TARGETS) % MAP_TARGETS;
    if (cmap.target[s] >= MAP_FILTER && cmap.target[s] <= MAP_DECAY) {
        send_live(LIVE_OVERRIDE, 0, MAP_TARGET[cmap.target[s]].role, 0.0f, map_scope(s));   /* let the old one go */
    }
    cmap.target[s] = (uint8_t)t;
    switch (MAP_TARGET[t].arg) {               /* a sensible option for the new kind */
    case ARG_SCOPE:
        cmap.arg[s] = s < MAP_F1 ? SCOPE_SELECTED : SCOPE_ALL;
        break;
    case ARG_TRACK:
        cmap.arg[s] = s >= MAP_F1 && s <= MAP_F4 ? (uint8_t)(s - MAP_F1 + 1u) : 0u;
        break;
    case ARG_KNOB:
        cmap.arg[s] = (uint8_t)(s % KNOB_COUNT + 1u);
        break;
    case ARG_CC:
        cmap.arg[s] = 74u;                      /* brightness */
        break;
    default:
        cmap.arg[s] = 0;
        break;
    }
    map_value[s] = t == MAP_BEND ? 0.5f : 0.0f;
}

static void map_change_arg(int32_t turn)
{
    uint32_t s = mapui.row;
    int32_t step = turn > 0 ? 1 : -1;
    int32_t a = cmap.arg[s];
    switch (MAP_TARGET[cmap.target[s]].arg) {
    case ARG_TRACK:
        a = (a + step + TRACKS + 1) % (TRACKS + 1);         /* SEL TRK, 1..4 */
        break;
    case ARG_SCOPE:
        if (cmap.target[s] >= MAP_FILTER) {
            send_live(LIVE_OVERRIDE, 0, MAP_TARGET[cmap.target[s]].role, 0.0f, map_scope(s));
        }
        a = (a + step + TRACKS + 2) % (TRACKS + 2);         /* ALL, 1..4, SEL TRK */
        break;
    case ARG_KNOB:
        a = (a - 1 + step + KNOB_COUNT) % KNOB_COUNT + 1;
        break;
    case ARG_CC:
        a = clamp(a + turn, 0, 127);
        break;
    default:
        return;
    }
    cmap.arg[s] = (uint8_t)a;
}

/* the keys play while a menu page is open (the FM-1's go through the app's key scan, without a layer) */
static void menu_keys(void)
{
    touch_poll();
    uint32_t keys = touch_keys();
    scan_keys(keys, 0);
    ui.keys = keys;
}

/* the menu's input, in place of the app's (keys still play on a target whose keys go straight to the sound) */
static void map_input(void)
{
    uint32_t buttons = plat_buttons();
    uint32_t pressed = buttons & ~mapui.buttons;
    mapui.buttons = buttons;
    if (pressed & (MENU_BUTTONS | (1u << BTN_VALIDATE))) {
        map_close();
        return;
    }
    if (mapui.row == APP_MAP_SLOTS + CONF_MENU && (pressed & ((1u << B_OCTDN) | (1u << B_OCTUP)))) {
        mapui.to_sounds = 1;                    /* the MENU row: OCT- / OCT+ go to SOUNDS too */
    }
    if (pressed & (1u << B_PLAY)) {
        send_live(LIVE_PLAY_TOGGLE, 0, 0, 0.0f, 0);
    }
    menu_keys();
    int32_t turn = plat_enc(EN_SELECT);
    if (turn) {
        mapui.row = (uint8_t)clamp((int32_t)mapui.row + (turn > 0 ? 1 : -1), 0, (int32_t)MAP_ROWS - 1);
        mapui.drawn = 0;
    }
    turn = plat_enc(EN_PRESET);
    int32_t option = plat_enc(EN_ALGO);
    if (mapui.row >= APP_MAP_SLOTS) {           /* a settings row: either knob changes it */
        if (turn + option) {
            conf_turn(mapui.row - APP_MAP_SLOTS, turn + option);
            mapui.drawn = 0;
        }
        turn = option = 0;
    }
    if (mapui.to_sounds) {                      /* MENU: on to SOUNDS */
        mapui.to_sounds = 0;
        map_close();
        fav_open();
        return;
    }
    if (turn) {
        map_change_target(turn);
        mapui.dirty = 1;
        mapui.drawn = 0;
    }
    turn = option;
    if (turn) {
        map_change_arg(turn);
        mapui.dirty = 1;
        mapui.drawn = 0;
    }
    for (uint32_t k = 0; k < KNOB_COUNT; k++) {
        if (plat_enc((int)(EN_K1 + k))) {
            map_touch(k);                       /* a knob turned: its row */
        }
    }
}

#define MAP_ROW_H 18
#define MAP_TOP 34

static void map_draw(void)
{
    if (mapui.drawn) {
        return;
    }
    mapui.drawn = 1;
    cv_begin(240, MAP_TOP, C_BLACK);
    cv_text(8, 6, &FONT_B, "CONTROL MAP", C_HI);
    {
        char v[32] = "OpenVALive ";                      /* the firmware and its version, in the logo's orange */
        app_version_text(v + 9, sizeof v - 9u);
        cv_text(234 - text_w(&FONT_XS, v), 9, &FONT_XS, v, RGB(238, 90, 36));
    }
    cv_rect(0, MAP_TOP - 2, 240, 1, C_LINE);
    cv_blit(0, 0);
    /* the rows in view (scrolled with the row), three at a time (the canvas is 240 x 80) */
    if (mapui.row < mapui.top) {
        mapui.top = mapui.row;
    } else if (mapui.row >= mapui.top + MAP_VISIBLE) {
        mapui.top = (uint8_t)(mapui.row - MAP_VISIBLE + 1u);
    }
    for (uint32_t first = 0; first < MAP_VISIBLE; first += 3u) {
        cv_begin(240, 3 * MAP_ROW_H, C_BLACK);
        for (uint32_t i = first; i < first + 3u && i < MAP_VISIBLE; i++) {
            uint32_t s = mapui.top + i;
            if (s >= MAP_ROWS) {
                break;
            }
            int32_t y = (int32_t)(i - first) * MAP_ROW_H;
            int sel = s == mapui.row;
            if (sel) {
                cv_rect(0, y, 240, MAP_ROW_H - 1, C_LINE);
            }
            if (s >= APP_MAP_SLOTS) {                       /* a setting: its name in pink (SETUP's colour) */
                const char *v = conf_text(s - APP_MAP_SLOTS);
                cv_text(6, y + 3, &FONT_XS, CONF_NAME[s - APP_MAP_SLOTS], sel ? C_WHITE : C_PINK);
                cv_text(234 - text_w(&FONT_XS, v), y + 3, &FONT_XS, v, sel ? C_WHITE : C_AMB);
                continue;
            }
            char arg[12];
            map_arg_text(s, arg);
            const char *name = MAP_TARGET[cmap.target[s]].name;
            cv_text(6, y + 3, &FONT_XS, MAP_SLOT_NAME[s], sel ? C_WHITE : C_GRAY);
            cv_text(70, y + 3, &FONT_XS, name, sel ? C_WHITE : C_HI);
            cv_text(234 - text_w(&FONT_XS, arg), y + 3, &FONT_XS, arg, sel ? C_WHITE : C_AMB);
        }
        cv_blit(0, MAP_TOP + first * MAP_ROW_H);
    }
    cv_begin(240, 36, C_BLACK);
    cv_text(6, 4, &FONT_XS, "SELECT row  PRESET target", C_GRAY);
    cv_text(6, 20, &FONT_XS, "ALGO option  MENU BTN: OUT", C_GRAY);
    cv_blit(0, 240 - 36);
}
