#include "lps22hb.h"
#include <math.h>
int LPS22HB_ReadRegisters(LPS22HB *s, uint8_t reg, uint8_t *data, uint16_t n)
{ return s->bus.read(s->address, reg, data, n); }
int LPS22HB_WriteRegisters(LPS22HB *s, uint8_t reg, const uint8_t *data, uint16_t n)
{ return s->bus.write(s->address, reg, data, n); }
int LPS22HB_Init(LPS22HB *s, const SensorBus *bus)
{
    uint8_t id, value;
    if (!s || !bus || !bus->read || !bus->write || !bus->delay_ms) return -1;
    s->bus = *bus;
    for (s->address = LPS22HB_ADDRESS_LOW; s->address <= LPS22HB_ADDRESS_HIGH; ++s->address)
        if (!LPS22HB_ReadRegisters(s, LPS22HB_WHO_AM_I, &id, 1) && id == LPS22HB_ID) break;
    if (s->address > LPS22HB_ADDRESS_HIGH) return -2;
    value = 0; /* ODR=0, BDU=0: one-shot; burst read only after completion. */
    if (LPS22HB_WriteRegisters(s, LPS22HB_CTRL1, &value, 1)) return -3;
    value = 0x10; /* Address increment, no FIFO. */
    return LPS22HB_WriteRegisters(s, LPS22HB_CTRL2, &value, 1);
}
int LPS22HB_ReadPressure(LPS22HB *s, float *pressure)
{
    uint8_t value = 0x11, raw[3];
    if (!pressure || LPS22HB_WriteRegisters(s, LPS22HB_CTRL2, &value, 1)) return -1;
    for (unsigned i = 0; i < 500; ++i) {
        s->bus.delay_ms(1);
        if (LPS22HB_ReadRegisters(s, LPS22HB_CTRL2, &value, 1)) return -1;
        if (!(value & 1U)) {
            if (LPS22HB_ReadRegisters(s, LPS22HB_PRESS_OUT, raw, 3)) return -1;
            int32_t p = raw[0] | ((int32_t)raw[1] << 8) | ((int32_t)raw[2] << 16);
            if (p & 0x800000) p -= 0x1000000;
            *pressure = p / 4096.0f;
            return (*pressure >= 260 && *pressure <= 1260) ? 0 : -3;
        }
    }
    return -2;
}
float LPS22HB_RelativeHeight(float pressure, float reference_pressure)
{
    if (pressure <= 0 || reference_pressure <= 0) return NAN;
    return 44330.0f * (1.0f - powf(pressure / reference_pressure, 0.19029495f));
}
