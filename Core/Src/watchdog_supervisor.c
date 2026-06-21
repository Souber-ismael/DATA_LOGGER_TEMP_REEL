/* ============================================================
 * watchdog_supervisor.c
 *
 * Superviseur système FreeRTOS + STM32 IWDG
 *
 * Rôle :
 *
 *  - Surveiller que les tâches critiques fonctionnent
 *  - Rafraîchir le watchdog matériel uniquement si
 *    toutes les tâches répondent
 *  - Détecter un reset provoqué par watchdog
 *  - Garder des informations après reset (.noinit)
 *
 *
 * Tâches surveillées :
 *
 *  TASK_SENSOR
 *       |
 *       v
 *  TASK_UART
 *       |
 *       v
 *  TASK_LOGGER
 *
 *
 * Mécanisme :
 *
 * Chaque tâche appelle :
 *
 * Watchdog_NotifyAlive()
 *
 * Le watchdog reçoit un bit dans un EventGroup.
 *
 * Quand tous les bits sont reçus :
 *      -> Refresh IWDG
 *
 * Sinon :
 *      -> pas de refresh
 *      -> reset automatique MCU
 *
 * ============================================================ */


#include "watchdog_supervisor.h"
#include "uart.h"
#include "main.h"
#include "stdbool.h"



/* Handle watchdog matériel STM32 */
extern IWDG_HandleTypeDef hiwdg;



/*
 * EventGroup utilisé comme tableau de présence.
 *
 * Chaque bit représente une tâche vivante.
 */
EventGroupHandle_t task_alive_group;




/*
 * Variables conservées après reset.
 *
 * Section .noinit :
 * le linker ne les initialise pas au démarrage.
 *
 * Permet de savoir :
 * - qui était actif avant reset
 * - combien de resets
 * - pourquoi le MCU a redémarré
 */
__attribute__((section(".noinit")))
uint32_t last_alive_task;


__attribute__((section(".noinit")))
uint32_t reset_count;


__attribute__((section(".noinit")))
uint32_t reset_reason;




/*
 * Dernière activité de chaque tâche.
 *
 * Utilisé pour diagnostic.
 *
 * Valeur = tick FreeRTOS au dernier heartbeat.
 */
static volatile uint32_t s_last_tick_sensor = 0u;

static volatile uint32_t s_last_tick_display = 0u;

static volatile uint32_t s_last_tick_logger = 0u;





/*
 * Initialisation du superviseur watchdog.
 *
 * - création EventGroup
 * - analyse cause dernier reset
 * - initialisation compteurs
 */
void Watchdog_I(void)
{


    /*
     * Création du groupe de supervision.
     *
     * Chaque tâche déposera son bit ici.
     */
    task_alive_group = xEventGroupCreate();



    if(task_alive_group == NULL)
    {
        Error_Handler();
    }




    /*
     * Vérification de la cause du dernier reset.
     *
     * Si IWDG a déclenché :
     * -> le programme précédent a bloqué
     */
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
    {

        reset_count++;

        reset_reason =
        RESET_REASON_WATCHDOG;


        __HAL_RCC_CLEAR_RESET_FLAGS();



        UART_TX_send_string(
        "[WDG] Reset watchdog détecté\r\n");

    }
    else
    {

        /*
         * Premier démarrage
         * ou reset normal.
         */
        reset_count = 0u;

        reset_reason =
        RESET_REASON_POWERON;

    }




    /*
     * Initialisation des timestamps.
     *
     * Évite un faux timeout au démarrage.
     */
    s_last_tick_sensor =
    xTaskGetTickCount();


    s_last_tick_display =
    xTaskGetTickCount();


    s_last_tick_logger =
    xTaskGetTickCount();

}






/*
 * Appelée par chaque tâche.
 *
 * Elle indique :
 *
 * "Je suis vivante,
 * mon cycle est terminé"
 *
 */
void Watchdog_NotifyAlive(
        uint32_t task_id,
        EventBits_t task_bit)
{

    uint32_t now =
    xTaskGetTickCount();



    /*
     * Mise à jour diagnostic
     * de la dernière tâche active.
     */
    switch(task_id)
    {

        case TASK_SENSOR_ID:

            s_last_tick_sensor = now;

            last_alive_task = task_id;

            break;



        case TASK_DISPLAY_ID:

            s_last_tick_display = now;

            last_alive_task = task_id;

            break;



        case TASK_LOGGER_ID:

            s_last_tick_logger = now;

            last_alive_task = task_id;

            break;



        default:

            break;
    }





    /*
     * Dépose le bit de vie.
     *
     * Exemple :
     *
     * Sensor -> bit 0
     * UART   -> bit 1
     * Logger -> bit 2
     */
    xEventGroupSetBits(
        task_alive_group,
        task_bit);

}






/*
 * Tâche superviseur.
 *
 * Fonctionnement :
 *
 * Attendre les réponses de toutes les tâches.
 *
 * Si :
 * SENSOR OK
 * UART OK
 * LOGGER OK
 *
 * alors :
 * refresh IWDG
 *
 * Sinon :
 * laisse le watchdog expirer.
 */
void TASK_WATCHDOG(void *pv)
{


    UART_TX_send_string(
    "[WDG] Superviseur démarré\r\n");




    for(;;)
    {


        /*
         * Attente des heartbeats.
         *
         * clearOnExit = pdTRUE :
         * les bits sont effacés après lecture.
         *
         * Toutes les tâches doivent
         * répondre à chaque cycle.
         */
        EventBits_t bits =
        xEventGroupWaitBits(
            task_alive_group,

            ALL_TASKS,

            pdTRUE,

            pdTRUE,

            pdMS_TO_TICKS(500)
        );





        /*
         * Toutes les tâches ont répondu.
         *
         * Le système est considéré vivant.
         */
        if(bits == ALL_TASKS)
        {

            HAL_IWDG_Refresh(&hiwdg);

        }



        /*
         * Si une tâche manque :
         *
         * pas de refresh
         *
         * IWDG expire (~8s)
         *
         * reset MCU
         */
    }
}