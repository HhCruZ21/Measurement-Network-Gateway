#ifndef THREADS_H
#define THREADS_H

#define BASE_TICK_US 100U

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

void *snsrThrdFunc(void *arg);
void *ntwrkThrdFunc(void *arg);
#endif
