# 01 — Overview

**OpenVALive CORE** is the heart of OpenVALive: a live-first synthesizer in portable C99, and the app that
plays it on small devices. It has no hardware code: the devices have their own repositories, which include this one
as a git submodule.

| Repository | What | Licence |
|---|---|---|
| **OpenVALive_CORE** (this one) | the DSP, the engines, the Live Core, the shared app, the desktop app | MIT |
| [OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) | the M-VAVE FM-1 firmware (on fm1-base) | MIT |
| [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37) | the M-VAVE SMK-37 Elite / Pro firmware (on M-VAVE's smk37_open) | GPL-3.0 |

## What is in it

| Part | Folder | In short |
|---|---|---|
| DSP blocks | `core/dsp/` | PolyBLEP oscillators, ZDF SVF filter, ADSR, LFO, delay, chorus, phaser |
| VA engine | `core/engines/va/` | 2 oscillators + sub + noise, multimode filter with drive, 2 envelopes, 2 LFOs (free or locked to the tempo, retrig, one cycle), **modulation matrix** (4 slots), delay, Juno-style chorus, phaser, bitcrusher, 17 presets |
| Drum engine | `core/engines/drum/` | synthesised 808 family: kick, snare, clap, hats, toms, rim, cowbell; 4 kits |
| Live Core | `core/live/` | clock (MIDI clock in / out), 4 tracks with loops, REC, CAPTURE, undo, scenes, Morph, perform effects, Creative Engine, macros, sidechain, scene chain, live sets |
| The app | `app/` | the instrument's UI for a 240 × 240 screen and the FM-1's panel (pages, layers, MAP CONTROL, SOUND EDIT), on fm1-base's `plat.h` API |
| Connectors | `connectors/` | desktop app (Windows / Linux), PC simulator, C API, Daisy / Teensy templates |

How it is wired: [03 — Architecture](03-architecture.md). The Live Core: [16](16-the-live-core.md). Writing an
engine: [07](07-writing-an-engine.md).

## Rules that make it fast

C99 and `float` only, no `malloc`, no C library on the device; expensive maths only at control rate (every 32
samples); every engine sleeps when silent and renders any block size identically; a refactor keeps the sound
bit-identical (the demo render is compared). Details: [CLAUDE.md](../../CLAUDE.md).
