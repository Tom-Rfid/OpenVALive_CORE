/*
 * OpenVALive desktop - offline rendering to a WAV file.
 *
 * Same code path as live playback (event queue -> desktop_render), but
 * without an audio device: useful for tests, CI and quick listening.
 * With --demo, a small live set is built with the Live Core and played.
 */
#include <stdio.h>
#include <stdlib.h>
#include "desktop.h"
#include "../../core/live/pattern.h"

#define OFFLINE_RATE 48000
#define OFFLINE_BLOCK 256

static void put_u32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void put_u16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }

static void write_wav_header(FILE *f, uint32_t frames)
{
    uint32_t data = frames * 4;
    fwrite("RIFF", 1, 4, f);
    put_u32(f, 36 + data);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    put_u32(f, 16);
    put_u16(f, 1);
    put_u16(f, 2);
    put_u32(f, OFFLINE_RATE);
    put_u32(f, OFFLINE_RATE * 4);
    put_u16(f, 4);
    put_u16(f, 16);
    fwrite("data", 1, 4, f);
    put_u32(f, data);
}

/* The demo: a 4-bar live set made with the Live Core itself, then played.
 *   track 1  chords (Am F C G), stabs on 1 and the "and" of 2
 *   track 2  bass, a Euclidean 5/16 rhythm on each chord's root
 *   track 3  a lead motif, mutated by the Creative Engine
 *   track 4  drums: kick on the beats, snare on 2 and 4, eighth hats, a clap at the end */
static void build_demo_set(void)
{
    static const uint8_t chords[4][3] = { { 57, 60, 64 }, { 53, 57, 60 }, { 55, 60, 64 }, { 55, 59, 62 } };
    static const uint8_t roots[4] = { 33, 29, 36, 31 };
    static const uint8_t motif[8] = { 69, 72, 76, 74, 72, 69, 67, 69 };
    ols_live_t *L = &app.live;
    const uint32_t bar = OLS_TICKS_PER_BAR;

    for (int t = 0; t < TRACKS; t++) {
        pattern_clear(&L->track[t].pat, 4 * bar);
    }
    for (uint32_t b = 0; b < 4; b++) {
        for (int k = 0; k < 3; k++) {
            pattern_add(&L->track[0].pat, b * bar, 120, chords[b][k], 90);
            pattern_add(&L->track[0].pat, b * bar + 144, 60, chords[b][k], 70);
        }
        for (uint32_t s = 0; s < 16; s++) {
            if ((s * 5u) % 16u < 5u) {
                pattern_add(&L->track[1].pat, b * bar + s * OLS_TICKS_PER_STEP, 20, roots[b], s ? 90 : 120);
            }
        }
        for (uint32_t beat = 0; beat < 4; beat++) {
            uint32_t at = b * bar + beat * OLS_PPQN;
            pattern_add(&L->track[3].pat, at, 12, 36, 120);                       /* kick */
            pattern_add(&L->track[3].pat, at + OLS_PPQN / 2u, 12, 42, 70);        /* closed hat */
            if (beat % 2u == 1u) {
                pattern_add(&L->track[3].pat, at, 12, 38, 100);                   /* snare */
            }
        }
    }
    pattern_add(&L->track[3].pat, 3 * bar + 3 * OLS_PPQN + 48, 12, 39, 110);     /* clap */
    for (uint32_t i = 0; i < 8; i++) {
        pattern_add(&L->track[2].pat, i * 48u, 40, motif[i], 100);
    }
    pattern_set_length(&L->track[2].pat, bar);       /* one bar against four: it shifts over the chords */
    creative_mutate(&L->track[2].pat, &L->rng, 0.4f);
    L->track[2].gain = 0.6f;
    L->track[3].gain = 0.8f;
    ols_live_play(L);
}

int render_to_wav(const char *path, float seconds, int with_demo)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "Cannot write %s\n", path);
        return 0;
    }

    desktop_init_synth((float)OFFLINE_RATE);
    if (with_demo) {
        build_demo_set();                               /* no audio thread yet: direct calls are fine */
    }

    uint32_t total = (uint32_t)(seconds * OFFLINE_RATE);
    write_wav_header(f, total);

    static float block[OFFLINE_BLOCK * 2];
    static int16_t pcm[OFFLINE_BLOCK * 2];
    int stopped = 0;
    int bad = 0;

    for (uint32_t done = 0; done < total; done += OFFLINE_BLOCK) {
        double t = (double)done / OFFLINE_RATE;
        if (with_demo && !stopped && t >= seconds - 1.0) {
            desktop_send(EV_LIVE, 0, LIVE_STOP, 0, 0, 0.0f);   /* last second: the tails */
            stopped = 1;
        }

        int n = (int)(total - done < OFFLINE_BLOCK ? total - done : OFFLINE_BLOCK);
        desktop_render(block, n);
        for (int i = 0; i < 2 * n; i++) {
            float s = block[i];
            if (!(s >= -1.0f && s <= 1.0f)) {           /* also catches NaN */
                bad++;
                s = 0.0f;
            }
            pcm[i] = (int16_t)(s * 32767.0f);
        }
        fwrite(pcm, sizeof(int16_t), (size_t)(2 * n), f);
    }
    fclose(f);

    printf("%s: %.1f s, track 1 %s, %d bad samples\n", path, seconds, ols_sound_name(app.preset[0]), bad);
    return bad == 0;
}
