# 16 — The Live Core: loops, CAPTURE, scenes, Creative Engine

The Live Core (`core/live/`) is what makes OpenVALive an instrument rather than a synth. It is portable C99
with no allocation and no C library. The same code runs on the FM-1, in the desktop app, in the browser emulator
and in the tests. This page explains how it works and how to use it in your own host.

## The pieces

| File | Job |
|---|---|
| `live_types.h` | time units (96 ticks per quarter note), notes, patterns, a tiny random generator |
| `clock.c` | the master clock: internal tempo or MIDI clock in, a free-running counter for CAPTURE |
| `pattern.c` | a loop: up to 96 notes sorted by time, 1 step to 8 bars |
| `live.c` | tracks, transport, the sequencer and the mix (the render) |
| `live_record.c` | REC (overdub) and CAPTURE |
| `live_history.c` | undo / redo, clear, Creative Engine and Euclid on a track |
| `live_scenes.c` | scenes, Morph, hold overrides, stutter |
| `live_cmd.c` | commands from the event queue; the MIDI front door (`ols_live_input`) |
| `creative.c` | the Creative Engine: pure functions on a pattern |
| `project.c` | a whole live set as bytes (flash, file), with a CRC |
| `midi_in.c`, `event_queue.h` | MIDI parser, lock-free queue between the UI and the audio |

Engines (`core/engines/`) plug in through `engine.h`. `bank.h` lists every sound (17 synth presets, 3 drum kits)
and switches a track between the VA engine and the drum engine.

## Time, to the sample

A tick is 1/96 of a quarter note (5.2 ms at 120 BPM). The renderer asks the clock how many samples remain until
the next tick and cuts its block there. The notes of that tick start on their exact sample, whatever the
platform's block size (256 on the FM-1, 512 on a PC). The test `test_sample_accurate_notes` checks this to the
sample.

The control rate of the VA engine (every 32 samples) carries over between calls. A block cut at any point sounds
bit-for-bit the same as one call (`test_va_split_render`).

Swing moves the off-beat 16ths later, by up to two thirds of a step. Each note can also have a **chance**
(0–100 %, rolled once per pass) and a **ratchet** (2–4 quick hits inside its length).

MIDI clock in (24 pulses per quarter = 4 ticks each) plays smoothly between pulses at the measured tempo, and
catches up if it falls behind. MIDI clock out sends a pulse every 4 ticks, plus start and stop.

## Undo per track, loops on the grid

- **Undo / redo per track**: the 8-step history is shared, but each step knows its track. A track's UNDO takes its own
  newest step and leaves the other tracks alone; `OLS_TRACK_ALL` takes the newest step of any track
  (`ols_live_undo_track`, `LIVE_UNDO` with a target).
- **Loops (stutter) per track, let go on the grid**: a loop runs on one track or on all (`ols_live_stutter_track`).
  Let go, it plays to the end of the slice it is in, then the music is back where it would be, in time with the bar
  and the MIDI clock; `id = 1` lets go at once (panic).

## REC and CAPTURE

- **REC** (overdub): a note enters the loop when its key is released, with its real length and its timing as
  played. With **quantize** on, it snaps to the 16th. A note that snaps just ahead of the playhead is not played
  twice. A whole take (REC on to REC off) is one undo step.
- **CAPTURE**: the last 128 notes are always kept, recording or not, transport running or not. CAPTURE walks back
  from the newest note of the selected track and stops at a silence of 1.5 bars, or at 8 bars. It rounds the
  length to 1, 2, 4 or 8 bars, then:
  - if the transport **ran**, the notes keep their place in the bars;
  - if it was **stopped**, the first note becomes the downbeat and the transport starts at the point of the
    phrase where you are now. The loop carries on in time with your playing.

## Undo

The history is one ring of 8 steps. Undo **swaps** the saved pattern with the track's pattern, so the same slot
then holds what redo needs: 8 × 0.8 KB in all. These gestures save a step: a REC take, CAPTURE, CLEAR, a length
change, every Creative Engine gesture and Euclid.

## Scenes, Morph, hold effects

- A **scene** stores every parameter of every track (16-bit), the mutes and each track's engine kind.
- **Morph** (0..1) between two scenes: continuous parameters are interpolated and switches (waveform, mode)
  flip half-way. Engines smooth every parameter change (5 ms), so Morph never clicks. A track whose engine kind
  changed since the scene was stored is left alone.
- **Overrides** are named by **role** (`OLS_ROLE_CUTOFF`, `_DRIVE`, `_DELAY_MIX`...). "Filter shut on every
  track" therefore works on synths and drums, and skips an engine without that role. Releasing an override
  restores the value it found.
- **Stutter** repeats the current 1/4..1/32 slice on every track. On release, the music is where it would have
  been, like a beat repeat.

## The Creative Engine

Never start from an empty grid: play something, then reshape it. Every operation keeps the material of the loop
(its pitches, velocities and groove). `amount` 0..1 sets the strength; on the FM-1 it comes from key velocity.

| Gesture | What it does |
|---|---|
| MUTATE | moves some notes to other pitches of the loop, accents/ghosts, a step earlier/later (never the downbeat) |
| HUMANIZE | ±6 ticks and ±18 velocity at most |
| FILL | adds notes in empty steps, from the loop's pitches, more likely towards the end |
| SIMPLIFY | removes the weakest notes (off-beat and quiet first), never empties the loop |
| ROTATE, REVERSE, TRANSPOSE | shift by steps, play backwards, semitones / octaves |
| EXPAND / CONTRACT | twice as long and slow / half as long and fast (they undo each other) |
| CHANCE | off-beat notes get a probability; beats always play |
| RATCHET | some notes become 2–4 quick hits |
| EUCLID | k hits spread over n steps (3/8 = tresillo), on the last note played |

All of them are pure functions (`creative.h`) and are tested for order, range and identities
(`rotate +3 −3`, `reverse` twice, `expand` + `contract`).

## Live sets (projects)

`ols_project_save` writes "OLSP" version 1: tempo, swing, quantize, then per track the mute, gain, engine kind,
every parameter and the loop, then the scenes, and a CRC-32 at the end. `ols_project_load` checks everything
before it changes anything. A set made with fewer parameters (0.1's 40) loads what fits. On the FM-1 a set takes
two flash objects. On the desktop it is the file `openvalive.set`.

## Using it in your own host

```c
ols_live_t live;                 /* ~16 KB, no allocation */
engine_va_t va[4]; engine_drum_t drum[4]; ols_slot_t slot[4];
ols_engine_t eng[4];
for (int t = 0; t < 4; t++) {
    engine_va_init(&va[t], delay_ram[t], DELAY_LEN);
    engine_drum_init(&drum[t]);
    slot[t].va = &va[t]; slot[t].drum = &drum[t];
    eng[t] = ols_slot_engine(&slot[t], OLS_KIND_VA);
}
ols_live_init(&live, eng, 4, sample_rate);
ols_bank_attach(&live, slot);
ols_bank_select(&live, slot, 3, OLS_SOUND_FIRST_DRUM);      /* track 4: DRUMS 808 */

/* audio thread */
while (evq_pop(&queue, &e)) ols_live_apply_event(&live, &e);
ols_live_render(&live, left, right, frames);

/* UI thread: never touch `live`, send commands */
push(EV_LIVE, LIVE_CAPTURE, ...);
```

The rule is the same as for engines. Only the audio side changes the Live Core. The UI sends `EV_LIVE` commands
through the queue and only reads the state to draw it.

## CPU

| Case | Cost (PC benchmark, relative to one synth voice) |
|---|---|
| a silent track | almost nothing: an engine with no voice and no tail sleeps |
| four idle tracks | about one eighth of a 4-voice pad |
| the Live Core itself (clock, sequencer, mix) | a few % of a voice |

On the FM-1, a synth voice costs about 5 % of the CPU. Four busy tracks are kept under budget by the voice
limits (4 + 1 + 2 + 4 = 11). Above 90 %, the busiest track drops a voice.
