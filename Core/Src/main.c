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
#include "timestamp.h"
#include "dht11.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* SystemState_t defined in main.h */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TASK_LED_STACK     256
#define TASK_KEY_STACK     1024
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
#define TASK_DHT11_STACK   256
#define TASK_DHT11_PRIO    3
#define DHT11_READ_INTERVAL_MS  2000

#define FILTER_SAMPLE_COUNT    10
#define FILTER_MAX_RETRIES     5
#define FILTER_STD_TEMP_THRESHOLD   1.0f
#define FILTER_STD_HUMI_THRESHOLD   3.0f
#define FILTER_RETRY_SUPPLY_COUNT   2
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
TaskHandle_t xTaskLED1Handle = NULL;
TaskHandle_t xTaskLED2Handle = NULL;
TaskHandle_t xTaskKeyHandle  = NULL;
TaskHandle_t xTaskSQLiteHandle = NULL;
TaskHandle_t xTaskDHT11Handle = NULL;

volatile SystemState_t g_system_state = SYSTEM_RUNNING;

typedef struct {
    float temperature;
    float humidity;
} Sample_t;

typedef enum {
    FILTER_STATE_PHASE1_ESTABLISH,
    FILTER_STATE_PHASE2_MONITOR
} FilterState_t;

static Sample_t g_samples[FILTER_SAMPLE_COUNT];
static uint8_t g_sample_count = 0;
static FilterState_t g_filter_state = FILTER_STATE_PHASE1_ESTABLISH;
static uint8_t g_retry_count = 0;
static uint8_t g_error_count = 0;
static float g_last_standard_temp = 0.0f;
static float g_last_standard_humi = 0.0f;
static uint32_t g_last_cycle_start = 0;
static uint8_t g_cycle_minute = 0;

static uint8_t g_phase1_pending = 0;
static uint32_t g_phase1_timestamp = 0;
static float g_phase1_temp = 0.0f;
static float g_phase1_humi = 0.0f;
static uint8_t g_phase1_retry = 0;

#define MINUTE_CYCLE_MS 60000

static uint8_t filter_check_std_deviation(uint8_t count);
static void filter_remove_extremes(uint8_t *count);
static void filter_add_sample(float temperature, float humidity, uint8_t *count);
static uint8_t filter_establish_standard(float *out_temp, float *out_humi, uint8_t *out_env_changed);
static uint8_t filter_monitor_standard(float *out_temp, float *out_humi, uint8_t *out_env_changed);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
static void vTaskLED1(void *pvParameters);
static void vTaskLED2(void *pvParameters);
static void vTaskKey(void *pvParameters);
static void vTaskDHT11(void *pvParameters);
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
  timestamp_init();

  /* USER CODE BEGIN 2 */
  xTaskCreate(vTaskLED1,  "LED1_Task",  TASK_LED_STACK,  NULL, TASK_LED_PRIO,  &xTaskLED1Handle);
  xTaskCreate(vTaskLED2,  "LED2_Task",  TASK_LED_STACK,  NULL, TASK_LED_PRIO,  &xTaskLED2Handle);
  xTaskCreate(vTaskKey,   "Key_Task",   TASK_KEY_STACK,  NULL, TASK_KEY_PRIO,   &xTaskKeyHandle);
  xTaskCreate(vTaskSQLite, "SQLite_Task", TASK_SQLITE_STACK, NULL, TASK_SQLITE_PRIO, &xTaskSQLiteHandle);
  xTaskCreate(vTaskDHT11,  "DHT11_Task", TASK_DHT11_STACK,  NULL, TASK_DHT11_PRIO, &xTaskDHT11Handle);

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
  static uint8_t last_key = 0;

  printf("[KEY] Key Task Started\r\n");

  for(;;)
  {
    key_value = key_scan(0);

    if (last_key == 0 && key_value != 0)
    {
      // 按下
    }
    else if (last_key != 0 && key_value == 0)
    {
      switch (last_key)
      {
        case KEY0_PRES:
          if (g_system_state == SYSTEM_PAUSED)
          {
            printf("[KEY] KEY0 -> Export CSV\r\n");
            sqlite_task_export_csv();
          }
          break;

        case KEY1_PRES:
          if (g_system_state == SYSTEM_PAUSED)
          {
            uint32_t ms = get_tick_ms();
            uint64_t us = get_tick_us();
            printf("\r\n");
            printf("========================================\r\n");
            printf("       KEY1 - 传感器数据查询            \r\n");
            printf("========================================\r\n");
            printf("  Timestamp: %u ms (%llu us)\r\n", ms, us);

            float temperature, humidity;
            uint8_t retry_count, error_count, env_changed;
            if(sqlite_task_query_latest(&temperature, &humidity, &retry_count, &error_count, &env_changed) == 0)
            {
              printf("  Temperature: %.1f C\r\n", temperature);
              printf("  Humidity:    %.1f %%\r\n", humidity);
              printf("  Retry Count: %d\r\n", retry_count);
              printf("  Error Count: %d\r\n", error_count);
              printf("  Env Changed: %d\r\n", env_changed);
              printf("========================================\r\n");

              printf("\r\n  [Hint] Press KEY0 to export CSV\r\n");
            }
            else
            {
              printf("  [Info] No sensor data available\r\n");
              printf("========================================\r\n");
            }
          }
          break;

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

        case (KEY1_PRES | KEY2_PRES):
          printf("[KEY] KEY2+KEY1 -> Print SQLite Status\r\n");
          sqlite_task_print_status();
          break;

        default:
          printf("[KEY] ILLEGAL KEY: 0x%02X\r\n", last_key);
          break;
      }
    }

    last_key = key_value;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */

/* USER CODE END 4 */

/* USER CODE BEGIN 6 */
static uint8_t filter_check_std_deviation(uint8_t count)
{
    if(count < 2) return 0;

    float temp_sum = 0, humi_sum = 0;
    for(uint8_t i = 0; i < count; i++)
    {
        temp_sum += g_samples[i].temperature;
        humi_sum += g_samples[i].humidity;
    }

    float temp_avg = temp_sum / count;
    float humi_avg = humi_sum / count;

    float temp_var = 0, humi_var = 0;
    for(uint8_t i = 0; i < count; i++)
    {
        float temp_diff = g_samples[i].temperature - temp_avg;
        float humi_diff = g_samples[i].humidity - humi_avg;
        temp_var += temp_diff * temp_diff;
        humi_var += humi_diff * humi_diff;
    }

    float temp_std = sqrtf(temp_var / count);
    float humi_std = sqrtf(humi_var / count);

    return (temp_std <= FILTER_STD_TEMP_THRESHOLD) && (humi_std <= FILTER_STD_HUMI_THRESHOLD);
}

static void filter_remove_extremes(uint8_t *count)
{
    if(*count < 4) return;

    uint8_t max_temp_idx = 0, min_temp_idx = 0;
    uint8_t max_humi_idx = 0, min_humi_idx = 0;

    for(uint8_t i = 1; i < *count; i++)
    {
        if(g_samples[i].temperature > g_samples[max_temp_idx].temperature)
            max_temp_idx = i;
        if(g_samples[i].temperature < g_samples[min_temp_idx].temperature)
            min_temp_idx = i;
        if(g_samples[i].humidity > g_samples[max_humi_idx].humidity)
            max_humi_idx = i;
        if(g_samples[i].humidity < g_samples[min_humi_idx].humidity)
            min_humi_idx = i;
    }

    uint8_t remove_count = 0;
    uint8_t remove_idx[4] = {255, 255, 255, 255};

    if(remove_count < 4 && max_temp_idx != min_temp_idx)
    {
        uint8_t already = 0;
        for(uint8_t i = 0; i < remove_count; i++)
            if(remove_idx[i] == max_temp_idx) already = 1;
        if(!already) remove_idx[remove_count++] = max_temp_idx;
    }

    if(remove_count < 4 && min_temp_idx != max_temp_idx)
    {
        uint8_t already = 0;
        for(uint8_t i = 0; i < remove_count; i++)
            if(remove_idx[i] == min_temp_idx) already = 1;
        if(!already) remove_idx[remove_count++] = min_temp_idx;
    }

    if(remove_count < 4 && max_humi_idx != max_temp_idx && max_humi_idx != min_temp_idx)
    {
        uint8_t already = 0;
        for(uint8_t i = 0; i < remove_count; i++)
            if(remove_idx[i] == max_humi_idx) already = 1;
        if(!already) remove_idx[remove_count++] = max_humi_idx;
    }

    if(remove_count < 4 && min_humi_idx != max_temp_idx && min_humi_idx != min_temp_idx && min_humi_idx != max_humi_idx)
    {
        uint8_t already = 0;
        for(uint8_t i = 0; i < remove_count; i++)
            if(remove_idx[i] == min_humi_idx) already = 1;
        if(!already) remove_idx[remove_count++] = min_humi_idx;
    }

    for(uint8_t i = remove_count; i > 0; i--)
    {
        for(uint8_t j = 0; j < i - 1; j++)
        {
            if(remove_idx[j] > remove_idx[j + 1])
            {
                uint8_t temp = remove_idx[j];
                remove_idx[j] = remove_idx[j + 1];
                remove_idx[j + 1] = temp;
            }
        }
    }

    for(uint8_t i = 0; i < remove_count; i++)
    {
        for(uint8_t j = remove_idx[i]; j < (*count - 1); j++)
        {
            g_samples[j] = g_samples[j + 1];
        }
        (*count)--;
        for(uint8_t k = i + 1; k < remove_count; k++)
        {
            remove_idx[k]--;
        }
    }
}

static void filter_add_sample(float temperature, float humidity, uint8_t *count)
{
    if(*count >= FILTER_SAMPLE_COUNT) return;

    g_samples[*count].temperature = temperature;
    g_samples[*count].humidity = humidity;
    (*count)++;
}

static uint8_t filter_establish_standard(float *out_temp, float *out_humi, uint8_t *out_env_changed)
{
    for(g_retry_count = 0; g_retry_count < FILTER_MAX_RETRIES; g_retry_count++)
    {
        if(filter_check_std_deviation(g_sample_count))
        {
            float temp_sum = 0, humi_sum = 0;
            for(uint8_t i = 0; i < g_sample_count; i++)
            {
                temp_sum += g_samples[i].temperature;
                humi_sum += g_samples[i].humidity;
            }
            *out_temp = temp_sum / g_sample_count;
            *out_humi = humi_sum / g_sample_count;

            float temp_diff = fabsf(*out_temp - g_last_standard_temp);
            float humi_diff = fabsf(*out_humi - g_last_standard_humi);
            *out_env_changed = (temp_diff > FILTER_STD_TEMP_THRESHOLD * 2.0f) || 
                              (humi_diff > FILTER_STD_HUMI_THRESHOLD * 2.0f);

            return 1;
        }

        if(g_sample_count >= 4)
        {
            filter_remove_extremes(&g_sample_count);
        }
    }

    g_retry_count = FILTER_MAX_RETRIES;
    return 0;
}

static uint8_t filter_monitor_standard(float *out_temp, float *out_humi, uint8_t *out_env_changed)
{
    if(!filter_check_std_deviation(g_sample_count))
    {
        return 0;
    }

    float temp_sum = 0, humi_sum = 0;
    for(uint8_t i = 0; i < g_sample_count; i++)
    {
        temp_sum += g_samples[i].temperature;
        humi_sum += g_samples[i].humidity;
    }

    *out_temp = temp_sum / g_sample_count;
    *out_humi = humi_sum / g_sample_count;

    float temp_diff = fabsf(*out_temp - g_last_standard_temp);
    float humi_diff = fabsf(*out_humi - g_last_standard_humi);

    *out_env_changed = (temp_diff > FILTER_STD_TEMP_THRESHOLD * 2.0f) || 
                      (humi_diff > FILTER_STD_HUMI_THRESHOLD * 2.0f);

    return 1;
}

static void vTaskDHT11(void *pvParameters)
{
    (void)pvParameters;
    uint8_t temperature = 0;
    uint8_t humidity = 0;
    uint32_t tick = 0;

    printf("[DHT11] Task Started\r\n");

    if (dht11_init() != 0)
    {
        printf("[DHT11] ERROR: DHT11 not found!\r\n");
        vTaskDelete(NULL);
        return;
    }

    printf("[DHT11] Initialization OK\r\n");

    g_sample_count = 0;
    g_retry_count = 0;
    g_error_count = 0;
    g_filter_state = FILTER_STATE_PHASE1_ESTABLISH;
    g_last_cycle_start = get_tick_ms();
    g_cycle_minute = 1;
    g_phase1_pending = 0;

    for(;;)
    {
        if (g_system_state == SYSTEM_RUNNING)
        {
            tick = get_tick_ms();

            if(tick - g_last_cycle_start >= MINUTE_CYCLE_MS)
            {
                if(g_phase1_pending)
                {
                    sqlite_task_update_by_timestamp(g_phase1_timestamp, g_error_count);
                    g_phase1_pending = 0;
                }
                else
                {
                    if(sqlite_task_insert(tick, 0.0f, 0.0f, g_retry_count, g_error_count, 1) == 0)
                    {
                        printf("[%ums P1] 阶段失败 -> T=0.0C H=0.0%% [min=%d retry=%d error=%d]\r\n",
                               tick, g_cycle_minute, g_retry_count, g_error_count);
                    }
                }

                g_last_cycle_start = tick;
                g_cycle_minute++;
                g_filter_state = FILTER_STATE_PHASE1_ESTABLISH;
                g_sample_count = 0;
                g_retry_count = 0;
                g_error_count = 0;
            }

            if (dht11_read_data(&temperature, &humidity) == 0)
            {
                filter_add_sample((float)temperature, (float)humidity, &g_sample_count);

                if(g_sample_count >= FILTER_SAMPLE_COUNT)
                {
                    float std_temp, std_humi;
                    uint8_t env_changed = 0;
                    uint8_t success = 0;

                    if(g_filter_state == FILTER_STATE_PHASE1_ESTABLISH)
                    {
                        success = filter_establish_standard(&std_temp, &std_humi, &env_changed);

                        if(success)
                        {
                            g_phase1_timestamp = tick;
                            g_phase1_pending = 1;

                            if(sqlite_task_insert(tick, std_temp, std_humi, g_retry_count, 0, 1) == 0)
                            {
                                printf("[%ums P1] 新标准已存储 -> T=%.1fC H=%.1f%% [min=%d retry=%d]\r\n",
                                       tick, std_temp, std_humi, g_cycle_minute, g_retry_count);
                            }

                            g_last_standard_temp = std_temp;
                            g_last_standard_humi = std_humi;

                            g_filter_state = FILTER_STATE_PHASE2_MONITOR;
                            g_sample_count = 0;
                        }
                        else {g_retry_count++;}
                    }
                    else
                    {
                        success = filter_monitor_standard(&std_temp, &std_humi, &env_changed);

                        if(success)
                        {
                            if(env_changed)
                            {
                                if(g_phase1_pending)
                                {
                                    sqlite_task_update_by_timestamp(g_phase1_timestamp, g_error_count);
                                    g_phase1_pending = 0;
                                }

                                if(sqlite_task_insert(tick, std_temp, std_humi, 0, 0, env_changed) == 0)
                                {
                                    printf("[%ums P2] 新标准已存储 -> T=%.1fC H=%.1f%% [env=%d]\r\n",
                                           tick, std_temp, std_humi, env_changed);
                                }

                                g_last_standard_temp = std_temp;
                                g_last_standard_humi = std_humi;
                                g_error_count = 0;
                            }
                            g_sample_count = 0;
                        }
                        else
                        {
                            g_error_count++;
                            if(g_phase1_pending)
                            {
                                sqlite_task_update_by_timestamp(g_phase1_timestamp, g_error_count);
                            }
                            filter_remove_extremes(&g_sample_count);
                        }
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DHT11_READ_INTERVAL_MS));
    }
}
/* USER CODE END 6 */

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
