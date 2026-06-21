# DATA_LOGGER_TEMP_REEL
Voici un `README.md` prêt à copier-coller en anglais avec emojis et tableaux :

````markdown
# 🌡️ STM32 FreeRTOS Data Logger

## 📌 Overview

This project is an embedded **Data Logger system** based on **STM32 + FreeRTOS**.

The system collects environmental data from multiple sensors, displays measurements through UART, and stores data on an SD card using FATFS.

The firmware is designed with a **real-time multitasking architecture**, including task synchronization, error handling, and watchdog supervision.

---

## 🚀 Features

| Feature | Description |
|---|---|
| 🌡️ Sensor acquisition | Read temperature, humidity, pressure and RTC data |
| 💾 Data storage | Save measurements into CSV files on SD card |
| 🖥️ UART interface | Real-time data display using UART DMA |
| ⚙️ FreeRTOS | Multitasking architecture |
| 🛡️ Watchdog | Detect task blocking and recover system |
| 🚨 Error handling | Sensor, queue, memory and communication errors |

---

# 🏗️ System Architecture

```
                    +----------------+
                    |  TASK_SENSOR   |
                    |                |
                    | AHT20          |
                    | BMP280         |
                    | DS3231         |
                    +-------+--------+
                            |
                            |
                    sensor_data_t
                            |
              +-------------+-------------+
              |                           |
              v                           v

     +----------------+          +----------------+
     |   TASK_UART    |          |  TASK_LOGGER   |
     |                |          |                |
     | UART DMA       |          | FATFS + SD     |
     | Display data   |          | CSV Logging    |
     +----------------+          +----------------+


                    +----------------+
                    | TASK_WATCHDOG |
                    |                |
                    | Heartbeat      |
                    | IWDG Reset    |
                    +----------------+

```

---

# 🔄 FreeRTOS Tasks

## 🌡️ TASK_SENSOR

Responsible for:

- Reading all sensors
- Checking measurement validity
- Detecting sensor errors
- Sending data to other tasks


Communication:

| Queue | Destination |
|---|---|
| `uartQueue` | UART display task |
| `logQueue` | Logger task |


---

## 🖥️ TASK_UART

Responsible for:

- Receiving sensor data
- Formatting messages
- Sending data through UART DMA


Example output:

```
09/06/26 10:15:00 |
AHT:25.4C 50.2% |
BMP:25.1C 1013hPa
```

---

## 💾 TASK_LOGGER

Responsible for:

- SD card initialization
- FATFS management
- CSV file creation
- Data recording


Generated file:

```
data.csv
```


Example:

```csv
Time,Temperature,Humidity,Pressure

10:15:00,25.4,50.2,1013
10:15:02,25.5,50.1,1012
```

---

## 🛡️ TASK_WATCHDOG

The watchdog supervises all critical tasks.

Each task sends a heartbeat:

```c
Watchdog_NotifyAlive(
    TASK_ID,
    TASK_BIT
);
```

The IWDG is refreshed only if all tasks are alive.

If a task stops:

```
Task freeze
     |
     v
No heartbeat
     |
     v
No watchdog refresh
     |
     v
STM32 reset
```

---

# 🔐 Synchronization

## Mutex

Used to protect shared resources:

| Mutex | Purpose |
|---|---|
| `i2c_mutex` | Protect I2C bus |
| `uart_mutex` | Protect UART output |


---

## Queues

Inter-task communication:

| Queue | Data |
|---|---|
| `uartQueue` | Sensor data for display |
| `logQueue` | Sensor data for storage |


---

## Event Groups

Used for:

- Task supervision
- Sensor error status


Example:

```c
SENSOR_AHT20_ERROR
SENSOR_BMP280_ERROR
SENSOR_DS3231_ERROR
```

---

# 🚨 Error Management

The system handles:

| Error | Action |
|---|---|
| Sensor communication failure | Set error event |
| Invalid measurement | Reject data |
| Mutex timeout | Retry operation |
| Queue full | Report error |
| SD card failure | Stop logging safely |
| Task freeze | Watchdog reset |

---

# 📂 Project Structure

```
STM32_DataLogger/

├── app_task_sensor.c
├── app_task_sensor.h

├── app_task_uart.c
├── app_task_uart.h

├── logger_task.c
├── logger_task.h

├── datalogger.c
├── datalogger.h

├── watchdog_supervisor.c
├── watchdog_supervisor.h

├── aht20.c
├── aht20.h

├── bmp280.c
├── bmp280.h

├── ds3231.c
├── ds3231.h

└── README.md
```

---

# 🔧 Hardware

| Component | Interface |
|---|---|
| STM32 MCU | Main controller |
| AHT20 | I2C |
| BMP280 | I2C |
| DS3231 RTC | I2C |
| SD Card | SPI |
| UART | DMA |

---

# 🛠️ Technologies

| Technology | Usage |
|---|---|
| STM32 HAL | Hardware abstraction |
| FreeRTOS | Real-time tasks |
| FATFS | SD card filesystem |
| I2C | Sensor communication |
| SPI | SD communication |
| UART DMA | Debug/output |

---

# 🎯 Project Goal

Build a reliable embedded data acquisition system with:

✅ Real-time multitasking  
✅ Clean task separation  
✅ Safe inter-task communication  
✅ Error detection  
✅ Watchdog protection  
✅ Reliable data storage  

---

## 👨‍💻 Author

STM32 FreeRTOS Data Logger Project
````

