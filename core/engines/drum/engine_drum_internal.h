/*
 * OpenVALive - DRUM engine: shared between its files (not a public API).
 */
#ifndef ENGINE_DRUM_INTERNAL_H
#define ENGINE_DRUM_INTERNAL_H

#include "engine_drum.h"

/* set-up done once per hit, not per sample: built for size where the compiler can (results unchanged) */
#if defined(__clang__)
#define DR_COLD __attribute__((minsize))
#else
#define DR_COLD
#endif

/* engine_drum_voices.c */
void dr_trigger(engine_drum_t *e, dr_inst_t inst, float level);
void dr_render_voice(engine_drum_t *e, dr_inst_t inst, float *mix, int n);

#endif /* ENGINE_DRUM_INTERNAL_H */
