/* SPDX-License-Identifier: MIT
 * OpenVALive - SOUNDS: which sounds PRESET offers, family by family (HOME + white key 14). A hidden sound still plays
 * where it is: only PRESET and the family jumps (OCT- / OCT+) leave it out.
 *   SELECT       the family above / below                   PRESET  the sound before / after in the family
 *   SEL          the sound offered (green) or hidden (red)   OCT- / OCT+   the whole family hidden / offered
 *   HOME         back (kept at once, past a power cycle) */

static const char *const FAMILY_NAME[FAMILIES] = { "ACID", "BASSES", "DRUMS", "LEADS & KEYS", "PADS", "TEXTURES",
                                                   "USER" };

static int fav_is_open(void) { return fav.open; }

/* the sounds of family f in PRESET's order (the USER family: all 8 slots) */
static uint32_t fav_list(int f, int32_t *out)
{
    uint32_t n = 0;
    for (int32_t pos = 0; pos < OLS_SOUND_TOTAL; pos++) {
        int32_t s = sound_at(pos);
        if (sound_family(s) == f) {
            out[n++] = s;
        }
    }
    return n;
}

static void fav_open(void)
{
    fav.open = 1;
    fav.drawn = 0;
    fav.family = (uint8_t)(sound_family(ui.on_a ? set.live_sound : set.preset[mirror.track]) - 1);
    fav.row = 0;
    fav.buttons = plat_buttons();
    lcd_fill(0, 0, 240, 240, C_BLACK);
}

static void fav_close(void)
{
    fav.open = 0;
    settings_keep();                            /* kept at once, past a power cycle (no SAVE needed) */
    ui.buttons = plat_buttons();
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = 1;
    app_say("SOUNDS", "KEPT");
}

static void fav_set(int32_t sound, int hide)
{
    uint8_t bit = (uint8_t)(1u << (sound & 7));
    set.hidden[sound >> 3] = (uint8_t)(hide ? (set.hidden[sound >> 3] | bit) : (set.hidden[sound >> 3] & ~bit));
    fav.drawn = 0;
}

static void fav_input(void)
{
    int32_t list[OLS_SOUND_TOTAL];
    uint32_t n = fav_list(fav.family + 1, list);
    uint32_t buttons = plat_buttons();
    uint32_t pressed = buttons & ~fav.buttons;
    fav.buttons = buttons;
    menu_keys();
    if (pressed & (1u << B_HOME)) {
        fav_close();
        return;
    }
    if (pressed & (1u << BTN_VALIDATE) && fav.row < n) {
        fav_set(list[fav.row], !sound_hidden(list[fav.row]));
    }
    if (pressed & ((1u << B_OCTDN) | (1u << B_OCTUP))) {
        for (uint32_t i = 0; i < n; i++) {
            fav_set(list[i], (pressed >> B_OCTDN) & 1u);    /* OCT-: the family hidden, OCT+: offered */
        }
    }
    int32_t turn = plat_enc(EN_SELECT);
    if (turn) {
        fav.family = (uint8_t)((fav.family + FAMILIES + (turn > 0 ? 1 : -1)) % FAMILIES);
        fav.row = 0;
        fav.drawn = 0;
    }
    turn = plat_enc(EN_PRESET);
    if (turn && n) {
        fav.row = (uint8_t)clamp((int32_t)fav.row + (turn > 0 ? 1 : -1), 0, (int32_t)n - 1);
        fav.drawn = 0;
    }
    (void)plat_enc(EN_ALGO);
    for (uint32_t k = 0; k < KNOB_COUNT; k++) {
        (void)plat_enc((int)(EN_K1 + k));
    }
}

#define FAV_TOP 34
#define FAV_ROW 18

static void fav_draw(void)
{
    if (fav.drawn) {
        return;
    }
    fav.drawn = 1;
    int32_t list[OLS_SOUND_TOTAL];
    uint32_t n = fav_list(fav.family + 1, list);
    char t[24] = "F1 ";
    t[1] = (char)('1' + fav.family);
    copy_text(t + 3, FAMILY_NAME[fav.family], sizeof t - 3u);
    cv_begin(240, FAV_TOP, C_BLACK);
    cv_text(8, 6, &FONT_B, "SOUNDS", C_RED);
    cv_text(232 - text_w(&FONT_XS, t), 12, &FONT_XS, t, C_HI);
    cv_rect(0, FAV_TOP - 2, 240, 1, C_LINE);
    cv_blit(0, 0);

    for (uint32_t band = 0; band < 9u; band += 3u) {      /* up to 9 sounds a family */
        cv_begin(240, 3 * FAV_ROW, C_BLACK);
        for (uint32_t r = band; r < band + 3u && r < n; r++) {
            int32_t y = (int32_t)(r - band) * FAV_ROW;
            int32_t s = list[r];
            int empty = s >= OLS_SOUND_FIRST_USER && !user_sounds[s - OLS_SOUND_FIRST_USER].valid;
            int hidden = sound_hidden(s);
            if (r == fav.row) {
                cv_rect(0, y, 240, FAV_ROW - 1, C_VIO_BG);
            }
            cv_rect(8, y + 5, 8, 8, hidden ? C_RED : C_GREEN);          /* offered: green, hidden: red */
            cv_text(24, y + 2, &FONT_XS, ols_sound_name((uint32_t)s), empty ? C_DIM : (hidden ? C_RED : C_HI));
            cv_text(232 - text_w(&FONT_XS, hidden ? "HIDDEN" : "ON"), y + 2, &FONT_XS, hidden ? "HIDDEN" : "ON",
                    hidden ? C_RED : C_GREEN);
        }
        cv_blit(0, FAV_TOP + band * FAV_ROW);
    }

    cv_begin(240, 240 - FAV_TOP - 9 * FAV_ROW, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    for (int f = 0; f < FAMILIES; f++) {                  /* the families, the one shown in white */
        char ft[3] = "F1";
        ft[1] = (char)('1' + f);
        cv_text(8 + f * 33, 6, &FONT_XS, ft, f == fav.family ? C_WHITE : C_GRAY);
    }
    cv_text(8, 26, &FONT_XS, "SEL ON/OFF  OCT- / OCT+ ALL  HOME", C_GRAY);
    cv_blit(0, FAV_TOP + 9 * FAV_ROW);
}
