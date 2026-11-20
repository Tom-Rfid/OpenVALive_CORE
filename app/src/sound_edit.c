/*
 * OpenVALive - FM-1 app: the SOUND EDIT page, every parameter of the selected track's sound.
 *
 * ARP + SEL opens it (the SMK-37: SHIFT + SELECT too). SELECT chooses the parameter, PRESET changes it (a step of
 * 1/32, or the next choice), ALGO finely (1/256). The keys still play while it is open, so the sound is heard as
 * it changes. A menu button (HOME FX ARP SEQ GLO SAVE) or SEL closes it. The changes go to the sound like a knob's
 * (undo, scenes, SAVE).
 *
 * For the VA, the choices have names (waveforms, filter modes, LFO lengths, the modulation matrix's sources and
 * destinations); every other parameter shows 0..100, the bipolar ones -100..+100.
 */

static struct {
    uint8_t open, drawn;
    uint16_t row, top;
    uint32_t buttons;
} sedit;

static const char *const WAVE_NAMES[] = { "SAW", "SQUARE", "TRI" };
static const char *const FILTER_NAMES[] = { "LP", "BP", "HP", "NOTCH" };
static const char *const LFO_NAMES[] = { "SINE", "TRI", "SQUARE", "S&H" };
static const char *const CHORUS_NAMES[] = { "OFF", "I", "II", "I+II" };
static const char *const MONO_NAMES[] = { "POLY", "MONO" };

static int sedit_is_open(void) { return sedit.open; }

static int sedit_is_va(void) { return track_api(mirror.track) == &ENGINE_VA_API; }

/* the named choices of a VA parameter (count 0: a continuous one) */
static int va_choices(uint16_t id, const char *const **names)
{
    *names = 0;
    switch (id) {
    case VA_P_OSC1_WAVE: case VA_P_OSC2_WAVE: *names = WAVE_NAMES; return OSC_WAVE_COUNT;
    case VA_P_FILTER_MODE:                     *names = FILTER_NAMES; return SVF_MODE_COUNT;
    case VA_P_LFO1_WAVE: case VA_P_LFO2_WAVE: *names = LFO_NAMES; return LFO_WAVE_COUNT;
    case VA_P_CHORUS_MODE:                     *names = CHORUS_NAMES; return CHORUS_MODE_COUNT;
    case VA_P_MONO:                            *names = MONO_NAMES; return 2;
    case VA_P_OSC2_COARSE:                     return 49;                   /* -24 .. +24 semitones */
    case VA_P_LFO1_SYNC: case VA_P_LFO2_SYNC: *names = VA_LFO_SYNC_NAMES; return VA_LFO_SYNCS;
    case VA_P_LFO1_TRIG: case VA_P_LFO2_TRIG: *names = VA_LFO_TRIG_NAMES; return VA_TRIG_COUNT;
    case VA_P_MOD1_SRC: case VA_P_MOD2_SRC: case VA_P_MOD3_SRC: case VA_P_MOD4_SRC:
        *names = VA_MOD_SRC_NAMES;
        return VA_SRC_COUNT;
    case VA_P_MOD1_DST: case VA_P_MOD2_DST: case VA_P_MOD3_DST: case VA_P_MOD4_DST:
        *names = VA_MOD_DST_NAMES;
        return VA_DST_COUNT;
    default:
        return 0;
    }
}

static int va_bipolar(uint16_t id)
{
    return id == VA_P_FENV_AMT || id == VA_P_OSC2_FINE || id == VA_P_MOD1_AMT || id == VA_P_MOD2_AMT
        || id == VA_P_MOD3_AMT || id == VA_P_MOD4_AMT;
}

static int sedit_choice_index(float v, int count)
{
    int i = (int)(v * (float)count);
    return i >= count ? count - 1 : i;
}

/* the value as text: a choice's name, semitones, or a percentage */
static void sedit_value_text(uint16_t id, float v, char *t)
{
    const char *const *names;
    int count = sedit_is_va() ? va_choices(id, &names) : 0;
    if (count && names) {
        copy_text(t, names[sedit_choice_index(v, count)], 12);
        return;
    }
    int32_t n;
    if (count == 49) {
        n = sedit_choice_index(v, count) - 24;          /* semitones */
    } else if (sedit_is_va() && va_bipolar(id)) {
        n = (int32_t)(v * 200.0f + 0.5f) - 100;
    } else {
        n = (int32_t)(v * 100.0f + 0.5f);
    }
    if (n > 0 && ((sedit_is_va() && va_bipolar(id)) || count == 49)) {
        t[0] = '+';
        int_text(n, t + 1);
    } else {
        int_text(n, t);
    }
}

static void sedit_open(void)
{
    sedit.open = 1;
    sedit.drawn = 0;
    sedit.buttons = plat_buttons();
    resync_shadow();
    lcd_fill(0, 0, 240, 240, C_BLACK);
}

static void sedit_close(void)
{
    sedit.open = 0;
    ui.buttons = plat_buttons();
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = 1;
}

static void sedit_change(int32_t turn, int fine)
{
    uint8_t t = mirror.track;
    uint16_t id = sedit.row;
    float v = ui.shadow[t][id];
    const char *const *names;
    int count = sedit_is_va() ? va_choices(id, &names) : 0;
    if (count && !fine) {
        int i = clamp(sedit_choice_index(v, count) + (turn > 0 ? 1 : -1), 0, count - 1);
        v = ((float)i + 0.5f) / (float)count;
    } else {
        v = fm_clampf(v + (float)turn * (fine ? 1.0f / 256.0f : 1.0f / 32.0f), 0.0f, 1.0f);
    }
    ui.shadow[t][id] = v;
    send_param(t, id, v);
    mark_dirty();
    sedit.drawn = 0;
}

static void sedit_input(void)
{
    uint32_t buttons = plat_buttons();
    uint32_t pressed = buttons & ~sedit.buttons;
    sedit.buttons = buttons;
    if (pressed & (MENU_BUTTONS | (1u << BTN_VALIDATE))) {    /* any menu button, or SEL: back */
        sedit_close();
        return;
    }
    menu_lights(0);
    if (pressed & (1u << B_PLAY)) {
        send_live(LIVE_PLAY_TOGGLE, 0, 0, 0.0f, 0);
    }
    menu_keys();
    uint16_t count = track_api(mirror.track)->param_count;
    if (count > OLS_MAX_PARAMS) {
        count = OLS_MAX_PARAMS;
    }
    int32_t turn = plat_enc(EN_SELECT);
    if (turn) {
        sedit.row = (uint16_t)clamp((int32_t)sedit.row + (turn > 0 ? 1 : -1), 0, (int32_t)count - 1);
        sedit.drawn = 0;
    }
    if (sedit.row >= count) {
        sedit.row = 0;
    }
    turn = plat_enc(EN_PRESET);
    if (turn) {
        sedit_change(turn, 0);
    }
    turn = plat_enc(EN_ALGO);
    if (turn) {
        sedit_change(turn, 1);
    }
    for (uint32_t k = 0; k < KNOB_COUNT; k++) {
        (void)plat_enc((int)(EN_K1 + k));      /* (the knobs wait) */
    }
}

#define SEDIT_ROWS 9
#define SEDIT_ROW_H 18
#define SEDIT_TOP 34

static void sedit_draw(void)
{
    if (sedit.drawn) {
        return;
    }
    sedit.drawn = 1;
    const ols_engine_api_t *api = track_api(mirror.track);
    uint16_t count = api->param_count < OLS_MAX_PARAMS ? api->param_count : (uint16_t)OLS_MAX_PARAMS;
    if (sedit.row < sedit.top) {
        sedit.top = sedit.row;
    } else if (sedit.row >= sedit.top + SEDIT_ROWS) {
        sedit.top = (uint16_t)(sedit.row - SEDIT_ROWS + 1);
    }

    char title[16] = "EDIT  T";
    int_text(mirror.track + 1, title + 7);
    cv_begin(240, SEDIT_TOP, C_BLACK);
    cv_text(8, 6, &FONT_B, title, C_HI);
    cv_text(232 - text_w(&FONT_XS, sound_name(set.preset[mirror.track])), 12, &FONT_XS,
            sound_name(set.preset[mirror.track]), C_GRAY);
    cv_rect(0, SEDIT_TOP - 2, 240, 1, C_LINE);
    cv_blit(0, 0);

    for (uint32_t band = 0; band < SEDIT_ROWS; band += 3u) {
        cv_begin(240, 3 * SEDIT_ROW_H, C_BLACK);
        for (uint32_t r = band; r < band + 3u && r < SEDIT_ROWS; r++) {
            uint16_t id = (uint16_t)(sedit.top + r);
            if (id >= count) {
                continue;
            }
            int32_t y = (int32_t)(r - band) * SEDIT_ROW_H;
            int sel = id == sedit.row;
            float v = ui.shadow[mirror.track][id];
            if (sel) {
                cv_rect(0, y, 240, SEDIT_ROW_H - 1, C_LINE);
                cv_rect(100, y + SEDIT_ROW_H - 4, (int32_t)(v * 70.0f), 2, C_HI);
            }
            char value[12];
            sedit_value_text(id, v, value);
            cv_text(6, y + 3, &FONT_XS, api->param_names ? api->param_names[id] : "?", sel ? C_WHITE : C_GRAY);
            cv_text(234 - text_w(&FONT_XS, value), y + 3, &FONT_XS, value, sel ? C_WHITE : C_HI);
        }
        scroll_bar((int32_t)(band * SEDIT_ROW_H), 3 * SEDIT_ROW_H, SEDIT_ROWS * SEDIT_ROW_H, sedit.top, SEDIT_ROWS, count);
        cv_blit(0, SEDIT_TOP + band * SEDIT_ROW_H);
    }
    cv_begin(240, 36, C_BLACK);
    char h[32] = "SELECT param  ";
    text_cat(h, APP_PRESET_NAME);
    text_cat(h, " value");
    cv_text(6, 4, &FONT_XS, h, C_GRAY);
    copy_text(h, APP_ALGO_NAME, sizeof h);
    text_cat(h, " fine");
    cv_text(6, 20, &FONT_XS, h, C_GRAY);
    help_menu_out(20, C_GRAY);
    cv_blit(0, 240 - 36);
}
