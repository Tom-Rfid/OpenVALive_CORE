# 07 — Writing an engine (instrument)

An engine is an instrument: VA today, Acid, FM, Drum tomorrow. It reuses the DSP core and is exposed through the
common interface `core/engines/engine.h`. The sequencer, scenes, MIDI and UI only know that interface.

## The interface

```c
typedef struct {
    const char *name;
    uint16_t param_count;
    const char *const *param_names;            /* short names (<= 8 characters) */
    void  (*note_on)(void *ctx, uint8_t note, uint8_t velocity);
    void  (*note_off)(void *ctx, uint8_t note);
    void  (*all_notes_off)(void *ctx);
    void  (*set_param)(void *ctx, uint16_t id, float value01);
    float (*get_param)(void *ctx, uint16_t id);
    void  (*pitch_bend)(void *ctx, float semitones);
    void  (*render)(void *ctx, float *out_l, float *out_r, int n);
} ols_engine_api_t;
```

## Steps

1. **Folder** `core/engines/<name>/`. Copy the layout of `engines/va`: a public header, then one file per job
   (api, params, presets, voices, render) and a private `_internal.h`.
2. **Parameters**: an `enum` of ids, values **normalised 0..1** stored in `p[]`, and every conversion (Hz,
   seconds, semitones) in `set_param`, stored in a "derived" struct. Never convert inside `render`.
3. **Render**: loop over sub-blocks of `DSP_BLOCK`; modulations at the start of each sub-block, samples in the
   inner loop. Copy the values the inner loop needs into local `const` variables.
4. **Memory**: no allocation. Big buffers (delays, samples) are given by the caller (`PLAT_POOL` on the FM-1).
5. **Table**: `const ols_engine_api_t ENGINE_<NAME>_API = { ... };`.
6. **Presets**: at least INIT and one sound that shows the engine's character.
7. **Build files**: add the `.c` files to `app/ols_dsp.c` (FM-1) and to `CORE_SRC` in the `Makefile`
   and in `tests/run_tests.sh`.
8. **Tests**: add a case to `tests/run_tests.sh` (no NaN, no clipping, pitch) and a demo render.
9. **Budget**: measure with `make bench` (adapt `connectors/pc-sim/bench.c`), then on the FM-1 (CPU on screen).
   Target: under 10 % of the FM-1 per voice, under 50 KB of flash per engine.

## DSP rules of the project

- `float` only, `f` suffix on every constant (`0.5f`); the FM-1 build rejects any `double`.
- No `tanf` / `expf` / `powf` at audio rate; use the `dsp_*` names from `dsp_common.h` (libm or fastmath).
- No `<string.h>`, no `NULL` (write `0`), no `malloc`.
- The engine never reads buttons or MIDI: it receives calls.
- Readable before clever: one function per job, English comments explaining *why*.

## References for the next engines

| Engine | To study | Licence of the reference |
|---|---|---|
| Acid (303) | Open303, x0xb0x, AcidBox | check each before reusing code |
| FM | Dexed (msfa) | Dexed GPL-3; msfa Apache-2.0 |
| Drum | Felucca `drum_voice.c` | GPL-3: get ideas, don't copy |
| various | Felucca-Engines (ANALOG, LOFI, TRIO, WHEEL, NOISE, PHASE) | **MIT**: reusable with the notice |
| various | Mutable Instruments (Plaits, Braids, Elements) | MIT |

Before importing external code, read [09 — Licences](09-licences.md).
