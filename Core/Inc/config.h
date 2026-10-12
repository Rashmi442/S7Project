/**
 ******************************************************************************
 * @file    config.h
 * @brief   Global compile-time configuration for the Secure Real-Time
 *          Medical Telemetry Framework (SMTF v1.0).
 ******************************************************************************
 */
#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cmsis_os2.h"

/* ---------------------------------------------------------------------- */
/* Hardware / Sensor Source Selection                                     */
/* ---------------------------------------------------------------------- */
/* Set to 0 for synthetic clinical simulation (demo/evaluation mode)
 * Set to 1 to pull data from physical sensors (AD8232 via ADC1, MAX30102/MAX30205 via I2C1) */
#define USE_REAL_HARDWARE_SENSORS   0U

/* ---------------------------------------------------------------------- */
/* Firmware / Protocol Identity                                           */
/* ---------------------------------------------------------------------- */
#define FW_VERSION_MAJOR            1
#define FW_VERSION_MINOR            0
#define FW_VERSION_PATCH            0
#define PROTOCOL_VERSION            0x01U     /* Wire protocol version 1.0 */
#define DEVICE_ID                   0x0001U   /* Unique telemetry node ID  */

/* ---------------------------------------------------------------------- */
/* Sampling & Adaptive Transmission Rates                                 */
/* ---------------------------------------------------------------------- */
#define ECG_SAMPLE_RATE_HZ          250U      /* 250 Hz ECG sampling rate  */
#define ECG_SAMPLE_PERIOD_MS        (1000U / ECG_SAMPLE_RATE_HZ) /* 4 ms   */
#define ECG_SAMPLES_PER_PACKET      10U       /* 10 samples (40ms ECG/pkt) */

/* Adaptive transmission periods governed by VitalTask */
#define RATE_NORMAL_PERIOD_MS       1000U     /* Normal vitals: 1 Hz       */
#define RATE_WARNING_PERIOD_MS      500U      /* Warning vitals: 2 Hz      */
#define RATE_CRITICAL_PERIOD_MS     100U      /* Critical vitals: 10 Hz    */

#define MONITOR_PERIOD_MS           500U      /* MonitorTask check period  */
#define TASK_STALE_THRESHOLD_MS     3000U     /* Liveness supervision limit*/
#define IWDG_REFRESH_PERIOD_MS      500U      /* Watchdog refresh cadence  */

/* ---------------------------------------------------------------------- */
/* FreeRTOS Task Priorities & Stack Sizes                                 */
/* ---------------------------------------------------------------------- */
#define TASK_PRIORITY_VITAL         ((osPriority_t)osPriorityAboveNormal)
#define TASK_PRIORITY_CRYPTO        ((osPriority_t)osPriorityHigh)
#define TASK_PRIORITY_COMM          ((osPriority_t)osPriorityNormal)
#define TASK_PRIORITY_MONITOR       ((osPriority_t)osPriorityLow)

#define TASK_STACK_VITAL_WORDS      512U
#define TASK_STACK_CRYPTO_WORDS     1024U
#define TASK_STACK_COMM_WORDS       768U
#define TASK_STACK_MONITOR_WORDS    512U

/* ---------------------------------------------------------------------- */
/* Queue Capacities                                                       */
/* ---------------------------------------------------------------------- */
#define TELEMETRY_QUEUE_LEN         10U
#define CRYPTO_QUEUE_LEN            10U
#define UART_QUEUE_LEN              10U

/* ---------------------------------------------------------------------- */
/* Cryptographic Parameters (AES-128-GCM via mbedTLS)                     */
/* ---------------------------------------------------------------------- */
#define CRYPTO_KEY_BITS             128U
#define CRYPTO_KEY_BYTES            (CRYPTO_KEY_BITS / 8U)   /* 16 bytes  */
#define CRYPTO_NONCE_BYTES          12U                      /* 96 bits   */
#define CRYPTO_TAG_BYTES            16U                      /* 128 bits  */

/* Pre-shared demonstration key (symmetric AES-128-GCM) */
extern const uint8_t g_aes_key[CRYPTO_KEY_BYTES];

/* ---------------------------------------------------------------------- */
/* Wire Protocol Framing & Constants                                      */
/* ---------------------------------------------------------------------- */
#define PACKET_SYNC_BYTE_0          0xAAU
#define PACKET_SYNC_BYTE_1          0x55U

#define PACKET_TYPE_TELEMETRY       0x01U
#define PACKET_TYPE_DIAGNOSTIC      0x02U
#define PACKET_TYPE_HEARTBEAT       0x03U

#define PACKET_MAX_PLAINTEXT_LEN    64U
#define PACKET_SAMPLE_PLAINTEXT_LEN 40U   /* Size of TelemetrySample_t    */

/* ---------------------------------------------------------------------- */
/* Clinical Physiological Bounds & Emergency Thresholds                   */
/* ---------------------------------------------------------------------- */
#define HR_BASELINE_BPM             75.0f
#define SPO2_BASELINE_PCT           98.5f
#define TEMP_BASELINE_C             36.8f

#define HR_BRADYCARDIA_THRESH_BPM   50.0f
#define HR_TACHYCARDIA_WARN_BPM     100.0f
#define HR_TACHYCARDIA_CRIT_BPM     125.0f

#define SPO2_WARN_THRESH_PCT        94.0f
#define SPO2_CRIT_THRESH_PCT        90.0f

#define TEMP_FEVER_WARN_C           37.8f
#define TEMP_FEVER_CRIT_C           39.0f
#define TEMP_HYPOTHERMIA_CRIT_C     35.0f

void Config_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H */
