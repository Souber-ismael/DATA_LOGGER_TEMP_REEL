/* ============================================================
 * task_uart.c
 * Tâche d'affichage UART — formatage et envoi des données
 * Intégrée avec la FSM système
 * ============================================================ */

#include "app_task_uart.h"
#include "main.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "app_types.h"
#include "watchdog_supervisor.h"

extern QueueHandle_t uartQueue;
extern EventGroupHandle_t SystemEvents;


#define UART_QUEUE_TIMEOUT_MS    5000u   /* attente max données queue      */
#define UART_MSG_SIZE            160u    /* taille buffer message           */


static int Format_Timestamp(char *buf, int size, const sensor_data_t *data)
{
    if (data->flags & FLAG_RTC_INVALID) {
        return snprintf(buf, size, "??/??/?? ??:??:??");
    }

    return snprintf(buf, size, "%02d/%02d/%02d %02d:%02d:%02d",
                    data->time.date,
                    data->time.month,
                    data->time.year,
                    data->time.hours,
                    data->time.minutes,
                    data->time.seconds);
}


static int Format_AHT20(char *buf, int size, const sensor_data_t *data)
{
    if (data->flags & FLAG_AHT20_INVALID) {
        return snprintf(buf, size, "AHT:[ERREUR]");
    }

    return snprintf(buf, size, "AHT:%d.%dC %d.%d%%",
                    data->t_int,
                    data->t_frac,
                    data->h_int,
                    data->h_frac);
}


static int Format_BMP280(char *buf, int size, const sensor_data_t *data)
{
    if (data->flags & FLAG_BMP280_INVALID) {
        return snprintf(buf, size, "BMP:[ERREUR]");
    }

    return snprintf(buf, size, "BMP:%d.%dC %d.%dhPa",
                    data->bmp_t_int,
                    data->bmp_t_frac,
                    data->bmp_p_int,
                    data->bmp_p_frac);
}

/* Assemble le message complet */
static int Format_FullMessage(char *buf, int size, const sensor_data_t *data)
{
    char ts[32]  = {0};
    char aht[32] = {0};
    char bmp[32] = {0};

    Format_Timestamp(ts,  sizeof(ts),  data);
    Format_AHT20    (aht, sizeof(aht), data);
    Format_BMP280   (bmp, sizeof(bmp), data);

    

    return snprintf(buf, size, " %s | %s | %s\r\n",
                     ts, aht, bmp);
}


void TASK_UART(void *pv)
{
    sensor_data_t data  = {0};
    char          msg[UART_MSG_SIZE] = {0};
    

    UART_TX_send_string("[UART] Tâche démarrée\r\n");

    for (;;)
    {
   
       
       Watchdog_NotifyAlive(TASK_DISPLAY_ID, TASK_DISPLAY_BIT);
       
       EventBits_t ev = xEventGroupGetBits(SystemEvents);
       
         if (ev & SENSOR_AHT20_ERROR) {
            UART_TX_send_string("[UART] Erreur AHT20\r\n");
            vTaskDelay(pdMS_TO_TICKS(600));
            continue;
         }

        if (ev & SENSOR_BMP280_ERROR) {
            UART_TX_send_string("[UART] Erreur BMP280\r\n");
            vTaskDelay(pdMS_TO_TICKS(600));
            continue;
        }

        if (ev & SENSOR_DS3231_ERROR) {
            UART_TX_send_string("[UART] Erreur DS3231\r\n");
            vTaskDelay(pdMS_TO_TICKS(600));
            continue;
        }

        

       if (xQueueReceive(uartQueue,&data,
        pdMS_TO_TICKS(UART_QUEUE_TIMEOUT_MS)) == pdPASS){
        int len = Format_FullMessage(msg, sizeof(msg), &data);

        if (len > 0 && len < (int)sizeof(msg)) {
            UART_TX_send_string(msg);
        } else {
            UART_TX_send_string("[UART] Erreur formatage message\r\n");
        }
        }
        

    }
}
