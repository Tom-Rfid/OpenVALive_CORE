# 13 — The desktop app (Windows, Linux, macOS)

**`openvalive-desktop`** runs the same Live Core as the FM-1 on a computer: four tracks (synths and drums), loops,
CAPTURE, undo, scenes, Morph and the Creative Engine. Play it with a MIDI keyboard (an SMK-37, the FM-1 itself
over USB, any MIDI keyboard) or with the computer keyboard. It is also the quickest way to try a live set without
flashing anything.

## Windows: download and run

`prebuilt/windows/openvalive-desktop.exe` (or the release assets). Double-click `PLAY-OPENVALIVE.bat` or the
`.exe`. A console window opens, the sound comes out of the default output, and the first MIDI keyboard is used.
There is nothing to install. A French keyboard is detected and the note keys switch to AZERTY.

```
OpenVALive desktop
Audio: <your sound card>, 48000 Hz
MIDI: <your MIDI keyboard>

PLAY 120 | T4 DRUMS 808  step  5/16   4 notes | TUNE  50 | voices  3 | CPU  2% | MIDI: SMK-37 | euclid 4/16
```

## Keys

The commands avoid every note key of both layouts, so they are the same in QWERTY and AZERTY.

| Keys | Action |
|---|---|
| `a w s e d f t g y h u j k o l p ;` (AZERTY `q z s e d f t g y h u j k o l p m`) | notes, C4 to E5, on the selected track |
| `b` / `n` | previous / next track |
| `Space` | play / stop |
| `r` (or Enter) | record into the track |
| `c` | **CAPTURE** the phrase you just played |
| `x` / `X` | undo / redo |
| `v` | mute the track |
| `i` | quantize on / off |
| `!` | panic |
| Up / Down | octave |
| Left / Right | sound of the track: 17 synths, 4 drum kits |
| `Tab`, `+` / `-` | choose a control and change it: the track's four macros, then MORPH, SWING, TEMPO, LEVEL |
| `M H F S R E C P T` | Creative Engine: mutate, humanize, fill, simplify, reverse, expand, contract, chance, ratchet |
| `<` `>` / `U` `D` / `G` | rotate / transpose / Euclid (one more hit each press) |
| `W` / `Q` | store a scene (next slot) / recall the next one; MORPH glides between the last two |
| `L` / `A` | metronome (REC from stop then counts one bar in) / tap tempo |
| `K` | save the live set to `openvalive.set` (loaded again at the next start) |
| `h`, `Esc` | help, quit |

A MIDI keyboard plays the selected track, with velocity, and is recorded and captured. MIDI clock in works too.
A terminal reports key presses but not releases, so a tapped computer key plays for 0.7 s. A MIDI keyboard
gives real note lengths.

## Options

```
--list              list MIDI inputs
--midi N|PATH       MIDI input (Windows: its number; Linux: a /dev/snd/midi path)
--azerty            French layout for the note keys
--preset NAME       track 1's sound: ACID (303) BASS SUB LEAD PLUCK BRASS ORGAN PAD STRINGS SWEEP GRUNGE SCREECH DRONE WIND HOLD WHALE INIT
--set FILE          live set file (default openvalive.set): loaded at start, K saves it
--rate HZ           audio sample rate (default: the device's)
--render FILE.wav   render offline instead of playing (--seconds N, --demo: a 4-track set)
```

## Linux / macOS: build it

```bash
make desktop
build/openvalive-desktop --list
build/openvalive-desktop --midi /dev/snd/midiC1D0
build/openvalive-desktop --render demo.wav --seconds 9 --demo     # hear the Live Core without a sound card
```

| OS | Audio | MIDI |
|---|---|---|
| Windows | WASAPI (miniaudio) | WinMM, built in |
| Linux | PulseAudio / ALSA (miniaudio) | raw ALSA devices `/dev/snd/midiC*D*` (user in the `audio` group) |
| macOS | Core Audio (miniaudio) | not yet (CoreMIDI: contributions welcome); the computer keyboard works |

## How it is built

`connectors/desktop/` is a small, readable example of a real-time host for the Live Core:

```
main thread    keyboard, commands, status line ──► event queue ──┐
MIDI thread    WinMM callback / ALSA reader    ──► byte ring  ───┼──► audio thread (miniaudio callback)
                                                                  │    applies both, renders ols_live
```

- `engine_bridge.c` is the only code that changes the engines and the Live Core (audio thread). It also saves and
  loads live sets (`core/live/project.h`).
- `key_commands.c` turns keys into `EV_LIVE` commands. The FM-1 sends the same commands from its buttons.
- `render_offline.c` uses the same path without a device. The tests run it, and `--demo` builds a set with the
  Live Core's own functions.

Windows build from Linux: `make windows` (mingw-w64) makes one static `.exe`.

## Tested

The Linux build and the offline render run in CI. The Windows `.exe` is cross-built with mingw-w64 and run under
Wine (offline render, MIDI listing). Live audio on a real Windows PC and with a real MIDI keyboard has not been
tested yet: please report what you find.
