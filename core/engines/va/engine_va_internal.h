/*
 * OpenVALive - VA engine, functions shared between its source files.
 * Not part of the public API: only engine_va*.c include this header.
 */
#ifndef ENGINE_VA_INTERNAL_H
#define ENGINE_VA_INTERNAL_H

#include "engine_va.h"

/* engine_va_params.c */
void va_update_envelopes(engine_va_t *e);
void va_update_delay(engine_va_t *e);

/* engine_va_voices.c */
void va_voices_init(engine_va_t *e);
void va_voice_stop(va_voice_t *v);

/* engine_va_params.c */
float va_effective(const engine_va_t *e, uint16_t id);

/* engine_va_mod.c */
void va_mod_update(engine_va_t *e);              /* after a MOD / LFO timing parameter changed */
void va_mod_engine_tick(engine_va_t *e);         /* control rate, before the voices: LFOs, engine-wide targets */
void va_mod_voice(engine_va_t *e, va_voice_t *v, float m[VA_DST_COUNT]);   /* per voice: its modulation */
void va_lfo_trigger(engine_va_t *e);             /* the first key of a phrase */

/* engine_va_render.c */
void va_smooth_step(engine_va_t *e);
void va_render_voice(engine_va_t *e, va_voice_t *v, float *mix, int n);

#endif /* ENGINE_VA_INTERNAL_H */
