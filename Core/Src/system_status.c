/**
 ******************************************************************************
 * @file    system_status.c
 * @brief   Thread-safe system status tracking and snapshotting.
 ******************************************************************************
 */
#include "system_status.h"
#include "crypto.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

static SystemStatus_t s_status;

void SystemStatus_Init(void)
{
    s_status.state           = SYS_STATE_INIT;
    s_status.uptime_ms       = 0U;
    s_status.packets_sent    = 0U;
    s_status.packets_dropped = 0U;
    s_status.crypto_ops      = 0U;
    s_status.last_seq        = 0U;
    s_status.session_id      = 0U;
}

void SystemStatus_SetState(SystemState_t new_state)
{
    taskENTER_CRITICAL();
    s_status.state = new_state;
    taskEXIT_CRITICAL();
}

SystemState_t SystemStatus_GetState(void)
{
    return s_status.state;
}

void SystemStatus_IncPacketsSent(void)
{
    taskENTER_CRITICAL();
    s_status.packets_sent++;
    taskEXIT_CRITICAL();
}

void SystemStatus_IncPacketsDropped(void)
{
    taskENTER_CRITICAL();
    s_status.packets_dropped++;
    taskEXIT_CRITICAL();
}

void SystemStatus_IncCryptoOps(void)
{
    taskENTER_CRITICAL();
    s_status.crypto_ops++;
    taskEXIT_CRITICAL();
}

void SystemStatus_SetLastSeq(uint16_t seq)
{
    taskENTER_CRITICAL();
    s_status.last_seq = seq;
    taskEXIT_CRITICAL();
}

SystemStatus_t SystemStatus_Snapshot(void)
{
    taskENTER_CRITICAL();
    s_status.uptime_ms  = osKernelGetTickCount();
    s_status.session_id = Crypto_GetSessionId();
    SystemStatus_t snapshot = s_status;
    taskEXIT_CRITICAL();

    return snapshot;
}
