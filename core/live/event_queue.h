/*
 * OpenVALive - Live Core
 * event_queue.h : lock-free single-producer / single-consumer event queue.
 *
 * This is the only channel between the user interface (main loop: keys,
 * encoders, later the sequencer) and the audio engine (audio interrupt or
 * audio thread). The UI NEVER touches the engine directly: it pushes events,
 * and the audio side applies them at the start of each block.
 *
 * Every event carries a timestamp in samples (`at`, for logs and future
 * use). UI events are applied at the start of the next audio block (5.8 ms
 * on the FM-1); the sequencer's own events land on their exact sample
 * (live.c splits the block at every tick).
 *
 * Safe on a single core where the consumer is an interrupt that pre-empts
 * the producer: data is written before the index (compiler barrier).
 * With two cores, OLS_BARRIER must become a hardware memory barrier
 * (csync on pi32v2, __DMB() on ARM).
 */
#ifndef OLS_EVENT_QUEUE_H
#define OLS_EVENT_QUEUE_H

#include <stdint.h>

#ifndef OLS_EVQ_SIZE
#define OLS_EVQ_SIZE 128u   /* must be a power of two */
#endif

#ifndef OLS_BARRIER
#define OLS_BARRIER() __asm__ volatile("" ::: "memory")
#endif

typedef enum {
    EV_NOTE_ON = 1,    /* a = note, b = velocity */
    EV_NOTE_OFF,       /* a = note */
    EV_PARAM,          /* id = parameter, v = value 0..1 */
    EV_ALL_OFF,
    EV_PRESET,         /* a = preset number */
    EV_BEND,           /* v = semitones */
    EV_LIVE,           /* a Live Core command: a = ols_live_cmd_t, b / id / v / target = its arguments */
    EV_PRESSURE,       /* v = key pressure 0..1 (aftertouch) */
    EV_CLIP            /* a host's clips (app/src/clips.c): a = what, b = slot, target = the line */
} ols_ev_type_t;

typedef struct {
    uint32_t at;       /* timestamp, in samples of the audio clock */
    uint8_t type;      /* ols_ev_type_t */
    uint8_t target;    /* track (0xFF: the selected track) */
    uint8_t a, b;
    uint16_t id;
    uint16_t reserved;
    float v;
} ols_event_t;

typedef struct {
    ols_event_t buf[OLS_EVQ_SIZE];
    volatile uint32_t head;      /* written by the producer only */
    volatile uint32_t tail;      /* written by the consumer only */
    volatile uint32_t dropped;   /* events lost because the queue was full */
} ols_evq_t;

static inline void evq_init(ols_evq_t *q)
{
    q->head = 0;
    q->tail = 0;
    q->dropped = 0;
}

/* Producer side. Returns 0 if the queue is full (the event is counted as dropped). */
static inline int evq_push(ols_evq_t *q, const ols_event_t *e)
{
    uint32_t head = q->head;
    if (head - q->tail >= OLS_EVQ_SIZE) {
        q->dropped++;
        return 0;
    }
    q->buf[head & (OLS_EVQ_SIZE - 1u)] = *e;
    OLS_BARRIER();                 /* the event must be written before it is published */
    q->head = head + 1u;
    return 1;
}

/* Consumer side. Returns 0 if the queue is empty. */
static inline int evq_pop(ols_evq_t *q, ols_event_t *e)
{
    uint32_t tail = q->tail;
    if (tail == q->head) {
        return 0;
    }
    OLS_BARRIER();
    *e = q->buf[tail & (OLS_EVQ_SIZE - 1u)];
    OLS_BARRIER();                 /* read the event before freeing its slot */
    q->tail = tail + 1u;
    return 1;
}

#endif /* OLS_EVENT_QUEUE_H */
