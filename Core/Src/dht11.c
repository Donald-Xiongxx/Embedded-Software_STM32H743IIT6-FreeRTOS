#include "dht11.h"
#include "utils.h"

static void dht11_reset(void)
{
    DHT11_DQ_OUT(0);
    delay_ms(20);
    DHT11_DQ_OUT(1);
    delay_us(30);
}

uint8_t dht11_check(void)
{
    uint8_t retry = 0;
    uint8_t rval = 0;

    while (DHT11_DQ_IN && retry < 100)
    {
        retry++;
        delay_us(1);
    }

    if (retry >= 100)
    {
        rval = 1;
    }
    else
    {
        retry = 0;

        while (!DHT11_DQ_IN && retry < 100)
        {
            retry++;
            delay_us(1);
        }
        if (retry >= 100) rval = 1;
    }

    return rval;
}

static uint8_t dht11_read_bit(void)
{
    uint8_t retry = 0;

    while (DHT11_DQ_IN && retry < 100)
    {
        retry++;
        delay_us(1);
    }

    retry = 0;

    while (!DHT11_DQ_IN && retry < 100)
    {
        retry++;
        delay_us(1);
    }

    delay_us(40);

    if (DHT11_DQ_IN)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static uint8_t dht11_read_byte(void)
{
    uint8_t i, data = 0;

    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        data |= dht11_read_bit();
    }

    return data;
}

uint8_t dht11_read_data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];
    uint8_t i;
    dht11_reset();

    if (dht11_check() == 0)
    {
        for (i = 0; i < 5; i++)
        {
            buf[i] = dht11_read_byte();
        }

        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];
        }
    }
    else
    {
        return 1;
    }

    return 0;
}

uint8_t dht11_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;

    DHT11_DQ_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin = DHT11_DQ_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_DQ_GPIO_PORT, &gpio_init_struct);

    dht11_reset();
    return dht11_check();
}
