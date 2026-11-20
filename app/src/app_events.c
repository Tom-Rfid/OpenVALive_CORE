/*
 * OpenVALive - FM-1 app: sending events from the UI to the audio side.
 * Every change of sound, loop or transport goes through here.
 */

static void push_event(uint8_t type, uint8_t target, uint8_t a, uint8_t b, uint16_t id, float v)
{
    ols_event_t e;
    e.at = audio_clock;
    e.type = type;
    e.target = target;
    e.a = a;
    e.b = b;
    e.id = id;
    e.reserved = 0;
    e.v = v;
    evq_push(&evq, &e);
}

static void send_note_on(uint8_t track, uint8_t note, uint8_t velocity)
{
    if (ledit.open && !ledit.quiet) {           /* LOOP EDIT: a key played gives the note its pitch */
        ledit.key = note;
        ledit.heard = 1;
    }
    push_event(EV_NOTE_ON, track, note, velocity, 0, 0.0f);
    cap_note(track);
}

static void send_note_off(uint8_t track, uint8_t note)
{
    push_event(EV_NOTE_OFF, track, note, 0, 0, 0.0f);
}

static void send_param(uint8_t track, uint16_t id, float value)
{
    push_event(EV_PARAM, track, 0, 0, id, value);
}

static void send_preset(uint8_t track, uint8_t preset)
{
    push_event(EV_PRESET, track, preset, 0, 0, 0.0f);
}

static void send_all_off(void)
{
    push_event(EV_ALL_OFF, 0, 0, 0, 0, 0.0f);
}

/* A Live Core command (live.h, ols_live_cmd_t). */
static void send_live(ols_live_cmd_t cmd, uint8_t b, uint16_t id, float v, uint8_t target)
{
    push_event(EV_LIVE, target, (uint8_t)cmd, b, id, v);
}

/* USB-MIDI out of the keys and knobs: the FM-1 has no MIDI OUT jack, it all goes out over USB,
 * on the channel of the track (track 1 = channel 1...). */
static void midi_out_note_on(uint32_t track, uint32_t note, uint32_t velocity)
{
    plat_midi_send(0x09u | (0x90u | track) << 8 | note << 16 | velocity << 24, TO_USB);
}

static void midi_out_note_off(uint32_t track, uint32_t note)
{
    plat_midi_send(0x08u | (0x80u | track) << 8 | note << 16, TO_USB);
}

static void midi_out_cc(uint32_t track, uint32_t cc, uint32_t value)
{
    plat_midi_send(0x0Bu | (0xB0u | track) << 8 | cc << 16 | value << 24, TO_USB);
}

static void midi_out_pressure(uint32_t track, uint32_t value)
{
    plat_midi_send(0x0Du | (0xD0u | track) << 8 | value << 16, TO_USB);
}
