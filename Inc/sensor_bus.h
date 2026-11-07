#ifndef SENSOR_BUS_H
#define SENSOR_BUS_H
#include <stdint.h>
/* Addresses are unshifted 7-bit I2C addresses. Zero means success. */
typedef struct {
    int (*read)(uint8_t address, uint8_t reg, uint8_t *data, uint16_t length);
    int (*write)(uint8_t address, uint8_t reg, const uint8_t *data, uint16_t length);
    void (*delay_ms)(uint32_t ms);
} SensorBus;
#endif
