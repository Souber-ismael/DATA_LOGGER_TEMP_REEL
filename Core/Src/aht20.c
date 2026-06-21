#include "aht20.h"

HAL_StatusTypeDef AHT20_Init(AHT20_HandleTypeDef *aht, I2C_HandleTypeDef *hi2c)
{
    aht->hi2c = hi2c;

    // Soft reset
    uint8_t reset_cmd[1] = {0xBA};
    HAL_I2C_Master_Transmit(hi2c, AHT20_ADDR << 1, reset_cmd, 1, 200);
    HAL_Delay(40);

    // Attendre power-on
    HAL_Delay(40);

    // Lire le statut
    uint8_t status = 0;
    if (HAL_I2C_Master_Receive(hi2c, AHT20_ADDR << 1, &status, 1, 200) != HAL_OK)
        return HAL_ERROR;

    // Vérifier calibration
    if (!(status & 0x08))
    {
        uint8_t cmd[3] = {0xBE, 0x08, 0x00};
        if (HAL_I2C_Master_Transmit(hi2c, AHT20_ADDR << 1, cmd, 3, 200) != HAL_OK)
            return HAL_ERROR;
        HAL_Delay(40);
    }

    return HAL_OK;
}

HAL_StatusTypeDef AHT20_Read(AHT20_HandleTypeDef *aht,
                              int *temp_int, int *temp_frac,
                              int *hum_int,  int *hum_frac)
{
    // 1. Commande de mesure
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    if (HAL_I2C_Master_Transmit(aht->hi2c, AHT20_ADDR << 1, cmd, 3, 200) != HAL_OK)
        return HAL_ERROR;

    // 2. Attendre 80ms
    HAL_Delay(80);

    // 3. Lecture avec retry
    uint8_t data[6];
    HAL_StatusTypeDef status;
    uint8_t retries = 0;

    do {
        status = HAL_I2C_Master_Receive(aht->hi2c, AHT20_ADDR << 1, data, 6, 200);
        if (status == HAL_OK)
            break;
        retries++;
        HAL_Delay(20);
    } while (retries < 5);

    if (status != HAL_OK)
        return HAL_ERROR;

    // 4. Vérifier bit busy
    if (data[0] & 0x80) {
        HAL_Delay(20);
        if (HAL_I2C_Master_Receive(aht->hi2c, AHT20_ADDR << 1, data, 6, 200) != HAL_OK)
            return HAL_ERROR;
    }

    // 5. Extraire les données
    uint32_t rawHum = ((uint32_t)data[1] << 12) |
                      ((uint32_t)data[2] << 4)  |
                      (data[3] >> 4);

    uint32_t rawTemp = ((uint32_t)(data[3] & 0x0F) << 16) |
                       ((uint32_t)data[4] << 8) |
                       data[5];

    // 6. Convertir
    *hum_int   = (int)((rawHum * 100UL) / 1048576);
    *hum_frac  = (int)((rawHum * 10000UL / 1048576) % 100);
    *temp_int  = (int)((rawTemp * 200UL) / 1048576) - 50;
    *temp_frac = (int)((rawTemp * 20000UL / 1048576) % 100);

    // Corriger fractions négatives
    if (*temp_frac < 0) *temp_frac = -(*temp_frac);
    if (*hum_frac < 0) *hum_frac = -(*hum_frac);

    return HAL_OK;
}
