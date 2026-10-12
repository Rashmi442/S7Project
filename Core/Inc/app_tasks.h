/**
 ******************************************************************************
 * @file    app_tasks.h
 * @brief   FreeRTOS task declarations and RTOS object handles.
 ******************************************************************************
 */
#ifndef __APP_TASKS_H
#define __APP_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h"
#include <stdint.h>

/* --- RTOS Queue Handles ----------------------------------------------- */
extern osMessageQueueId_t telemetryQueueHandle;
extern osMessageQueueId_t cryptoQueueHandle;
extern osMessageQueueId_t uartQueueHandle;

/* --- RTOS Synchronization Handles ------------------------------------- */
extern osSemaphoreId_t    uartTxSemaphoreHandle;
extern osMutexId_t        uartMutexHandle;

/* --- RTOS Task Handles ------------------------------------------------ */
extern osThreadId_t VitalTaskHandle;
extern osThreadId_t CryptoTaskHandle;
extern osThreadId_t CommTaskHandle;
extern osThreadId_t MonitorTaskHandle;

/* --- Task Entry Functions --------------------------------------------- */
void StartDefaultTask(void *argument); /* VitalTask   (Priority: High)   */
void StartTask02(void *argument);      /* CryptoTask  (Priority: High)   */
void StartTask03(void *argument);      /* CommTask    (Priority: Medium) */
void StartTask04(void *argument);      /* MonitorTask (Priority: Low)    */

#ifdef __cplusplus
}
#endif

#endif /* __APP_TASKS_H */
