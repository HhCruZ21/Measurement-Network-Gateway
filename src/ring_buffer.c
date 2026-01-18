

#include <stdint.h>
#include "headers.h"
#include "ring_buffer.h"

void ringBufferInit(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

/* Producer side */
int ringBufferPush(ring_buffer_t *rb, const sample_t *s)
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
int ringBufferPop(ring_buffer_t *rb, sample_t *s)
{
    if (rb->head == rb->tail)
    {
        return -1;
    }

    *s = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RING_BUFFER_SAMPLES;
    return 0;
}

uint32_t ringBufferCount(const ring_buffer_t *rb)
{
    if (rb->head >= rb->tail)
        return rb->head - rb->tail;
    else
        return RING_BUFFER_SAMPLES - (rb->tail - rb->head);
}
