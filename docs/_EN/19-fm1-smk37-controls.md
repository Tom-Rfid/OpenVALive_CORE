# 19 — FM-1 ↔ SMK-37: the controls side by side

The shared app (`app/`) is written for the FM-1's panel; the SMK-37 maps its own controls onto it
(`targets/smk37/smk37_app.c` in OpenVALive_SMK-37: `BUTTON_MAP`, `ENCODER_MAP`, `smk_screen_key`, the pads).
**Rule:** a gesture added to the app is added here, and checked on both (FM-1 simulator script, SMK-37 host test).
Same behaviour on both; the SMK-37 only adds what it has more (pads, faders, wheels, pedal, more keys).

## Buttons (same names on both panels)

| App button | FM-1 | SMK-37 | Alone | Held (layer) |
|---|---|---|---|---|
| `B_FX` (`BTN_PAGES`) | FX | FX | next knob page (MAP CONTROL's order) | line reset (key 1), wheel BEND / PAN (key 14), drum map (keys 15 / 16) |
| `B_SEL` (`BTN_TRACKS`, `BTN_VALIDATE`) | SEL | SELECT (button) | takes a waiting sound | tracks layer: mutes, CLEAR, QUANTIZE, CLOCK, EUCLID, CONTROLLER; black keys MUTE ALL, A VOL |
| `B_SEQ` (`BTN_PERFORM`) | SEQ | SEQ | tapped: a loop lets go | perform layer (16 effects) |
| `B_ARP` | ARP | ARP | on A: arpeggio mode | creative layer |
| `B_GLO` | GLO | GLO | — | scenes layer; GLO + SEL: MAP CONTROL; GLO + PLAY: chain |
| `B_HOME` | HOME | HOME | panic | PANIC / RESTART / STOP, SOUNDS (14), LOOP EDIT (15), OPTION CONTROL (16) |
| `B_SAVE` | SAVE | SAVE | saves the live set | SAVE + key: user sound |
| `B_ENV` / `B_LFO` | ENV / LFO | ENV / LFO | UNDO / REDO | — |
| `B_EDIT` | EDIT | EDIT | CAPTURE; in a menu: the row's default | EDIT + note: punch |
| `B_PLAY` / `B_REC` | PLAY / REC | PLAY / REC | transport / record | SEL + PLAY tap tempo, SEL + REC metronome |
| `B_OCTDN` / `B_OCTUP` | OCT− / OCT+ | OCT− / OCT+ | octave; family jump; loop ×2 / ÷2 | — |
| — | — | KNOB BANK = SHIFT | — | SHIFT + K1–K4: LIVE page; SHIFT + SELECT: SOUND EDIT; SHIFT + pads 1–8: store scenes |
| — | — | FADER BANK | faders: levels → macros → USER | — |
| — | — | < (Bluetooth) | TAP tempo | held 0.5 s: MAP CONTROL |
| — | — | > (PAD/STEP) | pads: PERFORM → PLAY → SCENES | — |

## Knobs

| App encoder | FM-1 | SMK-37 |
|---|---|---|
| `EN_SELECT` | SELECT | encoder 1 |
| `EN_PRESET` | PRESET | encoder 2 (PRESETS) |
| `EN_K1`..`EN_K4` | K1–K4 | encoders 3–6 |
| `EN_ALGO` | ALGO (tempo) | encoder 7 (PAGE) |
| master | MASTER pot | encoder 8 |

## Keys under a layer

| | FM-1 (27 keys, F3–G5) | SMK-37 (37 keys, C3–C6) |
|---|---|---|
| layer slots 1–16 | the 16 white keys F3 … G5 | the first 16 white keys C3 … D5 |
| black keys (`BLACK_KEY[]`) | F#3 G#3 A#3 C#4 D#4 F#4 … (11) | C#3 D#3 F#3 G#3 A#3 … (the n-th black = the FM-1's n-th) |
| SEL + 1st black key: MUTE ALL | F#3 | C#3 (and SCENES pad 13) |
| SEL + 4th black key: A VOL | C#4 | G#3 (and SCENES pad 14) |
| drum PERC keys (OPTION CONTROL) | PERC 1–4: G#3 D#4 G#4 D#5 | PERC 1–6: D#3 G#3 D#4 G#4 D#5 G#5 (`APP_PERC_FIRST` / `APP_PERC_COUNT`) |
| key lights | the keys' LEDs | the screen keyboard (KEYS ON SCREEN), while a layer is held |

## Only on the SMK-37

Pads (PERFORM / PLAY with rolls by pressure / SCENES with mutes, MUTE ALL, A VOL), 4 faders (levels, macros, USER
slots), pitch wheel (BEND or PAN) and mod wheel (MORPH by default; both mappable), sustain pedal, MIDI OUT TRS, key
velocity curve.
