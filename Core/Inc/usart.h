/**
 ******************************************************************************
 * @file    usart.h
 * @brief   USART2 init (PA2=TX/PA3=RX, async, DMA TX) per the .ioc
 *          USART2.VirtualMode=VM_ASYNC configuration.
 ******************************************************************************
 */
#ifndef __USART_H
#define __USART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern UART_HandleTypeDef huart2;

void MX_USART2_UART_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __USART_H */
