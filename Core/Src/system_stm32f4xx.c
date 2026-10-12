/**
 ******************************************************************************
 * @file    system_stm32f4xx.c
 * @brief   CMSIS Cortex-M4 device system source file for STM32F4xx devices.
 *          Provides SystemInit() (called from the startup assembly before
 *          main()) and SystemCoreClockUpdate().
 *
 * This is the standard CMSIS device system file used by all STM32F4
 * STM32CubeIDE-generated projects, trimmed to the options relevant to this
 * project (HSE 8 MHz bypass-off crystal, VTOR at flash base, no external
 * SRAM/SDRAM). Regenerate from STM32CubeMX if you change memory layout.
 ******************************************************************************
 */
#include "stm32f4xx.h"

/* This variable is updated by SystemCoreClockUpdate() and used by the HAL
 * (see stm32f4xx_hal_rcc.h) and FreeRTOSConfig.h (configCPU_CLOCK_HZ). */
uint32_t SystemCoreClock = 16000000U;

const uint8_t AHBPrescTable[16]  = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]   = {0, 0, 0, 0, 1, 2, 3, 4};

/**
 * @brief  Minimal core/clock setup executed before main() and before the
 *         static/global data relocation performed by the startup code.
 *         The full PLL configuration itself happens later in
 *         SystemClock_Config() (main.c), once HAL_Init() has run.
 */
void SystemInit(void)
{
    /* FPU settings: enable CP10/CP11 full access for the Cortex-M4F FPU */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
#endif

    /* Reset RCC clock configuration to the default reset state ---------- */
    RCC->CR |= (uint32_t)0x00000001;      /* enable HSI */
    RCC->CFGR = 0x00000000;               /* reset CFGR */
    RCC->CR &= (uint32_t)0xFEF6FFFF;      /* reset HSEON, CSSON, PLLON */
    RCC->PLLCFGR = 0x24003010;            /* reset PLLCFGR to its default */
    RCC->CR &= (uint32_t)0xFFFBFFFF;      /* reset HSEBYP */
    RCC->CIR = 0x00000000;                /* disable all interrupts */

    /* Vector table relocation: flash-based, offset 0 (no bootloader) ---- */
#ifdef VECT_TAB_SRAM
    SCB->VTOR = SRAM_BASE | 0x00;
#else
    SCB->VTOR = FLASH_BASE | 0x00;
#endif
}

/**
 * @brief  Recomputes SystemCoreClock from the live RCC register state.
 *         Called by HAL_RCC_ClockConfig() and HAL_Init().
 */
void SystemCoreClockUpdate(void)
{
    uint32_t tmp = 0U, pllvco = 0U, pllp = 2U, pllsource = 0U, pllm = 2U;

    tmp = RCC->CFGR & RCC_CFGR_SWS;

    switch (tmp)
    {
        case 0x00U: /* HSI used as system clock source */
            SystemCoreClock = HSI_VALUE;
            break;

        case 0x04U: /* HSE used as system clock source */
            SystemCoreClock = HSE_VALUE;
            break;

        case 0x08U: /* PLL used as system clock source */
            pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
            pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;

            if (pllsource != 0U)
            {
                pllvco = (HSE_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
            }
            else
            {
                pllvco = (HSI_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
            }

            pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> 16) + 1U) * 2U;
            SystemCoreClock = pllvco / pllp;
            break;

        default:
            SystemCoreClock = HSI_VALUE;
            break;
    }

    /* AHB prescaler */
    tmp = AHBPrescTable[(RCC->CFGR & RCC_CFGR_HPRE) >> 4];
    SystemCoreClock >>= tmp;
}
