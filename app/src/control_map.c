/*
 * OpenVALive - FM-1 app: MAP CONTROL and OPTION CONTROL, the set-up screens next to SOUNDS.
 *
 * MAP CONTROL (GLO + SEL; the SMK-37: a long press on its Bluetooth button too):
 *   - the knob pages FX goes through, in their order: PRESET moves one up / down, ALGO shows / hides it;
 *   - what the four knobs of MACRO, LIVE, EQ and USER do (a target's faders and wheels follow USER's): each slot
 *     has a target (a level, a macro, an effect, a sound knob, the EQ, the tempo, bend, a MIDI CC...) and an option
 *     (which track, which CC). PRESET the target, ALGO the option; moving a control jumps to its row.
 * OPTION CONTROL (HOME + white key 10): the drum keys' map, each D# / G# key's percussion, the keys on screen,
 *   and for each line the page a new sound brings (KEEP: none).
 * Both: SELECT the row, EDIT puts the row back to its default, the MENU row (last) goes to the next screen with
 * PRESET or OCT (SOUNDS, MAP CONTROL, OPTION CONTROL in turn), a menu button closes; what changed is kept.
 *
 * Effects (FILTER, RESO, DRIVE, DELAY, CHORUS, CRUSH, DECAY) hold the role's value on the chosen tracks while the
 * control is up; all the way down (under 2 %), the sounds are back as they are stored.
 */

#ifndef APP_MAP_SLOTS
#define APP_MAP_SLOTS 4u            /* the FM-1: the USER page's four knobs */
#endif

/* the slots: USER's knobs and a target's own controls first (their numbers are kept in the flash), then the
 * knobs of the other mapped pages */
enum {
    MAP_K1, MAP_K2, MAP_K3, MAP_K4, MAP_F1, MAP_F2, MAP_F3, MAP_F4, MAP_PITCH, MAP_MOD,
    MAP_MACRO1 = 10, MAP_LIVE1 = 14, MAP_EQ1 = 18, MAP_ALL_SLOTS = 22
};
static const char *const MAP_SLOT_NAME[MAP_ALL_SLOTS] = {
    "USER K1", "USER K2", "USER K3", "USER K4", "FADER 1", "FADER 2", "FADER 3", "FADER 4", "PITCH", "MOD",
    "MACRO K1", "MACRO K2", "MACRO K3", "MACRO K4", "LIVE K1", "LIVE K2", "LIVE K3", "LIVE K4",
    "EQ K1", "EQ K2", "EQ K3", "EQ K4",
};

enum { ARG_NONE, ARG_TRACK, ARG_SCOPE, ARG_KNOB, ARG_CC };
enum {
    MAP_NONE, MAP_LEVEL, MAP_ENERGY, MAP_SPACE, MAP_TENSION, MAP_DENSITY, MAP_MORPH, MAP_SWING, MAP_PUMP,
    MAP_FILTER, MAP_RESO, MAP_DRIVE, MAP_DELAY, MAP_CHORUS, MAP_CRUSH, MAP_DECAY,
    MAP_SOUND, MAP_TEMPO, MAP_BEND, MAP_PRESSURE, MAP_CC,
    MAP_EQ_BASS, MAP_EQ_MID, MAP_EQ_TREBLE, MAP_EQ_FREQ, MAP_PAN, MAP_TARGETS   /* (new ones at the end: numbers kept) */
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
    { "BASS", ARG_NONE, 0 },        { "MID", ARG_NONE, 0 },         { "TREBLE", ARG_NONE, 0 },
    { "MID FREQ", ARG_NONE, 0 },    { "PAN", ARG_TRACK, 0 },
};
#define SCOPE_ALL 0u                /* ARG_SCOPE: 0 every track, 1..4 one track, 5 the selected one */
#define SCOPE_SELECTED 5u

#define MAP_MAGIC 0x4F4C4D32u       /* "OLM2" */
#define MAP_MAGIC_V1 0x4F4C4D31u    /* "OLM1": USER's knobs and the target's controls only */
#define MAP_V1_SLOTS 10u
typedef struct {
    uint32_t magic;
    uint8_t target[MAP_ALL_SLOTS];
    uint8_t arg[MAP_ALL_SLOTS];
    uint8_t order[PAGE_COUNT];      /* the knob pages in the order FX goes through them */
    uint8_t page_off;               /* the pages hidden from FX (bit = page) */
} control_map_t;
typedef struct {
    uint32_t magic;
    uint8_t target[MAP_V1_SLOTS];
    uint8_t arg[MAP_V1_SLOTS];
} control_map_v1_t;
static const uint8_t PAGE_ORDER_DEFAULT[PAGE_COUNT] = {
    PAGE_SOUND, PAGE_MACRO, PAGE_LIVE, PAGE_SETUP, PAGE_USER, PAGE_EQ, PAGE_DRUM
};

static control_map_t cmap;
static float map_value[MAP_ALL_SLOTS];  /* where each control last put its target, 0..1 */
enum { SCREEN_SOUNDS, SCREEN_MAP, SCREEN_OPTION, SCREENS };   /* the set-up screens, in the MENU row's turn */
static struct {
    uint8_t open, screen, row, top, dirty, conf_dirty, drawn;
    uint32_t buttons;
    int8_t go;                                  /* the MENU row moved: -1 the screen before, 1 the one after */
} mapui;

#ifndef APP_PERC_FIRST
#define APP_PERC_FIRST 1u           /* the FM-1: G#3 D#4 G#4 D#5 (PERC_KEYS 1..4) */
#define APP_PERC_COUNT 4u
#endif

/* OPTION CONTROL's rows */
enum { CONF_PLAY_MODE, CONF_DRUM_MAP, CONF_PERC, CONF_KEYS_SCREEN = CONF_PERC + APP_PERC_COUNT, CONF_HELP, CONF_NEW_PAGE,
       CONF_MENU = CONF_NEW_PAGE + TRACKS + 1, CONF_ROWS };
/* the menu buttons: one pressed leaves a set-up screen (what it changed is kept) */
#define MENU_BUTTONS ((1u << B_HOME) | (1u << B_GLO) | (1u << BTN_PAGES) | (1u << B_ARP) | (1u << BTN_PERFORM) \
                      | (1u << B_SAVE))
#define MAP_VISIBLE 9u

/* MAP CONTROL's rows: the pages, then the slots (MACRO, LIVE, EQ, USER, the target's controls), then MENU */
static const uint8_t SLOT_ROW[] = {
    MAP_MACRO1, MAP_MACRO1 + 1, MAP_MACRO1 + 2, MAP_MACRO1 + 3, MAP_LIVE1, MAP_LIVE1 + 1, MAP_LIVE1 + 2, MAP_LIVE1 + 3,
    MAP_EQ1, MAP_EQ1 + 1, MAP_EQ1 + 2, MAP_EQ1 + 3, MAP_K1, MAP_K2, MAP_K3, MAP_K4,
    MAP_F1, MAP_F2, MAP_F3, MAP_F4, MAP_PITCH, MAP_MOD,
};
#define MAP_SLOT_ROWS (12u + APP_MAP_SLOTS)
#define MAP_MENU_ROW (PAGE_COUNT + MAP_SLOT_ROWS)

static uint32_t menu_rows(void)
{
    return mapui.screen == SCREEN_MAP ? MAP_MENU_ROW + 1u : (uint32_t)CONF_ROWS;
}

static int menu_on_last(void)
{
    return mapui.row + 1u == menu_rows();
}

/* a key as the screen keyboard draws it: violet (the one in use), else half filled in mint with a mint outline */
static void key_chip(int32_t x, int32_t y, int32_t w, int32_t h, int on)
{
    if (on) {
        cv_rect(x, y, w, h, C_VIO);
        return;
    }
    cv_rect(x, y, w, h, RGB(18, 18, 22));
    cv_rect(x, y + h / 2, w, h - h / 2, C_MINT);
    cv_rect(x, y, w, 1, C_MINT);
    cv_rect(x, y, 1, h, C_MINT);
    cv_rect(x + w - 1, y, 1, h, C_MINT);
}

/* "(arrow down) MENU: OUT" at the right of a help line: press a menu button to leave (the arrow: press, as the
 * arrow up of the layers says let go) */
static void help_menu_out(int32_t y, uint16_t c)
{
    int32_t x = 234 - text_w(&FONT_XS, "MENU: OUT") - 10;
    cv_rect(x + 2, y + 2, 3, 4, c);                 /* the shaft, then the head */
    for (int32_t i = 0; i < 4; i++) {
        cv_rect(x + i, y + 6 + i, 7 - 2 * i, 1, c);
    }
    cv_text(x + 10, y, &FONT_XS, "MENU: OUT", c);
}

static const char *const NEW_PAGE_LINE[TRACKS + 1] = { "T1 NEW SOUND", "T2 NEW SOUND", "T3 NEW SOUND",
                                                         "T4 NEW SOUND", "A NEW SOUND" };

/* a PERC row's key: "PERC 1 G#3" */
static void perc_row_name(uint32_t i, char *t)
{
    uint32_t j = APP_PERC_FIRST + i;
    copy_text(t, "PERC ", 16);
    int_text((int32_t)(i + 1u), t + 5);
    text_cat(t, (j & 1u) ? " G#" : " D#");
    char o[4];
    int_text((int32_t)(3u + j / 2u), o);
    text_cat(t, o);
}

static void conf_name(uint32_t c, char *t)
{
    if (c >= CONF_PERC && c < CONF_PERC + APP_PERC_COUNT) {
        perc_row_name(c - CONF_PERC, t);
    } else if (c >= CONF_NEW_PAGE && c < CONF_MENU) {
        copy_text(t, NEW_PAGE_LINE[c - CONF_NEW_PAGE], 16);
    } else {
        copy_text(t, c == CONF_PLAY_MODE ? "PLAY MODE" : (c == CONF_DRUM_MAP ? "DRUM KEYS"
                     : (c == CONF_KEYS_SCREEN ? "KEYS ON SCREEN" : (c == CONF_HELP ? "HELP AT START" : "MENU"))), 16);
    }
}

static const char *conf_text(uint32_t c)
{
    if (c >= CONF_PERC && c < CONF_PERC + APP_PERC_COUNT) {
        return DRUM_PERC[set.perc[APP_PERC_FIRST + c - CONF_PERC] % DRUM_PERCS].name;
    }
    if (c >= CONF_NEW_PAGE && c < CONF_MENU) {
        uint8_t p = set.new_page[c - CONF_NEW_PAGE];
        return p ? PAGE_NAME[(p - 1u) % PAGE_COUNT] : "SAME PAGE";
    }
    if (c == CONF_PLAY_MODE) {                  /* LIVE: the sounds inside; FULL MIDI: the keys and pads only send MIDI */
        return set.controller ? "FULL MIDI" : "LIVE";
    }
    if (c == CONF_DRUM_MAP) {
        return set.drum_map == DRUM_MAP_GM ? "GM" : "ORIGINAL";
    }
    if (c == CONF_HELP) {
        return set.no_help ? "OFF" : "ON";
    }
    return c == CONF_KEYS_SCREEN ? (set.keys_screen ? "ON" : "OFF") : "";
}

static void conf_turn(uint32_t c, int32_t turn)
{
    int32_t step = turn > 0 ? 1 : -1;
    if (c >= CONF_PERC && c < CONF_PERC + APP_PERC_COUNT) {
        uint8_t *p = &set.perc[APP_PERC_FIRST + c - CONF_PERC];
        *p = (uint8_t)((*p + DRUM_PERCS + (uint32_t)(step + (int32_t)DRUM_PERCS)) % DRUM_PERCS);
    } else if (c >= CONF_NEW_PAGE && c < CONF_MENU) {
        uint8_t *p = &set.new_page[c - CONF_NEW_PAGE];
        *p = (uint8_t)((*p + PAGE_COUNT + 1u + (uint32_t)step) % (PAGE_COUNT + 1u));   /* KEEP, SOUND .. EQ */
    } else if (c == CONF_PLAY_MODE) {
        set.controller = (uint8_t)!set.controller;
        mark_dirty();
    } else if (c == CONF_DRUM_MAP) {
        set.drum_map = (uint8_t)!set.drum_map;
    } else if (c == CONF_KEYS_SCREEN) {
        set.keys_screen = (uint8_t)!set.keys_screen;
    } else if (c == CONF_HELP) {
        set.no_help = (uint8_t)!set.no_help;
    } else {
        return;
    }
    mapui.conf_dirty = 1;
}

/* EDIT on an OPTION row: its default */
static void conf_default(uint32_t c)
{
    if (c >= CONF_PERC && c < CONF_PERC + APP_PERC_COUNT) {
        uint32_t j = APP_PERC_FIRST + c - CONF_PERC;
        set.perc[j] = (j & 1u) ? 2u : 0u;           /* G#: TRIANGLE, D#: COWBELL */
    } else if (c >= CONF_NEW_PAGE && c < CONF_MENU) {
        set.new_page[c - CONF_NEW_PAGE] = 0;
    } else if (c == CONF_PLAY_MODE) {
        set.controller = 0;                     /* LIVE */
        mark_dirty();
    } else if (c == CONF_DRUM_MAP) {
        set.drum_map = DRUM_MAP_ORIGINAL;
    } else if (c == CONF_KEYS_SCREEN) {
        set.keys_screen = APP_KEYS_SCREEN;
    } else if (c == CONF_HELP) {
        set.no_help = 0;                        /* ON */
    } else {
        return;
    }
    mapui.conf_dirty = 1;
}

/* each slot's default: USER the selected track's effects, the faders effects on the mix, the wheels bend and Morph,
 * MACRO the four macros, LIVE MORPH SWING PUMP LEVEL, EQ BASS MID TREBLE MID FREQ */
static const uint8_t SLOT_TARGET[MAP_ALL_SLOTS] = {
    MAP_FILTER, MAP_RESO, MAP_DELAY, MAP_CHORUS, MAP_FILTER, MAP_DELAY, MAP_DRIVE, MAP_CRUSH, MAP_BEND, MAP_MORPH,
    MAP_ENERGY, MAP_SPACE, MAP_TENSION, MAP_DENSITY, MAP_MORPH, MAP_SWING, MAP_PUMP, MAP_LEVEL,
    MAP_EQ_BASS, MAP_EQ_MID, MAP_EQ_TREBLE, MAP_EQ_FREQ,
};
static const uint8_t SLOT_ARG[MAP_ALL_SLOTS] = {
    SCOPE_SELECTED, SCOPE_SELECTED, SCOPE_SELECTED, SCOPE_SELECTED, SCOPE_ALL, SCOPE_ALL, SCOPE_ALL, SCOPE_ALL, 0, 0,
};

static void slot_default(uint32_t s)
{
    cmap.target[s] = SLOT_TARGET[s];
    cmap.arg[s] = SLOT_ARG[s];
    map_value[s] = SLOT_TARGET[s] == MAP_BEND ? 0.5f : 0.0f;
}

static void pages_default(void)
{
    for (uint32_t i = 0; i < PAGE_COUNT; i++) {
        cmap.order[i] = PAGE_ORDER_DEFAULT[i];
    }
    cmap.page_off = 0;
}

static void map_defaults(void)
{
    cmap.magic = MAP_MAGIC;
    for (uint32_t s = 0; s < MAP_ALL_SLOTS; s++) {
        slot_default(s);
    }
    pages_default();
}

/* the page FX goes to after `p`: MAP CONTROL's order, the hidden pages (and DRUM EQ off a drum line) skipped */
static uint32_t page_after(uint32_t p)
{
    uint32_t at = 0;
    while (at < PAGE_COUNT && cmap.order[at] != p) {
        at++;
    }
    for (uint32_t i = 1; i <= PAGE_COUNT; i++) {
        uint32_t q = cmap.order[(at + i) % PAGE_COUNT];
        if (!((cmap.page_off >> q) & 1u) && (q != PAGE_DRUM || drum_line())) {
            return q;
        }
    }
    return PAGE_SOUND;
}

/* the slot as it was at first (a page's own knobs: its own behaviour, A's level and all) */
static int slot_is_default(uint32_t s)
{
    return cmap.target[s] == SLOT_TARGET[s] && cmap.arg[s] == SLOT_ARG[s];
}

/* the slot a page's knob k turns, or -1 (SOUND, SETUP, DRUM EQ: their own) */
static int32_t page_slot(uint32_t page, uint32_t k)
{
    switch (page) {
    case PAGE_MACRO: return (int32_t)(MAP_MACRO1 + k);
    case PAGE_LIVE: return (int32_t)(MAP_LIVE1 + k);
    case PAGE_EQ: return (int32_t)(MAP_EQ1 + k);
    case PAGE_USER: return (int32_t)(MAP_K1 + k);
    default: return -1;
    }
}

static int map_valid(int size)
{
    if (size != (int)sizeof cmap || cmap.magic != MAP_MAGIC || cmap.page_off & (1u << PAGE_SOUND)) {
        return 0;
    }
    for (uint32_t s = 0; s < MAP_ALL_SLOTS; s++) {
        if (cmap.target[s] >= MAP_TARGETS || cmap.arg[s] > 127u) {
            return 0;
        }
    }
    uint32_t seen = 0;
    for (uint32_t i = 0; i < PAGE_COUNT; i++) {
        seen |= cmap.order[i] < PAGE_COUNT ? 1u << cmap.order[i] : 0u;
    }
    return seen == (1u << PAGE_COUNT) - 1u;     /* every page once */
}

/* OLM1: USER's knobs and the target's controls as they were, the rest by default */
static int map_migrate_v1(void)
{
    static control_map_v1_t old;
    int size = plat_store_load(OBJ_MAP, &old, sizeof old);
    if (size != (int)sizeof old || old.magic != MAP_MAGIC_V1) {
        return 0;
    }
    map_defaults();
    for (uint32_t s = 0; s < MAP_V1_SLOTS; s++) {
        if (old.target[s] < MAP_TARGETS && old.arg[s] <= 127u) {
            cmap.target[s] = old.target[s];
            cmap.arg[s] = old.arg[s];
        }
    }
    return 1;
}

static void map_load(void)
{
    int size = plat_store_load(OBJ_MAP, &cmap, sizeof cmap);
    if (!map_valid(size) && !map_migrate_v1()) {
        map_defaults();
    }
    for (uint32_t s = 0; s < MAP_ALL_SLOTS; s++) {
        map_value[s] = cmap.target[s] == MAP_BEND ? 0.5f : 0.0f;
    }
}

static int map_save(void)
{
    static uint32_t stored;                     /* (what the flash holds: written only when it changed) */
    if (!store_if_changed(OBJ_MAP, &cmap, sizeof cmap, &stored)) {
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
    case MAP_EQ_BASS: case MAP_EQ_MID: case MAP_EQ_TREBLE: case MAP_EQ_FREQ:
        return (float)set.eq[cmap.target[slot] - MAP_EQ_BASS] * (1.0f / 255.0f);
    case MAP_PAN:
        return 0.5f + 0.5f * live.track[map_track(slot)].pan;
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
    if (slot >= APP_MAP_SLOTS && slot < MAP_MACRO1) {
        return;                                 /* a control this target does not have */
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
    case MAP_EQ_BASS: case MAP_EQ_MID: case MAP_EQ_TREBLE: {   /* the master EQ (the audio side follows) */
        set.eq[target - MAP_EQ_BASS] = (uint8_t)(v * 255.0f + 0.5f);
        int32_t db = (int32_t)((v - 0.5f) * 24.0f + (v >= 0.5f ? 0.5f : -0.5f));
        say_int(name, db, " DB");
        return;
    }
    case MAP_PAN: {                             /* the track in the stereo field: left .. centre .. right */
        uint8_t t = map_track(slot);
        float pan = (v - 0.5f) * 2.0f;
        pan = pan > -0.04f && pan < 0.04f ? 0.0f : pan;    /* a notch in the middle: the centre is easy to find */
        send_live(LIVE_PAN, 0, 0, pan, t);
        say_int(name, (int32_t)(pan * 100.0f + (pan >= 0.0f ? 0.5f : -0.5f)), pan < 0.0f ? " L" : (pan > 0.0f ? " R" : " C"));
        return;
    }
    case MAP_EQ_FREQ:
        set.eq[3] = (uint8_t)(v * 255.0f + 0.5f);
        say_int(name, (int32_t)(dsp_expmap(v, 250.0f, 4000.0f) + 0.5f), " HZ");
        return;
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

/* a mapped page's knob turned: from where its target is */
static void map_turn(uint32_t slot, int32_t turn)
{
    map_set(slot, map_current(slot) + (float)turn * ENCODER_STEP);
}

/* a mapped page's knob as drawn: the target's name and where it is */
static void map_knob_value(uint32_t k, float *bar, int32_t *shown, const char **label)
{
    uint8_t target = cmap.target[k];
    *label = MAP_TARGET[target].name;
    if (target == MAP_SOUND) {
        *label = track_api(mirror.track)->macro_names[(cmap.arg[k] - 1u) % KNOB_COUNT];
    }
    *bar = map_current(k);
    *shown = target == MAP_TEMPO ? (int32_t)(live.clock.bpm + 0.5f) : (int32_t)(*bar * 100.0f + 0.5f);
    if (target >= MAP_EQ_BASS && target <= MAP_EQ_TREBLE) {
        *shown = (int32_t)((*bar - 0.5f) * 24.0f + (*bar >= 0.5f ? 0.5f : -0.5f));     /* dB */
    } else if (target == MAP_EQ_FREQ) {
        *shown = (int32_t)(dsp_expmap(*bar, 250.0f, 4000.0f) + 0.5f);                 /* Hz */
    }
}

/* ------------------------------------------------------------ the menu */

static int map_is_open(void) { return mapui.open; }

static void fav_open(void);
static uint16_t colour_of_page(uint32_t page);

/* MAP CONTROL or OPTION CONTROL, on its first row */
static void map_open_screen(uint32_t screen)
{
    mapui.open = 1;
    mapui.screen = (uint8_t)screen;
    mapui.row = 0;
    mapui.top = 0;
    mapui.go = 0;
    mapui.drawn = 0;
    mapui.buttons = plat_buttons();             /* what is held now does not count as a press */
    lcd_fill(0, 0, 240, 240, C_BLACK);
}

static void map_open(void) { map_open_screen(SCREEN_MAP); }
static void map_open_settings(void) { map_open_screen(SCREEN_OPTION); }

static void map_close(void)
{
    mapui.open = 0;
    if (mapui.conf_dirty) {
        settings_keep();                        /* the settings: kept at once, past a power cycle */
        mapui.conf_dirty = 0;
    }
    if (mapui.dirty && !map_save()) {
        app_say("MAP CONTROL", "NOT SAVED");
    } else {
        app_say(mapui.screen == SCREEN_MAP ? "MAP CONTROL" : "OPTIONS", "KEPT");
    }
    ui.buttons = plat_buttons();                /* the app does not see the closing press again */
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = 1;
}

/* the set-up screen `dir` away from `from` (SOUNDS, MAP CONTROL, OPTION CONTROL in turn), on its MENU row, so the
 * same move comes straight back */
static void menu_go(uint32_t from, int32_t dir)
{
    uint32_t to = (from + SCREENS + (dir < 0 ? SCREENS - 1u : 1u)) % SCREENS;
    if (to == SCREEN_SOUNDS) {
        fav_open();
        fav.family = FAMILIES;                  /* (its MENU row) */
        return;
    }
    map_open_screen(to);
    mapui.row = (uint8_t)(menu_rows() - 1u);
}

static const char *const SCREEN_NAME[SCREENS] = { "SOUNDS", "MAP", "OPTIONS" };

/* MAP CONTROL's row of a slot (MAP_MENU_ROW: none on this target) */
static uint32_t map_row_of(uint32_t slot)
{
    for (uint32_t i = 0; i < MAP_SLOT_ROWS; i++) {
        if (SLOT_ROW[i] == slot) {
            return PAGE_COUNT + i;
        }
    }
    return MAP_MENU_ROW;
}

/* a mapped control moved while MAP CONTROL is open: its row is chosen */
static void map_touch(uint32_t slot)
{
    uint32_t row = map_row_of(slot);
    if (mapui.open && mapui.screen == SCREEN_MAP && row != MAP_MENU_ROW && mapui.row != row) {
        mapui.row = (uint8_t)row;
        mapui.drawn = 0;
    }
}

static void map_retarget(uint32_t s, int32_t t);
static void map_change_target(uint32_t s, int32_t turn)
{
    map_retarget(s, ((int32_t)cmap.target[s] + (turn > 0 ? 1 : -1) + MAP_TARGETS) % MAP_TARGETS);
}

/* slot s now drives target t (the old effect let go; a sensible option for the new kind). Also a target's own
 * gesture (the SMK-37's LIVE 2 pad that turns what the mod wheel does) */
static void map_retarget(uint32_t s, int32_t t)
{
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

static void map_change_arg(uint32_t s, int32_t turn)
{
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

/* a set-up screen's lights: the menu buttons (any closes) and PLAY dim (PLAY green when running), `lit` the ones that
 * act here (EDIT: a row's default; SEL, OCT: SOUNDS), the keys as they are played */
static void menu_lights(uint32_t lit)
{
    uint32_t dim = (MENU_BUTTONS | (1u << BTN_VALIDATE) | (1u << B_PLAY)) & ~lit;
    uint32_t play = 1u << B_PLAY;
    dim = (dim & ~play) | (live.clock.running ? 0u : LED_PLAY_GREEN);
    lit = (lit & ~play) | (live.clock.running ? LED_PLAY_GREEN : 0u);
    plat_play_red(live.recording);
    plat_lights(lit, ui.keys, dim, 0, 1);
}

/* the keys play while a menu page is open (the FM-1's go through the app's key scan, without a layer) */
static void menu_keys(void)
{
    touch_poll();
    uint32_t keys = touch_keys();
    scan_keys(keys, 0);
    ui.keys = keys;
}

/* a page row: PRESET moves the page one place up / down, ALGO shows / hides it (SOUND always shown) */
static void page_row_turn(uint32_t i, int32_t move, int32_t show)
{
    if (move) {
        int32_t j = (int32_t)i + (move > 0 ? 1 : -1);
        if (j >= 0 && j < (int32_t)PAGE_COUNT) {
            uint8_t x = cmap.order[i];
            cmap.order[i] = cmap.order[j];
            cmap.order[j] = x;
            mapui.row = (uint8_t)j;             /* the row goes with the page */
        }
    }
    if (show && cmap.order[mapui.row] != PAGE_SOUND) {
        cmap.page_off = (uint8_t)(cmap.page_off ^ (1u << cmap.order[mapui.row]));
    }
}

/* EDIT: the row back to its default (a page row: the pages' default order, all shown) */
static void map_row_default(uint32_t row)
{
    if (row < PAGE_COUNT) {
        pages_default();
    } else if (row < MAP_MENU_ROW) {
        uint32_t s = SLOT_ROW[row - PAGE_COUNT];
        if (cmap.target[s] >= MAP_FILTER && cmap.target[s] <= MAP_DECAY) {
            send_live(LIVE_OVERRIDE, 0, MAP_TARGET[cmap.target[s]].role, 0.0f, map_scope(s));   /* let it go */
        }
        slot_default(s);
    }
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
    if (pressed & (1u << B_PLAY)) {
        send_live(LIVE_PLAY_TOGGLE, 0, 0, 0.0f, 0);
    }
    menu_keys();
    menu_lights(1u << B_EDIT);
    int32_t turn = plat_enc(EN_SELECT);
    if (turn) {
        mapui.row = (uint8_t)clamp((int32_t)mapui.row + (turn > 0 ? 1 : -1), 0, (int32_t)menu_rows() - 1);
        mapui.drawn = 0;
    }
    turn = plat_enc(EN_PRESET);
    int32_t option = plat_enc(EN_ALGO);
    int32_t oct = (pressed >> B_OCTUP) & 1u ? 1 : ((pressed >> B_OCTDN) & 1u ? -1 : 0);
    for (uint32_t k = 0; k < KNOB_COUNT; k++) {
        if (plat_enc((int)(EN_K1 + k)) && page_slot(set.page, k) >= 0) {
            map_touch((uint32_t)page_slot(set.page, k));    /* a knob turned: its row */
        }
    }
    if (menu_on_last()) {                       /* MENU: PRESET, ALGO or OCT, the next / previous screen */
        int32_t go = turn + option + oct;
        if (go) {
            uint32_t from = mapui.screen;
            map_close();
            menu_go(from, go);
        }
        return;
    }
    if (pressed & (1u << B_EDIT)) {
        if (mapui.screen == SCREEN_MAP) {
            map_row_default(mapui.row);
            mapui.dirty = 1;
        } else {
            conf_default(mapui.row);
        }
        mapui.drawn = 0;
    }
    if (!(turn || option)) {
        return;
    }
    mapui.drawn = 0;
    if (mapui.screen == SCREEN_OPTION) {        /* an option: either knob changes it */
        conf_turn(mapui.row, turn + option);
        return;
    }
    mapui.dirty = 1;
    if (mapui.row < PAGE_COUNT) {
        page_row_turn(mapui.row, turn, option);
        return;
    }
    uint32_t s = SLOT_ROW[mapui.row - PAGE_COUNT];
    if (turn) {
        map_change_target(s, turn);
    }
    if (option) {
        map_change_arg(s, option);
    }
}

#define MAP_ROW_H 18
#define MAP_TOP 34

/* the MENU row: where PRESET / OCT go, before and after */
static void draw_menu_row(int32_t y, int sel, uint32_t screen)
{
    char t[32] = "< ";
    text_cat(t, SCREEN_NAME[(screen + SCREENS - 1u) % SCREENS]);
    text_cat(t, "   ");
    text_cat(t, SCREEN_NAME[(screen + 1u) % SCREENS]);
    text_cat(t, " >");
    uint16_t c = sel ? C_WHITE : C_GRAY;
    cv_rect(6, y + 4, 9, 2, c);                 /* an arrow: the next page */
    for (int32_t i = 0; i < 4; i++) {
        cv_rect(12 + i, y + 1 + i, 1, 8 - 2 * i, c);
    }
    cv_text(22, y, &FONT_XS, "NEXT PAGE", c);
    cv_text(234 - text_w(&FONT_XS, t), y, &FONT_XS, t, sel ? C_PINK : C_GRAY);
}

/* on the next-page row, the help says how to go: "<- PRESET ->" (the target's name) */
static void help_next_page(int32_t y, uint16_t c)
{
    char h[24] = "<- ";
    text_cat(h, APP_PRESET_NAME);
    text_cat(h, " ->");
    cv_text(6, y, &FONT_XS, h, c);
}

static void map_draw_row(uint32_t row, int32_t y, int sel)
{
    if (row == MAP_MENU_ROW) {
        draw_menu_row(y + 3, sel, SCREEN_MAP);
        return;
    }
    if (row < PAGE_COUNT) {                     /* a page: its place, its name in its colour, ON / OFF */
        char n[8] = "PAGE ";
        int_text((int32_t)(row + 1u), n + 5);
        uint32_t page = cmap.order[row];
        int off = (cmap.page_off >> page) & 1u;
        cv_text(6, y + 3, &FONT_XS, n, sel ? C_WHITE : C_GRAY);
        cv_text(70, y + 3, &FONT_XS, PAGE_NAME[page], off ? C_DIM : colour_of_page(page));
        cv_text(234 - text_w(&FONT_XS, off ? "OFF" : "ON"), y + 3, &FONT_XS, off ? "OFF" : "ON", off ? C_RED : C_GREEN);
        return;
    }
    uint32_t s = SLOT_ROW[row - PAGE_COUNT];
    char arg[12];
    map_arg_text(s, arg);
    const char *name = MAP_TARGET[cmap.target[s]].name;
    cv_text(6, y + 3, &FONT_XS, MAP_SLOT_NAME[s], sel ? C_WHITE : C_GRAY);
    cv_text(70, y + 3, &FONT_XS, name, slot_is_default(s) ? (sel ? C_WHITE : C_HI) : C_CYAN);   /* changed: cyan */
    cv_text(234 - text_w(&FONT_XS, arg), y + 3, &FONT_XS, arg, sel ? C_WHITE : C_AMB);
}

static void conf_draw_row(uint32_t c, int32_t y, int sel)
{
    if (c == CONF_MENU) {
        draw_menu_row(y + 3, sel, SCREEN_OPTION);
        return;
    }
    char name[16];
    conf_name(c, name);
    const char *v = conf_text(c);
    uint16_t vc = sel ? C_WHITE : C_AMB;
    if (c == CONF_PLAY_MODE) {
        vc = set.controller ? C_CYAN : C_MINT;              /* LIVE mint, FULL MIDI cyan */
    } else if (c == CONF_KEYS_SCREEN) {
        vc = set.keys_screen ? C_GREEN : C_RED;             /* ON green, OFF red */
    } else if (c == CONF_HELP) {
        vc = set.no_help ? C_RED : C_GREEN;
    } else if (c == CONF_DRUM_MAP) {                        /* the two FX keys, as the keyboard shows them */
        key_chip(124, y + 3, 9, 11, set.drum_map != DRUM_MAP_GM);
        key_chip(136, y + 3, 9, 11, set.drum_map == DRUM_MAP_GM);
    } else if (c >= CONF_NEW_PAGE) {
        uint8_t p = set.new_page[c - CONF_NEW_PAGE];
        vc = p ? colour_of_page(p - 1u) : C_GRAY;
    }
    cv_text(6, y + 3, &FONT_XS, name, sel ? C_WHITE : C_PINK);
    cv_text(234 - text_w(&FONT_XS, v), y + 3, &FONT_XS, v, vc);
}

static void map_draw(void)
{
    if (mapui.drawn) {
        return;
    }
    mapui.drawn = 1;
    int map = mapui.screen == SCREEN_MAP;
    cv_begin(240, MAP_TOP, C_BLACK);
    cv_text(8, 6, &FONT_B, map ? "MAP CONTROL" : "OPTION CONTROL", C_HI);
    if (map) {
        char v[32] = "OpenVALive ";                      /* the firmware and its version, in the logo's orange */
        app_version_text(v + 11, sizeof v - 11u);
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
            uint32_t row = mapui.top + i;
            if (row >= menu_rows()) {
                break;
            }
            int32_t y = (int32_t)(i - first) * MAP_ROW_H;
            int sel = row == mapui.row;
            if (sel) {
                cv_rect(0, y, 240, MAP_ROW_H - 1, C_LINE);
            }
            if (map) {
                map_draw_row(row, y, sel);
            } else {
                conf_draw_row(row, y, sel);
            }
        }
        scroll_bar((int32_t)(first * MAP_ROW_H), 3 * MAP_ROW_H, MAP_VISIBLE * MAP_ROW_H, mapui.top, MAP_VISIBLE,
                   menu_rows());                /* (there is more below / above) */
        cv_blit(0, MAP_TOP + first * MAP_ROW_H);
    }
    cv_begin(240, 36, C_BLACK);
    int on_page = map && mapui.row < PAGE_COUNT;
    char h[40];                                 /* the target's names for PRESET, ALGO, EDIT */
    copy_text(h, APP_PRESET_NAME, sizeof h);
    text_cat(h, map ? (on_page ? " move  " : " target  ") : " or ");
    text_cat(h, APP_ALGO_NAME);
    text_cat(h, map ? (on_page ? " on/off" : " option") : ": change");
    if (mapui.row + 1u == menu_rows()) {
        help_next_page(4, C_GRAY);              /* the last row: the next page */
    } else {
        cv_text(6, 4, &FONT_XS, h, C_GRAY);
    }
    copy_text(h, BTN_NAME[B_EDIT], sizeof h);
    text_cat(h, " default");
    cv_text(6, 20, &FONT_XS, h, C_GRAY);
    help_menu_out(20, C_GRAY);
    cv_blit(0, 240 - 36);
}
