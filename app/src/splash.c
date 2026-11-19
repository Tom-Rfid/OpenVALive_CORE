/*
 * The boot screen: logo 5C on its orange ground, and the version.
 *
 * Built only for a target that defines APP_SPLASH in its app.h (fm1-base then calls app_splash() instead of writing the
 * name; the SMK-37 calls it from its own boot). The picture is data made by tools/make_splash.py (splash_data.h):
 * the text as alpha masks, the mark as polylines. It is drawn band by band through the app's canvas (240 x 80), so
 * it needs no 115 KB image and no screen-sized buffer. Integer maths only.
 */
#ifdef APP_SPLASH
#include "splash_data.h"

#define SPL_BG    RGB(238, 90, 36)
#define SPL_VIO   RGB(58, 28, 140)
#define SPL_MINT  RGB(90, 200, 160)
#define SPL_WHITE RGB(255, 255, 255)
#define SPL_BAND  80

/* colour c over the canvas pixel at screen (x, y), alpha a = 0..15; outside the band: nothing */
static void spl_blend(int32_t x, int32_t y, uint16_t c, uint32_t a)
{
    y += cv_oy;
    if (!a || (uint32_t)x >= cv_w || y < cv_y0 || y >= cv_y1)
        return;
    uint16_t *p = &cv_px[(uint32_t)y * cv_w + (uint32_t)x];
    if (a >= 15u) {
        *p = swap16(c);
        return;
    }
    uint32_t bg = swap16(*p);
    uint32_t r = ((c >> 11) * a + (bg >> 11) * (15u - a)) / 15u;
    uint32_t g = (((c >> 5) & 63u) * a + ((bg >> 5) & 63u) * (15u - a)) / 15u;
    uint32_t b = ((c & 31u) * a + (bg & 31u) * (15u - a)) / 15u;
    *p = swap16((r << 11) | (g << 5) | b);
}

/* a run-length coded alpha mask (splash_data.h) in colour c */
static void spl_mask(const spl_mask_t *m, uint16_t c)
{
    uint32_t i = 0, n = (uint32_t)m->w * (uint32_t)m->h;
    const uint8_t *p = m->rle;
    while (i < n) {
        uint32_t a = *p >> 4, run = (*p & 15u) + 1u;
        p++;
        for (; run && i < n; run--, i++)
            spl_blend(m->x + (int32_t)(i % (uint32_t)m->w), m->y + (int32_t)(i / (uint32_t)m->w), c, a);
    }
}

/* a round-ended segment of half-width r (all in 1/16 px), smoothed edge, clipped to the rows y0 .. y1 */
static void spl_seg(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t r, uint16_t c, int32_t ya, int32_t yb)
{
    int32_t dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
    int32_t lo = (r + 8) * (r + 8), hi = r > 8 ? (r - 8) * (r - 8) : 0;    /* (r +- half a pixel)^2 */
    int32_t px0 = ((x0 < x1 ? x0 : x1) - r) / 16 - 1, px1 = ((x0 > x1 ? x0 : x1) + r) / 16 + 1;
    int32_t py0 = ((y0 < y1 ? y0 : y1) - r) / 16 - 1, py1 = ((y0 > y1 ? y0 : y1) + r) / 16 + 1, px, py;
    if (py0 < ya)
        py0 = ya;
    if (py1 > yb)
        py1 = yb;
    for (py = py0; py <= py1; py++)
        for (px = px0; px <= px1; px++) {
            int32_t cx = px * 16 + 8 - x0, cy = py * 16 + 8 - y0, t = l2 ? cx * dx + cy * dy : 0, d2;
            if (t < 0)
                t = 0;
            if (t > l2)
                t = l2;
            if (l2) {                                  /* the nearest point of the segment */
                cx -= dx * t / l2;
                cy -= dy * t / l2;
            }
            d2 = cx * cx + cy * cy;
            if (d2 < lo)
                spl_blend(px, py, c, d2 <= hi ? 15u : (uint32_t)(15 * (lo - d2) / (lo - hi)));
        }
}

static void spl_line(const spl_pt_t *p, uint32_t n, int32_t r, uint16_t c, int32_t ya, int32_t yb)
{
    uint32_t i;
    for (i = 0; i + 1u < n; i++)
        spl_seg(p[i].x, p[i].y, p[i + 1u].x, p[i + 1u].y, r, c, ya, yb);
}

void app_splash(void)
{
    char ver[24];
    uint32_t band;
    app_version_text(ver, sizeof ver);
    for (band = 0; band < 240u / SPL_BAND; band++) {
        int32_t ya = (int32_t)(band * SPL_BAND), yb = ya + SPL_BAND - 1;
        cv_begin(240, SPL_BAND, SPL_BG);
        cv_oy = -ya;                                   /* screen rows ya .. yb land in this band */
        cv_y0 = 0;
        cv_y1 = SPL_BAND;
        spl_mask(&SPL_OPEN, SPL_WHITE);
        spl_mask(&SPL_VA, SPL_MINT);                   /* OpenVALive: the VA in mint */
        spl_mask(&SPL_LIVE, SPL_VIO);
        spl_seg(SPL_DOT_X16, SPL_DOT_Y16, SPL_DOT_X16, SPL_DOT_Y16, SPL_DOT_R16, SPL_MINT, ya, yb);
        spl_mask(&SPL_SUB, SPL_MINT);
        spl_line(SPL_WAVE_BACK, sizeof SPL_WAVE_BACK / sizeof SPL_WAVE_BACK[0], SPL_WAVE_R16, SPL_MINT, ya, yb);
        spl_line(SPL_CURVE, sizeof SPL_CURVE / sizeof SPL_CURVE[0], SPL_CURVE_R16, SPL_VIO, ya, yb);
        spl_line(SPL_WAVE_FRONT, sizeof SPL_WAVE_FRONT / sizeof SPL_WAVE_FRONT[0], SPL_WAVE_R16, SPL_MINT, ya, yb);
        spl_seg(SPL_CUTOFF_X16, SPL_CUTOFF_Y16, SPL_CUTOFF_X16, SPL_CUTOFF_Y16, SPL_CUTOFF_R16, SPL_WHITE, ya, yb);
        cv_text(SPL_TITLE_X + SPL_INSET, SPL_VERSION_Y, &FONT_XS, ver, SPL_VIO);
        cv_oy = 0;
        cv_blit(0, (uint32_t)ya);
    }
}
#endif /* APP_SPLASH */
