# OpenVALive desktop

Play the OpenVALive engine **on a computer**: Windows, Linux or macOS.
Same DSP, same presets, same controls as on the FM-1.

- sound on the default audio output (WASAPI, PulseAudio/ALSA, Core Audio, via [miniaudio](../../third_party/));
- notes from a **MIDI keyboard** (an SMK-37, an FM-1 over USB, any USB MIDI keyboard);
- or from the **computer keyboard**, like a tracker.

## Windows

Download `openvalive-desktop.exe` from `prebuilt/windows/` (or the latest
release) and double-click it. Nothing to install. A French keyboard is
detected automatically (AZERTY note keys).

## Linux / macOS

```bash
make desktop
build/openvalive-desktop
```
Linux MIDI uses the raw ALSA devices (`/dev/snd/midiC*D*`, your user must be
in the `audio` group). macOS has no MIDI input yet (CoreMIDI: contributions
welcome); the computer keyboard works.

## Keys

| Key | Action |
|---|---|
| `a w s e d f t g y h u j k o l p ;` | notes C4..E5 (`--azerty`: `q z s e d f t g y h u j k o l p m`) |
| Up / Down | octave |
| Left / Right | preset: INIT, BASS, PAD, LEAD |
| Tab | select CUTOFF / RESO / ENV / FX |
| `+` / `-` | change the selected control |
| Space | all notes off |
| `h` | help |
| Esc | quit |

A terminal reports key presses but not releases: a tapped key plays for
0.7 s, a held key keeps playing through the keyboard's auto-repeat. A MIDI
keyboard plays normally, with velocity, CC 74/71/73/72/5/1/93/94 and pitch bend.

## Options

```
--list              list MIDI inputs
--midi N|PATH       MIDI input to use (Windows: its number; Linux: /dev/snd/midiC1D0)
--azerty            French layout for the note keys
--preset NAME       INIT, BASS, PAD or LEAD
--rate HZ           ask the audio device for this sample rate
--render FILE.wav   render offline instead of playing (--seconds N, --demo)
```

## Files

| File | Role |
|---|---|
| `main.c` | options, main loop |
| `desktop.h` | shared state and declarations |
| `engine_bridge.c` | the only code that touches the synth (audio thread) |
| `audio_device.c`, `miniaudio_impl.c` | audio output |
| `midi_input_windows.c`, `midi_input_linux.c`, `midi_input_none.c` | MIDI input per OS |
| `byte_ring.h` | lock-free MIDI bytes queue (MIDI thread -> audio thread) |
| `keyboard_input.c`, `key_commands.c` | computer keyboard |
| `status_line.c` | the live status line |
| `render_offline.c` | WAV rendering (tests, CI) |
| `timing.c` | clock and sleep per OS |

Tested: Linux build and offline rendering (CI), Windows `.exe` built with
mingw-w64 and run under Wine (offline rendering). Live audio on a real
Windows PC: please report.
