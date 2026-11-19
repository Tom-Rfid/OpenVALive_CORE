# 09 — Licences

*Not legal advice: a summary of the licences involved and the rules the project follows.*

## OpenVALive

The code of this repository is **MIT** ([LICENSE](../../LICENSE)): anyone can reuse it, including in commercial or
closed products, keeping the copyright notice.

## What it builds on

| Component | Role | Licence | Consequence |
|---|---|---|---|
| [fm1-base](https://github.com/charlesvestal/fm1-base) | FM-1 platform (screen, audio, MIDI, storage, simulator, tools, site, web installer) | **MPL-2.0** | our files keep their licence; a change to an fm1-base file is published under MPL-2.0 |
| [felucca-bsp](https://github.com/hugelton/felucca-bsp) | hardware layer (chip, FM-1 board, update loader) | **MPL-2.0** | same |
| JieLi AC79 SDK (3 files) | needed for the `.fwsc` package | Apache-2.0 | downloaded at build time, never in this repository |
| JieLi toolchain | compiler | proprietary, downloaded by `get_toolchain.sh` | never in this repository |
| fm1-base fonts (Barlow, Terminus) | screen | SIL OFL 1.1 | embedded in the firmware by fm1-base |
| [miniaudio](https://miniaud.io) | desktop audio | public domain or MIT-0 | vendored in `third_party/` |

The firmware (`.fwsc`) therefore combines MIT (OpenVALive), MPL-2.0 (fm1-base, felucca-bsp) and Apache-2.0
files. To distribute it, pointing to this repository and to fm1-base / felucca-bsp provides the MPL sources.
The firmware releases are built by the device repositories' CI from their source and this repository's.

## What we don't copy

**Felucca** (the full firmware) is **GPL-3.0-only**. Copying its code into OpenVALive would force the whole
project to GPL-3.0. We study it and learn from it; we don't copy it. Its engines, **Felucca-Engines** (ANALOG,
LOFI, TRIO, WHEEL, NOISE, PHASE, `dsp.c`), are **MIT**: reusable with their copyright notice.

## Before importing external code

1. Check the licence **of the file** (not just the repository: Felucca mixes GPL, MIT, MPL, Apache and LGPL).
2. Compatible with MIT as is: MIT, BSD, zlib, Apache-2.0 (keep the NOTICE), public domain / CC0 / MIT-0.
3. GPL, LGPL: no, unless the project decides to change its licence.
4. Keep the original copyright header and add a line to `third_party/README.md` or the README.

## The device repositories

[OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) is MIT too. [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37)
is **GPL-3.0** (with the SDK's linking exception), because it is built on M-VAVE's `smk37_open`: the SMK-37 firmware
as a whole is GPL-3.0 and includes this repository's MIT code, which the GPL allows. The rule here: **no GPL code
in this repository.**

## Trademarks

M-VAVE, FM-1 and SMK-37 are trademarks of their owners. OpenVALive is not affiliated with or endorsed by
M-VAVE, JieLi, Hügelton Instruments or the authors of fm1-base.
