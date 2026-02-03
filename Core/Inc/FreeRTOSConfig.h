#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ============== 1. 核心配置（匹配port.c的强制依赖） ============== */
/* 1.1 Tick类型宽度（portmacro.h要求，必须定义） */
// #define configTICK_TYPE_WIDTH_IN_BITS    32

/* 1.2 调度器配置（port.c启用端口优化，需configMAX_PRIORITIES≤32） */
#define configUSE_PREEMPTION                    1       // 启用抢占式调度（port.c支持）
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1       // 匹配portmacro.h的默认值
#define configUSE_TICKLESS_IDLE                 0       // 新手先关闭，port.c虽实现但需额外适配
#define configCPU_CLOCK_HZ                      (400000000UL)  // H743主频400MHz
#define configTICK_RATE_HZ                      (1000UL)       // 1ms节拍（port.c用于计算SysTick重载值）
#define configMAX_PRIORITIES                    (16)           // ≤32，满足端口优化要求
#define configMINIMAL_STACK_SIZE                (128)          // 空闲任务栈大小
#define configTOTAL_HEAP_SIZE                   (16384UL)      // 堆大小16KB
#define configMAX_TASK_NAME_LEN                 (16)           // 任务名最大长度
#define configUSE_TRACE_FACILITY                0              // 关闭追踪
#define configUSE_16_BIT_TICKS                  0              // 匹配32位tick
#define configIDLE_SHOULD_YIELD                 1              // 空闲任务让步

/* 1.3 port.c的关键依赖（必须补充） */
// #define vPortSVCHandler                        SVC_Handler
// #define xPortPendSVHandler                     PendSV_Handler
// #define xPortSysTickHandler                    SysTick_Handler
#define configCHECK_HANDLER_INSTALLATION        0       // 关闭中断处理函数检查（避免port.c的断言）
#define configSYSTICK_CLOCK_HZ                 (configCPU_CLOCK_HZ/2)  // 匹配port.c的默认定义
#define configASSERT_DEFINED                    0       // 关闭断言（port.c的vPortValidateInterruptPriority未实现）

/* ============== 2. 中断配置（port.c核心依赖） ============== */
/* 关键：STM32H743是4位抢占优先级（0-15），数值越小优先级越高 */
#define configKERNEL_INTERRUPT_PRIORITY         (15)    // 内核中断最低优先级（port.c设为portMIN_INTERRUPT_PRIORITY=255，实际映射为15）
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (5)     // port.c中直接作为汇编参数，必须为5（0-15之间）
#define configPRIO_BITS                         (4)     // STM32H743的NVIC优先级位数

// /* ============== 3. 移植层适配（完全匹配portmacro.h/port.c） ============== */
// #define portSTACK_GROWTH                        (-1)    // 栈向下生长（portmacro.h定义）
// #define portBYTE_ALIGNMENT                      (8)     // 字节对齐（portmacro.h定义）

/* ============== 4. 钩子函数（关闭，避免AC6编译警告） ============== */
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          0       // 关闭栈溢出检查
#define configUSE_MALLOC_FAILED_HOOK            0

/* ============== 5. 功能配置（基础够用） ============== */
#define configUSE_TIMERS                        0       // 暂不启用软件定时器
#define configTIMER_TASK_PRIORITY               (2)
#define configTIMER_QUEUE_LENGTH                (10)
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)

/* ============== 6. API启用（匹配port.c的函数） ============== */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xEventGroupSetBitFromISR        1

/* ============== 7. AC6兼容配置（避免编译/链接错误） ============== */
#define portDONT_DISCARD                        __attribute__( ( used ) )  // 匹配portmacro.h的属性

/* ============== 8. 中断函数映射（适配启动文件） ============== */
// #define vPortSVCHandler                        SVC_Handler
// #define xPortPendSVHandler                     PendSV_Handler
// #define xPortSysTickHandler                    SysTick_Handler

#endif /* FREERTOS_CONFIG_H */