/*
 * OpenVALive desktop - lock-free single-producer / single-consumer byte ring.
 *
 * Carries raw MIDI bytes from the MIDI thread (producer) to the audio
 * thread (consumer). On desktop CPUs the two threads run on different cores,
 * so a full hardware memory barrier is used.
 */
#ifndef OLS_BYTE_RING_H
#define OLS_BYTE_RING_H

#include <stdint.h>

#define BYTE_RING_SIZE 1024u   /* power of two */

typedef struct {
    uint8_t buf[BYTE_RING_SIZE];
    volatile uint32_t head;    /* written by the producer */
    volatile uint32_t tail;    /* written by the consumer */
} byte_ring_t;

static inline int byte_ring_push(byte_ring_t *r, uint8_t byte)
{
    uint32_t head = r->head;
    if (head - r->tail >= BYTE_RING_SIZE) {
        return 0;              /* full: byte dropped */
    }
    r->buf[head & (BYTE_RING_SIZE - 1u)] = byte;
    __sync_synchronize();
    r->head = head + 1u;
    return 1;
}

static inline int byte_ring_pop(byte_ring_t *r, uint8_t *byte)
{
    uint32_t tail = r->tail;
    if (tail == r->head) {
        return 0;
    }
    __sync_synchronize();
    *byte = r->buf[tail & (BYTE_RING_SIZE - 1u)];
    __sync_synchronize();
    r->tail = tail + 1u;
    return 1;
}

#endif
