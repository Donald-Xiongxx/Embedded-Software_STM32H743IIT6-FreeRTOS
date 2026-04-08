#ifndef SQLITE_TASK_H
#define SQLITE_TASK_H

#include "FreeRTOS.h"
#include "task.h"

int sqlite_task_init(void);
int sqlite_task_insert(float temperature, float humidity);
int sqlite_task_query_latest(float *temperature, float *humidity);
void sqlite_task_print_status(void);
void vTaskSQLite(void *pvParameters);

#endif
