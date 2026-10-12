/**
 ******************************************************************************
 * @file    telemetry.h
 * @brief   Heap-backed producer/consumer helpers moving TelemetrySample_t
 *          and SecurePacket_t objects between FreeRTOS tasks via the
 *          telemetryQueue / cryptoQueue (which, per the .ioc, are declared
 *          as queues-of-uint32_t and here carry heap pointers).
 ******************************************************************************
 */
#ifndef __TELEMETRY_H
#define __TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"
#include "packet.h"

/* Allocates a TelemetrySample_t on the FreeRTOS heap and posts its address
 * (cast to uint32_t) onto the given queue. Returns false and frees the
 * allocation if the queue is full within the timeout. */
bool Telemetry_PostSample(osMessageQueueId_t queue, const TelemetrySample_t *sample, uint32_t timeout_ms);

/* Blocking receive of a TelemetrySample_t* previously posted with
 * Telemetry_PostSample(). Caller owns the returned pointer and must free
 * it with Telemetry_ReleaseSample() once done. */
TelemetrySample_t *Telemetry_ReceiveSample(osMessageQueueId_t queue, uint32_t timeout_ms);
void                Telemetry_ReleaseSample(TelemetrySample_t *sample);

/* Same pattern for fully-encrypted SecurePacket_t objects flowing from
 * CryptoTask to CommTask. */
bool             Telemetry_PostPacket(osMessageQueueId_t queue, const SecurePacket_t *pkt, uint32_t timeout_ms);
SecurePacket_t  *Telemetry_ReceivePacket(osMessageQueueId_t queue, uint32_t timeout_ms);
void             Telemetry_ReleasePacket(SecurePacket_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* __TELEMETRY_H */
