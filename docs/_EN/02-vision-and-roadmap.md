# 02 — Vision and roadmap

## Principles

1. **Live first.** Every feature is judged by: does it help to play, right now?
2. **Performance ≠ configuration.** Performance mode has no menus. Configuration (audio, MIDI, controls,
   system) is separate, behind a dedicated gesture, **two levels deep at most**.
3. **Colour shows state.** Green = active, yellow = armed, red = recording, grey = muted. No decorative animation.
4. **Few things, done well.** Three waveforms, one filter, three effects to start. Every addition must earn its
   RAM, its CPU and its clarity.
5. **One shared core.** DSP, sequencer and scenes are shared by every instrument. A new engine rewrites neither
   audio, nor MIDI, nor presets.

## Three layers

```
OpenVALiveCore          DSP core · parameter system · MIDI · clock · storage · platform · simulator
OpenVALive         engines: VA (done), then Acid, FM, Drum...
OpenVALivePerformance   Morph · scenes · capture · undo · live effects · Creative Engine
```

## Hybrid

The FM-1 plays its own sounds **and** drives other gear or a DAW over USB-MIDI, on one clock. A track will have a
destination: the internal engine or a MIDI channel. Only standalone playing is built in V1; the architecture is
ready for the rest. (The FM-1 has no MIDI OUT jack: MIDI out goes over USB, and Bluetooth if enabled one day.)

## The Creative Engine

| Function | Effect |
|---|---|
| **Mutate 5 %** | two notes of the loop change; again, two others |
| **Humanize** | a little micro-timing and velocity variation |
| **Fill** | while held: ratchets and ghost notes |
| **Simplify / Densify** | removes or adds notes, keeping the strong beats |
| **Rotate, Reverse, Transpose, Expand, Contract** | pattern variations |

## Roadmap

| Priority | Content | Status |
|---|---|---|
| **P0** | FM-1 port | **done** (V0.1) |
| P0 | DSP core | **done** |
| P0 | PC simulator (engine) + whole FM-1 app in the fm1-base simulator + browser emulator | **done** |
| P0 | Event queue (timestamped) | **done** |
| P0 | Parameter smoothing, interpolation type (continuous / discrete) | **done** (V0.2) |
| P0 | Central clock, MIDI clock in/out, sample-accurate events | **done** (V0.2) |
| P0 | Presets and projects (versioned "OLSP" live sets) | **done** (V0.2) |
| **P1** | 4 tracks, MIDI loops, capture, undo | **done** (V0.2) |
| P1 | Morph, 8 scenes, hold-to-apply performance effects | **done** (V0.2) |
| **P2** | Humanize, Mutate, Fill, Simplify, probability, ratchet, Euclidean | **done** (V0.2) |
| **P3** | Drum engine | **done** (V0.2, at the owner's request); Acid = a VA preset; FM later |
| P3 | Phaser | **done** (V0.2) |
| P3 | SMK-37 target | **done** (V0.3, GPL-3.0 on `smk37_open`: doc 17, doc 20); to check on hardware |
| **P4** | reverb, modal and waveguide engines, advanced modulation matrix | later |

**V1.0 "the first instrument"** = VA + Drums + Loop + Capture + Morph + Scenes + Undo + Creative Engine, on the
FM-1 and the SMK-37, checked on both. It is the base for the mode forks.

**Not in V1, on purpose:** MPE, granular, wavetables, flanger, free audio routing, heavy reverb, Octatrack
crossfader, modulation matrix with more than 4 slots. Each adds menus, RAM or bugs before it adds play.

## The sequencer: events, not steps

```c
typedef struct {
    uint16_t tick;      /* start, 96 per quarter note: micro-timing is native */
    uint16_t len;       /* may run over the bar */
    uint8_t note, vel;
    uint8_t chance;     /* 0..100 % */
    uint8_t ratchet;    /* 1..4 hits */
} ols_note_t;           /* core/live/live_types.h: 8 bytes */
```

Chords are several notes at the same tick. Off-beats, long notes and micro-timing are native. Loops are MIDI (a
few bytes), not audio (hundreds of KB). How it all works: [16 — The Live Core](16-the-live-core.md).

## Known risks

- **Doing everything too early** (VA + FM + Acid + Drum...): one engine until V1.
- **Too many P0s**: V0.1 validated the hardware first; every version must stay playable.
- **Menus creeping back**: every new UI page has to be justified in its pull request.

## Measurements that drive decisions

On the real FM-1, 4 voices cost 24–26 % of the CPU, one voice ~5 %. V0.2 chose four engines with 11 voices in all
and engines that sleep when silent. The FM-1 measurements of V0.2 will say whether the voice limits can grow or a
reverb fits.
