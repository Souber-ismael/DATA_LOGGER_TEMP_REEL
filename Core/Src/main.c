/*
====================================================
MAIN INITIALIZATION

Séquence :

1) Initialisation HAL
2) Configuration horloge
3) Initialisation GPIO/I2C/SPI/UART
4) Initialisation capteurs
5) Initialisation watchdog
6) Création tâches FreeRTOS
7) Démarrage scheduler


Après scheduler :

TASK_SENSOR :
 acquisition I2C

TASK_UART :
 affichage DMA

TASK_LOGGER :
 FATFS + SD

TASK_WATCHDOG :
 supervision IWDG

====================================================
*/
#include "main.h"
#include "cmsis_os.h"
#include "aht20.h"
#include "bmp280.h"
#include "uart.h"
#include "FreeRTOS.h"
#include"app_task_sensor.h"
#include "app_task_uart.h"
#include  "watchdog_supervisor.h"
#include "ds3231.h"
#include "app_types.h"
#include "fatfs.h"



I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;

IWDG_HandleTypeDef hiwdg;

AHT20_HandleTypeDef aht;



void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void check_reset_cause(void);
static void watchdog_init(void);
static void MX_SPI2_Init(void);








/* Macro de debug bloquant — fonctionne sans FreeRTOS */
#define DBG(s) HAL_UART_Transmit(&huart1, (uint8_t*)(s), sizeof(s)-1, 500)

int main(void)
{
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_I2C1_Init();
	MX_SPI2_Init();
	MX_USART1_UART_Init();
	UART_TX_Init(&huart1);
	MX_FATFS_Init();

	check_reset_cause();


	 if (AHT20_Init(&aht, &hi2c1) == HAL_OK)
	        DBG("✅ AHT20 init OK\r\n");
	    else
	        DBG("❌ AHT20 init FAILED\r\n");

	if (BMP280_Init(&hi2c1) == HAL_OK)
		DBG("✅ BMP280 init OK\r\n");


	if (DS3231_Init(&hi2c1) == HAL_OK)
		DBG("✅ DS3231 init OK\r\n");


		DS3231_DateTime_t now = {
			.seconds = 0, .minutes = 15, .hours = 10,
			.day = 2, .date = 9, .month = 6, .year = 26
		};
		DS3231_SetDateTime(&now);

	watchdog_init();
	MX_FREERTOS_Init();
	vTaskStartScheduler();

    while (1)
    {
    }
}


void check_reset_cause(void) {

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {

        reset_count++;

        switch (last_alive_task) {
            case TASK_SENSOR_ID:
                HAL_UART_Transmit(&huart1,
                    (uint8_t *)"WDG reset - bloque dans task_sensor\r\n", 37, 500);
                break;
            case TASK_DISPLAY_ID:
                HAL_UART_Transmit(&huart1,
                    (uint8_t *)"WDG reset - bloque dans task_uart\r\n", 35, 500);
                break;
            default:
                HAL_UART_Transmit(&huart1,
                    (uint8_t *)"WDG reset - tache inconnue (init ?)\r\n", 37, 500);
                break;
        }

        if (reset_count >= 5) {
            HAL_UART_Transmit(&huart1,
                (uint8_t *)"ALERTE - 5 resets watchdog successifs\r\n", 39, 500);
        }

    } else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) {
        reset_count = 0;
        HAL_UART_Transmit(&huart1,
            (uint8_t *)"Power-on reset - demarrage normal\r\n", 35, 500);

    } else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)) {
        DBG("Reset bouton - demarrage normal\r\n");
    }

    __HAL_RCC_CLEAR_RESET_FLAGS();
}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void watchdog_init(void) {
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg.Init.Reload    = 1249;  hiwdg.Init.Reload = 1249;  /* timeout ≈ 8.0s nominal
                              LSI +15% → ≈ 6.9s minimum
                              cycle capteur ~2.3s → marge ~4.6s */

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}


static void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }


}

static void MX_SPI2_Init(void)
{


  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }


}


/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
     HAL_NVIC_EnableIRQ(USART1_IRQn);

}

void USART1_IRQHandler(void)
{

    HAL_UART_IRQHandler(&huart1);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin|USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
