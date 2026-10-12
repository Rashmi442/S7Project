/**
 ******************************************************************************
 * @file    packet.h
 * @brief   Telemetry wire-protocol framing, serialization, and CRC16-CCITT.
 *
 * SMTF v1.0 Wire Frame Layout (all little-endian, total 77 bytes):
 *
 *   [0]      SYNC0           0xAA
 *   [1]      SYNC1           0x55
 *   [2]      VERSION         0x01 (SMTF v1.0)
 *   [3]      TYPE            0x01 (1=Telemetry, 2=Diagnostic, 3=Heartbeat)
 *   [4:5]    SEQ             uint16_t (Monotonic sequence number)
 *   [6]      CIPHER_LEN      uint8_t (40 bytes payload)
 *   [7:18]   NONCE           12 bytes (96-bit AES-GCM IV: 4B session + 8B counter)
 *   [19:58]  CIPHERTEXT      40 bytes (AES-128-GCM encrypted TelemetrySample_t)
 *   [59:74]  TAG             16 bytes (128-bit GCM authentication tag)
 *   [75:76]  CRC16-CCITT     uint16_t (Calculated over bytes [0..74])
 ******************************************************************************
 */
#ifndef __PACKET_H
#define __PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "config.h"

/* --- Protocol Constants ----------------------------------------------- */
#define PACKET_SYNC0                PACKET_SYNC_BYTE_0 /* 0xAA */
#define PACKET_SYNC1                PACKET_SYNC_BYTE_1 /* 0x55 */
#define PACKET_VERSION              PROTOCOL_VERSION   /* 0x01 */

/* Additional Authenticated Data (AAD) header length:
 * sync0 (1) + sync1 (1) + version (1) + type (1) + seq (2) + cipher_len (1) = 7 */
#define PACKET_AAD_LEN              7U
#define PACKET_HEADER_LEN           (PACKET_AAD_LEN + CRYPTO_NONCE_BYTES) /* 19 */
#define PACKET_CRC_LEN              2U

/* Total wire frame length for standard 40-byte TelemetrySample_t:
 * 7 (AAD) + 12 (Nonce) + 40 (Ciphertext) + 16 (Tag) + 2 (CRC) = 77 */
#define PACKET_TOTAL_FRAME_LEN      (PACKET_HEADER_LEN + PACKET_SAMPLE_PLAINTEXT_LEN + CRYPTO_TAG_BYTES + PACKET_CRC_LEN)

/* --- Clinical Alert & Emergency Bitmask Definitions ------------------- */
#define ALERT_LEVEL_NORMAL          0x0000U
#define ALERT_LEVEL_WARNING         0x0001U
#define ALERT_LEVEL_CRITICAL        0x0002U
#define ALERT_LEVEL_MASK            0x0003U

#define ALERT_FLAG_TACHYCARDIA      0x0004U /* HR > Warning/Critical threshold */
#define ALERT_FLAG_BRADYCARDIA      0x0008U /* HR < 50 bpm                     */
#define ALERT_FLAG_HYPOXIA          0x0010U /* SpO2 < 90%                      */
#define ALERT_FLAG_FEVER            0x0020U /* Temp > 38.5 C                   */
#define ALERT_FLAG_HYPOTHERMIA      0x0040U /* Temp < 35.0 C                   */
#define ALERT_FLAG_ARRHYTHMIA       0x0080U /* Abnormal cardiac rhythm         */
#define ALERT_FLAG_SIM_MODE_ACTIVE  0x8000U /* Diagnostic/Demo injection active*/

#pragma pack(push, 1)

/**
 * @brief Plaintext telemetry sample payload (40 bytes total).
 *        Produced by VitalTask, sealed by CryptoTask, authenticated on Host.
 */
typedef struct
{
    uint32_t timestamp_ms;                  /* MCU uptime in milliseconds       */
    uint16_t seq;                           /* Monotonic sample sequence number */
    int16_t  ecg_samples[ECG_SAMPLES_PER_PACKET]; /* 10 raw Lead-II samples (20 B)*/
    float    heart_rate_bpm;                /* Heart rate in beats-per-minute   */
    float    spo2_pct;                      /* Blood oxygen saturation in %     */
    float    temperature_c;                 /* Core body temperature in Celsius */
    uint16_t alert_flags;                   /* Emergency level and alert bitmask*/
} TelemetrySample_t;

/**
 * @brief In-memory representation of an encrypted secure packet.
 */
typedef struct
{
    uint8_t  sync0;
    uint8_t  sync1;
    uint8_t  version;
    uint8_t  type;
    uint16_t seq;
    uint8_t  cipher_len;
    uint8_t  nonce[CRYPTO_NONCE_BYTES];
    uint8_t  ciphertext[PACKET_MAX_PLAINTEXT_LEN];
    uint8_t  tag[CRYPTO_TAG_BYTES];
    uint16_t crc16;
} SecurePacket_t;

#pragma pack(pop)

/* --- Helper APIs ------------------------------------------------------ */
size_t   Packet_SerializeSample(const TelemetrySample_t *sample, uint8_t *out_buf, size_t out_buf_size);
uint16_t Packet_CRC16(const uint8_t *data, size_t len);
size_t   Packet_BuildFrame(SecurePacket_t *pkt, uint8_t *tx_buf, size_t tx_buf_size);

#ifdef __cplusplus
}
#endif

#endif /* __PACKET_H */
