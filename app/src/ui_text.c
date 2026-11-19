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
    ui.redraw = 1;

    if (ann_on()) {
        char spoken[48] = "";
        text_cat(spoken, ui.msg[0]);
        text_cat(spoken, " ");
        text_cat(spoken, ui.msg[1]);
        ann_say(spoken, 0, ANN_NOW);
    }
}
