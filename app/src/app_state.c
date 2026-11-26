/*
 * OpenVALive - FM-1 app: shared state.
 *
 * Part of the app's unity build: app.c includes every file of src/ in order,
 * so these "static" variables are visible to all of them.
 *
 * Ownership rule:
 *   - the AUDIO side (app_render, in the audio interrupt) owns the engines
 *     and the Live Core (`live`);
 *   - the UI side (app_frame, in the main loop) owns `set` and `ui`;
 *   - the UI talks to the audio side only through the event queue `evq`,
 *     and only READS the Live Core to draw it.
 */

/* ------------------------------------------------------------ engines and Live Core */

#define TRACKS OLS_TRACKS                      /* 4 tracks; each owns a VA engine and a drum engine */
#ifdef APP_DELAY_LEN
#define DELAY_LEN APP_DELAY_LEN                 /* a target short of RAM: shorter (the longest delay times clamp) */
#else
#define DELAY_LEN 16384u                       /* 371 ms of int16 samples = 32 KB per track, in the pool */
#endif

static int16_t delay_buf[TRACKS][DELAY_LEN] PLAT_POOL;
static engine_va_t va[TRACKS] PLAT_POOL;
static engine_drum_t drum[TRACKS] PLAT_POOL;
static ols_slot_t slots[TRACKS];               /* which engines each track can switch between (bank.h) */
static ols_live_t live PLAT_POOL;
static midi_in_t midi;
static ols_evq_t evq;

static float buf_l[APP_BLOCK];
static float buf_r[APP_BLOCK];
static float gain_now;                          /* smoothed MASTER gain */

/* the built-in speaker cannot play deep bass: below ~130 Hz it only distorts, so it gets a low cut
 * (the headphones keep the whole sound). Written by the UI (jack switch), read by the audio. */
static volatile uint8_t on_speaker;
static dsp_svf_t speaker_cut[2];
static volatile uint32_t audio_clock;           /* samples rendered since boot */

/* oscilloscope: the left channel, one sample out of SCOPE_DECIMATE, read by the UI */
#define SCOPE_N 512u
#define SCOPE_DECIMATE 2u
static int16_t scope[SCOPE_N];
static volatile uint32_t scope_write;
static uint32_t scope_phase;                    /* decimation counter */

/* ------------------------------------------------------------ names and defaults */

/* a band ready to play: the acid (the first thing heard), bass, lead, drums (sound numbers of bank.h) */
static const uint8_t DEFAULT_SOUND[TRACKS] = { VA_PRESET_ACID, VA_PRESET_BASS, VA_PRESET_LEAD,
                                               OLS_SOUND_FIRST_DRUM + DR_KIT_808 };

/* synth voices per track: 11 at most in all, so four busy tracks stay inside the CPU */
static const uint8_t VOICE_LIMIT[TRACKS] = { 4, 1, 2, 4 };

/* ------------------------------------------------------------ A: the live line */

/* A fifth line under the four tracks. With A selected (SELECT past track 4), the keys play a sound of their own, live,
 * over the loops (not recorded; arpeggiated: ARP tapped), and the gestures (perform effects, CLEAR, UNDO / REDO, the
 * Creative Engine) act on every track; with a track selected they act on that track only. */
#define T_LIVE 0xFDu                           /* event target: the live voice */
#define T_PADS 0xFCu                           /* event target: the pads' drum kit (APP_CLIPS targets) */
#define LIVE_VOICES 3u
#define LIVE_DEFAULT_SOUND VA_PRESET_LEAD

/* the order PRESET shows the factory sounds in (their numbers, kept in saves, do not change), nine families by
 * character, the most played first in each: acid, basses, drums, short percussive tones, leads, keys and chords, soft
 * and enveloping ones, effects and transitions, the dark and harsh ones last; the USER sounds follow. */
static const uint8_t SOUND_ORDER[OLS_SOUND_COUNT] = {
    VA_PRESET_ACID, VA_PRESET_SQUELCH, VA_PRESET_ACID_SQUARE, VA_PRESET_ACID_DEEP, VA_PRESET_ACID_SCREAM,
    VA_PRESET_TALK,
    VA_PRESET_BASS, VA_PRESET_SUB, VA_PRESET_REESE, VA_PRESET_PSY_BASS, VA_PRESET_DEEP_SUB, VA_PRESET_BOOM,
    VA_PRESET_RUBBER, VA_PRESET_WOBBLE,
    OLS_SOUND_FIRST_DRUM + DR_KIT_808, OLS_SOUND_FIRST_DRUM + DR_KIT_909, OLS_SOUND_FIRST_DRUM + DR_KIT_TIGHT,
    OLS_SOUND_FIRST_DRUM + DR_KIT_LOFI, OLS_SOUND_FIRST_DRUM + DR_KIT_IRON,
    VA_PRESET_PLUCK, VA_PRESET_BLEEP, VA_PRESET_CLICK, VA_PRESET_METAL, VA_PRESET_PLATE, VA_PRESET_BLIP,
    VA_PRESET_DROPS,
    VA_PRESET_LEAD, VA_PRESET_GOA_LEAD, VA_PRESET_HOOVER, VA_PRESET_ARP, VA_PRESET_SAX, VA_PRESET_BRASS,
    VA_PRESET_BAGPIPE, VA_PRESET_INIT,
    VA_PRESET_STAB, VA_PRESET_DEEP_STAB, VA_PRESET_DUB_CHORD, VA_PRESET_POLY, VA_PRESET_ORGAN, VA_PRESET_VKEYS,
    VA_PRESET_LUTE,
    VA_PRESET_PAD, VA_PRESET_STRINGS, VA_PRESET_DEEP_PAD, VA_PRESET_STRINGS70, VA_PRESET_CHOIR, VA_PRESET_SWEEP,
    VA_PRESET_FLUTE, VA_PRESET_WHALE,
    VA_PRESET_RISER, VA_PRESET_DOWNLIFTER, VA_PRESET_NOISE, VA_PRESET_ZAP, VA_PRESET_LASER, VA_PRESET_DUB_SIREN,
    VA_PRESET_SHAKER, VA_PRESET_RAIN, VA_PRESET_THUNDER,
    VA_PRESET_DRONE, VA_PRESET_RUMBLE, VA_PRESET_WIND, VA_PRESET_HOLD, VA_PRESET_CREAK, VA_PRESET_GRUNGE,
    VA_PRESET_SCREECH,
};
/* the families, by their first place in SOUND_ORDER: F1 acid, F2 bass, F3 drums, F4 perc (short tones: plucks, bleeps,
 * metal, drops), F5 lead (and INIT), F6 keys (stabs, chords, organ, keys), F7 soft (pads, strings, choir), F8 FX
 * (transitions, zaps, weather), F9 dark (drones, rumbles, creaks, the harsh ones); the USER sounds last ("US"). 9 sounds
 * a family at most. OCT- / OCT+ while a sound is chosen jump between them. */
static const uint8_t FAMILY_AT[] = { 0, 6, 14, 19, 26, 34, 41, 49, 58 };
#define FAMILIES 10                             /* 9 factory families, then USER */
typedef char sound_order_complete[(OLS_SOUND_COUNT == 65) ? 1 : -1];   /* a sound added: put it in SOUND_ORDER */

static engine_va_t lv PLAT_POOL;               /* the live voice: a VA engine without delay (the RAM goes to the loops);
                                                * it renders into the Live Core's chunk buffers, free once it has rendered */
static volatile float lv_gain = 1.0f;
static volatile uint8_t slice_len = 24u;       /* PERFORM SLICER: ticks a slice (24 = 1/16), OCT- / OCT+ while it runs */
static volatile uint8_t out_hold;               /* 1: the output fades to silence (before a SAVE that stops it) */
static volatile uint8_t slicer_on;              /* PERFORM SLICER: the whole mix gated in 1/16s (audio side reads it) */          /* its level (LIVE page, LEVEL with A selected) */

/* the arpeggiator of the live line: the keys held, played one at a time on the 16ths */
enum { ARP_OFF, ARP_UP, ARP_DOWN, ARP_UPDOWN, ARP_RANDOM, ARP_MODES };
static const char *const ARP_NAME[ARP_MODES] = { "OFF", "UP", "DOWN", "UP/DOWN", "RANDOM" };
static volatile uint8_t arp_mode;              /* written by the UI (one byte), read by the audio side */
#define ARP_HELD 8
static struct {
    uint8_t note[ARP_HELD], vel[ARP_HELD];      /* the keys held, in the order they went down */
    uint8_t count;
    uint8_t sounding;                           /* the note playing (0: none) */
    uint8_t pos;                                /* index into the sorted notes */
    int8_t dir;                                 /* UP/DOWN: +1 / -1 */
    uint8_t gate_open;
    float phase;                                /* 0..1 across a 16th */
    uint32_t rng;
} arp;

/* A plays synth sounds: the factory ones and the user sounds (a user sound that holds drums is skipped when chosen) */
static int live_sound_ok(uint32_t sound)
{
    return sound < OLS_SOUND_FIRST_DRUM || (sound >= OLS_SOUND_FIRST_USER && sound < OLS_SOUND_TOTAL);
}

/* colours: the logo's, for what is selected (violet: a track, mint: A) and the pages */
#define C_VIO RGB(150, 120, 255)
#define C_VIO_BG RGB(44, 26, 104)
#define C_MINT RGB(90, 200, 160)
#define C_MINT_BG RGB(16, 60, 48)
#define C_ORANGE RGB(238, 90, 36)
#define C_PINK RGB(255, 120, 190)
#define C_GREEN RGB(60, 210, 90)
#define C_YELLOW RGB(255, 214, 40)
#define C_PINK_BG RGB(120, 36, 86)                /* a track line being recorded */
#define C_CYAN RGB(60, 200, 235)
#define C_BAR_BG RGB(96, 96, 106)
#define C_RED RGB(235, 40, 40)
#define C_VALUE RGB(130, 118, 178)               /* the values over their bars: grey violet, one colour
                                                  * (two colours cut a number in two as it changes) */
enum { MSG_PLAIN, MSG_ARMED, MSG_TAKEN, MSG_NOT_TAKEN, MSG_PRESS };   /* press: "REC (arrow down) PLAY FIRST" in green */   /* armed: "LET GO .." green; taken: mint; not: orange */
#define C_DARK_ORANGE RGB(200, 96, 20)
#define C_LOOP_BG RGB(110, 18, 18)              /* a track whose loop (stutter) runs */

/* ------------------------------------------------------------ knob pages */

/* SEL cycles the pages of the four encoders K1..K4. */
enum { PAGE_SOUND, PAGE_MACRO, PAGE_LIVE, PAGE_SETUP, PAGE_USER, PAGE_DRUM, PAGE_EQ, PAGE_COUNT };   /* DRUM EQ: drum
                                                 * lines only; EQ: the master tone (numbers kept: new ones last) */
static const char *const PAGE_NAME[PAGE_COUNT] = { "SOUND", "MACRO", "LIVE", "SETUP", "USER", "DRUM EQ", "EQ" };

/* MAP CONTROL and OPTION CONTROL (src/control_map.c, after the UI it serves): the mapped pages, the menus */
static int map_is_open(void);
static void map_open(void);
static void map_open_settings(void);
static void map_input(void);
static void map_draw(void);
static void map_turn(uint32_t slot, int32_t turn);
static void map_knob_value(uint32_t k, float *bar, int32_t *shown, const char **label);
static int32_t page_slot(uint32_t page, uint32_t k);
static int slot_is_default(uint32_t s);
static uint32_t page_after(uint32_t p);
static void new_sound_page(uint32_t line, int32_t sound);
static void track_key(int w);
static uint8_t rec_painting[TRACKS];             /* a take started (its first note): the line paints pink */
static void map_load(void);
static int map_save(void);
static void cap_note(uint8_t track);            /* (ui_input.c) a note played: CAPTURE's green line may start */
/* SOUND EDIT (src/sound_edit.c) */
static int sedit_is_open(void);
static void sedit_open(void);
static void sedit_input(void);
static void sedit_draw(void);

/* LOOP EDIT (src/loop_edit.c) */
static struct {
    uint8_t open, drawn, cursor, want, heard, quiet, key, sounding, sound_note, add_note, want_note;
    uint8_t chord;                              /* EDIT held made a chord: its release adds nothing more */
    uint16_t want_tick;
    uint32_t buttons, sound_off;
} ledit;
static int ledit_is_open(void);

/* SOUNDS (src/sound_fav.c): which sounds PRESET offers */
static struct {
    uint8_t open, drawn, family, row;
    uint32_t buttons;
} fav;
static int fav_is_open(void);
static void fav_open(void);
static void fav_input(void);
static void fav_draw(void);
static void ledit_open(void);
static void ledit_input(void);
static void ledit_draw(void);

/* MACRO: one knob, many parameters on every track (50 = the sound as it is) */
static const char *const MACRO_NAME[OLS_MACROS] = { "ENERGY", "SPACE", "TENSION", "DENSITY" };

/* SOUND: the four macros of the selected track's engine (synth: CUTOFF RESO ENV FX,
 * drums: TUNE DECAY CUTOFF DRIVE) */
#define KNOB_COUNT OLS_MACROS

/* controller mode: the CCs the four knobs send */
static const uint8_t CONTROLLER_CC[KNOB_COUNT] = { 74, 71, 75, 94 };

/* LIVE: performance values */
static const char *const LIVE_NAME[KNOB_COUNT] = { "MORPH", "SWING", "PUMP", "LEVEL" };

/* SETUP: preparation, not for the middle of a set */
static const char *const SETUP_NAME[KNOB_COUNT] = { "MIDI IN", "XFADE", "QUANT", "BARS" };
static const uint8_t CHAIN_BARS[3] = { 4, 8, 16 };          /* scene chain: bars per scene */
static const uint8_t XFADE_BARS[4] = { 0, 1, 2, 4 };      /* a scene change: a cut, or a cross-fade of n bars */

/* ------------------------------------------------------------ settings */

#define SET_MAGIC 0x4F4C5346u                   /* "OLSF": settings format; any other value loads defaults */
#define SET_MAGIC_VE 0x4F4C5345u                /* "OLSE": 60 sounds (the drum kits and USER 5 lower) */
#define SET_MAGIC_VD 0x4F4C5344u                /* "OLSD": the same without the USER sounds' origins */
#define SET_MAGIC_VC 0x4F4C5343u                /* "OLSC": 48 sounds (the drum kits and USER 12 lower) */
#define SET_MAGIC_VB 0x4F4C5342u                /* "OLSB": 36 sounds (the drum kits and USER 12 lower) */
#define SET_MAGIC_VA 0x4F4C5341u                /* "OLSA": 31 sounds (the drum kits and USER 5 lower), no PERC keys,
                                                 * EQ or page per line */
#define SET_MAGIC_V9 0x4F4C5339u                /* "OLS9": the same without the sounds hidden from PRESET */
#define SET_MAGIC_V8 0x4F4C5338u                /* "OLS8": the same, the drum kits and USER sounds 2 lower */
#define SET_MAGIC_V7 0x4F4C5337u                /* "OLS7": the same, the drum kits and USER sounds 7 lower */
#define SET_MAGIC_V6 0x4F4C5336u                /* "OLS6": the same without the drum keys and the keys on screen */
#define SET_MAGIC_V5 0x4F4C5335u                /* "OLS5": the same, the synth sounds numbered in their old order */
#define SET_MAGIC_V4 0x4F4C5334u                /* "OLS4": the same, before DRUMS 909 (the sounds past DRUMS 808 one lower) */
#define SET_MAGIC_V3 0x4F4C5333u                /* "OLS3": one octave for every line */
#define SET_MAGIC_V2 0x4F4C5332u                /* "OLS2" (0.0.3 and before): no live line */
#define SET_MAGIC_V1 0x4F4C5331u                /* "OLS1" (0.1): one sound, moved to track 1 */

#define PERC_KEYS 6u                            /* D#3 G#3 D#4 G#4 D#5 G#5: the FM-1 has the middle four */
#define EQ_BANDS 4u

typedef struct {
    uint32_t magic;
    int8_t octave[TRACKS + 1];                  /* -3 .. +3, per line: tracks 1..4, then A */
    uint8_t page;
    uint8_t preset[TRACKS];                     /* sound chosen per track (bank.h numbers) */
    uint8_t controller;                         /* 1: keys and knobs only send MIDI ("local off") */
    uint8_t midi_ch;                            /* MIDI in: 0 all channels, 1..16 one, MIDI_CH_CLOCK = clock only */
    uint8_t live_sound;                         /* A's sound (a synth sound, or a user sound) */
    uint8_t arp_mode;                           /* A's arpeggiator, ARP_* */
    uint8_t drum_map;                           /* the drum keys: DRUM_MAP_ORIGINAL or DRUM_MAP_GM */
    uint8_t drum_ds, drum_gs;                   /* the original map's D# and G#: DRUM_PERC numbers */
    uint8_t keys_screen;                        /* 1: a layer button held shows its keys in place of the scope */
    uint8_t wheel_pan;                          /* 1: a target's pitch wheel pans the selected track (APP_WHEEL) */
    uint8_t hidden[(OLS_SOUND_TOTAL + 7) / 8];  /* sounds PRESET does not offer (the SOUNDS screen); 0: all offered */
    uint8_t perc[PERC_KEYS];                    /* the original drum map's D# and G# keys, each its own (DRUM_PERC) */
    uint8_t eq[EQ_BANDS];                       /* the master EQ: BASS MID TREBLE (128 flat) and the MID's frequency */
    uint8_t new_page[TRACKS + 1];               /* a new sound taken on a line: 0 the page stays, else page + 1 */
    uint8_t pad_kit;                            /* the pads' drum kit (DRUMS pads), 0..DR_KIT_COUNT-1 */
    uint8_t user_from[OLS_USER_SOUNDS];         /* the factory sound each USER sound was made from (its name shows:
                                                 * "U3 ACID 303"), USER_FROM_NONE unknown */
    uint8_t no_help;                            /* 1: no help pages after the boot screen (HELP AT START OFF); in what
                                                 * was padding, so 0 (help on) in settings saved before it */
} settings_t;
typedef char settings_size_kept[(sizeof(settings_t) == 60u) ? 1 : -1];   /* (a field more: a new format) */
#define USER_FROM_NONE 0xFFu

/* OLSE's settings: `hidden` for 60 sounds + USER (9 bytes); OLSD's: the same without `user_from` */
typedef struct {
    uint32_t magic;
    int8_t octave[TRACKS + 1];
    uint8_t page;
    uint8_t preset[TRACKS];
    uint8_t controller, midi_ch, live_sound, arp_mode, drum_map, drum_ds, drum_gs, keys_screen, wheel_pan;
    uint8_t hidden[9];
    uint8_t perc[PERC_KEYS];
    uint8_t eq[EQ_BANDS];
    uint8_t new_page[TRACKS + 1];
    uint8_t pad_kit;
    uint8_t user_from[OLS_USER_SOUNDS];
} settings_ve_t;
typedef char settings_ve_size[(sizeof(settings_ve_t) == 56u) ? 1 : -1];
#define SETTINGS_VD_SIZE 48u                    /* OLSD: the same up to pad_kit */
#define VA_SOUNDS_VE 55u                        /* OLSE's (and OLSD's) synth sounds */

/* OLSC's settings: the same, `hidden` for 48 sounds + USER (7 bytes) */
typedef struct {
    uint32_t magic;
    int8_t octave[TRACKS + 1];
    uint8_t page;
    uint8_t preset[TRACKS];
    uint8_t controller, midi_ch, live_sound, arp_mode, drum_map, drum_ds, drum_gs, keys_screen, wheel_pan;
    uint8_t hidden[7];
    uint8_t perc[PERC_KEYS];
    uint8_t eq[EQ_BANDS];
    uint8_t new_page[TRACKS + 1];
    uint8_t pad_kit;
} settings_vc_t;
#define VA_SOUNDS_VC 43u                        /* OLSC's synth sounds */
typedef char settings_vc_size[(sizeof(settings_vc_t) == 48u) ? 1 : -1];

/* OLSA's settings: the same up to `hidden`, 31 sounds then */
typedef struct {
    uint32_t magic;
    int8_t octave[TRACKS + 1];
    uint8_t page;
    uint8_t preset[TRACKS];
    uint8_t controller, midi_ch, live_sound, arp_mode, drum_map, drum_ds, drum_gs, keys_screen, wheel_pan;
    uint8_t hidden[5];
} settings_va_t;
typedef char settings_va_size[(sizeof(settings_va_t) == 28u) ? 1 : -1];
#define VA_SOUNDS_VA 26u                        /* OLSA's synth sounds: its drum kits and USER sounds from there */

/* OLSB's settings: the same, `hidden` one byte shorter (36 sounds + USER) */
typedef struct {
    uint32_t magic;
    int8_t octave[TRACKS + 1];
    uint8_t page;
    uint8_t preset[TRACKS];
    uint8_t controller, midi_ch, live_sound, arp_mode, drum_map, drum_ds, drum_gs, keys_screen, wheel_pan;
    uint8_t hidden[6];
    uint8_t perc[PERC_KEYS];
    uint8_t eq[EQ_BANDS];
    uint8_t new_page[TRACKS + 1];
} settings_vb_t;
#define VA_SOUNDS_VB 31u                        /* OLSB's synth sounds */

/* OLS7 .. OLS9's settings: the same without `hidden` (24 bytes) */
typedef struct {
    uint32_t magic;
    int8_t octave[TRACKS + 1];
    uint8_t page;
    uint8_t preset[TRACKS];
    uint8_t controller, midi_ch, live_sound, arp_mode, drum_map, drum_ds, drum_gs, keys_screen, wheel_pan;
} settings_v9_t;
typedef char settings_v9_size[(sizeof(settings_v9_t) == 24u) ? 1 : -1];

/* OLS6's settings: the same without the last four */
typedef struct {
    uint32_t magic;
    int8_t octave[TRACKS + 1];
    uint8_t page;
    uint8_t preset[TRACKS];
    uint8_t controller, midi_ch, live_sound, arp_mode;
} settings_v6_t;

/* The drum keys (General MIDI note numbers, so a computer and MIDI OUT see the right instruments): the original map
 * is made to be played (D# and G# chosen, the toms high to low), GM's is the computers' 36..47, every octave. */
enum { DRUM_MAP_ORIGINAL, DRUM_MAP_GM };
#define DRUM_PERCS 10u
static const struct {
    uint8_t note;
    const char *name;
} DRUM_PERC[DRUM_PERCS] = {                    /* (numbers kept in the settings: new ones at the end) */
    { 56, "COWBELL" }, { 39, "CLAP" }, { 81, "TRIANGLE" }, { 37, "RIM" }, { 44, "PEDAL HAT" },
    { 67, "BELL HIGH" }, { 68, "BELL LOW" }, { 80, "TRI MUTE" }, { 54, "TAMBOURINE" }, { 75, "CLAVES" },
};
#ifndef APP_KEYS_SCREEN
#define APP_KEYS_SCREEN 0u                      /* a target without key lights (SMK-37) turns it on */
#endif

/* the buttons and knobs as the panel prints them (the screen says these): the FM-1's, or a target's own (the
 * SMK-37: SEL is NOTE, EDIT GLOBE, SAVE PARA...). In the order of plat.h's B_* */
#ifndef APP_BUTTON_NAMES
#define APP_BUTTON_NAMES { "FX", "SEL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC", \
                           "OCT-", "OCT+" }
#define APP_ALGO_NAME "ALGO"
#define APP_PRESET_NAME "PRESET"
#endif
static const char *const BTN_NAME[NB] = APP_BUTTON_NAMES;
#ifndef APP_WHEEL
#define APP_WHEEL 0                             /* 1: the target has a pitch wheel (SMK-37): FX + key 14 makes it pan */
#endif

/* OLS3's settings: one octave for every line */
typedef struct {
    uint32_t magic;
    int8_t octave;
    uint8_t page;
    uint8_t preset[TRACKS];
    uint8_t controller;
    uint8_t midi_ch;
    uint8_t live_sound;
    uint8_t arp_mode;
} settings_v3_t;

/* OLS2's settings: the same without the live line */
typedef struct {
    uint32_t magic;
    int8_t octave;
    uint8_t page;
    uint8_t preset[TRACKS];
    uint8_t controller;
    uint8_t midi_ch;
} settings_v2_t;

#define MIDI_CH_CLOCK 17u                       /* notes ignored, clock followed: a drum machine as the master */

/* 0.1's settings, read once to keep the sound of an update */
typedef struct {
    uint32_t magic;
    int8_t octave;
    uint8_t preset;
    uint8_t reserved[2];
    float p[40];                                /* 0.1 had 40 parameters */
} settings_v1_t;

static settings_t set;
static uint8_t set_dirty;                       /* changed since the last save: SAVE is lit */
static uint32_t saved_changes;                  /* live.changes at the last save */

/* the 8 user sounds (SAVE + white key 1..8 keeps the track's sound): written on the audio side,
 * saved by the UI with SAVE */
static ols_user_sound_t user_sounds[OLS_USER_SOUNDS];
/* a user sound keeps its octave in valid's high nibble (bit 0: valid): sounds kept before read octave 0 */
#define USER_VALID(oct) ((uint8_t)(1u | ((uint8_t)(oct) & 0x0Fu) << 4))
#define USER_OCTAVE(u) ((int8_t)((u)->valid & 0xF0u) >> 4)

/* the saved live set is read at boot but applied once the UI is up: holding HOME while switching on
 * skips it (safe start, if a set ever misbehaves) */
static volatile uint8_t project_pending;       /* 1: bytes in project_buf wait for the audio side */
static uint32_t project_size;
static uint8_t ui_started;                      /* app_ui_init ran once since boot (it runs again after a restore) */

/* project buffer: the live set as bytes (UI side, for saving and loading) */
static uint8_t project_buf[OLS_PROJECT_MAX + 2u] PLAT_POOL;   /* + the 2-byte size word */

/* ------------------------------------------------------------ UI */

/* what a key does while a layer button is held */
enum { KEY_NOTE, KEY_FX, KEY_NONE, KEY_ARMED_ARP, KEY_ARMED_SEQ, KEY_ARMED_GLO, KEY_ARMED_HOME, KEY_ARMED_FX,
       KEY_ARMED_SEL, KEY_ARMED_SAVE, KEY_ARMED_REC };   /* ARMED: on a track, ARP / SEQ + key only shows
                                                                         * the gesture; letting ARP / SEQ go first applies it */

/* the perform effects on the white keys while FX is held: tap = latch, hold = momentary */
#define FX_KEYS 16                              /* the perform effects on the keys */
#define FX_ALL 17                               /* ...and LOOP 1/32, reached by OCT+ from LOOP 1/16 only */
#define FX_TAP_MS 250u

static struct {
    char msg[2][24];                            /* short message over the header */
    uint8_t cap_on, cap_track;                  /* a loop take running (EDIT tapped, then the first note): the line paints
                                                 * green from that note (EDIT blinks fast, the infinity sign shows) until
                                                 * EDIT or a menu button takes it, a stop, REC or another line */
    uint8_t cap_armed;                          /* EDIT tapped while playing: the take waits for its first note */
    uint8_t edit_used;                          /* this EDIT press ended a take: nothing more on its release */
    uint8_t msg_inf;                            /* an infinity sign before the title (PERFORM: "∞ DIV / FILTER") */
    const char *msg_right;                      /* and at its right (SEL held: "A VOL"), 0 none */
    uint32_t msg_until;
    uint8_t msg_style;                          /* MSG_*: how the header message shows */
    uint8_t punch;                              /* EDIT held + a note: a punch records (EDIT let go stops it) */
    uint8_t given_up;                           /* the armed role whose key was let go first (said when its button is) */
    uint8_t given_up_w;
    uint32_t keys_conf;                         /* the keys of the held layer that set things up (shown red / slow) */
    uint8_t arp_vel;                            /* the armed ARP key's strength (its %), shown before it is taken */
    uint8_t arp_pend, arp_pend_w, arp_pend_vel; /* ARP: the last key tapped stays chosen until ARP is let go */
    uint32_t fx_flash_until;                    /* a perform effect just taken: green on the bottom line until then */
    uint8_t seq_used;                           /* SEQ held for a key or OCT: its release is not a tap */
    uint32_t keys;                              /* keys held at the last scan */
    uint32_t buttons;                           /* buttons held at the last scan */
    uint8_t key_note[NKEYS];                    /* note played by each key (the octave may change while held) */
    uint8_t key_track[NKEYS];                   /* the track it played on (the track may change while held) */
    uint8_t key_role[NKEYS];                    /* KEY_* */
    uint8_t key_fx[NKEYS];                      /* the perform effect a held key started (KEY_FX) */
    uint32_t key_down_at[NKEYS];                /* when it was pressed: a short tap latches */
    uint8_t fx_on[FX_ALL];                      /* perform effects running (held or latched) */
    uint8_t fx_latched[FX_ALL];                 /* ...and those that stay after the key is let go */
    uint8_t fx_target[FX_ALL];                 /* the track it acts on, or OLS_TRACK_ALL (A selected) */
    uint8_t on_a;                               /* A is selected (mirror.track stays the last track selected) */
    uint32_t arp_down_at;                       /* ARP went down: a tap (no key, short) changes A's arpeggio */
    uint8_t arp_used;                           /* a key was played while ARP was held */
    float lv_level;                             /* A's level */
    float shadow[TRACKS][OLS_MAX_PARAMS];       /* UI copy of the parameters */
    uint8_t cc_value[KNOB_COUNT];               /* controller mode: the last CC values sent */
    uint8_t pressure_sent;                      /* last key pressure sent (aftertouch), 0..127 */
    uint8_t metronome;
    uint8_t xfade;                              /* index in XFADE_BARS */
    float macro[KNOB_COUNT];                    /* MACRO page, 0..1 (0.5 neutral) */
    float pump;                                 /* sidechain depth */
    uint8_t chain_bars;                         /* index in CHAIN_BARS */
    uint8_t sync_after_load;                    /* the saved set is being loaded: re-read it when done */
    float morph, swing, level[TRACKS];          /* LIVE page values */
    uint32_t turned_at;                         /* last encoder turn, ms */
    uint32_t sound_at;                          /* PRESET last turned, ms: OCT- / OCT+ jump families for a while */
    uint32_t last_draw;
    uint8_t redraw;
    int16_t pend_sound;                         /* a sound turned on PRESET for a track with a loop: waits for SEL (-1: none) */
    uint8_t hover;                              /* SEL held + SELECT turned: the line pointed at (0..3, TRACKS = A) */
    uint8_t hovering;                           /* ...and it moved: SEL let go selects it (from A: swaps A with it) */
    uint8_t swap_track;                         /* the last A <-> track swap: its track + 1 (0: none) */
    uint8_t swap_undone;                        /* 1: ENV put it back (LFO swaps again) */
    uint32_t swap_mark;                         /* live.undo_top at the swap: undoable while nothing newer */
    uint8_t rec_used;                           /* REC held served a key (RESET): no record toggle on release */
    uint8_t pages_used;                         /* SEL did something else while held: no page turn on release */
    uint32_t splash_until;                      /* the boot screen stays until then, or until a hand touches something */
    uint8_t activity;                           /* something was touched since the last frame */
    uint8_t touched;                            /* a key, a pad or a button since power-on: the boot screen goes */
    uint8_t help;                               /* the help pages after the boot screen (HELP AT START) are up */
    uint8_t help_shown;                         /* the help page on the screen + 1 (0: none drawn yet) */
    uint32_t help_since;                        /* when they came up: a page every 3 s */
    uint8_t help_sel;                           /* SEL turned them off for good: that press does nothing more till let go */
} ui;

/* the MASTER position in %, for the header: fm1-base's pot, or a target's own (the SMK-37's encoder) */
#ifndef APP_MASTER_PCT
#define APP_MASTER_PCT() ((plat_master() * 100u + 2048u) / 4096u)
#endif
