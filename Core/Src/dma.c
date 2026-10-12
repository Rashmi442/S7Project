/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.c
  * @brief   DMA controller initialization
  ******************************************************************************
  */
/* USER CODE END Header */

#include "dma.h"

/* DMA handle used by USART2 TX */
DMA_HandleTypeDef hdma_usart2_tx;

/**
  * Enable DMA controller clock
  */
void MX_DMA_Init(void)
{
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}
