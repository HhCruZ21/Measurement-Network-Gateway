#include <stdio.h>
#include <pthread.h>

#include "headers.h"
#include "threads.h"
#include "ring_buffer.h"

int main()
{
    pthread_t snsr_thrd, ntwrk_thrd;
    ring_buffer_t rb;

    ringBufferInit(&rb);

    pthread_create(&snsr_thrd, NULL, snsrThrdFunc, &rb);
    pthread_create(&ntwrk_thrd, NULL, ntwrkThrdFunc, &rb);

    pthread_join(snsr_thrd, NULL);
    pthread_join(ntwrk_thrd, NULL);

    return 0;
}