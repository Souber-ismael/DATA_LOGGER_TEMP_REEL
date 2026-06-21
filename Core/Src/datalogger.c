#include "datalogger.h"
#include "stdio.h"
#include "string.h"



static FIL logFile;

static bool logger_ready = false;



LoggerStatus_t DataLogger_Init(void)
{
    FRESULT res;


    res = f_mount(&SDFatFS,
                  "",
                  1);


    if(res != FR_OK)
    {
        return LOGGER_SD_ERROR;
    }



    res = f_open(&logFile,
                 "data.csv",
                 FA_OPEN_ALWAYS |
                 FA_WRITE);



    if(res != FR_OK)
    {
        return LOGGER_OPEN_ERROR;
    }



    // aller à la fin du fichier

    f_lseek(&logFile,
            f_size(&logFile));



    // écrire header si fichier vide

    if(f_size(&logFile) == 0)
    {
        char header[] =
        "Time,Temperature,Humidity,Pressure\r\n";


        UINT bw;


        f_write(&logFile,
                header,
                strlen(header),
                &bw);
    }



    logger_ready = true;


    return LOGGER_OK;
}




LoggerStatus_t DataLogger_Write(sensor_data_t *data)
{

    if(!logger_ready)
    {
        return LOGGER_SD_ERROR;
    }



    char buffer[100];


    sprintf(buffer,

    "%02d:%02d:%02d,%.2f,%.2f,%.2f\r\n",

    data->hour,
    data->min,
    data->sec,

    data->temperature,
    data->humidity,
    data->pressure);



    UINT bw;



    FRESULT res;


    res = f_write(&logFile,
                  buffer,
                  strlen(buffer),
                  &bw);



    if(res != FR_OK)
    {
        return LOGGER_WRITE_ERROR;
    }



    // force écriture SD

    f_sync(&logFile);



    return LOGGER_OK;

}





LoggerStatus_t DataLogger_Close(void)
{

    if(!logger_ready)
        return LOGGER_OK;



    if(f_close(&logFile) != FR_OK)
    {
        return LOGGER_CLOSE_ERROR;
    }


    logger_ready = false;


    return LOGGER_OK;
}