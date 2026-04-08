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

#define WKUP_GPIO_PORT GPIOA
#define WKUP_GPIO_PIN  GPIO_PIN_0
#define WKUP_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

#define KEY0 HAL_GPIO_ReadPin(KEY0_GPIO_PORT, KEY0_GPIO_PIN)
#define KEY1 HAL_GPIO_ReadPin(KEY1_GPIO_PORT, KEY1_GPIO_PIN)
#define KEY2 HAL_GPIO_ReadPin(KEY2_GPIO_PORT, KEY2_GPIO_PIN)
#define WKUP HAL_GPIO_ReadPin(WKUP_GPIO_PORT, WKUP_GPIO_PIN)

#define KEY0_MASK   0x01
#define KEY1_MASK   0x02
#define KEY2_MASK   0x04
#define WKUP_MASK   0x08

#define KEY0_PRES 0x01
#define KEY1_PRES 0x02
#define KEY2_PRES 0x04
#define WKUP_PRES 0x08

void key_init(void);
uint8_t key_scan(uint8_t mode);

#endif
