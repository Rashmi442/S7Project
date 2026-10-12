/**
 ******************************************************************************
 * @file    stm32f4xx_it.c
 * @brief   Interrupt Service Routines: Cortex-M4 core faults, SysTick
 *          (shared between HAL and FreeRTOS), DMA1_Stream6 (USART2 TX) and
 *          EXTI15_10 (user button B1 on PC13 for simulation mode control).
 ******************************************************************************
 */
#include "main.h"
#include "stm32f4xx_it.h"
#include "usart.h"
#include "dma.h"
#include "communication.h"
#include "diagnostics.h"
#include "sensors.h"
#include "cmsis_os2.h"

extern void xPortSysTickHandler(void);

/* ------------------------------------------------------------------------ */
/* Cortex-M4 Core Fault Handlers                                            */
/* ------------------------------------------------------------------------ */
void NMI_Handler(void)
{
    while (1) { }
}

void HardFault_Handler(void)
{
    Diagnostics_FaultBlink(10U);
    while (1) { }
}

void MemManage_Handler(void)
{
    while (1) { }
}

void BusFault_Handler(void)
{
    while (1) { }
}

void UsageFault_Handler(void)
{
    while (1) { }
}

void DebugMon_Handler(void)
{
}

/* SysTick handler shared between HAL tick and FreeRTOS kernel */
void SysTick_Handler(void)
{
    HAL_IncTick();

    if (osKernelGetState() == osKernelRunning)
    {
        xPortSysTickHandler();
    }
}

/* ------------------------------------------------------------------------ */
/* Peripheral Interrupt Handlers                                            */
/* ------------------------------------------------------------------------ */
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(B1_Pin);
}

void DMA1_Stream6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

/* ------------------------------------------------------------------------ */
/* HAL Callback Implementations                                             */
/* ------------------------------------------------------------------------ */

/* Called from DMA1_Stream6_IRQHandler once DMA TX transfer is complete */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        Communication_TxCompleteFromISR();
    }
}

/* User button B1 (PC13): Emergency Simulation Control trigger */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == B1_Pin)
    {
        Sensors_CycleSimulationMode();
        SensorSimMode_t mode = Sensors_GetSimulationMode();
        Diagnostics_FaultBlink((uint8_t)(mode + 1));
    }
}
