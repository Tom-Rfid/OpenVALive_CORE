/*
 * OpenVALive - common engine interface
 *
 * Every instrument (VA, Acid, FM, Chiptune, Ambient...) exposes this table.
 * The Live Core (sequencer, scenes, morph, undo) only talks to this
 * interface: it never needs to know which engine is running behind it.
 *
 * Parameters are normalised to 0..1, so a knob, a MIDI CC, a Morph macro or
 * a scene all write exactly the same kind of value.
 */
#ifndef OLS_ENGINE_H
#define OLS_ENGINE_H

#include <stdint.h>

/* How a parameter behaves when scenes or Morph interpolate it. */
enum {
    OLS_PARAM_CONTINUOUS = 0,   /* interpolated smoothly (cutoff, levels, times...) */
    OLS_PARAM_DISCRETE = 1      /* switches at the half-way point (waveform, mode...) */
};

/* Roles: the parameters a performance gesture means, whatever the engine.
 * "Filter shut on every track" asks each engine for its OLS_ROLE_CUTOFF. */
enum {
    OLS_ROLE_CUTOFF,
    OLS_ROLE_RESO,
    OLS_ROLE_DRIVE,
    OLS_ROLE_DELAY_MIX,
    OLS_ROLE_VOLUME,
    OLS_ROLE_CRUSH,
    OLS_ROLE_DECAY,       /* how long sounds ring (envelope decays) */
    OLS_ROLE_RELEASE,     /* tails after the key */
    OLS_ROLE_FENV,        /* filter envelope amount */
    OLS_ROLE_CHORUS,      /* width / modulation effects */
    OLS_ROLE_COUNT
};
#define OLS_NO_PARAM 0xFFFFu
#define OLS_MACROS 4             /* parameters on the four performance knobs */

typedef struct {
    const char *name;
    uint16_t param_count;
    const char *const *param_names;   /* short names for the UI (8 chars max advised) */
    const uint8_t *param_kinds;       /* OLS_PARAM_CONTINUOUS / OLS_PARAM_DISCRETE, per parameter */

    void  (*note_on)(void *ctx, uint8_t note, uint8_t velocity);
    void  (*note_off)(void *ctx, uint8_t note);
    void  (*all_notes_off)(void *ctx);
    void  (*set_param)(void *ctx, uint16_t id, float value01);
    float (*get_param)(void *ctx, uint16_t id);
    void  (*pitch_bend)(void *ctx, float semitones);

    /* Renders n stereo samples into out_l / out_r, in [-1, 1].
     * Any n works; multiples of DSP_BLOCK are the most efficient. */
    void  (*render)(void *ctx, float *out_l, float *out_r, int n);

    /* optional (0 = none) */
    const uint16_t *roles;            /* OLS_ROLE_COUNT parameter ids, OLS_NO_PARAM if the engine has none */
    const uint16_t *macros;           /* OLS_MACROS parameter ids for the knobs */
    const char *const *macro_names;   /* their labels (6 characters) */

    /* optional: how hard the keys are held (aftertouch), 0..1, for expression */
    void  (*pressure)(void *ctx, float amount01);

    /* optional: a performance offset (-1..+1, 0 = none) added to the parameter(s) playing a role.
     * The stored parameters do not change (get_param, scenes, projects): the macros (ENERGY,
     * SPACE, TENSION) ride on top of the sound and go back to it at 0. */
    void  (*role_offset)(void *ctx, uint16_t role, float offset);

    /* optional: the tempo and where the music is, in beats (quarter notes from PLAY, or a free count when
     * stopped), given before every rendered piece: LFOs locked to the tempo, motion follows the bar */
    void  (*clock)(void *ctx, float bpm, float beats);

    /* optional: the track's motion lane (a value the Live Core plays in time, 0..1): a modulation source */
    void  (*motion)(void *ctx, float value01);
} ols_engine_api_t;

/* An engine instance: its function table plus its private state. */
typedef struct {
    const ols_engine_api_t *api;
    void *ctx;
} ols_engine_t;

#endif /* OLS_ENGINE_H */
