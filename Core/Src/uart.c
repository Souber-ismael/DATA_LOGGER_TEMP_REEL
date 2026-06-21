#include "uart.h"
#include <stdio.h>
#include "cmsis_os.h"
#include <stdio.h>
#include <stdarg.h>       /* pour les arguments variables (...) */
#include <string.h>
static DMA_HandleTypeDef    hdma_tx;
static UART_HandleTypeDef  *_huart;   /* saved in UART_TX_Init, used by all send functions */

TaskHandle_t uartTxTaskHandle;

#define UART_LOG_BUF_SIZE    128u   /* taille max d'un message log */

extern SemaphoreHandle_t s_log_mutex ;

static char s_log_buf[UART_LOG_BUF_SIZE];


/* ----------------------------------------------------------------
 * UART_TX_Init
 * Bug 1 fix: parameter was named `huart` same as the static pointer,
 * shadowing it so the static was never assigned → all sends used NULL.
 * Now the parameter is saved into _huart immediately.
 * ---------------------------------------------------------------- */
void UART_TX_Init(UART_HandleTypeDef *huart)
{
    _huart = huart;

    __HAL_RCC_DMA1_CLK_ENABLE();   /* must be before HAL_DMA_Init() */

    hdma_tx.Instance                 = DMA1_Channel4;
    hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_tx.Init.Mode                = DMA_NORMAL;
    hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;

    if (HAL_DMA_Init(&hdma_tx) != HAL_OK)
        return;

    __HAL_LINKDMA(huart, hdmatx, hdma_tx);

    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

}

void UART_Log(const char *format, ...)
{

    if (s_log_mutex == NULL) {
        return;
    }

   
    if (xSemaphoreTake(s_log_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
       
        return;
    }

    
    va_list args;
    va_start(args, format);
    int len = vsnprintf(s_log_buf, sizeof(s_log_buf), format, args);
    va_end(args);

    if (len > 0 && len < (int)sizeof(s_log_buf)) {
        UART_TX_send_string(s_log_buf);
    }

    xSemaphoreGive(s_log_mutex);
}

void UART_TX_send_string(const char *str)
{
    static uint8_t tx_buf[128];
    uint16_t len = strlen(str);

    if (len == 0 || len >= sizeof(tx_buf))
        return;

    memcpy(tx_buf, str, len);

    if (HAL_UART_Transmit_DMA(_huart, tx_buf, len) != HAL_OK)
        return;

    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000)) == 0)
    {
        HAL_UART_Abort(_huart);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(uartTxTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}



/* ----------------------------------------------------------------
 * DMA1_Channel4_IRQHandler — USART1 TX
 * Routes to HAL which clears TC and calls HAL_UART_TxCpltCallback.
 * ---------------------------------------------------------------- */
void DMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_tx);
}





