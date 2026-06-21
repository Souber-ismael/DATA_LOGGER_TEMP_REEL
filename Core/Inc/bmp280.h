/*
 * bmp280.h
 *
 * Driver for the Bosch BMP280 pressure and temperature sensor.
 * Communicates via I2C using HAL.
 *
 * Datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf
 */

#ifndef BMP280_H
#define BMP280_H

#include "stm32f1xx_hal.h"

/*
 * I2C address — SDO pin determines the address:
 * SDO connected to GND → 0x76
 * SDO connected to VDD → 0x77
 * Shifted left by 1 as required by HAL
 */
#define BMP280_ADDR         (0x77 << 1)

/* Expected value of the chip ID register (0xD0) — used to verify communication */
#define BMP280_CHIP_ID      0x58

/* Register addresses */
#define BMP280_REG_CHIP_ID  0xD0
#define BMP280_REG_CTRL     0xF4
#define BMP280_REG_CONFIG   0xF5
#define BMP280_REG_DATA     0xF7
#define BMP280_REG_CALIB    0x88

/*
 * Calibration data — stored in the sensor at factory.
 * Each BMP280 has unique coefficients used to compensate
 * raw ADC values into real temperature and pressure.
 *
 * T coefficients: used for temperature compensation.
 * P coefficients: used for pressure compensation.
 * t_fine: intermediate value shared between T and P compensation.
 *         Must be computed by BMP280_Compensate_T before calling
 *         BMP280_Compensate_P.
 */
typedef struct
{
    /* Temperature compensation coefficients */
    uint16_t dig_T1;  /* unsigned */
    int16_t  dig_T2;
    int16_t  dig_T3;

    /* Pressure compensation coefficients */
    uint16_t dig_P1;  /* unsigned */
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    /*
     * Intermediate temperature value shared with pressure compensation.
     * Computed internally by BMP280_Compensate_T — do not set manually.
     */
    int32_t t_fine;

} BMP280_CalibData;

/*
 * Initialize the BMP280 sensor.
 *
 * Verifies chip ID, configures measurement mode and oversampling,
 * then reads and stores factory calibration data.
 *
 * Must be called once before any read.
 *
 * Returns HAL_OK on success, HAL_ERROR if chip ID does not match
 * or if I2C communication fails.
 */
HAL_StatusTypeDef BMP280_Init(I2C_HandleTypeDef *hi2c);

/*
 * Read temperature and pressure from BMP280.
 *
 * Applies Bosch compensation formulas using factory calibration data.
 * Output values are split into integer and fractional parts
 * to avoid using float (suitable for bare metal / no FPU).
 *
 * temp_int  : integer part of temperature  (e.g. 25  for 25.36 C)
 * temp_frac : fractional part              (e.g. 36  for 25.36 C)
 * press_int : integer part of pressure     (e.g. 1013 for 1013.25 hPa)
 * press_frac: fractional part              (e.g. 25   for 1013.25 hPa)
 *
 * Note: BMP280_Init must be called before this function.
 * t_fine is computed internally during temperature compensation
 * and reused automatically for pressure compensation.
 *
 * Returns HAL_OK on success, HAL_ERROR on I2C failure.
 */
HAL_StatusTypeDef BMP280_ReadAll_Int(I2C_HandleTypeDef *hi2c,
                                     int *temp_int, int *temp_frac,
                                     int *press_int, int *press_frac);

#endif /* BMP280_H */
