/*
 * OpenVALive - Live Core
 * midi_in.h : byte-by-byte MIDI parser (DIN/UART, unpacked USB-MIDI).
 *
 * Handles running status. Ignores SysEx and system common messages.
 * Notes, CCs, pitch bend and channel pressure go to the target engine;
 * program changes to an optional callback.
 * Real-time bytes (clock, start, stop...) are passed to an optional callback:
 * the Live Core's clock follows them (ols_live_midi_realtime). No hardware dependency: the platform pushes
 * the bytes it receives.
 */
#ifndef OLS_MIDI_IN_H
#define OLS_MIDI_IN_H

#include <stdint.h>
#include "../engines/engine.h"

#define MIDI_CC_MAP_SIZE 128
#define MIDI_CC_UNMAPPED 0xFFFF
#define MIDI_OMNI        0xFF   /* every channel */
#define MIDI_NONE        0xFE   /* no channel message at all: only the clock (a drum machine's notes ignored) */

typedef struct {
    ols_engine_t *target;                 /* engine receiving the notes */
    uint8_t channel;                      /* 0..15, or MIDI_OMNI */
    uint16_t cc_map[MIDI_CC_MAP_SIZE];    /* CC number -> engine parameter id */
    float bend_range;                     /* semitones */

    /* parser state */
    uint8_t status;
    uint8_t data[2];
    uint8_t count;
    uint8_t in_sysex;

    /* optional: 0xF8 clock, 0xFA start, 0xFB continue, 0xFC stop */
    void (*on_realtime)(void *ctx, uint8_t byte);
    void *realtime_ctx;

    /* optional: program change (the Live Core recalls a scene) */
    void (*on_program)(void *ctx, uint8_t program);
    void *program_ctx;
} midi_in_t;

void midi_in_init(midi_in_t *m, ols_engine_t *target, uint8_t channel);
void midi_in_map_cc(midi_in_t *m, uint8_t cc, uint16_t param_id);
void midi_in_byte(midi_in_t *m, uint8_t byte);

/* Program changes go to fn(ctx, program). */
void midi_in_on_program(midi_in_t *m, void (*fn)(void *ctx, uint8_t program), void *ctx);

/* Real-time messages (clock, start, stop) go to fn(ctx, byte). */
void midi_in_on_realtime(midi_in_t *m, void (*fn)(void *ctx, uint8_t byte), void *ctx);

/* USB-MIDI event packet: 4 bytes (cable/CIN, then up to 3 MIDI bytes). */
void midi_in_usb_packet(midi_in_t *m, const uint8_t packet[4]);

#endif /* OLS_MIDI_IN_H */
