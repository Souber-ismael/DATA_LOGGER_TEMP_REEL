/* ============================================================
 * logger_task.c
 * Tâche de stockage Data Logger
 *
 * Rôle :
 *  - Recevoir les données capteurs depuis logQueue
 *  - Écrire les mesures dans la carte SD via FATFS
 *  - Superviser son état avec le watchdog
 *
 * Architecture :
 *
 * TASK_SENSOR
 *       |
 *       | sensor_data_t
 *       v
 *    logQueue
 *       |
 *       v
 * TASK_LOGGER
 *       |
 *       v
 * DataLogger (FATFS)
 *       |
 *       v
 *      SD Card
 *
 * ============================================================ */


#include "logger_task.h"
#include "watchdog_supervisor.h"
#include "main.h"
#include "uart.h"



void TASK_LOGGER(void *pv)
{

    /* Buffer de réception des données capteurs */
    sensor_data_t data;



    /*
     * Message de démarrage.
     * Permet de vérifier que la tâche est créée
     * et exécutée correctement.
     */
    UART_TX_send_string(
        "[LOGGER] Tache demarree\r\n");



    /*
     * Initialisation de la couche DataLogger :
     *
     * - montage FATFS
     * - ouverture/création fichier CSV
     * - préparation carte SD
     *
     * Si erreur :
     * on continue mais les écritures seront refusées.
     */
    if(DataLogger_Init() != LOGGER_OK)
    {
        UART_Log(
        "[LOGGER] LOGGER NO INIT\r\n");
    }



    for(;;)
    {


        /*
         * Attente d'une nouvelle mesure.
         *
         * La tâche reste bloquée maximum 1s.
         * Si aucune donnée :
         * elle repart et signale quand même
         * son état au watchdog.
         */
        if(xQueueReceive(
                logQueue,
                &data,
                pdMS_TO_TICKS(1000))
                == pdPASS)
        {


            LoggerStatus_t st;



            /*
             * Sauvegarde de la mesure sur SD.
             *
             * DataLogger_Write() contient :
             * - formatage CSV
             * - f_write()
             * - gestion FATFS
             */
            st = DataLogger_Write(&data);



            /*
             * Erreur écriture SD :
             * - carte absente
             * - erreur FATFS
             * - problème disque
             */
            if(st != LOGGER_OK)
            {
                UART_Log(
                "[LOGGER] LOGGER NO OK\r\n");
            }

        }



        /*
         * Heartbeat watchdog.
         *
         * La tâche indique au superviseur :
         * "mon cycle est terminé".
         *
         * Si LOGGER bloque :
         * ce bit ne sera jamais envoyé
         * -> watchdog ne rafraîchit pas IWDG
         * -> reset MCU
         */
        Watchdog_NotifyAlive(
            TASK_LOGGER_ID,
            TASK_LOGGER_BIT);


    }

}