# 00 — The OpenVALive strategy, simply

## The one-sentence version

> Don't build a more complex synth. Build a more inspiring instrument — and give away the parts.

## Three bets

**1. Live first, not synth first.**
Small synths don't lack sounds, they lack *playing*. OpenVALive judges every feature by one question:
does it help you play right now? So: no menus in performance mode, four knobs that matter, loops and scenes
before more oscillators. Our neighbours are the Dirtywave M8, OP-1, Circuit Tracks, Electribe and Deluge —
improvisation machines — not Serum or Diva.

**2. Open the parts, not just the product.**
A firmware for one device helps a few hundred people. A clean, portable DSP core helps every project on the
same chip, and every hobbyist with a Daisy, a Teensy or a plugin SDK. So the work is split in layers anyone can
reuse on its own:

```
core/        the sound: DSP blocks, engines, MIDI, real-time events        (portable C99, MIT)
connectors/  the same sound on a PC, a plugin, a Daisy, a Teensy...
devices      their own repositories: OpenVALive_FM-1, OpenVALive_SMK-37 (this one as a submodule)
```

**3. Never flash to find out.**
Every change can be heard, seen and measured on a computer first: the engine simulator, the whole FM-1 app in
the fm1-base simulator (screen, keys, MIDI, storage), the browser emulator, automatic audio tests (pitch,
aliasing, no-libc build). Flashing is the last step, not the debugger.

## How we decide what to build

| Priority | What | Why first |
|---|---|---|
| P0 | the FM-1 port, the DSP core, the simulator, the event queue — **done** | everything else rests on them |
| P0 | parameter system with smoothing, a central clock | Morph, scenes, MIDI, automation and the sequencer all need them |
| P1 | 4 tracks, MIDI loops, capture, undo, Morph, scenes | the first real *instrument* (V1.0) |
| P2 | Creative Engine: Mutate, Humanize, Fill, Simplify | what makes OpenVALive different |
| P3 | more engines (Acid, FM, Drum) | only once one engine is great |
| P4 | reverb, big modulation matrix, wavetables | maybe, if they earn their RAM and their menus |

And a list of things we refuse in V1, on purpose: MPE, granular, wavetables, phaser, flanger, free audio routing,
heavy reverb, Octatrack-style crossfader, a giant modulation matrix.

## The Creative Engine: our signature

Transformations you apply to a loop while it plays, without opening an editor: *Mutate 5 %* changes two notes,
*Humanize* adds micro-timing, *Fill* adds ratchets while a button is held, *Simplify* removes notes but keeps the
strong beats. No AI, no cloud: small, predictable algorithms on MIDI events, at zero DSP cost.

## Hybrid by design

The FM-1 plays its own sounds **and** can drive other gear or a DAW over USB-MIDI, on one clock. In V1 only the
standalone instrument is built, but a track already has a *destination* (internal engine or MIDI out), so nothing
has to be rewritten later.

## Built to be forked

Everything needed is in the repository or downloaded by a script: source, prebuilt firmware and desktop app,
build scripts, tests, CI that builds and publishes a fork exactly like the original, and a
[forking guide](12-forking-guide.md). Licences are chosen for that: our code is MIT, the FM-1 platform underneath
is MPL-2.0, and we never copy GPL code into the project.

## What success looks like

Someone switches on an FM-1 and improvises for an hour without reading anything. Someone else takes
`core/` and ships a plugin. A third person forks the repo, adds an Acid engine and sends it back.
