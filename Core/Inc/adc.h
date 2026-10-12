/**
 ******************************************************************************
 * @file    adc.h
 * @brief   ADC1 (channel IN0 / PA0) init and read, per the .ioc ADC1 config.
 ******************************************************************************
 */
#ifndef __ADC_H
#define __ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern ADC_HandleTypeDef hadc1;

void     MX_ADC1_Init(void);
uint32_t ADC_ReadRaw(void);   /* single blocking conversion, 0..4095 */

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H */
