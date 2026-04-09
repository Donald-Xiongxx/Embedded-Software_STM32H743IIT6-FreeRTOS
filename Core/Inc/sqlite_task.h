#ifndef SQLITE_TASK_H
#define SQLITE_TASK_H

#include "FreeRTOS.h"
#include "task.h"

int sqlite_task_insert(uint32_t timestamp, float temperature, float humidity,
                       uint8_t retry_count,
                       uint8_t error_count,
                       uint8_t env_changed);
int sqlite_task_update_by_timestamp(uint32_t timestamp, uint8_t error_count);
int sqlite_task_query_latest(float *temperature, float *humidity,
                            uint8_t *retry_count,
                            uint8_t *error_count,
                            uint8_t *env_changed);
void sqlite_task_print_status(void);
void sqlite_task_export_csv(void);
void vTaskSQLite(void *pvParameters);

#endif
