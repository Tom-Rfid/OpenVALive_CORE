# STATUS — OpenVALive CORE

The core of OpenVALive, split out of OpenVALive on 17 November 2026 (history kept). The device firmwares are in
[OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) and [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37),
which include this repository as a submodule.

## What works

| Item | Where | Verified |
|---|---|---|
| DSP: PolyBLEP oscillators, ZDF SVF, ADSR, LFO, delay, chorus, phaser, 3-band master EQ | `core/dsp` | tests: pitch, aliasing, no NaN |
| VA engine: 4 voices, poly / mono legato, 61 parameters, 43 presets (every one plays and ends: `tests/presets_test.c`), 5 ms smoothing, sample-accurate split render, sleep when silent | `core/engines/va` | tests; V0.1 subset measured on a real FM-1 |
| **VA modulation**: 4-slot matrix (9 sources, 11 destinations) at control rate, LFOs free or locked to the tempo (4 bars .. 1/32), RETRIG / ONE | `core/engines/va/engine_va_mod.c` | 170 Live Core checks (bit-identical without modulation, matrix, LFO timing, clock, projects) |
| DRUM engine (808 family, synthesised), 5 kits, bells and triangles (agogo, mute) | `core/engines/drum` | tests (range, length, brightness) |
| Sound bank: 48 factory sounds (9 families by character with USER) + 8 user sounds | `core/engines/bank.*` | tests |
| Live Core: clock (internal / MIDI in / out, swing), 4 tracks, REC, CAPTURE, undo ×8, scenes ×8, Morph, overrides, stutter, Creative Engine, macros ENERGY SPACE TENSION DENSITY, PUMP sidechain, scene chain, live sets with CRC | `core/live` | `tests/live_test.c` |
| MIDI parser (DIN, USB-MIDI), CC mapping, real-time, program change | `core/live/midi_in.*` | tests + FM-1 (V0.1) |
| The app: pages SOUND / MACRO / LIVE / SETUP / USER / EQ / DRUM EQ (order and knobs set in MAP CONTROL), OPTION CONTROL, SOUNDS, A VOL, layers, perform effects, SOUND EDIT, user sounds, safe start | `app/` | in the device repositories: FM-1 simulator (181 checks), SMK-37 host test (65) |
| Desktop app (Windows / Linux) | `connectors/desktop` | Linux build + offline render; Windows .exe under Wine |

### Measured

- Real FM-1, V0.1: ~5 % CPU per voice, 26 % worst case (PAD, 4 notes).
- The V0.2+ Live Core and the modulation are measured in the device repositories' simulators; not yet on hardware.

## Decisions

- Licence **MIT**; nothing GPL here (the SMK-37 repository is the GPL one).
- Engines are synthesised only (no samples). Performance gestures are named by **role** so they work on every engine.
- Parameters are stored by number: new ones at the end (`OLS_MAX_PARAMS` 61; a full live set fits two FM-1 objects).
- Branches: `dev` by default, `main` for releases.

## Next steps

1. The device repositories move their submodule to this version and run their tests (FM-1 simulator, SMK-37 host).
2. Hardware results (FM-1, SMK-37 Elite) tune the voice limits, levels and presets.
