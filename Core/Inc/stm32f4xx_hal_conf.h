/**
 ******************************************************************************
 * @file    stm32f4xx_hal_conf.h
 * @brief   HAL configuration file -- enabled modules match the .ioc peripheral
 *          set (ADC, DMA, I2C, IWDG, UART/USART, plus core CORTEX/GPIO/RCC/PWR/FLASH).
 ******************************************************************************
 */
#ifndef __STM32F4xx_HAL_CONF_H
#define __STM32F4xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------- Module Enable ----------------------------------- */
#define HAL_MODULE_ENABLED
#define HAL_ADC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED

/* ---------------------- Oscillator values in Hz -------------------------- */
#if !defined(HSE_VALUE)
#define HSE_VALUE               8000000U
#endif
#define HSE_STARTUP_TIMEOUT     100U

#if !defined(HSI_VALUE)
#define HSI_VALUE               16000000U
#endif

#if !defined(LSI_VALUE)
#define LSI_VALUE                32000U
#endif
#if !defined(LSE_VALUE)
#define LSE_VALUE                32768U
#endif
#define LSE_STARTUP_TIMEOUT      5000U

/* Used by HAL_RCCEx_GetPeriphCLKFreq() for I2S/SAI external clock input
 * (RCC_CFGR_I2SSRC). Not used by this project (no external audio clock
 * wired up), but stm32f4xx_hal_rcc_ex.c references the symbol
 * unconditionally, so it must be defined or the HAL driver fails to build. */
#if !defined(EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE      12288000U
#endif

#define VDD_VALUE                3300U
#define TICK_INT_PRIORITY        0x0FU
#define USE_RTOS                 0U
#define PREFETCH_ENABLE          1U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE        1U

#define USE_HAL_ADC_REGISTER_CALLBACKS   0U
#define USE_HAL_UART_REGISTER_CALLBACKS  0U
#define USE_HAL_I2C_REGISTER_CALLBACKS   0U

/* ---------------------- Includes ----------------------------------------- */
#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32f4xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32f4xx_hal_gpio.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32f4xx_hal_dma.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32f4xx_hal_cortex.h"
#endif
#ifdef HAL_ADC_MODULE_ENABLED
#include "stm32f4xx_hal_adc.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32f4xx_hal_flash.h"
#endif
#ifdef HAL_I2C_MODULE_ENABLED
#include "stm32f4xx_hal_i2c.h"
#endif
#ifdef HAL_IWDG_MODULE_ENABLED
#include "stm32f4xx_hal_iwdg.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32f4xx_hal_pwr.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
#include "stm32f4xx_hal_uart.h"
#endif
#ifdef HAL_EXTI_MODULE_ENABLED
#include "stm32f4xx_hal_exti.h"
#endif

/* ---------------------- Assert selection ---------------------------------- */
#define USE_FULL_ASSERT    1U
#ifdef USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_HAL_CONF_H */
