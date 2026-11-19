# 12 — Forking guide

OpenVALive is meant to be forked: your own instrument on the FM-1, a different UI, a new engine, another
board. This page lists what to change so your fork builds, publishes and installs cleanly, and stays easy to
merge with upstream.

## 1. Fork and build

1. Click **Fork** on GitHub.
2. Enable Actions in your fork (Actions tab). The `tests` and `build` workflows run as they do here.
3. Enable Pages: **Settings → Pages → Build and deployment → Branch: `gh-pages`**. After the next push to `main`,
   your site (emulator + web installer + manual) is at `https://<you>.github.io/<repo>/`.

## 2. Give your firmware its own identity

In [OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1)'s `targets/fm1/fm1.toml` (a firmware fork starts from a device repository; an engine or a Live Core change starts here):

| Key | Change to |
|---|---|
| `id_prefix` | **a new `FM-1_` + digits value**, unused by any other firmware. The installers use it to recognise the package; two firmwares with the same identity confuse users and update tools. |
| `name` | your firmware's name (file names, screen, splash) — also `APP_NAME` in `targets/fm1/app.h` |
| `usb_manufacturer`, `usb_product` | how the FM-1 shows up on the computer |
| `repo`, `install`, `site` | your repository and Pages URLs |
| `author`, `tagline`, `description`, `tags`, `screenshot` | your firmware's card |

Also change `SET_MAGIC` in `app/src/app_state.c` if your settings format differs, so a device that ran
OpenVALive before loads defaults instead of misreading your data.

## 3. Where to make changes

| You want to... | Change | Keep untouched if possible |
|---|---|---|
| a new sound engine | add `core/engines/<name>/` ([07](07-writing-an-engine.md)) | `core/dsp/` APIs |
| a different UI on the FM-1 | `app/src/ui_*.c` | `app_audio.c` (the real-time rules) |
| other knobs / MIDI CCs | `KNOB_PARAM` in `app_state.c`, `midi_in_map_cc` in `app.c` | — |
| another board | a new `targets/<board>/` or `connectors/<board>/` | `core/` |
| better DSP | `core/dsp/*` + a test in `tests/` | function names and signatures |

The more your changes live in **new files**, the easier it is to pull upstream fixes (`git pull upstream main`).

## 4. Rules that keep it working

- Run `FM1_BASE=~/fm1-base sh tests/run_tests.sh` before pushing; CI runs it anyway.
- Respect the `core/` rules ([CLAUDE.md](../../CLAUDE.md)): float only, no libc, no malloc, no expensive math per
  sample.
- Never block in `app_render` (it is the audio interrupt).
- Licences: your additions can be any licence compatible with MIT; keep the existing notices; don't paste GPL
  code (Felucca, Dexed) unless your fork becomes GPL ([09](09-licences.md)).

## 5. Releasing

```bash
echo 0.2.0 > VERSION
git commit -am "Release 0.2.0" && git tag v0.2.0 && git push --tags
```
CI builds and publishes the GitHub release (firmware, installer files, desktop apps, site zip) and the site.
`bash scripts/update-prebuilt.sh v0.2.0` then refreshes `prebuilt/` in the repository.

## 6. Giving back

Fixes and engines that help everyone are welcome upstream: one topic per pull request, measurements included
(CPU on the FM-1 if DSP), docs and `STATUS.md` updated. See [CONTRIBUTING](../../CONTRIBUTING.md).

## 7. Listing your firmware

fm1-base keeps a list of firmwares built on it (open an issue with its "List your firmware" form), and FM-1
firmware hubs read `firmware.json` (written by every build).
