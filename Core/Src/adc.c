/**
 ******************************************************************************
 * @file    adc.c
 * @brief   ADC1 initialization (channel 0 / PA0, single conversion,
 *          3-cycle sampling time -- matches the .ioc ADC1 IPParameters).
 ******************************************************************************
 */
#include "adc.h"
#include "diagnostics.h"

ADC_HandleTypeDef hadc1;

void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.Channel      = ADC_CHANNEL_0;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

uint32_t ADC_ReadRaw(void)
{
    uint32_t value = 0U;

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        Diagnostics_ReportFault(FAULT_ADC_TIMEOUT);
        return 0U;
    }

    if (HAL_ADC_PollForConversion(&hadc1, 5U) == HAL_OK)
    {
        value = HAL_ADC_GetValue(&hadc1);
    }
    else
    {
        Diagnostics_ReportFault(FAULT_ADC_TIMEOUT);
    }

    HAL_ADC_Stop(&hadc1);
    return value;
}
