# X-NUCLEO-IKS01A2 sensor implementation

Custom drivers: HTS221 (temperature/humidity), LPS22HB (pressure).
The assignment names LPS25HB; the user's IKS01A2 board contains LPS22HB.
Confirm that hardware substitution with the instructor. No vendor sensor driver was copied.

## Design

- HTS221/ and LPS22HB/ are separate CubeIDE source folders, registered in .cproject.
- Inc/sensor_bus.h defines registered read/write/delay callbacks. Sensor libraries do not include STM32 or i2c.h headers.
- i2c.c implements bounded LL polling transfers with repeated START for register reads, NACK/error checks and peripheral reset after errors. Addresses passed to callbacks are unshifted 7-bit addresses. Length 1 handles single-byte accesses; 2..254 uses one multi-byte transfer. It does not recover a slave physically holding SDA low.
- HTS221 identity 0xBC at address 0x5F, factory calibration loaded once per initialization, one-shot conversion, BDU, separate temperature and humidity functions. Multi-byte register access sets subaddress bit 7.
- LPS22HB identity 0xB1, probes 0x5C and 0x5D. ODR=0, BDU=0, address increment enabled. Starts one-shot conversion and waits for ONE_SHOT to clear before reading all three pressure bytes. No conversion runs during the burst. This avoids the datasheet's I2C BDU restriction while satisfying the assignment's multi-byte requirement.
- Relative height uses 44330*(1-(pressure/reference_pressure)^0.19029495), with the first valid pressure as zero. Reset or recovery after a sensor error establishes a new zero. This estimates barometric height, not direct distance above terrain; weather/noise affect it.
- CSV: temperature (1 decimal), humidity (integer), pressure (2 decimals), height (2 decimals). Updates roughly once per second plus conversion time. Diagnostic lines begin with '#'; skip these in a CSV plotter. Failed readings never produce numeric CSV.
- New application code is in USER CODE sections. Existing peripheral initialization remains CubeMX-managed. USART2 is 115200 8N1, PA2 TX / PA15 RX. I2C1 is PB6 SCL / PB7 SDA. The polling transfer enables I2C explicitly; no I2C peripheral interrupts are enabled by this code.

## Build and test

Refresh the project (F5); close/reopen it if CubeIDE has cached the old .cproject. Clean and Build. Both Debug and Release enable float printf. Verify HTS221 and LPS22HB appear in the build output. Flash using the working launch configuration, then Resume.

Terminal: 115200 8N1, no flow control. Look for sensor detection followed by changing numeric values. Initialization errors: -2 identity/read failure, HTS221 -4 invalid calibration. Read errors: -1 bus access, -2 conversion timeout, LPS22HB -3 pressure outside range. If detection fails, verify board power, common ground, SDA/SCL wiring and jumpers. MCU pins are PB7 SDA and PB6 SCL; USART RX PA15 is unrelated to sensors.

Host tests (from project root):

```sh
gcc -std=c11 -Wall -Wextra -Werror -IInc -IHTS221 -ILPS22HB tests/test_sensors.c HTS221/hts221.c LPS22HB/lps22hb.c -lm -o /tmp/test_vrs_sensors
/tmp/test_vrs_sensors
```

Tests mock the bus: calibration (including signed samples/high bits), humidity clipping, pressure burst length, address fallback, bad identities/calibration, bus errors, conversion timeouts, and relative height. They do not verify electrical I2C timing or real sensors.

## Sources

- https://www.st.com/resource/en/datasheet/hts221.pdf (register map and calibration table 19)
- https://www.st.com/resource/en/datasheet/lps22hb.pdf (CTRL_REG1/2; I2C BDU footnote; one-shot mode)
- https://www.st.com/resource/en/product_presentation/x-nucleo-iks01a2_quick_start_guide.pdf
