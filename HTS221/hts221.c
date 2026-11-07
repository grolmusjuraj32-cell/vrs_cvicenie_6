#include "hts221.h"
static int16_t signed16(const uint8_t *p)
{
    uint16_t u = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
    return (int16_t)((u & 0x8000U) ? (int32_t)u - 65536 : u);
}
int HTS221_ReadRegisters(HTS221 *s, uint8_t reg, uint8_t *data, uint16_t n)
{
    return s->bus.read(HTS221_ADDRESS, reg | (n > 1 ? 0x80U : 0U), data, n);
}
int HTS221_WriteRegisters(HTS221 *s, uint8_t reg, const uint8_t *data, uint16_t n)
{
    return s->bus.write(HTS221_ADDRESS, reg | (n > 1 ? 0x80U : 0U), data, n);
}
int HTS221_Init(HTS221 *s, const SensorBus *bus)
{
    uint8_t id, c[16], value;
    if (!s || !bus || !bus->read || !bus->write || !bus->delay_ms) return -1;
    s->bus = *bus;
    if (HTS221_ReadRegisters(s, HTS221_WHO_AM_I, &id, 1) || id != HTS221_ID) return -2;
    if (HTS221_ReadRegisters(s, HTS221_CALIBRATION, c, sizeof(c))) return -3;
    s->h0 = c[0] / 2.0f; s->h1 = c[1] / 2.0f;
    s->t0 = (c[2] | ((c[5] & 3U) << 8)) / 8.0f;
    s->t1 = (c[3] | ((c[5] & 12U) << 6)) / 8.0f;
    s->h0_out = signed16(c + 6); s->h1_out = signed16(c + 10);
    s->t0_out = signed16(c + 12); s->t1_out = signed16(c + 14);
    if (s->h0_out == s->h1_out || s->t0_out == s->t1_out) return -4;
    value = 0x1B; /* Average 16 temperature / 32 humidity samples. */
    if (HTS221_WriteRegisters(s, HTS221_AV_CONF, &value, 1)) return -3;
    value = 0; /* Heater off. */
    if (HTS221_WriteRegisters(s, HTS221_CTRL2, &value, 1)) return -3;
    value = 0x84; /* Active, BDU, one-shot mode. */
    return HTS221_WriteRegisters(s, HTS221_CTRL1, &value, 1);
}
int HTS221_Measure(HTS221 *s)
{
    uint8_t value = 1, status;
    if (HTS221_WriteRegisters(s, HTS221_CTRL2, &value, 1)) return -1;
    for (unsigned i = 0; i < 500; ++i) {
        s->bus.delay_ms(1);
        if (HTS221_ReadRegisters(s, HTS221_CTRL2, &value, 1)) return -1;
        if (!(value & 1U)) {
            if (HTS221_ReadRegisters(s, HTS221_STATUS, &status, 1)) return -1;
            if ((status & 3U) == 3U) return 0;
        }
    }
    return -2;
}
int HTS221_ReadTemperature(HTS221 *s, float *temperature)
{
    uint8_t raw[2];
    if (!temperature || HTS221_ReadRegisters(s, HTS221_TEMP_OUT, raw, 2)) return -1;
    *temperature = s->t0 + (signed16(raw) - s->t0_out) * (s->t1 - s->t0)
                   / (s->t1_out - s->t0_out);
    return 0;
}
int HTS221_ReadHumidity(HTS221 *s, float *humidity)
{
    uint8_t raw[2];
    if (!humidity || HTS221_ReadRegisters(s, HTS221_HUMIDITY_OUT, raw, 2)) return -1;
    float h = s->h0 + (signed16(raw) - s->h0_out) * (s->h1 - s->h0)
              / (s->h1_out - s->h0_out);
    *humidity = h < 0 ? 0 : (h > 100 ? 100 : h);
    return 0;
}
