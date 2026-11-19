/*
 * OpenVALive - DSP Core
 * dsp_runtime.c : sample rate chosen at run time.
 *
 * Only compile this file when DSP_SR_F / DSP_SAMPLE_RATE are NOT defined
 * (the FM-1 build uses a fixed rate and does not need it).
 */
#include "dsp_common.h"

#ifndef DSP_SR_F

float dsp_sr = 48000.0f;
float dsp_inv_sr = 1.0f / 48000.0f;

void dsp_set_sample_rate(float sample_rate_hz)
{
    if (sample_rate_hz < 8000.0f) {
        sample_rate_hz = 8000.0f;
    }
    if (sample_rate_hz > 192000.0f) {
        sample_rate_hz = 192000.0f;
    }
    dsp_sr = sample_rate_hz;
    dsp_inv_sr = 1.0f / sample_rate_hz;
}

#endif
