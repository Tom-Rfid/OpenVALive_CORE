/*
 * OpenVALive - PC simulator: saw sweep from 50 Hz to 15 kHz.
 * Listen to it (or look at a spectrogram) to judge the oscillator aliasing.
 */
#include <stdio.h>
#include <stdlib.h>
#include "sim.h"

static engine_va_t synth;

int sim_sweep(const char *path)
{
    const int total = (int)(6.0f * DSP_SR);

    engine_va_init(&synth, 0, 0);
    engine_va_load_preset(&synth, VA_PRESET_INIT);
    engine_va_set_param(&synth, VA_P_MIX_OSC2, 0.0f);   /* one saw only */
    engine_va_set_param(&synth, VA_P_CUTOFF, 1.0f);     /* filter wide open */
    engine_va_set_param(&synth, VA_P_FENV_AMT, 0.5f);
    engine_va_set_param(&synth, VA_P_RESO, 0.0f);
    engine_va_set_param(&synth, VA_P_A_SUSTAIN, 1.0f);
    engine_va_set_param(&synth, VA_P_KEYTRACK, 0.0f);

    float *out_l = calloc((size_t)total, sizeof(float));
    float *out_r = calloc((size_t)total, sizeof(float));

    /* note 31 (~50 Hz) + up to 99 semitones of pitch bend (~15 kHz) */
    engine_va_note_on(&synth, 31, 127);
    for (int pos = 0; pos + DSP_BLOCK <= total; pos += DSP_BLOCK) {
        float t = (float)pos / (float)total;
        engine_va_pitch_bend(&synth, 99.0f * t);
        engine_va_render(&synth, out_l + pos, out_r + pos, DSP_BLOCK);
    }

    signal_report(out_l, out_r, total);
    int rc = wav_write(path, out_l, out_r, total, SIM_SR);
    printf("  -> %s\n", path);
    free(out_l);
    free(out_r);
    return rc;
}
