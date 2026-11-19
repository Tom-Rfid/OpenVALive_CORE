/*
 * OpenVALive - PC simulator: CPU cost per case and static RAM.
 *
 * These numbers are for THIS computer. On the FM-1, the app shows the real
 * load on screen (measured by fm1-base). Measured there: ~5 % per voice.
 */
#include <stdio.h>
#include <time.h>
#include "sim.h"

static engine_va_t synth;
static int16_t delay_ram[SIM_DELAY_LEN];

/* Returns nanoseconds per stereo sample. */
static double bench_case(int voices, int sub_and_noise, int drive, int effects)
{
    engine_va_init(&synth, effects ? delay_ram : 0, effects ? SIM_DELAY_LEN : 0);
    engine_va_load_preset(&synth, VA_PRESET_INIT);
    engine_va_set_param(&synth, VA_P_A_SUSTAIN, 1.0f);
    engine_va_set_param(&synth, VA_P_MIX_SUB, sub_and_noise ? 0.5f : 0.0f);
    engine_va_set_param(&synth, VA_P_MIX_NOISE, sub_and_noise ? 0.3f : 0.0f);
    engine_va_set_param(&synth, VA_P_DRIVE, drive ? 0.5f : 0.0f);
    engine_va_set_param(&synth, VA_P_CHORUS_MODE, effects ? 0.3f : 0.0f);
    engine_va_set_param(&synth, VA_P_DELAY_MIX, effects ? 0.3f : 0.0f);
    for (int v = 0; v < voices; v++) {
        engine_va_note_on(&synth, (uint8_t)(48 + 4 * v), 100);
    }

    float l[DSP_BLOCK], r[DSP_BLOCK];
    const int blocks = (int)(30.0f * DSP_SR / DSP_BLOCK);   /* 30 s of audio */
    volatile float sink = 0.0f;

    clock_t t0 = clock();
    for (int b = 0; b < blocks; b++) {
        engine_va_render(&synth, l, r, DSP_BLOCK);
        sink += l[0];
    }
    double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
    (void)sink;
    return secs * 1e9 / ((double)blocks * DSP_BLOCK);
}

void sim_bench(void)
{
    printf("CPU cost on this PC (ns per stereo sample; one sample lasts %.1f us at %d Hz)\n",
           1e6 / SIM_SR, SIM_SR);
    printf("  %-40s %8s\n", "case", "ns/smp");
    printf("  %-40s %8.1f\n", "0 voices (effects only)", bench_case(0, 0, 0, 1));
    printf("  %-40s %8.1f\n", "1 voice, 2 osc, no effects", bench_case(1, 0, 0, 0));
    printf("  %-40s %8.1f\n", "4 voices, 2 osc, no effects", bench_case(4, 0, 0, 0));
    printf("  %-40s %8.1f\n", "4 voices, 2 osc + sub + noise + drive", bench_case(4, 1, 1, 0));
    printf("  %-40s %8.1f\n", "4 full voices + delay + chorus", bench_case(4, 1, 1, 1));

    printf("\nStatic RAM:\n");
    printf("  engine_va_t (4 voices, chorus included) : %zu bytes\n", sizeof(engine_va_t));
    printf("  one voice                               : %zu bytes\n", sizeof(va_voice_t));
    printf("  chorus                                  : %zu bytes\n", sizeof(dsp_chorus_t));
    printf("  delay buffer (given by the platform)    : %zu bytes (%.0f ms)\n",
           sizeof(int16_t) * SIM_DELAY_LEN, SIM_DELAY_LEN * 1000.0 / SIM_SR);
    printf("  midi_in_t                               : %zu bytes\n", sizeof(midi_in_t));
}
