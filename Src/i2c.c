/**
  ******************************************************************************
  * File Name          : I2C.c
  * Description        : This file provides code for the configuration
  *                      of the I2C instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "i2c.h"

/* USER CODE BEGIN 0 */
uint8_t i2c_rx_data = 0;

/* USER CODE END 0 */

/* I2C1 init function */
void MX_I2C1_Init(void)
{
  LL_I2C_InitTypeDef I2C_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  /**I2C1 GPIO Configuration
  PB6   ------> I2C1_SCL
  PB7   ------> I2C1_SDA
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6|LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

  /* I2C1 interrupt Init */
  NVIC_SetPriority(I2C1_EV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(I2C1_EV_IRQn);

  /** I2C Initialization
  */
  LL_I2C_EnableAutoEndMode(I2C1);
  LL_I2C_DisableOwnAddress2(I2C1);
  LL_I2C_DisableGeneralCall(I2C1);
  LL_I2C_EnableClockStretching(I2C1);
  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.Timing = 0x2000090E;
  I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  I2C_InitStruct.DigitalFilter = 0;
  I2C_InitStruct.OwnAddress1 = 2;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  LL_I2C_Init(I2C1, &I2C_InitStruct);
  LL_I2C_SetOwnAddress2(I2C1, 0, LL_I2C_OWNADDRESS2_NOMASK);

}

/* USER CODE BEGIN 1 */
/* Bounded polling: this driver does not enable I2C peripheral interrupts. */
static int wait_flag(uint32_t flag)
{
    uint32_t budget = SystemCoreClock / 10U;
    while (budget--) {
        uint32_t status = I2C1->ISR;
        if (status & (I2C_ISR_NACKF | I2C_ISR_BERR | I2C_ISR_ARLO | I2C_ISR_OVR)) return -1;
        if (status & flag) return 0;
        if (status & I2C_ISR_STOPF) return -1;
    }
    return -2;
}
static int finish_transfer(int result)
{
    if (result) {
        /* PE reset aborts an incomplete transfer without an unbounded STOP wait. */
        LL_I2C_Disable(I2C1);
        LL_I2C_Enable(I2C1);
    }
    I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF | I2C_ICR_BERRCF |
                I2C_ICR_ARLOCF | I2C_ICR_OVRCF;
    return result;
}
static int start_transfer(void)
{
    LL_I2C_Enable(I2C1);
    uint32_t budget = SystemCoreClock / 10U;
    while (LL_I2C_IsActiveFlag_BUSY(I2C1))
        if (!budget--) return finish_transfer(-2);
    return finish_transfer(0);
}
int I2C_ReadRegisters(uint8_t address, uint8_t reg, uint8_t *data, uint16_t length)
{
    int result;
    if (!data || !length || length > 254 || address > 0x7F) return -3;
    if ((result = start_transfer())) return result;
    LL_I2C_HandleTransfer(I2C1, (uint32_t)address << 1, LL_I2C_ADDRSLAVE_7BIT,
                         1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
    if ((result = wait_flag(I2C_ISR_TXIS))) return finish_transfer(result);
    LL_I2C_TransmitData8(I2C1, reg);
    if ((result = wait_flag(I2C_ISR_TC))) return finish_transfer(result);
    LL_I2C_HandleTransfer(I2C1, (uint32_t)address << 1, LL_I2C_ADDRSLAVE_7BIT,
                         length, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
    for (uint16_t i = 0; i < length; ++i) {
        if ((result = wait_flag(I2C_ISR_RXNE))) return finish_transfer(result);
        data[i] = LL_I2C_ReceiveData8(I2C1);
    }
    return finish_transfer(wait_flag(I2C_ISR_STOPF));
}
int I2C_WriteRegisters(uint8_t address, uint8_t reg, const uint8_t *data, uint16_t length)
{
    int result;
    if (!data || !length || length > 254 || address > 0x7F) return -3;
    if ((result = start_transfer())) return result;
    LL_I2C_HandleTransfer(I2C1, (uint32_t)address << 1, LL_I2C_ADDRSLAVE_7BIT,
                         length + 1U, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
    if ((result = wait_flag(I2C_ISR_TXIS))) return finish_transfer(result);
    LL_I2C_TransmitData8(I2C1, reg);
    for (uint16_t i = 0; i < length; ++i) {
        if ((result = wait_flag(I2C_ISR_TXIS))) return finish_transfer(result);
        LL_I2C_TransmitData8(I2C1, data[i]);
    }
    return finish_transfer(wait_flag(I2C_ISR_STOPF));
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
