/**
 ******************************************************************************
 * @file    packet.c
 * @brief   Telemetry sample serialization, wire-protocol framing, and CRC16.
 ******************************************************************************
 */
#include <string.h>
#include "packet.h"

/**
 * @brief Serialize a TelemetrySample_t into a flat little-endian byte buffer.
 */
size_t Packet_SerializeSample(const TelemetrySample_t *sample, uint8_t *out_buf, size_t out_buf_size)
{
    if ((sample == NULL) || (out_buf == NULL))
    {
        return 0U;
    }
    if (out_buf_size < sizeof(TelemetrySample_t))
    {
        return 0U;
    }

    /* Fixed-width packed structure copied directly */
    memcpy(out_buf, sample, sizeof(TelemetrySample_t));
    return sizeof(TelemetrySample_t);
}

/**
 * @brief Standard CRC16-CCITT (polynomial 0x1021, init 0xFFFF).
 *        Calculated over the entire frame preceding the 2-byte CRC field.
 */
uint16_t Packet_CRC16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFU;

    for (size_t i = 0U; i < len; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0U; b < 8U; b++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            }
            else
            {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

/**
 * @brief Flatten a SecurePacket_t into a contiguous wire frame for UART DMA.
 *        Layout: SYNC0 (1) + SYNC1 (1) + VERSION (1) + TYPE (1) + SEQ (2) +
 *                CIPHER_LEN (1) + NONCE (12) + CIPHERTEXT (N) + TAG (16) + CRC16 (2).
 */
size_t Packet_BuildFrame(SecurePacket_t *pkt, uint8_t *tx_buf, size_t tx_buf_size)
{
    if ((pkt == NULL) || (tx_buf == NULL))
    {
        return 0U;
    }

    size_t header_len = PACKET_AAD_LEN; /* 7 bytes: sync0, sync1, version, type, seq, cipher_len */
    size_t body_len   = CRYPTO_NONCE_BYTES + pkt->cipher_len + CRYPTO_TAG_BYTES;
    size_t total_len  = header_len + body_len + PACKET_CRC_LEN;

    if (tx_buf_size < total_len)
    {
        return 0U;
    }

    pkt->sync0   = PACKET_SYNC0;
    pkt->sync1   = PACKET_SYNC1;
    pkt->version = PACKET_VERSION;

    size_t offset = 0U;
    tx_buf[offset++] = pkt->sync0;
    tx_buf[offset++] = pkt->sync1;
    tx_buf[offset++] = pkt->version;
    tx_buf[offset++] = pkt->type;

    /* Little-endian 16-bit sequence number */
    tx_buf[offset++] = (uint8_t)(pkt->seq & 0xFFU);
    tx_buf[offset++] = (uint8_t)((pkt->seq >> 8) & 0xFFU);

    tx_buf[offset++] = pkt->cipher_len;

    /* 12-byte Nonce (4B session ID + 8B counter) */
    memcpy(&tx_buf[offset], pkt->nonce, CRYPTO_NONCE_BYTES);
    offset += CRYPTO_NONCE_BYTES;

    /* Ciphertext */
    memcpy(&tx_buf[offset], pkt->ciphertext, pkt->cipher_len);
    offset += pkt->cipher_len;

    /* 16-byte GCM Authentication Tag */
    memcpy(&tx_buf[offset], pkt->tag, CRYPTO_TAG_BYTES);
    offset += CRYPTO_TAG_BYTES;

    /* CRC16-CCITT over entire buffer up to this point */
    uint16_t crc = Packet_CRC16(tx_buf, offset);
    tx_buf[offset++] = (uint8_t)(crc & 0xFFU);
    tx_buf[offset++] = (uint8_t)((crc >> 8) & 0xFFU);

    pkt->crc16 = crc;
    return offset;
}
