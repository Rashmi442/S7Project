/**
 ******************************************************************************
 * @file    main.c
 * @brief   Application entry point: Bring up HSI/PLL (84 MHz), GPIO, DMA,
 *          USART2, ADC1, I2C1, and start FreeRTOS scheduler.
 ******************************************************************************
 */
#include "main.h"
#include "cmsis_os2.h"

#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "usart.h"
#include "gpio.h"

#include "config.h"
#include "diagnostics.h"
#include "system_status.h"

void SystemClock_Config(void);
void MX_FREERTOS_Init(void);

int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock to 84 MHz using reliable HSI + PLL */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();
    MX_I2C1_Init();
    MX_IWDG_Init();

    /* Application-level initialization */
    Config_Init();
    SystemStatus_Init();
    Diagnostics_Init();

    /* Initialize scheduler, create RTOS objects/tasks, start scheduler */
    osKernelInitialize();
    MX_FREERTOS_Init();
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    while (1)
    {
    }
}

/**
 * @brief System Clock Configuration
 *        Configures the system clock to 84 MHz using the internal 16 MHz HSI RC
 *        oscillator + PLL (PLLM=16, PLLN=336, PLLP=/4, APB1=/2, APB2=/1).
 *        Guaranteed to work reliably on all STM32F411RE boards.
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Configure HSI as clock source for PLL */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.LSIState            = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 16;  /* 16 MHz / 16 = 1 MHz */
    RCC_OscInitStruct.PLL.PLLN            = 336; /* 1 MHz * 336 = 336 MHz VCO */
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV4; /* 336 MHz / 4 = 84 MHz */
    RCC_OscInitStruct.PLL.PLLQ            = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure system clocks */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;  /* 84 MHz */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;    /* 42 MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;    /* 84 MHz */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief  Fault handler: blinks LD2 rapidly to indicate a fault condition.
 */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        for (volatile uint32_t i = 0; i < 200000; i++) { }
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
