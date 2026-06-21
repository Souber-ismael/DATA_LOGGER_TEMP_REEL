#ifndef DATALOGGER_H
#define DATALOGGER_H

#include "main.h"
#include "fatfs.h"
#include "stdbool.h"


typedef enum
{
    LOGGER_OK = 0,

    LOGGER_SD_ERROR,
    LOGGER_OPEN_ERROR,
    LOGGER_WRITE_ERROR,
    LOGGER_CLOSE_ERROR

} LoggerStatus_t;



typedef struct
{
    float temperature;
    float humidity;
    float pressure;

    uint8_t hour;
    uint8_t min;
    uint8_t sec;

} sensor_data_t;



LoggerStatus_t DataLogger_Init(void);


LoggerStatus_t DataLogger_Write(sensor_data_t *data);


LoggerStatus_t DataLogger_Close(void);


#endif