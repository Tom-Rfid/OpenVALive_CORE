/* Test: every synth sound plays a held note (C3 and C5, 2 s, then 2 s released): no NaN, it sounds (peak > -40 dB),
 * it does not blow up (peak < 4), and it ends quiet. Prints one line per failing sound, then a summary.
 *   presets_test */
#include <stdio.h>
#include <math.h>
#include "../core/api/ols_synth.h"

static int16_t delay_ram[48000];
static ols_synth_t s;

int main(void)
{
    static float l[256], r[256];
    const float sr = 44100.0f;
    int bad = 0;
    for (int p = 0; p < VA_PRESET_COUNT; p++) {
        float peak = 0.0f, tail = 0.0f;
        int nan = 0;
        ols_synth_init(&s, sr, delay_ram, 48000);
        ols_synth_load_preset(&s, (va_preset_t)p);
        ols_synth_note_on(&s, 48, 110);
        ols_synth_note_on(&s, 72, 110);
        for (int done = 0; done < (int)(sr * 8.0f); done += 256) {
            if (done >= (int)(sr * 2.0f) && done < (int)(sr * 2.0f) + 256) {
                ols_synth_note_off(&s, 48);
                ols_synth_note_off(&s, 72);
            }
            ols_synth_render_f32(&s, l, r, 256);
            for (int i = 0; i < 256; i++) {
                float a = fabsf(l[i]) > fabsf(r[i]) ? fabsf(l[i]) : fabsf(r[i]);
                nan |= !(a == a) || a > 1e9f;
                peak = a > peak ? a : peak;
                if (done >= (int)(sr * 7.5f)) {
                    tail = a > tail ? a : tail;
                }
            }
        }
        if (nan || peak < 0.01f || peak > 4.0f || tail > 0.05f) {
            printf("%s: peak %.3f tail %.3f%s\n", VA_PRESET_NAMES[p], peak, tail, nan ? " NaN" : "");
            bad++;
        }
    }
    printf("sounds: %d synth sounds play and end, %d failed\n", VA_PRESET_COUNT, bad);
    return bad != 0;
}
