/*
 * OpenVALive - VA engine: factory presets.
 *
 * Every preset starts from INIT and changes a few parameters, so it is easy
 * to read what makes each sound.
 */
#include "engine_va_internal.h"

static const float PRESET_INIT[VA_P_COUNT] = {
    [VA_P_OSC1_WAVE] = 0.0f,   [VA_P_OSC2_WAVE] = 0.0f,
    [VA_P_OSC2_COARSE] = 0.5f, [VA_P_OSC2_FINE] = 0.56f,  [VA_P_PW] = 0.5f,
    [VA_P_MIX_OSC1] = 0.8f,    [VA_P_MIX_OSC2] = 0.6f,    [VA_P_MIX_SUB] = 0.0f,   [VA_P_MIX_NOISE] = 0.0f,
    [VA_P_CUTOFF] = 0.65f,     [VA_P_RESO] = 0.2f,        [VA_P_FILTER_MODE] = 0.0f,
    [VA_P_FENV_AMT] = 0.65f,   [VA_P_KEYTRACK] = 0.5f,    [VA_P_DRIVE] = 0.0f,
    [VA_P_F_ATTACK] = 0.0f,    [VA_P_F_DECAY] = 0.55f,    [VA_P_F_SUSTAIN] = 0.3f, [VA_P_F_RELEASE] = 0.5f,
    [VA_P_A_ATTACK] = 0.05f,   [VA_P_A_DECAY] = 0.5f,     [VA_P_A_SUSTAIN] = 0.8f, [VA_P_A_RELEASE] = 0.45f,
    [VA_P_LFO1_RATE] = 0.55f,  [VA_P_LFO1_WAVE] = 0.0f,   [VA_P_LFO1_PITCH] = 0.0f, [VA_P_LFO1_PW] = 0.0f,
    [VA_P_LFO2_RATE] = 0.35f,  [VA_P_LFO2_WAVE] = 0.0f,   [VA_P_LFO2_CUTOFF] = 0.0f,
    [VA_P_GLIDE] = 0.0f,       [VA_P_MONO] = 0.0f,        [VA_P_VEL_SENS] = 0.5f,
    [VA_P_DELAY_TIME] = 0.6f,  [VA_P_DELAY_FB] = 0.35f,   [VA_P_DELAY_TONE] = 0.4f, [VA_P_DELAY_MIX] = 0.0f,
    [VA_P_CHORUS_MODE] = 0.0f, [VA_P_CHORUS_MIX] = 0.5f,  [VA_P_VOLUME] = 0.7f,
    [VA_P_PHASER_RATE] = 0.35f, [VA_P_PHASER_DEPTH] = 0.6f, [VA_P_PHASER_MIX] = 0.0f,
    [VA_P_CRUSH] = 0.0f,       [VA_P_VEL_FILTER] = 0.0f,
    [VA_P_MOD1_AMT] = 0.5f,    [VA_P_MOD2_AMT] = 0.5f,    [VA_P_MOD3_AMT] = 0.5f,  [VA_P_MOD4_AMT] = 0.5f,
};

const char *const VA_PRESET_NAMES[VA_PRESET_COUNT] = {
    [VA_PRESET_ACID] = "ACID 303", [VA_PRESET_BASS] = "BASS", [VA_PRESET_SUB] = "SUB", [VA_PRESET_LEAD] = "LEAD",
    [VA_PRESET_PLUCK] = "PLUCK", [VA_PRESET_BRASS] = "BRASS", [VA_PRESET_ORGAN] = "ORGAN", [VA_PRESET_PAD] = "PAD",
    [VA_PRESET_STRINGS] = "STRINGS", [VA_PRESET_SWEEP] = "SWEEP", [VA_PRESET_GRUNGE] = "GRUNGE",
    [VA_PRESET_SCREECH] = "SCREECH", [VA_PRESET_DRONE] = "DRONE", [VA_PRESET_WIND] = "WIND", [VA_PRESET_HOLD] = "HOLD",
    [VA_PRESET_WHALE] = "WHALE", [VA_PRESET_INIT] = "INIT", [VA_PRESET_ACID_SQUARE] = "ACID SQUARE",
    [VA_PRESET_ACID_SCREAM] = "ACID SCREAM", [VA_PRESET_ACID_DEEP] = "ACID DEEP", [VA_PRESET_REESE] = "REESE",
    [VA_PRESET_WOBBLE] = "WOBBLE", [VA_PRESET_DEEP_SUB] = "DEEP SUB", [VA_PRESET_DUB_SIREN] = "DUB SIREN",
    [VA_PRESET_STAB] = "STAB", [VA_PRESET_POLY] = "POLY",
};

/* Choices are spread over 0..1: these values land in the middle of each one. */
#define WAVE_SAW 0.0f
#define WAVE_SQUARE 0.5f
#define WAVE_TRIANGLE 0.85f
#define FILTER_BP 0.3f
#define FILTER_HP 0.55f
#define LFO_TRIANGLE 0.3f
#define LFO_SH 0.85f
#define CHORUS_I 0.3f
#define CHORUS_II 0.55f
#define CHORUS_BOTH 0.85f
#define SEMITONES(n) (0.5f + (float)(n) / 48.0f)   /* OSC2_COARSE: -24 .. +24 */

/* A preset is INIT and then the changes in its table, applied in order (data, not code: the tables take a
 * quarter of the flash the same changes took as calls). */
typedef struct {
    uint8_t id;
    float value;
} va_change_t;

/* Mono saw + sub, snappy resonant filter, drive, glide. */
static const va_change_t BASS[] = {
    {VA_P_MONO, 1.0f},
    {VA_P_MIX_OSC2, 0.0f},
    {VA_P_MIX_SUB, 0.75f},
    {VA_P_CUTOFF, 0.35f},
    {VA_P_RESO, 0.55f},
    {VA_P_FENV_AMT, 0.78f},
    {VA_P_F_DECAY, 0.42f},
    {VA_P_F_SUSTAIN, 0.0f},
    {VA_P_A_ATTACK, 0.0f},
    {VA_P_A_RELEASE, 0.3f},
    {VA_P_DRIVE, 0.45f},
    {VA_P_GLIDE, 0.35f},
    {VA_P_VEL_SENS, 0.8f},
};

/* Two detuned saws, slow attacks, moving filter, chorus I and delay. */
static const va_change_t PAD[] = {
    {VA_P_OSC2_FINE, 0.62f},
    {VA_P_MIX_OSC2, 0.8f},
    {VA_P_CUTOFF, 0.55f},
    {VA_P_FENV_AMT, 0.58f},
    {VA_P_F_ATTACK, 0.6f},
    {VA_P_A_ATTACK, 0.6f},
    {VA_P_A_RELEASE, 0.7f},
    {VA_P_LFO2_CUTOFF, 0.3f},
    {VA_P_CHORUS_MODE, 0.3f}, /* chorus I */
    {VA_P_CHORUS_MIX, 0.7f},
    {VA_P_DELAY_MIX, 0.25f},
    {VA_P_VOLUME, 0.6f},
};

/* Mono legato: PWM square + saw one octave up, vibrato, delay. */
static const va_change_t LEAD[] = {
    {VA_P_MONO, 1.0f},
    {VA_P_OSC1_WAVE, 0.5f}, /* square */
    {VA_P_OSC2_WAVE, 0.0f}, /* saw */
    {VA_P_OSC2_COARSE, 0.75f}, /* +12 semitones */
    {VA_P_LFO1_PITCH, 0.12f},
    {VA_P_LFO1_PW, 0.6f},
    {VA_P_CUTOFF, 0.7f},
    {VA_P_GLIDE, 0.25f},
    {VA_P_DELAY_MIX, 0.3f},
};

/* TB-303-style: one saw, high resonance, a short envelope, accent from velocity. */
static const va_change_t ACID[] = {
    {VA_P_MONO, 1.0f},
    {VA_P_MIX_OSC2, 0.0f},
    {VA_P_CUTOFF, 0.3f},
    {VA_P_RESO, 0.85f},
    {VA_P_FENV_AMT, 0.82f},
    {VA_P_F_DECAY, 0.32f},
    {VA_P_F_SUSTAIN, 0.0f},
    {VA_P_A_ATTACK, 0.0f},
    {VA_P_A_DECAY, 0.4f},
    {VA_P_A_SUSTAIN, 0.6f},
    {VA_P_A_RELEASE, 0.2f},
    {VA_P_DRIVE, 0.5f},
    {VA_P_GLIDE, 0.2f},
    {VA_P_VEL_SENS, 0.9f},
    {VA_P_VEL_FILTER, 0.6f}, /* accents: hard notes squelch */
    {VA_P_CUTOFF, 0.24f},
    {VA_P_DELAY_MIX, 0.15f},
    {VA_P_VOLUME, 0.6f},
};

/* The 303's other wave: a square, hollower, the same squelch. */
static const va_change_t ACID_SQUARE[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_CUTOFF, 0.26f},
    {VA_P_RESO, 0.82f}, {VA_P_FENV_AMT, 0.8f}, {VA_P_F_DECAY, 0.3f}, {VA_P_F_SUSTAIN, 0.0f}, {VA_P_A_ATTACK, 0.0f},
    {VA_P_A_DECAY, 0.4f}, {VA_P_A_SUSTAIN, 0.6f}, {VA_P_A_RELEASE, 0.2f}, {VA_P_DRIVE, 0.45f}, {VA_P_GLIDE, 0.2f},
    {VA_P_VEL_SENS, 0.9f}, {VA_P_VEL_FILTER, 0.6f}, {VA_P_DELAY_MIX, 0.15f}, {VA_P_VOLUME, 0.6f},
};

/* Acid that screams: resonance at the edge, a short hard envelope, pushed drive (for the build-ups). */
static const va_change_t ACID_SCREAM[] = {
    {VA_P_MONO, 1.0f}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_CUTOFF, 0.2f}, {VA_P_RESO, 0.95f}, {VA_P_FENV_AMT, 0.95f},
    {VA_P_F_DECAY, 0.22f}, {VA_P_F_SUSTAIN, 0.0f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.35f},
    {VA_P_A_SUSTAIN, 0.55f}, {VA_P_A_RELEASE, 0.15f}, {VA_P_DRIVE, 0.85f}, {VA_P_GLIDE, 0.15f}, {VA_P_VEL_SENS, 0.9f},
    {VA_P_VEL_FILTER, 0.8f}, {VA_P_DELAY_MIX, 0.1f}, {VA_P_VOLUME, 0.5f},
};

/* Round, hypnotic acid: the filter low, a long decay, a longer glide (minimal, dub techno). */
static const va_change_t ACID_DEEP[] = {
    {VA_P_MONO, 1.0f}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_SUB, 0.3f}, {VA_P_CUTOFF, 0.18f}, {VA_P_RESO, 0.72f},
    {VA_P_FENV_AMT, 0.68f}, {VA_P_F_DECAY, 0.55f}, {VA_P_F_SUSTAIN, 0.1f}, {VA_P_A_ATTACK, 0.0f},
    {VA_P_A_DECAY, 0.55f}, {VA_P_A_SUSTAIN, 0.7f}, {VA_P_A_RELEASE, 0.3f}, {VA_P_DRIVE, 0.3f}, {VA_P_GLIDE, 0.35f},
    {VA_P_VEL_SENS, 0.7f}, {VA_P_VEL_FILTER, 0.4f}, {VA_P_DELAY_MIX, 0.25f}, {VA_P_DELAY_FB, 0.5f},
    {VA_P_VOLUME, 0.65f},
};

/* Drum & bass Reese: two saws detuned against each other, low-passed, mono. */
static const va_change_t REESE[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC2_FINE, 0.68f}, {VA_P_MIX_OSC1, 0.8f}, {VA_P_MIX_OSC2, 0.8f}, {VA_P_MIX_SUB, 0.35f},
    {VA_P_CUTOFF, 0.38f}, {VA_P_RESO, 0.25f}, {VA_P_FENV_AMT, 0.55f}, {VA_P_F_DECAY, 0.6f}, {VA_P_F_SUSTAIN, 0.6f},
    {VA_P_A_ATTACK, 0.02f}, {VA_P_A_SUSTAIN, 0.9f}, {VA_P_A_RELEASE, 0.3f}, {VA_P_DRIVE, 0.45f},
    {VA_P_GLIDE, 0.12f}, {VA_P_VOLUME, 0.6f},
};

/* Wobble: an LFO on the tempo (1/8, from each note) opens and shuts the filter. */
static const va_change_t WOBBLE[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC2_FINE, 0.6f}, {VA_P_MIX_OSC2, 0.7f}, {VA_P_MIX_SUB, 0.5f}, {VA_P_CUTOFF, 0.2f},
    {VA_P_RESO, 0.55f}, {VA_P_FENV_AMT, 0.5f}, {VA_P_LFO2_CUTOFF, 0.75f}, {VA_P_LFO2_WAVE, LFO_TRIANGLE},
    {VA_P_LFO2_SYNC, (9.0f + 0.5f) / (float)VA_LFO_SYNCS}, {VA_P_LFO2_TRIG, (1.0f + 0.5f) / (float)VA_TRIG_COUNT},
    {VA_P_A_ATTACK, 0.01f}, {VA_P_A_SUSTAIN, 0.9f}, {VA_P_A_RELEASE, 0.25f}, {VA_P_DRIVE, 0.6f},
    {VA_P_VOLUME, 0.6f},
};

/* Dub's deep sub: a sine-like triangle, round, the sub under it, little filter movement. */
static const va_change_t DEEP_SUB[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_TRIANGLE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_SUB, 1.0f},
    {VA_P_CUTOFF, 0.2f}, {VA_P_RESO, 0.05f}, {VA_P_FENV_AMT, 0.52f}, {VA_P_A_ATTACK, 0.02f}, {VA_P_A_DECAY, 0.6f},
    {VA_P_A_SUSTAIN, 0.85f}, {VA_P_A_RELEASE, 0.35f}, {VA_P_GLIDE, 0.1f}, {VA_P_DRIVE, 0.15f}, {VA_P_VOLUME, 0.75f},
};

/* Dub siren: a square swept up and down by a slow LFO on the pitch, into a long delay. */
static const va_change_t DUB_SIREN[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_CUTOFF, 0.7f},
    {VA_P_RESO, 0.3f}, {VA_P_FENV_AMT, 0.5f}, {VA_P_LFO1_WAVE, LFO_TRIANGLE}, {VA_P_LFO1_RATE, 0.42f},
    {VA_P_LFO1_PITCH, 0.6f}, {VA_P_A_ATTACK, 0.02f}, {VA_P_A_SUSTAIN, 0.9f}, {VA_P_A_RELEASE, 0.5f},
    {VA_P_DELAY_MIX, 0.45f}, {VA_P_DELAY_FB, 0.65f}, {VA_P_DELAY_TIME, 0.75f}, {VA_P_VOLUME, 0.5f},
};

/* House / techno chord stab: two saws, a short snappy envelope on the filter and the amp, chorus, an echo. */
static const va_change_t STAB[] = {
    {VA_P_OSC2_FINE, 0.6f}, {VA_P_MIX_OSC2, 0.75f}, {VA_P_CUTOFF, 0.35f}, {VA_P_RESO, 0.35f},
    {VA_P_FENV_AMT, 0.85f}, {VA_P_F_DECAY, 0.28f}, {VA_P_F_SUSTAIN, 0.05f}, {VA_P_A_ATTACK, 0.0f},
    {VA_P_A_DECAY, 0.32f}, {VA_P_A_SUSTAIN, 0.0f}, {VA_P_A_RELEASE, 0.25f}, {VA_P_VEL_FILTER, 0.5f},
    {VA_P_CHORUS_MODE, (1.0f + 0.5f) / 4.0f}, {VA_P_CHORUS_MIX, 0.5f}, {VA_P_DELAY_MIX, 0.25f},
    {VA_P_DELAY_FB, 0.35f}, {VA_P_VOLUME, 0.55f},
};

/* Warm poly chords, Juno style: two saws gently detuned, the filter half open, a soft attack, both choruses. */
static const va_change_t POLY[] = {
    {VA_P_OSC2_FINE, 0.58f}, {VA_P_MIX_OSC2, 0.7f}, {VA_P_MIX_SUB, 0.15f}, {VA_P_CUTOFF, 0.5f},
    {VA_P_RESO, 0.2f}, {VA_P_FENV_AMT, 0.6f}, {VA_P_F_DECAY, 0.55f}, {VA_P_F_SUSTAIN, 0.45f},
    {VA_P_A_ATTACK, 0.12f}, {VA_P_A_DECAY, 0.6f}, {VA_P_A_SUSTAIN, 0.8f}, {VA_P_A_RELEASE, 0.5f},
    {VA_P_CHORUS_MODE, (3.0f + 0.5f) / 4.0f}, {VA_P_CHORUS_MIX, 0.6f}, {VA_P_VOLUME, 0.55f},
};

/* String ensemble: detuned saws, slow attack, both choruses. */
static const va_change_t STRINGS[] = {
    {VA_P_OSC2_FINE, 0.64f},
    {VA_P_MIX_OSC2, 0.8f},
    {VA_P_CUTOFF, 0.6f},
    {VA_P_FENV_AMT, 0.55f},
    {VA_P_A_ATTACK, 0.55f},
    {VA_P_A_RELEASE, 0.65f},
    {VA_P_LFO1_RATE, 0.5f},
    {VA_P_LFO1_PITCH, 0.05f},
    {VA_P_CHORUS_MODE, CHORUS_BOTH},
    {VA_P_CHORUS_MIX, 0.8f},
    {VA_P_VOLUME, 0.6f},
};

/* Brass: the filter opens slowly after the attack, like a breath. */
static const va_change_t BRASS[] = {
    {VA_P_OSC2_FINE, 0.58f},
    {VA_P_MIX_OSC2, 0.8f},
    {VA_P_CUTOFF, 0.4f},
    {VA_P_RESO, 0.15f},
    {VA_P_FENV_AMT, 0.75f},
    {VA_P_F_ATTACK, 0.35f},
    {VA_P_F_SUSTAIN, 0.5f},
    {VA_P_A_ATTACK, 0.25f},
    {VA_P_A_RELEASE, 0.4f},
    {VA_P_DRIVE, 0.2f},
    {VA_P_LFO1_PITCH, 0.04f},
};

/* Pluck: a bright attack that dies away, echoes behind it. */
static const va_change_t PLUCK[] = {
    {VA_P_OSC2_WAVE, WAVE_SQUARE},
    {VA_P_OSC2_COARSE, SEMITONES(12)},
    {VA_P_MIX_OSC2, 0.35f},
    {VA_P_CUTOFF, 0.35f},
    {VA_P_FENV_AMT, 0.85f},
    {VA_P_F_DECAY, 0.35f},
    {VA_P_F_SUSTAIN, 0.0f},
    {VA_P_A_ATTACK, 0.0f},
    {VA_P_A_DECAY, 0.45f},
    {VA_P_A_SUSTAIN, 0.0f},
    {VA_P_A_RELEASE, 0.4f},
    {VA_P_DELAY_MIX, 0.3f},
    {VA_P_DELAY_FB, 0.45f},
};

/* Organ: triangle + square an octave up + sub, no envelope, chorus II as a rotary. */
static const va_change_t ORGAN[] = {
    {VA_P_OSC1_WAVE, WAVE_TRIANGLE},
    {VA_P_OSC2_WAVE, WAVE_SQUARE},
    {VA_P_OSC2_COARSE, SEMITONES(12)},
    {VA_P_OSC2_FINE, 0.5f},
    {VA_P_MIX_OSC2, 0.45f},
    {VA_P_MIX_SUB, 0.4f},
    {VA_P_CUTOFF, 0.75f},
    {VA_P_FENV_AMT, 0.5f},
    {VA_P_A_ATTACK, 0.02f},
    {VA_P_A_SUSTAIN, 1.0f},
    {VA_P_A_RELEASE, 0.15f},
    {VA_P_CHORUS_MODE, CHORUS_II},
    {VA_P_CHORUS_MIX, 0.6f},
};

/* Sub bass: a round triangle and its sub-octave, a touch of drive. */
static const va_change_t SUB[] = {
    {VA_P_MONO, 1.0f},
    {VA_P_OSC1_WAVE, WAVE_TRIANGLE},
    {VA_P_MIX_OSC2, 0.0f},
    {VA_P_MIX_SUB, 0.8f},
    {VA_P_CUTOFF, 0.25f},
    {VA_P_FENV_AMT, 0.55f},
    {VA_P_A_ATTACK, 0.01f},
    {VA_P_A_RELEASE, 0.25f},
    {VA_P_GLIDE, 0.15f},
    {VA_P_DRIVE, 0.25f},
};

/* Sweep: a slow pad through the phaser. */
static const va_change_t SWEEP[] = {
    /* PAD first */
    {VA_P_OSC2_FINE, 0.62f},
    {VA_P_MIX_OSC2, 0.8f},
    {VA_P_CUTOFF, 0.55f},
    {VA_P_FENV_AMT, 0.58f},
    {VA_P_F_ATTACK, 0.6f},
    {VA_P_A_ATTACK, 0.6f},
    {VA_P_A_RELEASE, 0.7f},
    {VA_P_LFO2_CUTOFF, 0.3f},
    {VA_P_CHORUS_MODE, 0.3f}, /* chorus I */
    {VA_P_CHORUS_MIX, 0.7f},
    {VA_P_DELAY_MIX, 0.25f},
    {VA_P_VOLUME, 0.6f},
    {VA_P_CHORUS_MODE, 0.0f},
    {VA_P_PHASER_RATE, 0.25f},
    {VA_P_PHASER_DEPTH, 0.75f},
    {VA_P_PHASER_MIX, 0.9f},
};

/* Wind: filtered noise; the keys move the band, a slow LFO breathes. */
static const va_change_t WIND[] = {
    {VA_P_MIX_OSC1, 0.0f},
    {VA_P_MIX_OSC2, 0.0f},
    {VA_P_MIX_NOISE, 0.9f},
    {VA_P_FILTER_MODE, FILTER_BP},
    {VA_P_CUTOFF, 0.45f},
    {VA_P_RESO, 0.7f},
    {VA_P_KEYTRACK, 1.0f},
    {VA_P_FENV_AMT, 0.5f},
    {VA_P_LFO2_RATE, 0.15f},
    {VA_P_LFO2_CUTOFF, 0.45f},
    {VA_P_A_ATTACK, 0.7f},
    {VA_P_A_RELEASE, 0.8f},
    {VA_P_DELAY_MIX, 0.3f},
    {VA_P_CHORUS_MODE, CHORUS_BOTH},
    {VA_P_VOLUME, 0.9f},
};


static const va_change_t HOLD[] = {
    {VA_P_MIX_OSC1, 0.35f},
    {VA_P_MIX_OSC2, 0.0f},
    {VA_P_MIX_SUB, 0.4f},
    {VA_P_MIX_NOISE, 0.7f},
    {VA_P_FILTER_MODE, FILTER_BP},
    {VA_P_CUTOFF, 0.3f},
    {VA_P_RESO, 0.88f},
    {VA_P_KEYTRACK, 0.6f},
    {VA_P_FENV_AMT, 0.5f},
    {VA_P_LFO2_RATE, 0.3f},
    {VA_P_LFO2_WAVE, LFO_SH},
    {VA_P_LFO2_CUTOFF, 0.35f},
    {VA_P_LFO1_RATE, 0.2f},
    {VA_P_LFO1_PITCH, 0.25f},
    {VA_P_DRIVE, 0.45f},
    {VA_P_A_ATTACK, 0.6f},
    {VA_P_A_RELEASE, 0.75f},
    {VA_P_DELAY_TIME, 0.85f},
    {VA_P_DELAY_FB, 0.6f},
    {VA_P_DELAY_TONE, 0.2f},
    {VA_P_DELAY_MIX, 0.35f},
    {VA_P_VOLUME, 0.8f},
};


static const va_change_t WHALE[] = {
    {VA_P_MONO, 1.0f},
    {VA_P_OSC1_WAVE, WAVE_TRIANGLE},
    {VA_P_MIX_OSC2, 0.0f},
    {VA_P_MIX_SUB, 0.2f},
    {VA_P_CUTOFF, 0.42f},
    {VA_P_RESO, 0.35f},
    {VA_P_FENV_AMT, 0.56f},
    {VA_P_F_ATTACK, 0.65f},
    {VA_P_GLIDE, 0.85f},
    {VA_P_LFO1_RATE, 0.25f},
    {VA_P_LFO1_PITCH, 0.18f},
    {VA_P_A_ATTACK, 0.6f},
    {VA_P_A_RELEASE, 0.8f},
    {VA_P_DELAY_TIME, 0.9f},
    {VA_P_DELAY_FB, 0.55f},
    {VA_P_DELAY_TONE, 0.15f},
    {VA_P_DELAY_MIX, 0.4f},
    {VA_P_CHORUS_MODE, CHORUS_BOTH},
    {VA_P_VOLUME, 0.9f},
};


static const va_change_t SCREECH[] = {
    {VA_P_MIX_OSC2, 0.0f},
    {VA_P_MIX_NOISE, 0.5f},
    {VA_P_FILTER_MODE, FILTER_HP},
    {VA_P_CUTOFF, 0.6f},
    {VA_P_RESO, 0.95f},
    {VA_P_FENV_AMT, 0.8f},
    {VA_P_F_DECAY, 0.6f},
    {VA_P_LFO2_RATE, 0.75f},
    {VA_P_LFO2_WAVE, 0.6f}, /* square */
    {VA_P_LFO2_CUTOFF, 0.3f},
    {VA_P_DRIVE, 0.7f},
    {VA_P_CRUSH, 0.35f},
    {VA_P_VOLUME, 0.5f},
};

/* Grunge: two detuned saws and the sub, driven hard, through the phaser and the crusher. */
static const va_change_t GRUNGE[] = {
    {VA_P_OSC2_FINE, 0.6f},
    {VA_P_MIX_OSC2, 0.9f},
    {VA_P_MIX_SUB, 0.6f},
    {VA_P_CUTOFF, 0.5f},
    {VA_P_RESO, 0.4f},
    {VA_P_FENV_AMT, 0.62f},
    {VA_P_DRIVE, 1.0f},
    {VA_P_CRUSH, 0.5f},
    {VA_P_PHASER_RATE, 0.4f},
    {VA_P_PHASER_MIX, 0.6f},
    {VA_P_A_ATTACK, 0.0f},
    {VA_P_VOLUME, 0.55f},
};


static const va_change_t DRONE[] = {
    {VA_P_OSC2_WAVE, WAVE_SQUARE},
    {VA_P_OSC2_COARSE, SEMITONES(-12)},
    {VA_P_MIX_OSC2, 0.7f},
    {VA_P_CUTOFF, 0.35f},
    {VA_P_RESO, 0.5f},
    {VA_P_LFO2_RATE, 0.05f},
    {VA_P_LFO2_WAVE, LFO_TRIANGLE},
    {VA_P_LFO2_CUTOFF, 0.55f},
    {VA_P_A_ATTACK, 0.75f},
    {VA_P_A_RELEASE, 0.85f},
    {VA_P_PHASER_RATE, 0.1f},
    {VA_P_PHASER_MIX, 0.7f},
    {VA_P_DELAY_MIX, 0.3f},
    {VA_P_DELAY_FB, 0.6f},
    {VA_P_VOLUME, 0.6f},
};

#define CHANGES(t) { t, (uint8_t)(sizeof t / sizeof t[0]) }
static const struct {
    const va_change_t *c;
    uint8_t n;
} PRESET_CHANGES[VA_PRESET_COUNT] = {
    [VA_PRESET_BASS] = CHANGES(BASS),
    [VA_PRESET_PAD] = CHANGES(PAD),
    [VA_PRESET_LEAD] = CHANGES(LEAD),
    [VA_PRESET_ACID] = CHANGES(ACID),
    [VA_PRESET_STRINGS] = CHANGES(STRINGS),
    [VA_PRESET_BRASS] = CHANGES(BRASS),
    [VA_PRESET_PLUCK] = CHANGES(PLUCK),
    [VA_PRESET_ORGAN] = CHANGES(ORGAN),
    [VA_PRESET_SUB] = CHANGES(SUB),
    [VA_PRESET_SWEEP] = CHANGES(SWEEP),
    [VA_PRESET_WIND] = CHANGES(WIND),
    [VA_PRESET_HOLD] = CHANGES(HOLD),
    [VA_PRESET_WHALE] = CHANGES(WHALE),
    [VA_PRESET_SCREECH] = CHANGES(SCREECH),
    [VA_PRESET_GRUNGE] = CHANGES(GRUNGE),
    [VA_PRESET_DRONE] = CHANGES(DRONE),
    [VA_PRESET_ACID_SQUARE] = CHANGES(ACID_SQUARE),
    [VA_PRESET_ACID_SCREAM] = CHANGES(ACID_SCREAM),
    [VA_PRESET_ACID_DEEP] = CHANGES(ACID_DEEP),
    [VA_PRESET_REESE] = CHANGES(REESE),
    [VA_PRESET_WOBBLE] = CHANGES(WOBBLE),
    [VA_PRESET_DEEP_SUB] = CHANGES(DEEP_SUB),
    [VA_PRESET_DUB_SIREN] = CHANGES(DUB_SIREN),
    [VA_PRESET_STAB] = CHANGES(STAB),
    [VA_PRESET_POLY] = CHANGES(POLY),
};

void engine_va_load_preset(engine_va_t *e, va_preset_t preset)
{
    for (int i = 0; i < VA_P_COUNT; i++) {
        engine_va_set_param(e, (uint16_t)i, PRESET_INIT[i]);
    }

    if ((unsigned)preset < VA_PRESET_COUNT) {
        for (uint32_t i = 0; i < PRESET_CHANGES[preset].n; i++) {
            engine_va_set_param(e, PRESET_CHANGES[preset].c[i].id, PRESET_CHANGES[preset].c[i].value);
        }
    }
    /* a new sound starts exactly where it should, without gliding from the old one */
    engine_va_snap(e);
}
