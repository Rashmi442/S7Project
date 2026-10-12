/**
 ******************************************************************************
 * @file    FreeRTOSConfig.h
 * @brief   FreeRTOS config for STM32F411RE, CMSIS-RTOS2 (CMSIS_V2) interface.
 *          Values mirror the .ioc FREERTOS.configTOTAL_HEAP_SIZE=16384 and
 *          the project's Cortex-M4F / 84 MHz system clock.
 ******************************************************************************
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
extern uint32_t SystemCoreClock;

/* Required by Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/
 * freertos_os2.h -- tells it which CMSIS device header to pull in for
 * IRQn_Type, SysTick, NVIC_SetPriority, etc. Without this, cmsis_os2.c
 * fails to compile (the errors you just hit). */
#define CMSIS_device_header "stm32f4xx.h"

#define configUSE_PREEMPTION                    1
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configUSE_IDLE_HOOK                      1
#define configUSE_TICK_HOOK                      0
#define configCPU_CLOCK_HZ                       (SystemCoreClock)
#define configTICK_RATE_HZ                       ((TickType_t)1000)
#define configMAX_PRIORITIES                     (56)
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                    ((size_t)32768)
#define configMAX_TASK_NAME_LEN                  16
#define configUSE_16_BIT_TICKS                   0
#define configUSE_MUTEXES                        1
#define configQUEUE_REGISTRY_SIZE                8
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
#define configUSE_TICKLESS_IDLE                  0

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          (2)

/* Software timer definitions. */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                (2)
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             256

/* Optional functions - most linked only if used. */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskCleanUpResources             0
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTimerPendFunctionCall           1
#define INCLUDE_xQueueGetMutexHolder             1
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_eTaskGetState                    1
#define INCLUDE_xTaskGetIdleTaskHandle            1

/* Required by the CMSIS-RTOS v2 wrapper (freertos_os2.h) for
 * osThreadEnumerate() / osThreadGetName() support. */
#define configUSE_TRACE_FACILITY                 1
#define configUSE_STATS_FORMATTING_FUNCTIONS      0

#ifdef __NVIC_PRIO_BITS
#define configPRIO_BITS                          __NVIC_PRIO_BITS
#else
#define configPRIO_BITS                          4
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY     5

#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configASSERT(x) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );}

/* Cortex-M specific definitions. */
#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler

/* IMPORTANT: SysTick handler is implemented in stm32f4xx_it.c and calls
 * both HAL_IncTick() and osSystickHandler()/xPortSysTickHandler(). */
//#define xPortSysTickHandler SysTick_Handler

#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION 1

#define configUSE_NEWLIB_REENTRANT       1

/* CMSIS-RTOS2 mapping */
#define configENABLE_FPU                 1
#define configENABLE_MPU                 0

#endif /* FREERTOS_CONFIG_H */
