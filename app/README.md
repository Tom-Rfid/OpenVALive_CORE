# The OpenVALive app (`app/`)

The instrument's UI and its audio side, shared by every device: 4 tracks, the knob pages (SOUND, MACRO, LIVE,
SETUP, USER, EQ, DRUM EQ), the layers on the white keys (ARP creative, GLO scenes, FX perform, SEQ tracks), the MAP CONTROL and
SOUND EDIT pages, saving, MIDI. It is written for a **240 × 240 screen** and the FM-1's panel (27 keys, 14 buttons,
7 encoders), on **fm1-base's platform API** (`plat.h`).

## What a target supplies

| File (in the target) | What |
|---|---|
| `app.h` | includes `openvalive_app.h`, and adds `APP_NAME`, `APP_SPLASH_RGB`, `APP_STORE_MAP` (where each flash object lives), the platform switches (`APP_TOUCH`, `APP_OVERLOAD`, `APP_HOST_EXPECT`) |
| `app.c` | one line: `#include "<path>/app/openvalive_app.c"` (fm1-base compiles the target's `app.c`) |
| `ols_dsp.c` | one line: `#include "<path>/app/ols_dsp.c"` (the core at -O2) |
| the platform | fm1-base (the FM-1), or the target's own implementation of `plat.h`, `lcd_*`, `touch_*` (the SMK-37) |

Optional hooks a target defines before including `openvalive_app.c`: `APP_KEY_LAYER(k)` (a key stands for a layer:
the SMK-37's pads), `APP_MAP_SLOTS` (more MAP CONTROL slots: faders, wheels), `DSP_SAMPLE_RATE` / `DSP_SR_F`.

## Rules

- `app_render` runs in the audio interrupt: nothing blocking, no storage, no screen; MIDI in is read there.
- The UI never changes an engine directly: it pushes `ols_event_t` into the queue (`app_events.c`).
- Audio out: int32 **Q23**, interleaved stereo. Big buffers in `PLAT_POOL`.
- Keep this folder free of device specifics: a device adds a hook here, and its code in its own repository.
