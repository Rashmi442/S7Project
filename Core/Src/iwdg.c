/**
 ******************************************************************************
 * @file    iwdg.c
 * @brief   IWDG configured for a ~2 second timeout (LSI ~32 kHz, prescaler
 *          /64, reload 1000), refreshed only by MonitorTask when all
 *          supervised tasks are alive (see monitor.c).
 ******************************************************************************
 */
#include "iwdg.h"

IWDG_HandleTypeDef hiwdg;

void MX_IWDG_Init(void)
{
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload    = 1000;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        Error_Handler();
    }
}
