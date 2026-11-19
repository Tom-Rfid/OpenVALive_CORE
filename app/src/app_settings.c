/*
 * OpenVALive - FM-1 app: what is kept in flash (fm1-base's store).
 *
 *   OBJ_SETTINGS     octave, knob page, presets shown per track (a few bytes)
 *   OBJ_PROJECT_A/B  the live set: every sound, loop and scene (core/live/project.c),
 *                    split in two objects because one holds at most 3840 bytes
 *
 * The project chunk A starts with the total size (2 bytes), so a loader
 * knows whether chunk B belongs to it.
 */

#define CHUNK_A_DATA (PLAT_STORE_MAX - 2u)
/* a full live set (every pattern full) must fit the two objects: more engine parameters need a third one */
typedef char project_fits_two_objects[(OLS_PROJECT_MAX <= CHUNK_A_DATA + PLAT_STORE_MAX) ? 1 : -1];

/* the settings OLSB added, from an older format: each D# / G# key as its D# / G# was, the EQ flat, the page kept */
static void settings_fill_new(void)
{
    for (uint32_t j = 0; j < PERC_KEYS; j++) {
        set.perc[j] = (j & 1u) ? set.drum_gs : set.drum_ds;
    }
    for (uint32_t b = 0; b < EQ_BANDS; b++) {
        set.eq[b] = 128u;
    }
    ols_zero(set.new_page, sizeof set.new_page);
}

static void conf_defaults(void)
{
    set.drum_map = DRUM_MAP_ORIGINAL;
    set.drum_ds = 0;                            /* COWBELL */
    set.drum_gs = 2;                            /* TRIANGLE */
    set.keys_screen = APP_KEYS_SCREEN;
    set.wheel_pan = 0;
    ols_zero(set.hidden, sizeof set.hidden);  /* every sound offered */
    settings_fill_new();
}

static void settings_defaults(void)
{
    set.magic = SET_MAGIC;
    for (int l = 0; l <= TRACKS; l++) {
        set.octave[l] = 0;
    }
    set.page = PAGE_SOUND;
    for (int t = 0; t < TRACKS; t++) {
        set.preset[t] = DEFAULT_SOUND[t];
    }
    set.controller = 0;
    set.midi_ch = 0;
    set.live_sound = LIVE_DEFAULT_SOUND;
    set.arp_mode = ARP_OFF;
    conf_defaults();
}

/* The MIDI parser's channel (a byte the audio side reads: one store, no tearing). */
static void apply_midi_channel(void)
{
    if (set.midi_ch == 0u) {
        midi.channel = MIDI_OMNI;
    } else if (set.midi_ch == MIDI_CH_CLOCK) {
        midi.channel = MIDI_NONE;
    } else {
        midi.channel = (uint8_t)(set.midi_ch - 1u);
    }
}

static int new_valid(void)
{
    for (uint32_t j = 0; j < PERC_KEYS; j++) {
        if (set.perc[j] >= DRUM_PERCS) {
            return 0;
        }
    }
    for (int l = 0; l <= TRACKS; l++) {
        if (set.new_page[l] > PAGE_COUNT) {
            return 0;
        }
    }
    return 1;
}

static int settings_valid(int loaded_size)
{
    if (loaded_size != (int)sizeof set || set.magic != SET_MAGIC || set.page >= PAGE_COUNT) {
        return 0;
    }
    for (int l = 0; l <= TRACKS; l++) {
        if (set.octave[l] < -3 || set.octave[l] > 3) {
            return 0;
        }
    }
    for (int t = 0; t < TRACKS; t++) {
        if (set.preset[t] >= OLS_SOUND_TOTAL) {
            return 0;
        }
    }
    return set.controller <= 1u && set.midi_ch <= MIDI_CH_CLOCK && set.arp_mode < ARP_MODES
           && live_sound_ok(set.live_sound) && set.drum_map <= DRUM_MAP_GM && set.drum_ds < DRUM_PERCS
           && set.drum_gs < DRUM_PERCS && set.keys_screen <= 1u && set.wheel_pan <= 1u && new_valid();
}

/* Before OLS8 the 17 synth sounds came first, the drum kits and USER sounds right after: 7 synth sounds were added
 * after INIT, so those move 7 up (every older format ends here) */
#define OLD_VA_SOUNDS 17u
/* the sounds numbered from `first` (the drum kits, the USER sounds) move up past the synth sounds added since */
static void sounds_from(uint32_t first)
{
    for (int t = 0; t < TRACKS; t++) {
        if (set.preset[t] >= first) {
            set.preset[t] = (uint8_t)(set.preset[t] + (VA_PRESET_COUNT - first));
        }
    }
    if (set.live_sound >= first && set.live_sound < 0xF0u) {
        set.live_sound = (uint8_t)(set.live_sound + (VA_PRESET_COUNT - first));
    }
}
static void sounds_v7(void) { sounds_from(OLD_VA_SOUNDS); }

/* the hidden sounds of an older format whose drum kits and USER sounds started at `first`: they move up with them */
static void hidden_from(const uint8_t *old, uint32_t old_bytes, uint32_t first)
{
    ols_zero(set.hidden, sizeof set.hidden);
    for (uint32_t s = 0; s < 8u * old_bytes; s++) {
        uint32_t now = s < first ? s : s + (VA_PRESET_COUNT - first);
        if (((old[s >> 3] >> (s & 7u)) & 1u) && now < OLS_SOUND_TOTAL) {
            set.hidden[now >> 3] = (uint8_t)(set.hidden[now >> 3] | (1u << (now & 7u)));
        }
    }
}

/* OLSB: 36 sounds (TALK .. SHAKER came after) */
static int settings_migrate_vb(void)
{
    static settings_vb_t old;
    int size = plat_store_load(OBJ_SETTINGS, &old, sizeof old);
    if (size != (int)sizeof old || old.magic != SET_MAGIC_VB) {
        return 0;
    }
    ols_zero(&set, sizeof set);
    for (uint32_t i = 0; i < sizeof(settings_v9_t); i++) {  /* the same up to `hidden` */
        ((uint8_t *)&set)[i] = ((const uint8_t *)&old)[i];
    }
    hidden_from(old.hidden, sizeof old.hidden, VA_SOUNDS_VB);
    for (uint32_t j = 0; j < PERC_KEYS; j++) {
        set.perc[j] = old.perc[j];
    }
    for (uint32_t b = 0; b < EQ_BANDS; b++) {
        set.eq[b] = old.eq[b];
    }
    for (int l = 0; l <= TRACKS; l++) {
        set.new_page[l] = old.new_page[l];
    }
    set.magic = SET_MAGIC;
    sounds_from(VA_SOUNDS_VB);
    return settings_valid((int)sizeof set);
}

/* OLSA: 31 sounds (THUNDER .. VINTAGE KEYS came after), its hidden sounds by those numbers too */
static int settings_migrate_va(void)
{
    static settings_va_t old;
    int size = plat_store_load(OBJ_SETTINGS, &old, sizeof old);
    if (size != (int)sizeof old || old.magic != SET_MAGIC_VA) {
        return 0;
    }
    ols_zero(&set, sizeof set);
    for (uint32_t i = 0; i < sizeof(settings_v9_t); i++) {  /* the same up to `hidden` */
        ((uint8_t *)&set)[i] = ((const uint8_t *)&old)[i];
    }
    hidden_from(old.hidden, sizeof old.hidden, VA_SOUNDS_VA);
    set.magic = SET_MAGIC;
    settings_fill_new();
    sounds_from(VA_SOUNDS_VA);
    return settings_valid((int)sizeof set);
}

/* OLS8: 24 synth sounds (STAB and POLY came after); its padding byte is now the wheel's mode */
/* OLS9: the same without `hidden` */
static int settings_migrate_v9(void)
{
    ols_zero(&set, sizeof set);
    int size = plat_store_load(OBJ_SETTINGS, &set, sizeof(settings_v9_t));
    if (size != (int)sizeof(settings_v9_t) || set.magic != SET_MAGIC_V9) {
        return 0;
    }
    set.magic = SET_MAGIC;
    settings_fill_new();
    sounds_from(VA_SOUNDS_VA);
    return settings_valid((int)sizeof set);
}

static int settings_migrate_v8(void)
{
    ols_zero(&set, sizeof set);
    int size = plat_store_load(OBJ_SETTINGS, &set, sizeof(settings_v9_t));
    if (size != (int)sizeof(settings_v9_t) || set.magic != SET_MAGIC_V8) {
        return 0;
    }
    set.magic = SET_MAGIC;
    set.wheel_pan = 0;
    settings_fill_new();
    sounds_from(24u);
    return settings_valid((int)sizeof set);
}

/* OLS7: the same settings */
static int settings_migrate_v7(void)
{
    ols_zero(&set, sizeof set);
    int size = plat_store_load(OBJ_SETTINGS, &set, sizeof(settings_v9_t));
    if (size != (int)sizeof(settings_v9_t) || set.magic != SET_MAGIC_V7) {
        return 0;
    }
    set.magic = SET_MAGIC;
    set.wheel_pan = 0;
    settings_fill_new();
    sounds_v7();
    return settings_valid((int)sizeof set);
}

/* OLS5 numbered the synth sounds INIT BASS PAD LEAD ACID STRINGS BRASS PLUCK ORGAN SUB SWEEP WIND HOLD WHALE
 * SCREECH GRUNGE DRONE: where each is now */
static uint8_t sound_v5(uint8_t sound)
{
    static const uint8_t NOW[OLD_VA_SOUNDS] = {
        VA_PRESET_INIT, VA_PRESET_BASS, VA_PRESET_PAD, VA_PRESET_LEAD, VA_PRESET_ACID, VA_PRESET_STRINGS,
        VA_PRESET_BRASS, VA_PRESET_PLUCK, VA_PRESET_ORGAN, VA_PRESET_SUB, VA_PRESET_SWEEP, VA_PRESET_WIND,
        VA_PRESET_HOLD, VA_PRESET_WHALE, VA_PRESET_SCREECH, VA_PRESET_GRUNGE, VA_PRESET_DRONE,
    };
    return sound < OLD_VA_SOUNDS ? NOW[sound] : sound;
}

/* OLS4 had no DRUMS 909 (now right after DRUMS 808): its sounds past DRUMS 808 move one up */
static uint8_t sound_v4(uint8_t sound)
{
    return sound_v5((uint8_t)(sound > OLD_VA_SOUNDS ? sound + 1u : sound));
}

/* OLS6: the same, the drum keys and the keys on screen at their defaults */
static int settings_migrate_v6(void)
{
    int size = plat_store_load(OBJ_SETTINGS, &set, sizeof(settings_v6_t));
    if (size != (int)sizeof(settings_v6_t) || set.magic != SET_MAGIC_V6) {
        return 0;
    }
    set.magic = SET_MAGIC;
    conf_defaults();
    sounds_v7();
    return settings_valid((int)sizeof set);
}

static int settings_migrate_v5(void)
{
    int size = plat_store_load(OBJ_SETTINGS, &set, sizeof(settings_v6_t));
    if (size != (int)sizeof(settings_v6_t) || set.magic != SET_MAGIC_V5) {
        return 0;
    }
    set.magic = SET_MAGIC;
    conf_defaults();
    for (int t = 0; t < TRACKS; t++) {
        set.preset[t] = sound_v5(set.preset[t]);
    }
    set.live_sound = sound_v5(set.live_sound);
    sounds_v7();
    return settings_valid((int)sizeof set);
}

static int settings_migrate_v4(void)
{
    int size = plat_store_load(OBJ_SETTINGS, &set, sizeof(settings_v6_t));
    if (size != (int)sizeof(settings_v6_t) || set.magic != SET_MAGIC_V4) {
        return 0;
    }
    set.magic = SET_MAGIC;
    conf_defaults();
    for (int t = 0; t < TRACKS; t++) {
        set.preset[t] = sound_v4(set.preset[t]);
    }
    set.live_sound = sound_v4(set.live_sound);
    sounds_v7();
    return settings_valid((int)sizeof set);
}

static void octave_all(int8_t octave)
{
    for (int l = 0; l <= TRACKS; l++) {
        set.octave[l] = (int8_t)(octave >= -3 && octave <= 3 ? octave : 0);
    }
}

/* OLS3: the same with one octave: every line gets it */
static int settings_migrate_v3(void)
{
    static settings_v3_t old;
    int size = plat_store_load(OBJ_SETTINGS, &old, sizeof old);
    if (size != (int)sizeof old || old.magic != SET_MAGIC_V3) {
        return 0;
    }
    settings_defaults();
    octave_all(old.octave);
    set.page = old.page;
    for (int t = 0; t < TRACKS; t++) {
        set.preset[t] = sound_v4(old.preset[t]);
    }
    set.controller = old.controller;
    set.midi_ch = old.midi_ch;
    set.live_sound = sound_v4(old.live_sound);
    set.arp_mode = old.arp_mode;
    sounds_v7();
    return settings_valid((int)sizeof set);
}

/* 0.0.3's settings: the same, A gets its default sound */
static int settings_migrate_v2(void)
{
    static settings_v2_t old;
    int size = plat_store_load(OBJ_SETTINGS, &old, sizeof old);
    if (size != (int)sizeof old || old.magic != SET_MAGIC_V2) {
        return 0;
    }
    settings_defaults();
    octave_all(old.octave);
    set.page = old.page;
    for (int t = 0; t < TRACKS; t++) {
        set.preset[t] = sound_v4(old.preset[t]);
    }
    set.controller = old.controller;
    set.midi_ch = old.midi_ch;
    sounds_v7();
    return settings_valid((int)sizeof set);
}

/* 0.1 kept one sound: it becomes track 1's sound, so an update keeps it. */
static int settings_migrate_v1(void)
{
    static settings_v1_t old;
    int size = plat_store_load(OBJ_SETTINGS, &old, sizeof old);
    if (size != (int)sizeof old || old.magic != SET_MAGIC_V1 || old.preset >= VA_PRESET_COUNT) {
        return 0;
    }
    settings_defaults();
    octave_all(old.octave);
    set.preset[0] = sound_v5(old.preset);
    for (uint32_t i = 0; i < 40u; i++) {               /* 0.1 had 40 parameters */
        engine_va_set_param(&va[0], (uint16_t)i, old.p[i]);
    }
    engine_va_snap(&va[0]);
    return 1;
}

/* ------------------------------------------------------------ project */

/* At boot (audio not running): reads the set into project_buf. It is applied by the audio side
 * once the UI is up (load_pending_project), unless HOME is held (safe start). */
static int project_load(void)
{
    int a = plat_store_load(OBJ_PROJECT_A, project_buf, PLAT_STORE_MAX);
    if (a < 3) {
        return 0;
    }
    uint32_t total = (uint32_t)project_buf[0] | (uint32_t)project_buf[1] << 8;
    if (total > OLS_PROJECT_MAX) {
        return 0;
    }
    if (total > CHUNK_A_DATA) {
        int b = plat_store_load(OBJ_PROJECT_B, project_buf + PLAT_STORE_MAX, OLS_PROJECT_MAX - CHUNK_A_DATA);
        if (b != (int)(total - CHUNK_A_DATA)) {
            return 0;
        }
    } else if ((uint32_t)a != total + 2u) {
        return 0;
    }
    /* the project starts 2 bytes in: everything after the size word is contiguous */
    project_size = total;
    project_pending = 2;                        /* 2: read, waiting for the UI's go (or HOME: safe start) */
    return 1;
}

/* 0.3 kept 48 parameters per user sound; 0.4 keeps 61 (the modulation matrix): an update keeps the sounds,
 * with the new parameters off (the matrix amounts at their middle: none) */
#define OLD_SOUND_PARAMS 48u
typedef struct {
    uint8_t valid, kind;
    uint16_t v[OLD_SOUND_PARAMS];
} user_sound_v03_t;

static int user_sounds_migrate(void)
{
    user_sound_v03_t *old = (user_sound_v03_t *)(void *)project_buf;     /* (free at boot) */
    int size = plat_store_load(OBJ_USER_SOUNDS, old, sizeof(user_sound_v03_t) * OLS_USER_SOUNDS);
    if (size != (int)(sizeof(user_sound_v03_t) * OLS_USER_SOUNDS)) {
        return 0;
    }
    ols_zero(user_sounds, sizeof user_sounds);
    for (int u = 0; u < OLS_USER_SOUNDS; u++) {
        user_sounds[u].valid = old[u].valid;
        user_sounds[u].kind = old[u].kind;
        for (uint32_t id = 0; id < OLD_SOUND_PARAMS; id++) {
            user_sounds[u].v[id] = old[u].v[id];
        }
        if (old[u].kind == OLS_KIND_VA) {
            user_sounds[u].v[VA_P_MOD1_AMT] = 32768u;
            user_sounds[u].v[VA_P_MOD2_AMT] = 32768u;
            user_sounds[u].v[VA_P_MOD3_AMT] = 32768u;
            user_sounds[u].v[VA_P_MOD4_AMT] = 32768u;
        }
    }
    return 1;
}

static void user_sounds_load(void)
{
    int size = plat_store_load(OBJ_USER_SOUNDS, user_sounds, sizeof user_sounds);
    if (size != (int)sizeof user_sounds && !user_sounds_migrate()) {
        ols_zero(user_sounds, sizeof user_sounds);
    }
    for (int u = 0; u < OLS_USER_SOUNDS; u++) {
        if (user_sounds[u].valid > 1u || user_sounds[u].kind >= OLS_KIND_COUNT) {
            user_sounds[u].valid = 0;           /* damaged: forget that one */
        }
    }
}

/* From the main loop while audio runs: the audio side may change the set
 * during the copy, so the copy is taken again until it is a quiet one. */
static uint32_t project_serialise(void)
{
    for (int attempt = 0; attempt < 4; attempt++) {
        uint32_t before = live.changes;
        uint32_t size = ols_project_save(&live, project_buf + 2, OLS_PROJECT_MAX);
        if (size > 0u && live.changes == before) {
            return size;
        }
    }
    return 0;
}

static int project_save(void)
{
    uint32_t size = project_serialise();
    if (size == 0u) {
        return 0;
    }
    project_buf[0] = (uint8_t)size;
    project_buf[1] = (uint8_t)(size >> 8);

    uint32_t in_a = size < CHUNK_A_DATA ? size : CHUNK_A_DATA;
    if (plat_store_save(OBJ_PROJECT_A, project_buf, in_a + 2u) != 0) {
        return 0;
    }
    if (size > CHUNK_A_DATA) {
        /* chunk B is the rest, which starts right after chunk A in the buffer */
        if (plat_store_save(OBJ_PROJECT_B, project_buf + PLAT_STORE_MAX, size - CHUNK_A_DATA) != 0) {
            return 0;
        }
    }
    return 1;
}

/* ------------------------------------------------------------ boot and SAVE */

/* At boot, before audio starts. */
static void settings_load(void)
{
    int size = plat_store_load(OBJ_SETTINGS, &set, sizeof set);
    if (!settings_valid(size) && !settings_migrate_vb() && !settings_migrate_va() && !settings_migrate_v9() && !settings_migrate_v8() && !settings_migrate_v7() && !settings_migrate_v6() && !settings_migrate_v5() && !settings_migrate_v4()
        && !settings_migrate_v3() && !settings_migrate_v2() && !settings_migrate_v1()) {
        settings_defaults();
    }
    user_sounds_load();
    map_load();
    project_load();
    saved_changes = live.changes;
}

/* A setting made to last (the drum keys, the keys on screen): kept at once, without SAVE */
static void settings_keep(void)
{
    (void)plat_store_save(OBJ_SETTINGS, &set, sizeof set);
}

/* SAVE: the settings and the whole live set as it is heard (MIDI changes included). */
static int settings_save(void)
{
    if (plat_store_save(OBJ_SETTINGS, &set, sizeof set) != 0 || !project_save()
        || plat_store_save(OBJ_USER_SOUNDS, user_sounds, sizeof user_sounds) != 0 || !map_save()) {
        return 0;
    }
    set_dirty = 0;
    saved_changes = live.changes;
    return 1;
}
