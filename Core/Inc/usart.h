#ifndef __USART_H
#define __USART_H

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_uart.h"
#include "stdio.h"

#define S_IFCHR  0020000
#define S_IFMPC  0020000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFIFO  0010000

struct stat {
    int st_dev;
    int st_ino;
    unsigned short st_mode;
    int st_nlink;
    int st_uid;
    int st_gid;
    int st_rdev;
    long st_size;
    long st_atime;
    long st_mtime;
    long st_ctime;
};

#define USART_UX USART1
#define USART_TX_GPIO_PORT GPIOA
#define USART_TX_GPIO_PIN GPIO_PIN_9
#define USART_RX_GPIO_PORT GPIOA
#define USART_RX_GPIO_PIN GPIO_PIN_10
#define USART_TX_GPIO_AF GPIO_AF7_USART1
#define USART_RX_GPIO_AF GPIO_AF7_USART1

#define USART_UX_CLK_ENABLE() do{ __HAL_RCC_USART1_CLK_ENABLE(); }while(0)
#define USART_TX_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)
#define USART_RX_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)

extern UART_HandleTypeDef g_uart1_handle;

void usart_init(uint32_t baudrate);
int fputc(int ch, FILE *f);

#endif
