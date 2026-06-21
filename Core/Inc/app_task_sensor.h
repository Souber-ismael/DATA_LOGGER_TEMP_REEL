#ifndef APP_TASK_SENSOR_H
#define APP_TASK_SENSOR_H

#include "FreeRTOS.h"
#include "semphr.h"
typedef enum {
    STATUS_OK = 0,
    CAPTEUR_ABSENT,
    COMM_ERROR,
    DATA_INVALIDE
} sensor_status_t;

void TASK_SENSOR(void *pv);

#endif