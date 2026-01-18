#ifndef HEADERS_H
#define HEADERS_H

#include <stdint.h>

typedef enum
{
    SENSOR_PT100 = 1,
    SENSOR_SPI_ADC,
    SENSOR_PL_ADC,
    SENSOR_GPIO_BTN,
    SENSOR_GPIO_SW
} sensor_id_t;

/**
sensorId   : 1 byte
padding    : 7 bytes
timestamp  : 8 bytes
value      : 4 bytes
padding    : 4 bytes
-------------------
TOTAL      : 24 bytes
**/

typedef struct
{
    uint8_t sensorId;
    uint64_t timestampUs; // think about reducing this,
    int32_t value;
} __attribute__((packed)) sample_t; // __attribute__((packed)) sample_t, compliers may insert padding, affects performance.

typedef struct {
    int64_t min_jitter_us;
    int64_t max_jitter_us;
    int64_t sum_jitter_us;
    uint64_t samples;
    uint64_t missed_deadlines;
} __attribute__((packed)) jitter_stats_t;

#endif
