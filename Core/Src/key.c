#include "key.h"
#include "FreeRTOS.h"
#include "task.h"

void key_init(void)
{
}

uint8_t key_scan(uint8_t mode)
{
    static uint8_t key_up = 1;

    if (mode) key_up = 1;

    if (key_up && (KEY0 == GPIO_PIN_RESET || KEY1 == GPIO_PIN_RESET || KEY2 == GPIO_PIN_RESET))
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        key_up = 0;

        if (KEY0 == GPIO_PIN_RESET) return KEY0_PRES;
        if (KEY1 == GPIO_PIN_RESET) return KEY1_PRES;
        if (KEY2 == GPIO_PIN_RESET) return KEY2_PRES;
    }
    else if (KEY0 == GPIO_PIN_SET && KEY1 == GPIO_PIN_SET && KEY2 == GPIO_PIN_SET)
    {
        key_up = 1;
    }

    return 0;
}
