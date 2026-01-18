

#include <stdint.h>
#include "headers.h"
#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

/* Producer side */
int ring_buffer_push(ring_buffer_t *rb, const sample_t *s)
{
    uint32_t next_head = (rb->head + 1) % RING_BUFFER_SAMPLES;

    /* Buffer full condition */
    if (next_head == rb->tail)
    {
        return -1; // overflow
    }

    rb->buffer[rb->head] = *s;
    rb->head = next_head;
    return 0;
}

/* Consumer side */
int ring_buffer_pop(ring_buffer_t *rb, sample_t *s)
{
    if (rb->head == rb->tail)
    {
        return -1;
    }

    *s = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUFFER_SAMPLES;
    return 0;
}

uint32_t ring_buffer_count(const ring_buffer_t *rb)
{
    if (rb->head >= rb->tail)
        return rb->head - rb->tail;
    else
        return RING_BUFFER_SAMPLES - (rb->tail - rb->head);
}
