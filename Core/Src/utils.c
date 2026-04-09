#include "utils.h"

#define CPU_FREQ_MHZ (HAL_RCC_GetSysClockFreq() / 1000000UL)

void delay_us(uint32_t nus)
{
    uint32_t ticks = nus * CPU_FREQ_MHZ;
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < ticks);
}

void delay_ms(uint16_t nms)
{
    delay_us((uint32_t)nms * 1000);
}
