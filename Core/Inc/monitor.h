/**
 ******************************************************************************
 * @file    monitor.h
 * @brief   Watchdog-gated liveness supervision, FreeRTOS heap/stack/queue
 *          monitoring for the worker tasks.
 ******************************************************************************
 */
#ifndef __MONITOR_H
#define __MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    MONITOR_TASK_VITAL = 0,
    MONITOR_TASK_CRYPTO,
    MONITOR_TASK_COMM,
    MONITOR_TASK_COUNT
} MonitorTaskId_t;

void Monitor_Init(void);
void Monitor_CheckIn(MonitorTaskId_t task_id);
void Monitor_RunChecks(void);

#ifdef __cplusplus
}
#endif

#endif /* __MONITOR_H */
