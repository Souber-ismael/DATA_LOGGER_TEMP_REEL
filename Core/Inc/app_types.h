/*
 * app_types.h
 *
 *  Created on: Jun 9, 2026
 *      Author: admin
 */

#ifndef INC_APP_TYPES_H_
#define INC_APP_TYPES_H_

#define UART_MSG_SIZE 80
#define UART_QUEUE_SIZE 10
typedef struct {
    char msg[UART_MSG_SIZE];
    uint8_t len;
} uart_msg_t;

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;      // 1 = Sunday ... 7 = Saturday (your choice, but keep consistent)
    uint8_t date;     // 1..31
    uint8_t month;    // 1..12
    uint8_t year;     // 0..99
} DS3231_DateTime_t;


/* Dans votre header sensor_data_t — ajouter le champ flags */
typedef struct {
    int32_t  t_int;
    uint32_t t_frac;
    int32_t  h_int;
    uint32_t h_frac;
    int32_t  bmp_t_int;
    uint32_t bmp_t_frac;
    int32_t  bmp_p_int;
    uint32_t bmp_p_frac;
    DS3231_DateTime_t time;
    uint8_t  flags;         /* ← AJOUTER — bits d'état des données */
} sensor_data_t;

/* Masques de flags */
#define FLAG_AHT20_INVALID    (1u << 0)   /* données AHT20 non fiables  */
#define FLAG_BMP280_INVALID   (1u << 1)   /* données BMP280 non fiables */
#define FLAG_RTC_INVALID      (1u << 2)   /* timestamp estimé           */


#endif /* INC_APP_TYPES_H_ */
