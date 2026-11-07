#ifndef LPS22HB_H
#define LPS22HB_H
#include "sensor_bus.h"
#define LPS22HB_ADDRESS_LOW 0x5CU
#define LPS22HB_ADDRESS_HIGH 0x5DU
#define LPS22HB_WHO_AM_I 0x0FU
#define LPS22HB_ID 0xB1U
#define LPS22HB_CTRL1 0x10U
#define LPS22HB_CTRL2 0x11U
#define LPS22HB_PRESS_OUT 0x28U
typedef struct { SensorBus bus; uint8_t address; } LPS22HB;
int LPS22HB_Init(LPS22HB *sensor, const SensorBus *bus);
int LPS22HB_ReadRegisters(LPS22HB *sensor, uint8_t reg, uint8_t *data, uint16_t length);
int LPS22HB_WriteRegisters(LPS22HB *sensor, uint8_t reg, const uint8_t *data, uint16_t length);
int LPS22HB_ReadPressure(LPS22HB *sensor, float *pressure);
float LPS22HB_RelativeHeight(float pressure, float reference_pressure);
#endif
