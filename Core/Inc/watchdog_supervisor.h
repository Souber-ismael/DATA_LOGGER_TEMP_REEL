#ifndef WATCHDOG_SUPERVISOR_H
#define WATCHDOG_SUPERVISOR_H

#include "FreeRTOS.h"
#include "event_groups.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

/* Bits de l'event group — une tâche = un bit */
#define TASK_SENSOR_BIT     (1u << 0)
#define TASK_DISPLAY_BIT    (1u << 1)
#define TASK_LOGGER_BIT     (1u << 2)

#define ALL_TASKS (TASK_SENSOR_BIT | \
                   TASK_DISPLAY_BIT | \
                   TASK_LOGGER_BIT)





/* IDs des tâches pour last_alive_task */
#define TASK_SENSOR_ID      0x01u
#define TASK_DISPLAY_ID     0x02u
#define TASK_WATCHDOG_ID    0x03u
#define TASK_LOGGER_ID      0x04u

/* Event group global — défini dans watchdog_supervisor.c */
extern EventGroupHandle_t task_alive_group;

#define WDG_TIMEOUT_SENSOR_MS    4000u
#define WDG_TIMEOUT_DISPLAY_MS   7000u


#define WDG_IWDG_TIMEOUT_MS      8000u


extern __attribute__((section(".noinit"))) uint32_t last_alive_task;
extern __attribute__((section(".noinit"))) uint32_t reset_count;
extern __attribute__((section(".noinit"))) uint32_t reset_reason;


#define RESET_REASON_POWERON     0xAA000001u
#define RESET_REASON_WATCHDOG    0xAA000002u
#define RESET_REASON_SENSOR_HANG 0xAA000003u
#define RESET_REASON_DISPLAY_HANG 0xAA000004u


/* API */
void Watchdog_I(void);        /* init event group — appeler dans MX_FREERTOS_Init */
void TASK_WATCHDOG(void *pv); /* tâche superviseur — créer dans freertos.c       */
/* ============================================================
 * watchdog_supervisor.h — version améliorée
 * ============================================================ */

#endif











