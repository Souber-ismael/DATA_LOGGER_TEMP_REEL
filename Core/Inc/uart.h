/*
 * uart.h
 *
 * UART TX driver for USART1 on STM32F1 — HAL DMA mode.
 * All send functions are non-blocking: the DMA transfers the data
 * while the CPU continues.  The caller must ensure the buffer stays
 * valid until the transfer completes (check UART_TX_busy()).
 *
 * DMA channel mapping on STM32F1 (fixed in silicon):
 *   DMA1 Channel 4 → USART1 TX   (used here)
 *   DMA1 Channel 5 → USART1 RX
 */

#ifndef UART_H
#define UART_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>
#include "cmsis_os.h"


// uart.h
extern TaskHandle_t uartTxTaskHandle;


void UART_TX_Init(UART_HandleTypeDef *huart);
void UART_TX_send_string( const char *str);

void UART_Log(const char *format, ...);


#endif /* UART_H */
