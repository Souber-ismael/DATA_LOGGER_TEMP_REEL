#ifndef AHT20_H
#define AHT20_H

#include "stm32f1xx_hal.h"

#define AHT20_ADDR         0x38        // 7-bit address
#define AHT20_MAX_RETRIES  5

typedef struct {
    I2C_HandleTypeDef *hi2c;
} AHT20_HandleTypeDef;

extern AHT20_HandleTypeDef aht;

HAL_StatusTypeDef AHT20_Init(AHT20_HandleTypeDef *aht, I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef AHT20_Read(AHT20_HandleTypeDef *aht,
                              int *temp_int, int *temp_frac,
                              int *hum_int,  int *hum_frac);

#endif
