/**
 ******************************************************************************
 * @file    system_status.h
 * @brief   Top-level thread-safe system status tracking and metrics.
 ******************************************************************************
 */
#ifndef __SYSTEM_STATUS_H
#define __SYSTEM_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    SYS_STATE_INIT = 0,
    SYS_STATE_RUNNING,
    SYS_STATE_DEGRADED,   /* Non-fatal faults observed                        */
    SYS_STATE_FAULT       /* Unrecoverable condition, awaiting IWDG reset     */
} SystemState_t;

typedef struct
{
    SystemState_t state;
    uint32_t      uptime_ms;
    uint32_t      packets_sent;
    uint32_t      packets_dropped;
    uint32_t      crypto_ops;
    uint16_t      last_seq;
    uint32_t      session_id;
} SystemStatus_t;

void           SystemStatus_Init(void);
void           SystemStatus_SetState(SystemState_t new_state);
SystemState_t  SystemStatus_GetState(void);
void           SystemStatus_IncPacketsSent(void);
void           SystemStatus_IncPacketsDropped(void);
void           SystemStatus_IncCryptoOps(void);
void           SystemStatus_SetLastSeq(uint16_t seq);
SystemStatus_t SystemStatus_Snapshot(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_STATUS_H */
