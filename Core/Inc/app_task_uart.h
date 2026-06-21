#ifndef APP_TASK_UART_H
#define APP_TASK_UART_H
#define SENSOR_AHT20_ERROR     (1 << 0)
#define SENSOR_BMP280_ERROR    (1 << 1)
#define SENSOR_DS3231_ERROR    (1 << 2)

void TASK_UART(void *argument);

#endif