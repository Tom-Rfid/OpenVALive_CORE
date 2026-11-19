/*
 * OpenVALive - simple C API implementation, see ols_synth.h
 */
#include "ols_synth.h"

void ols_synth_init(ols_synth_t *s, float sample_rate, int16_t *delay_buf, uint32_t delay_len)
{
#ifndef DSP_SR_F
    dsp_set_sample_rate(sample_rate);     /* before the engine init: coefficients depend on it */
#else
    (void)sample_rate;
#endif
    engine_va_init(&s->va, delay_buf, delay_len);
    s->engine.api = &ENGINE_VA_API;
    s->engine.ctx = &s->va;
    midi_in_init(&s->midi, &s->engine, 0xFF);
    midi_in_map_cc(&s->midi, 74, VA_P_CUTOFF);
    midi_in_map_cc(&s->midi, 71, VA_P_RESO);
    midi_in_map_cc(&s->midi, 73, VA_P_A_ATTACK);
    midi_in_map_cc(&s->midi, 72, VA_P_A_RELEASE);
    midi_in_map_cc(&s->midi, 5, VA_P_GLIDE);
    midi_in_map_cc(&s->midi, 1, VA_P_LFO1_PITCH);
    midi_in_map_cc(&s->midi, 93, VA_P_CHORUS_MIX);
    midi_in_map_cc(&s->midi, 94, VA_P_DELAY_MIX);
    s->out_gain = 1.0f;
}

void ols_synth_midi(ols_synth_t *s, const uint8_t *bytes, int len)
{
    for (int i = 0; i < len; i++) midi_in_byte(&s->midi, bytes[i]);
}

void ols_synth_usb_midi(ols_synth_t *s, const uint8_t packet[4]) { midi_in_usb_packet(&s->midi, packet); }
void ols_synth_note_on(ols_synth_t *s, uint8_t note, uint8_t velocity) { engine_va_note_on(&s->va, note, velocity); }
void ols_synth_note_off(ols_synth_t *s, uint8_t note) { engine_va_note_off(&s->va, note); }
void ols_synth_all_notes_off(ols_synth_t *s) { engine_va_all_notes_off(&s->va); }
void ols_synth_set_param(ols_synth_t *s, uint16_t id, float v) { engine_va_set_param(&s->va, id, v); }
float ols_synth_get_param(const ols_synth_t *s, uint16_t id) { return id < VA_P_COUNT ? s->va.p[id] : 0.0f; }
uint16_t ols_synth_param_count(void) { return VA_P_COUNT; }

const char *ols_synth_param_name(uint16_t id)
{
    return id < VA_P_COUNT ? ENGINE_VA_API.param_names[id] : "";
}

void ols_synth_load_preset(ols_synth_t *s, va_preset_t preset)
{
    engine_va_all_notes_off(&s->va);
    engine_va_load_preset(&s->va, preset);
}

void ols_synth_map_cc(ols_synth_t *s, uint8_t cc, uint16_t param_id) { midi_in_map_cc(&s->midi, cc, param_id); }
void ols_synth_set_gain(ols_synth_t *s, float gain) { s->out_gain = gain; }

void ols_synth_render_f32(ols_synth_t *s, float *left, float *right, int n)
{
    engine_va_render(&s->va, left, right, n);
    if (s->out_gain != 1.0f)
        for (int i = 0; i < n; i++) { left[i] *= s->out_gain; right[i] *= s->out_gain; }
}

void ols_synth_render_i16(ols_synth_t *s, int16_t *out, int n, float *scratch)
{
    float *l = scratch, *r = scratch + n;
    ols_synth_render_f32(s, l, r, n);
    for (int i = 0; i < n; i++) {
        out[2 * i]     = (int16_t)(dsp_clampf(l[i], -1.0f, 1.0f) * 32767.0f);
        out[2 * i + 1] = (int16_t)(dsp_clampf(r[i], -1.0f, 1.0f) * 32767.0f);
    }
}
