/*
 * OpenVALive - FM-1 build configuration (included first by every FM-1 unit)
 */
#pragma once

/* no libm on the device: use fastmath.h from fm1-base */
#define OLS_FASTMATH 1

/* fixed sample rate: the real ALNK0 clock, 24 MHz / 544 = 44117.647 Hz
 * (another target sharing this app sets its own first: the SMK-37 runs at 44100 Hz) */
#ifndef DSP_SAMPLE_RATE
#define DSP_SAMPLE_RATE 44118
#define DSP_SR_F (24000000.0f / 544.0f)
#endif

/* control block: 32 samples = 0.73 ms */
#define DSP_BLOCK 32
