/**
 ******************************************************************************
 * @file    app_tasks.c
 * @brief   VitalTask / CryptoTask / CommTask / MonitorTask implementations.
 *          Implements single-master adaptive rate scheduling and event-driven
 *          cryptographic / communications processing.
 ******************************************************************************
 */
#include "app_tasks.h"
#include "config.h"
#include "sensors.h"
#include "telemetry.h"
#include "crypto.h"
#include "communication.h"
#include "monitor.h"
#include "system_status.h"
#include "diagnostics.h"

/**
 * @brief VitalTask (StartDefaultTask): Primary master scheduler.
 *        Acquires sensor data, performs autonomous clinical threshold
 *        evaluation, posts plaintext samples to telemetryQueue, and adjusts
 *        its task execution cadence dynamically according to the alert level.
 */
void StartDefaultTask(void *argument)
{
    (void)argument;
    Sensors_Init();

    uint16_t seq = 0U;
    for (;;)
    {
        TelemetrySample_t sample;
        Sensors_AcquireSample(&sample, seq++);

        /* Determine dynamic adaptive period from evaluated clinical severity */
        uint32_t period_ms = RATE_NORMAL_PERIOD_MS;
        uint16_t level = sample.alert_flags & ALERT_LEVEL_MASK;

        if (level == ALERT_LEVEL_CRITICAL)
        {
            period_ms = RATE_CRITICAL_PERIOD_MS; /* 100 ms (10 Hz) */
        }
        else if (level == ALERT_LEVEL_WARNING)
        {
            period_ms = RATE_WARNING_PERIOD_MS;  /* 500 ms (2 Hz)  */
        }
        else
        {
            period_ms = RATE_NORMAL_PERIOD_MS;   /* 1000 ms (1 Hz) */
        }

        /* Post sample to telemetryQueue */
        if (!Telemetry_PostSample(telemetryQueueHandle, &sample, 20U))
        {
            SystemStatus_IncPacketsDropped();
            Diagnostics_ReportFault(FAULT_QUEUE_FULL);
        }

        Monitor_CheckIn(MONITOR_TASK_VITAL);

        /* Paced delay: account for the 10ms spent in ECG sub-sample burst */
        uint32_t burst_time_ms = ECG_SAMPLES_PER_PACKET * 1U;
        uint32_t delay_ms = (period_ms > burst_time_ms) ? (period_ms - burst_time_ms) : 10U;
        osDelay(delay_ms);
    }
}

/**
 * @brief CryptoTask (StartTask02): Event-driven cryptographic pipeline.
 *        Blocks on telemetryQueue, seals plaintext using AES-128-GCM,
 *        attaches unique 96-bit nonce and 128-bit authentication tag,
 *        and passes the secured packet to cryptoQueue.
 */
void StartTask02(void *argument)
{
    (void)argument;

    if (Crypto_Init() != CRYPTO_OK)
    {
        Diagnostics_ReportFault(FAULT_CRYPTO_INIT);
        SystemStatus_SetState(SYS_STATE_FAULT);
    }

    for (;;)
    {
        TelemetrySample_t *sample = Telemetry_ReceiveSample(telemetryQueueHandle, osWaitForever);
        if (sample == NULL)
        {
            continue;
        }

        SecurePacket_t pkt;
        CryptoStatus_t status = Crypto_EncryptSample(sample, sample->seq, PACKET_TYPE_TELEMETRY, &pkt);
        Telemetry_ReleaseSample(sample);

        if (status == CRYPTO_OK)
        {
            SystemStatus_IncCryptoOps();
            SystemStatus_SetLastSeq(pkt.seq);

            if (!Telemetry_PostPacket(cryptoQueueHandle, &pkt, 20U))
            {
                SystemStatus_IncPacketsDropped();
                Diagnostics_ReportFault(FAULT_QUEUE_FULL);
            }
        }
        else
        {
            Diagnostics_ReportFault(FAULT_CRYPTO_ENCRYPT);
        }

        Monitor_CheckIn(MONITOR_TASK_CRYPTO);
    }
}

/**
 * @brief CommTask (StartTask03): Non-blocking communications handler.
 *        Blocks on cryptoQueue, packages wire frame with trailing CRC16-CCITT,
 *        and initiates background USART2 DMA transmission guarded by mutex.
 */
void StartTask03(void *argument)
{
    (void)argument;
    Communication_Init();

    for (;;)
    {
        SecurePacket_t *pkt = Telemetry_ReceivePacket(cryptoQueueHandle, osWaitForever);
        if (pkt == NULL)
        {
            continue;
        }

        CommStatus_t status = Communication_SendPacket(pkt, 100U);
        Telemetry_ReleasePacket(pkt);

        if (status == COMM_OK)
        {
            SystemStatus_IncPacketsSent();
        }
        else
        {
            SystemStatus_IncPacketsDropped();
        }

        Monitor_CheckIn(MONITOR_TASK_COMM);
    }
}

/**
 * @brief MonitorTask (StartTask04): Low-priority supervisory daemon.
 *        Measures FreeRTOS heap memory, task stack high watermarks,
 *        queue utilization, validates task check-in liveness, and feeds IWDG.
 */
void StartTask04(void *argument)
{
    (void)argument;
    Diagnostics_Init();
    Monitor_Init();
    SystemStatus_Init();
    SystemStatus_SetState(SYS_STATE_RUNNING);

    for (;;)
    {
        Monitor_RunChecks();
        osDelay(MONITOR_PERIOD_MS);
    }
}
