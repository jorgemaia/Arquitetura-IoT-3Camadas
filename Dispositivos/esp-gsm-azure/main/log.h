#pragma once

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_spiffs.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include "simCom.h"
//#include "IoT_Hub_MQTT.h"
#include "iothub_client_sample_mqtt.h"
// SPI pins for sd card
#define PIN_NUM_MISO 2
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

#define LOG_COPY_PERIOD         (2*60000 / portTICK_RATE_MS)
#define WRITE_QUEUE_PERIOD      (1*60000 / portTICK_RATE_MS)

typedef struct{
    bool active;
    uint32_t caminhaoId;
    uint32_t sensorId;
    uint16_t pressure;
    uint8_t temp;
    double lat;
    double lon;
    struct tm dthora;
}jsonData;

jsonData oldSensorData[4];


extern char* connectionString;
extern SemaphoreHandle_t spiffsSemaphore;
extern SemaphoreHandle_t SDSemaphore;


void initInternMem(void);
void writeLog(char* line);
void initSDCard(void);
esp_err_t mountSDCard(sdmmc_card_t** card);
void SDTask(void *args);
void writeOfflineData(uint8_t module, uint32_t idCaminhao, uint32_t idSensor, uint16_t pressure, uint8_t temp);
void writeToQueue(void *args);