/* OpenVALive - the boot screen on a PC: tools/splash_preview OUT.ppm (no version text: the font is the device's).
 *   cc -std=c99 -O2 -I app/src -o build/splash_preview tools/splash_preview.c */
#include <stdint.h>
#include <stdio.h>
#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
static uint16_t screen[240 * 240], cv_px[240 * 80];
static uint32_t cv_w, cv_h;
static int32_t cv_oy, cv_y0, cv_y1;
static inline uint16_t swap16(uint32_t c) { return (uint16_t)(((c >> 8) & 0xFFu) | ((c & 0xFFu) << 8)); }
static void cv_begin(uint32_t w, uint32_t h, uint16_t bg)
{
    uint32_t i;
    cv_w = w; cv_h = h; cv_oy = cv_y0 = 0; cv_y1 = (int32_t)h;
    for (i = 0; i < w * h; i++)
        cv_px[i] = swap16(bg);
}
static void cv_blit(uint32_t x, uint32_t y)
{
    uint32_t i, j;
    for (j = 0; j < cv_h; j++)
        for (i = 0; i < cv_w; i++)
            screen[(y + j) * 240u + x + i] = swap16(cv_px[j * cv_w + i]);
}
typedef int felucca_font_t;
static const felucca_font_t FONT_XS = 0;
static int32_t cv_text(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    (void)f; (void)c;
    fprintf(stderr, "text at %d,%d: %s\n", (int)x, (int)y, s);
    return x;
}
#define APP_VERSION "0.0.3"
#define APP_SPLASH
#include "app_version.c"
#include "splash.c"
int main(int argc, char **argv)
{
    FILE *f = fopen(argc > 1 ? argv[1] : "splash.ppm", "wb");
    uint32_t i;
    if (!f)
        return 1;
    app_splash();
    fprintf(f, "P6 240 240 255\n");
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t c = screen[i];
        fputc((c >> 11) << 3, f); fputc(((c >> 5) & 63u) << 2, f); fputc((c & 31u) << 3, f);
    }
    return fclose(f);
}
