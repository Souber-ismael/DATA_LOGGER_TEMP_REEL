/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "uart.h"
#include "app_task_sensor.h"
#include "app_task_uart.h"
#include "watchdog_supervisor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
QueueHandle_t uartQueue;
QueueHandle_t logQueue;
SemaphoreHandle_t i2c_mutex = NULL;
SemaphoreHandle_t s_log_mutex = NULL;
EventGroupHandle_t SystemEvents= NULL;
extern UART_HandleTypeDef huart1;
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}



void MX_FREERTOS_Init(void)
{
    /* Event group en premier — les tâches en ont besoin */
    Watchdog_I();
    /* Dans main() — AVANT osKernelStart() */
    i2c_mutex = xSemaphoreCreateMutex();
      if (i2c_mutex == NULL) {
        Error_Handler();
    }

   s_log_mutex = xSemaphoreCreateMutex();
    if (s_log_mutex == NULL) {
        
        return;
    }
    uartQueue = xQueueCreate(5, sizeof(sensor_data_t));
    if (uartQueue == NULL)
        Error_Handler();
    
    logQueue = xQueueCreate(10, sizeof(sensor_data_t));
    if (logQueue == NULL)
        Error_Handler();

   SystemEvents = xEventGroupCreate();


    if (xTaskCreate(TASK_SENSOR,   "SENSOR",  1024, NULL, 2, NULL)       != pdPASS) Error_Handler();
    if (xTaskCreate(TASK_UART,     "UART_TX",  512, NULL, 1, &uartTxTaskHandle) != pdPASS) Error_Handler();
    if (xTaskCreate(TASK_WATCHDOG, "WDG",       256, NULL, 3, NULL)       != pdPASS) Error_Handler();
    if (xTaskCreate(TASK_WATCHDOG, "WDG",       256, NULL, 3, NULL)       != pdPASS) Error_Handler();
    if ( xTaskCreate(TASK_LOGGER,"LOGGER",512, NULL, 3, NULL )  != pdPASS) Error_Handler();
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

