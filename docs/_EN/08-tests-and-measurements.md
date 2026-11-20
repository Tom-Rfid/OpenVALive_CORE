# 08 — Tests and measurements

## Running the tests

```bash
make test                                   # PC tests
FM1_BASE=~/fm1-base sh tests/run_tests.sh   # + FM-1 tests
```

Needs a C compiler, `make`, Python 3 with **numpy** (otherwise the audio measurements are skipped) and fm1-base
for the FM-1 tests. CI runs all of them on every push and pull request.

## What is checked

| Test | Checks | Threshold |
|---|---|---|
| strict build | the core builds with `-Wall -Wextra -Wdouble-promotion -Werror` | 0 warnings |
| simulator | 33 s demo (mono bass + 4-voice pad, through real MIDI bytes) | 0 NaN, 0 clipped samples |
| C API | the example builds and runs at 44.1 kHz | — |
| desktop app | builds and renders a WAV offline | 0 bad samples |
| pitch | A440 rendered at 44.1 / 48 / 96 kHz, measured by FFT | ±2 cents (measured: < 0.1) |
| aliasing | saw at 1760 Hz, energy outside the harmonics | < −25 dB (measured: −36) |
| FM-1 without libc | `app/ols_dsp.c` built with `-ffreestanding -nostdinc` | like the JieLi compiler |
| fastmath = libm | demo computed with the FM-1's math and the PC's | > 40 dB difference (measured: 87) |
| FM-1 app | the whole app in the fm1-base simulator: boot, chord, presets, MIDI, CC, save, reboot | 13 checks |

## The device tests

The whole app runs in the fm1-base simulator (screen, keys, MIDI, storage) in
[OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) (`targets/fm1/tests/basic.txt`), and on the SMK-37's own
host test in [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37). Both run in their CI against the
version of this repository they pin.

## The engine simulator

```bash
make demo                    # build/demo.wav
build/ols_sim sweep x.wav    # 50 Hz -> 15 kHz sweep: listen to the aliasing
make bench                   # CPU cost per case, static RAM
```

## The sounds' levels

`python3 tools/level_sounds.py` sets every synth sound's VOLUME so they all sound about as loud; `--check` only
measures. Each sound plays the same phrase (C3 G3 C4 G4, 1 s each, velocity 100); its loudness is a K-weighted level
(the ear's curve of ITU-R BS.1770) in 100 ms windows, each note counted by its loudest window (a pluck by its strike,
a pad once swelled), the four notes power-averaged. The target is the sounds' median; a sound goes no louder than
3 dB under full scale at its peak (the output is a tanh). Three rounds, as the tanh bends the loud ones. Run it after
adding or changing a sound. Measured 20 Nov 2026: 50 of the 59 sounds within 0.1 dB of the target (-17.6 dB); the
others (CLICK, BLEEP, DUB CHORD, DROPS, NOISE, WIND, RUBBER, VINTAGE KEYS, RISER) are held by their peaks or their
VOLUME at the top.

## Measurements on the real FM-1 (V0.1, 2026-11-17)

| Preset | Idle | 4 keys | Note |
|---|---|---|---|
| INIT | 4 % | 24 % | 4 voices |
| PAD | 6 % | 26 % | + chorus |
| LEAD | 4 % | 9 % | mono: 1 voice |
| BASS | 4 % | 10 % | mono + sub + drive |

About 4 % fixed cost (delay, chorus, UI), about 5 % per voice.

Real build (JieLi toolchain): image 100 KB, `.data+.bss` 36 / 96 KB, pool 100 / 336 KB, RAM code 5 / 32 KB,
no `double`.

## To add

- A click test: fast parameter changes, detect discontinuities (with the V0.2 parameter system).
- Filter frequency response and stability at maximum resonance.
- Simulator vs device: record the FM-1 through USB audio and compare with the simulator's WAV.

## V0.2 additions

| Test | What it proves |
|---|---|
| `live core: 170 checks` (`tests/live_test.c`) | sequenced notes start on their exact sample; ratchet, chance, mute, swing; REC with quantize (no double note); CAPTURE stopped and running; undo depth; every Creative Engine gesture keeps a valid pattern, and the identities hold; scenes, Morph, overrides by role; project save → load → save gives identical bytes and a damaged file is refused; MIDI clock in (within a tick of the clock, tempo measured); stutter; the VA render cut anywhere is bit-identical; every drum sounds in range, ends by itself, kick low, hat bright, closed hat chokes open; engine switching and projects with a drum track |
| `FM-1: app in the fm1-base simulator (80 checks)` | the V0.2 FM-1 app: tracks, MIDI to the selected track, CAPTURE, undo/redo, creative, REC, scenes, Euclid, mutes, drums, controller mode, sound changes, tempo, save and reboot |
