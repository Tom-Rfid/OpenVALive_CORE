# 06 — API and connectors: using the DSP elsewhere

The OpenVALive core is a dependency-free C99 library: no hardware, no allocation, no C library required.
Three levels of use.

## Level 1: the C API (simplest)

```c
#include "core/api/ols_synth.h"

static int16_t delay_ram[16384];          /* 341 ms at 48 kHz, or 0 for no delay */
static ols_synth_t synth;

void setup(void)
{
    ols_synth_init(&synth, 48000.0f, delay_ram, 16384);
    ols_synth_load_preset(&synth, VA_PRESET_PAD);
}

void on_midi_bytes(const uint8_t *bytes, int n) { ols_synth_midi(&synth, bytes, n); }

void audio_callback(float *left, float *right, int frames)
{
    ols_synth_render_f32(&synth, left, right, frames);
}
```

Files to compile:
```
core/api/ols_synth.c
core/engines/va/engine_va*.c        (5 files)
core/live/midi_in.c
core/dsp/dsp_runtime.c              (run-time sample rate)
```
Include path: the repository root (includes are relative). Link `-lm` on a PC.

| Function | Role |
|---|---|
| `ols_synth_init(s, rate, delay, len)` | init; `rate` in Hz |
| `ols_synth_midi(s, bytes, n)` | raw MIDI bytes (running status handled, SysEx ignored) |
| `ols_synth_usb_midi(s, packet)` | 4-byte USB-MIDI packet |
| `ols_synth_note_on / note_off / all_notes_off` | direct notes |
| `ols_synth_set_param(s, id, v01)` | normalised parameter; `id` = `va_param_t` |
| `ols_synth_param_count / param_name` | to build a UI automatically |
| `ols_synth_load_preset(s, VA_PRESET_*)` | INIT, BASS, PAD, LEAD |
| `ols_synth_map_cc(s, cc, id)` | map a CC to a parameter |
| `ols_synth_render_f32(s, l, r, n)` | render, output in [−1, 1] |
| `ols_synth_render_i16(s, lr, n, scratch)` | interleaved int16 for I2S |

Complete, tested example: [`connectors/c-api/example.c`](../../connectors/c-api/example.c) (`make build/api_example`).

### Rules

- **Threads**: call everything from the audio thread, or protect the calls (Teensy: `AudioNoInterrupts()`), or go
  through `core/live/event_queue.h` (lock-free SPSC queue, as the FM-1 and desktop apps do).
- **Sample rate**: one per program, set by `ols_synth_init`. Several instances must share it.
- **Block size**: any. The engine works in sub-blocks of 32.
- **RAM**: `ols_synth_t` ≈ 3.4 KB, plus the delay buffer you give it.
- **CPU** (indicative): ~670 x86 instructions per sample for 4 voices + chorus + delay; ~5 % of a 360 MHz pi32v2
  per voice (measured on the FM-1).

## Level 2: the engine directly

`core/engines/va/engine_va.h` gives access to everything: `engine_va_init`, `note_on`, `set_param`, `render`,
`voice_limit`... The generic interface `core/engines/engine.h` (`ols_engine_api_t`) lets you write code that drives
any future engine (Acid, FM...) without knowing it.

## Level 3: the DSP blocks

Each block of `core/dsp/` is a standalone header of `static inline` functions:

```c
#include "core/dsp/osc.h"
#include "core/dsp/svf.h"

dsp_osc_t o;  osc_init(&o, 0.0f);  osc_set_freq(&o, 110.0f);
dsp_svf_t f;  svf_reset(&f);       svf_set(&f, 800.0f, 0.6f);   /* once per block */

for (int i = 0; i < n; i++) {
    osc_advance(&o);
    out[i] = svf_process(&f, osc_value(&o, OSC_SAW), SVF_LP);
}
```

## Connectors in the repository

| Connector | Status | Notes |
|---|---|---|
| `connectors/desktop` | tested (Linux CI, Windows .exe under Wine) | live play, MIDI + computer keyboard ([13](13-desktop-app.md)) |
| `connectors/c-api` | tested | writes a WAV at 44.1 kHz |
| `connectors/pc-sim` | tested | demo, aliasing sweep, CPU benchmark |
| [OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) · [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37) | the devices, in their own repositories | fm1-base · smk37_open |
| `connectors/daisy-seed` | **untested** | libDaisy, UART MIDI, delay in SDRAM |
| `connectors/teensy` | **untested** | `AudioStream` object, usbMIDI |

## Porting ideas

| Target | Notes |
|---|---|
| VST3 / CLAP / AU plugin | JUCE: `prepareToPlay` → `ols_synth_init(sampleRate)`; `processBlock` → the buffer's MIDI, then `render_f32` into channels 0 and 1. One sample rate per process is fine for a plugin. |
| RP2040 / RP2350 | I2S through PIO; the RP2350 has a single-precision FPU. On the RP2040 (no FPU) plan fewer voices. |
| ESP32-S3 | native I2S, single-precision FPU; render in a task pinned to one core. |
| Web Audio | compile to WebAssembly (emscripten) and render in an `AudioWorklet`. fm1-base already runs the whole FM-1 app in a browser ([14](https://github.com/Tom-Rfid/OpenVALive_FM-1/blob/dev/docs/_EN/14-web-simulator-and-site.md)). |

A working port deserves a pull request: a `connectors/<target>/` folder, a README with the build commands, and
"tested on ...".

## The Live Core and the drum engine (V0.2)

`core/api/ols_synth.h` stays the simplest way to get one synth. For the whole instrument (four tracks, loops,
CAPTURE, scenes, Creative Engine, drums), use `core/live/live.h` with `core/engines/bank.h`. The desktop app
(`connectors/desktop/engine_bridge.c`) is a complete, small example, and
[16 — The Live Core](16-the-live-core.md) shows the setup in a few lines. The drum engine alone
(`core/engines/drum/engine_drum.h`) is an `ols_engine_t` like the VA: any host can use it.
