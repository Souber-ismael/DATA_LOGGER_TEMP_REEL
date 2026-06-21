#include "ds3231.h"
#include "app_types.h"

#define DS3231_I2C_ADDR   (0x68 << 1)

#define DS3231_REG_SECONDS   0x00
#define DS3231_REG_MINUTES   0x01
#define DS3231_REG_HOURS     0x02
#define DS3231_REG_DAY       0x03
#define DS3231_REG_DATE      0x04
#define DS3231_REG_MONTH     0x05
#define DS3231_REG_YEAR      0x06
#define DS3231_REG_CONTROL   0x0E
#define DS3231_REG_STATUS    0x0F

static I2C_HandleTypeDef *ds3231_hi2c = NULL;

static uint8_t decToBcd(uint8_t val)
{
    return (uint8_t)(((val / 10) << 4) | (val % 10));
}

static uint8_t bcdToDec(uint8_t val)
{
    return (uint8_t)(((val >> 4) * 10) + (val & 0x0F));
}

HAL_StatusTypeDef DS3231_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t status;

    ds3231_hi2c = hi2c;

    if (HAL_I2C_IsDeviceReady(ds3231_hi2c, DS3231_I2C_ADDR, 3, 100) != HAL_OK)
        return HAL_ERROR;

    if (HAL_I2C_Mem_Read(ds3231_hi2c, DS3231_I2C_ADDR, DS3231_REG_STATUS,
                         I2C_MEMADD_SIZE_8BIT, &status, 1, 100) != HAL_OK)
        return HAL_ERROR;

    status &= ~(1 << 7);

    if (HAL_I2C_Mem_Write(ds3231_hi2c, DS3231_I2C_ADDR, DS3231_REG_STATUS,
                          I2C_MEMADD_SIZE_8BIT, &status, 1, 100) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}

HAL_StatusTypeDef DS3231_SetTime(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    uint8_t data[3];

    data[0] = decToBcd(seconds) & 0x7F;
    data[1] = decToBcd(minutes) & 0x7F;
    data[2] = decToBcd(hours) & 0x3F;   // 24-hour mode

    return HAL_I2C_Mem_Write(ds3231_hi2c, DS3231_I2C_ADDR, DS3231_REG_SECONDS,
                             I2C_MEMADD_SIZE_8BIT, data, 3, 100);
}

HAL_StatusTypeDef DS3231_GetTime(uint8_t *hours, uint8_t *minutes, uint8_t *seconds)
{
    uint8_t data[3];

    if (HAL_I2C_Mem_Read(ds3231_hi2c, DS3231_I2C_ADDR, DS3231_REG_SECONDS,
                         I2C_MEMADD_SIZE_8BIT, data, 3, 100) != HAL_OK)
        return HAL_ERROR;

    *seconds = bcdToDec(data[0] & 0x7F);
    *minutes = bcdToDec(data[1] & 0x7F);
    *hours   = bcdToDec(data[2] & 0x3F);

    return HAL_OK;
}

HAL_StatusTypeDef DS3231_SetDate(uint8_t day, uint8_t date, uint8_t month, uint8_t year)
{
    uint8_t data[4];

    data[0] = decToBcd(day) & 0x07;
    data[1] = decToBcd(date) & 0x3F;
    data[2] = decToBcd(month) & 0x1F;
    data[3] = decToBcd(year);

    return HAL_I2C_Mem_Write(ds3231_hi2c, DS3231_I2C_ADDR, DS3231_REG_DAY,
                             I2C_MEMADD_SIZE_8BIT, data, 4, 100);
}

HAL_StatusTypeDef DS3231_GetDate(uint8_t *day, uint8_t *date, uint8_t *month, uint8_t *year)
{
    uint8_t data[4];

    if (HAL_I2C_Mem_Read(ds3231_hi2c, DS3231_I2C_ADDR, DS3231_REG_DAY,
                         I2C_MEMADD_SIZE_8BIT, data, 4, 100) != HAL_OK)
        return HAL_ERROR;

    *day   = bcdToDec(data[0] & 0x07);
    *date  = bcdToDec(data[1] & 0x3F);
    *month = bcdToDec(data[2] & 0x1F);
    *year  = bcdToDec(data[3]);

    return HAL_OK;
}

HAL_StatusTypeDef DS3231_SetDateTime(DS3231_DateTime_t *dt)
{
    uint8_t data[7];

    data[0] = decToBcd(dt->seconds) & 0x7F;
    data[1] = decToBcd(dt->minutes) & 0x7F;
    data[2] = decToBcd(dt->hours) & 0x3F;
    data[3] = decToBcd(dt->day) & 0x07;
    data[4] = decToBcd(dt->date) & 0x3F;
    data[5] = decToBcd(dt->month) & 0x1F;
    data[6] = decToBcd(dt->year);

    if (HAL_I2C_Mem_Write(ds3231_hi2c, DS3231_I2C_ADDR, DS3231_REG_SECONDS,
                          I2C_MEMADD_SIZE_8BIT, data, 7, 100) != HAL_OK)
        return HAL_ERROR;

    /* Clear OSF flag so DS3231_NeedsTimeSet() returns 0 on next boot */
    uint8_t status = 0;
    HAL_I2C_Mem_Read(ds3231_hi2c, DS3231_I2C_ADDR, DS3231_REG_STATUS,
                     I2C_MEMADD_SIZE_8BIT, &status, 1, 100);
    status &= ~(0x80);
    HAL_I2C_Mem_Write(ds3231_hi2c, DS3231_I2C_ADDR, DS3231_REG_STATUS,
                      I2C_MEMADD_SIZE_8BIT, &status, 1, 100);

    return HAL_OK;
}

HAL_StatusTypeDef DS3231_GetDateTime(DS3231_DateTime_t *dt)
{
    uint8_t data[7];

    if (HAL_I2C_Mem_Read(ds3231_hi2c, DS3231_I2C_ADDR, DS3231_REG_SECONDS,
                         I2C_MEMADD_SIZE_8BIT, data, 7, 100) != HAL_OK)
        return HAL_ERROR;

    dt->seconds = bcdToDec(data[0] & 0x7F);
    dt->minutes = bcdToDec(data[1] & 0x7F);
    dt->hours   = bcdToDec(data[2] & 0x3F);
    dt->day     = bcdToDec(data[3] & 0x07);
    dt->date    = bcdToDec(data[4] & 0x3F);
    dt->month   = bcdToDec(data[5] & 0x1F);
    dt->year    = bcdToDec(data[6]);

    return HAL_OK;
}

/* ----------------------------------------------------------------
 * DS3231_NeedsTimeSet
 * Returns 1 if the OSF (Oscillator Stop Flag) is set — meaning the
 * DS3231 lost power or has never been initialized.
 * The flag is cleared automatically by DS3231_SetDateTime.
 * ---------------------------------------------------------------- */
uint8_t DS3231_NeedsTimeSet(I2C_HandleTypeDef *hi2c)
{
    uint8_t status = 0;
    if (HAL_I2C_Mem_Read(hi2c, DS3231_I2C_ADDR, DS3231_REG_STATUS,
                         I2C_MEMADD_SIZE_8BIT, &status, 1, 100) != HAL_OK)
        return 1; /* assume needs set if can't read */
    return (status & 0x80) ? 1 : 0;
}
