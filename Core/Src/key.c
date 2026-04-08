#include "key.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"

void key_init(void)
{
}

uint8_t key_scan(uint8_t mode)
{
    static uint8_t last_stable = 0;
    uint8_t current = 0;
    uint8_t debounced = 0;

    if (mode) {
        last_stable = 0;
        return 0;
    }

    if (KEY0 == GPIO_PIN_RESET) current |= KEY0_MASK;
    if (KEY1 == GPIO_PIN_RESET) current |= KEY1_MASK;
    if (KEY2 == GPIO_PIN_RESET) current |= KEY2_MASK;
    if (WKUP == GPIO_PIN_SET)   current |= WKUP_MASK;

    if (current != last_stable) {
        vTaskDelay(pdMS_TO_TICKS(10));

        if (KEY0 == GPIO_PIN_RESET) debounced |= KEY0_MASK;
        if (KEY1 == GPIO_PIN_RESET) debounced |= KEY1_MASK;
        if (KEY2 == GPIO_PIN_RESET) debounced |= KEY2_MASK;
        if (WKUP == GPIO_PIN_SET)   debounced |= WKUP_MASK;

        if (debounced == current) {
            last_stable = current;
        }
    }

    return last_stable;
}
