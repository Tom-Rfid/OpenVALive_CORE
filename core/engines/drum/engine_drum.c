/*
 * OpenVALive - DRUM engine: parameters, kits, triggering and the mix.
 * The sound of each instrument is in engine_drum_voices.c.
 */
#include "engine_drum.h"
#include "engine_drum_internal.h"

const char *const DR_PARAM_NAMES[DR_P_COUNT] = {
    "TUNE", "DECAY", "CUTOFF", "DRIVE",
    "KckPitc", "KckDec", "KckClik", "KckSwep",
    "SnrTone", "SnrSnap", "SnrDec",
    "HatTone", "HatDec",
    "ClapDec", "TomDec",
    "LvlKick", "LvlSnar", "LvlHat", "LvlPerc",
    "Reso", "Volume",
};

const char *const DR_KIT_NAMES[DR_KIT_COUNT] = { "808", "909", "TIGHT", "LOFI", "IRON" };

static const uint16_t DR_ROLES[OLS_ROLE_COUNT] = {
    [OLS_ROLE_CUTOFF] = DR_P_CUTOFF,
    [OLS_ROLE_RESO] = DR_P_RESO,
    [OLS_ROLE_DRIVE] = DR_P_DRIVE,
    [OLS_ROLE_DELAY_MIX] = OLS_NO_PARAM,
    [OLS_ROLE_VOLUME] = DR_P_VOLUME,
    [OLS_ROLE_CRUSH] = OLS_NO_PARAM,
    [OLS_ROLE_DECAY] = DR_P_DECAY,
    [OLS_ROLE_RELEASE] = OLS_NO_PARAM,
    [OLS_ROLE_FENV] = OLS_NO_PARAM,
    [OLS_ROLE_CHORUS] = OLS_NO_PARAM,
};

static const uint16_t DR_MACROS[OLS_MACROS] = { DR_P_TUNE, DR_P_DECAY, DR_P_CUTOFF, DR_P_DRIVE };
static const char *const DR_MACRO_NAMES[OLS_MACROS] = { "TUNE", "DECAY", "CUTOFF", "DRIVE" };

const ols_engine_api_t ENGINE_DRUM_API = {
    .name = "DRUM",
    .param_count = DR_P_COUNT,
    .param_names = DR_PARAM_NAMES,
    .param_kinds = 0,                       /* all continuous */
    .note_on = engine_drum_note_on,
    .note_off = engine_drum_note_off,
    .all_notes_off = engine_drum_all_notes_off,
    .set_param = engine_drum_set_param,
    .get_param = engine_drum_get_param,
    .pitch_bend = engine_drum_pitch_bend,
    .render = engine_drum_render,
    .roles = DR_ROLES,
    .macros = DR_MACROS,
    .macro_names = DR_MACRO_NAMES,
    .role_offset = engine_drum_role_offset,
};

/* ---------------------------------------------------------------- kits */

static const float KIT_808[DR_P_COUNT] = {
    [DR_P_TUNE] = 0.5f, [DR_P_DECAY] = 0.5f, [DR_P_CUTOFF] = 1.0f, [DR_P_DRIVE] = 0.1f,
    [DR_P_KICK_PITCH] = 0.35f, [DR_P_KICK_DECAY] = 0.6f, [DR_P_KICK_CLICK] = 0.3f, [DR_P_KICK_SWEEP] = 0.5f,
    [DR_P_SNARE_TONE] = 0.5f, [DR_P_SNARE_SNAP] = 0.55f, [DR_P_SNARE_DECAY] = 0.45f,
    [DR_P_HAT_TONE] = 0.5f, [DR_P_HAT_DECAY] = 0.45f,
    [DR_P_CLAP_DECAY] = 0.5f, [DR_P_TOM_DECAY] = 0.5f,
    [DR_P_LEVEL_KICK] = 0.9f, [DR_P_LEVEL_SNARE] = 0.75f, [DR_P_LEVEL_HAT] = 0.55f, [DR_P_LEVEL_PERC] = 0.7f,
    [DR_P_RESO] = 0.1f, [DR_P_VOLUME] = 0.7f,
};

/* 909 style from the same voices: a higher, shorter kick with a strong click and a fast sweep, a snare with more
 * noise (snap), bright short metallic hats */
static void kit_909(engine_drum_t *e)
{
    engine_drum_set_param(e, DR_P_KICK_PITCH, 0.45f);
    engine_drum_set_param(e, DR_P_KICK_DECAY, 0.42f);
    engine_drum_set_param(e, DR_P_KICK_CLICK, 0.8f);
    engine_drum_set_param(e, DR_P_KICK_SWEEP, 0.72f);
    engine_drum_set_param(e, DR_P_SNARE_TONE, 0.62f);
    engine_drum_set_param(e, DR_P_SNARE_SNAP, 0.85f);
    engine_drum_set_param(e, DR_P_SNARE_DECAY, 0.4f);
    engine_drum_set_param(e, DR_P_HAT_TONE, 0.78f);
    engine_drum_set_param(e, DR_P_HAT_DECAY, 0.35f);
    engine_drum_set_param(e, DR_P_CLAP_DECAY, 0.45f);
    engine_drum_set_param(e, DR_P_TOM_DECAY, 0.45f);
    engine_drum_set_param(e, DR_P_DRIVE, 0.2f);
}

static void kit_tight(engine_drum_t *e)
{
    engine_drum_set_param(e, DR_P_KICK_PITCH, 0.45f);
    engine_drum_set_param(e, DR_P_KICK_DECAY, 0.35f);
    engine_drum_set_param(e, DR_P_KICK_CLICK, 0.6f);
    engine_drum_set_param(e, DR_P_KICK_SWEEP, 0.65f);
    engine_drum_set_param(e, DR_P_SNARE_TONE, 0.65f);
    engine_drum_set_param(e, DR_P_SNARE_SNAP, 0.75f);
    engine_drum_set_param(e, DR_P_SNARE_DECAY, 0.3f);
    engine_drum_set_param(e, DR_P_HAT_DECAY, 0.3f);
    engine_drum_set_param(e, DR_P_HAT_TONE, 0.65f);
    engine_drum_set_param(e, DR_P_DRIVE, 0.3f);
}

static void kit_lofi(engine_drum_t *e)
{
    engine_drum_set_param(e, DR_P_TUNE, 0.42f);
    engine_drum_set_param(e, DR_P_CUTOFF, 0.55f);
    engine_drum_set_param(e, DR_P_RESO, 0.3f);
    engine_drum_set_param(e, DR_P_DRIVE, 0.6f);
    engine_drum_set_param(e, DR_P_HAT_TONE, 0.35f);
    engine_drum_set_param(e, DR_P_KICK_DECAY, 0.7f);
}

static void kit_iron(engine_drum_t *e)
{
    engine_drum_set_param(e, DR_P_TUNE, 0.35f);
    engine_drum_set_param(e, DR_P_DRIVE, 0.9f);
    engine_drum_set_param(e, DR_P_CUTOFF, 0.75f);
    engine_drum_set_param(e, DR_P_RESO, 0.55f);
    engine_drum_set_param(e, DR_P_KICK_CLICK, 1.0f);
    engine_drum_set_param(e, DR_P_KICK_DECAY, 0.45f);
    engine_drum_set_param(e, DR_P_SNARE_SNAP, 0.9f);
    engine_drum_set_param(e, DR_P_HAT_TONE, 0.2f);
    engine_drum_set_param(e, DR_P_HAT_DECAY, 0.85f);
    engine_drum_set_param(e, DR_P_LEVEL_HAT, 0.4f);
    engine_drum_set_param(e, DR_P_CLAP_DECAY, 0.8f);
}

void engine_drum_load_kit(engine_drum_t *e, dr_kit_t kit)
{
    for (int i = 0; i < DR_P_COUNT; i++) {
        engine_drum_set_param(e, (uint16_t)i, KIT_808[i]);
    }
    switch (kit) {
    case DR_KIT_909:   kit_909(e); break;
    case DR_KIT_TIGHT: kit_tight(e); break;
    case DR_KIT_LOFI:  kit_lofi(e); break;
    case DR_KIT_IRON:  kit_iron(e); break;
    case DR_KIT_808:
    default:
        break;
    }
    e->cutoff_now = e->cutoff_hz;            /* no glide from the previous kit */
}

/* ---------------------------------------------------------------- set-up */

void engine_drum_init(engine_drum_t *e)
{
    for (uint32_t i = 0; i < sizeof *e; i++) {       /* zeroed by a loop: no flash copy of a zero state */
        ((uint8_t *)e)[i] = 0;
    }
    e->rng = 0x7F4A7C15u;
    svf_reset(&e->out_filter);
    for (int i = 0; i < DR_COUNT; i++) {
        svf_reset(&e->voice[i].svf);
    }
    engine_drum_load_kit(e, DR_KIT_808);
}

/* ---------------------------------------------------------------- parameters */

static int drum_role_of(uint16_t id)
{
    switch (id) {
    case DR_P_CUTOFF: return OLS_ROLE_CUTOFF;
    case DR_P_RESO:   return OLS_ROLE_RESO;
    case DR_P_DRIVE:  return OLS_ROLE_DRIVE;
    case DR_P_DECAY:  return OLS_ROLE_DECAY;
    default:          return -1;
    }
}

void engine_drum_role_offset(void *ctx, uint16_t role, float offset)
{
    engine_drum_t *e = (engine_drum_t *)ctx;
    if (role >= OLS_ROLE_COUNT || e->role_off[role] == offset) {
        return;
    }
    e->role_off[role] = offset;
    for (uint16_t id = 0; id < DR_P_COUNT; id++) {
        if (drum_role_of(id) == (int)role) {
            engine_drum_set_param(e, id, e->p[id]);
        }
    }
}

void engine_drum_set_param(void *ctx, uint16_t id, float value)
{
    engine_drum_t *e = (engine_drum_t *)ctx;
    if (id >= DR_P_COUNT) {
        return;
    }
    value = dsp_clampf(value, 0.0f, 1.0f);
    e->p[id] = value;
    int role = drum_role_of(id);
    if (role >= 0) {
        value = dsp_clampf(value + e->role_off[role], 0.0f, 1.0f);   /* the macros ride on top */
    }

    switch (id) {
    case DR_P_TUNE:   e->tune = dsp_exp2((value - 0.5f) * 2.0f); break;          /* +-12 semitones */
    case DR_P_DECAY:  e->decay = dsp_exp2((value - 0.5f) * 4.0f); break;         /* x0.25 .. x4 */
    case DR_P_CUTOFF: e->cutoff_hz = dsp_expmap(value, 200.0f, 20000.0f); break;
    case DR_P_RESO:   e->reso = value * 0.7f; break;
    case DR_P_DRIVE:
        e->drive_gain = 1.0f + 7.0f * value * value;
        e->drive_comp = 0.7f / dsp_tanh(0.7f * e->drive_gain);
        break;
    case DR_P_VOLUME: e->volume = 2.08f * value * value; break;   /* (+30 %: the kits sat under the synths) */
    default:
        break;                                /* read when an instrument is hit */
    }
}

float engine_drum_get_param(void *ctx, uint16_t id)
{
    engine_drum_t *e = (engine_drum_t *)ctx;
    return id < DR_P_COUNT ? e->p[id] : 0.0f;
}

void engine_drum_pitch_bend(void *ctx, float semitones)
{
    (void)ctx;
    (void)semitones;                          /* drums ignore pitch bend */
}

/* ---------------------------------------------------------------- notes */

/* General MIDI's drum map (channel 10), on our voices: six toms on four voices by their pitch, the cymbals on the
 * open hat. A note GM does not name plays as its place in the GM octave 36..47. *pitch: the tom's ratio. */
static dr_inst_t gm_instrument(uint8_t note, float *pitch)
{
    static const uint8_t OCTAVE[12] = {           /* 36 .. 47 */
        DR_KICK, DR_RIM, DR_SNARE, DR_CLAP, DR_SNARE, DR_FTOM, DR_CH, DR_FTOM, DR_CH, DR_LTOM, DR_OH, DR_MTOM,
    };
    *pitch = 1.0f;
    switch (note) {
    case 35: return DR_KICK;
    case 41: *pitch = 0.92f; return DR_FTOM;      /* low floor tom */
    case 43: *pitch = 1.12f; return DR_FTOM;      /* high floor tom */
    case 47: *pitch = 0.9f; return DR_MTOM;       /* low-mid tom */
    case 48: return DR_MTOM;                      /* hi-mid tom */
    case 50: return DR_HTOM;
    case 49: case 51: case 52: case 53: case 55: case 57: case 59: return DR_OH;   /* cymbals */
    case 54: return DR_CH;                        /* tambourine */
    case 56: return DR_COWBELL;
    case 75: return DR_RIM;                       /* claves */
    case 80: case 81: return DR_TRIANGLE;
    default: break;
    }
    dr_inst_t inst = (dr_inst_t)OCTAVE[note % 12];
    if (inst == DR_FTOM) {
        *pitch = note % 12 == 5 ? 0.92f : 1.12f;
    } else if (inst == DR_MTOM) {
        *pitch = 0.9f;
    }
    return inst;
}

dr_inst_t engine_drum_instrument(uint8_t note)
{
    float pitch;
    return gm_instrument(note, &pitch);
}

void engine_drum_note_on(void *ctx, uint8_t note, uint8_t velocity)
{
    engine_drum_t *e = (engine_drum_t *)ctx;
    dr_inst_t inst = gm_instrument(note, &e->hit_pitch);
    float vel = (float)velocity * (1.0f / 127.0f);

    if (inst == DR_CH) {
        e->voice[DR_OH].active = 0;           /* the closed hat chokes the open one */
    }
    dr_trigger(e, inst, 0.25f + 0.75f * vel * vel);
}

void engine_drum_note_off(void *ctx, uint8_t note)
{
    (void)ctx;
    (void)note;                               /* drums play to their end */
}

void engine_drum_all_notes_off(void *ctx)
{
    engine_drum_t *e = (engine_drum_t *)ctx;
    for (int i = 0; i < DR_COUNT; i++) {
        e->voice[i].active = 0;
    }
}

int engine_drum_active(const engine_drum_t *e)
{
    int n = 0;
    for (int i = 0; i < DR_COUNT; i++) {
        n += e->voice[i].active;
    }
    return n;
}

/* ---------------------------------------------------------------- render */

void engine_drum_render(void *ctx, float *out_l, float *out_r, int n)
{
    engine_drum_t *e = (engine_drum_t *)ctx;

    if (engine_drum_active(e) == 0) {
        /* nothing sounding: silence costs nothing (no tail: drums end by themselves) */
        for (int i = 0; i < n; i++) {
            out_l[i] = 0.0f;
            out_r[i] = 0.0f;
        }
        return;
    }

    int offset = 0;
    while (offset < n) {
        if (e->ctrl_left <= 0) {
            /* control rate: the mix filter glides towards its cutoff */
            e->cutoff_now += 0.2f * (e->cutoff_hz - e->cutoff_now);
            svf_set(&e->out_filter, e->cutoff_now, e->reso);
            e->ctrl_left = DSP_BLOCK;
        }
        int count = n - offset;
        if (count > e->ctrl_left) {
            count = e->ctrl_left;
        }

        float *mix = out_l + offset;
        for (int i = 0; i < count; i++) {
            mix[i] = 0.0f;
        }
        for (int v = 0; v < DR_COUNT; v++) {
            if (e->voice[v].active) {
                dr_render_voice(e, (dr_inst_t)v, mix, count);
            }
        }

        const int filtered = e->cutoff_now < 19000.0f;
        for (int i = 0; i < count; i++) {
            float x = mix[i];
            if (filtered) {
                x = svf_process(&e->out_filter, x, SVF_LP);
            }
            x = dsp_tanh(x * e->drive_gain) * e->drive_comp * e->volume;
            out_l[offset + i] = x;
            out_r[offset + i] = x;
        }

        e->ctrl_left -= count;
        offset += count;
    }
}
