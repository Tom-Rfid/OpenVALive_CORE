/*
 * OpenVALive desktop - audio output through miniaudio
 * (WASAPI on Windows, PulseAudio/ALSA on Linux, Core Audio on macOS).
 */
#include <stdio.h>
#include "desktop.h"

/* same configuration as miniaudio_impl.c */
#define MA_NO_ENCODING
#define MA_NO_DECODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#include "../../third_party/miniaudio/miniaudio.h"

static ma_device device;
static int device_started;

static void data_callback(ma_device *dev, void *output, const void *input, ma_uint32 frames)
{
    (void)input;
    double t0 = time_now();

    desktop_render((float *)output, (int)frames);

    /* CPU load = time spent / time available for these frames, smoothed */
    double budget = (double)frames / (double)dev->sampleRate;
    double load = (time_now() - t0) / budget;
    app.cpu_load = (float)(0.95 * app.cpu_load + 0.05 * load);
}

int audio_device_start(float requested_rate)
{
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = (ma_uint32)requested_rate;   /* 0 = the device's own rate */
    config.periodSizeInFrames = 256;                 /* ~5 ms: low latency */
    config.dataCallback = data_callback;

    if (ma_device_init(0, &config, &device) != MA_SUCCESS) {
        fprintf(stderr, "No audio output device could be opened.\n");
        return 0;
    }

    /* the synth must know the real rate before the first callback */
    desktop_init_synth((float)device.sampleRate);
    if (app.set_path && desktop_load_set(app.set_path)) {
        printf("Live set: %s\n", app.set_path);
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        fprintf(stderr, "The audio device did not start.\n");
        ma_device_uninit(&device);
        return 0;
    }
    device_started = 1;
    printf("Audio: %s, %u Hz\n", device.playback.name, device.sampleRate);
    return 1;
}

void audio_device_stop(void)
{
    if (device_started) {
        ma_device_uninit(&device);
        device_started = 0;
    }
}
