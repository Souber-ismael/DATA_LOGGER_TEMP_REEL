#ifndef LOGGER_TASK_H
#define LOGGER_TASK_H


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "datalogger.h"


extern QueueHandle_t logQueue;


void TASK_LOGGER(void *pv);


#endif