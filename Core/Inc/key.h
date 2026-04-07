#ifndef __KEY_H
#define __KEY_H

#include "stm32h7xx_hal.h"

#define KEY0_GPIO_PORT GPIOH
#define KEY0_GPIO_PIN  GPIO_PIN_3
#define KEY0_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOH_CLK_ENABLE(); }while(0)

#define KEY1_GPIO_PORT GPIOH
#define KEY1_GPIO_PIN  GPIO_PIN_2
#define KEY1_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOH_CLK_ENABLE(); }while(0)

#define KEY2_GPIO_PORT GPIOC
#define KEY2_GPIO_PIN  GPIO_PIN_13
#define KEY2_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0)

#define KEY0 HAL_GPIO_ReadPin(KEY0_GPIO_PORT, KEY0_GPIO_PIN)
#define KEY1 HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN)
#define KEY2 HAL_GPIO_ReadPin(KEY2_GPIO_PORT, KEY2_GPIO_PIN)

#define KEY0_PRES 1
#define KEY1_PRES 2
#define KEY2_PRES 3

void key_init(void);
uint8_t key_scan(uint8_t mode);

#endif
