# 18 — Rules: code, git and publishing

The rules everyone follows here, people and AI agents alike. They keep OpenVALive fast, readable and publishable.
The agents' version, with the commands, is [CLAUDE.md](../../CLAUDE.md).

## Branches

| Branch | What goes there |
|---|---|
| `dev` (default) | the work: every pull request targets `dev` |
| `feat/<topic>`, `fix/<topic>` | one topic each, from `dev`, back to `dev` by pull request |
| `main` | **releases only**: `dev` is merged into `main` by a pull request "Release vX.Y.Z", then published |

No direct push to `main`. An urgent fix on a release: `fix/<topic>` from `main`, PR to `main`, then merge `main`
back into `dev`.

## Commits and pull requests

- One topic per commit; a short English summary (imperative, ≤ 72 characters), then *why* in the body.
- Say **where** it is tested: PC, simulator, real device (model, firmware).
- `FM1_BASE=~/fm1-base sh tests/run_tests.sh` must end with "All tests pass." before a PR; CI runs it again.
- Work done with an AI agent says so: a `Co-Authored-By:` line in the commit.

## Code

- **C99, `float` only** (`f` suffix), no `double`: the JieLi compiler rejects it (`-Wdouble-promotion -Werror`).
- **No C library, no `malloc`** in the core and the app: `<stdint.h>` only; maths through `dsp_*`.
- Expensive maths (tan, exp, pow) only at **control rate** (every 32 samples) or when a parameter changes.
- The audio interrupt never blocks: no storage, no screen. The UI never touches an engine: it sends events.
- Engines sleep when silent and render any block size identically. **A refactor keeps the sound bit-identical.**
- New parameters go **at the end** (saved live sets store them by number).
- Small functions with one job, comments that say *why*, English in the code.
- Licence of this repository: **MIT**. Never copy code with a licence that does not allow it; MIT code from
  elsewhere keeps its notice.

## Docs

- **English and French together**: `docs/_EN` and `docs/_FR`, same numbers, same content.
- The repository describes what is **done or in progress**; undecided ideas stay outside it.
- Every "tested" says where and how. `STATUS.md` and `CHANGELOG.md` are updated with the work.

## Releasing

1. `dev` is green; `VERSION`, `CHANGELOG.md` and `STATUS.md` are up to date.
2. Pull request `dev` → `main` "Release vX.Y.Z", merged.
3. Actions > ci > Run workflow, branch `main`, "release" ticked: the tag `vX.Y.Z` and the release are made.
4. Test the release on the real device (install, play, save, power cycle, leave again) and write it in `STATUS.md`.

## Releases of this repository

This repository has no firmware: its releases are **versions of the synth** that the device repositories pin
(`v0.4.0`), with the desktop apps attached. A device release always pins one of them.
