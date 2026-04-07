#ifndef __TIMESTAMP_H
#define __TIMESTAMP_H

#include "stm32h7xx_hal.h"

void timestamp_init(void);
uint32_t get_tick_ms(void);
uint64_t get_tick_us(void);
void TIM3_IRQHandler(void);

#endif
