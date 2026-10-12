/**
 ******************************************************************************
 * @file    communication.c
 * @brief   Non-blocking UART2 DMA transport for sealed telemetry wire frames.
 ******************************************************************************
 */
#include "communication.h"
#include "usart.h"
#include "app_tasks.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"
#include "diagnostics.h"

static uint8_t s_tx_frame[PACKET_TOTAL_FRAME_LEN + 16U];

void Communication_Init(void)
{
    /* Peripherals initialized during system bring-up in main.c */
}

CommStatus_t Communication_SendPacket(const SecurePacket_t *pkt, uint32_t timeout_ms)
{
    if (pkt == NULL)
    {
        return COMM_ERR_FRAME;
    }

    SecurePacket_t local_copy = *pkt;
    size_t frame_len = Packet_BuildFrame(&local_copy, s_tx_frame, sizeof(s_tx_frame));
    if (frame_len == 0U)
    {
        Diagnostics_ReportFault(FAULT_COMM_FRAME_ERROR);
        return COMM_ERR_FRAME;
    }

    if (osMutexAcquire(uartMutexHandle, timeout_ms) != osOK)
    {
        return COMM_ERR_BUSY;
    }

    /* Drain any stale semaphore token */
    (void)osSemaphoreAcquire(uartTxSemaphoreHandle, 0U);

    HAL_StatusTypeDef hal_status = HAL_UART_Transmit_DMA(&huart2, s_tx_frame, (uint16_t)frame_len);
    if (hal_status != HAL_OK)
    {
        /* Fallback if DMA is busy */
        hal_status = HAL_UART_Transmit(&huart2, s_tx_frame, (uint16_t)frame_len, timeout_ms);
        osMutexRelease(uartMutexHandle);
        return (hal_status == HAL_OK) ? COMM_OK : COMM_ERR_HAL;
    }

    CommStatus_t status = COMM_OK;
    if (osSemaphoreAcquire(uartTxSemaphoreHandle, timeout_ms) != osOK)
    {
        HAL_UART_AbortTransmit(&huart2);
        status = COMM_ERR_TIMEOUT;
        Diagnostics_ReportFault(FAULT_COMM_TIMEOUT);
    }

    osMutexRelease(uartMutexHandle);
    return status;
}

void Communication_TxCompleteFromISR(void)
{
    osSemaphoreRelease(uartTxSemaphoreHandle);
}
