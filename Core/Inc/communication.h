/**
 ******************************************************************************
 * @file    communication.h
 * @brief   Non-blocking UART2 DMA transport for sealed telemetry packets.
 ******************************************************************************
 */
#ifndef __COMMUNICATION_H
#define __COMMUNICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "packet.h"

typedef enum
{
    COMM_OK = 0,
    COMM_ERR_BUSY,
    COMM_ERR_TIMEOUT,
    COMM_ERR_HAL,
    COMM_ERR_FRAME
} CommStatus_t;

void         Communication_Init(void);

/* Frames `pkt` and transmits it over USART2 via DMA. Blocks (via the
 * FreeRTOS uartMutex + uartTxSemaphore) only for as long as it takes to
 * acquire the bus and hand the buffer to the DMA controller -- the actual
 * transfer happens in the background and completion is signalled from the
 * HAL_UART_TxCpltCallback ISR. */
CommStatus_t Communication_SendPacket(const SecurePacket_t *pkt, uint32_t timeout_ms);

/* Called from HAL_UART_TxCpltCallback() in stm32f4xx_it.c */
void         Communication_TxCompleteFromISR(void);

#ifdef __cplusplus
}
#endif

#endif /* __COMMUNICATION_H */
