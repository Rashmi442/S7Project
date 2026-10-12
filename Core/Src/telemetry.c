/**
 ******************************************************************************
 * @file    telemetry.c
 * @brief   Heap-backed inter-task telemetry object passing.
 ******************************************************************************
 */
#include <string.h>
#include "telemetry.h"
#include "FreeRTOS.h"
#include "task.h"

bool Telemetry_PostSample(osMessageQueueId_t queue, const TelemetrySample_t *sample, uint32_t timeout_ms)
{
    if ((queue == NULL) || (sample == NULL))
    {
        return false;
    }

    TelemetrySample_t *heap_copy = (TelemetrySample_t *)pvPortMalloc(sizeof(TelemetrySample_t));
    if (heap_copy == NULL)
    {
        return false; /* Out of heap -- drop sample rather than block forever */
    }
    memcpy(heap_copy, sample, sizeof(TelemetrySample_t));

    uint32_t addr = (uint32_t)(uintptr_t)heap_copy;
    if (osMessageQueuePut(queue, &addr, 0U, timeout_ms) != osOK)
    {
        vPortFree(heap_copy);
        return false;
    }
    return true;
}

TelemetrySample_t *Telemetry_ReceiveSample(osMessageQueueId_t queue, uint32_t timeout_ms)
{
    uint32_t addr = 0U;
    if (queue == NULL)
    {
        return NULL;
    }
    if (osMessageQueueGet(queue, &addr, NULL, timeout_ms) != osOK)
    {
        return NULL;
    }
    return (TelemetrySample_t *)(uintptr_t)addr;
}

void Telemetry_ReleaseSample(TelemetrySample_t *sample)
{
    if (sample != NULL)
    {
        vPortFree(sample);
    }
}

bool Telemetry_PostPacket(osMessageQueueId_t queue, const SecurePacket_t *pkt, uint32_t timeout_ms)
{
    if ((queue == NULL) || (pkt == NULL))
    {
        return false;
    }

    SecurePacket_t *heap_copy = (SecurePacket_t *)pvPortMalloc(sizeof(SecurePacket_t));
    if (heap_copy == NULL)
    {
        return false;
    }
    memcpy(heap_copy, pkt, sizeof(SecurePacket_t));

    uint32_t addr = (uint32_t)(uintptr_t)heap_copy;
    if (osMessageQueuePut(queue, &addr, 0U, timeout_ms) != osOK)
    {
        vPortFree(heap_copy);
        return false;
    }
    return true;
}

SecurePacket_t *Telemetry_ReceivePacket(osMessageQueueId_t queue, uint32_t timeout_ms)
{
    uint32_t addr = 0U;
    if (queue == NULL)
    {
        return NULL;
    }
    if (osMessageQueueGet(queue, &addr, NULL, timeout_ms) != osOK)
    {
        return NULL;
    }
    return (SecurePacket_t *)(uintptr_t)addr;
}

void Telemetry_ReleasePacket(SecurePacket_t *pkt)
{
    if (pkt != NULL)
    {
        vPortFree(pkt);
    }
}
