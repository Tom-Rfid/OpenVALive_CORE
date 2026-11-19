/*
 * OpenVALive - VA engine: notes and voice allocation.
 *
 * Poly mode: a new note takes, in this order,
 *   1. the voice already playing the same note,
 *   2. a free (silent) voice,
 *   3. the oldest released voice,
 *   4. the oldest voice.
 *
 * Mono mode: one voice and a small note stack. The last note wins; releasing
 * it glides back to the previous key still held (legato, like a mono synth).
 */
#include "engine_va_internal.h"

void va_voices_init(engine_va_t *e)
{
    for (int i = 0; i < VA_VOICES; i++) {
        va_voice_t *v = &e->voice[i];

        osc_init(&v->osc1, 0.0f);

        /* different start phases per voice sound more "analog" */
        float phase2 = 0.37f * (float)(i + 1);
        while (phase2 >= 1.0f) {
            phase2 -= 1.0f;
        }
        osc_init(&v->osc2, phase2);

        osc_init(&v->sub, 0.0f);
        svf_reset(&v->svf);
        adsr_init(&v->amp);
        adsr_init(&v->fenv);
        v->note_cur = 60.0f;
        v->note_target = 60.0f;
        v->gain_cur = 1.0f;
        v->mix1 = 1.0f;
        v->mix2 = 1.0f;
    }
}

static void voice_start(engine_va_t *e, va_voice_t *v, uint8_t note, uint8_t velocity, int retrigger)
{
    float vel01 = (float)velocity * (1.0f / 127.0f);
    v->velocity = 1.0f - e->d.vel_sens * (1.0f - vel01);
    v->accent = vel01;

    v->note = note;
    v->note_target = (float)note;
    v->rnd = dsp_noise(&e->mod_rng);         /* the RANDOM modulation source of this note */

    /* a silent voice, or no glide: jump straight to the new pitch */
    if (!adsr_active(&v->amp) || e->d.glide_coef >= 1.0f) {
        v->note_cur = (float)note;
    }

    v->gate = 1;
    e->age_counter++;
    v->age = e->age_counter;

    if (retrigger) {
        adsr_gate_on(&v->amp);
        adsr_gate_on(&v->fenv);
        /* a voice started between two control ticks gets its pitch and
         * filter before its first sample (see engine_va_render) */
        v->in_period = 1;
        v->needs_control = 1;
    }
}

void va_voice_stop(va_voice_t *v)
{
    v->gate = 0;
    adsr_gate_off(&v->amp);
    adsr_gate_off(&v->fenv);
}

/* ------------------------------------------------------------ mono mode */

static void mono_stack_remove(engine_va_t *e, uint8_t note)
{
    uint8_t kept = 0;
    for (uint8_t i = 0; i < e->mono_count; i++) {
        if (e->mono_stack[i] != note) {
            e->mono_stack[kept] = e->mono_stack[i];
            kept++;
        }
    }
    e->mono_count = kept;
}

static void mono_stack_push(engine_va_t *e, uint8_t note)
{
    mono_stack_remove(e, note);

    if (e->mono_count >= VA_MONO_STACK) {
        /* full: forget the oldest note */
        for (uint8_t i = 1; i < VA_MONO_STACK; i++) {
            e->mono_stack[i - 1] = e->mono_stack[i];
        }
        e->mono_count--;
    }
    e->mono_stack[e->mono_count] = note;
    e->mono_count++;
}

static void mono_note_on(engine_va_t *e, uint8_t note, uint8_t velocity)
{
    mono_stack_push(e, note);

    va_voice_t *v = &e->voice[0];
    int legato = v->gate;                    /* a key is already held: no retrigger */
    voice_start(e, v, note, velocity, !legato);
}

static void mono_note_off(engine_va_t *e, uint8_t note)
{
    mono_stack_remove(e, note);
    va_voice_t *v = &e->voice[0];

    if (e->mono_count > 0) {
        /* glide back to the most recent key still held */
        v->note = e->mono_stack[e->mono_count - 1];
        v->note_target = (float)v->note;
    } else if (v->gate) {
        va_voice_stop(v);
    }
}

/* ------------------------------------------------------------ poly mode */

static va_voice_t *find_voice(engine_va_t *e, uint8_t note)
{
    const int n = e->voice_limit;

    /* 1. same note already sounding */
    for (int i = 0; i < n; i++) {
        if (e->voice[i].note == note && adsr_active(&e->voice[i].amp)) {
            return &e->voice[i];
        }
    }

    /* 2. a silent voice */
    for (int i = 0; i < n; i++) {
        if (!adsr_active(&e->voice[i].amp)) {
            return &e->voice[i];
        }
    }

    /* 3. the oldest released voice */
    va_voice_t *oldest_released = 0;
    for (int i = 0; i < n; i++) {
        va_voice_t *v = &e->voice[i];
        if (!v->gate && (oldest_released == 0 || v->age < oldest_released->age)) {
            oldest_released = v;
        }
    }
    if (oldest_released != 0) {
        return oldest_released;
    }

    /* 4. the oldest voice */
    va_voice_t *oldest = &e->voice[0];
    for (int i = 1; i < n; i++) {
        if (e->voice[i].age < oldest->age) {
            oldest = &e->voice[i];
        }
    }
    return oldest;
}

/* ---------------------------------------------------------- public API */

void engine_va_note_on(void *ctx, uint8_t note, uint8_t velocity)
{
    engine_va_t *e = (engine_va_t *)ctx;

    if (velocity == 0) {                     /* MIDI convention: velocity 0 = note off */
        engine_va_note_off(ctx, note);
        return;
    }

    int held = 0;
    for (int i = 0; i < VA_VOICES; i++) {
        held |= e->voice[i].gate;
    }
    if (!held) {
        va_lfo_trigger(e);                   /* the first key of a phrase: RETRIG / ONE LFOs start again */
    }

    if (e->d.mono) {
        mono_note_on(e, note, velocity);
    } else {
        voice_start(e, find_voice(e, note), note, velocity, 1);
    }
}

void engine_va_note_off(void *ctx, uint8_t note)
{
    engine_va_t *e = (engine_va_t *)ctx;

    if (e->d.mono) {
        mono_note_off(e, note);
        return;
    }

    for (int i = 0; i < VA_VOICES; i++) {
        if (e->voice[i].gate && e->voice[i].note == note) {
            va_voice_stop(&e->voice[i]);
        }
    }
}

void engine_va_all_notes_off(void *ctx)
{
    engine_va_t *e = (engine_va_t *)ctx;
    e->mono_count = 0;
    for (int i = 0; i < VA_VOICES; i++) {
        va_voice_stop(&e->voice[i]);
    }
}
