/* The synth sounds' loudness, measured the same way for each, so their levels can be set to one target
 * (tools/level_sounds.py does that: it runs this, sets each sound's VOLUME, and runs it again).
 *
 * Each sound plays the same phrase (C3 G3 C4 G4, velocity 100, 1 s each, a short gap between: pads and risers reach
 * their level) through ols_synth.
 * Its loudness is a short-term K-weighted level (the ear's curve of ITU-R BS.1770: low end down, presence up) in
 * 100 ms windows; each note counts by its loudest window (a pluck by its strike, a pad once it has swelled), and the
 * sound's loudness is the power average of its four notes: how loud it sounds when it plays, whatever its shape.
 * Printed for each: index, name, loudness (dB), peak (dB), mean level while on (dB).
 *   cc -std=c99 -O2 -o level_sounds tools/level_sounds.c core/api/ols_synth.c <the core sources> -lm */
#include <math.h>
#include <stdio.h>
#include "../core/api/ols_synth.h"

#define SR 44100.0f
#define WIN 4410                                /* 100 ms */
#define MAX_WINS 64
#define NOTE_S 1.0f                              /* each note held */
#define STEP_S 1.1f                              /* note to note */

static int16_t delay_ram[48000];
static ols_synth_t s;

typedef struct { float b0, b1, b2, a1, a2, x1, x2, y1, y2; } biquad_t;

static float bq(biquad_t *f, float x)
{
    float y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2 - f->a1 * f->y1 - f->a2 * f->y2;
    f->x2 = f->x1; f->x1 = x; f->y2 = f->y1; f->y1 = y;
    return y;
}

/* RBJ cookbook: high shelf (+4 dB at 1681 Hz) and high-pass (38 Hz): BS.1770's K-weighting at this rate */
static void k_weighting(biquad_t *shelf, biquad_t *hp)
{
    double A = pow(10.0, 4.0 / 40.0), w = 2.0 * M_PI * 1681.0 / SR, c = cos(w), sn = sin(w), al = sn / 2.0 * sqrt(2.0);
    double a0 = (A + 1) - (A - 1) * c + 2 * sqrt(A) * al;
    *shelf = (biquad_t){ (float)(A * ((A + 1) + (A - 1) * c + 2 * sqrt(A) * al) / a0),
                         (float)(-2 * A * ((A - 1) + (A + 1) * c) / a0),
                         (float)(A * ((A + 1) + (A - 1) * c - 2 * sqrt(A) * al) / a0),
                         (float)(2 * ((A - 1) - (A + 1) * c) / a0),
                         (float)(((A + 1) - (A - 1) * c - 2 * sqrt(A) * al) / a0), 0, 0, 0, 0 };
    w = 2.0 * M_PI * 38.0 / SR; c = cos(w); sn = sin(w); al = sn / (2.0 * 0.5);
    a0 = 1 + al;
    *hp = (biquad_t){ (float)((1 + c) / 2 / a0), (float)(-(1 + c) / a0), (float)((1 + c) / 2 / a0),
                      (float)(-2 * c / a0), (float)((1 - al) / a0), 0, 0, 0, 0 };
}

static float db(float x) { return 20.0f * log10f(x > 1e-9f ? x : 1e-9f); }

int main(void)
{
    static const uint8_t NOTE[4] = { 48, 55, 60, 67 };
    static float l[WIN], r[WIN];
    for (int p = 0; p < VA_PRESET_COUNT; p++) {
        biquad_t shelf, hp;
        k_weighting(&shelf, &hp);
        ols_synth_init(&s, SR, delay_ram, 48000);
        ols_synth_load_preset(&s, (va_preset_t)p);
        float win_db[MAX_WINS], peak = 0.0f, note_max[4] = { -200.0f, -200.0f, -200.0f, -200.0f };
        int wins = 0;
        double abs_sum = 0.0;
        long abs_n = 0;
        for (int w = 0; w < 52 && wins < MAX_WINS; w++) {      /* 5.2 s: 4 notes, then the tails */
            int t = w * WIN;                                    /* (samples) */
            for (int k = 0; k < 4; k++) {
                if (t == (int)(k * STEP_S * SR) / WIN * WIN)
                    ols_synth_note_on(&s, NOTE[k], 100);
                if (t == (int)((k * STEP_S + NOTE_S) * SR) / WIN * WIN)
                    ols_synth_note_off(&s, NOTE[k]);
            }
            ols_synth_render_f32(&s, l, r, WIN);
            double e = 0.0, a = 0.0;
            for (int i = 0; i < WIN; i++) {
                float m = 0.5f * (l[i] + r[i]);
                float k = bq(&hp, bq(&shelf, m));
                e += (double)k * k;
                a += fabs(m);
                float pk = fabsf(l[i]) > fabsf(r[i]) ? fabsf(l[i]) : fabsf(r[i]);
                peak = pk > peak ? pk : peak;
            }
            win_db[wins] = db((float)sqrt(e / WIN));
            int k = (int)((float)t / (STEP_S * SR));
            if (k < 4 && win_db[wins] > note_max[k])
                note_max[k] = win_db[wins];
            wins++;
            abs_sum += a;
            abs_n += WIN;
            (void)abs_n;
        }
        float top = -200.0f;
        for (int i = 0; i < wins; i++)
            top = win_db[i] > top ? win_db[i] : top;
        int n = 0;
        double mean = 0.0;
        for (int i = 0; i < wins; i++) {
            if (win_db[i] > top - 30.0f) {     /* (the mean while on: windows within 30 dB of the loudest) */
                n++;
                mean += pow(10.0, win_db[i] / 20.0);
            }
        }
        double pw = 0.0;
        for (int k = 0; k < 4; k++)
            pw += pow(10.0, note_max[k] / 10.0) / 4.0;
        float loud = (float)(10.0 * log10(pw));
        printf("%d\t%s\t%.2f\t%.2f\t%.2f\n", p, VA_PRESET_NAMES[p], loud, db(peak), db((float)(mean / (n ? n : 1))));
    }
    return 0;
}
