/**
 ******************************************************************************
 * @file    crypto.c
 * @brief   AES-128-GCM authenticated encryption using mbedTLS library.
 *          Implements session-based 96-bit nonce generation and performance
 *          profiling.
 ******************************************************************************
 */
#include <string.h>
#include "crypto.h"
#include "config.h"
#include "adc.h"
#include "mbedtls/gcm.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal.h"

static mbedtls_gcm_context s_gcm_ctx;
static uint32_t            s_session_id     = 0U;
static uint64_t            s_nonce_counter  = 0ULL;
static bool                s_initialized    = false;
static uint32_t            s_last_enc_time_us = 0U;

/**
 * @brief Generates a unique 32-bit session ID combining MCU 96-bit Unique ID,
 *        SysTick timer register, and ADC analog noise.
 */
static uint32_t generate_session_id(void)
{
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();
    uint32_t tick = HAL_GetTick();
    uint32_t adc_raw = ADC_ReadRaw();

    /* 32-bit FNV-1a hash */
    uint32_t hash = 2166136261UL;
    uint32_t inputs[5] = { uid0, uid1, uid2, tick, adc_raw };

    for (int i = 0; i < 5; i++)
    {
        uint32_t val = inputs[i];
        for (int b = 0; b < 4; b++)
        {
            hash ^= (uint8_t)(val >> (b * 8));
            hash *= 16777619UL;
        }
    }

    /* Ensure non-zero session ID */
    if (hash == 0U)
    {
        hash = 0xA5A55A5AU;
    }
    return hash;
}

CryptoStatus_t Crypto_Init(void)
{
    /* Enable DWT cycle counter for precise microsecond benchmarking */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    mbedtls_gcm_init(&s_gcm_ctx);

    int ret = mbedtls_gcm_setkey(&s_gcm_ctx, MBEDTLS_CIPHER_ID_AES, g_aes_key, CRYPTO_KEY_BITS);
    if (ret != 0)
    {
        return CRYPTO_ERR_INIT;
    }

    s_session_id    = generate_session_id();
    s_nonce_counter = 0ULL;
    s_initialized   = true;
    return CRYPTO_OK;
}

uint32_t Crypto_GetSessionId(void)
{
    return s_session_id;
}

uint64_t Crypto_GetNonceCounter(void)
{
    return s_nonce_counter;
}

uint32_t Crypto_GetLastEncryptionTimeUs(void)
{
    return s_last_enc_time_us;
}

void Crypto_GenerateNonce(uint8_t nonce_out[CRYPTO_NONCE_BYTES])
{
    uint32_t session = s_session_id;
    uint64_t counter = s_nonce_counter++;

    /* Bytes 0..3: Session ID (Little Endian) */
    nonce_out[0] = (uint8_t)(session & 0xFFU);
    nonce_out[1] = (uint8_t)((session >> 8) & 0xFFU);
    nonce_out[2] = (uint8_t)((session >> 16) & 0xFFU);
    nonce_out[3] = (uint8_t)((session >> 24) & 0xFFU);

    /* Bytes 4..11: Monotonic 64-bit counter (Little Endian) */
    for (int i = 0; i < 8; i++)
    {
        nonce_out[4 + i] = (uint8_t)((counter >> (i * 8)) & 0xFFU);
    }
}

CryptoStatus_t Crypto_EncryptSample(const TelemetrySample_t *sample, uint16_t seq, uint8_t type, SecurePacket_t *pkt_out)
{
    if (!s_initialized)
    {
        return CRYPTO_ERR_INIT;
    }
    if ((sample == NULL) || (pkt_out == NULL))
    {
        return CRYPTO_ERR_NULL_PARAM;
    }

    uint8_t plaintext[PACKET_SAMPLE_PLAINTEXT_LEN];
    size_t plain_len = Packet_SerializeSample(sample, plaintext, sizeof(plaintext));
    if (plain_len != PACKET_SAMPLE_PLAINTEXT_LEN)
    {
        return CRYPTO_ERR_LENGTH;
    }

    memset(pkt_out, 0, sizeof(SecurePacket_t));
    pkt_out->sync0      = PACKET_SYNC0;
    pkt_out->sync1      = PACKET_SYNC1;
    pkt_out->version    = PACKET_VERSION;
    pkt_out->type       = type;
    pkt_out->seq        = seq;
    pkt_out->cipher_len = (uint8_t)plain_len;

    Crypto_GenerateNonce(pkt_out->nonce);

    /* AAD: 7 bytes (sync0, sync1, version, type, seq[2], cipher_len) */
    uint8_t aad[PACKET_AAD_LEN];
    aad[0] = pkt_out->sync0;
    aad[1] = pkt_out->sync1;
    aad[2] = pkt_out->version;
    aad[3] = pkt_out->type;
    aad[4] = (uint8_t)(pkt_out->seq & 0xFFU);
    aad[5] = (uint8_t)((pkt_out->seq >> 8) & 0xFFU);
    aad[6] = pkt_out->cipher_len;

    uint32_t start_cycles = DWT->CYCCNT;

    int ret = mbedtls_gcm_crypt_and_tag(
        &s_gcm_ctx, MBEDTLS_GCM_ENCRYPT,
        plain_len,
        pkt_out->nonce, CRYPTO_NONCE_BYTES,
        aad, sizeof(aad),
        plaintext, pkt_out->ciphertext,
        CRYPTO_TAG_BYTES, pkt_out->tag);

    uint32_t end_cycles = DWT->CYCCNT;
    uint32_t cycles = (end_cycles >= start_cycles) ? (end_cycles - start_cycles) : (UINT32_MAX - start_cycles + end_cycles);
    s_last_enc_time_us = cycles / (SystemCoreClock / 1000000U);

    return (ret == 0) ? CRYPTO_OK : CRYPTO_ERR_ENCRYPT;
}

CryptoStatus_t Crypto_DecryptSample(const SecurePacket_t *pkt, TelemetrySample_t *sample_out)
{
    if (!s_initialized)
    {
        return CRYPTO_ERR_INIT;
    }
    if ((pkt == NULL) || (sample_out == NULL))
    {
        return CRYPTO_ERR_NULL_PARAM;
    }
    if (pkt->cipher_len != PACKET_SAMPLE_PLAINTEXT_LEN)
    {
        return CRYPTO_ERR_LENGTH;
    }

    uint8_t aad[PACKET_AAD_LEN];
    aad[0] = pkt->sync0;
    aad[1] = pkt->sync1;
    aad[2] = pkt->version;
    aad[3] = pkt->type;
    aad[4] = (uint8_t)(pkt->seq & 0xFFU);
    aad[5] = (uint8_t)((pkt->seq >> 8) & 0xFFU);
    aad[6] = pkt->cipher_len;

    uint8_t plaintext[PACKET_SAMPLE_PLAINTEXT_LEN];

    int ret = mbedtls_gcm_auth_decrypt(
        &s_gcm_ctx,
        pkt->cipher_len,
        pkt->nonce, CRYPTO_NONCE_BYTES,
        aad, sizeof(aad),
        pkt->tag, CRYPTO_TAG_BYTES,
        pkt->ciphertext, plaintext);

    if (ret != 0)
    {
        return CRYPTO_ERR_AUTH_FAIL;
    }

    memcpy(sample_out, plaintext, sizeof(TelemetrySample_t));
    return CRYPTO_OK;
}
