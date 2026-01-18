#include <stdio.h>
#include <pthread.h>

#include "../include/headers.h"
#include "../include/threads.h"
#include "../include/ring_buffer.h"

int main()
{
    printf("[MAIN] Starting sensor network application...\n");

    pthread_t snsr_thrd, ntwrk_thrd;
    ring_buffer_t rb;

    ringBufferInit(&rb);

    int r1 = pthread_create(&snsr_thrd, NULL, snsrThrdFunc, &rb);
    printf("pthread_create sensor = %d\n", r1);

    int r2 = pthread_create(&ntwrk_thrd, NULL, ntwrkThrdFunc, &rb);
    printf("pthread_create sensor r2 = %d\n", r2);

    pthread_join(snsr_thrd, NULL);
    pthread_join(ntwrk_thrd, NULL);

    return 0;
}