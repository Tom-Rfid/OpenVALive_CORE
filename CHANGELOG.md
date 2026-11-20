# Changelog







## 20 November 2026 — STAB, PLUCK, RISER

- STAB and PLUCK hold with the keys (a body after the attack, a longer release) instead of a 200 ms blip; RISER is
  heard at once (shorter attack, the filter a little open).

## 20 November 2026 — CAPTURE shows what it takes; clips in less RAM

- Playing on the selected track (not recording), from the first note the line paints green as the playhead goes (what
  CAPTURE would take: you see when the loop comes round), the red infinity sign shows and EDIT blinks fast; CAPTURE
  (EDIT tapped) puts the line back at once. A stop, REC or another line ends it too.
- Clips keep their notes in one pool (160 notes for the 14): 3 KB of RAM less.

## 20 November 2026 — REC + key 1: RESET

- REC is a layer too: REC + the leftmost white key, REC let go first, RESETs (clears) the selected track's loop (ENV
  undoes it). REC alone now acts on release.

## 20 November 2026 — families: PSY BASS with the basses, PSY before USER

- PSY BASS joins the basses (F2) and holds as long as the key. The psy hits (ZAP, LASER, BLIP, METAL, ARP) go to F8,
  just before USER: ACID, BASSES, DRUMS, PUNCH, SFX, UNDER, SOFT, PSY, USER.

## 20 November 2026 — no breathing

- Lights and the screen keyboard: offered but not on = dim (steady), on = lit, a few blink fast (MUTE ALL...);
  nothing breathes any more (keys, buttons, A VOL).

## 20 November 2026 — boot screen on the FM-1, hints, HOME set-up keys, sounds

- Fixed: on the FM-1 a key played at power-on did not end the boot screen (keys read between frames).
- "REC (arrow down) PLAY FIRST": green, 1.6 s. A layer button held, its title gone: the header stays empty.
- Stopped: the tracks' notes mint. CAPTURE: "LOOP CAPTURED T1" in green, the line cyan a moment.
- The red infinity sign shows while EDIT is held playing (a loop being made), in place of PLAY.
- HOME: OPTIONS on white key 7, SOUNDS on 8, LOOP EDIT on the 4th black key.
- SOUNDS screen: the families' names stay on their lines, the chosen sound's name on top.
- SCREECH 20 % lower; PSY BASS redone (two saws, strong sub, driven, a little sustain).

## 20 November 2026 — scroll bars, NEXT PAGE

- MAP CONTROL, OPTION CONTROL and SOUND EDIT show a scroll bar on the right when their list is longer than the screen.
- The menus' last line is "→ NEXT PAGE" (was MENU); on it, the help says "<- PRESET ->".

## 20 November 2026 — SAVE ALL is a key

- SAVE alone saves nothing any more: SAVE + the 1st black key = **SAVE ALL** (blinking when something is to be
  saved). Every SAVE-layer key is valid on release (SAVE let go first): LOAD USER 1–8, SAVE USER 1–8, SAVE ALL.
- On a screen keyboard: LOAD keys violet when they hold a sound, grey when empty.

## 20 November 2026 — SAVE in steps, lights by meaning

- SAVE goes in steps a fifth of a second apart (settings, live set, USER sounds, map), each written only if it
  changed: no long freeze of the sound and the screen. SAVE blinks while it saves.
- A mute, the track chosen, RESTART, REC / punch no longer count as changes to save (`live.changes`): SAVE no longer
  lights after a mute.
- Lights: FX and SEL lit; ARP, SEQ, GLO breathe over 2 s (the other buttons offered at the mid level); EDIT lit while
  playing. Screen keyboard: yellow pressed, violet on, mint offered, pink valid on release, mint / pink both, blue
  set-up screens, red muted. A VOL valid on release.

## 20 November 2026 — LOAD / SAVE keys, the screen keyboard shows what is pressed, pad kit, SINGLE / LOOP clips

- GLO and SAVE layers alike: white keys 1–8 **LOAD** (scenes; USER sounds onto the selected line), 9–16 **SAVE**
  (pink on a screen keyboard). Titles "LOAD n" / "SAVE n". SAVE + keys 1–8 used to save: now 9–16.
- Screen keyboard: a key held is yellow; SEL's mute keys red while muted; the set-up keys go red to blue over 2 s.
- OPTION CONTROL: a new sound's page "SAME PAGE" (was KEEP); SEL + 16th key says PLAY MODE LIVE / FULL MIDI.
- Clips: SINGLE (once) or LOOP (latched, `CLIP_LATCH`). `T_PADS`: the pads' own drum kit (`set.pad_kit`, in the
  settings' padding: same format).

## 20 November 2026 — clips, boot screen, "∞ DIV / FILTER"

- `app/src/clips.c` (targets with `APP_CLIPS`): 14 clips, phrases taken on a line (a track or A) from its first
  note, length rounded to the beat, at most 4 bars / 48 notes; played back on the audio side (`EV_CLIP`), now or on
  the next beat, looping, on the line's sound, not recorded in the loop. The line being taken goes green on screen.
- The boot screen goes at the first key, pad or button, even pressed in its first half second (it was missed then).
- PERFORM's title: an infinity sign, "DIV / FILTER".
- OPTION CONTROL: **PLAY MODE** LIVE (default) / FULL MIDI (the keys and pads only send MIDI; was CONTROLLER).
- Doc 19: settings, what only one of the two has, the FM-1 sets the common functions.

## 20 November 2026 — breathing keys, brighter glow (FM-1)

- A layer held: the keys it offers breathe (a second up and down: the dim pulse swept 0.5–4 µs, the mid level at the
  top; `APP_DIM_LEVELS` targets), and their fill on a screen keyboard too; otherwise the glow at its brightest
  (4 µs, was the low glow 1.6 µs). HOME's set-up keys breathe instead of blinking.

## 20 November 2026 — names per target, HOME set-up keys moved, titles, REC / CAPTURE while playing

- `APP_BUTTON_NAMES`, `APP_ALGO_NAME`, `APP_PRESET_NAME`: the screen names the target's buttons (SMK-37: NOTE,
  GLOBE, PARA, PATCH, BT, SCALE, CHORD, PAGE, PRESETS) in the menus' help, the layers' "let go" and "+SEL".
- HOME + white keys 8 / 9 / 10 (from the 2nd F): SOUNDS, LOOP EDIT, OPTIONS (were 14–16); HOME's keys lit, the
  set-up ones blinking slowly, violet on a screen keyboard.
- Titles: a gesture taken shows its name in green with a check mark (0.7 s), given up in red (0.7 s); any key
  pressed clears the title shown.
- REC and CAPTURE only while playing ("PLAY FIRST" at STOP). SOUNDS help: OCT -/+.

## 19 November 2026 — valid on release on SEL, lights, drum levels, the infinity sign

- SEL layer: CLEAR .. CONTROLLER (white keys 6–16) valid on release (`KEY_ARMED_SEL`), the mutes stay at once.
  Muted tracks blink fast, MUTE ALL lit (fast blink: all out). SEL blinks fast while a sound waits for it.
- The infinity sign replaces PLAY while loops play (not while recording, not at STOP).
- Drums: the kick +10 % (engine); on a drum track every D# key 10 % softer, every G# 20 % (`app_play_vel`).
- CAPTURE says "ENV: UNDO".

## 19 November 2026 — LOOP EDIT chords, UNDER, recording sign, one pause for SAVE

- LOOP EDIT: EDIT held + keys add notes at the note's place (chords); EDIT tapped still adds one a 1/16 later.
- Family F7 renamed UNDER (underground). The red infinity sign shows with the pink line (from the first note),
  just left of REC (it was under the MASTER figure). EDIT blinks at 200 ms while recording (seen on the SMK-37's LEDs).
- `APP_SAVE_BEGIN` / `APP_SAVE_END`: a target can wrap the whole SAVE in one audio pause.

## 19 November 2026 — 12 sounds, families by character

- TALK, PSY BASS, ZAP, LASER, BLIP, METAL, ARP, CHOIR, NOISE, RISER, DOWNLIFTER, SHAKER (48 sounds, 43 synth).
  Families by character: F1 acid, F2 psy, F3 basses, F4 drums, F5 punch, F6 SFX, F7 dark, F8 soft, F9 USER.
  Settings OLSC (OLSB moved: drum kits, USER and hidden sounds 12 up; `hidden_from`).

## 19 November 2026 — the take's pink starts at the first note

- Recording: the line turns pink from the first note taken (or the punch), not from REC pressed, so the take's start
  is seen coming back round: when to stop. The playhead is green only over that pink (or a running loop).

## 19 November 2026 — lights checked, PAN target

- Lights, every layer checked in the FM-1 simulator (keys and screen keyboard): SAVE held now lights keys 1–8 (lit:
  a USER sound kept there); FX held lights key 14 while the wheel pans; FX and SAVE pressed show their title like
  the other layers. The set-up screens (SOUNDS, MAP / OPTION CONTROL, SOUND EDIT, LOOP EDIT) light their buttons
  (`menu_lights`: the menu buttons and PLAY dim, the ones acting there lit) and all close with any menu button.
- MAP CONTROL target **PAN** (a track in the stereo field, notched in the middle): the FM-1 pans with a knob.

## 19 November 2026 — 5 sounds, 9 families, MAP CONTROL / OPTION CONTROL, master EQ, PERC keys

- Sounds: THUNDER, RAIN, CREAK, STRINGS 70, VINTAGE KEYS (36). Families F1 acid, F2 basses, F3 drums, F4 leads,
  F5 keys, F6 pads, F7 textures, F8 SFX, F9 USER (SCREECH moved to leads). Settings OLSB (older ones moved: the
  drum kits and USER sounds 5 up, hidden sounds too). `tests/presets_test.c`: every synth sound plays and ends.
- Drums: the original map's D# / G# keys each their own percussion (`set.perc`, PERC_KEYS 6, `APP_PERC_FIRST` /
  `APP_PERC_COUNT`); new BELL HIGH / BELL LOW (agogo: the cowbell higher), TRI MUTE, TAMBOURINE, CLAVES.
- Master EQ (`core/dsp/eq3.h`: low shelf, peak, high shelf, ±12 dB; nothing while flat), the EQ page (orange).
- MAP CONTROL (`control_map.c`, map OLM2): the knob pages' order and which are shown; MACRO, LIVE, EQ, USER knobs
  mappable (a default slot keeps its page's own behaviour); EDIT: a row's default. OPTION CONTROL: DRUM KEYS,
  PERC, KEYS ON SCREEN, the page a new sound brings per line (`new_sound_page`). MENU rows go SOUNDS → MAP →
  OPTIONS in turn.
- The playhead is green over a red (loop) or pink (recording) line. SEQ tapped lets a loop go even from another
  track (it could stay stuck).

## 19 November 2026 — A VOL, MUTE ALL on a black key

- SEL + the first black key (`KEY_MUTE_ALL`, F#3): MUTE ALL (fast blink while all are out); white key 5 is free.
- SEL + the fourth black key (`KEY_AVOL`, C#4): **A VOL** (`avol_key` / `avol_tick`): the playing tracks' levels and
  A's go to one level (4 tracks: the 2 middle ones averaged; 2 or 3: the 2 loudest), linearly over the longest loop
  playing, then it stops; again or a LEVEL turned by hand stops it. The key: 2 s swell (lit, dim, off, dim). "A VOL"
  at the right of the SEL title (`ui.msg_right`). `BLACK_KEY[]` for targets. Host expects: level, a_level, avol.
- SOUNDS: the place line shows "3/5" under the family names.

## 19 November 2026 — SOUNDS and SETTINGS screens tidied

- SOUNDS: the chosen sound's place (F4 3/5), family and ON / HIDDEN under the title, the name lined up with the
  squares; family names in their colour; help line SEL ON (green) / OFF (red), OCT + (green) / - (red), arrow down
  MENU: OUT (`help_menu_out`). SETTINGS: DRUM KEYS shows the two FX keys as the screen keyboard does (violet the one
  in use, `key_chip`), KEYS ON SCREEN ON green / OFF red; CONTROL MAP title shows "OpenVALive" whole again.

## 19 November 2026 — SOUNDS: one line a family, a MENU line

- `src/sound_fav.c`: families F1–F7 on lines (label green / blue / red line = all / some / none offered, a square a
  sound), SELECT the line, PRESET the sound; the last line MENU goes to SETTINGS with PRESET or OCT, and SETTINGS
  has a MENU line back. The chosen line shows the chosen sound's name; below, its position (F4 3/5). Any menu button (HOME FX ARP SEQ GLO SAVE, `MENU_BUTTONS`) closes a set-up screen and saves.

## 19 November 2026 — SOUNDS (favourites)

- `src/sound_fav.c`: HOME + key 14, the sounds PRESET offers (`set.hidden`, settings OLSA; older ones keep everything
  offered). PRESET and the OCT family jumps go through `offered_step` / `family_step`: hidden sounds and empty
  families skipped, everything offered again if all is hidden. Set-up keys: slow blink, red on a screen keyboard.
## 19 November 2026 — STAB, POLY, DRUM EQ, track pan

- STAB and POLY (31 sounds; settings OLS9, older ones moved by themselves).
- The drum engine's two-band EQ (`DR_P_BASS`, `DR_P_TREBLE`) and the DRUM EQ page (red, drum lines only).
- `LIVE_PAN`: a track's pan (ramped, no zipper); `APP_WHEEL`: FX + key 14 switches a target's pitch wheel to PAN.
## 19 November 2026 — first tests on the SMK-37: titles held, ARP %, recording shown, drums louder

- A layer button held: what its keys last said stays on top until the button is let go or another message comes.
- Armed gestures show an arrow up and the button (↑ ARP) instead of "LET GO ARP"; ARP shows the % the key gives
  ("RANDOM 64%"), a tap chooses it, ARP let go takes the last one chosen.
- Recording a loop: a red infinity sign left of the tempo; the recorded line turns pink where the playhead records,
  and back as it passes once recording stops.
- MORPH with fewer than two scenes recalled: its bar red, no value. BARS on A: "A HAS NO LOOP" (it changed track 4).
- The boot screen stays 3 s unless a hand moves (the first 0.5 s ignored).
- Drums: kits +30 %; clap, cowbell and triangle −20 %.
- `APP_KEY_LAYER`: a pad's own layer replaces whatever button is held (the pads never follow a held layer).
## 18 November 2026 — sound families

- The sounds in 7 families, shown before the name: F1 acid, F2 basses, F3 drums, F4 leads and keys, F5 pads,
  F6 textures, F7 USER. While a sound is chosen (PRESET just turned, or a sound waiting for SEL), OCT- / OCT+ jump
  to the previous / next family's first sound.
## 18 November 2026 — 7 new sounds: acid, drum & bass, dub

- ACID SQUARE, ACID SCREAM, ACID DEEP after ACID 303; REESE, WOBBLE, DEEP SUB with the basses; DUB SIREN with the
  textures. Numbered after INIT; the drum kits and USER sounds 7 higher (settings OLS8, older ones moved by themselves).
## 18 November 2026 — LOOP EDIT

- HOME + key 15: **LOOP EDIT** of the selected track's loop, a note at a time (a small piano roll): SELECT goes from
  note to note, a key gives it its pitch, K1–K4 position / length / velocity / pitch, REC takes it out, EDIT adds one;
  the whole edit is one UNDO step. The Live Core does it (`LIVE_NOTE_EDIT`), playing or not.
## 18 November 2026 — two drum maps, a fourth tom, the keys on screen, the settings rows

- Drum keys send General MIDI notes (to the loop, MIDI OUT, a computer): the ORIGINAL map (made to be played: D#
  and G# chosen among COWBELL CLAP TRIANGLE RIM PEDAL HAT, by default COWBELL and TRIANGLE; the toms high to low F G
  A B) or GM's (36..47, every octave). FX + white key 15 / 16, FX let go first: ORIGINAL / GM, kept at once.
- The drum engine reads GM's map (six toms on four voices by pitch: a floor tom added; cymbals on the open hat; 56
  cowbell, 80 / 81 triangle); a note GM does not name plays as its place in 36..47. Live Core: 190 checks.
- The CONTROL MAP menu gets settings rows (DRUM KEYS, DRUM D#, DRUM G#, KEYS ON SCREEN), scrolled; HOME + white key
  16 opens it on them; kept when it closes. Settings "OLS7" (OLS6 and older migrated).
- Keys on screen (`APP_KEYS_SCREEN`, on for a target without key lights): a layer button held shows its keys in
  place of the scope: dark grey = nothing, half filled in mint with a mint outline = it does something now, violet =
  on, blinking = between half filled and dark grey. A target draws its own keyboard (`APP_SCREEN_KEYS`,
  `APP_SCREEN_FIRST`, `APP_SCREEN_KEY`: the SMK-37's 37 keys from C).

## 18 November 2026 — DRUMS 909, ACID 303, drum keys lit

- DRUMS 909 (after DRUMS 808): a higher, shorter kick with its click, a cracking snare, bright metallic hats, from
  the drum engine's own voices. ACID is named ACID 303. Settings "OLS5": the sounds saved past DRUMS 808 move one
  up (OLS4, OLS3 and OLS2 migrated).
- On a drum track, the kicks (C) and the open hat (A#) blink every half second; the snares (D, E) and the closed
  hat (F#) are lit.
- G# (the second closed hat) is now a triangle (two high inharmonic partials, a long ring), on the cowbell's voice
  (no RAM for an 11th): GM 44 stays the pedal hat, GM 80 / 81 play the triangle. Live Core: 190 checks.
- Desktop: `--preset ACID` still finds ACID 303 (the first word is enough).
- PRESET shows the sounds in a played order (`SOUND_ORDER`): ACID 303, BASS, SUB, the five kits, LEAD, PLUCK,
  BRASS, ORGAN, PAD, STRINGS, SWEEP, GRUNGE, SCREECH, DRONE, WIND, HOLD, WHALE, INIT, then USER 1-8. The sound
  numbers (in saves) do not change. Track 1 starts on ACID 303.
- The scope has an auto-gain (x1 to x8: down at once, up slowly): a quiet sound shows its shape too.
- The synth sounds are then numbered in that order too (ACID 303 = 0 ... INIT = 16); settings "OLS6" (OLS5 and
  older moved to the new numbers).

## 18 November 2026 — HOME and FX gestures, readable bars

- HOME + white key 1 / 2 / 3, HOME let go first: PANIC / RESTART (`LIVE_RESTART`: every track from its start;
  with a MIDI clock in, the clock keeps them) / STOP ALL. HOME alone does nothing more.
- FX + white key 1, FX let go first: the selected line's reset (its effects off, unmuted, octave 0, level unity).
- A gesture given up is said when its button is let go (name in red, 1 s), not while it is held.
- Every bar white on grey, its value bold black; MACRO cyan.
- `OLS_COLD` (clang's minsize) on the Live Core's control side (commands, gestures, undo, capture, scenes stored, the drum hit set-up,
  project save / load): smaller flash, never on what runs per sample or per tick; results unchanged.

## 18 November 2026 — recording a muted track, the bottom line, quieter messages

- REC or PUNCH on a muted track: its old loop goes (undoable) and it plays again. Live Core: 188 checks.
- The selected track's number in bold yellow; A bold when selected.
- A gesture's title stays while ARP / SEQ / GLO is held; given up: its name in red 0.5 s (no NOT TAKEN).
- A waiting sound: dark orange, +SEL green, blinking with the track.
- Bottom line: the page on the left; on the right an arrow and the next page, each in its colour.
- The engines' init zero their state with a loop: the "static const zero" copies cost ~4.6 KB of flash (same
  result, bit-identical).

## 18 November 2026 — SEL validates, PUNCH, GLO on release, lights

- Buttons: SEL holds the tracks layer, takes a waiting sound (+SEL) and, held with SELECT, points at a line (from
  A: swaps); FX turns the knob pages. `BTN_VALIDATE` joins `BTN_TRACKS` / `BTN_PAGES` / `BTN_PERFORM`.
- PUNCH (`LIVE_PUNCH`): EDIT held + a note while playing records over the selected track, replacing the notes the
  playhead passes (one loop length at most), until EDIT is let go; one undo step. EDIT tapped: CAPTURE (on release).
  Live Core: 186 checks.
- GLO + key on a track: taken when GLO is let go first, like ARP / SEQ. "LET GO .." in green; taken: mint 0.5 s;
  given up: orange behind 0.5 s. A SEQ effect taken: white on green on the bottom line, 0.5 s.
- OCT- / OCT+ resize a running loop (no SEQ needed), lit when they can. A loop running: SEQ flickers; recording:
  EDIT flickers. ENV lit with something to undo, LFO with something to redo.
- Values over the bars: one colour, grey violet. SETUP pink, USER violet. VOX gone from the bottom line.

## 18 November 2026 — look before you act (ARP / SEQ on a track), loop size on OCT

- On a track, ARP / SEQ + white key only names the gesture; ARP / SEQ let go first (the key held) applies it,
  the name in mint for 0.5 s; the key let go first gives it up (NOT TAKEN). A SEQ effect taken stays on (the same
  gesture again: off). On A, and on the SMK-37's perform pads, the keys act at once as before.
- SEQ tapped while a loop runs lets it go (on A: every loop). SEQ held + OCT- / OCT+: the loop twice as long /
  half as long; OCT- lit unless 1 BAR, OCT+ lit unless 1/32.

## 18 November 2026 — A swaps with a track, page colours, LOOP on EDIT

- From A (SEL lit), SEL held + SELECT on a synth track, SEL let go: A and the track swap every sound parameter
  (effects included) and their octaves; the loop stays. ENV on that track swaps back, LFO again, while nothing
  newer happened on it (`live.undo_top` unchanged). A drum track does not swap.
- A target may set `BTN_TRACKS` / `BTN_PAGES` (the FM-1 does: its platform's B_FX is the printed SEL).
- Page colours on knob names, bars and the page name: SOUND accent, MACRO white (dark orange values), LIVE mint,
  SETUP violet, USER pink. Values centred, bold orange, in the bar's contrast colour where it is filled.
- A running LOOP: its track turns red and EDIT flickers. The waiting sound says +SEL.
- LIVE LEVEL shows 0..100 over the whole knob (x1.5 at the top, 67 = unity).
- BARS longer: the new bars play the loop again (`pattern_repeat`), no silent hole. Live Core: 183 checks.
- The split value colour needs no buffer (the SMK-37's heap was 1.4 KB short).
- Turning the knob page no longer writes KNOBS + the page in the header: the colours and the name bottom right show it.

## 18 November 2026 — an octave per line, A live only, LOOP in sight

- OCT- / OCT+ move the selected line's octave: T1..T4 and A each keep their own (settings "OLS4", OLS3 migrated:
  every line gets the old octave). SAVE + white key keeps the octave with the USER sound; choosing that sound
  (PRESET, then SEL when the loop plays) brings it back.
- A is live only: no UNDO / REDO there (ENV / LFO say NO UNDO / NO REDO and stay dark).
- A running LOOP (1 BAR .. 1/32): white on red on the bottom line for as long as it runs, SEQ flickers fast.
- T1..T4 everywhere (the track change no longer says TRACK 1); VOX shows the voices sounding only.

## 18 November 2026 — three buttons turn, presets as tables

- FX holds the tracks layer (mutes, all four, clear, quantize, clock, Euclid; FX + PLAY tap tempo, FX + REC
  metronome, FX + PRESET MIDI channel), SEQ the perform layer, SEL turns the pages on release, takes a waiting
  sound, points at a line with SELECT (GLO + SEL: CONTROL MAP, ARP + SEL: SOUND EDIT). `BTN_TRACKS`,
  `BTN_PERFORM`, `BTN_PAGES` in `openvalive_app.h` say which button holds which role.
- The VA presets are tables of changes instead of calls: the same values in the same order (every preset checked
  identical, demo render bit-identical), about 3 KB less flash.

## 18 November 2026 — sound on SEL, pointing at a line, the header

- A sound turned on PRESET for a track whose loop plays waits: the track blinks red, the sound is red with "+VAL" in
  mint, SEL takes it, another track gives it up. An empty track still changes at once.
- SEL held + SELECT: point at a line (1-4, A) in violet; SEL let go selects it.
- Header: MASTER % (dark orange) over the CPU load (dark blue, red past 80 %); PLAY green, REC red.
- The boot screen stays 2 s more, or until a key, a button or a knob moves.
- Messages: SEL shows MUTE 1 2 3 4 ALL; a track key MUTE n / UNMUTE n.

## 18 November 2026 — the live line, per-track gestures

- **A, the live line**: a fifth line under the four tracks (SELECT past track 4). Selected, the keys play a sound of
  its own (chosen on PRESET, kept in the settings), live over the loops and not recorded, with an **arpeggiator**
  (ARP tapped: UP, DOWN, UP/DOWN, RANDOM, on the 16ths of the tempo). Its voice has no delay (no RAM for one).
- **Per track**: with a track selected, the perform effects, CLEAR, UNDO / REDO and the Creative Engine act on it
  alone; with A selected, on every track. The Live Core's undo history is per track (`ols_live_undo_track`).
- **Loops let go on the grid**: a perform loop plays to the end of its slice before the music comes back (it used
  to jump at once, out of time); HOME and ALL FX OFF still stop it at once.
- **Buttons**: SEL is the tracks layer (MUTE 1-4, MUTE ALL 4 (A plays on; A is let go), CLEAR, QUANTIZE, CLOCK
  IN / OUT, EUCLID 3..8, CONTROLLER), SEQ the knob pages (GLO + SEQ: CONTROL MAP, ARP + SEQ: SOUND EDIT).
- **Lights**: dim = available now, lit = on; a latched effect blinks.
- **Screen**: the value written over its bar (a line saved for A), the selected track in violet, A in mint, MACRO
  mint, SETUP violet, the version in the CONTROL MAP.
- Tested: 181 Live Core checks, the FM-1 simulator (96 checks), the SMK-37 host test (47).

## 17 November 2026 — one repository becomes three

OpenVALive is split: this repository keeps the core, the shared app (`app/`, from `targets/fm1`), the
connectors and the tests, with their history. The FM-1 firmware is OpenVALive_FM-1 (the old repository, renamed), the
SMK-37 firmware OpenVALive_SMK-37 (GPL-3.0). The entries below describe the whole project as it was.

## V0.4 (in progress) "Modulation"

Tested by 170 Live Core checks, the FM-1 simulator (80 checks) and the SMK-37 host test (47); the demo render is
bit-identical (a sound without modulation runs exactly the code it ran before).

### VA engine
- **Modulation matrix**, 4 slots per sound: source (LFO1, LFO2, filter env, amp env, velocity, pressure, key,
  random per note, motion) -> destination (pitch, osc 2, PW, cutoff, reso, level, osc mix, filter env amount, LFO
  rates, delay) x amount (-100..+100 %). Control rate: a few multiply-adds per voice every 32 samples.
- **LFOs locked to the tempo**: FREE or 4 bars .. 1/32 (dotted and triplets), on the bar (the Live Core gives
  every engine the tempo and the beat). **RETRIG** (the first key restarts the cycle) and **ONE** (one cycle, then
  hold: an extra envelope).
- 16 parameters at the end (61 in all): saved sets and 0.3 user sounds load; `OLS_MAX_PARAMS` 61.
- Engine API: optional `clock` (tempo + beats) and `motion` (a lane the Live Core will play).

### FM-1 and SMK-37
- **SOUND EDIT** page (ARP + SEL; SMK-37: SHIFT + SELECT too): every parameter of the track's sound, named
  choices for the VA (waveforms, modes, LFO lengths, the matrix's sources and destinations); the keys still play.

## V0.3 "SMK-37" — 2026-11-17

Built by CI for the SMK-37 Elite and Pro, tested on the host with a fake SDK (47 checks); not yet run on a real
SMK-37. The FM-1 firmware is the V0.2 one, rebuilt (identity `FM-1_77000300`).

### CONTROL MAP (FM-1 and SMK-37)
- A **USER** knob page and a **CONTROL MAP** menu (GLO + SEL; on the SMK-37, hold the Bluetooth button) to give
  each control a target: track level, macros, MORPH, SWING, PUMP, effects (FILTER, RESO, DRIVE, DELAY, CHORUS,
  CRUSH, DECAY on one track, the selected one or all), a sound knob, TEMPO, BEND, aftertouch, a MIDI CC (0–127).
- SMK-37: 10 slots: K1–K4, the 4 faders (a third FADER BANK, USER, LED blinking), the pitch and mod wheels.
- Kept in the flash (its own object: FM-1 0xD4000; SMK-37 in the sectors already reserved).

### SMK-37 Elite / Pro (`targets/smk37`, GPL-3.0)
- The FM-1 app on M-VAVE's `smk37_open` SDK (route C, approved by the owner): M-VAVE's scanner reads each key's and
  pad's factory calibration; their screen driver; our audio interrupt, pads and storage.
- **37 keys** with velocity; the white keys are the layers' 16 slots while GLO / ARP / SEQ / FX / SAVE is held.
- **16 RGB pads**: PERFORM (the 16 perform effects; **pressure sets how far** filter, delay, drive and crush go) or
  PLAY (PAD/STEP: notes, drums from C2; pressure = aftertouch). Colours show the layer and what runs.
- **4 faders**: track levels, or (FADER BANK) the macros ENERGY SPACE TENSION DENSITY.
- Pitch wheel ±2 semitones, **mod wheel = MORPH**, pedal = sustain, 8th encoder = MASTER, SHIFT + K1–K4 = LIVE page,
  SHIFT + ENV = REDO, Bluetooth button = TAP (Bluetooth off).
- **MIDI OUT jack**: clock, start/stop and notes, as over USB: a Roland T-8 follows.
- Linux build (`build.py`: the SDK's compile flags and linker script, a package writer ported from `Package.cs`),
  Windows flash script (`FLASH-SMK37.bat`), host test in `tests/run_tests.sh`, CI builds both models.

### FM-1
- The canvas is 240 × 80 (the tallest band drawn): 31 KB of RAM back.
- Release zip for Windows (`OpenVALive-FM1-X.Y.Z.zip`): browsers often refuse a bare `.bat`.
- Release button: Actions > build > Run workflow with "release" ticked.

### Docs
- New: 20 SMK-37 (controls, install, T-8 sync, test plan), EN + FR. 17: route C decided.

## V0.2 "Live Core" — 2026-11-17

Tested by 152 unit checks and the fm1-base simulator (75 checks); not yet flashed on a real FM-1.

### Live Core (`core/live`)
- Master clock: internal tempo, MIDI clock in (smooth between pulses) and out, start/stop, swing.
- Sample-accurate sequencing: the render is cut at every tick (96 PPQN).
- Four tracks, each with its own loop length (1 step to 8 bars), mute and level; chance and ratchet per note.
- REC (overdub, optional 1/16 quantize), **CAPTURE** (the last phrase becomes the loop, in time, even with the
  transport stopped), undo/redo (8 steps).
- 8 scenes, **Morph** between two scenes, hold overrides by role (filter, drive, delay), stutter 1/4..1/32.
- **Creative Engine**: Mutate, Humanize, Fill, Simplify, Rotate, Reverse, Transpose, Expand, Contract, Chance,
  Ratchet, Euclid.
- Live sets ("OLSP" v1, CRC-32): sounds, loops, scenes, tempo; sets from 0.1-sized engines load.

### Sound
- Parameter smoothing (5 ms) in the VA engine: no clicks from knobs, MIDI CCs or Morph.
- An engine with no voice and no tail sleeps: four idle tracks cost almost nothing.
- **Phaser** (4 stages, feedback) in the VA engine; 3 new parameters at the end (saved sets stay compatible).
- **17 VA presets**: INIT, BASS, PAD, LEAD, ACID, STRINGS, BRASS, PLUCK, ORGAN, SUB, SWEEP, WIND, HOLD, WHALE,
  SCREECH, GRUNGE, DRONE.
- **DRUM engine**, all synthesised (808 family): kick, snare, clap, closed/open hat with choke, 3 toms, rim, cowbell;
  kits 808, TIGHT, LOFI, IRON. GM notes and an every-octave layout for the keys.
- Engine API: roles and macros, so performance gestures and knobs work on every engine; a sound bank.

### FM-1
- New controls: SELECT track, ALGO tempo, PLAY, REC, EDIT = CAPTURE, ENV/LFO = undo/redo, SEL knob pages
  (SOUND / LIVE), layers on the white keys (ARP creative, GLO scenes, FX perform, SEQ tracks).
- **Controller mode** (local off): keys and knobs only send MIDI (channel = track, CC 74 71 75 94).
- Following another machine (Roland T-8, a DAW): MIDI in time-stamped where it lands (steady tempo), MIDI IN
  channel ALL / 1..16 / CLOCK only (hold SEQ, turn PRESET), program change n recalls scene n + 1.
- Expression: key pressure and MIDI channel pressure open the filter and deepen the vibrato; sent out as
  channel pressure.
- The built-in speaker gets a 130 Hz low cut (headphones keep the full sound).
- **User sounds**: SAVE + white key 1–8 keeps a track's sound as USER 1–8 (saved in flash, on any track).
- **Metronome** (SEQ + REC) with a one-bar **count-in** for REC from stop; **tap tempo** (SEQ + PLAY).
- **Safe start**: HOME held at power-on skips loading the saved set.
- **DJ-style performance**: perform effects latch on a tap and last while held; LOOP 1 bar..1/32, FILTER SHUT/OPEN,
  DELAY THROW, DRIVE, CRUSH, BREAK, SOLO, DRUMS ONLY, ALL OUT, ALL FX OFF; tracks taken out come back on the next
  bar; scenes recalled while playing land on the next bar, as a cut or a 1/2/4-bar cross-fade (auto-Morph).
- A SETUP knob page for preparation: MIDI IN, XFADE, QUANTIZE, BARS.
- **Instrument macros** (MACRO page): ENERGY, SPACE, TENSION, DENSITY move many parameters of every track at once
  (by role, on top of the stored sounds) and the density of the loops.
- **PUMP**: sidechain from the drum track to the others (LIVE page).
- **Scene chain** (song mode): GLO + PLAY plays the stored scenes in turn, 4/8/16 bars each.
- ACID gets **accents**: velocity opens the filter (new VA parameter VEL FILTER).
- Underground / grunge sounds, all synthesised: HOLD (ship's hold), WHALE, SCREECH, GRUNGE, DRONE, and the
  DRUMS IRON kit; a **bitcrusher** in the VA engine (bits and rate).
- Screen: track and sound, tempo and transport, the four loops with playheads, knob pages.
- SAVE keeps the whole live set (two new flash objects); updating from 0.1 keeps the 0.1 sound on track 1.
- Firmware identity is now **FM-1_77** (FM-1_42 is MODELAR's); `fmhub.json` for The Firmware Hub.

### Computer
- The desktop app runs the Live Core: four tracks, drums, every gesture on the keyboard, `openvalive.set` files,
  a 4-track `--demo`.

### Docs
- New: 16 the Live Core, 17 publishing (Firmware Hub, releases, SMK-37), 18 future modes and forks (EN + FR).
- Manual, using the FM-1, desktop app rewritten for V0.2; new screenshots.


## V0.1 "Foundations" — 2026-11-17

First version, validated on a real FM-1.

### DSP core
- PolyBLEP oscillators (saw, PWM square, triangle), sub, noise.
- Trapezoidal ZDF state variable filter (LP, BP, HP, Notch).
- Exponential ADSR envelopes, 4-shape LFOs, rational tanh saturation.
- int16 delay with filtered feedback, Juno-style stereo chorus (I, II, I+II).
- Fixed (compile-time) or run-time sample rate.
- libm (PC) or fastmath (FM-1), builds without a C library.

### VA engine
- 4 voices, poly or mono legato with glide, voice stealing, load shedding.
- 40 normalised parameters, 4 presets (INIT, BASS, PAD, LEAD).
- Split into small files (api, params, presets, voices, render); audio bit-identical to the first version.

### Live core
- MIDI parser (running status, USB-MIDI), CC mapping.
- Timestamped SPSC event queue.

### FM-1
- App on fm1-base: velocity keys, encoders, presets, octave, oscilloscope (~11 ms window), CPU load, save,
  MIDI in USB/TRS, MIDI out USB, voice shedding. Code split into `src/`.
- Measured on the device: 4 % idle, ~5 % per voice, 26 % worst case.
- Manual and landing page for the fm1-base site; screenshots generated by a simulator script.

### Computer
- Desktop app for Windows / Linux / macOS (miniaudio; WinMM and ALSA MIDI; computer keyboard; offline render).
- C API with a tested example; PC simulator (demo, sweep, benchmark).
- Daisy Seed and Teensy templates (untested).

### Tooling
- 11 automatic tests; CI building the release firmware, browser emulator, web-installer site, desktop apps and
  releases (nightly, preview, tags).
- Automatic Windows install (WSL, build, Python 3.12, flash), WSL scripts, `scripts/update-prebuilt.sh`.
- `prebuilt/` with the firmware, installer, desktop apps and site.
- Documentation: 16 documents in English (`docs/_EN`) and French (`docs/_FR`).
