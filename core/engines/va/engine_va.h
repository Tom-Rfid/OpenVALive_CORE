/*
 * OpenVALive - VA (Virtual Analog) engine, public interface
 *
 *   OSC1 (PolyBLEP) --+
 *   OSC2 (PolyBLEP) --+
 *   SUB  (-1 octave) -+--> MIX --> ZDF SVF --> DRIVE --> VCA (ADSR)     (per voice)
 *   NOISE ------------+               ^
 *                    filter env + LFO2 + key tracking
 *
 *   sum of voices --> DELAY (mono) --> CHORUS (stereo) --> soft clip --> L / R
 *
 *   LFO1, LFO2: free, or locked to the tempo (1/16 .. 4 bars), free-running, restarted by the first key, or
 *   one cycle only. MODULATION MATRIX: 4 slots, source -> destination x amount (engine_va_mod.c).
 *
 * 4 voices (= 4 notes per sequencer event). Poly mode, or mono legato with glide.
 *
 * Source files:
 *   engine_va.c          API table, init, small accessors
 *   engine_va_params.c   parameter names and value mapping (0..1 -> Hz, s, ...)
 *   engine_va_presets.c  factory presets
 *   engine_va_voices.c   note on/off, voice allocation, mono note stack
 *   engine_va_render.c   audio rendering
 *   engine_va_mod.c      modulation matrix, tempo-locked and triggered LFOs
 */
#ifndef ENGINE_VA_H
#define ENGINE_VA_H

#include "../../dsp/dsp_common.h"
#include "../../dsp/osc.h"
#include "../../dsp/svf.h"
#include "../../dsp/adsr.h"
#include "../../dsp/lfo.h"
#include "../../dsp/delay.h"
#include "../../dsp/chorus.h"
#include "../../dsp/phaser.h"
#include "../engine.h"

#ifndef VA_VOICES
#define VA_VOICES 4
#endif

#define VA_MONO_STACK 8   /* notes remembered in mono mode (for legato returns) */

/* Parameter identifiers. Every value is normalised 0..1. */
typedef enum {
    /* oscillators */
    VA_P_OSC1_WAVE = 0,   /* saw / square / triangle */
    VA_P_OSC2_WAVE,
    VA_P_OSC2_COARSE,     /* -24 .. +24 semitones */
    VA_P_OSC2_FINE,       /* -50 .. +50 cents */
    VA_P_PW,              /* pulse width of the square waves */

    /* mixer */
    VA_P_MIX_OSC1,
    VA_P_MIX_OSC2,
    VA_P_MIX_SUB,
    VA_P_MIX_NOISE,

    /* filter */
    VA_P_CUTOFF,
    VA_P_RESO,
    VA_P_FILTER_MODE,     /* LP / BP / HP / Notch */
    VA_P_FENV_AMT,        /* bipolar: 0.5 = no envelope */
    VA_P_KEYTRACK,
    VA_P_DRIVE,

    /* filter envelope */
    VA_P_F_ATTACK, VA_P_F_DECAY, VA_P_F_SUSTAIN, VA_P_F_RELEASE,

    /* amplifier envelope */
    VA_P_A_ATTACK, VA_P_A_DECAY, VA_P_A_SUSTAIN, VA_P_A_RELEASE,

    /* LFO1 -> pitch (vibrato) and pulse width */
    VA_P_LFO1_RATE, VA_P_LFO1_WAVE, VA_P_LFO1_PITCH, VA_P_LFO1_PW,

    /* LFO2 -> cutoff */
    VA_P_LFO2_RATE, VA_P_LFO2_WAVE, VA_P_LFO2_CUTOFF,

    /* playing */
    VA_P_GLIDE,
    VA_P_MONO,            /* < 0.5 poly, >= 0.5 mono legato */
    VA_P_VEL_SENS,

    /* effects */
    VA_P_DELAY_TIME, VA_P_DELAY_FB, VA_P_DELAY_TONE, VA_P_DELAY_MIX,
    VA_P_CHORUS_MODE,     /* off / I / II / I+II */
    VA_P_CHORUS_MIX,
    VA_P_VOLUME,

    /* added in 0.2: at the end, so the numbers above never change (saved sets) */
    VA_P_PHASER_RATE,
    VA_P_PHASER_DEPTH,    /* sweep range, and some feedback */
    VA_P_PHASER_MIX,      /* 0 = off (and no CPU) */
    VA_P_CRUSH,           /* bitcrusher: 0 = off, 1 = 4 bits at 1/16 of the rate (grunge, lo-fi) */
    VA_P_VEL_FILTER,      /* accent: velocity opens the filter (up to 3 octaves), the acid squelch */

    /* added in 0.4 (at the end): LFO timing, and the modulation matrix */
    VA_P_LFO1_SYNC,       /* FREE (the rate knob), or a length locked to the tempo: VA_LFO_SYNC_NAMES */
    VA_P_LFO1_TRIG,       /* FREE running / RETRIG by the first key / ONE cycle per first key */
    VA_P_LFO2_SYNC,
    VA_P_LFO2_TRIG,
    VA_P_MOD1_SRC, VA_P_MOD1_DST, VA_P_MOD1_AMT,     /* source, destination, amount (0.5 = none, bipolar) */
    VA_P_MOD2_SRC, VA_P_MOD2_DST, VA_P_MOD2_AMT,
    VA_P_MOD3_SRC, VA_P_MOD3_DST, VA_P_MOD3_AMT,
    VA_P_MOD4_SRC, VA_P_MOD4_DST, VA_P_MOD4_AMT,

    VA_P_COUNT
} va_param_t;

#define VA_MOD_SLOTS 4

/* Modulation sources. LFOs are bipolar; envelopes, velocity, pressure and motion go 0..1; KEY is the note
 * around middle C (+-1 for +-4 octaves); RANDOM is drawn at each note. */
typedef enum {
    VA_SRC_OFF, VA_SRC_LFO1, VA_SRC_LFO2, VA_SRC_FENV, VA_SRC_AENV, VA_SRC_VELOCITY, VA_SRC_PRESSURE,
    VA_SRC_KEY, VA_SRC_RANDOM, VA_SRC_MOTION, VA_SRC_COUNT
} va_mod_src_t;

/* Modulation destinations, and how far the full amount goes: pitch and OSC2 +-24 semitones, PW +-0.45,
 * cutoff +-60 semitones, reso +-1, level x0..x2, MIX towards osc 1 (-) or osc 2 (+), filter envelope +-48
 * semitones, LFO rates x1/8..x8, delay mix +-1. The first seven act per voice; the last three on the engine
 * (from the LFOs, pressure and motion). */
typedef enum {
    VA_DST_OFF, VA_DST_PITCH, VA_DST_OSC2, VA_DST_PW, VA_DST_CUTOFF, VA_DST_RESO, VA_DST_LEVEL, VA_DST_MIX,
    VA_DST_FENV, VA_DST_LFO1_RATE, VA_DST_LFO2_RATE, VA_DST_DELAY, VA_DST_COUNT
} va_mod_dst_t;

/* LFO lengths locked to the tempo, in beats (VA_P_LFO*_SYNC); 0 = FREE */
#define VA_LFO_SYNCS 14
enum { VA_TRIG_FREE, VA_TRIG_RETRIG, VA_TRIG_ONESHOT, VA_TRIG_COUNT };

/* in the order they are played: the acid first, basses, leads and keys, pads, textures, INIT last */
typedef enum {
    VA_PRESET_ACID = 0,
    VA_PRESET_BASS,
    VA_PRESET_SUB,
    VA_PRESET_LEAD,
    VA_PRESET_PLUCK,
    VA_PRESET_BRASS,
    VA_PRESET_ORGAN,
    VA_PRESET_PAD,
    VA_PRESET_STRINGS,
    VA_PRESET_SWEEP,
    VA_PRESET_GRUNGE,
    VA_PRESET_SCREECH,
    VA_PRESET_DRONE,
    VA_PRESET_WIND,
    VA_PRESET_HOLD,       /* underground: a ship's hold, creaks and groans */
    VA_PRESET_WHALE,
    VA_PRESET_INIT,
    VA_PRESET_ACID_SQUARE,  /* added later: numbered after INIT (saves keep their numbers), placed by SOUND_ORDER */
    VA_PRESET_ACID_SCREAM,
    VA_PRESET_ACID_DEEP,
    VA_PRESET_REESE,
    VA_PRESET_WOBBLE,
    VA_PRESET_DEEP_SUB,
    VA_PRESET_DUB_SIREN,
    VA_PRESET_STAB,
    VA_PRESET_POLY,
    VA_PRESET_THUNDER,      /* the weather and a door: noise and resonance */
    VA_PRESET_RAIN,
    VA_PRESET_CREAK,
    VA_PRESET_STRINGS70,    /* vintage: a 70s string machine, an electric piano */
    VA_PRESET_VKEYS,
    VA_PRESET_TALK,         /* psytrance and transitions */
    VA_PRESET_PSY_BASS,
    VA_PRESET_ZAP,
    VA_PRESET_LASER,
    VA_PRESET_BLIP,
    VA_PRESET_METAL,
    VA_PRESET_ARP,
    VA_PRESET_CHOIR,
    VA_PRESET_NOISE,
    VA_PRESET_RISER,
    VA_PRESET_DOWNLIFTER,
    VA_PRESET_SHAKER,
    VA_PRESET_PLATE,        /* deep and dub, between acid, techno and psytrance */
    VA_PRESET_DUB_CHORD,
    VA_PRESET_DEEP_PAD,
    VA_PRESET_DEEP_STAB,
    VA_PRESET_BOOM,
    VA_PRESET_RUMBLE,
    VA_PRESET_HOOVER,
    VA_PRESET_GOA_LEAD,
    VA_PRESET_SQUELCH,
    VA_PRESET_RUBBER,       /* minimal */
    VA_PRESET_BLEEP,
    VA_PRESET_CLICK,
    VA_PRESET_COUNT
} va_preset_t;

/* One voice. */
typedef struct {
    dsp_osc_t osc1, osc2, sub;
    dsp_svf_t svf;
    dsp_adsr_t amp;              /* amplifier envelope */
    dsp_adsr_t fenv;             /* filter envelope */
    float note_cur;              /* current pitch (glides towards note_target) */
    float note_target;
    float velocity;              /* gain from velocity, 0..1 */
    float accent;                /* velocity 0..1, for the filter */
    uint8_t note;
    uint8_t gate;                /* key held */
    uint8_t in_period;           /* rendered during the current control period */
    uint8_t needs_control;       /* started mid-period: pitch/filter not set yet */
    float rnd;                   /* the RANDOM modulation source, drawn at the note, -1..1 */
    float gain_cur, gain_step;   /* LEVEL modulation: the gain now, its ramp per sample */
    float mix1, mix2;            /* MIX modulation: osc 1 and osc 2 gains (1 = as the mixer says) */
    uint32_t age;                /* for voice stealing */
} va_voice_t;

/* Values derived from the 0..1 parameters, computed in set_param so the
 * render loop never converts anything. */
typedef struct {
    osc_wave_t wave1, wave2;
    svf_mode_t filter_mode;
    float osc2_offset;           /* semitones */
    float pw;
    float gain_osc1, gain_osc2, gain_sub, gain_noise;
    float cutoff_note;           /* cutoff as a MIDI note number */
    float reso;
    float fenv_amount;           /* semitones */
    float keytrack;
    float drive_gain;
    float lfo1_to_pitch, lfo1_to_pw, lfo2_to_cutoff;
    float glide_coef;
    float vel_sens;
    float delay_mix;
    float chorus_mix;
    float volume;
    float phaser_mix;
    float phaser_span;           /* octaves swept above 150 Hz */
    float pressure;              /* aftertouch 0..1: opens the filter, deepens the vibrato */
    float crush_levels;          /* bitcrusher: quantisation steps per unit (0 = off) */
    float vel_filter;            /* semitones of cutoff at full velocity */
    int crush_hold;              /* bitcrusher: samples held (rate reduction) */
    int mono;
    /* modulation (engine_va_mod.c) */
    uint8_t mod_src[VA_MOD_SLOTS], mod_dst[VA_MOD_SLOTS];
    float mod_amt[VA_MOD_SLOTS];          /* -1..+1 */
    uint8_t mod_voice;                    /* a slot acts per voice: the voices compute it */
    uint8_t mod_engine;                   /* a slot acts on the engine (LFO rates, delay) */
    float lfo_sync[2];                    /* beats per cycle, 0 = free */
    uint8_t lfo_trig[2];                  /* VA_TRIG_* */
} va_derived_t;

/* Continuous values that glide towards their target (va_derived_t) at
 * control rate, so turning a knob, a MIDI CC or Morph never clicks. */
typedef struct {
    float cutoff_note;
    float reso;
    float fenv_amount;
    float gain_osc1, gain_osc2, gain_sub, gain_noise;
    float drive_gain;
    float drive_comp;            /* derived from drive_gain each block */
    float volume;
    float delay_mix;
    float chorus_mix;
    float phaser_mix;
    float pressure;
    float pw;
} va_smoothed_t;

/* Complete engine state. Allocate it statically: it is about 3 KB. */
typedef struct {
    float p[VA_P_COUNT];         /* parameters, normalised 0..1 */
    va_derived_t d;              /* targets, computed by set_param */
    va_smoothed_t s;             /* what the audio actually uses, gliding towards d */
    float smooth_coef;           /* per control block: ~5 ms time constant */
    float role_off[OLS_ROLE_COUNT];   /* performance offsets (macros), see engine.h */

    va_voice_t voice[VA_VOICES];
    int voice_limit;             /* usable voices (lowered when the CPU is overloaded) */
    uint32_t age_counter;

    uint8_t mono_stack[VA_MONO_STACK];
    uint8_t mono_count;
    float bend;                  /* pitch bend, semitones */

    dsp_lfo_t lfo1, lfo2, lfo_phaser;
    float lfo_inc[2];            /* the rate knobs' phase steps (before rate modulation) */
    float lfo_origin[2];         /* beats when the LFO was last started by a key (tempo-locked + RETRIG/ONE) */
    uint8_t lfo_done[2];         /* ONE: the cycle is over, the value holds */
    float bpm, beats;            /* the clock (engine.h clock): tempo, and beats at its last call */
    uint32_t since_clock;        /* samples rendered since */
    float motion;                /* the motion lane, 0..1 */
    uint32_t mod_rng;            /* the RANDOM source's own generator (the noise oscillator keeps rng) */
    float mod_engine_now[VA_DST_COUNT];   /* this period's engine-wide modulation */
    float phaser_lfo_now;        /* 0..1 for the current control period */
    dsp_delay_t delay;
    dsp_chorus_t chorus;
    dsp_phaser_t phaser;
    float crush_value;           /* bitcrusher: the held sample */
    int crush_count;
    uint32_t rng;

    /* Control rate: exactly every DSP_BLOCK samples, even when the host
     * splits a block to apply an event at an exact sample. */
    int ctrl_left;               /* samples left in the current control period */
    float lfo1_now, lfo2_now;    /* LFO values for the current period */

    /* Sleep: an engine with no voice and no audible tail renders nothing.
     * Four tracks idle cost almost nothing this way. */
    uint32_t quiet;              /* samples of silence in a row */
    uint32_t sleep_after;        /* long enough for the delay line to hold only silence */

    float mixbuf[DSP_BLOCK];     /* voice sum for one control block */
} engine_va_t;

/* --- lifecycle ------------------------------------------------------- */

/* delay_buf: int16 RAM given by the platform, may be 0 (no delay). */
void engine_va_init(engine_va_t *e, int16_t *delay_buf, uint32_t delay_len);
void engine_va_load_preset(engine_va_t *e, va_preset_t preset);

/* Jumps every smoothed value to its target at once (after a preset load). */
void engine_va_snap(engine_va_t *e);

/* Overload protection: limits the number of voices (1..VA_VOICES). */
void engine_va_set_voice_limit(engine_va_t *e, int n);

/* --- generic engine API (also reachable through ENGINE_VA_API) -------- */

void  engine_va_note_on(void *ctx, uint8_t note, uint8_t velocity);
void  engine_va_note_off(void *ctx, uint8_t note);
void  engine_va_all_notes_off(void *ctx);
void  engine_va_set_param(void *ctx, uint16_t id, float value01);
float engine_va_get_param(void *ctx, uint16_t id);
void  engine_va_pitch_bend(void *ctx, float semitones);
void  engine_va_pressure(void *ctx, float amount01);
void  engine_va_role_offset(void *ctx, uint16_t role, float offset);
void  engine_va_render(void *ctx, float *out_l, float *out_r, int n);
void  engine_va_clock(void *ctx, float bpm, float beats);
void  engine_va_motion(void *ctx, float value01);

extern const ols_engine_api_t ENGINE_VA_API;
extern const char *const VA_PARAM_NAMES[VA_P_COUNT];
extern const uint8_t VA_PARAM_KINDS[VA_P_COUNT];
extern const char *const VA_PRESET_NAMES[VA_PRESET_COUNT];
extern const char *const VA_MOD_SRC_NAMES[VA_SRC_COUNT];
extern const char *const VA_MOD_DST_NAMES[VA_DST_COUNT];
extern const char *const VA_LFO_SYNC_NAMES[VA_LFO_SYNCS];
extern const char *const VA_LFO_TRIG_NAMES[VA_TRIG_COUNT];

#endif /* ENGINE_VA_H */
