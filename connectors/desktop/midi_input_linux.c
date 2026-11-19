/*
 * OpenVALive desktop - MIDI input on Linux, through ALSA raw MIDI devices
 * (/dev/snd/midiC<card>D<device>). No library needed: a thread reads bytes
 * from the device and pushes them into app.midi_bytes.
 */
#ifdef __linux__
#include <stdio.h>
#include <string.h>
#include <glob.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "desktop.h"

static int fd = -1;
static pthread_t reader;
static volatile int running;
static char device_path[256];

static void *read_loop(void *arg)
{
    (void)arg;
    uint8_t buf[64];
    while (running) {
        ssize_t n = read(fd, buf, sizeof buf);
        if (n > 0) {
            for (ssize_t i = 0; i < n; i++) {
                byte_ring_push(&app.midi_bytes, buf[i]);
            }
        } else {
            sleep_ms(1);
        }
    }
    return 0;
}

void midi_input_list(void)
{
    glob_t g;
    if (glob("/dev/snd/midiC*D*", 0, 0, &g) != 0) {
        printf("MIDI inputs: none found in /dev/snd\n");
        return;
    }
    printf("MIDI inputs (use --midi PATH):\n");
    for (size_t i = 0; i < g.gl_pathc; i++) {
        printf("  %s\n", g.gl_pathv[i]);
    }
    globfree(&g);
}

int midi_input_open(const char *which)
{
    if (which) {
        snprintf(device_path, sizeof device_path, "%s", which);
    } else {
        glob_t g;
        if (glob("/dev/snd/midiC*D*", 0, 0, &g) != 0 || g.gl_pathc == 0) {
            return 0;
        }
        snprintf(device_path, sizeof device_path, "%s", g.gl_pathv[0]);
        globfree(&g);
    }

    fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s (busy, or not in the 'audio' group?).\n", device_path);
        return 0;
    }
    running = 1;
    if (pthread_create(&reader, 0, read_loop, 0) != 0) {
        close(fd);
        fd = -1;
        return 0;
    }
    app.midi_name = device_path;
    return 1;
}

void midi_input_close(void)
{
    if (fd >= 0) {
        running = 0;
        pthread_join(reader, 0);
        close(fd);
        fd = -1;
    }
}
#endif /* __linux__ */
