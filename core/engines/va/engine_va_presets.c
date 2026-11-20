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
    [VA_PRESET_STAB] = "STAB", [VA_PRESET_POLY] = "POLY", [VA_PRESET_THUNDER] = "THUNDER", [VA_PRESET_RAIN] = "RAIN",
    [VA_PRESET_CREAK] = "CREAK", [VA_PRESET_STRINGS70] = "STRINGS 70", [VA_PRESET_VKEYS] = "VINTAGE KEYS",
    [VA_PRESET_TALK] = "TALK", [VA_PRESET_PSY_BASS] = "PSY BASS", [VA_PRESET_ZAP] = "ZAP", [VA_PRESET_LASER] = "LASER",
    [VA_PRESET_BLIP] = "BLIP", [VA_PRESET_METAL] = "METAL", [VA_PRESET_ARP] = "ARP", [VA_PRESET_CHOIR] = "CHOIR",
    [VA_PRESET_NOISE] = "NOISE", [VA_PRESET_RISER] = "RISER", [VA_PRESET_DOWNLIFTER] = "DOWNLIFTER",
    [VA_PRESET_SHAKER] = "SHAKER", [VA_PRESET_PLATE] = "PLATE", [VA_PRESET_DUB_CHORD] = "DUB CHORD",
    [VA_PRESET_DEEP_PAD] = "DEEP PAD", [VA_PRESET_DEEP_STAB] = "DEEP STAB", [VA_PRESET_BOOM] = "BOOM",
    [VA_PRESET_RUMBLE] = "RUMBLE", [VA_PRESET_HOOVER] = "HOOVER", [VA_PRESET_GOA_LEAD] = "GOA LEAD",
    [VA_PRESET_SQUELCH] = "SQUELCH", [VA_PRESET_RUBBER] = "RUBBER", [VA_PRESET_BLEEP] = "BLEEP",
    [VA_PRESET_CLICK] = "CLICK", [VA_PRESET_SAX] = "RAVE SAX", [VA_PRESET_LUTE] = "LUTE", [VA_PRESET_FLUTE] = "FLUTE",
    [VA_PRESET_BAGPIPE] = "BAGPIPE", [VA_PRESET_DROPS] = "DROPS",
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
    {VA_P_VOLUME, 0.66f},
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
    {VA_P_A_SUSTAIN, 0.85f}, {VA_P_A_RELEASE, 0.35f}, {VA_P_GLIDE, 0.1f}, {VA_P_DRIVE, 0.15f}, {VA_P_VOLUME, 0.9f},
};

/* Dub siren: a square swept up and down by a slow LFO on the pitch, into a long delay. */
static const va_change_t DUB_SIREN[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_CUTOFF, 0.7f},
    {VA_P_RESO, 0.3f}, {VA_P_FENV_AMT, 0.5f}, {VA_P_LFO1_WAVE, LFO_TRIANGLE}, {VA_P_LFO1_RATE, 0.42f},
    {VA_P_LFO1_PITCH, 0.6f}, {VA_P_A_ATTACK, 0.02f}, {VA_P_A_SUSTAIN, 0.9f}, {VA_P_A_RELEASE, 0.5f},
    {VA_P_DELAY_MIX, 0.45f}, {VA_P_DELAY_FB, 0.65f}, {VA_P_DELAY_TIME, 0.75f}, {VA_P_VOLUME, 0.5f},
};

/* House / techno chord stab: two saws and a little sub, a snappy filter envelope on the attack, then a body that
 * holds while the keys are (sustain), chorus, an echo. */
static const va_change_t STAB[] = {
    {VA_P_OSC2_FINE, 0.6f}, {VA_P_MIX_OSC2, 0.75f}, {VA_P_MIX_SUB, 0.2f}, {VA_P_CUTOFF, 0.4f}, {VA_P_RESO, 0.35f},
    {VA_P_FENV_AMT, 0.8f}, {VA_P_F_DECAY, 0.32f}, {VA_P_F_SUSTAIN, 0.2f}, {VA_P_A_ATTACK, 0.0f},
    {VA_P_A_DECAY, 0.45f}, {VA_P_A_SUSTAIN, 0.4f}, {VA_P_A_RELEASE, 0.35f}, {VA_P_VEL_FILTER, 0.5f},
    {VA_P_CHORUS_MODE, (1.0f + 0.5f) / 4.0f}, {VA_P_CHORUS_MIX, 0.5f}, {VA_P_DELAY_MIX, 0.3f},
    {VA_P_DELAY_FB, 0.4f}, {VA_P_VOLUME, 0.6f},
};

/* Warm poly chords, Juno style: two saws gently detuned, the filter half open, a soft attack, both choruses. */
static const va_change_t POLY[] = {
    {VA_P_OSC2_FINE, 0.58f}, {VA_P_MIX_OSC2, 0.7f}, {VA_P_MIX_SUB, 0.15f}, {VA_P_CUTOFF, 0.5f},
    {VA_P_RESO, 0.2f}, {VA_P_FENV_AMT, 0.6f}, {VA_P_F_DECAY, 0.55f}, {VA_P_F_SUSTAIN, 0.45f},
    {VA_P_A_ATTACK, 0.12f}, {VA_P_A_DECAY, 0.6f}, {VA_P_A_SUSTAIN, 0.8f}, {VA_P_A_RELEASE, 0.5f},
    {VA_P_CHORUS_MODE, (3.0f + 0.5f) / 4.0f}, {VA_P_CHORUS_MIX, 0.6f}, {VA_P_VOLUME, 0.605f},
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
    {VA_P_VOLUME, 0.66f},
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

/* Pluck: a bright attack that dies down to a soft tone held with the key, a ringing release, chorus and echoes
 * behind it. */
static const va_change_t PLUCK[] = {
    {VA_P_OSC2_WAVE, WAVE_SQUARE},
    {VA_P_OSC2_COARSE, SEMITONES(12)},
    {VA_P_MIX_OSC2, 0.35f},
    {VA_P_CUTOFF, 0.4f},
    {VA_P_RESO, 0.3f},
    {VA_P_FENV_AMT, 0.8f},
    {VA_P_F_DECAY, 0.45f},
    {VA_P_F_SUSTAIN, 0.15f},
    {VA_P_A_ATTACK, 0.0f},
    {VA_P_A_DECAY, 0.6f},
    {VA_P_A_SUSTAIN, 0.25f},
    {VA_P_A_RELEASE, 0.5f},
    {VA_P_CHORUS_MIX, 0.35f},
    {VA_P_DELAY_MIX, 0.35f},
    {VA_P_DELAY_FB, 0.5f},
    {VA_P_VOLUME, 0.715f},
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
    {VA_P_VOLUME, 0.66f},
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
    {VA_P_VOLUME, 0.855f},
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
    {VA_P_VOLUME, 0.76f},
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
    {VA_P_VOLUME, 0.99f},
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
    {VA_P_VOLUME, 0.427f},                       /* (20 % lower than 0.5: volume is 2 v^2) */
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
    {VA_P_VOLUME, 0.522f},
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
    {VA_P_VOLUME, 0.57f},
};

/* the modulation matrix's choices, in the middle of their range */
#define MOD_SRC(s) (((float)(s) + 0.5f) / (float)VA_SRC_COUNT)
#define MOD_DST(d) (((float)(d) + 0.5f) / (float)VA_DST_COUNT)

/* Thunder: noise through a closed low-pass that swells in, rumbles (slow random cutoff) and dies away; low notes
 * add the boom of the sub, a hard hit cracks (velocity opens the filter). Hold it for a long roll. */
static const va_change_t THUNDER[] = {
    {VA_P_MIX_OSC1, 0.0f}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_SUB, 0.35f}, {VA_P_MIX_NOISE, 1.0f},
    {VA_P_CUTOFF, 0.16f}, {VA_P_RESO, 0.25f}, {VA_P_KEYTRACK, 0.3f}, {VA_P_FENV_AMT, 0.75f},
    {VA_P_F_ATTACK, 0.05f}, {VA_P_F_DECAY, 0.6f}, {VA_P_F_SUSTAIN, 0.25f}, {VA_P_F_RELEASE, 0.75f},
    {VA_P_A_ATTACK, 0.25f}, {VA_P_A_DECAY, 0.75f}, {VA_P_A_SUSTAIN, 0.6f}, {VA_P_A_RELEASE, 0.85f},
    {VA_P_LFO2_RATE, 0.4f}, {VA_P_LFO2_WAVE, LFO_SH}, {VA_P_LFO2_CUTOFF, 0.3f},
    {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_LFO2)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_LEVEL)}, {VA_P_MOD1_AMT, 0.68f},
    {VA_P_DRIVE, 0.5f}, {VA_P_VEL_FILTER, 0.5f}, {VA_P_DELAY_TIME, 0.8f}, {VA_P_DELAY_FB, 0.45f},
    {VA_P_DELAY_TONE, 0.15f}, {VA_P_DELAY_MIX, 0.2f}, {VA_P_VOLUME, 0.75f},
};

/* Rain: bright noise in a band-pass, drops from a fast random level and cutoff; the note sets the drops' pitch,
 * the higher the lighter. Chorus spreads it wide. */
static const va_change_t RAIN[] = {
    {VA_P_MIX_OSC1, 0.0f}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_NOISE, 0.9f}, {VA_P_FILTER_MODE, FILTER_BP},
    {VA_P_CUTOFF, 0.62f}, {VA_P_RESO, 0.45f}, {VA_P_KEYTRACK, 0.8f}, {VA_P_FENV_AMT, 0.5f},
    {VA_P_A_ATTACK, 0.2f}, {VA_P_A_SUSTAIN, 1.0f}, {VA_P_A_RELEASE, 0.6f},
    {VA_P_LFO2_RATE, 0.88f}, {VA_P_LFO2_WAVE, LFO_SH}, {VA_P_LFO2_CUTOFF, 0.35f},
    {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_LFO2)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_LEVEL)}, {VA_P_MOD1_AMT, 0.8f},
    {VA_P_MOD2_SRC, MOD_SRC(VA_SRC_LFO2)}, {VA_P_MOD2_DST, MOD_DST(VA_DST_RESO)}, {VA_P_MOD2_AMT, 0.7f},
    {VA_P_CHORUS_MODE, CHORUS_BOTH}, {VA_P_CHORUS_MIX, 0.7f}, {VA_P_VOLUME, 0.65f},
};

/* Creak: a door or a hull, a low saw through a sharp band-pass, its level cut into grains by a fast random
 * (the "crrrk"), the pitch slowly bending; slow notes groan, hard ones crack. */
static const va_change_t CREAK[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_SAW}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_NOISE, 0.25f},
    {VA_P_FILTER_MODE, FILTER_BP}, {VA_P_CUTOFF, 0.5f}, {VA_P_RESO, 0.88f}, {VA_P_KEYTRACK, 0.7f},
    {VA_P_FENV_AMT, 0.5f}, {VA_P_A_ATTACK, 0.15f}, {VA_P_A_SUSTAIN, 0.9f}, {VA_P_A_RELEASE, 0.35f},
    {VA_P_LFO1_RATE, 0.25f}, {VA_P_LFO1_WAVE, LFO_TRIANGLE}, {VA_P_LFO1_PITCH, 0.3f},
    {VA_P_LFO2_RATE, 0.78f}, {VA_P_LFO2_WAVE, LFO_SH}, {VA_P_LFO2_CUTOFF, 0.2f},
    {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_LFO2)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_LEVEL)}, {VA_P_MOD1_AMT, 0.92f},
    {VA_P_MOD2_SRC, MOD_SRC(VA_SRC_VELOCITY)}, {VA_P_MOD2_DST, MOD_DST(VA_DST_LFO2_RATE)}, {VA_P_MOD2_AMT, 0.7f},
    {VA_P_DRIVE, 0.35f}, {VA_P_GLIDE, 0.5f}, {VA_P_DELAY_MIX, 0.15f}, {VA_P_VOLUME, 0.617f},
};

/* A 70s string machine: two saws close together, no resonance, a gentle attack and both choruses deep (the
 * ensemble), a slow phaser over it. */
static const va_change_t STRINGS70[] = {
    {VA_P_OSC2_FINE, 0.56f}, {VA_P_MIX_OSC2, 0.75f}, {VA_P_CUTOFF, 0.6f}, {VA_P_RESO, 0.0f}, {VA_P_FENV_AMT, 0.5f},
    {VA_P_A_ATTACK, 0.45f}, {VA_P_A_SUSTAIN, 1.0f}, {VA_P_A_RELEASE, 0.6f},
    {VA_P_CHORUS_MODE, CHORUS_BOTH}, {VA_P_CHORUS_MIX, 0.9f},
    {VA_P_PHASER_RATE, 0.12f}, {VA_P_PHASER_DEPTH, 0.5f}, {VA_P_PHASER_MIX, 0.45f}, {VA_P_VOLUME, 0.605f},
};

/* An electric piano of the 70s: triangle and a quiet one an octave up, a bell-like start that fades, the softer
 * the darker, a slow tremolo and chorus I. */
static const va_change_t VKEYS[] = {
    {VA_P_OSC1_WAVE, WAVE_TRIANGLE}, {VA_P_OSC2_WAVE, WAVE_TRIANGLE}, {VA_P_OSC2_COARSE, SEMITONES(12)},
    {VA_P_MIX_OSC2, 0.3f}, {VA_P_CUTOFF, 0.5f}, {VA_P_FENV_AMT, 0.68f}, {VA_P_F_DECAY, 0.45f},
    {VA_P_F_SUSTAIN, 0.2f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.65f}, {VA_P_A_SUSTAIN, 0.35f},
    {VA_P_A_RELEASE, 0.45f}, {VA_P_VEL_SENS, 0.85f}, {VA_P_VEL_FILTER, 0.4f},
    {VA_P_LFO1_RATE, 0.45f}, {VA_P_LFO1_WAVE, LFO_TRIANGLE},
    {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_LFO1)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_LEVEL)}, {VA_P_MOD1_AMT, 0.58f},
    {VA_P_CHORUS_MODE, CHORUS_I}, {VA_P_CHORUS_MIX, 0.5f}, {VA_P_VOLUME, 0.715f},
};

/* LFO timing choices: a tempo length (VA_LFO_SYNC_NAMES: 9 = 1/8, 11 = 1/16), restarted by each first key */
#define LFO_SYNC(i) (((float)(i) + 0.5f) / (float)VA_LFO_SYNCS)
#define LFO_RETRIG ((1.0f + 0.5f) / (float)VA_TRIG_COUNT)

/* Talk: a saw through a narrow band-pass that sways on the tempo (1/8), a vowel-like "yoi"; acid's glide and drive. */
static const va_change_t TALK[] = {
    {VA_P_MONO, 1.0f}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_FILTER_MODE, FILTER_BP}, {VA_P_CUTOFF, 0.42f}, {VA_P_RESO, 0.78f},
    {VA_P_KEYTRACK, 0.5f}, {VA_P_FENV_AMT, 0.62f}, {VA_P_F_DECAY, 0.5f}, {VA_P_F_SUSTAIN, 0.3f},
    {VA_P_LFO2_WAVE, LFO_TRIANGLE}, {VA_P_LFO2_SYNC, LFO_SYNC(9)}, {VA_P_LFO2_TRIG, LFO_RETRIG}, {VA_P_LFO2_CUTOFF, 0.45f},
    {VA_P_A_ATTACK, 0.0f}, {VA_P_A_SUSTAIN, 0.8f}, {VA_P_A_RELEASE, 0.25f}, {VA_P_DRIVE, 0.45f}, {VA_P_GLIDE, 0.25f},
    {VA_P_VEL_FILTER, 0.5f}, {VA_P_DELAY_MIX, 0.15f}, {VA_P_VOLUME, 0.7f},
};

/* The psytrance bass: two saws a hair apart and a strong sub, a clicky filter snap, driven; it holds as long as the
 * key (short notes give the rolling K-B-B-B lines). */
static const va_change_t PSY_BASS[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC2_FINE, 0.53f}, {VA_P_MIX_OSC2, 0.45f}, {VA_P_MIX_SUB, 0.75f}, {VA_P_CUTOFF, 0.36f},
    {VA_P_RESO, 0.45f}, {VA_P_FENV_AMT, 0.85f}, {VA_P_F_ATTACK, 0.0f}, {VA_P_F_DECAY, 0.32f}, {VA_P_F_SUSTAIN, 0.15f},
    {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.4f}, {VA_P_A_SUSTAIN, 0.85f}, {VA_P_A_RELEASE, 0.22f},
    {VA_P_DRIVE, 0.6f}, {VA_P_GLIDE, 0.0f}, {VA_P_VEL_SENS, 0.3f}, {VA_P_VOLUME, 0.68f},
};

/* Zap: the pitch dives two octaves in a blink (the filter envelope on the pitch), a little echo. */
static const va_change_t ZAP[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_CUTOFF, 0.6f}, {VA_P_RESO, 0.5f},
    {VA_P_FENV_AMT, 0.7f}, {VA_P_F_ATTACK, 0.0f}, {VA_P_F_DECAY, 0.45f}, {VA_P_F_SUSTAIN, 0.0f},
    {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_FENV)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_PITCH)}, {VA_P_MOD1_AMT, 0.95f},
    {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.5f}, {VA_P_A_SUSTAIN, 0.0f}, {VA_P_A_RELEASE, 0.35f},
    {VA_P_DELAY_MIX, 0.25f}, {VA_P_VOLUME, 0.75f},
};

/* Laser: a longer dive, the filter singing on the way down, echoes that keep it flying. */
static const va_change_t LASER[] = {
    {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_OSC2_WAVE, WAVE_SAW}, {VA_P_OSC2_FINE, 0.56f}, {VA_P_MIX_OSC2, 0.5f},
    {VA_P_CUTOFF, 0.55f}, {VA_P_RESO, 0.72f}, {VA_P_FENV_AMT, 0.8f}, {VA_P_F_ATTACK, 0.0f}, {VA_P_F_DECAY, 0.6f},
    {VA_P_F_SUSTAIN, 0.0f}, {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_FENV)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_PITCH)},
    {VA_P_MOD1_AMT, 1.0f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.62f}, {VA_P_A_SUSTAIN, 0.0f},
    {VA_P_A_RELEASE, 0.45f}, {VA_P_DELAY_TIME, 0.55f}, {VA_P_DELAY_FB, 0.55f}, {VA_P_DELAY_MIX, 0.4f},
    {VA_P_VOLUME, 0.55f},
};

/* Blip: a tiny square, each note at a random pitch and brightness, for glitchy fast patterns behind the mix. */
static const va_change_t BLIP[] = {
    {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_CUTOFF, 0.68f}, {VA_P_RESO, 0.3f},
    {VA_P_FENV_AMT, 0.5f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.58f}, {VA_P_A_SUSTAIN, 0.0f},
    {VA_P_A_RELEASE, 0.4f}, {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_RANDOM)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_PITCH)},
    {VA_P_MOD1_AMT, 0.75f}, {VA_P_MOD2_SRC, MOD_SRC(VA_SRC_RANDOM)}, {VA_P_MOD2_DST, MOD_DST(VA_DST_CUTOFF)},
    {VA_P_MOD2_AMT, 0.7f}, {VA_P_DELAY_MIX, 0.25f}, {VA_P_VOLUME, 0.8f},
};

/* Metal: a square and a saw a ninth apart and slightly off, through a ringing band-pass: a struck metal or wood
 * (close to an FM percussion, without FM). Low notes are wood, high ones metal. It rings on while the key is held
 * and after, a little chorus and echo around it. */
static const va_change_t METAL[] = {
    {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_OSC2_WAVE, WAVE_SAW}, {VA_P_OSC2_COARSE, SEMITONES(14)},
    {VA_P_OSC2_FINE, 0.68f}, {VA_P_MIX_OSC2, 0.7f}, {VA_P_FILTER_MODE, FILTER_BP}, {VA_P_CUTOFF, 0.62f},
    {VA_P_RESO, 0.82f}, {VA_P_KEYTRACK, 0.8f}, {VA_P_FENV_AMT, 0.6f}, {VA_P_F_DECAY, 0.6f}, {VA_P_F_SUSTAIN, 0.1f},
    {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.7f}, {VA_P_A_SUSTAIN, 0.2f}, {VA_P_A_RELEASE, 0.65f},
    {VA_P_DRIVE, 0.3f}, {VA_P_CHORUS_MIX, 0.3f}, {VA_P_DELAY_MIX, 0.2f}, {VA_P_DELAY_FB, 0.4f}, {VA_P_VOLUME, 0.665f},
};

/* Arp: cut for fast hypnotic arpeggios, a saw and a square, a snappy filter, a short body and an echo. */
static const va_change_t ARP[] = {
    {VA_P_OSC2_WAVE, WAVE_SQUARE}, {VA_P_OSC2_FINE, 0.53f}, {VA_P_MIX_OSC2, 0.55f}, {VA_P_CUTOFF, 0.45f},
    {VA_P_RESO, 0.45f}, {VA_P_FENV_AMT, 0.72f}, {VA_P_F_DECAY, 0.42f}, {VA_P_F_SUSTAIN, 0.1f},
    {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.5f}, {VA_P_A_SUSTAIN, 0.15f}, {VA_P_A_RELEASE, 0.3f},
    {VA_P_VEL_FILTER, 0.35f}, {VA_P_DELAY_TIME, 0.5f}, {VA_P_DELAY_FB, 0.4f}, {VA_P_DELAY_MIX, 0.3f},
    {VA_P_VOLUME, 0.72f},
};

/* Choir: detuned saws through a band-pass around the voice, a slow breath in, vibrato and both choruses: the
 * synth choirs of the breaks. */
static const va_change_t CHOIR[] = {
    {VA_P_OSC2_FINE, 0.6f}, {VA_P_MIX_OSC2, 0.8f}, {VA_P_FILTER_MODE, FILTER_BP}, {VA_P_CUTOFF, 0.5f},
    {VA_P_RESO, 0.42f}, {VA_P_KEYTRACK, 0.6f}, {VA_P_FENV_AMT, 0.5f}, {VA_P_A_ATTACK, 0.55f}, {VA_P_A_SUSTAIN, 1.0f},
    {VA_P_A_RELEASE, 0.65f}, {VA_P_LFO1_RATE, 0.45f}, {VA_P_LFO1_PITCH, 0.05f}, {VA_P_CHORUS_MODE, CHORUS_BOTH},
    {VA_P_CHORUS_MIX, 0.85f}, {VA_P_DELAY_MIX, 0.2f}, {VA_P_VOLUME, 0.682f},
};

/* Noise: white noise in a low-pass, held: K1 (cutoff) opens it for a build-up of your own. */
static const va_change_t NOISE[] = {
    {VA_P_MIX_OSC1, 0.0f}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_NOISE, 1.0f}, {VA_P_CUTOFF, 0.5f}, {VA_P_RESO, 0.3f},
    {VA_P_KEYTRACK, 0.0f}, {VA_P_FENV_AMT, 0.5f}, {VA_P_A_ATTACK, 0.2f}, {VA_P_A_SUSTAIN, 1.0f}, {VA_P_A_RELEASE, 0.5f},
    {VA_P_VOLUME, 0.85f},
};

/* Riser: noise and a saw, heard at once, the filter and the pitch (an octave) rising for about 8 s (4 bars at 120
 * BPM) while the note is held, then staying up. */
static const va_change_t RISER[] = {
    {VA_P_MIX_OSC1, 0.4f}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_NOISE, 0.75f}, {VA_P_CUTOFF, 0.25f}, {VA_P_RESO, 0.45f},
    {VA_P_FENV_AMT, 0.95f}, {VA_P_F_ATTACK, 1.0f}, {VA_P_F_SUSTAIN, 1.0f}, {VA_P_F_RELEASE, 0.6f},
    {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_FENV)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_PITCH)}, {VA_P_MOD1_AMT, 0.75f},
    {VA_P_A_ATTACK, 0.3f}, {VA_P_A_SUSTAIN, 1.0f}, {VA_P_A_RELEASE, 0.6f}, {VA_P_PHASER_MIX, 0.3f},
    {VA_P_DELAY_FB, 0.5f}, {VA_P_DELAY_MIX, 0.25f}, {VA_P_VOLUME, 0.8f},
};

/* Downlifter: the other way: it starts open and an octave up, then falls and fades for about 8 s (held). */
static const va_change_t DOWNLIFTER[] = {
    {VA_P_MIX_OSC1, 0.3f}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_NOISE, 0.75f}, {VA_P_CUTOFF, 0.25f}, {VA_P_RESO, 0.4f},
    {VA_P_FENV_AMT, 0.95f}, {VA_P_F_ATTACK, 0.0f}, {VA_P_F_DECAY, 1.0f}, {VA_P_F_SUSTAIN, 0.0f},
    {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_FENV)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_PITCH)}, {VA_P_MOD1_AMT, 0.75f},
    {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 1.0f}, {VA_P_A_SUSTAIN, 0.45f}, {VA_P_A_RELEASE, 0.8f},
    {VA_P_DELAY_FB, 0.55f}, {VA_P_DELAY_MIX, 0.3f}, {VA_P_VOLUME, 0.7f},
};

/* Shaker: high-passed noise pulsing on the 1/16 (an LFO on the level, on the tempo): a hat loop to hold. */
static const va_change_t SHAKER[] = {
    {VA_P_MIX_OSC1, 0.0f}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_NOISE, 1.0f}, {VA_P_FILTER_MODE, FILTER_HP},
    {VA_P_CUTOFF, 0.75f}, {VA_P_RESO, 0.2f}, {VA_P_KEYTRACK, 0.3f}, {VA_P_FENV_AMT, 0.5f},
    {VA_P_LFO2_WAVE, LFO_TRIANGLE}, {VA_P_LFO2_SYNC, LFO_SYNC(11)}, {VA_P_LFO2_TRIG, LFO_RETRIG},
    {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_LFO2)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_LEVEL)}, {VA_P_MOD1_AMT, 1.0f},
    {VA_P_A_ATTACK, 0.1f}, {VA_P_A_SUSTAIN, 1.0f}, {VA_P_A_RELEASE, 0.3f}, {VA_P_VOLUME, 0.4f},
};

/* Plate: a struck metal sheet that rings: two squares an odd interval apart (a minor tenth and a bit), through a
 * singing band-pass, a long decay that holds while the key does and rings on after, chorus and a dark echo. */
static const va_change_t PLATE[] = {
    {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_OSC2_WAVE, WAVE_SQUARE}, {VA_P_OSC2_COARSE, SEMITONES(15)},
    {VA_P_OSC2_FINE, 0.73f}, {VA_P_MIX_OSC2, 0.8f}, {VA_P_PW, 0.3f}, {VA_P_FILTER_MODE, FILTER_BP},
    {VA_P_CUTOFF, 0.58f}, {VA_P_RESO, 0.88f}, {VA_P_KEYTRACK, 0.7f}, {VA_P_FENV_AMT, 0.62f}, {VA_P_F_DECAY, 0.75f},
    {VA_P_F_SUSTAIN, 0.2f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.85f}, {VA_P_A_SUSTAIN, 0.35f},
    {VA_P_A_RELEASE, 0.8f}, {VA_P_CHORUS_MODE, (3.0f + 0.5f) / 4.0f}, {VA_P_CHORUS_MIX, 0.45f},
    {VA_P_DELAY_TIME, 0.7f}, {VA_P_DELAY_FB, 0.5f}, {VA_P_DELAY_TONE, 0.25f}, {VA_P_DELAY_MIX, 0.3f},
    {VA_P_VOLUME, 0.6f},
};

/* Dub chord: the dub techno chord: two saws, a dark, muted filter, a short body, thrown into a long dark echo that
 * keeps turning (play chords, short). */
static const va_change_t DUB_CHORD[] = {
    {VA_P_OSC2_FINE, 0.58f}, {VA_P_MIX_OSC2, 0.75f}, {VA_P_CUTOFF, 0.32f}, {VA_P_RESO, 0.3f}, {VA_P_FENV_AMT, 0.62f},
    {VA_P_F_DECAY, 0.35f}, {VA_P_F_SUSTAIN, 0.1f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.42f},
    {VA_P_A_SUSTAIN, 0.2f}, {VA_P_A_RELEASE, 0.4f}, {VA_P_CHORUS_MODE, (1.0f + 0.5f) / 4.0f}, {VA_P_CHORUS_MIX, 0.4f},
    {VA_P_DELAY_TIME, 0.8f}, {VA_P_DELAY_FB, 0.75f}, {VA_P_DELAY_TONE, 0.2f}, {VA_P_DELAY_MIX, 0.5f},
    {VA_P_VOLUME, 0.96f},
};

/* Deep pad: dark and slow: two saws and the sub, a low filter breathing on a slow LFO, a long attack and release,
 * both choruses. */
static const va_change_t DEEP_PAD[] = {
    {VA_P_OSC2_FINE, 0.6f}, {VA_P_MIX_OSC2, 0.8f}, {VA_P_MIX_SUB, 0.35f}, {VA_P_CUTOFF, 0.27f}, {VA_P_RESO, 0.25f},
    {VA_P_FENV_AMT, 0.55f}, {VA_P_F_ATTACK, 0.6f}, {VA_P_F_SUSTAIN, 0.5f}, {VA_P_LFO2_RATE, 0.12f},
    {VA_P_LFO2_WAVE, LFO_TRIANGLE}, {VA_P_LFO2_CUTOFF, 0.6f}, {VA_P_A_ATTACK, 0.55f}, {VA_P_A_DECAY, 0.6f},
    {VA_P_A_SUSTAIN, 0.9f}, {VA_P_A_RELEASE, 0.75f}, {VA_P_CHORUS_MODE, (3.0f + 0.5f) / 4.0f},
    {VA_P_CHORUS_MIX, 0.6f}, {VA_P_DELAY_MIX, 0.15f}, {VA_P_DELAY_TONE, 0.25f}, {VA_P_VOLUME, 0.66f},
};

/* Deep stab: the deep house stab, rounder and warmer than STAB: a square and a saw, a soft filter snap, a body
 * that holds with the keys, chorus, an echo. */
static const va_change_t DEEP_STAB[] = {
    {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_OSC2_FINE, 0.57f}, {VA_P_MIX_OSC2, 0.65f}, {VA_P_MIX_SUB, 0.25f},
    {VA_P_CUTOFF, 0.32f}, {VA_P_RESO, 0.2f}, {VA_P_FENV_AMT, 0.68f}, {VA_P_F_DECAY, 0.4f}, {VA_P_F_SUSTAIN, 0.2f},
    {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.5f}, {VA_P_A_SUSTAIN, 0.4f}, {VA_P_A_RELEASE, 0.4f},
    {VA_P_CHORUS_MODE, (2.0f + 0.5f) / 4.0f}, {VA_P_CHORUS_MIX, 0.5f}, {VA_P_DELAY_MIX, 0.25f},
    {VA_P_DELAY_FB, 0.45f}, {VA_P_DELAY_TONE, 0.3f}, {VA_P_VOLUME, 0.65f},
};

/* Boom: a big 808-style bass: a round triangle and its sub, the pitch falling a little on the attack, a long hold,
 * a touch of drive. */
static const va_change_t BOOM[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_TRIANGLE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_SUB, 0.9f},
    {VA_P_CUTOFF, 0.3f}, {VA_P_RESO, 0.05f}, {VA_P_FENV_AMT, 0.5f}, {VA_P_F_ATTACK, 0.0f}, {VA_P_F_DECAY, 0.3f},
    {VA_P_F_SUSTAIN, 0.0f}, {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_FENV)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_PITCH)},
    {VA_P_MOD1_AMT, 0.62f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.85f}, {VA_P_A_SUSTAIN, 0.6f},
    {VA_P_A_RELEASE, 0.45f}, {VA_P_GLIDE, 0.25f}, {VA_P_DRIVE, 0.35f}, {VA_P_VOLUME, 0.85f},
};

/* Rumble: the techno rumble under the kick: a dark sub and a little noise, the filter low, a dark echo feeding back
 * on the off-beats. */
static const va_change_t RUMBLE[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_TRIANGLE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_SUB, 0.8f},
    {VA_P_MIX_NOISE, 0.15f}, {VA_P_CUTOFF, 0.18f}, {VA_P_RESO, 0.3f}, {VA_P_FENV_AMT, 0.6f}, {VA_P_F_DECAY, 0.35f},
    {VA_P_F_SUSTAIN, 0.0f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.55f}, {VA_P_A_SUSTAIN, 0.3f},
    {VA_P_A_RELEASE, 0.35f}, {VA_P_DRIVE, 0.55f}, {VA_P_DELAY_TIME, 0.45f}, {VA_P_DELAY_FB, 0.65f},
    {VA_P_DELAY_TONE, 0.1f}, {VA_P_DELAY_MIX, 0.45f}, {VA_P_VOLUME, 0.76f},
};

/* Hoover: the rave / hard techno hoover: two saws pulled far apart, both choruses, a slow glide, driven. */
static const va_change_t HOOVER[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC2_COARSE, SEMITONES(-12)}, {VA_P_OSC2_FINE, 0.72f}, {VA_P_MIX_OSC2, 0.85f},
    {VA_P_MIX_SUB, 0.3f}, {VA_P_CUTOFF, 0.55f}, {VA_P_RESO, 0.25f}, {VA_P_FENV_AMT, 0.6f}, {VA_P_F_ATTACK, 0.2f},
    {VA_P_F_DECAY, 0.5f}, {VA_P_F_SUSTAIN, 0.5f}, {VA_P_A_ATTACK, 0.02f}, {VA_P_A_SUSTAIN, 0.9f},
    {VA_P_A_RELEASE, 0.35f}, {VA_P_GLIDE, 0.4f}, {VA_P_CHORUS_MODE, (3.0f + 0.5f) / 4.0f}, {VA_P_CHORUS_MIX, 0.75f},
    {VA_P_DRIVE, 0.45f}, {VA_P_VOLUME, 0.55f},
};

/* Goa lead: the psytrance / goa lead: a saw and a square an octave up, a resonant filter, vibrato after the attack,
 * a glide, a long echo. */
static const va_change_t GOA_LEAD[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC2_WAVE, WAVE_SQUARE}, {VA_P_OSC2_COARSE, SEMITONES(12)}, {VA_P_MIX_OSC2, 0.5f},
    {VA_P_CUTOFF, 0.5f}, {VA_P_RESO, 0.6f}, {VA_P_FENV_AMT, 0.7f}, {VA_P_F_DECAY, 0.45f}, {VA_P_F_SUSTAIN, 0.35f},
    {VA_P_LFO1_RATE, 0.6f}, {VA_P_LFO1_PITCH, 0.1f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_SUSTAIN, 0.85f},
    {VA_P_A_RELEASE, 0.35f}, {VA_P_GLIDE, 0.2f}, {VA_P_DELAY_TIME, 0.65f}, {VA_P_DELAY_FB, 0.5f},
    {VA_P_DELAY_MIX, 0.3f}, {VA_P_DRIVE, 0.25f}, {VA_P_VOLUME, 0.57f},
};

/* Squelch: between acid and techno: a square, the filter very resonant and snapped shut on each note, the accent
 * on velocity, driven, glide on the legato notes. */
static const va_change_t SQUELCH[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_SUB, 0.3f},
    {VA_P_CUTOFF, 0.22f}, {VA_P_RESO, 0.9f}, {VA_P_FENV_AMT, 0.9f}, {VA_P_F_DECAY, 0.24f}, {VA_P_F_SUSTAIN, 0.0f},
    {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.45f}, {VA_P_A_SUSTAIN, 0.6f}, {VA_P_A_RELEASE, 0.2f},
    {VA_P_VEL_FILTER, 0.7f}, {VA_P_GLIDE, 0.25f}, {VA_P_DRIVE, 0.65f}, {VA_P_VOLUME, 0.55f},
};

/* Rubber: the minimal bass: a triangle and a square, a low resonant filter bumped open on each note, short, bouncy,
 * a little glide. */
static const va_change_t RUBBER[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_TRIANGLE}, {VA_P_OSC2_WAVE, WAVE_SQUARE}, {VA_P_MIX_OSC2, 0.35f},
    {VA_P_MIX_SUB, 0.5f}, {VA_P_CUTOFF, 0.22f}, {VA_P_RESO, 0.55f}, {VA_P_FENV_AMT, 0.72f}, {VA_P_F_DECAY, 0.22f},
    {VA_P_F_SUSTAIN, 0.0f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.4f}, {VA_P_A_SUSTAIN, 0.3f},
    {VA_P_A_RELEASE, 0.15f}, {VA_P_GLIDE, 0.12f}, {VA_P_DRIVE, 0.3f}, {VA_P_VOLUME, 0.88f},
};

/* Bleep: the minimal bleep: a round tone, short, high, into a ping-pong of echoes. */
static const va_change_t BLEEP[] = {
    {VA_P_OSC1_WAVE, WAVE_TRIANGLE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_CUTOFF, 0.6f}, {VA_P_RESO, 0.2f},
    {VA_P_FENV_AMT, 0.6f}, {VA_P_F_DECAY, 0.25f}, {VA_P_F_SUSTAIN, 0.2f}, {VA_P_A_ATTACK, 0.0f},
    {VA_P_A_DECAY, 0.35f}, {VA_P_A_SUSTAIN, 0.2f}, {VA_P_A_RELEASE, 0.3f}, {VA_P_DELAY_TIME, 0.5f},
    {VA_P_DELAY_FB, 0.55f}, {VA_P_DELAY_MIX, 0.4f}, {VA_P_VOLUME, 0.7f},
};

/* Click: minimal's tiny pitched percussion: a square and a breath of noise through a narrow band-pass that follows
 * the key, very short, a short echo. */
static const va_change_t CLICK[] = {
    {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_NOISE, 0.2f}, {VA_P_FILTER_MODE, FILTER_BP},
    {VA_P_CUTOFF, 0.58f}, {VA_P_RESO, 0.6f}, {VA_P_KEYTRACK, 1.0f}, {VA_P_FENV_AMT, 0.7f}, {VA_P_F_DECAY, 0.28f},
    {VA_P_F_SUSTAIN, 0.0f}, {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.42f}, {VA_P_A_SUSTAIN, 0.0f},
    {VA_P_A_RELEASE, 0.35f}, {VA_P_DELAY_TIME, 0.35f}, {VA_P_DELAY_FB, 0.35f}, {VA_P_DELAY_MIX, 0.25f},
    {VA_P_VOLUME, 1.0f},
};

/* Rave sax: the 90s rave / house sax, brassy: two saws close together, the filter blown open on the attack then
 * settling (the "blat" of a horn), the pitch scooping up into the note, a breath, drive, vibrato, an echo. */
static const va_change_t SAX[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC1_WAVE, WAVE_SAW}, {VA_P_OSC2_WAVE, WAVE_SAW}, {VA_P_OSC2_FINE, 0.54f},
    {VA_P_MIX_OSC2, 0.7f}, {VA_P_MIX_NOISE, 0.06f}, {VA_P_CUTOFF, 0.33f}, {VA_P_RESO, 0.2f}, {VA_P_FENV_AMT, 0.85f},
    {VA_P_F_ATTACK, 0.07f}, {VA_P_F_DECAY, 0.45f}, {VA_P_F_SUSTAIN, 0.55f}, {VA_P_KEYTRACK, 0.6f},
    {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_FENV)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_PITCH)}, {VA_P_MOD1_AMT, 0.47f},
    {VA_P_LFO1_RATE, 0.55f}, {VA_P_LFO1_PITCH, 0.06f}, {VA_P_A_ATTACK, 0.04f}, {VA_P_A_SUSTAIN, 0.85f},
    {VA_P_A_RELEASE, 0.3f}, {VA_P_GLIDE, 0.12f}, {VA_P_DRIVE, 0.5f}, {VA_P_CHORUS_MIX, 0.3f}, {VA_P_DELAY_MIX, 0.2f},
    {VA_P_VOLUME, 0.55f},
};

/* Lute: a dark electro lute: a pluck and its fifth (the old modes), a resonant snap, a slow phaser, a dark echo. */
static const va_change_t LUTE[] = {
    {VA_P_OSC1_WAVE, WAVE_SQUARE}, {VA_P_OSC2_COARSE, SEMITONES(7)}, {VA_P_MIX_OSC2, 0.45f}, {VA_P_CUTOFF, 0.38f},
    {VA_P_RESO, 0.4f}, {VA_P_FENV_AMT, 0.8f}, {VA_P_F_DECAY, 0.35f}, {VA_P_F_SUSTAIN, 0.05f}, {VA_P_A_ATTACK, 0.0f},
    {VA_P_A_DECAY, 0.5f}, {VA_P_A_SUSTAIN, 0.1f}, {VA_P_A_RELEASE, 0.45f}, {VA_P_PHASER_MIX, 0.2f},
    {VA_P_DELAY_TIME, 0.55f}, {VA_P_DELAY_FB, 0.45f}, {VA_P_DELAY_TONE, 0.3f}, {VA_P_DELAY_MIX, 0.3f},
    {VA_P_VOLUME, 0.715f},
};

/* Flute: a breathy recorder from a cave: a round triangle, breath noise, a soft attack, vibrato, a long dark echo. */
static const va_change_t FLUTE[] = {
    {VA_P_OSC1_WAVE, WAVE_TRIANGLE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_MIX_NOISE, 0.15f}, {VA_P_CUTOFF, 0.5f},
    {VA_P_RESO, 0.15f}, {VA_P_FENV_AMT, 0.55f}, {VA_P_LFO1_RATE, 0.5f}, {VA_P_LFO1_PITCH, 0.06f},
    {VA_P_A_ATTACK, 0.12f}, {VA_P_A_SUSTAIN, 0.9f}, {VA_P_A_RELEASE, 0.35f}, {VA_P_CHORUS_MIX, 0.35f},
    {VA_P_DELAY_TIME, 0.7f}, {VA_P_DELAY_FB, 0.5f}, {VA_P_DELAY_TONE, 0.25f}, {VA_P_DELAY_MIX, 0.35f},
    {VA_P_VOLUME, 0.715f},
};

/* Bagpipe: an electro bagpipe: two saws a fourth apart (the drone under the chanter), a nasal resonant filter,
 * driven, held, a slow phaser. */
static const va_change_t BAGPIPE[] = {
    {VA_P_MONO, 1.0f}, {VA_P_OSC2_COARSE, SEMITONES(-5)}, {VA_P_OSC2_FINE, 0.52f}, {VA_P_MIX_OSC2, 0.6f},
    {VA_P_CUTOFF, 0.55f}, {VA_P_RESO, 0.55f}, {VA_P_FENV_AMT, 0.55f}, {VA_P_A_ATTACK, 0.03f}, {VA_P_A_SUSTAIN, 1.0f},
    {VA_P_A_RELEASE, 0.25f}, {VA_P_GLIDE, 0.05f}, {VA_P_DRIVE, 0.4f}, {VA_P_PHASER_MIX, 0.25f}, {VA_P_VOLUME, 0.55f},
};

/* Drops: water drops: a round tone whose pitch springs up as it dies (the plop), each a little apart, an echo; played
 * fast or arpeggiated: a trickle. */
static const va_change_t DROPS[] = {
    {VA_P_OSC1_WAVE, WAVE_TRIANGLE}, {VA_P_MIX_OSC2, 0.0f}, {VA_P_CUTOFF, 0.7f}, {VA_P_RESO, 0.3f},
    {VA_P_FENV_AMT, 0.5f}, {VA_P_F_ATTACK, 0.0f}, {VA_P_F_DECAY, 0.25f}, {VA_P_F_SUSTAIN, 0.0f},
    {VA_P_MOD1_SRC, MOD_SRC(VA_SRC_FENV)}, {VA_P_MOD1_DST, MOD_DST(VA_DST_PITCH)}, {VA_P_MOD1_AMT, 0.38f},
    {VA_P_MOD2_SRC, MOD_SRC(VA_SRC_RANDOM)}, {VA_P_MOD2_DST, MOD_DST(VA_DST_PITCH)}, {VA_P_MOD2_AMT, 0.56f},
    {VA_P_A_ATTACK, 0.0f}, {VA_P_A_DECAY, 0.5f}, {VA_P_A_SUSTAIN, 0.0f}, {VA_P_A_RELEASE, 0.4f},
    {VA_P_DELAY_TIME, 0.45f}, {VA_P_DELAY_FB, 0.45f}, {VA_P_DELAY_MIX, 0.35f}, {VA_P_VOLUME, 1.0f},
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
    [VA_PRESET_THUNDER] = CHANGES(THUNDER),
    [VA_PRESET_RAIN] = CHANGES(RAIN),
    [VA_PRESET_CREAK] = CHANGES(CREAK),
    [VA_PRESET_STRINGS70] = CHANGES(STRINGS70),
    [VA_PRESET_VKEYS] = CHANGES(VKEYS),
    [VA_PRESET_TALK] = CHANGES(TALK),
    [VA_PRESET_PSY_BASS] = CHANGES(PSY_BASS),
    [VA_PRESET_ZAP] = CHANGES(ZAP),
    [VA_PRESET_LASER] = CHANGES(LASER),
    [VA_PRESET_BLIP] = CHANGES(BLIP),
    [VA_PRESET_METAL] = CHANGES(METAL),
    [VA_PRESET_ARP] = CHANGES(ARP),
    [VA_PRESET_CHOIR] = CHANGES(CHOIR),
    [VA_PRESET_NOISE] = CHANGES(NOISE),
    [VA_PRESET_RISER] = CHANGES(RISER),
    [VA_PRESET_DOWNLIFTER] = CHANGES(DOWNLIFTER),
    [VA_PRESET_SHAKER] = CHANGES(SHAKER),
    [VA_PRESET_PLATE] = CHANGES(PLATE),
    [VA_PRESET_DUB_CHORD] = CHANGES(DUB_CHORD),
    [VA_PRESET_DEEP_PAD] = CHANGES(DEEP_PAD),
    [VA_PRESET_DEEP_STAB] = CHANGES(DEEP_STAB),
    [VA_PRESET_BOOM] = CHANGES(BOOM),
    [VA_PRESET_RUMBLE] = CHANGES(RUMBLE),
    [VA_PRESET_HOOVER] = CHANGES(HOOVER),
    [VA_PRESET_GOA_LEAD] = CHANGES(GOA_LEAD),
    [VA_PRESET_SQUELCH] = CHANGES(SQUELCH),
    [VA_PRESET_RUBBER] = CHANGES(RUBBER),
    [VA_PRESET_BLEEP] = CHANGES(BLEEP),
    [VA_PRESET_CLICK] = CHANGES(CLICK),
    [VA_PRESET_SAX] = CHANGES(SAX),
    [VA_PRESET_LUTE] = CHANGES(LUTE),
    [VA_PRESET_FLUTE] = CHANGES(FLUTE),
    [VA_PRESET_BAGPIPE] = CHANGES(BAGPIPE),
    [VA_PRESET_DROPS] = CHANGES(DROPS),
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
