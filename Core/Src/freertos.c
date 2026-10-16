/**
 ******************************************************************************
 * @file    freertos.c
 * @brief   FreeRTOS initialization and task/queue/semaphore instantiation.
 *          Implements low-power idle hook (WFI) for ARM Cortex-M4.
 ******************************************************************************
 */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os2.h"
#include "app_tasks.h"
#include "config.h"

/* --- RTOS Object Handles ------------------------------------------------ */
osMessageQueueId_t telemetryQueueHandle;
osMessageQueueId_t cryptoQueueHandle;
osMessageQueueId_t uartQueueHandle;
osSemaphoreId_t    uartTxSemaphoreHandle;
osMutexId_t        uartMutexHandle;

/* --- Task Handles ------------------------------------------------------- */
osThreadId_t VitalTaskHandle;
osThreadId_t CryptoTaskHandle;
osThreadId_t CommTaskHandle;
osThreadId_t MonitorTaskHandle;

/* Task Attributes */
const osThreadAttr_t VitalTask_attributes = {
    .name       = "VitalTask",
    .stack_size = TASK_STACK_VITAL_WORDS * 4,
    .priority   = (osPriority_t)TASK_PRIORITY_VITAL,
};
const osThreadAttr_t CryptoTask_attributes = {
    .name       = "CryptoTask",
    .stack_size = TASK_STACK_CRYPTO_WORDS * 4,
    .priority   = (osPriority_t)TASK_PRIORITY_CRYPTO,
};
const osThreadAttr_t CommTask_attributes = {
    .name       = "CommTask",
    .stack_size = TASK_STACK_COMM_WORDS * 4,
    .priority   = (osPriority_t)TASK_PRIORITY_COMM,
};
const osThreadAttr_t MonitorTask_attributes = {
    .name       = "MonitorTask",
    .stack_size = TASK_STACK_MONITOR_WORDS * 4,
    .priority   = (osPriority_t)TASK_PRIORITY_MONITOR,
};

/* Queue & Sync Attributes */
const osMessageQueueAttr_t telemetryQueue_attributes = { .name = "telemetryQueue" };
const osMessageQueueAttr_t cryptoQueue_attributes    = { .name = "cryptoQueue" };
const osMessageQueueAttr_t uartQueue_attributes      = { .name = "uartQueue" };
const osSemaphoreAttr_t    uartTxSemaphore_attributes = { .name = "uartTxSemaphore" };
const osMutexAttr_t        uartMutex_attributes        = { .name = "uartMutex" };

void MX_FREERTOS_Init(void)
{
    /* Queues store heap-allocated object pointers (sizeof(uint32_t)) */
    telemetryQueueHandle  = osMessageQueueNew(TELEMETRY_QUEUE_LEN, sizeof(uint32_t), &telemetryQueue_attributes);
    cryptoQueueHandle     = osMessageQueueNew(CRYPTO_QUEUE_LEN, sizeof(uint32_t), &cryptoQueue_attributes);
    uartQueueHandle       = osMessageQueueNew(UART_QUEUE_LEN, sizeof(uint32_t), &uartQueue_attributes);

    /* Binary semaphore for DMA TX completion signaling */
    uartTxSemaphoreHandle = osSemaphoreNew(1, 0, &uartTxSemaphore_attributes);

    /* Mutex for USART2 DMA bus serialization */
    uartMutexHandle       = osMutexNew(&uartMutex_attributes);

    /* Create the 4 core tasks */
    VitalTaskHandle   = osThreadNew(StartDefaultTask, NULL, &VitalTask_attributes);
    CryptoTaskHandle  = osThreadNew(StartTask02, NULL, &CryptoTask_attributes);
    CommTaskHandle    = osThreadNew(StartTask03, NULL, &CommTask_attributes);
    MonitorTaskHandle = osThreadNew(StartTask04, NULL, &MonitorTask_attributes);
}

/**
 * @brief FreeRTOS Application Idle Hook:
 *        Executes when no application tasks are ready to run.
 *        In Debug mode, we avoid WFI to prevent disconnecting the ST-LINK SWD clock.
 */
void vApplicationIdleHook(void)
{
#ifndef DEBUG
    __WFI(); /* Wait For Interrupt (Low power management in release) */
#endif
}
