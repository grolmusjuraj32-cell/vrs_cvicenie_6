#ifndef HTS221_H
#define HTS221_H
#include "sensor_bus.h"
#define HTS221_ADDRESS 0x5FU
#define HTS221_WHO_AM_I 0x0FU
#define HTS221_ID 0xBCU
#define HTS221_AV_CONF 0x10U
#define HTS221_CTRL1 0x20U
#define HTS221_CTRL2 0x21U
#define HTS221_STATUS 0x27U
#define HTS221_HUMIDITY_OUT 0x28U
#define HTS221_TEMP_OUT 0x2AU
#define HTS221_CALIBRATION 0x30U
typedef struct {
    SensorBus bus;
    float h0, h1, t0, t1;
    int16_t h0_out, h1_out, t0_out, t1_out;
} HTS221;
int HTS221_Init(HTS221 *sensor, const SensorBus *bus);
int HTS221_ReadRegisters(HTS221 *sensor, uint8_t reg, uint8_t *data, uint16_t length);
int HTS221_WriteRegisters(HTS221 *sensor, uint8_t reg, const uint8_t *data, uint16_t length);
int HTS221_Measure(HTS221 *sensor);
int HTS221_ReadTemperature(HTS221 *sensor, float *temperature);
int HTS221_ReadHumidity(HTS221 *sensor, float *humidity);
#endif
