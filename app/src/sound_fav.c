/* SPDX-License-Identifier: MIT
 * OpenVALive - SOUNDS: which sounds PRESET offers (HOME + white key 8, HOME let go: the screen stays, both hands
 * free). A hidden sound still plays where it is: only PRESET and the family jumps (OCT- / OCT+) leave it out.
 *   on top the chosen sound's place (3/5), its family and ON / HIDDEN; then one line a family, F1 .. F9, its
 *   label and name green (all offered), blue (some), red (none), a square a sound, the chosen line its sound's name
 *   SELECT       the line above / below (the last line: MENU)       PRESET  the sound before / after on the line
 *   SEL          the sound offered (green) or hidden (red)           OCT- / OCT+  the whole family hidden / offered
 *   MENU line    PRESET or OCT: MAP CONTROL / OPTIONS               a menu button (HOME FX ARP SEQ GLO SAVE): back,
 *                                                                    what changed kept past a power cycle */

static const char *const FAMILY_NAME[FAMILIES] = { "ACID", "PSY", "BASSES", "DRUMS", "PUNCH", "SFX", "UNDER", "SOFT",
                                                   "USER" };
#define FAV_MENU_ROW FAMILIES                    /* the line under the families */

static int fav_is_open(void) { return fav.open; }

/* the sounds of family f (1..FAMILIES) in PRESET's order (the USER family: all 8 slots) */
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
    fav.buttons = plat_buttons();               /* what is held now does not count as a press */
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
    int on_menu = fav.family == FAV_MENU_ROW;
    uint32_t n = on_menu ? 0u : fav_list(fav.family + 1, list);
    uint32_t buttons = plat_buttons();
    uint32_t pressed = buttons & ~fav.buttons;
    fav.buttons = buttons;
    menu_keys();
    menu_lights((1u << BTN_VALIDATE) | (1u << B_OCTDN) | (1u << B_OCTUP));   /* SEL on / off, OCT the family */
    if (pressed & MENU_BUTTONS) {
        fav_close();
        return;
    }
    int32_t turn = plat_enc(EN_SELECT);
    if (turn) {
        fav.family = (uint8_t)clamp((int32_t)fav.family + (turn > 0 ? 1 : -1), 0, FAV_MENU_ROW);
        fav.drawn = 0;
        return;
    }
    turn = plat_enc(EN_PRESET);
    if (on_menu) {                              /* MENU: the next screen (MAP CONTROL) or the one before (OPTIONS) */
        int32_t go = turn + plat_enc(EN_ALGO) + ((pressed >> B_OCTUP) & 1u) - ((pressed >> B_OCTDN) & 1u);
        if (go) {
            fav_close();
            menu_go(SCREEN_SOUNDS, go);
        }
        return;
    }
    if (fav.row >= n) {
        fav.row = (uint8_t)(n ? n - 1u : 0u);
    }
    if (turn && n) {
        fav.row = (uint8_t)clamp((int32_t)fav.row + (turn > 0 ? 1 : -1), 0, (int32_t)n - 1);
        fav.drawn = 0;
    }
    if ((pressed & (1u << BTN_VALIDATE)) && fav.row < n) {
        fav_set(list[fav.row], !sound_hidden(list[fav.row]));
    }
    if (pressed & ((1u << B_OCTDN) | (1u << B_OCTUP))) {
        for (uint32_t i = 0; i < n; i++) {
            fav_set(list[i], (pressed >> B_OCTDN) & 1u);    /* OCT-: the family hidden, OCT+: offered */
        }
    }
    (void)plat_enc(EN_ALGO);
    for (uint32_t k = 0; k < KNOB_COUNT; k++) {
        (void)plat_enc((int)(EN_K1 + k));
    }
}

#define FAV_TOP 30
#define FAV_ROW 16
#define FAV_SQUARES 104                          /* where a line's squares start (the place line's name too) */

static uint16_t fav_colour(int f)               /* a family: green all offered, blue some, red none */
{
    int32_t list[OLS_SOUND_TOTAL];
    uint32_t n = fav_list(f + 1, list), shown = 0;
    for (uint32_t i = 0; i < n; i++) {
        shown += !sound_hidden(list[i]);
    }
    return shown == n ? C_GREEN : (shown == 0u ? C_RED : C_CYAN);
}

/* under the title: the chosen sound's place, its family's name and ON / HIDDEN (the MENU line: where it goes) */
static void fav_draw_place(void)
{
    int32_t list[OLS_SOUND_TOTAL];
    cv_begin(240, FAV_ROW, C_BLACK);
    if (fav.family == FAV_MENU_ROW) {
        char h[40];
        copy_text(h, APP_PRESET_NAME, sizeof h);
        text_cat(h, " OR OCT: THE NEXT SCREEN");
        cv_text(6, 2, &FONT_XS, h, C_HI);
    } else {
        uint32_t n = fav_list(fav.family + 1, list);
        if (n) {
            uint32_t at = fav.row < n ? fav.row : n - 1u;
            int hidden = sound_hidden(list[at]);
            uint16_t fc = fav_colour(fav.family);
            char pos[4] = { (char)('1' + at), '/', (char)('0' + n), 0 };   /* a family: 9 sounds at most */
            cv_text(26, 2, &FONT_XS, pos, fc);         /* under the family names */
            cv_text(FAV_SQUARES, 2, &FONT_XS, FAMILY_NAME[fav.family], fc);
            cv_text(232 - text_w(&FONT_XS, hidden ? "HIDDEN" : "ON"), 2, &FONT_XS, hidden ? "HIDDEN" : "ON",
                    hidden ? C_RED : C_GREEN);
        }
    }
    cv_rect(0, FAV_ROW - 1, 240, 1, C_LINE);
    cv_blit(0, FAV_TOP);
}

/* one family's line at y in the band being drawn */
static void fav_draw_family(int f, int32_t y, int sel)
{
    int32_t list[OLS_SOUND_TOTAL];
    uint32_t n = fav_list(f + 1, list);
    uint16_t lc = fav_colour(f);
    char ft[3] = "F1";
    ft[1] = (char)('1' + f);
    cv_text(6, y + 2, &FONT_XS, ft, lc);
    cv_text(7, y + 2, &FONT_XS, ft, lc);
    if (sel && n) {                                     /* the chosen line: the chosen sound's name */
        int32_t s = list[fav.row < n ? fav.row : n - 1u];
        cv_text(26, y + 2, &FONT_XS, ols_sound_name((uint32_t)s), sound_hidden(s) ? C_RED : C_WHITE);
    } else {
        cv_text(26, y + 2, &FONT_XS, FAMILY_NAME[f], lc);
    }
    for (uint32_t i = 0; i < n; i++) {                  /* a square a sound, the chosen one framed */
        int32_t x = FAV_SQUARES + (int32_t)i * 14;
        int empty = list[i] >= OLS_SOUND_FIRST_USER && !user_sounds[list[i] - OLS_SOUND_FIRST_USER].valid;
        if (sel && i == fav.row) {
            cv_rect(x - 2, y + 1, 14, 14, C_WHITE);
        }
        cv_rect(x, y + 3, 10, 10, sound_hidden(list[i]) ? C_RED : (empty ? C_DIM : C_GREEN));
    }
}

static void fav_draw(void)
{
    if (fav.drawn) {
        return;
    }
    fav.drawn = 1;
    cv_begin(240, FAV_TOP, C_BLACK);
    cv_text(8, 5, &FONT_B, "SOUNDS", C_RED);
    cv_text(232 - text_w(&FONT_XS, "IN PRESET"), 10, &FONT_XS, "IN PRESET", C_GRAY);
    cv_rect(0, FAV_TOP - 2, 240, 1, C_LINE);
    cv_blit(0, 0);
    fav_draw_place();

    /* the families, then the MENU line, two lines a band */
    int32_t top = FAV_TOP + FAV_ROW;
    for (int band = 0; band <= FAV_MENU_ROW; band += 2) {
        cv_begin(240, 2 * FAV_ROW, C_BLACK);
        for (int f = band; f < band + 2 && f <= FAV_MENU_ROW; f++) {
            int32_t y = (f - band) * FAV_ROW;
            int sel = f == fav.family;
            if (sel) {
                cv_rect(0, y, 240, FAV_ROW - 1, C_VIO_BG);
            }
            if (f == FAV_MENU_ROW) {
                draw_menu_row(y + 2, sel, SCREEN_SOUNDS);
            } else {
                fav_draw_family(f, y, sel);
            }
        }
        cv_blit(0, (uint32_t)(top + band * FAV_ROW));
    }

    /* what the buttons do here: SEL ON (green) / OFF (red), OCT + (green) / - (red), a menu button to leave */
    int32_t y0 = top + (FAV_MENU_ROW + 1) * FAV_ROW;
    cv_begin(240, 240 - y0, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    char sel[12];
    copy_text(sel, BTN_NAME[B_SEL], sizeof sel);
    text_cat(sel, " ");
    int32_t ty = (240 - y0) / 2 - 4, x = cv_text(6, ty, &FONT_XS, sel, C_GRAY);
    x = cv_text(x, ty, &FONT_XS, "ON", C_GREEN);
    x = cv_text(x, ty, &FONT_XS, "/", C_GRAY);
    cv_text(x, ty, &FONT_XS, "OFF", C_RED);
    x = cv_text(120 - text_w(&FONT_XS, "OCT -/+") / 2, ty, &FONT_XS, "OCT ", C_GRAY);   /* in the middle, as the
                                                 * buttons sit: OCT- left (hide), OCT+ right (offer) */
    x = cv_text(x, ty, &FONT_XS, "-", C_RED);
    x = cv_text(x, ty, &FONT_XS, "/", C_GRAY);
    cv_text(x, ty, &FONT_XS, "+", C_GREEN);
    help_menu_out(ty, C_GRAY);
    cv_blit(0, (uint32_t)y0);
}
