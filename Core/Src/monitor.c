/**
 ******************************************************************************
 * @file    monitor.c
 * @brief   Supervises worker task liveness, measures FreeRTOS heap/stack/queue
 *          performance metrics, and feeds the IWDG watchdog.
 ******************************************************************************
 */
#include "monitor.h"
#include "diagnostics.h"
#include "system_status.h"
#include "config.h"
#include "iwdg.h"
#include "crypto.h"
#include "app_tasks.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

static volatile uint32_t s_last_checkin_ms[MONITOR_TASK_COUNT];

void Monitor_Init(void)
{
    uint32_t now = osKernelGetTickCount();
    for (int i = 0; i < MONITOR_TASK_COUNT; i++)
    {
        s_last_checkin_ms[i] = now;
    }
}

void Monitor_CheckIn(MonitorTaskId_t task_id)
{
    if (task_id < MONITOR_TASK_COUNT)
    {
        s_last_checkin_ms[task_id] = osKernelGetTickCount();
    }
}

void Monitor_RunChecks(void)
{
    uint32_t now = osKernelGetTickCount();
    bool all_alive = true;

    /* 1. Verify liveness of all supervised tasks */
    for (int i = 0; i < MONITOR_TASK_COUNT; i++)
    {
        if ((now - s_last_checkin_ms[i]) > TASK_STALE_THRESHOLD_MS)
        {
            all_alive = false;
            Diagnostics_ReportFault(FAULT_IWDG_LATE_REFRESH);
        }
    }

    /* 2. Collect RTOS runtime performance metrics */
    DiagnosticsMetrics_t metrics;
    metrics.free_heap_bytes              = (uint32_t)xPortGetFreeHeapSize();
    metrics.min_ever_free_heap_bytes     = (uint32_t)xPortGetMinimumEverFreeHeapSize();

    metrics.vital_stack_high_watermark   = (VitalTaskHandle != NULL) ?
        (uint32_t)uxTaskGetStackHighWaterMark((TaskHandle_t)VitalTaskHandle) : 0U;
    metrics.crypto_stack_high_watermark  = (CryptoTaskHandle != NULL) ?
        (uint32_t)uxTaskGetStackHighWaterMark((TaskHandle_t)CryptoTaskHandle) : 0U;
    metrics.comm_stack_high_watermark    = (CommTaskHandle != NULL) ?
        (uint32_t)uxTaskGetStackHighWaterMark((TaskHandle_t)CommTaskHandle) : 0U;
    metrics.monitor_stack_high_watermark = (MonitorTaskHandle != NULL) ?
        (uint32_t)uxTaskGetStackHighWaterMark((TaskHandle_t)MonitorTaskHandle) : 0U;

    metrics.telemetry_queue_used         = (telemetryQueueHandle != NULL) ?
        (uint32_t)uxQueueMessagesWaiting((QueueHandle_t)telemetryQueueHandle) : 0U;
    metrics.crypto_queue_used            = (cryptoQueueHandle != NULL) ?
        (uint32_t)uxQueueMessagesWaiting((QueueHandle_t)cryptoQueueHandle) : 0U;
    metrics.uart_queue_used              = (uartQueueHandle != NULL) ?
        (uint32_t)uxQueueMessagesWaiting((QueueHandle_t)uartQueueHandle) : 0U;

    metrics.last_encryption_time_us      = Crypto_GetLastEncryptionTimeUs();

    Diagnostics_UpdateMetrics(&metrics);

    /* 3. Check memory & stack health thresholds */
    if (metrics.free_heap_bytes < 512U)
    {
        Diagnostics_ReportFault(FAULT_HEAP_LOW);
    }
    if ((metrics.vital_stack_high_watermark < 32U) ||
        (metrics.crypto_stack_high_watermark < 32U) ||
        (metrics.comm_stack_high_watermark < 32U) ||
        (metrics.monitor_stack_high_watermark < 32U))
    {
        Diagnostics_ReportFault(FAULT_STACK_LOW);
    }

    /* 4. Watchdog gating: only feed IWDG when all tasks are demonstrably alive */
    if (all_alive)
    {
        HAL_IWDG_Refresh(&hiwdg);
        SystemStatus_SetState(SYS_STATE_RUNNING);
    }
    else
    {
        /* Deliberately withhold refresh: IWDG will reset MCU if tasks deadlock */
        SystemStatus_SetState(SYS_STATE_DEGRADED);
    }

    /* 5. Heartbeat LED pulse */
    Diagnostics_HeartbeatToggle();
}
