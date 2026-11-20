/*
 * OpenVALive - FM-1 app: the screen (240 x 240).
 *
 *   header   0..29    track and its sound, tempo, transport   (or a message for 1.2 s)
 *   scope   30..89    real-time oscilloscope, triggered on a rising zero crossing
 *   tracks  90..169   the four loops: notes, playhead, selected (violet) / muted
 *   A      170..189   the live line: its sound and arpeggio (mint when selected)
 *   knobs  190..239   the four encoder values of the page (the value written over its bar), then a status line
 *
 * Colour is used for state only: values in the accent colour, labels grey, the selected track violet, A mint,
 * what is muted dim. The pages keep a colour: SOUND, LIVE, USER the accent, MACRO mint, SETUP violet.
 */

#define HEADER_H 30
#define SCOPE_Y 30
#define SCOPE_H 60
#define TRACKS_Y 90
#define TRACK_ROW_H 20
#define TRACKS_H (TRACKS * TRACK_ROW_H)
#define A_Y 170
#define A_H 20
#define KNOBS_Y 190
#define KNOBS_H 50

#define STRIP_X 20                      /* where the loops start, after the track number */
#define STRIP_W 216

/* synth voices and drum instruments sounding */
static int voices_sounding(void)
{
    int n = 0;
    for (int t = 0; t < TRACKS; t++) {
        for (int i = 0; i < VA_VOICES; i++) {
            if (va[t].voice[i].amp.stage != ENV_IDLE) {
                n++;
            }
        }
        n += engine_drum_active(&drum[t]);
    }
    return n;
}

/* ------------------------------------------------------------ header */

/* the transport: PLAY green, REC red, STOP / EXT grey */
static uint16_t transport_colour(void)
{
    if (live.recording) {
        return RGB(235, 40, 40);
    }
    return live.clock.running ? RGB(60, 210, 90) : C_GRAY;
}

/* EDIT held while playing (a loop being made: CAPTURE on release, a punch with notes): the transport shows a red
 * infinity sign in place of PLAY, until EDIT is let go */
static int edit_loop_on(void)
{
    return live.clock.running && (button_held(ui.buttons, B_EDIT) || ui.punch || ui.cap_armed);
}

static const char *transport_word(void)
{
    if (live.recording) {
        return "REC";
    }
    if (live.clock.running) {
        return "PLAY";
    }
    return live.clock.external ? "EXT" : "STOP";
}

/* the infinity sign: two rings of 2-pixel dots, centred on (cx, cy), 18 wide */
static void draw_inf(int32_t cx, int32_t cy, uint16_t c)
{
    static const int8_t RING[24][2] = { {4,0},{4,1},{3,2},{3,3},{2,3},{1,4},{0,4},{-1,4},{-2,3},{-3,3},
        {-3,2},{-4,1},{-4,0},{-4,-1},{-3,-2},{-3,-3},{-2,-3},{-1,-4},{0,-4},{1,-4},{2,-3},{3,-3},{3,-2},{4,-1} };
    for (int i = 0; i < 24; i++) {
        cv_rect(cx - 4 + RING[i][0], cy + RING[i][1], 2, 2, c);
        cv_rect(cx + 4 + RING[i][0], cy + RING[i][1], 2, 2, c);
    }
}

static void draw_header(void)
{
    cv_begin(240, HEADER_H, C_BLACK);

    if (plat_ms() < ui.msg_until) {
        uint16_t c0 = C_WHITE, c1 = C_HI;
        if (ui.msg_style == MSG_NOT_TAKEN) {    /* a gesture given up: its name in red, 0.7 s */
            c0 = C_RED;
        } else if (ui.msg_style == MSG_TAKEN) { /* taken: green, with a check mark when it stands alone, 0.7 s */
            c0 = c1 = C_GREEN;
        } else if (ui.msg_style == MSG_ARMED) { /* "LET GO SEQ": green, it says how to take it */
            c1 = C_GREEN;
        } else if (ui.msg_style == MSG_PRESS) { /* "REC (arrow down) PLAY FIRST": green, a button to press */
            c0 = c1 = C_GREEN;
        }
        int32_t x = 8;
        if (ui.msg_inf) {                       /* PERFORM: the loops (divided) and the filters */
            draw_inf(x + 9, 14, c0);
            x += 24;
        }
        x = cv_text(x, 6, &FONT_B, ui.msg[0], c0);
        if (ui.msg_style == MSG_ARMED) {        /* an arrow up: let the button go to take it */
            int32_t ax = x + 8;
            for (int32_t i = 0; i < 6; i++) {
                cv_rect(ax + 5 - i, 7 + i, 2 * i + 1, 1, C_GREEN);
            }
            cv_rect(ax + 3, 13, 5, 9, C_GREEN);
            x = ax + 6;
        } else if (ui.msg_style == MSG_PRESS) { /* an arrow down: press that button */
            int32_t ax = x + 8;
            cv_rect(ax + 3, 6, 5, 9, C_GREEN);
            for (int32_t i = 0; i < 6; i++) {
                cv_rect(ax + i, 15 + i, 11 - 2 * i, 1, C_GREEN);
            }
            x = ax + 6;
        }
        if (ui.msg_style == MSG_TAKEN && !ui.msg[1][0]) {
            int32_t cx = x + 10;                /* a check mark: the short stroke down, the long one up */
            for (int32_t i = 0; i < 5; i++) {
                cv_rect(cx + i, 12 + i, 3, 3, C_GREEN);
            }
            for (int32_t i = 0; i < 10; i++) {
                cv_rect(cx + 5 + i, 16 - i, 3, 3, C_GREEN);
            }
        } else {
            cv_text(x + 8, 6, &FONT_B, ui.msg[1], c1);
        }
        if (ui.msg_right) {                     /* the layer's black-key gesture, far right */
            cv_text(232 - text_w(&FONT_B, ui.msg_right), 6, &FONT_B, ui.msg_right, avol.on ? C_GREEN : C_MINT);
        }
    } else if (ui.buttons & LAYER_BUTTONS_ALL) {
        /* a layer button held, its title gone (a key tapped): nothing, not the main screen's title */
    } else {
        char t[4] = "T1";
        t[1] = (char)('1' + mirror.track);
        if (ui.on_a) {
            t[0] = 'A';
            t[1] = 0;
        }
        int pending = ui.pend_sound >= 0 && !ui.on_a;
        uint16_t tc = ui.on_a ? C_MINT : C_VIO;
        int blink = (plat_ms() / 250u) & 1u;
        if (pending) {                          /* a sound waits for SEL: the track and +SEL blink, the sound dark orange */
            tc = blink ? C_DARK_ORANGE : C_BLACK;
            ui.redraw = 1;
        }
        int32_t x = cv_text(8, 6, &FONT_B, t, tc);
        if (ui.on_a) {
            cv_text(x + 8, 6, &FONT_B, sound_label(set.live_sound), C_MINT);
        } else if (pending) {
            x = cv_text(x + 8, 6, &FONT_B, sound_label(ui.pend_sound), C_DARK_ORANGE);
            char plus[8] = "+";
            text_cat(plus, BTN_NAME[B_SEL]);
            cv_text(x + 6, 6, &FONT_B, plus, blink ? C_GREEN : C_BLACK);
        } else if (set.controller) {
            char ch[8] = "MIDI ";
            int_text(mirror.track + 1, ch + 5);
            cv_text(x + 8, 6, &FONT_B, ch, C_HI);          /* controller mode: the keys only send MIDI */
        } else {
            cv_text(x + 8, 6, &FONT_B, sound_label(set.preset[mirror.track]), C_HI);
        }

        char bpm[16];
        int_text((int32_t)(clock_bpm(&live.clock) + 0.5f), bpm);
        text_cat(bpm, " BPM");
        cv_text(232 - text_w(&FONT_XS, bpm), 0, &FONT_XS, bpm, C_GRAY);
        const char *word = transport_word();
        if (edit_loop_on()) {                   /* EDIT making a loop: a red infinity sign in place of PLAY */
            draw_inf(224, 21, C_RED);
            ui.redraw = 1;
        } else {
            cv_text(232 - text_w(&FONT_XS, word), 14, &FONT_XS, word, transport_colour());
        }

        /* MASTER (dark orange) over the CPU load (dark blue, red past 80 %) */
        char vol[12];
        int_text((int32_t)APP_MASTER_PCT(), vol);
        text_cat(vol, "%");
        cv_text(184 - text_w(&FONT_XS, vol), 0, &FONT_XS, vol, RGB(200, 96, 20));
        char cpu[12];
        int_text((int32_t)plat_cpu_pct(), cpu);
        text_cat(cpu, "%");
        cv_text(184 - text_w(&FONT_XS, cpu), 14, &FONT_XS, cpu, plat_cpu_pct() > 80u ? RGB(235, 40, 40) : RGB(70, 100, 210));
    }

    cv_rect(0, HEADER_H - 1, 240, 1, C_LINE);
    cv_blit(0, 0);
}

/* ------------------------------------------------------------ scope */

/* Copies the scope ring and finds a rising zero crossing so the wave stands still. */
static uint32_t scope_snapshot(int16_t *snap)
{
    uint32_t w = scope_write;
    for (uint32_t i = 0; i < SCOPE_N; i++) {
        snap[i] = scope[(w + i) & (SCOPE_N - 1u)];
    }
    for (uint32_t i = 1; i < SCOPE_N - 240u; i++) {
        if (snap[i - 1] < 0 && snap[i] >= 0) {
            return i;
        }
    }
    return 0;
}

/* The keyboard drawn while a layer button is held (keys on screen on). A target with another keyboard than the
 * FM-1's 27 keys from F gives its own: APP_SCREEN_KEYS keys from pitch class APP_SCREEN_FIRST, and APP_SCREEN_KEY(i)
 * the app key a drawn key stands for (-1: none; the SMK-37's 37 keys from C). */
#ifndef APP_SCREEN_KEYS
#define APP_SCREEN_KEYS NKEYS
#define APP_SCREEN_FIRST 5u
#define APP_SCREEN_KEY(i) ((int32_t)(i))
#endif

static int screen_key_black(uint32_t i)
{
    uint32_t pc = (APP_SCREEN_FIRST + i) % 12u;
    return pc == 1u || pc == 3u || pc == 6u || pc == 8u || pc == 10u;
}

/* two RGB565 colours mixed, t = 0 .. 256 of b */
static uint16_t mix565(uint16_t a, uint16_t b, uint32_t t)
{
    uint32_t r = ((a >> 11) * (256u - t) + (b >> 11) * t) >> 8;
    uint32_t g = (((a >> 5) & 63u) * (256u - t) + ((b >> 5) & 63u) * t) >> 8;
    uint32_t bl = ((a & 31u) * (256u - t) + (b & 31u) * t) >> 8;
    return (uint16_t)(r << 11 | g << 5 | bl);
}

/* one key: dark grey (nothing), half filled in mint with a mint outline (it does something now),
 * violet (on); a blinking one goes between the half-filled and the dark grey */
static void screen_key(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t lit, uint32_t dim, uint32_t blink,
                       uint16_t c, uint16_t solid)
{
    int off = blink && ((plat_ms() / 125u) & 1u);
    if (solid) {                                /* held (yellow), a muted track (red), a set-up key */
        cv_rect(x, y, w, h, solid);
        return;
    }
    if (lit && !blink) {
        cv_rect(x, y, w, h, C_VIO);
        return;
    }
    if ((dim || blink) && !off) {
        cv_rect(x, y, w, h, RGB(18, 18, 22));
        cv_rect(x, y + h / 2, w, h - h / 2, blink ? c : mix565(RGB(18, 18, 22), c, 150u));   /* offered: steady */
        cv_rect(x, y, w, 1, c);
        cv_rect(x, y, 1, h, c);
        cv_rect(x + w - 1, y, 1, h, c);
        return;
    }
    cv_rect(x, y, w, h, RGB(52, 52, 60));
}

/* a key on the screen keyboard: held yellow; SEL's mute keys red while muted; the set-up screens (HOME) blue; valid on
 * release (the button let go first takes it) pink, and mint / pink in turn when it also shows something on; something
 * on violet; offered mint */
static void screen_key_colours(int32_t k, uint32_t bit, uint32_t lit, uint16_t *c, uint16_t *solid, uint32_t *dim)
{
    if (k < 0) {
        return;
    }
    int w = white_index((uint32_t)k);
    if (button_held(ui.buttons, BTN_TRACKS)) {             /* SEL: the mutes show their state */
        int all = 1;
        for (int t = 0; t < TRACKS; t++) {
            all &= live.track[t].mute != 0;
        }
        if ((w >= 0 && w < TRACKS && live.track[w].mute) || ((uint32_t)k == KEY_MUTE_ALL && all)) {
            *solid = C_RED;
            return;
        }
    }
    if (ui.keys & bit) {
        *solid = C_YELLOW;
        return;
    }
    if (ui.keys_conf & bit) {
        *c = RGB(60, 110, 255);
        *dim |= bit;
        return;
    }
    if (keys_armable(ui.buttons) & bit) {
        if (lit & bit) {
            *solid = (plat_ms() / 250u) & 1u ? C_PINK : C_MINT;
            return;
        }
        *c = C_PINK;
        *dim |= bit;
    }
}

/* --- the keys' names on the screen keyboard: 3 characters at most, stacked down the key in a 5x7 font (white keys at
 * the bottom, under the black ones, every name starting on the same line; black keys at the top), a dark shadow
 * under them --- */
static const char FONT57_CHARS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-+<>#";
static const uint8_t FONT57[][7] = {           /* 7 rows of 5 pixels, top row first, left pixel the high bit */
    {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e},
    {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e},
    {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f},
    {0x1f, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0e},
    {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02},
    {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e},
    {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e},
    {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e},
    {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c},
    {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11},
    {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e},
    {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e},
    {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e},
    {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f},
    {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10},
    {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f},
    {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11},
    {0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e},
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0c},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f},
    {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11},
    {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11},
    {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e},
    {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10},
    {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d},
    {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11},
    {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e},
    {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a},
    {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11},
    {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04},
    {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f},
    {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00},
    {0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02},
    {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08},
    {0x0a, 0x0a, 0x1f, 0x0a, 0x1f, 0x0a, 0x0a}
};
#define NAME_LINE 8                             /* a character and a pixel between two */

static void glyph57(int32_t x, int32_t y, char ch, uint16_t col)
{
    for (uint32_t i = 0; FONT57_CHARS[i]; i++) {
        if (FONT57_CHARS[i] != ch) {
            continue;
        }
        for (int32_t r = 0; r < 7; r++) {
            for (int32_t b = 0; b < 5; b++) {
                if ((FONT57[i][r] >> (4 - b)) & 1u) {
                    cv_rect(x + b, y + r, 1, 1, col);
                }
            }
        }
        return;
    }
}

static void name57(int32_t x, int32_t y, const char *s, uint16_t col)
{
    for (int32_t i = 0; i < 3 && s[i]; i++) {
        glyph57(x + 1, y + NAME_LINE * i + 1, s[i], C_BLACK);
    }
    for (int32_t i = 0; i < 3 && s[i]; i++) {
        glyph57(x, y + NAME_LINE * i, s[i], col);
    }
}

enum { NAMES_SEL, NAMES_SEQ, NAMES_GLO, NAMES_ARP, NAMES_HOME, NAMES_FX, NAMES_SAVE, NAMES_REC };
enum { NAME_WHITE, NAME_YELLOW, NAME_RED, NAME_GREEN, NAME_BLUE, NAME_ORANGE, NAME_VIOLET };
#define NB(b) (16u + (b))                       /* a black key: its index among the black keys */
static const struct {
    uint8_t layer, key, col;                    /* key: a white key's index 0-15, or NB(the black key's) */
    char name[4];
} KEY_NAME[] = {
    { NAMES_SEL, 0, 0, "M1" }, { NAMES_SEL, 1, 0, "M2" }, { NAMES_SEL, 2, 0, "M3" }, { NAMES_SEL, 3, 0, "M4" },
    { NAMES_SEL, 5, 0, "AV" }, { NAMES_SEL, 6, 0, "QNT" }, { NAMES_SEL, 7, 0, "CKI" }, { NAMES_SEL, 8, 0, "CKO" },
    { NAMES_SEL, 9, NAME_YELLOW, "E3" }, { NAMES_SEL, 10, NAME_YELLOW, "E4" }, { NAMES_SEL, 11, NAME_YELLOW, "E5" },
    { NAMES_SEL, 12, 0, "E6" }, { NAMES_SEL, 13, 0, "E7" }, { NAMES_SEL, 14, 0, "E8" }, { NAMES_SEL, 15, 0, "MID" },
    { NAMES_SEL, NB(0), 0, "MA" }, { NAMES_SEL, NB(2), NAME_BLUE, "LE" }, { NAMES_SEL, NB(3), NAME_RED, "CLR" },
    { NAMES_SEQ, 0, 0, "L1" }, { NAMES_SEQ, 1, 0, "L2" }, { NAMES_SEQ, 2, 0, "L4" }, { NAMES_SEQ, 3, 0, "L8" },
    { NAMES_SEQ, 4, 0, "L16" }, { NAMES_SEQ, 5, NAME_VIOLET, "SLC" }, { NAMES_SEQ, 6, NAME_YELLOW, "DLY" },
    { NAMES_SEQ, 7, NAME_ORANGE, "FL-" }, { NAMES_SEQ, 8, NAME_ORANGE, "FL+" }, { NAMES_SEQ, 9, NAME_YELLOW, "DRV" },
    { NAMES_SEQ, 10, NAME_YELLOW, "CRU" }, { NAMES_SEQ, 11, NAME_RED, "BRK" }, { NAMES_SEQ, 12, NAME_RED, "SOL" },
    { NAMES_SEQ, 13, NAME_RED, "DRM" }, { NAMES_SEQ, 14, NAME_RED, "OUT" }, { NAMES_SEQ, 15, 0, "OFF" },
    { NAMES_GLO, 0, 0, "SC1" }, { NAMES_GLO, 1, 0, "SC2" }, { NAMES_GLO, 2, 0, "SC3" }, { NAMES_GLO, 3, 0, "SC4" },
    { NAMES_GLO, 4, 0, "SC5" }, { NAMES_GLO, 5, 0, "SC6" }, { NAMES_GLO, 6, 0, "SC7" }, { NAMES_GLO, 7, 0, "SC8" },
    { NAMES_GLO, 8, NAME_YELLOW, "SV1" }, { NAMES_GLO, 9, NAME_YELLOW, "SV2" }, { NAMES_GLO, 10, NAME_YELLOW, "SV3" },
    { NAMES_GLO, 11, NAME_YELLOW, "SV4" }, { NAMES_GLO, 12, NAME_YELLOW, "SV5" }, { NAMES_GLO, 13, NAME_YELLOW, "SV6" },
    { NAMES_GLO, 14, NAME_YELLOW, "SV7" }, { NAMES_GLO, 15, NAME_YELLOW, "SV8" },
    { NAMES_ARP, 0, 0, "MUT" }, { NAMES_ARP, 1, 0, "HUM" }, { NAMES_ARP, 2, 0, "FIL" }, { NAMES_ARP, 3, 0, "SMP" },
    { NAMES_ARP, 4, 0, "RO<" }, { NAMES_ARP, 5, 0, "RO>" }, { NAMES_ARP, 6, 0, "REV" },
    { NAMES_ARP, 7, NAME_RED, "T-1" }, { NAMES_ARP, 8, NAME_GREEN, "T+1" }, { NAMES_ARP, 9, 0, "EXP" },
    { NAMES_ARP, 10, 0, "CON" }, { NAMES_ARP, 11, 0, "CHA" }, { NAMES_ARP, 12, 0, "RAT" },
    { NAMES_ARP, 13, NAME_RED, "O-" }, { NAMES_ARP, 14, NAME_GREEN, "O+" },
    { NAMES_HOME, 0, 0, "PNC" }, { NAMES_HOME, 1, NAME_GREEN, "RST" }, { NAMES_HOME, 2, NAME_RED, "STP" },
    { NAMES_HOME, 6, 0, "OPT" }, { NAMES_HOME, 7, 0, "SND" }, { NAMES_HOME, NB(3), 0, "LE" },
    { NAMES_FX, 0, NAME_RED, "RST" },
#if APP_WHEEL
    { NAMES_FX, 13, 0, "PAN" },
#endif
    { NAMES_FX, 5, 0, "ORG" }, { NAMES_FX, 6, 0, "GM" },
    { NAMES_SAVE, 0, 0, "U1" }, { NAMES_SAVE, 1, 0, "U2" }, { NAMES_SAVE, 2, 0, "U3" }, { NAMES_SAVE, 3, 0, "U4" },
    { NAMES_SAVE, 4, 0, "U5" }, { NAMES_SAVE, 5, 0, "U6" }, { NAMES_SAVE, 6, 0, "U7" }, { NAMES_SAVE, 7, 0, "U8" },
    { NAMES_SAVE, 8, 0, "SU1" }, { NAMES_SAVE, 9, 0, "SU2" }, { NAMES_SAVE, 10, 0, "SU3" }, { NAMES_SAVE, 11, 0, "SU4" },
    { NAMES_SAVE, 12, 0, "SU5" }, { NAMES_SAVE, 13, 0, "SU6" }, { NAMES_SAVE, 14, 0, "SU7" },
    { NAMES_SAVE, 15, 0, "SU8" }, { NAMES_SAVE, NB(5), 0, "ALL" },
    { NAMES_REC, 0, 0, "RST" },
};

/* the layer whose names show (the same order as key_lights), -1: none */
static int key_names_layer(uint32_t buttons)
{
    if (button_held(buttons, B_GLO)) {
        return NAMES_GLO;
    }
    if (button_held(buttons, BTN_TRACKS)) {
        return NAMES_SEL;
    }
    if (button_held(buttons, BTN_PERFORM)) {
        return NAMES_SEQ;
    }
    if (button_held(buttons, B_HOME)) {
        return NAMES_HOME;
    }
    if (button_held(buttons, B_REC)) {
        return ui.on_a ? -1 : NAMES_REC;
    }
    if (button_held(buttons, BTN_PAGES)) {
        return NAMES_FX;
    }
    if (button_held(buttons, B_SAVE)) {
        return NAMES_SAVE;
    }
    return button_held(buttons, B_ARP) ? NAMES_ARP : -1;
}

/* the name of app key k on that layer (NULL: none), and its colour */
static const char *key_name(int layer, int32_t k, uint16_t *col)
{
    int idx = -1;
    if (k < 0 || layer < 0) {
        return 0;
    }
    int w = white_index((uint32_t)k);
    if (w >= 0) {
        idx = w;
    } else {
        for (int b = 0; b < BLACK_KEYS; b++) {
            if (BLACK_KEY[b] == (uint32_t)k) {
                idx = (int)NB(b);
            }
        }
    }
    for (uint32_t i = 0; i < sizeof KEY_NAME / sizeof KEY_NAME[0]; i++) {
        if (KEY_NAME[i].layer == layer && KEY_NAME[i].key == idx) {
            static const uint16_t COL[] = { RGB(235, 235, 235), RGB(255, 214, 40), RGB(255, 70, 60), RGB(60, 230, 90),
                                            RGB(90, 160, 255), RGB(255, 140, 40), RGB(190, 130, 255) };
            *col = COL[KEY_NAME[i].col];
            return KEY_NAME[i].name;
        }
    }
    return 0;
}

static int draw_layer_keys(void)
{
    const uint32_t LAYERS = (1u << B_ARP) | (1u << B_GLO) | (1u << BTN_PERFORM) | (1u << BTN_TRACKS) | (1u << B_SAVE)
                            | (1u << B_HOME) | (1u << BTN_PAGES) | (1u << B_REC);
    if (!set.keys_screen || !(ui.buttons & LAYERS) || ui.hovering) {
        return 0;                                       /* (SEL + SELECT pointing at a line: the title says it) */
    }
    uint32_t lit, dim, blink;
    key_lights(ui.keys, ui.buttons, &lit, &dim, &blink);
    const int names = key_names_layer(ui.buttons);
    uint32_t whites = 0;
    for (uint32_t i = 0; i < APP_SCREEN_KEYS; i++) {
        whites += !screen_key_black(i);
    }
    const int32_t ww = (int32_t)(236u / whites), x0 = (240 - ww * (int32_t)whites) / 2, top = 4, kh = SCOPE_H - 8;
    const int32_t bh = (kh * 12 + 24) / 25;     /* the black keys: 12/25 of the height (a name fits under them) */
    cv_begin(240, SCOPE_H, C_BLACK);
    for (int pass = 0; pass < 2; pass++) {             /* the white keys, then the black ones over them */
        int32_t white = 0;
        for (uint32_t i = 0; i < APP_SCREEN_KEYS; i++) {
            int black = screen_key_black(i);
            if (black != pass) {
                white += !black;
                continue;
            }
            int32_t k = APP_SCREEN_KEY(i);
            uint32_t bit = k >= 0 ? 1u << k : 0u;
            uint16_t solid = 0, c = C_MINT;
            screen_key_colours(k, bit, lit, &c, &solid, &dim);
            uint16_t ncol = 0;
            const char *name = key_name(names, k, &ncol);
            if (solid == C_YELLOW) {
                ncol = C_BLACK;                         /* (held: dark letters on the yellow) */
            }
            if (!black) {
                int32_t x = x0 + white * ww;
                screen_key(x, top, ww - 1, kh, lit & bit, dim & bit, blink & bit, c, solid);
                if ((APP_SCREEN_FIRST + i) % 12u == 0u) {
                    cv_rect(x - 2, top, 2, kh, RGB(170, 70, 0));   /* a dark orange line left of each C */
                }
                if (name) {
                    name57(x + (ww - 6) / 2, top + kh - 3 * NAME_LINE, name, ncol);
                }
                white++;
            } else {
                int32_t bw = ww * 3 / 5 | 1, x = x0 + white * ww - bw / 2;
                cv_rect(x - 1, top, bw + 2, bh + 1, C_BLACK);
                screen_key(x, top, bw, bh, lit & bit, dim & bit, blink & bit, c, solid);
                if (name) {
                    name57(x + (bw - 5) / 2, top + 1, name, ncol);
                }
            }
        }
    }
    cv_blit(0, SCOPE_Y);
    ui.redraw = 1;                                      /* (what blinks) */
    return 1;
}

/* --- the help pages, after the boot screen (OPTION CONTROL: HELP AT START): a page every 3 s, until a button (not
 * SEL) or a knob. Drawn in bands of 48 rows, each a canvas of its own (y: the screen's) --- */
#define HELP_BAND 48
#define HELP_PAGES 2u
#define HELP_TEXT RGB(225, 225, 225)

static void help_band(int32_t by)
{
    cv_begin(240, HELP_BAND, C_BLACK);
    cv_oy = -by;                                /* draw at the screen's y; the canvas keeps rows by .. by + 47 */
    cv_y0 = 0;
    cv_y1 = HELP_BAND;
}

/* do ré mi as the screen keyboard draws them, white keys offered (half filled, outlined) in colour c, do# and ré#
 * neutral: 39 x 38 */
static void help_keys(int32_t x, int32_t y, uint16_t c)
{
    for (int32_t i = 0; i < 3; i++) {
        screen_key(x + 13 * i, y, 12, 38, 0, 1, 0, c, 0);
    }
    for (int32_t i = 1; i < 3; i++) {
        cv_rect(x + 13 * i - 5, y, 9, 24, C_BLACK);
        cv_rect(x + 13 * i - 4, y, 7, 23, RGB(40, 40, 46));
    }
}

/* a small green arrow up, 7 x 10: let go */
static int32_t help_arrow(int32_t x, int32_t y)
{
    for (int32_t i = 0; i < 4; i++) {
        cv_rect(x + 3 - i, y + i, 2 * i + 1, 1, C_GREEN);
    }
    cv_rect(x + 2, y + 4, 3, 6, C_GREEN);
    return x + 7;
}

/* the two lines at the foot of each page: SEL (NOTE on the SMK-37) turns the help pages off for good, another
 * button closes them */
static void help_footer(void)
{
    int32_t x = cv_text(8, 204, &FONT_XS, BTN_NAME[B_SEL], C_YELLOW);
    cv_text(x, 204, &FONT_XS, ": NO HELP AT START", C_YELLOW);
    cv_text(8, 222, &FONT_XS, "OTHER BUTTON: CLOSE", C_GRAY);
}

static void help_page_keys(void)
{
    cv_text(8, 8, &FONT_B, "KEY COLOURS", C_WHITE);
    cv_text(232 - text_w(&FONT_XS, "1/2"), 12, &FONT_XS, "1/2", C_GRAY);
    const uint16_t BLUE = RGB(60, 110, 255);
    help_keys(8, 34, BLUE);
    cv_text(60, 34, &FONT_B, "BLUE", BLUE);
    cv_text(60, 56, &FONT_XS, "SET-UP SCREENS", HELP_TEXT);
    help_keys(8, 80, C_PINK);                   /* valid on release: the layer button let go first, then the key */
    cv_text(60, 80, &FONT_B, "PINK", C_PINK);
    int32_t x = cv_text(60, 102, &FONT_XS, "ON RELEASE", HELP_TEXT);
    help_arrow(x + 6, 103);
    cv_text(60, 118, &FONT_XS, "LET GO BTN MENU FIRST", HELP_TEXT);
    x = cv_text(60, 134, &FONT_XS, "THEN", HELP_TEXT);
    x = help_arrow(x + 6, 135);
    cv_text(x + 6, 134, &FONT_XS, "KEY", HELP_TEXT);
    help_keys(8, 152, C_MINT);
    cv_text(60, 152, &FONT_B, "MINT", C_MINT);
    cv_text(60, 174, &FONT_XS, "READY: ACTS AT ONCE", HELP_TEXT);
    help_footer();
}

static void help_page_play(void)
{
    cv_text(8, 8, &FONT_B, "IN PLAY", C_GREEN);
    cv_text(232 - text_w(&FONT_XS, "2/2"), 12, &FONT_XS, "2/2", C_GRAY);
    int32_t x = cv_text(8, 46, &FONT_B, "EDIT", C_WHITE);
    draw_inf(x + 18, 54, C_RED);                /* EDIT makes a loop: the red infinity sign */
    cv_text(16, 76, &FONT_XS, "PUSH", C_AMB);
    cv_text(70, 76, &FONT_XS, "FULL LOOP AT END", HELP_TEXT);
    cv_text(16, 96, &FONT_XS, "HOLD", C_AMB);
    cv_text(70, 96, &FONT_XS, "PART LIVE LOOP", HELP_TEXT);
    cv_rect(8, 124, 224, 1, RGB(52, 52, 60));
    cv_text(8, 136, &FONT_B, "REC", C_RED);
    cv_text(16, 166, &FONT_XS, "PUSH", C_AMB);
    cv_text(70, 166, &FONT_XS, "LIVE OVERDUB", HELP_TEXT);
    cv_text(70, 184, &FONT_XS, "ADDS NOTES ON TOP", HELP_TEXT);
    help_footer();
}

static void help_draw(uint32_t now)
{
    uint8_t page = (uint8_t)(((now - ui.help_since) / 3000u) % HELP_PAGES);
    if (ui.help_shown == page + 1u) {
        return;
    }
    ui.help_shown = (uint8_t)(page + 1u);
    for (int32_t by = 0; by < 240; by += HELP_BAND) {
        help_band(by);
        if (page == 0u) {
            help_page_keys();
        } else {
            help_page_play();
        }
        cv_blit(0, (uint32_t)by);
    }
}

static void draw_scope(void)
{
#ifdef APP_SCOPE_OVERLAY
    if (APP_SCOPE_OVERLAY()) {                  /* a target's own picture in place of the scope (SMK-37: the pads' map) */
        return;
    }
#endif
    if (draw_layer_keys()) {
        return;
    }
    int16_t snap[SCOPE_N];
    uint32_t start = scope_snapshot(snap);
    const int32_t mid = SCOPE_H / 2;
    int32_t prev_y = -1;

    cv_begin(240, SCOPE_H, C_BLACK);
    cv_rect(0, mid, 240, 1, C_LINE);

    /* auto-gain: a quiet sound fills the scope too (x1 to x8; down at once, up slowly), so its shape shows */
    static int32_t gain16 = 16;
    int32_t peak = 1;
    for (uint32_t x = 0; x < 240u; x++) {
        int32_t a = snap[start + x] < 0 ? -snap[start + x] : snap[start + x];
        peak = a > peak ? a : peak;
    }
    int32_t want = clamp(29000 * 16 / peak, 16, 128);
    gain16 = want < gain16 ? want : gain16 + (want - gain16 + 7) / 8;

    for (uint32_t x = 0; x < 240u; x++) {
        int32_t y = mid - ((int32_t)snap[start + x] * gain16 / 16 * (SCOPE_H - 6)) / 65536;
        y = clamp(y, 1, SCOPE_H - 2);
        if (prev_y >= 0) {
            cv_line((int32_t)x - 1, prev_y, (int32_t)x, y, C_HI);
        }
        prev_y = y;
    }
    cv_blit(0, SCOPE_Y);
}

/* ------------------------------------------------------------ tracks */

static void pitch_range(const ols_pattern_t *p, int32_t *lo, int32_t *hi)
{
    *lo = 127;
    *hi = 0;
    for (uint16_t i = 0; i < p->count; i++) {
        *lo = p->n[i].note < *lo ? p->n[i].note : *lo;
        *hi = p->n[i].note > *hi ? p->n[i].note : *hi;
    }
    if (*hi - *lo < 12) {
        *hi = *lo + 12;                 /* a one-note loop sits in the middle, not stretched */
    }
}

/* a clip being taken on this line (a target with pads): the line goes dark green when armed, then a brighter green
 * fills it from the left as the take goes, the whole width being the longest clip (CLIP_MAX_BARS) */
static void draw_clip_take(int line, int32_t y0, int32_t h)
{
#ifdef APP_CLIPS
    if (crec.slot < 0 || crec.line != (uint8_t)line) {
        return;
    }
    cv_rect(0, y0, 240, h, RGB(10, 40, 18));
    if (crec.started) {
        uint32_t at = live.clock.free_tick - crec.start;
        int32_t px = (int32_t)((at > CLIP_MAX_TICKS ? CLIP_MAX_TICKS : at) * 240u / CLIP_MAX_TICKS);
        cv_rect(0, y0, px, h, RGB(24, 120, 52));
        for (uint32_t b = 1; b < CLIP_MAX_BARS; b++) {      /* the bars, to see how long it already is */
            cv_rect((int32_t)(b * 240u / CLIP_MAX_BARS), y0 + 2, 1, h - 4, RGB(60, 210, 90));
        }
    }
    ui.redraw = 1;
#else
    (void)line;
    (void)y0;
    (void)h;
#endif
}

static void draw_track(int t)
{
    const ols_track_t *tr = &live.track[t];
    const ols_pattern_t *p = &tr->pat;
    int32_t y0 = t * TRACK_ROW_H;
    int selected = ui.hovering ? t == ui.hover : (!ui.on_a && t == mirror.track);   /* SEL + SELECT: the line pointed at */
    uint16_t label = tr->mute ? C_DIM : (selected ? C_VIO : C_GRAY);
    uint16_t note_colour = tr->mute ? C_DIM : (!live.clock.running ? C_MINT : (selected ? C_HI : C_AMB));   /* stopped:
                                                 * the notes mint */
    uint32_t length = p->length;

    if (live.stutter && tr->stutter) {
        cv_rect(0, y0, 240, TRACK_ROW_H - 1, C_LOOP_BG);     /* its loop runs: red as long as it does */
    } else if (selected) {
        cv_rect(0, y0, 240, TRACK_ROW_H - 1, C_VIO_BG);      /* the selected track's line, violet */
    }
    draw_clip_take(t, y0, TRACK_ROW_H - 1);
    if (ui.cap_armed && (!live.clock.running || live.recording || ui.on_a || ui.cap_track != mirror.track)) {
        ui.cap_on = 0;                          /* (a stop, REC, another line: the take is given up) */
        ui.cap_armed = 0;
    }
    int capturing = ui.cap_on && t == ui.cap_track;
    /* recording: from the first note taken (or the punch), the playhead paints the line pink where it records, so
     * the take's start is seen coming back round (when to stop); let go, it paints it back as it passes */
    uint8_t *painting = rec_painting;
    if (capturing) {
        painting[t] = 1;                        /* CAPTURE's notes: green, from the first one */
    } else if (!(live.recording && !ui.on_a && t == mirror.track)) {
        painting[t] = 0;
    } else if (tr->held_count || live.punch) {
        painting[t] = 1;                        /* a note is being recorded: the take starts here */
    }
    static uint8_t pink[TRACKS][30];
    static uint8_t was_capturing[TRACKS];
    if (was_capturing[t] && !capturing) {       /* CAPTURE taken (or given up): the line back to its colour at once */
        for (uint32_t i = 0; i < sizeof pink[t]; i++) {
            pink[t][i] = 0;
        }
    }
    was_capturing[t] = (uint8_t)capturing;
    if (length && live.clock.running) {
        static int16_t last_x[TRACKS] = { -1, -1, -1, -1 };
        int rec = painting[t];
        int32_t px = (int32_t)(ols_live_step(&live, t) * OLS_TICKS_PER_STEP * 240u / length);
        int32_t from = last_x[t] < 0 ? px : last_x[t];
        for (int32_t i = 0; i < 240; i++) {     /* from where it was to where it is, across the loop's end */
            int32_t c = (from + i) % 240;
            if (rec) {
                pink[t][c >> 3] |= (uint8_t)(1u << (c & 7));
            } else {
                pink[t][c >> 3] &= (uint8_t)~(1u << (c & 7));
            }
            if (c == px) {
                break;
            }
        }
        last_x[t] = (int16_t)px;
        for (int32_t c = 0; c < 240; c++) {
            if ((pink[t][c >> 3] >> (c & 7)) & 1u) {
                cv_rect(c, y0, 1, TRACK_ROW_H - 1, capturing ? RGB(24, 120, 52) : C_PINK_BG);
            }
        }
    }
    char n[2] = { (char)('1' + t), 0 };
    if (!ui.hovering && !ui.on_a && t == mirror.track) {
        cv_text(6, y0 + 3, &FONT_XS, n, C_YELLOW);         /* the selected track: its number bold yellow */
        cv_text(7, y0 + 3, &FONT_XS, n, C_YELLOW);
    } else {
        cv_text(6, y0 + 3, &FONT_XS, n, label);
    }

    /* bar lines, then the notes */
    for (uint32_t bar = 0; bar <= length / OLS_TICKS_PER_BAR; bar++) {
        int32_t x = STRIP_X + (int32_t)(bar * OLS_TICKS_PER_BAR * STRIP_W / length);
        cv_rect(x > STRIP_X + STRIP_W - 1 ? STRIP_X + STRIP_W - 1 : x, y0 + 2, 1, TRACK_ROW_H - 4, C_LINE);
    }

    int32_t lo, hi;
    pitch_range(p, &lo, &hi);
    for (uint16_t i = 0; i < p->count; i++) {
        const ols_note_t *note = &p->n[i];
        int32_t x = STRIP_X + (int32_t)((uint32_t)note->tick * STRIP_W / length);
        int32_t w = (int32_t)((uint32_t)note->len * STRIP_W / length);
        int32_t y = y0 + TRACK_ROW_H - 5 - (int32_t)(note->note - lo) * (TRACK_ROW_H - 8) / (hi - lo);
        w = w < 2 ? 2 : w;
        if (x + w > STRIP_X + STRIP_W) {
            w = STRIP_X + STRIP_W - x;
        }
        cv_rect(x, y, w, 3, note_colour);
    }

    /* playhead */
    if (live.clock.running) {
        uint32_t pos = ols_live_step(&live, t) * OLS_TICKS_PER_STEP;
        int32_t x = STRIP_X + (int32_t)(pos * STRIP_W / length);
        int coloured = (live.stutter && tr->stutter) || painting[t];
        cv_rect(x, y0 + 1, 2, TRACK_ROW_H - 2, coloured ? C_GREEN : (selected ? C_WHITE : C_GRAY));   /* green over
                                                 * a red (loop) or pink (recording) line, seen at once */
    }

    cv_rect(0, y0 + TRACK_ROW_H - 1, 240, 1, C_LINE);
}

/* A: the live line, under the tracks */
static void draw_a(void)
{
    int pointed = ui.hovering && ui.hover == TRACKS;
    cv_begin(240, A_H, pointed ? C_VIO_BG : (ui.on_a && !ui.hovering ? C_MINT_BG : C_BLACK));
    draw_clip_take(T_LIVE, 0, A_H - 1);
    cv_text(6, 3, &FONT_XS, "A", pointed ? C_VIO : (ui.on_a ? C_MINT : C_GRAY));
    if (ui.on_a && !pointed) {
        cv_text(7, 3, &FONT_XS, "A", C_MINT);   /* selected: bold */
    }
    int32_t x = cv_text(STRIP_X, 3, &FONT_XS, "LIVE", ui.on_a ? C_MINT : C_DIM);
    cv_text(x + 8, 3, &FONT_XS, sound_name(set.live_sound), ui.on_a ? C_WHITE : C_GRAY);
    if (set.arp_mode != ARP_OFF) {
        char t[16] = "ARP ";
        text_cat(t, ARP_NAME[set.arp_mode]);
        cv_text(234 - text_w(&FONT_XS, t), 3, &FONT_XS, t, ui.on_a ? C_MINT : C_GRAY);
    }
    cv_rect(0, A_H - 1, 240, 1, C_LINE);
    cv_blit(0, A_Y);
}

static void draw_tracks(void)
{
    cv_begin(240, TRACKS_H, C_BLACK);
    for (int t = 0; t < TRACKS; t++) {
        draw_track(t);
    }
    cv_blit(0, TRACKS_Y);
    draw_a();
}

/* ------------------------------------------------------------ knobs */

static void knob_value(uint32_t k, float *bar, int32_t *shown, const char **label, const char **text)
{
    uint8_t t = mirror.track;
    *text = 0;
    if (set.page == PAGE_SETUP) {
        static const char *const XFADE_TEXT[4] = { "CUT", "1 BAR", "2 BARS", "4 BARS" };
        static char channel[8];
        *label = SETUP_NAME[k];
        *shown = 0;
        switch (k) {
        case 0:
            *bar = (float)set.midi_ch / (float)MIDI_CH_CLOCK;
            if (set.midi_ch == 0u) {
                *text = "ALL";
            } else if (set.midi_ch == MIDI_CH_CLOCK) {
                *text = "CLOCK";
            } else {
                int_text(set.midi_ch, channel);
                *text = channel;
            }
            break;
        case 1:
            *bar = (float)ui.xfade / 3.0f;
            *text = XFADE_TEXT[ui.xfade & 3u];
            break;
        case 2:
            *bar = live.quantize ? 1.0f : 0.0f;
            *text = live.quantize ? "1/16" : "OFF";
            break;
        default:
            *bar = (float)live.track[t].pat.length / (float)OLS_MAX_TICKS;
            *shown = (int32_t)(live.track[t].pat.length / OLS_TICKS_PER_BAR);
            break;
        }
        return;
    }
    int32_t slot = page_slot(set.page, k);      /* a mapped page (MACRO, LIVE, EQ, USER) whose knob was changed in MAP
                                                 * CONTROL, or EQ and USER always: the target's name and value */
    if (slot >= 0 && (set.page == PAGE_USER || set.page == PAGE_EQ || !slot_is_default((uint32_t)slot))) {
        map_knob_value((uint32_t)slot, bar, shown, label);
        return;
    }
    if (set.page == PAGE_DRUM) {                    /* DRUM EQ: BASS TREBLE KICK HATS */
        const ols_track_t *tr = &live.track[t];
        *label = DRUM_EQ_NAME[k];
        *bar = tr->engine.api->get_param(tr->engine.ctx, DRUM_EQ_ID[k]);
        *shown = (int32_t)(*bar * 100.0f + 0.5f);
        return;
    }
    if (set.page == PAGE_MACRO) {
        *label = MACRO_NAME[k];
        *bar = live.macro[k];
        *shown = (int32_t)(live.macro[k] * 100.0f + 0.5f);
        return;
    }
    if (set.page == PAGE_SOUND && set.controller) {
        *bar = (float)ui.cc_value[k] / 127.0f;
        *shown = ui.cc_value[k];
        *label = "CC";
        return;
    }
    if (set.page == PAGE_SOUND && ui.on_a) {        /* A's sound */
        *bar = engine_va_get_param(&lv, ENGINE_VA_API.macros[k]);
        *shown = (int32_t)(*bar * 100.0f + 0.5f);
        *label = ENGINE_VA_API.macro_names[k];
        return;
    }
    if (set.page == PAGE_SOUND) {
        const ols_track_t *tr = &live.track[t];
        *bar = tr->engine.api->get_param(tr->engine.ctx, tr->engine.api->macros[k]);
        *shown = (int32_t)(*bar * 100.0f + 0.5f);
        *label = tr->engine.api->macro_names[k];
        return;
    }
    *label = LIVE_NAME[k];
    switch (k) {
    case 0:
        *bar = live.morph;
        break;
    case 1:
        *bar = live.swing;
        break;
    case 2:
        *bar = live.pump;
        break;
    default:
        if (ui.on_a) {
            *bar = ui.lv_level / 1.5f;
            *shown = (int32_t)(*bar * 100.0f + 0.5f);       /* 0..100 over the whole knob (x1.5 at the top) */
            return;
        }
        *bar = live.track[t].gain / 1.5f;
        *shown = (int32_t)(*bar * 100.0f + 0.5f);
        return;
    }
    *shown = (int32_t)(*bar * 100.0f + 0.5f);
}

/* each page its colour, on its knob names, bars and name (bottom right): SOUND the accent, MACRO cyan,
 * LIVE mint (as A), SETUP pink, USER violet. Every bar white on grey, its value centred, bold, black: readable. */
static const uint16_t PAGE_COLOUR[PAGE_COUNT] = { 0, C_CYAN, C_MINT, C_PINK, C_VIO, C_RED, C_ORANGE };

static uint16_t colour_of_page(uint32_t page)
{
    return page == PAGE_SOUND ? C_HI : PAGE_COLOUR[page % PAGE_COUNT];
}

static uint16_t page_colour(void)
{
    return colour_of_page(set.page);
}

static uint16_t page_colour_label(void)
{
    return page_colour();
}

static void bold_text(int32_t x, int32_t y, const char *s, uint16_t c)
{
    cv_text(x, y, &FONT_XS, s, c);
    cv_text(x + 1, y, &FONT_XS, s, c);
}

/* the value centred over its bar (x, 16, 52 x 16; filled over `fill` pixels): white on grey, bold black */
static void bar_value(int32_t x, int32_t fill, const char *s)
{
    cv_rect(x + 1, 17, 50, 14, C_BAR_BG);
    cv_rect(x + 1, 17, fill, 14, C_WHITE);
    bold_text(x + 1 + (50 - text_w(&FONT_XS, s) - 1) / 2, 18, s, C_BLACK);
}

static void draw_knob(uint32_t k)
{
    int32_t x = 6 + (int32_t)k * 58;
    float value;
    int32_t shown;
    const char *label;
    const char *text;
    knob_value(k, &value, &shown, &label, &text);

    int32_t bar = (int32_t)(fm_clampf(value, 0.0f, 1.0f) * 50.0f + 0.5f);
    char t[8];

    cv_text(x, 2, &FONT_XS, label, page_colour_label());
    cv_rect(x, 16, 52, 16, C_LINE);
    int_text(shown, t);
    if (set.page == PAGE_LIVE && k == 0 && mirror.scene_prev < 0) {
        cv_rect(x + 1, 17, 50, 14, C_RED);      /* MORPH: two scenes not recalled yet, it does nothing */
        return;
    }
    bar_value(x, bar, text ? text : t);         /* the value over its bar: one line saved */
}

static void draw_status_line(void)
{
    /* the perform effects running, else the octave */
    char fx[24] = "";
    int running = 0, looping = 0;
    for (int w = 0; w < FX_ALL; w++) {
        if (ui.fx_on[w]) {
            if (running == 0 || (PERFORM_KEY[w].type == FX_LOOP && !looping)) {
                copy_text(fx, PERFORM_KEY[w].name, sizeof fx);   /* a loop comes first: it holds the music */
            }
            looping |= PERFORM_KEY[w].type == FX_LOOP;
            running++;
        }
    }
    if (running > 1) {
        char more[6] = " +";
        int_text(running - 1, more + 2);
        text_cat(fx, more);
    }
    if (running && plat_ms() < ui.fx_flash_until) {   /* just taken: white on green, 0.5 s */
        cv_rect(84, 34, text_w(&FONT_XS, fx) + 8, 16, RGB(30, 150, 60));
        cv_text(88, 36, &FONT_XS, fx, C_WHITE);
        ui.redraw = 1;
    } else if (running) {
        cv_text(88, 36, &FONT_XS, fx, C_WHITE);
    } else {
        char oct[12] = "OCT ";
        octave_text(set.octave[ui.on_a ? TRACKS : mirror.track], oct + 4);
        cv_text(96, 36, &FONT_XS, oct, C_GRAY);
    }

    /* the page on the left, in its colour; on the right an arrow and the page FX turns to, in that one's */
    const char *page = set.controller && set.page == PAGE_SOUND ? "FULL MIDI" : PAGE_NAME[set.page];
    cv_text(6, 36, &FONT_XS, page, page_colour_label());
    uint32_t next = page_after(set.page);
    int32_t nx = 234 - text_w(&FONT_XS, PAGE_NAME[next]);
    cv_text(nx, 36, &FONT_XS, PAGE_NAME[next], colour_of_page(next));
    cv_line(nx - 14, 43, nx - 5, 43, C_GRAY);
    cv_line(nx - 8, 40, nx - 5, 43, C_GRAY);
    cv_line(nx - 8, 46, nx - 5, 43, C_GRAY);
}

static void draw_knobs(void)
{
    cv_begin(240, KNOBS_H, C_BLACK);
    for (uint32_t k = 0; k < KNOB_COUNT; k++) {
        draw_knob(k);
    }
    draw_status_line();
    cv_blit(0, KNOBS_Y);
}

static void draw(void)
{
    draw_header();
    draw_scope();
    draw_tracks();
    draw_knobs();
}
