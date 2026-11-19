/*
 * OpenVALive desktop - entry point and main loop.
 *
 *   openvalive-desktop                     play live (default audio output, first MIDI input)
 *   openvalive-desktop --list              list the MIDI inputs
 *   openvalive-desktop --midi N|PATH       choose the MIDI input (Windows: number, Linux: /dev/snd/midiC1D0)
 *   openvalive-desktop --azerty            French keyboard layout for the note keys
 *   openvalive-desktop --preset pad        track 1 starts with INIT, BASS, PAD or LEAD
 *   openvalive-desktop --set FILE          the live set file (default openvalive.set): loaded at start, K saves
 *   openvalive-desktop --rate 48000        ask the audio device for this sample rate
 *   openvalive-desktop --render out.wav [--seconds 8] [--demo]   render offline, no audio device
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "desktop.h"

static volatile int quit_requested;

static void on_signal(int sig)
{
    (void)sig;
    quit_requested = 1;
}

static int preset_from_name(const char *name)
{
    for (int i = 0; i < OLS_SOUND_COUNT; i++) {
        const char *p = ols_sound_name(i);
        int j = 0;
        while (p[j] && name[j] && (p[j] | 0x20) == (name[j] | 0x20)) {
            j++;
        }
        if ((p[j] == 0 || p[j] == ' ') && name[j] == 0) {
            return i;                       /* (the first word is enough: ACID for "ACID 303") */
        }
    }
    return -1;
}

static void usage(void)
{
    printf("OpenVALive desktop - play the OpenVALive engine on a computer\n\n");
    printf("  --list              list MIDI inputs\n");
    printf("  --midi N|PATH       MIDI input to use (default: the first one)\n");
    printf("  --azerty            French keyboard layout for the note keys\n");
    printf("  --preset NAME       track 1: ACID BASS SUB LEAD PLUCK BRASS ORGAN PAD STRINGS SWEEP GRUNGE...\n");
    printf("  --set FILE          live set file (default openvalive.set): loaded at start, K saves it\n");
    printf("  --rate HZ           audio sample rate (default: the device's)\n");
    printf("  --render FILE.wav   render offline instead of playing (with --seconds N, --demo)\n");
}

static int live_loop(void)
{
    keyboard_begin();      /* may switch to AZERTY on a French Windows keyboard */
    print_help();
    double next_status = 0.0;

    while (!quit_requested) {
        int key = keyboard_poll();
        if (key != KEY_NONE && !handle_key(key)) {
            break;
        }
        double now = time_now();
        update_note_timers(now);
        if (now >= next_status) {
            status_line_print();
            next_status = now + 0.1;
        }
        sleep_ms(4);
    }

    keyboard_end();
    printf("\n");
    return 0;
}

int main(int argc, char **argv)
{
    const char *midi = 0;
    const char *render_path = 0;
    float seconds = 8.0f;
    float rate = 0.0f;
    int demo = 0;

    app.start_preset = -1;
    app.set_path = "openvalive.set";

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *next = i + 1 < argc ? argv[i + 1] : 0;

        if (!strcmp(a, "--list")) {
            midi_input_list();
            return 0;
        } else if (!strcmp(a, "--midi") && next) {
            midi = next; i++;
        } else if (!strcmp(a, "--azerty")) {
            app.azerty = 1;
        } else if (!strcmp(a, "--preset") && next) {
            int p = preset_from_name(next); i++;
            if (p < 0) { fprintf(stderr, "Unknown preset %s\n", next); return 2; }
            app.start_preset = p;
        } else if (!strcmp(a, "--set") && next) {
            app.set_path = next; i++;
        } else if (!strcmp(a, "--rate") && next) {
            rate = (float)atof(next); i++;
        } else if (!strcmp(a, "--render") && next) {
            render_path = next; i++;
        } else if (!strcmp(a, "--seconds") && next) {
            seconds = (float)atof(next); i++;
        } else if (!strcmp(a, "--demo")) {
            demo = 1;
        } else {
            usage();
            return !strcmp(a, "--help") || !strcmp(a, "-h") ? 0 : 2;
        }
    }

    if (render_path) {
        return render_to_wav(render_path, seconds, demo) ? 0 : 1;
    }

    signal(SIGINT, on_signal);
    printf("OpenVALive desktop\n");
    if (!audio_device_start(rate)) {
        return 1;
    }
    if (midi_input_open(midi)) {
        printf("MIDI: %s\n", app.midi_name);
    } else {
        printf("MIDI: no input (computer keyboard only)\n");
    }

    int rc = live_loop();

    midi_input_close();
    audio_device_stop();
    return rc;
}
