/* Test helper: renders one note (single saw, filter open) at a given
 * sample rate, as raw mono float32 on stdout.
 *   tone SAMPLE_RATE NOTE SECONDS > out.f32 */
#include <stdio.h>
#include <stdlib.h>
#include "../core/api/ols_synth.h"

static int16_t delay_ram[4096];
static ols_synth_t s;

int main(int argc, char **argv)
{
    float sr = argc > 1 ? (float)atof(argv[1]) : 48000.0f;
    int note = argc > 2 ? atoi(argv[2]) : 69;
    float secs = argc > 3 ? (float)atof(argv[3]) : 1.0f;
    static float l[256], r[256];
    ols_synth_init(&s, sr, delay_ram, 4096);
    ols_synth_set_param(&s, VA_P_MIX_OSC2, 0.0f);
    ols_synth_set_param(&s, VA_P_CUTOFF, 1.0f);
    ols_synth_set_param(&s, VA_P_FENV_AMT, 0.5f);
    ols_synth_set_param(&s, VA_P_KEYTRACK, 0.0f);
    ols_synth_set_param(&s, VA_P_A_SUSTAIN, 1.0f);
    ols_synth_note_on(&s, (uint8_t)note, 127);
    for (int done = 0; done < (int)(sr * secs); done += 256) {
        ols_synth_render_f32(&s, l, r, 256);
        fwrite(l, sizeof(float), 256, stdout);
    }
    return 0;
}
