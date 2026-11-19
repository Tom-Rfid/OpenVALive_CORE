# OpenVALive VirtualAnalog

**The heart of OpenVALive: a live-first synthesizer in portable C99**, with a virtual analog engine, a drum engine,
a modulation matrix, LFOs locked to the tempo, and a **Live Core** for loops, CAPTURE, scenes and Morph. No
`malloc`, no C library on the device: it runs on microcontrollers (the M-VAVE FM-1 and SMK-37), on a desktop and in
a browser.

| Repository | What | Licence |
|---|---|---|
| **OpenVALive_CORE** (this one) | DSP, engines, Live Core, the shared app, the desktop app | MIT |
| [OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) | the FM-1 firmware (on fm1-base) | MIT |
| [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37) | the SMK-37 Elite / Pro firmware (on M-VAVE's smk37_open) | GPL-3.0 |

The device repositories include this one as a git submodule (`va/`), pinned to a commit or a release tag.

## Quick start

```bash
make            # build/ols_sim (engine simulator)
make demo       # build/demo.wav
make desktop    # build/openvalive-desktop: play it with the computer keyboard and a MIDI keyboard
make test       # tests (numpy for the audio measurements)
FM1_BASE=~/fm1-base sh tests/run_tests.sh   # + the device checks (no C library, fastmath)
```

## The map

```
core/dsp/       oscillators, filter, envelopes, LFO, delay, chorus, phaser (static inline headers)
core/engines/   the engine interface (roles, macros, clock, motion), VA, DRUM, the sound bank
core/live/      the Live Core: clock, tracks, record, capture, undo, scenes, Morph, Creative Engine, projects, MIDI
core/api/       ols_synth.h: the simple C API
app/            the instrument's UI and audio side, shared by the devices (app/README.md)
connectors/     desktop app, PC simulator, C API example, Daisy & Teensy templates
tests/          automatic tests (Live Core, pitch, aliasing, no-libc build, fastmath)
docs/           _EN (English) and _FR (français)
```

| Doc | EN | FR |
|---|---|---|
| Overview | [01](docs/_EN/01-overview.md) | [01](docs/_FR/01-presentation.md) |
| Strategy · vision | [00](docs/_EN/00-strategy.md) · [02](docs/_EN/02-vision-and-roadmap.md) | [00](docs/_FR/00-strategie.md) · [02](docs/_FR/02-vision-et-feuille-de-route.md) |
| Architecture | [03](docs/_EN/03-architecture.md) | [03](docs/_FR/03-architecture-technique.md) |
| API and connectors | [06](docs/_EN/06-api-and-connectors.md) | [06](docs/_FR/06-api-et-connecteurs.md) |
| Writing an engine | [07](docs/_EN/07-writing-an-engine.md) | [07](docs/_FR/07-creer-un-moteur.md) |
| Tests and measurements | [08](docs/_EN/08-tests-and-measurements.md) | [08](docs/_FR/08-tests-et-mesures.md) |
| Licences | [09](docs/_EN/09-licences.md) | [09](docs/_FR/09-licences.md) |
| Forking guide | [12](docs/_EN/12-forking-guide.md) | [12](docs/_FR/12-guide-des-forks.md) |
| Desktop app | [13](docs/_EN/13-desktop-app.md) | [13](docs/_FR/13-application-bureau.md) |
| The Live Core | [16](docs/_EN/16-the-live-core.md) | [16](docs/_FR/16-le-live-core.md) |
| Rules: code, git, publishing | [18](docs/_EN/18-rules-git-and-publishing.md) | [18](docs/_FR/18-regles-git-et-publication.md) |

State and decisions: [STATUS.md](STATUS.md) · rules: [CLAUDE.md](CLAUDE.md) · [CHANGELOG.md](CHANGELOG.md) ·
[CONTRIBUTING.md](CONTRIBUTING.md). Licence: MIT ([LICENSE](LICENSE)).
