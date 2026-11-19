/*
 * OpenVALive - minimal C API example (connector)
 *
 * Plays an arpeggio with the LEAD preset at 44.1 kHz and writes a WAV file.
 * This is the skeleton of any port: init, MIDI in, render by blocks.
 *
 *   make build/api_example && build/api_example build/api_example.wav
 */
#include <stdio.h>
#include "../../core/api/ols_synth.h"

#define SR      44100
#define BLOCK   128
#define SECONDS 6

static int16_t delay_ram[16384];
static ols_synth_t synth;

static void put32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void put16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "api_example.wav";
    static const uint8_t arp[8] = { 57, 60, 64, 69, 72, 69, 64, 60 };
    static int16_t pcm[BLOCK * 2];
    static float scratch[BLOCK * 2];
    const int total_blocks = SR * SECONDS / BLOCK;
    const int step_blocks = SR / 8 / BLOCK;           /* one note every 2 * step_blocks = 0.25 s */
    FILE *f = fopen(path, "wb");
    if (!f) return 1;

    ols_synth_init(&synth, (float)SR, delay_ram, 16384);
    ols_synth_load_preset(&synth, VA_PRESET_LEAD);
    ols_synth_set_param(&synth, VA_P_DELAY_MIX, 0.35f);

    uint32_t data = (uint32_t)total_blocks * BLOCK * 4;
    fwrite("RIFF", 1, 4, f); put32(f, 36 + data); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put32(f, 16); put16(f, 1); put16(f, 2);
    put32(f, SR); put32(f, SR * 4); put16(f, 4); put16(f, 16);
    fwrite("data", 1, 4, f); put32(f, data);

    int last = -1;
    for (int b = 0; b < total_blocks; b++) {
        int step = b / (step_blocks * 2);
        if (step != last && b < total_blocks - SR / BLOCK) {      /* last second: let the notes release */
            uint8_t msg[6] = { 0x80, 0, 0, 0x90, arp[step % 8], 100 };
            msg[1] = last >= 0 ? arp[last % 8] : arp[0];
            ols_synth_midi(&synth, msg + 3, 3);                   /* note on before note off: legato */
            if (last >= 0) ols_synth_midi(&synth, msg, 3);
            uint8_t cc[3] = { 0xB0, 74, (uint8_t)(40 + (step * 5) % 80) };   /* CC74 = cutoff */
            ols_synth_midi(&synth, cc, 3);
            last = step;
        } else if (b == total_blocks - SR / BLOCK) {
            ols_synth_all_notes_off(&synth);
        }
        ols_synth_render_i16(&synth, pcm, BLOCK, scratch);
        fwrite(pcm, sizeof(int16_t), BLOCK * 2, f);
    }
    fclose(f);
    printf("%s: %d s at %d Hz, %u parameters (\"%s\" ... \"%s\")\n", path, SECONDS, SR,
           (unsigned)ols_synth_param_count(), ols_synth_param_name(0),
           ols_synth_param_name((uint16_t)(ols_synth_param_count() - 1)));
    return 0;
}
