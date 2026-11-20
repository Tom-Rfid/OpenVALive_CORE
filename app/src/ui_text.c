/*
 * OpenVALive - FM-1 app: small text helpers and on-screen messages.
 * (No C library on the device: no sprintf, so we build strings by hand.)
 */

static void text_cat(char *dst, const char *src)
{
    while (*dst) {
        dst++;
    }
    while (*src) {
        *dst++ = *src++;
    }
    *dst = 0;
}

static void int_text(int32_t value, char *out)
{
    char digits[12];
    int n = 0;
    uint32_t u = (uint32_t)(value < 0 ? -value : value);

    do {
        digits[n++] = (char)('0' + u % 10u);
        u /= 10u;
    } while (u && n < 11);

    if (value < 0) {
        *out++ = '-';
    }
    while (n) {
        *out++ = digits[--n];
    }
    *out = 0;
}

static void octave_text(int32_t octave, char *out)
{
    out[0] = octave < 0 ? '-' : '+';
    out[1] = (char)('0' + (octave < 0 ? -octave : octave));
    out[2] = 0;
}

static void copy_text(char *dst, const char *src, uint32_t max)
{
    uint32_t i;
    for (i = 0; i + 1 < max && src && src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = 0;
}

/* A short message over the header for 1.2 s, also spoken by a screen reader. */
void app_say(const char *a, const char *b)
{
    copy_text(ui.msg[0], a, sizeof ui.msg[0]);
    copy_text(ui.msg[1], b, sizeof ui.msg[1]);
    ui.msg_until = plat_ms() + 1200u;
    ui.msg_style = MSG_PLAIN;
    ui.msg_right = 0;
    ui.msg_inf = 0;
    ui.redraw = 1;

    if (ann_on()) {
        char spoken[48] = "";
        text_cat(spoken, ui.msg[0]);
        text_cat(spoken, " ");
        text_cat(spoken, ui.msg[1]);
        ann_say(spoken, 0, ANN_NOW);
    }
}

/* A list longer than its view: a thin bar on the right edge (x 237-239) shows where the view is in it. Drawn a band
 * at a time (the canvas holds a few rows): `band_y` is where this band starts in the view, `band_h` its height, the
 * view `view_h` high shows rows top .. top + visible of total. Nothing when it all fits. */
static void scroll_bar(int32_t band_y, int32_t band_h, int32_t view_h, uint32_t top, uint32_t visible, uint32_t total)
{
    if (total <= visible || view_h <= 0) {
        return;
    }
    int32_t th = (int32_t)(visible * (uint32_t)view_h / total);
    th = th < 10 ? 10 : th;
    int32_t ty = (int32_t)(top * (uint32_t)(view_h - th) / (total - visible));
    for (int32_t y = 0; y < band_h; y++) {
        int32_t v = band_y + y;
        if (v >= ty && v < ty + th) {
            cv_rect(236, y, 3, 1, RGB(150, 150, 165));
        } else {
            cv_rect(237, y, 1, 1, RGB(50, 50, 58));
        }
    }
}
