/**
 ******************************************************************************
 * @file    i2c.h
 * @brief   I2C1 init (PB7=SDA, PB8=SCL) -- reserved for a future real pulse
 *          oximeter/temperature sensor (e.g. MAX30102), per the .ioc I2C1
 *          block. Currently unused by the simulated sensor pipeline.
 ******************************************************************************
 */
#ifndef __I2C_H
#define __I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

extern I2C_HandleTypeDef hi2c1;

void MX_I2C1_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_H */
