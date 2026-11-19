#!/usr/bin/env python3
"""OpenVALive's boot screen (logo 5C) -> app/src/splash_data.h

    python3 tools/make_splash.py --space-grotesk SpaceGrotesk[wght].ttf --archivo Archivo[wdth,wght].ttf

The screen is 240 x 240: an orange ground, "OpenVALive" (Space Grotesk 600, up to 48 px; "Open" white, "VA" mint, "Live" violet, the i's
dot mint), "VIRTUAL ANALOG" spread under it (Archivo 125 % 800, 15 px, mint), the mark (a violet resonance curve, a
noisy mint wave passing behind it then in front, a white cutoff dot) and, written by the firmware itself, the version.

The text becomes three alpha masks (4-bit, run-length coded: a byte = alpha << 4 | run - 1), the mark polylines in
1/16 px: about 4 KB in all, drawn by app/src/splash.c. The fonts (SIL Open Font Licence) are needed only to
regenerate the header, which is committed. Fonts: github.com/google/fonts (ofl/spacegrotesk, ofl/archivo).
"""
import argparse, math, pathlib
from PIL import Image, ImageDraw, ImageFont

ap = argparse.ArgumentParser()
ap.add_argument("--space-grotesk", required=True)
ap.add_argument("--archivo", required=True)
ap.add_argument("--out", default=str(pathlib.Path(__file__).resolve().parents[1] / "app/src/splash_data.h"))
a = ap.parse_args()

def font(path, px, axes):
    f = ImageFont.truetype(path, px)
    f.set_variation_by_axes(axes)
    return f

SUB = font(a.archivo, 15, [800, 125])        # axes: weight, width
BIG_PX = 48                                  # the title's size: 48 px, smaller until "OpenVALive" fits in 212 px
while True:
    BIG = font(a.space_grotesk, BIG_PX, [600])
    if BIG.getlength("OpenVALive") <= 212 or BIG_PX <= 30:
        break
    BIG_PX -= 1
TITLE_TOP, SUB_GAP, INSET = 52, 9, 3        # the title's ink top (px), the gap to VIRTUAL ANALOG, its left inset
VERSION_TOP, MARK = 222, 98                 # the version's top (the firmware writes it), the mark's size

def ink(draw_fn):
    """draw on a 240 x 240 alpha plane, return (x, y, w, h, alphas 0..15) of what was drawn"""
    im = Image.new("L", (240, 240), 0)
    draw_fn(ImageDraw.Draw(im))
    x0, y0, x1, y1 = im.getbbox()
    crop = im.crop((x0, y0, x1, y1))
    return x0, y0, x1 - x0, y1 - y0, [(p * 15 + 127) // 255 for p in crop.getdata()]

def rle(alphas):
    out, i = [], 0
    while i < len(alphas):
        j = i
        while j < len(alphas) and alphas[j] == alphas[i] and j - i < 16:
            j += 1
        out.append(alphas[i] << 4 | (j - i - 1))
        i = j
    return out

# the title: where its ink starts, so the ink's top lands on TITLE_TOP and the word is centred
probe = Image.new("L", (400, 120), 0)
ImageDraw.Draw(probe).text((0, 0), "OpenVALıve", font=BIG, fill=255)
bx0, by0, bx1, by1 = probe.getbbox()
w_open = BIG.getlength("Open")
w_va = BIG.getlength("VA")
tx, ty = (240 - (bx1 - bx0)) / 2 - bx0, TITLE_TOP - by0
title_l, title_r, title_b = tx + bx0, tx + bx1, TITLE_TOP + (by1 - by0)

OPEN = ink(lambda d: d.text((tx, ty), "Open", font=BIG, fill=255))
VA = ink(lambda d: d.text((tx + w_open, ty), "VA", font=BIG, fill=255))
LIVE = ink(lambda d: d.text((tx + w_open + w_va, ty), "Lıve", font=BIG, fill=255))
# the i's dot, mint: over the dotless i, its top on the L's top
xi = tx + w_open + w_va + BIG.getlength("L") + BIG.getlength("ı") / 2
dot_r = BIG_PX * 0.095
DOT = (xi, LIVE[1] + dot_r, dot_r)

# VIRTUAL ANALOG, spread from the O (plus INSET) to the e
letters = "VIRTUAL ANALOG"
widths = [SUB.getlength(c) for c in letters]
gap = (title_r - title_l - INSET - sum(widths)) / (len(letters) - 1)
def sub(d):
    x = title_l + INSET
    for c, w in zip(letters, widths):
        d.text((x, title_b + SUB_GAP), c, font=SUB, fill=255, anchor="la")
        x += w + gap
SUBM = ink(sub)

# the mark, in a 120-unit box scaled to MARK px, centred between VIRTUAL ANALOG and the version
mx = (240 - MARK) / 2
my = SUBM[1] + SUBM[3] + (VERSION_TOP - (SUBM[1] + SUBM[3]) - MARK) / 2
k = MARK / 120.0
def bez(p0, p1, p2, p3, n=24):
    return [tuple((1 - t) ** 3 * q0 + 3 * (1 - t) ** 2 * t * q1 + 3 * (1 - t) * t ** 2 * q2 + t ** 3 * q3
                  for q0, q1, q2, q3 in zip(p0, p1, p2, p3)) for t in (i / n for i in range(n + 1))]
curve = [(12, 64), (46, 64)] + bez((46, 64), (58, 64), (60, 30), (70, 30))[1:] + bez((70, 30), (80, 30), (84, 72), (108, 102))[1:]
wave, x = [], 12.0
while x <= 108.0:
    t = (x - 12.0) / 96.0
    amp = 20.0 * (0.62 + 0.38 * math.sin(2 * math.pi * (t * 1.3 + 0.15)))
    y = 66.0 - amp * math.sin(2 * math.pi * t * 2.6) + 1.6 * math.sin(x * 91.7) * math.cos(x * 37.3 + 1.1)
    wave.append((x, y))
    x += 1.5
split = next(i for i, (x, _) in enumerate(wave) if x >= 76)
to16 = lambda pts: [(round((mx + x * k) * 16), round((my + y * k) * 16)) for x, y in pts]

def c_mask(name, m):
    x, y, w, h, al = m
    data = rle(al)
    rows = [", ".join(f"0x{b:02x}" for b in data[i:i + 20]) for i in range(0, len(data), 20)]
    return (f"static const uint8_t {name}_RLE[{len(data)}] = {{\n    " + ",\n    ".join(rows) + "\n};\n"
            f"static const spl_mask_t {name} = {{{x}, {y}, {w}, {h}, {name}_RLE}};\n"), len(data)

def c_pts(name, pts):
    rows = [", ".join(f"{{{x}, {y}}}" for x, y in pts[i:i + 8]) for i in range(0, len(pts), 8)]
    return f"static const spl_pt_t {name}[{len(pts)}] = {{\n    " + ",\n    ".join(rows) + "\n};\n", 4 * len(pts)

parts, total = [], 0
for name, m in (("SPL_OPEN", OPEN), ("SPL_VA", VA), ("SPL_LIVE", LIVE), ("SPL_SUB", SUBM)):
    s, n = c_mask(name, m); parts.append(s); total += n
for name, pts in (("SPL_WAVE_BACK", to16(wave[:split + 1])), ("SPL_CURVE", to16(curve)), ("SPL_WAVE_FRONT", to16(wave[split:]))):
    s, n = c_pts(name, pts); parts.append(s); total += n

head = f"""/* OpenVALive's boot screen, made by tools/make_splash.py: do not edit. About {total} bytes. */
#pragma once
#include <stdint.h>

typedef struct {{ int16_t x, y, w, h; const uint8_t *rle; }} spl_mask_t;   /* alpha << 4 | run - 1, rows of w */
typedef struct {{ int16_t x, y; }} spl_pt_t;                                 /* 1/16 px */

#define SPL_TITLE_X {round(title_l)}          /* the O's left edge (the version starts here, + SPL_INSET) */
#define SPL_INSET {INSET}
#define SPL_VERSION_Y {VERSION_TOP}
#define SPL_DOT_X16 {round(DOT[0] * 16)}
#define SPL_DOT_Y16 {round(DOT[1] * 16)}
#define SPL_DOT_R16 {round(DOT[2] * 16)}
#define SPL_CUTOFF_X16 {round((mx + 70 * k) * 16)}
#define SPL_CUTOFF_Y16 {round((my + 30 * k) * 16)}
#define SPL_CUTOFF_R16 {round(9.5 * k * 16)}
#define SPL_CURVE_R16 {round(4.5 * k * 16)}       /* half the stroke */
#define SPL_WAVE_R16 {round(3.0 * k * 16)}

"""
pathlib.Path(a.out).write_text(head + "\n".join(parts))
print(f"{a.out}: {total} bytes of data")
