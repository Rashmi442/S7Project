/**
 ******************************************************************************
 * @file    crypto.h
 * @brief   AES-128-GCM authenticated encryption wrapper using mbedTLS.
 *          Implements robust session-based 96-bit nonce lifecycle management.
 ******************************************************************************
 */
#ifndef __CRYPTO_H
#define __CRYPTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "packet.h"

typedef enum
{
    CRYPTO_OK = 0,
    CRYPTO_ERR_INIT,
    CRYPTO_ERR_ENCRYPT,
    CRYPTO_ERR_DECRYPT,
    CRYPTO_ERR_AUTH_FAIL,
    CRYPTO_ERR_LENGTH,
    CRYPTO_ERR_NULL_PARAM
} CryptoStatus_t;

/* ===== LIVE EXPRESSIONS / DEBUG WATCH EXPORTS ===== */
extern volatile uint32_t          g_debug_stage;
extern volatile uint32_t          g_debug_encrypt_status;
extern volatile uint32_t          g_debug_decrypt_status;
extern volatile uint32_t          g_debug_packet_count;
extern volatile uint32_t          g_debug_enc_time_us;

/* 1. Plaintext data before encryption */
extern volatile TelemetrySample_t g_debug_plain_sample;
extern volatile uint8_t           g_debug_plaintext[PACKET_SAMPLE_PLAINTEXT_LEN];
extern volatile uint8_t           g_debug_plain_len;

/* 2. Nonce / IV (96-bit) */
extern volatile uint8_t           g_debug_nonce[CRYPTO_NONCE_BYTES];

/* 3. Encrypted ciphertext & 128-bit authentication tag */
extern volatile uint8_t           g_debug_ciphertext[PACKET_SAMPLE_PLAINTEXT_LEN];
extern volatile uint8_t           g_debug_cipher_len;
extern volatile uint8_t           g_debug_tag[CRYPTO_TAG_BYTES];

/* 4. Decrypted sample */
extern volatile TelemetrySample_t g_debug_decrypted_sample;

/**
 * @brief Initializes mbedTLS GCM context with g_aes_key, generates a unique
 *        32-bit session ID for this boot cycle, and resets the monotonic counter.
 */
CryptoStatus_t Crypto_Init(void);

/**
 * @brief Returns the active 32-bit boot session ID.
 */
uint32_t Crypto_GetSessionId(void);

/**
 * @brief Returns the current 64-bit monotonic nonce counter value.
 */
uint64_t Crypto_GetNonceCounter(void);

/**
 * @brief Generates a deterministic 96-bit nonce:
 *        Bytes 0..3:  Unique 32-bit Session ID (fixed per boot)
 *        Bytes 4..11: Monotonically increasing 64-bit counter (incremented per packet)
 */
void Crypto_GenerateNonce(uint8_t nonce_out[CRYPTO_NONCE_BYTES]);

/**
 * @brief Seals sample plaintext into pkt_out with AES-128-GCM.
 *        The 7-byte wire header is bound as Additional Authenticated Data (AAD).
 */
CryptoStatus_t Crypto_EncryptSample(const TelemetrySample_t *sample, uint16_t seq, uint8_t type, SecurePacket_t *pkt_out);

/**
 * @brief Verifies GCM auth tag and decrypts ciphertext back to TelemetrySample_t.
 */
CryptoStatus_t Crypto_DecryptSample(const SecurePacket_t *pkt, TelemetrySample_t *sample_out);

/**
 * @brief Returns the execution duration of the last encryption in microseconds.
 */
uint32_t Crypto_GetLastEncryptionTimeUs(void);

#ifdef __cplusplus
}
#endif

#endif /* __CRYPTO_H */
