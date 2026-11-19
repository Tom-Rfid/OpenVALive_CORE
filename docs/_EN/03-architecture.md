# 03 — Architecture

## Layers

```
              ┌────────────────────────────────────────────────────────────────┐
 targets &    │ targets/fm1 (fm1-base app)  connectors/desktop  pc-sim  daisy… │  hardware, UI
 connectors   └───────────────┬────────────────────────────────────────────────┘
                              │  events (SPSC queue)      audio rendered by blocks
              ┌───────────────▼────────────────────────────────────────────────┐
 core/live    │ midi_in (parser)    event_queue (UI -> audio, timestamped)     │
              ├────────────────────────────────────────────────────────────────┤
 core/engines │ engine.h: common interface (notes, params, render)             │
              │ va/: the VA engine (api, params, presets, voices, render)      │
              ├────────────────────────────────────────────────────────────────┤
 core/dsp     │ osc · svf · adsr · lfo · delay · chorus · dsp_common           │
              ├────────────────────────────────────────────────────────────────┤
 core/api     │ ols_synth.h: the simple C API for other projects               │
              └────────────────────────────────────────────────────────────────┘
```

Rules:
- `core/` knows **no hardware**, **never** allocates memory and only uses `stdint.h` (+ `math.h` on a PC).
  It builds without a C library.
- A target talks to an engine only through `engine.h` or the C API.
- The user interface **never** touches the engine directly: it pushes events.

## Source map

| File | Role |
|---|---|
| `core/dsp/dsp_common.h` | math backend (libm or fastmath), sample-rate modes, fast tanh / exp2 / mtof, noise |
| `core/dsp/osc.h` | PolyBLEP saw, PWM square, triangle |
| `core/dsp/svf.h` | zero-delay-feedback state variable filter |
| `core/dsp/adsr.h` · `lfo.h` | envelopes, LFOs |
| `core/dsp/delay.h` · `chorus.h` | effects |
| `core/engines/va/engine_va.h` | the VA engine's public interface |
| `engine_va.c` | API table, init, accessors |
| `engine_va_params.c` | 0..1 → Hz / seconds / semitones conversions |
| `engine_va_presets.c` | INIT, BASS, PAD, LEAD |
| `engine_va_voices.c` | note on/off, voice allocation, mono note stack |
| `engine_va_render.c` | control-rate and audio-rate rendering, effects |
| `core/live/midi_in.*` | MIDI parser (running status, USB-MIDI packets, CC mapping) |
| `core/live/event_queue.h` | lock-free SPSC queue of timestamped events |
| `core/api/ols_synth.*` | simple C API |

## The real-time model

On the FM-1, audio is rendered in the **interrupt** of the audio DMA (I2S ALNK0), 256 frames at a time
(5.8 ms). The UI runs in the main loop (~60 frames/s) and can be interrupted at any time.

```
main loop (UI)                          audio interrupt (every 5.8 ms)
  keys, encoders, presets                 1. drain the event queue
        │                                 2. read incoming MIDI (USB, TRS)
        └──► event_queue (SPSC) ────────► 3. render 256 frames (in sub-blocks of 32)
                                          4. write the DMA buffer, feed the oscilloscope
```

- **One queue, one producer, one consumer, no lock** (`core/live/event_queue.h`). Data is written before the
  index (compiler barrier): safe on one core. The desktop app uses a hardware barrier (`__sync_synchronize`);
  the pi32v2 second core would need `csync`.
- Every event carries a **sample timestamp** (`at`). Today it is applied at the start of the block; in V0.2, at
  its exact sample.
- MIDI is read **inside** the render: same context as the engine, no concurrent access.
- Under overload (> 90 % of the render time), fm1-base calls `app_overload()` and the app drops one voice
  rather than letting the sound crackle.

The desktop app follows the same model with threads: main thread → event queue, MIDI thread → byte ring,
audio thread renders.

## Two computation rates

| Rate | What | Example |
|---|---|---|
| audio (44.1–96 kHz) | oscillators, filter, envelopes, effects | `osc_value`, `svf_process`, `adsr_tick` |
| control (every 32 samples) | LFOs, pitch, glide, cutoff, filter coefficients | `svf_set` (uses tan), `osc_set_freq` |
| parameter (when a knob moves) | 0..1 → Hz, seconds, semitones | `engine_va_set_param` |

Expensive functions (`tan`, `exp`, `pow`) are never called at audio rate.

## Algorithms

| Block | Algorithm | Cost | Reference |
|---|---|---|---|
| Saw / square (PWM) | 2-point PolyBLEP | ~10 flops, 12 B | Välimäki & Huovilainen 2007 |
| Triangle | naive (harmonics fall as 1/n²) | ~3 flops | — |
| Sub | PolyBLEP square at f/2 | same | — |
| Filter | trapezoidal ZDF SVF, LP/BP/HP/Notch | ~12 flops, 28 B | A. Simper (Cytomic); V. Zavalishin, *The Art of VA Filter Design* |
| Envelopes | `y += c·(target − y)`, attack aiming at 1.3 | ~3 flops | classic (Mutable Instruments, Teensy Audio) |
| Saturation | Padé 3/2 rational tanh | ~6 flops | — |
| LFO | parabolic sine, triangle, square, S&H | control rate | — |
| Delay | **int16** circular buffer, low-pass in the feedback | 2 bytes/sample | — |
| Chorus | 2 lines modulated in opposite phase, Juno-60 values | 2 KB | Roland Juno-60 |

Measured by the tests: aliasing −36 dB at 1760 Hz (15–20 dB better than a naive saw), pitch error below 0.1 cent
at 44.1 / 48 / 96 kHz.

## The VA engine

```
OSC1 ─┐
OSC2 ─┤                                                   per voice (x4)
SUB  ─┼─► MIX ─► SVF ─► DRIVE ─► VCA (ADSR)
NOISE─┘           ▲
       filter env + LFO2 + key tracking + glide

Σ voices ─► DELAY (mono, int16) ─► CHORUS (stereo) ─► soft clip ─► L/R
```

- 40 **normalised 0..1** parameters (`va_param_t`): an encoder, a MIDI CC, a preset and future scenes all write the
  same value; conversions happen once, in `set_param`, into `e->d` (derived values).
- Poly 4 voices (steal: same note > free voice > oldest released > oldest) or mono legato (8-note stack, glide).
- `voice_limit`: dynamic load shedding.

## Sample rate

Two modes (`core/dsp/dsp_common.h`):
- **fixed** at compile time with `DSP_SR_F` (FM-1: `24 MHz / 544 = 44117.647 Hz`, the real ALNK0 clock,
  not 44100) → constants;
- **run time** with `dsp_set_sample_rate()` before the init (plugins, desktop, Daisy...) → compile
  `core/dsp/dsp_runtime.c`. One rate per program.

## No libm on the device

The FM-1 firmware has no libm and its build rejects any `double`. With `OLS_FASTMATH`, the core uses fm1-base's
`fastmath.h` (`fm_tanf`, `fm_expf`...). A test renders the demo both ways: the difference is more than 80 dB
below the signal.

## The FM-1 in numbers

| | Value | Source |
|---|---|---|
| CPU | pi32v2, 360 MHz, single-precision FPU | fm1-base `plat_cpu_khz` |
| Audio | I2S, stereo, **Q23 in int32**, 44117.647 Hz | felucca-bsp `docs/BOARD_IO.md` |
| Audio block | 256 frames (5.8 ms), double buffer | fm1-base `APP_BLOCK` |
| App flash | 581 KB | fm1-base README |
| RAM `.data/.bss` | 96 KB | same |
| Pool (big buffers) | 336 KB | same |
| MIDI | in: USB + TRS; out: USB (+ BLE); **no OUT jack** | fm1-base `plat.h` |

OpenVALive today (real build): flash 100 KB, RAM 36 KB, pool 100 KB (32 KB delay + screen), RAM code 5 of
32 KB. CPU on the device: 4 % idle, ~5 % per voice, 26 % worst case.

## The app (`app/`, shared by every device)

| File | Role |
|---|---|
| `openvalive_app.h` | what every target shares: storage objects, entry points (a target's `app.h` includes it) |
| `openvalive_app.c` | entry points (`app_init`, `app_frame`...), includes `src/` |
| `src/app_state.c` | shared state and ownership rules |
| `src/app_audio.c` | audio interrupt: events, MIDI in, render, Q23 output, scope |
| `src/app_events.c` | UI → audio events, USB-MIDI out |
| `src/app_settings.c` | load / save in flash |
| `src/ui_text.c` · `ui_input.c` · `ui_draw.c` | text helpers, controls, screen |
| `src/control_map.c` · `src/sound_edit.c` | MAP CONTROL, OPTION CONTROL and SOUND EDIT |
| `ols_dsp.c` · `ols_config.h` | the core compiled at -O2, fastmath, fixed sample rate |

The device repositories add the rest: [OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) (`targets/fm1`:
`app.c` / `app.h`, `fm1.toml`, site, manual, simulator scripts, on fm1-base) and
[OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37) (its own platform on M-VAVE's SDK). Each includes
this repository as a git submodule (`va/`). The app is compiled as one unit: `openvalive_app.c` includes the `src/`
files in order, so `static` state is shared.

## What V0.2 changes

1. **Parameter system**: each parameter gets `value`, `target`, smoothing and a type (linear, exponential,
   discrete) that also says how to interpolate it for Morph and scenes.
2. **Central clock**: one tempo in ticks (24 PPQN for MIDI clock, finer inside), follows an external clock,
   sends MIDI clock over USB.
3. **Sample-accurate events**: the render splits the block at event timestamps.

## V0.2: the Live Core and several engines

```
UI (main loop) ──EV_LIVE / notes / params──► event queue ──► audio side
                                                            ols_live_render
                                                            ├── clock: cut the block at every tick
                                                            ├── sequencer: 4 tracks, start/stop notes
                                                            ├── track 1..4 engine (VA or DRUM, via bank.h)
                                                            └── mix + soft limit
```

- `core/live/` holds the clock, tracks, record/capture, undo, scenes/Morph, the Creative Engine and the project
  format. Details: [16 — The Live Core](16-the-live-core.md).
- Engines expose **roles** (which parameter is the cutoff, the drive...) and **macros** (the four knob
  parameters), so the Live Core and the UIs never need to know which engine a track uses.
- `core/engines/bank.*` lists the sounds and switches a track's engine. A track owns one VA and one drum engine;
  only the active one renders, and a silent engine sleeps.
