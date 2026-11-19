/*
 * OpenVALive - PC simulator: 16-bit stereo WAV writer and signal checks.
 */
#include <stdio.h>
#include <math.h>
#include "sim.h"

static void put_u32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void put_u16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }

int wav_write(const char *path, const float *left, const float *right, int frames, int sample_rate)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        return -1;
    }

    uint32_t data_bytes = (uint32_t)frames * 4;
    fwrite("RIFF", 1, 4, f);
    put_u32(f, 36 + data_bytes);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    put_u32(f, 16);                         /* fmt chunk size */
    put_u16(f, 1);                          /* PCM */
    put_u16(f, 2);                          /* stereo */
    put_u32(f, (uint32_t)sample_rate);
    put_u32(f, (uint32_t)sample_rate * 4);  /* bytes per second */
    put_u16(f, 4);                          /* bytes per frame */
    put_u16(f, 16);                         /* bits per sample */
    fwrite("data", 1, 4, f);
    put_u32(f, data_bytes);

    for (int i = 0; i < frames; i++) {
        int16_t s[2];
        s[0] = (int16_t)lrintf(dsp_clampf(left[i], -1.0f, 1.0f) * 32767.0f);
        s[1] = (int16_t)lrintf(dsp_clampf(right[i], -1.0f, 1.0f) * 32767.0f);
        fwrite(s, 2, 2, f);
    }
    fclose(f);
    return 0;
}

/* Prints peak, RMS, NaN/Inf count and clipped samples. The tests read this line. */
void signal_report(const float *left, const float *right, int frames)
{
    double peak = 0.0, sum_sq = 0.0;
    int bad = 0, clipped = 0;

    for (int i = 0; i < frames; i++) {
        double a = left[i], b = right[i];
        if (isnan(a) || isnan(b) || isinf(a) || isinf(b)) {
            bad++;
            continue;
        }
        double m = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
        if (m > peak) {
            peak = m;
        }
        if (m > 0.999) {
            clipped++;
        }
        sum_sq += a * a + b * b;
    }
    double rms = sqrt(sum_sq / (2.0 * frames));
    printf("  peak %.3f (%.1f dBFS) | rms %.1f dBFS | NaN/Inf: %d | clipped: %d\n",
           peak, 20.0 * log10(peak + 1e-9), 20.0 * log10(rms + 1e-12), bad, clipped);
}
