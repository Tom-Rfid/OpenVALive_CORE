/*
 * OpenVALive on the Electrosmith Daisy Seed (libDaisy)  --  UNTESTED TEMPLATE
 *
 * Shows how to plug the C API in: audio in the callback, MIDI (UART, TRS/DIN
 * jack) read in the main loop and handed to the callback through a small
 * lock-free queue. Build with the usual libDaisy Makefile, adding to C_SOURCES:
 *   core/api/ols_synth.c core/engines/va/engine_va*.c core/live/midi_in.c core/dsp/dsp_runtime.c
 * Only reviewed, never compiled: corrections welcome.
 */
#include "daisy_seed.h"
extern "C" {
#include "core/api/ols_synth.h"
}
using namespace daisy;

static DaisySeed hw;
static MidiUartHandler midi;
static ols_synth_t synth;
static int16_t DSY_SDRAM_BSS delay_ram[48000];        /* 1 s of delay in SDRAM */

/* MIDI byte queue: main loop -> audio callback */
static volatile uint32_t q_head, q_tail;
static uint8_t q_buf[256];
static void q_push(uint8_t b)
{
    if (q_head - q_tail < sizeof q_buf) { q_buf[q_head & 255u] = b; __DMB(); q_head = q_head + 1; }
}

static float buf_l[256], buf_r[256];

static void AudioCallback(AudioHandle::InputBuffer, AudioHandle::OutputBuffer out, size_t size)
{
    while (q_tail != q_head) {                           /* MIDI received since the last block */
        uint8_t b = q_buf[q_tail & 255u];
        __DMB();
        q_tail = q_tail + 1;
        ols_synth_midi(&synth, &b, 1);
    }
    ols_synth_render_f32(&synth, buf_l, buf_r, (int)size);
    for (size_t i = 0; i < size; i++) { out[0][i] = buf_l[i]; out[1][i] = buf_r[i]; }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(48);                            /* 1 ms at 48 kHz */
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    ols_synth_init(&synth, hw.AudioSampleRate(), delay_ram, sizeof delay_ram / sizeof delay_ram[0]);
    ols_synth_load_preset(&synth, VA_PRESET_PAD);

    MidiUartHandler::Config cfg;
    midi.Init(cfg);
    midi.StartReceive();
    hw.StartAudio(AudioCallback);

    for (;;) {
        midi.Listen();
        while (midi.HasEvents()) {
            MidiEvent e = midi.PopEvent();
            uint8_t status = 0;
            switch (e.type) {
            case NoteOn:        status = 0x90; break;
            case NoteOff:       status = 0x80; break;
            case ControlChange: status = 0xB0; break;
            case PitchBend:     status = 0xE0; break;
            default: break;
            }
            if (!status) continue;
            q_push((uint8_t)(status | e.channel));
            q_push(e.data[0]);
            q_push(e.data[1]);
        }
    }
}
