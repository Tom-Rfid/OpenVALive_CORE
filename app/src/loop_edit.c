/* SPDX-License-Identifier: MIT
 * OpenVALive - LOOP EDIT: the notes of the selected track's loop, one at a time (HOME + key 15).
 *   SELECT       the note before / after (it sounds)        a key   its pitch becomes that key's note
 *   K1 position (1/16)   K2 length   K3 velocity   K4 pitch (a semitone)
 *   REC          take the note out                          EDIT    a note one 1/16 after it
 *   a menu button or SEL: back (one UNDO step)   PLAY    play / stop, still editing
 * The Live Core does the change (LIVE_NOTE_EDIT): it may be playing. */

static int ledit_is_open(void) { return ledit.open; }

static uint32_t text_len(const char *s)
{
    uint32_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static const ols_pattern_t *ledit_pat(void) { return &live.track[mirror.track].pat; }

static void ledit_open(void)
{
    if (ui.on_a || !ledit_pat()->length) {
        app_say("LOOP EDIT", ui.on_a ? "PICK A TRACK" : "NO LOOP");
        return;
    }
    ledit.open = 1;
    ledit.drawn = 0;
    ledit.cursor = 0;
    ledit.want = 0;
    ledit.heard = 0;
    ledit.buttons = plat_buttons();
    send_live(LIVE_NOTE_EDIT, NOTE_EDIT_BEGIN, 0, 0.0f, mirror.track);   /* one undo step for it all */
    lcd_fill(0, 0, 240, 240, C_BLACK);
}

static void ledit_close(void)
{
    ledit.open = 0;
    ui.buttons = plat_buttons();
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = 1;
    app_say("LOOP EDIT", "UNDO: ENV");
}

static void ledit_send(uint8_t op, float v)
{
    const ols_pattern_t *p = ledit_pat();
    if (op != NOTE_EDIT_ADD && ledit.cursor >= p->count) {
        return;
    }
    if (op == NOTE_EDIT_TICK || op == NOTE_EDIT_ADD) {   /* the note moves in the list: found again by start and pitch */
        ledit.want = 1;
        ledit.want_tick = (uint16_t)(((int32_t)v % (int32_t)p->length + (int32_t)p->length) % (int32_t)p->length);
        ledit.want_note = op == NOTE_EDIT_ADD ? ledit.add_note : p->n[ledit.cursor].note;
    }
    send_live(LIVE_NOTE_EDIT, op, op == NOTE_EDIT_ADD ? ledit.add_note : ledit.cursor, v, mirror.track);
    ledit.drawn = 0;
}

static void ledit_hear(void)
{
    const ols_pattern_t *p = ledit_pat();
    if (ledit.cursor >= p->count) {
        return;
    }
    if (ledit.sounding) {
        send_note_off(mirror.track, ledit.sound_note);
    }
    ledit.quiet = 1;                                     /* (not a key: no pitch change) */
    ledit.sound_note = p->n[ledit.cursor].note;
    send_note_on(mirror.track, ledit.sound_note, p->n[ledit.cursor].vel);
    ledit.quiet = 0;
    ledit.sounding = 1;
    ledit.sound_off = plat_ms() + 180u;
}

static void ledit_input(void)
{
    const ols_pattern_t *p = ledit_pat();
    uint32_t buttons = plat_buttons();
    uint32_t pressed = buttons & ~ledit.buttons;
    ledit.buttons = buttons;
    if (ledit.sounding && (int32_t)(plat_ms() - ledit.sound_off) >= 0) {
        send_note_off(mirror.track, ledit.sound_note);
        ledit.sounding = 0;
    }
    if (pressed & (MENU_BUTTONS | (1u << BTN_VALIDATE))) {    /* any menu button, or SEL: back */
        ledit_close();
        return;
    }
    menu_lights((1u << B_REC) | (1u << B_EDIT));         /* REC takes the note out, EDIT adds one */
    if (pressed & (1u << B_PLAY)) {
        send_live(LIVE_PLAY_TOGGLE, 0, 0, 0.0f, 0);
    }
    if (ledit.want) {                                    /* the note moved: where it is now */
        for (uint32_t i = 0; i < p->count; i++) {
            if (p->n[i].tick == ledit.want_tick && p->n[i].note == ledit.want_note) {
                ledit.cursor = (uint8_t)i;
                ledit.want = 0;
                ledit.drawn = 0;
                break;
            }
        }
    }
    if (p->count && ledit.cursor >= p->count) {
        ledit.cursor = (uint8_t)(p->count - 1u);
        ledit.drawn = 0;
    }
    menu_keys();                                         /* keys play (and give the note its pitch) */
    if (ledit.heard) {
        ledit.heard = 0;
        if (ledit.cursor < p->count) {
            ledit_send(NOTE_EDIT_PITCH, (float)ledit.key);
        }
    }
    const ols_note_t *n = ledit.cursor < p->count ? &p->n[ledit.cursor] : 0;
    if (pressed & (1u << B_REC)) {
        ledit_send(NOTE_EDIT_DELETE, 0.0f);
    }
    if (pressed & (1u << B_EDIT)) {
        ledit.add_note = n ? n->note : (live.track[mirror.track].kind == OLS_KIND_DRUM ? 36u : 60u);
        ledit_send(NOTE_EDIT_ADD, n ? (float)(n->tick / 24u * 24u + 24u) : 0.0f);
    }
    int32_t turn = plat_enc(EN_SELECT);
    if (turn && p->count) {
        ledit.cursor = (uint8_t)clamp((int32_t)ledit.cursor + (turn > 0 ? 1 : -1), 0, (int32_t)p->count - 1);
        ledit.drawn = 0;
        ledit_hear();
    }
    for (uint32_t k = 0; k < KNOB_COUNT; k++) {
        turn = plat_enc((int)(EN_K1 + k));
        if (!turn || !n) {
            continue;
        }
        int d = turn > 0 ? 1 : -1;
        if (k == 0) {
            ledit_send(NOTE_EDIT_TICK, (float)((int32_t)(n->tick / 24u) * 24 + d * 24));
        } else if (k == 1) {
            int32_t l = n->len;
            l = d > 0 ? (l < 24 ? l * 2 : (l / 24 + 1) * 24) : (l <= 24 ? l / 2 : ((l - 1) / 24) * 24);
            ledit_send(NOTE_EDIT_LEN, (float)(l < 6 ? 6 : l));
        } else if (k == 2) {
            ledit_send(NOTE_EDIT_VEL, (float)(n->vel + d * 5));
        } else {
            ledit_send(NOTE_EDIT_PITCH, (float)(n->note + d));
        }
    }
    (void)plat_enc(EN_ALGO);
    (void)plat_enc(EN_PRESET);
    if (live.clock.running) {
        ledit.drawn = 0;                                 /* (the playhead) */
    }
}

#define LEDIT_TOP 34
#define LEDIT_ROLL 144                                   /* three bands of 48 */

static void ledit_draw(void)
{
    if (ledit.drawn) {
        return;
    }
    ledit.drawn = 1;
    const ols_pattern_t *p = ledit_pat();
    char t[24] = "LOOP EDIT  T";
    int_text(mirror.track + 1, t + 12);
    cv_begin(240, LEDIT_TOP, C_BLACK);
    cv_text(8, 6, &FONT_B, t, C_HI);
    char c[12];
    int_text(p->count, c);
    copy_text(c + text_len(c), " NOTES", 8);
    cv_text(232 - text_w(&FONT_XS, c), 12, &FONT_XS, c, C_GRAY);
    cv_rect(0, LEDIT_TOP - 2, 240, 1, C_LINE);
    cv_blit(0, 0);

    int lo = 127, hi = 0;
    for (uint32_t i = 0; i < p->count; i++) {
        lo = p->n[i].note < lo ? p->n[i].note : lo;
        hi = p->n[i].note > hi ? p->n[i].note : hi;
    }
    if (hi - lo < 11) {
        lo = (lo + hi) / 2 - 6 < 0 ? 0 : (lo + hi) / 2 - 6;
        hi = lo + 11;
    }
    int32_t rows = hi - lo + 1, rh = LEDIT_ROLL / rows < 2 ? 2 : LEDIT_ROLL / rows;
    uint32_t len = p->length ? p->length : 1u;
    for (int32_t band = 0; band < LEDIT_ROLL; band += 48) {
        cv_begin(240, 48, C_BLACK);
        cv_oy -= band;
        for (uint32_t b = 0; b * 384u < len; b++) {      /* the bars */
            cv_rect(4 + (int32_t)(b * 384u * 232u / len), 0, 1, LEDIT_ROLL, C_LINE);
        }
        if (live.clock.running) {
            cv_rect(4 + (int32_t)((live.clock.tick % len) * 232u / len), 0, 1, LEDIT_ROLL, C_GRAY);
        }
        for (uint32_t i = 0; i < p->count; i++) {
            const ols_note_t *n = &p->n[i];
            int32_t x = 4 + (int32_t)(n->tick * 232u / len), w = (int32_t)(n->len * 232u / len);
            int32_t y = (hi - n->note) * LEDIT_ROLL / rows;
            cv_rect(x, y, w < 2 ? 2 : w, rh - 1, i == ledit.cursor ? C_YELLOW : C_MINT);
        }
        cv_oy += band;
        cv_blit(0, LEDIT_TOP + band);
    }

    cv_begin(240, 240 - LEDIT_TOP - LEDIT_ROLL, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    if (ledit.cursor < p->count) {
        const ols_note_t *n = &p->n[ledit.cursor];
        char v[40];
        uint32_t k = 0;
        int_text((int32_t)(n->tick / 384u + 1u), v);
        k = text_len(v);
        v[k++] = '.';
        int_text((int32_t)(n->tick % 384u / 96u + 1u), v + k);
        k = text_len(v);
        v[k++] = '.';
        int_text((int32_t)(n->tick % 96u / 24u + 1u), v + k);
        cv_text(8, 8, &FONT_XS, "POS", C_GRAY);
        cv_text(8, 22, &FONT_B, v, C_HI);
        cv_text(68, 8, &FONT_XS, "LEN", C_GRAY);
        if (n->len >= 24u) {
            int_text((int32_t)(n->len / 24u), v);
            copy_text(v + text_len(v), n->len % 24u ? "+" : "", 2);
        } else {
            copy_text(v, n->len >= 12u ? "1/32" : "1/64", 6);
        }
        cv_text(68, 22, &FONT_B, v, C_HI);
        cv_text(120, 8, &FONT_XS, "VEL", C_GRAY);
        int_text(n->vel, v);
        cv_text(120, 22, &FONT_B, v, C_HI);
        cv_text(172, 8, &FONT_XS, "NOTE", C_GRAY);
        int_text(n->note, v);
        cv_text(172, 22, &FONT_B, v, C_HI);
    } else {
        cv_text(8, 16, &FONT_B, "EMPTY: EDIT ADDS", C_GRAY);
    }
    cv_text(8, 46, &FONT_XS, "REC DEL  EDIT ADD", C_GRAY);
    help_menu_out(46, C_GRAY);
    cv_blit(0, LEDIT_TOP + LEDIT_ROLL);
}
