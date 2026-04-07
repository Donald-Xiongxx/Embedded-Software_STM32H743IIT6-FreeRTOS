/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"
#include "sqlite_task.h"
#include "key.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    SYSTEM_RUNNING = 0,
    SYSTEM_PAUSED
} SystemState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TASK_LED_STACK     256
#define TASK_KEY_STACK     256
#define TASK_SQLITE_STACK  2048

#define TASK_LED_PRIO      2
#define TASK_KEY_PRIO      1
#define TASK_SQLITE_PRIO   2

#define LED_BLINK_MS       500

#define LED1_PORT   GPIOB
#define LED1_PIN    GPIO_PIN_0
#define LED2_PORT   GPIOB
#define LED2_PIN    GPIO_PIN_1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
TaskHandle_t xTaskLED1Handle = NULL;
TaskHandle_t xTaskLED2Handle = NULL;
TaskHandle_t xTaskKeyHandle  = NULL;
TaskHandle_t xTaskSQLiteHandle = NULL;

volatile SystemState_t g_system_state = SYSTEM_RUNNING;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
static void vTaskLED1(void *pvParameters);
static void vTaskLED2(void *pvParameters);
static void vTaskKey(void *pvParameters);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  usart_init(115200);

  /* USER CODE BEGIN 2 */
  xTaskCreate(vTaskLED1,  "LED1_Task",  TASK_LED_STACK,  NULL, TASK_LED_PRIO,  &xTaskLED1Handle);
  xTaskCreate(vTaskLED2,  "LED2_Task",  TASK_LED_STACK,  NULL, TASK_LED_PRIO,  &xTaskLED2Handle);
  xTaskCreate(vTaskKey,   "Key_Task",   TASK_KEY_STACK,  NULL, TASK_KEY_PRIO,   &xTaskKeyHandle);
  xTaskCreate(vTaskSQLite, "SQLite_Task", TASK_SQLITE_STACK, NULL, TASK_SQLITE_PRIO, &xTaskSQLiteHandle);

  vTaskStartScheduler();
  /* USER CODE END 2 */

  while (1)
  {
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 20;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void vTaskLED1(void *pvParameters)
{
  (void)pvParameters;

  for(;;)
  {
    if (g_system_state == SYSTEM_RUNNING)
    {
      HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
      vTaskDelay(pdMS_TO_TICKS(LED_BLINK_MS));
      HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
      vTaskDelay(pdMS_TO_TICKS(LED_BLINK_MS));
    }
    else
    {
      HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

static void vTaskLED2(void *pvParameters)
{
  (void)pvParameters;

  for(;;)
  {
    if (g_system_state == SYSTEM_RUNNING)
    {
      HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_SET);
    }
    else
    {
      HAL_GPIO_WritePin(LED2_PORT, LED2_PIN, GPIO_PIN_RESET);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

static void vTaskKey(void *pvParameters)
{
  (void)pvParameters;
  uint8_t key_value;

  printf("[KEY] Key Task Started\r\n");

  for(;;)
  {
    key_value = key_scan(0);

    if (key_value != 0)
    {
      switch (key_value)
      {
        case KEY2_PRES:
          if (g_system_state == SYSTEM_RUNNING)
          {
            g_system_state = SYSTEM_PAUSED;
            printf("[KEY] KEY2 -> PAUSED\r\n");
          }
          else
          {
            g_system_state = SYSTEM_RUNNING;
            printf("[KEY] KEY2 -> RUNNING\r\n");
          }
          break;

        case KEY1_PRES:
          if (g_system_state == SYSTEM_PAUSED)
          {
            printf("[KEY] KEY1 -> Print Sensor Data\r\n");
          }
          break;

        case KEY0_PRES:
          if (g_system_state == SYSTEM_PAUSED)
          {
            printf("[KEY] KEY0 -> Print Scheduler Log\r\n");
          }
          break;

        default:
          break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */

/* USER CODE END 5 */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  HAL_MPU_Disable();

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RW_URO;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x30020000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x20000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
