/* ============================================================
 * task_sensor.c
 * Tâche acquisition capteurs
 *
 * Rôle :
 *  - Lire AHT20 (température + humidité)
 *  - Lire BMP280 (température + pression)
 *  - Lire DS3231 (date/heure)
 *  - Valider les mesures
 *  - Signaler les erreurs via EventGroup
 *  - Envoyer les données vers :
 *
 *       uartQueue  ---> TASK_UART
 *       logQueue   ---> TASK_LOGGER
 *
 * Ressources FreeRTOS :
 *
 *  Mutex :
 *      i2c_mutex
 *      protège le bus I2C
 *
 *  EventGroup :
 *      SystemEvents
 *      contient les erreurs capteurs
 *
 *  Queue :
 *      uartQueue
 *      logQueue
 *
 * Watchdog :
 *      signale que la tâche est vivante
 *
 * ============================================================ */

#include "app_task_sensor.h"
#include "app_task_uart.h"
#include "app_logger_task.h"
#include "aht20.h"
#include "bmp280.h"
#include "ds3231.h"
#include "cmsis_os.h"
#include "main.h"
#include "app_types.h"
#include "cmsis_os.h"
#include "queue.h"
#include "watchdog_supervisor.h"
#include "uart.h"
#include "stdbool.h"


extern QueueHandle_t uartQueue;
extern SemaphoreHandle_t i2c_mutex;
extern EventGroupHandle_t SystemEvents;


#define SENSOR_MUTEX_TIMEOUT_MS     200u    /* attente mutex max            */
#define SENSOR_QUEUE_TIMEOUT_MS     100u    /* envoi queue max              */
#define SENSOR_FAIL_THRESHOLD       3u      /* échecs avant EVT_SENSOR_FAIL */

/* Limites physiques AHT20 */
#define AHT20_TEMP_MIN_INT         (-40)
#define AHT20_TEMP_MAX_INT         (85)
#define AHT20_HUM_MIN_INT          (0)
#define AHT20_HUM_MAX_INT          (100)

/* Limites physiques BMP280 */
#define BMP280_TEMP_MIN_INT        (-40)
#define BMP280_TEMP_MAX_INT        (85)
#define BMP280_PRESS_MIN_INT       (300)    /* hPa */
#define BMP280_PRESS_MAX_INT       (1100)   /* hPa */


/*
 * Validation des données AHT20
 *
 * Vérifie que les mesures sont physiquement possibles.
 *
 * Exemple :
 * Température < -40 ou > 85 :
 * -> capteur incorrect
 * -> donnée rejetée
 *
 * Retour :
 * true  : donnée valide
 * false : donnée invalide
 */

static bool Sensor_ValidateAHT20(const sensor_data_t *data) {
    if (data == NULL) return false;

    if (data->t_int < AHT20_TEMP_MIN_INT ||
        data->t_int > AHT20_TEMP_MAX_INT) {
        UART_Log("[SENSOR] AHT20 temp hors limites : %d\r\n", data->t_int);
        return false;
    }
    if (data->h_int < AHT20_HUM_MIN_INT ||
        data->h_int > AHT20_HUM_MAX_INT) {
        UART_Log("[SENSOR] AHT20 humidité hors limites : %d\r\n", data->h_int);
        return false;
    }
    return true;
}

static bool Sensor_ValidateBMP280(const sensor_data_t *data) {
    if (data == NULL) return false;

    if (data->bmp_t_int < BMP280_TEMP_MIN_INT ||
        data->bmp_t_int > BMP280_TEMP_MAX_INT) {
        UART_Log("[SENSOR] BMP280 temp hors limites : %d\r\n", data->bmp_t_int);
        return false;
    }
    if (data->bmp_p_int < BMP280_PRESS_MIN_INT ||
        data->bmp_p_int > BMP280_PRESS_MAX_INT) {
        UART_Log("[SENSOR] BMP280 pression hors limites : %d\r\n", data->bmp_p_int);
        return false;
    }
    return true;
}

/*
 * Lecture Capteur
 *
 * Étapes :
 * 1) Lecture via I2C
 * 2) Vérification communication HAL
 * 3) Validation des valeurs
 *
 * Retourne un état :
 * STATUS_OK
 * CAPTEUR_ABSENT
 * DATA_INVALIDE
 */
sensor_status_t read_AHT20(sensor_data_t *data)
{
    // ✅ Passer les ADRESSES avec &
    if (AHT20_Read(&aht, &data->t_int, &data->t_frac,
                   &data->h_int, &data->h_frac) != HAL_OK)
    {
        UART_Log("[SENSOR] AHT20 lecture échouée\r\n");
        return CAPTEUR_ABSENT;
    }

    if (!Sensor_ValidateAHT20(data)) {
        return DATA_INVALIDE;
    }

    return STATUS_OK;
}

sensor_status_t read_BMP280(sensor_data_t *data)
{
    // ✅ Passer les ADRESSES avec &
    if (BMP280_ReadAll_Int(&hi2c1, &data->bmp_t_int, &data->bmp_t_frac,
                           &data->bmp_p_int, &data->bmp_p_frac) != HAL_OK)
    {
        UART_Log("[SENSOR] BMP280 lecture échouée\r\n");
        return COMM_ERROR;
    }

    if (!Sensor_ValidateBMP280(data)) {
        return DATA_INVALIDE;
    }

    return STATUS_OK;
}

sensor_status_t read_DS3231(sensor_data_t *data)
{
    // ✅ Passer l'adresse de data->time
    if (DS3231_GetDateTime(&data->time) != HAL_OK) {
        UART_Log("[SENSOR] DS3231 lecture échouée\r\n");
        return CAPTEUR_ABSENT;
    }

    // Ajouter validation si nécessaire
    return STATUS_OK;
}

void TASK_SENSOR(void *pv)
{
    sensor_data_t data;
    sensor_status_t st;
    bool i2c_locked = false;

    UART_Log("[SENSOR] Tâche démarrée\r\n");

    for (;;)
    {
        // 1. Notifier watchdog
        Watchdog_NotifyAlive(TASK_SENSOR_ID, TASK_SENSOR_BIT);

        // 2. Prendre le mutex I2C
        if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(SENSOR_MUTEX_TIMEOUT_MS)) != pdTRUE)
        {
            UART_Log("[SENSOR] Timeout mutex I2C\r\n");
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        i2c_locked = true;

        // 3. Lire AHT20
        st = read_AHT20(&data);
        if (st != STATUS_OK) {
            UART_Log("[SENSOR] AHT20 erreur: %d\r\n", st);
            xEventGroupSetBits(SystemEvents, SENSOR_AHT20_ERROR);
            xSemaphoreGive(i2c_mutex);
            i2c_locked = false;
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        // ✅ Effacer l'erreur si précédemment présente
        xEventGroupClearBits(SystemEvents, SENSOR_AHT20_ERROR);

        // 4. Lire BMP280
        st = read_BMP280(&data);
        if (st != STATUS_OK) {
            UART_Log("[SENSOR] BMP280 erreur: %d\r\n", st);
            xEventGroupSetBits(SystemEvents, SENSOR_BMP280_ERROR);
            xSemaphoreGive(i2c_mutex);
            i2c_locked = false;
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        xEventGroupClearBits(SystemEvents, SENSOR_BMP280_ERROR);

        // 5. Lire DS3231
        st = read_DS3231(&data);
        if (st != STATUS_OK) {
            UART_Log("[SENSOR] DS3231 erreur: %d\r\n", st);
            xEventGroupSetBits(SystemEvents, SENSOR_DS3231_ERROR);
            xSemaphoreGive(i2c_mutex);
            i2c_locked = false;
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        xEventGroupClearBits(SystemEvents, SENSOR_DS3231_ERROR);

        // 6. Libérer mutex I2C
        if (i2c_locked) {
            xSemaphoreGive(i2c_mutex);
            i2c_locked = false;
        }

        // 7. Effacer toutes les erreurs
        xEventGroupClearBits(SystemEvents,
            SENSOR_AHT20_ERROR | SENSOR_BMP280_ERROR | SENSOR_DS3231_ERROR);

        // 8. Envoyer les données à la file
        if (xQueueSend(uartQueue, &data, pdMS_TO_TICKS(SENSOR_QUEUE_TIMEOUT_MS)) != pdPASS)
        {
            UART_Log("[SENSOR] Queue pleine !\r\n");
        }
        if (xQueueSend(logQueue,
                       &data,
                       pdMS_TO_TICKS(SENSOR_QUEUE_TIMEOUT_MS)) != pdPASS)
        {
        	UART_Log("[SENSOR] Queue pleine !\r\n");
        }
        // 9. Attendre le prochain cycle
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
