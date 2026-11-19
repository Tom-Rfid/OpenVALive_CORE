/*
 * OpenVALive - Live Core
 * midi_in.c : MIDI parser implementation, see midi_in.h.
 */
#include "midi_in.h"

void midi_in_init(midi_in_t *m, ols_engine_t *target, uint8_t channel)
{
    m->target = target;
    m->channel = channel;
    m->bend_range = 2.0f;
    m->status = 0;
    m->count = 0;
    m->in_sysex = 0;
    m->on_realtime = 0;
    m->realtime_ctx = 0;
    m->on_program = 0;
    m->program_ctx = 0;
    for (int i = 0; i < MIDI_CC_MAP_SIZE; i++) {
        m->cc_map[i] = MIDI_CC_UNMAPPED;
    }
}

void midi_in_map_cc(midi_in_t *m, uint8_t cc, uint16_t param_id)
{
    if (cc < MIDI_CC_MAP_SIZE) {
        m->cc_map[cc] = param_id;
    }
}

void midi_in_on_program(midi_in_t *m, void (*fn)(void *ctx, uint8_t program), void *ctx)
{
    m->program_ctx = ctx;
    m->on_program = fn;
}

void midi_in_on_realtime(midi_in_t *m, void (*fn)(void *ctx, uint8_t byte), void *ctx)
{
    m->realtime_ctx = ctx;
    m->on_realtime = fn;
}

/* Number of data bytes that follow a channel status byte. */
static uint8_t data_length(uint8_t status)
{
    uint8_t type = status & 0xF0;
    if (type == 0xC0 || type == 0xD0) {   /* program change, channel pressure */
        return 1;
    }
    return 2;
}

static void handle_control_change(midi_in_t *m, uint8_t cc, uint8_t value)
{
    const ols_engine_api_t *api = m->target->api;
    void *ctx = m->target->ctx;

    if (cc == 120 || cc == 123) {         /* all sound off / all notes off */
        api->all_notes_off(ctx);
        return;
    }
    if (m->cc_map[cc] != MIDI_CC_UNMAPPED) {
        api->set_param(ctx, m->cc_map[cc], (float)value * (1.0f / 127.0f));
    }
}

static void handle_pitch_bend(midi_in_t *m, uint8_t lsb, uint8_t msb)
{
    const ols_engine_api_t *api = m->target->api;
    if (api->pitch_bend == 0) {
        return;
    }
    int value = ((int)msb << 7 | lsb) - 8192;          /* -8192 .. +8191 */
    api->pitch_bend(m->target->ctx, (float)value * (m->bend_range / 8192.0f));
}

/* A complete channel message has arrived: send it to the engine. */
static void dispatch(midi_in_t *m)
{
    uint8_t type = m->status & 0xF0;
    uint8_t channel = m->status & 0x0F;

    if (m->channel != MIDI_OMNI && channel != m->channel) {
        return;
    }
    if (m->target == 0 || m->target->api == 0) {
        return;
    }

    const ols_engine_api_t *api = m->target->api;
    void *ctx = m->target->ctx;

    switch (type) {
    case 0x90:
        if (m->data[1] > 0) {
            api->note_on(ctx, m->data[0], m->data[1]);
        } else {
            api->note_off(ctx, m->data[0]);   /* note on with velocity 0 */
        }
        break;
    case 0x80:
        api->note_off(ctx, m->data[0]);
        break;
    case 0xB0:
        handle_control_change(m, m->data[0], m->data[1]);
        break;
    case 0xE0:
        handle_pitch_bend(m, m->data[0], m->data[1]);
        break;
    case 0xD0:                               /* channel pressure (aftertouch) */
        if (api->pressure != 0) {
            api->pressure(ctx, (float)m->data[0] * (1.0f / 127.0f));
        }
        break;
    case 0xC0:
        if (m->on_program != 0) {
            m->on_program(m->program_ctx, m->data[0]);
        }
        break;
    default:
        break;                               /* polyphonic aftertouch: not used */
    }
}

void midi_in_byte(midi_in_t *m, uint8_t byte)
{
    /* real-time messages may appear anywhere, even inside another message */
    if (byte >= 0xF8) {
        if (m->on_realtime != 0) {
            m->on_realtime(m->realtime_ctx, byte);
        }
        return;
    }

    /* SysEx: skipped entirely */
    if (byte == 0xF0) {
        m->in_sysex = 1;
        return;
    }
    if (byte == 0xF7) {
        m->in_sysex = 0;
        return;
    }
    if (m->in_sysex) {
        return;
    }

    /* status byte */
    if (byte & 0x80) {
        if (byte >= 0xF0) {
            m->status = 0;                   /* system common: ignored, cancels running status */
        } else {
            m->status = byte;
        }
        m->count = 0;
        return;
    }

    /* data byte */
    if (m->status == 0) {
        return;                              /* data without a status: ignore */
    }
    m->data[m->count] = byte;
    m->count++;
    if (m->count >= data_length(m->status)) {
        dispatch(m);
        m->count = 0;                        /* keep the status: running status */
    }
}

void midi_in_usb_packet(midi_in_t *m, const uint8_t packet[4])
{
    uint8_t cin = packet[0] & 0x0F;          /* code index number */
    int length;

    switch (cin) {
    case 0x8: case 0x9: case 0xA: case 0xB: case 0xE:
        length = 3;
        break;
    case 0xC: case 0xD:
        length = 2;
        break;
    case 0xF:
        length = 1;                          /* single byte (real-time) */
        break;
    default:
        return;                              /* SysEx and others: ignored */
    }

    for (int i = 0; i < length; i++) {
        midi_in_byte(m, packet[1 + i]);
    }
}
