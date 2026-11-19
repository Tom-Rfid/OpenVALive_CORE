/*
 * OpenVALive - PC simulator: 16-bar demo, mono bass + 4-voice pad.
 *
 * Everything goes through MIDI bytes and the MIDI parser, like on the FM-1.
 * The bass cutoff and resonance are swept with CC74 / CC71 (a "Morph"
 * gesture), the pad plays Am - F - C - G as 4-note chords.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sim.h"

static int16_t delay_bass[SIM_DELAY_LEN];
static int16_t delay_pad[SIM_DELAY_LEN];
static engine_va_t bass;
static engine_va_t pad;
static ols_engine_t bass_engine = { &ENGINE_VA_API, &bass };
static ols_engine_t pad_engine = { &ENGINE_VA_API, &pad };

static void send3(midi_in_t *m, uint8_t a, uint8_t b, uint8_t c)
{
    midi_in_byte(m, a);
    midi_in_byte(m, b);
    midi_in_byte(m, c);
}

/* Am - F - C - G, four notes each (= one sequencer event) */
static const uint8_t CHORDS[4][4] = {
    { 57, 60, 64, 67 }, { 53, 57, 60, 64 }, { 55, 60, 64, 67 }, { 55, 59, 62, 67 }
};
static const uint8_t ROOTS[4] = { 33, 29, 36, 31 };
/* one bar of eighth notes: semitone offset from the root, 0xFF = rest */
static const uint8_t BASS_PATTERN[8] = { 0, 0, 12, 0, 0xFF, 0, 7, 10 };

int sim_demo(const char *path)
{
    const float bpm = 120.0f;
    const int bars = 16;
    const float seconds_per_beat = 60.0f / bpm;
    const int total = (int)((float)(bars * 4 + 2) * seconds_per_beat * DSP_SR);

    engine_va_init(&bass, delay_bass, SIM_DELAY_LEN);
    engine_va_init(&pad, delay_pad, SIM_DELAY_LEN);
    engine_va_load_preset(&bass, VA_PRESET_BASS);
    engine_va_load_preset(&pad, VA_PRESET_PAD);

    midi_in_t midi_bass, midi_pad;
    midi_in_init(&midi_bass, &bass_engine, 0);   /* channel 1 */
    midi_in_init(&midi_pad, &pad_engine, 1);     /* channel 2 */
    midi_in_map_cc(&midi_bass, 74, VA_P_CUTOFF);
    midi_in_map_cc(&midi_bass, 71, VA_P_RESO);
    midi_in_map_cc(&midi_pad, 74, VA_P_CUTOFF);

    float *out_l = calloc((size_t)total, sizeof(float));
    float *out_r = calloc((size_t)total, sizeof(float));
    float bl[DSP_BLOCK], br[DSP_BLOCK], pl[DSP_BLOCK], pr[DSP_BLOCK];

    int last_bass = -1, last_chord = -1, last_step = -1;
    clock_t t0 = clock();

    for (int pos = 0; pos < total; pos += DSP_BLOCK) {
        float beat = (float)pos / DSP_SR / seconds_per_beat;
        int step = (int)(beat * 2.0f);           /* eighth notes */

        if (step != last_step && beat < (float)(bars * 4)) {
            last_step = step;
            int s = step % 8;
            int chord = (step / 8) % 4;

            /* bass, legato: the new note before the old note-off */
            uint8_t offset = BASS_PATTERN[s];
            if (offset != 0xFF) {
                int note = ROOTS[chord] + offset;
                send3(&midi_bass, 0x90, (uint8_t)note, (s % 2) ? 80 : 120);
                if (last_bass >= 0 && last_bass != note) {
                    send3(&midi_bass, 0x80, (uint8_t)last_bass, 0);
                }
                last_bass = note;
            } else if (last_bass >= 0) {
                send3(&midi_bass, 0x80, (uint8_t)last_bass, 0);
                last_bass = -1;
            }

            /* pad: one chord per bar */
            if (s == 0) {
                for (int k = 0; k < 4 && last_chord >= 0; k++) {
                    send3(&midi_pad, 0x81, CHORDS[last_chord][k], 0);
                }
                for (int k = 0; k < 4; k++) {
                    send3(&midi_pad, 0x91, CHORDS[chord][k], 90);
                }
                last_chord = chord;
            }

            /* "Morph": bass cutoff up over 8 bars, then back down */
            float phase = beat / 32.0f;
            float morph = phase < 1.0f ? phase : 2.0f - phase;
            send3(&midi_bass, 0xB0, 74, (uint8_t)(30.0f + 60.0f * morph));
            send3(&midi_bass, 0xB0, 71, (uint8_t)(50.0f + 50.0f * morph));
        }

        /* end of the song: release everything */
        if (beat >= (float)(bars * 4) && last_chord >= 0) {
            for (int k = 0; k < 4; k++) {
                send3(&midi_pad, 0x81, CHORDS[last_chord][k], 0);
            }
            if (last_bass >= 0) {
                send3(&midi_bass, 0x80, (uint8_t)last_bass, 0);
            }
            last_chord = -1;
            last_bass = -1;
        }

        int n = total - pos < DSP_BLOCK ? total - pos : DSP_BLOCK;
        engine_va_render(&bass, bl, br, n);
        engine_va_render(&pad, pl, pr, n);
        for (int i = 0; i < n; i++) {
            out_l[pos + i] = 0.55f * bl[i] + 0.5f * pl[i];
            out_r[pos + i] = 0.55f * br[i] + 0.5f * pr[i];
        }
    }

    double secs = (double)(clock() - t0) / CLOCKS_PER_SEC;
    double audio_secs = (double)total / DSP_SR;
    printf("Demo: %d bars at %.0f BPM, %.1f s of audio rendered in %.3f s (x%.0f real time on this PC)\n",
           bars, bpm, audio_secs, secs, audio_secs / secs);
    signal_report(out_l, out_r, total);

    int rc = wav_write(path, out_l, out_r, total, SIM_SR);
    printf("  -> %s\n", rc ? "WAV WRITE ERROR" : path);
    free(out_l);
    free(out_r);
    return rc;
}
