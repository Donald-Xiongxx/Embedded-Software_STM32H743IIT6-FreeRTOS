#include "timestamp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

#define TIMESTAMP_TIMX_INT          TIM3
#define TIMESTAMP_TIMX_INT_IRQn     TIM3_IRQn
#define TIMESTAMP_TIMX_INT_CLK_ENABLE() do{ __HAL_RCC_TIM3_CLK_ENABLE(); }while(0)

TIM_HandleTypeDef g_timestamp_tim_handle;
volatile uint32_t g_tick_ms = 0;
static volatile uint32_t g_dwt_last = 0;
static volatile uint64_t g_dwt_overflow_count = 0;

extern volatile SystemState_t g_system_state;

void timestamp_init(void)
{
    TIMESTAMP_TIMX_INT_CLK_ENABLE();

    g_timestamp_tim_handle.Instance = TIMESTAMP_TIMX_INT;
    g_timestamp_tim_handle.Init.Prescaler = 240 - 1;
    g_timestamp_tim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_timestamp_tim_handle.Init.Period = 1000 - 1;
    HAL_TIM_Base_Init(&g_timestamp_tim_handle);

    HAL_NVIC_SetPriority(TIMESTAMP_TIMX_INT_IRQn, 9, 0);
    HAL_NVIC_EnableIRQ(TIMESTAMP_TIMX_INT_IRQn);

    HAL_TIM_Base_Start_IT(&g_timestamp_tim_handle);

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;
}

uint32_t get_tick_ms(void)
{
    return g_tick_ms;
}

uint64_t get_tick_us(void)
{
    uint32_t current = DWT->CYCCNT;
    uint32_t last = g_dwt_last;
    uint64_t overflows = g_dwt_overflow_count;

    if (current < last)
    {
        overflows++;
        g_dwt_overflow_count = overflows;
    }
    g_dwt_last = current;

    uint64_t total_cycles = (overflows * (uint64_t)UINT32_MAX) + current;
    uint32_t cpu_clk = HAL_RCC_GetSysClockFreq();
    return total_cycles * 1000000ULL / cpu_clk;
}

void TIM3_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&g_timestamp_tim_handle, TIM_FLAG_UPDATE) != RESET)
    {
        if (g_system_state == SYSTEM_RUNNING)
        {
            g_tick_ms++;
        }
        __HAL_TIM_CLEAR_IT(&g_timestamp_tim_handle, TIM_IT_UPDATE);
    }
}
