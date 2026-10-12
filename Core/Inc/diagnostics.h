/**
 ******************************************************************************
 * @file    diagnostics.h
 * @brief   System health diagnostics, FreeRTOS heap/stack metrics, and fault
 *          logging for the SecureRT Medical Telemetry Framework.
 ******************************************************************************
 */
#ifndef __DIAGNOSTICS_H
#define __DIAGNOSTICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    FAULT_NONE = 0,
    FAULT_ADC_TIMEOUT,
    FAULT_CRYPTO_INIT,
    FAULT_CRYPTO_ENCRYPT,
    FAULT_COMM_FRAME_ERROR,
    FAULT_COMM_HAL_ERROR,
    FAULT_COMM_TIMEOUT,
    FAULT_QUEUE_FULL,
    FAULT_HEAP_LOW,
    FAULT_STACK_LOW,
    FAULT_IWDG_LATE_REFRESH,
    FAULT_COUNT
} FaultCode_t;

typedef struct
{
    uint32_t free_heap_bytes;
    uint32_t min_ever_free_heap_bytes;
    uint32_t vital_stack_high_watermark;
    uint32_t crypto_stack_high_watermark;
    uint32_t comm_stack_high_watermark;
    uint32_t monitor_stack_high_watermark;
    uint32_t telemetry_queue_used;
    uint32_t crypto_queue_used;
    uint32_t uart_queue_used;
    uint32_t last_encryption_time_us;
} DiagnosticsMetrics_t;

void                 Diagnostics_Init(void);
void                 Diagnostics_ReportFault(FaultCode_t code);
uint32_t             Diagnostics_GetFaultCount(FaultCode_t code);
uint32_t             Diagnostics_GetTotalFaults(void);
DiagnosticsMetrics_t Diagnostics_GetMetrics(void);
void                 Diagnostics_UpdateMetrics(const DiagnosticsMetrics_t *metrics);

/* Non-blocking heartbeat LED toggle on LD2 (PA5) */
void                 Diagnostics_HeartbeatToggle(void);

/* Blocking multi-blink error pattern */
void                 Diagnostics_FaultBlink(uint8_t blink_count);

#ifdef __cplusplus
}
#endif

#endif /* __DIAGNOSTICS_H */
