#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "headers.h"
#include <stdint.h>

#define RING_BUFFER_BYTES 1024
#define SAMPLE_SIZE_BYTES sizeof(sample_t)
#define RING_BUFFER_SAMPLES (RING_BUFFER_BYTES / SAMPLE_SIZE_BYTES)

typedef struct
{
    sample_t buffer[RING_BUFFER_SAMPLES];
    uint32_t head; // write index (producer)
    uint32_t tail; // read index (consumer)
} ring_buffer_t;

/* Initialize ring buffer */
void ringBufferInit(ring_buffer_t *rb);

//  Producer: push one sample
int ringBufferPush(ring_buffer_t *rb, const sample_t *s);

// Consumer: pop one sample
int ringBufferPop(ring_buffer_t *rb, sample_t *s);

uint32_t ringBufferCount(const ring_buffer_t *rb);

#endif