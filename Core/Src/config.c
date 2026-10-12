/**
 ******************************************************************************
 * @file    config.c
 * @brief   Storage/definition for global configuration constants.
 ******************************************************************************
 */
#include "config.h"

/* Demo AES-128 key -- MUST match python_gui/telemetry_gui.py KEY constant.
 * Replace with a securely provisioned key before any real deployment. */
const uint8_t g_aes_key[CRYPTO_KEY_BYTES] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

void Config_Init(void)
{
    /* Reserved for future runtime configuration loading (e.g. from
     * on-chip OTP/flash config sector). Currently all parameters are
     * compile-time constants defined in config.h. */
}
