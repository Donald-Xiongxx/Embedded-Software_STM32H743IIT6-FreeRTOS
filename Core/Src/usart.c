#include "usart.h"
#include "main.h"

#if (__ARMCC_VERSION >= 6010050)
__asm(".global __use_no_semihosting\n\t");
__asm(".global __ARM_use_no_argv \n\t");
#else
#pragma import(__use_no_semihosting)
struct __FILE {
    int handle;
};
#endif

int _ttywrch(int ch)
{
    return ch;
}

void _sys_exit(int x)
{
    (void)x;
}

char *_sys_command_string(char *cmd, int len)
{
    (void)cmd;
    (void)len;
    return NULL;
}

FILE __stdout;

int fputc(int ch, FILE *f)
{
    (void)f;
    while ((USART_UX->ISR & 0X40) == 0);
    USART_UX->TDR = (uint8_t)ch;
    return ch;
}

int _write(int fd, char *ptr, int len)
{
    (void)fd;
    for(int i = 0; i < len; i++)
    {
        while((USART_UX->ISR & 0x40) == 0);
        USART_UX->TDR = (uint8_t)ptr[i];
    }
    return len;
}

int _read(int fd, char *ptr, int len)
{
    (void)fd;
    (void)ptr;
    return 0;
}

UART_HandleTypeDef g_uart1_handle;

void usart_init(uint32_t baudrate)
{
    g_uart1_handle.Instance = USART_UX;
    g_uart1_handle.Init.BaudRate = baudrate;
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart1_handle.Init.StopBits = UART_STOPBITS_1;
    g_uart1_handle.Init.Parity = UART_PARITY_NONE;
    g_uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_uart1_handle.Init.Mode = UART_MODE_TX_RX;

    if (HAL_UART_Init(&g_uart1_handle) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;

    if (huart->Instance == USART_UX)
    {
        USART_UX_CLK_ENABLE();
        USART_TX_GPIO_CLK_ENABLE();
        USART_RX_GPIO_CLK_ENABLE();

        gpio_init_struct.Pin = USART_TX_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = USART_TX_GPIO_AF;
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = USART_RX_GPIO_PIN;
        gpio_init_struct.Alternate = USART_RX_GPIO_AF;
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);
    }
}
