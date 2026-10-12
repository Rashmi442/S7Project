/**
 ******************************************************************************
 * @file    gpio.h
 * @brief   GPIO init -- LD2 (PA5, green LED) output, B1 (PC13) EXTI button,
 *          SWD debug pins, per the .ioc pin assignments.
 ******************************************************************************
 */
#ifndef __GPIO_H
#define __GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void MX_GPIO_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __GPIO_H */
