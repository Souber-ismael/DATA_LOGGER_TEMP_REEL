#include "bmp280.h"

static BMP280_CalibData bmp280_calib;

static void BMP280_ReadCalib(I2C_HandleTypeDef *hi2c)
{
    uint8_t calib[24];

    HAL_I2C_Mem_Read(hi2c, BMP280_ADDR, 0x88, 1, calib, 24, 100);

    bmp280_calib.dig_T1 = (uint16_t)((calib[1] << 8) | calib[0]);
    bmp280_calib.dig_T2 = (int16_t)((calib[3] << 8) | calib[2]);
    bmp280_calib.dig_T3 = (int16_t)((calib[5] << 8) | calib[4]);

    bmp280_calib.dig_P1 = (uint16_t)((calib[7] << 8) | calib[6]);
    bmp280_calib.dig_P2 = (int16_t)((calib[9] << 8) | calib[8]);
    bmp280_calib.dig_P3 = (int16_t)((calib[11] << 8) | calib[10]);
    bmp280_calib.dig_P4 = (int16_t)((calib[13] << 8) | calib[12]);
    bmp280_calib.dig_P5 = (int16_t)((calib[15] << 8) | calib[14]);
    bmp280_calib.dig_P6 = (int16_t)((calib[17] << 8) | calib[16]);
    bmp280_calib.dig_P7 = (int16_t)((calib[19] << 8) | calib[18]);
    bmp280_calib.dig_P8 = (int16_t)((calib[21] << 8) | calib[20]);
    bmp280_calib.dig_P9 = (int16_t)((calib[23] << 8) | calib[22]);
}

HAL_StatusTypeDef BMP280_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t id;

    HAL_I2C_Mem_Read(hi2c, BMP280_ADDR, 0xD0, 1, &id, 1, 100);

    if (id != 0x58 && id != 0x60)  /* 0x58=BMP280, 0x60=BME280 clone */
        return HAL_ERROR;

    uint8_t data;

    data = 0x27;
    HAL_I2C_Mem_Write(hi2c, BMP280_ADDR, 0xF4, 1, &data, 1, 100);

    data = 0xA0;
    HAL_I2C_Mem_Write(hi2c, BMP280_ADDR, 0xF5, 1, &data, 1, 100);

    BMP280_ReadCalib(hi2c);

    return HAL_OK;
}

HAL_StatusTypeDef BMP280_ReadRaw(I2C_HandleTypeDef *hi2c, int32_t *adc_T, int32_t *adc_P)
{
    uint8_t data[6];

        if(HAL_I2C_Mem_Read(hi2c, BMP280_ADDR, 0xF7, 1, data, 6, 100) != HAL_OK)
    {
        return HAL_ERROR;  // Pas de ; avant les accolades !
    }
    *adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    *adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

    return HAL_OK;
}

static int32_t BMP280_Compensate_T(int32_t adc_T)
{
    int32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32_t)bmp280_calib.dig_T1 << 1))) *
            ((int32_t)bmp280_calib.dig_T2)) >> 11;

    var2 = (((((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1))) >> 12) *
            ((int32_t)bmp280_calib.dig_T3)) >> 14;

    bmp280_calib.t_fine = var1 + var2;
    T = (bmp280_calib.t_fine * 5 + 128) >> 8;

    return T;
}

static uint32_t BMP280_Compensate_P(int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)bmp280_calib.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp280_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp280_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp280_calib.dig_P4) << 35);

    var1 = ((var1 * var1 * (int64_t)bmp280_calib.dig_P3) >> 8) +
           ((var1 * (int64_t)bmp280_calib.dig_P2) << 12);

    var1 = (((((int64_t)1) << 47) + var1)) *
           ((int64_t)bmp280_calib.dig_P1) >> 33;

    if (var1 == 0) return 0;

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;

    var1 = (((int64_t)bmp280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp280_calib.dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)bmp280_calib.dig_P7) << 4);

    return (uint32_t)p;
}

HAL_StatusTypeDef BMP280_ReadAll_Int(I2C_HandleTypeDef *hi2c,
                                     int *temp_int, int *temp_frac,
                                     int *press_int, int *press_frac)
{
    int32_t adc_T, adc_P;
    HAL_StatusTypeDef status;

    status = BMP280_ReadRaw(hi2c, &adc_T, &adc_P);
    if(status != HAL_OK)
        return status; 

    int32_t T = BMP280_Compensate_T(adc_T);     // en 0.01 °C
    uint32_t P = BMP280_Compensate_P(adc_P);    // Q24.8 format

    // Température
    *temp_int = T / 100;
    *temp_frac = T % 100;

    // Pression (hPa)
    uint32_t press = P / 256; // Pa

    *press_int = press / 100;
    *press_frac = press % 100;

    return HAL_OK;
}
