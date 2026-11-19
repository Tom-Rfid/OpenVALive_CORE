# CLAUDE.md — guide for agents (and busy humans)

OpenVALive CORE: the portable C99 heart of OpenVALive (DSP, engines, Live Core, the shared app, the desktop
app). The devices live in their own repositories, which include this one as a submodule `va/`:
OpenVALive_FM-1 (MIT) and OpenVALive_SMK-37 (GPL-3.0). A change here is tested in both before their submodule moves. Code comments
in **English**; docs in `docs/_EN` (English) and `docs/_FR` (French) — keep both in sync. Read `STATUS.md` first
(state, decisions, next steps), then `docs/_EN/03-architecture.md`.

## Commands

```bash
make                 # build/ols_sim (engine simulator)
make demo            # build/demo.wav
make bench           # CPU / RAM on this PC
make desktop         # build/openvalive-desktop (live play)
make windows         # build/openvalive-desktop.exe (mingw-w64)
make test            # PC tests (numpy for audio measurements)
FM1_BASE=~/fm1-base sh tests/run_tests.sh             # + device checks (no-libc build, fastmath)
```

Before any commit: `FM1_BASE=... sh tests/run_tests.sh` must end with "All tests pass.". A change to `app/` is also
run in OpenVALive_FM-1's simulator and OpenVALive_SMK-37's host test (their CI, after moving their submodule).

## Repository map

- `core/dsp/` DSP blocks as `static inline` headers; `dsp_common.h` picks libm or fastmath and the sample-rate mode.
- `core/engines/engine.h` common engine interface (roles, macros, clock, motion); `core/engines/va/` the VA engine
  (`engine_va_mod.c`: matrix + LFO timing), `core/engines/drum/` the drum engine; `core/engines/bank.*` the sounds.
- `core/live/` the Live Core: clock, tracks/sequencer (`live.c`), record + capture, undo + creative, scenes +
  Morph + overrides, commands, projects; MIDI (`midi_in`), event queue. `core/api/` the simple C API (one synth).
- `app/` the app shared by the devices (`openvalive_app.c` includes `src/*.c`, unity build); a target supplies
  `app.h`, a one-line `app.c` and its platform (`app/README.md`). Device code never goes here: only hooks.
- `connectors/` desktop app, PC simulator, C API example, Daisy/Teensy templates (untested).
- `third_party/` miniaudio.

## Branches

- **`dev`** is the working branch (the default): feature branches start from `dev` and come back to it by PR.
- **`main`** only receives releases: at a release, `dev` is merged into `main` (then Actions > ci > Run workflow
  with "release" ticked publishes vX.Y.Z).

## Non-negotiable rules for `core/`

- **C99, `float` only**, `f` suffix everywhere. The FM-1 build rejects any `double` (checked by
  `-Wdouble-promotion -Werror` in the tests).
- **No libc**: no `<string.h>`, `<stdio.h>`, `<stdlib.h>`, `<stddef.h>`, no `NULL` (write `0`), no
  `memset`/`memmove`, no `malloc`. Only `<stdint.h>` and, off the FM-1, `<math.h>` through `dsp_common.h`.
- Math: `dsp_tanf`, `dsp_expf`, `dsp_powf`, `dsp_floorf`, `dsp_fabsf` (never the libm names directly).
- Nothing expensive at audio rate: tan/exp/pow only at control rate (every `DSP_BLOCK` = 32 samples) or in `set_param`.
- No hardware access, no UI or MIDI reading inside an engine.
- Engine parameters **normalised 0..1**; conversions in `set_param`, stored in the derived struct. New VA
  parameters go **at the end** of the enum (saved live sets store parameters by number).
- The Live Core is changed **only on the audio side**: hosts send `EV_LIVE` commands; the UI only reads it.
- Engines must stay silent-cheap (sleep when nothing sounds) and render any block size identically.
- Readable code: small functions, one job each, comments that explain *why*. Don't compress code visually.
- A refactor must keep the audio identical: compare `build/ols_sim demo` output before/after (`cmp`).

## Rules for the app (`app/`)

- `app_render` runs in the audio interrupt: nothing blocking, no storage, no screen.
- The UI (`app_frame`, main loop) **never** changes the engine directly: it pushes `ols_event_t` into the
  queue. Exception: before audio starts (`app_init`).
- Incoming MIDI is read inside `app_render`.
- Audio out: int32 **Q23** (not Q31), interleaved stereo; the real rate is 24 MHz / 544 = 44117.647 Hz.
- Big buffers in `PLAT_POOL`.

## Licences

This repository: **MIT**. **Never copy GPL code here** (not from Felucca, not from the SMK-37 repository, not from
`smk37_open`); Felucca-Engines (MIT) and Mutable Instruments (MIT): yes, with the notice. Details:
`docs/_EN/09-licences.md`.

## Known pitfalls

- The JieLi compiler (device repositories) has no C library and rejects `double`: the no-libc test here catches it.
- Saved live sets store parameters by number: new parameters go at the end; `OLS_MAX_PARAMS` (61) bounds them and a
  full live set must fit two FM-1 flash objects (checked at compile time in `app/src/app_settings.c`).

## Docs style

Clear English / French, short sentences, tables to compare. Every "tested" claim says where and how. Update
`STATUS.md` and `CHANGELOG.md` at every step, and both `docs/_EN` and `docs/_FR`.

Rules for people (branches, commits, releases): `docs/_EN/18-rules-git-and-publishing.md` (FR: `docs/_FR/18-regles-git-et-publication.md`).
