#include "hts221.h"
#include "lps22hb.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint8_t hreg[256], preg[256];
static int fail_bus, stuck, pressure_bursts, temperature_bursts, humidity_bursts;
static int read_bus(uint8_t a, uint8_t r, uint8_t *d, uint16_t n)
{
    if (fail_bus) return -1;
    uint8_t *regs;
    if (a == 0x5f) {
        assert(n == 1 || (r & 0x80)); r &= 0x7f; regs = hreg;
        if (r == 0x2a) { assert(n == 2); temperature_bursts++; }
        if (r == 0x28) { assert(n == 2); humidity_bursts++; }
    } else if (a == 0x5d) {
        regs = preg;
        if (r == 0x28) { assert(n == 3); pressure_bursts++; }
    } else return -1;
    memcpy(d, regs + r, n); return 0;
}
static int write_bus(uint8_t a, uint8_t r, const uint8_t *d, uint16_t n)
{
    if (fail_bus) return -1;
    uint8_t *regs = a == 0x5f ? hreg : preg;
    r &= 0x7f;
    memcpy(regs + r, d, n);
    if (!stuck && a == 0x5f && r == 0x21 && (*d & 1)) { regs[r] &= ~1U; regs[0x27] = 3; }
    if (!stuck && a == 0x5d && r == 0x11) regs[r] &= ~1U;
    return 0;
}
static void delay_ms(uint32_t n) { (void)n; }
static void set16(uint8_t r, int16_t value) { hreg[r] = (uint16_t)value; hreg[r+1] = (uint16_t)value >> 8; }
int main(void)
{
    SensorBus bus = {read_bus, write_bus, delay_ms}; HTS221 h; LPS22HB p; float v;
    hreg[0xf] = 0xbc; preg[0xf] = 0xb1;
    hreg[0x30] = 40; hreg[0x31] = 160; /* 20..80 % */
    hreg[0x32] = 80; hreg[0x33] = 64; hreg[0x35] = 4; /* 10..40 C; high bits exercised */
    set16(0x36,-1000); set16(0x3a,1000); set16(0x3c,-1000); set16(0x3e,1000);
    set16(0x28,0); set16(0x2a,0);
    int raw = 1000 * 4096; preg[0x28]=raw; preg[0x29]=raw>>8; preg[0x2a]=raw>>16;
    assert(!HTS221_Init(&h,&bus)); assert(!LPS22HB_Init(&p,&bus)); assert(p.address==0x5d);
    assert(preg[0x10]==0 && preg[0x11]==0x10);
    assert(!HTS221_Measure(&h));
    assert(!HTS221_ReadTemperature(&h,&v) && fabsf(v-25)<0.001f);
    assert(!HTS221_ReadHumidity(&h,&v) && fabsf(v-50)<0.001f);
    set16(0x28,3000); assert(!HTS221_ReadHumidity(&h,&v) && v==100);
    set16(0x28,-3000); assert(!HTS221_ReadHumidity(&h,&v) && v==0);
    assert(!LPS22HB_ReadPressure(&p,&v) && v==1000);
    assert(pressure_bursts==1 && temperature_bursts==1 && humidity_bursts==3);
    assert(fabsf(LPS22HB_RelativeHeight(1000,1000))<0.001f);
    assert(LPS22HB_RelativeHeight(990,1000)>0);
    assert(LPS22HB_RelativeHeight(1010,1000)<0);
    stuck=1; assert(HTS221_Measure(&h)==-2); assert(LPS22HB_ReadPressure(&p,&v)==-2); stuck=0;
    fail_bus=1; assert(HTS221_ReadTemperature(&h,&v)!=0); assert(LPS22HB_ReadPressure(&p,&v)!=0); fail_bus=0;
    hreg[0xf]=0; assert(HTS221_Init(&h,&bus)==-2); hreg[0xf]=0xbc;
    set16(0x3e,-1000); assert(HTS221_Init(&h,&bus)==-4);
    preg[0xf]=0; assert(LPS22HB_Init(&p,&bus)==-2);
    puts("Sensor tests passed: calibration, burst reads, address selection, timeouts, errors, height.");
}
