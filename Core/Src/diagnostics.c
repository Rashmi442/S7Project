/**
 ******************************************************************************
 * @file    diagnostics.c
 * @brief   System health diagnostics, FreeRTOS heap/stack metrics, and fault
 *          logging implementation.
 ******************************************************************************
 */
#include <string.h>
#include "diagnostics.h"
#include "gpio.h"
#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

static uint32_t             s_fault_counts[FAULT_COUNT];
static uint32_t             s_total_faults = 0U;
static DiagnosticsMetrics_t s_metrics;

void Diagnostics_Init(void)
{
    for (int i = 0; i < FAULT_COUNT; i++)
    {
        s_fault_counts[i] = 0U;
    }
    s_total_faults = 0U;
    memset(&s_metrics, 0, sizeof(s_metrics));
}

void Diagnostics_ReportFault(FaultCode_t code)
{
    if (code < FAULT_COUNT)
    {
        taskENTER_CRITICAL();
        s_fault_counts[code]++;
        s_total_faults++;
        taskEXIT_CRITICAL();
    }
}

uint32_t Diagnostics_GetFaultCount(FaultCode_t code)
{
    uint32_t count = 0U;
    if (code < FAULT_COUNT)
    {
        taskENTER_CRITICAL();
        count = s_fault_counts[code];
        taskEXIT_CRITICAL();
    }
    return count;
}

uint32_t Diagnostics_GetTotalFaults(void)
{
    return s_total_faults;
}

DiagnosticsMetrics_t Diagnostics_GetMetrics(void)
{
    taskENTER_CRITICAL();
    DiagnosticsMetrics_t copy = s_metrics;
    taskEXIT_CRITICAL();
    return copy;
}

void Diagnostics_UpdateMetrics(const DiagnosticsMetrics_t *metrics)
{
    if (metrics != NULL)
    {
        taskENTER_CRITICAL();
        s_metrics = *metrics;
        taskEXIT_CRITICAL();
    }
}

void Diagnostics_HeartbeatToggle(void)
{
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
}

void Diagnostics_FaultBlink(uint8_t blink_count)
{
    for (uint8_t i = 0U; i < blink_count; i++)
    {
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
        for (volatile uint32_t d = 0; d < 200000; d++) { }
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
        for (volatile uint32_t d = 0; d < 200000; d++) { }
    }
}
