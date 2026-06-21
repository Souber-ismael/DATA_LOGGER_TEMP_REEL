#ifndef DS3231_H
#define DS3231_H

#include "stm32f1xx_hal.h"   // change to your STM32 family header
#include "app_types.h"

HAL_StatusTypeDef DS3231_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef DS3231_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
HAL_StatusTypeDef DS3231_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
HAL_StatusTypeDef DS3231_SetDate(uint8_t day, uint8_t date, uint8_t month, uint8_t year);
HAL_StatusTypeDef DS3231_GetDate(uint8_t *day, uint8_t *date, uint8_t *month, uint8_t *year);
HAL_StatusTypeDef DS3231_SetDateTime(DS3231_DateTime_t *dt);
HAL_StatusTypeDef DS3231_GetDateTime(DS3231_DateTime_t *dt);

/* Returns 1 if DS3231 lost power and needs time to be set (OSF flag) */
uint8_t DS3231_NeedsTimeSet(I2C_HandleTypeDef *hi2c);

#endif
