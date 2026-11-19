# Contributing

Thank you! The rules: `docs/_EN/18-rules-git-and-publishing.md` (FR: `docs/_FR/18-regles-git-et-publication.md`). Issues and pull requests in English or French are welcome.

## Most useful right now

1. **Test on your FM-1** and report the measurements (CPU on screen, clicks, pitch).
2. **Run the desktop app on Windows / macOS** with a MIDI keyboard and report.
3. **Run an untested connector** (Daisy Seed, Teensy) and fix what needs fixing.
4. **Try the automatic Windows install** (`INSTALL-WINDOWS.bat`) on a clean PC.
5. **SMK-37 hardware notes** (in [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37)).
6. **macOS MIDI** for the desktop app (CoreMIDI).

## Before coding

- Read `STATUS.md` (what's planned) and `docs/_EN/03-architecture.md`.
- For a new feature, open an issue first: the project deliberately refuses many things in V1
  (`docs/_EN/02-vision-and-roadmap.md`).
- Follow the `core/` rules summarised in `CLAUDE.md` (float, no libc, no malloc, nothing expensive at audio
  rate, small readable functions, English comments).

## Pull requests

- One topic per pull request.
- `make test` (and `FM1_BASE=… sh tests/run_tests.sh` if you touch the FM-1 or `core/`) must pass; CI runs both.
- Say **where** it is tested (PC, simulator, real FM-1) and paste measurements for DSP work (CPU, aliasing...).
- Update the related docs in **both** `docs/_EN` and `docs/_FR` (a short French note is fine if you don't speak
  French — say so and someone will translate), plus `STATUS.md` and `CHANGELOG.md`.
- No code copied from a GPL source (Felucca, Dexed...): see `docs/_EN/09-licences.md`.
- Building your own variant? See `docs/_EN/12-forking-guide.md`.
