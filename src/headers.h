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

#endif
