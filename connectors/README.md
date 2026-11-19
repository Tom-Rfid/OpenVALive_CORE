# Connectors: using OpenVALive elsewhere

The core (`core/`) knows no hardware. A **connector** is the small piece of
code that plugs it into a platform: it provides audio, MIDI and maybe a UI.

| Connector | Platform | Status |
|---|---|---|
| [`desktop/`](desktop/) | Windows, Linux, macOS: play live with a MIDI or computer keyboard | **tested** (Linux build + render in CI, Windows .exe under Wine) |
| [`c-api/`](c-api/) | any C/C++ project: example that writes a WAV | **tested** (automatic tests) |
| [`pc-sim/`](pc-sim/) | PC simulator: demo, aliasing sweep, CPU benchmark | **tested** |
| [OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) · [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37) | the M-VAVE FM-1 and SMK-37, in their own repositories | FM-1 **tested on the device** |
| [`daisy-seed/`](daisy-seed/) | Electrosmith Daisy Seed (libDaisy) | **untested** template |
| [`teensy/`](teensy/) | Teensy 4.x (Teensy Audio Library) | **untested** template |

Full guide (files to compile, threads, sample rate, ideas for VST/CLAP,
RP2350, ESP32, Web Audio): [`docs/_EN/06-api-and-connectors.md`](../docs/_EN/06-api-and-connectors.md).

Got an untested template running? A pull request that fixes it and marks it
"tested on ..." is one of the most useful contributions there is.
