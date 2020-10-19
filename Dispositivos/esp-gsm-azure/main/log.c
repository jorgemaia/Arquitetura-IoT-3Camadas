#include "log.h"


void initInternMem(void){
    static const char *fileSys = "SPIFFS";
    ESP_LOGI(fileSys, "Initializing SPIFFS");

    //spiffsSemaphore = xSemaphoreCreateBinary();
    // if(spiffsSemaphore == NULL) {
    //     ESP_LOGI(fileSys, "spiffs semaphore create failed");
    //     writeLog("spiffs semaphore create failed");
    // }
    // else ESP_LOGI(fileSys, "spiffs semaphore create successed");
    // xSemaphoreGive(spiffsSemaphore);
    
    //const char* memLabel = "INTERN_MEM";

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5, //if needed change max number of files here
      .format_if_mount_failed = true
    };
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(fileSys, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(fileSys, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(fileSys, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    //-----------------

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(fileSys, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(fileSys, "Partition size: total: %d, used: %d", total, used);
    }
    //-------------------

    esp_spiffs_format(NULL);


    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    FILE* logFile = NULL;
    struct stat logst;
    if (stat("/spiffs/log.txt", &logst) != 0) {
        ESP_LOGI(fileSys, "Creating log File.");
    
        logFile = fopen("/spiffs/log.txt", "a");

        if (logFile == NULL) {
            ESP_LOGE(fileSys, "Failed to create log file for writing");
        }
        else{
            fclose(logFile);
            ESP_LOGI(fileSys, "Log file created");
        }
    }
    else{
        remove("/spiffs/log.txt");
        logFile = fopen("/spiffs/log.txt", "a");
        fclose(logFile);
        ESP_LOGI(fileSys, "Log File Already Exists, Deleted and created new one");  
    } 

    for(uint8_t i=0; i<4; i++) oldSensorData[i].active = false;

    FILE* offlineFile = NULL;
    struct stat offst;
    if (stat("/spiffs/offline.csv", &offst) != 0) {
        ESP_LOGI(fileSys, "Creating Offline File.");
    
        offlineFile = fopen("/spiffs/offline.csv", "a");

        if (offlineFile == NULL) {
            ESP_LOGE(fileSys, "Failed to create offline file for writing");
        }
        else{
            fprintf(offlineFile, "time,modulo,idcaminhao,idsensor,pressao,temp,lat,lon\r\n");
            fclose(offlineFile);
            ESP_LOGI(fileSys, "Offline Header Written");
        }
    }
    else{
        remove("/spiffs/offline.csv");
        offlineFile = fopen("/spiffs/offline.csv", "a");
        fprintf(offlineFile, "time,modulo,idcaminhao,idsensor,pressao,temp,lat,lon\r\n");
        fclose(offlineFile);
        ESP_LOGI(fileSys, "Offline File Already Exists, Deleted and created new one");  
    } 

}

int32_t timeToString(char * str){
    int32_t len;
    while(xSemaphoreTake(tStmpSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(1 / portTICK_RATE_MS);
    // len =  sprintf(str, "%d/%d/%d - %d:%d:%d", tStmp.tm_mday, tStmp.tm_mon, tStmp.tm_year, tStmp.tm_hour, tStmp.tm_min, tStmp.tm_sec);
    time_t tt = time(NULL); //Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual
    tStmp = *gmtime(&tt); //Converte o tempo atual e atribui na estrutura
    //len = strftime(str, 64, "%d/%m/%Y %H:%M:%S", &tStmp);
    len = strftime(str, 64, "%Y/%m/%dT%H:%M:%SZ", &tStmp); //UTC format
    xSemaphoreGive(tStmpSemaphore);
    return len;
}

int32_t gpsToString(char* str){
    int32_t len = 0;
    while(xSemaphoreTake(gpsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(1 / portTICK_RATE_MS);
    if((lat != 0.0) && (lon != 0.0))
        len = sprintf(str, "%0.6lf,%0.6lf", lat, lon);
    else
        str = NULL;
    xSemaphoreGive(gpsSemaphore);
    return len;
}

void writeLog(char* line){
    //static const char *fileSys = "WRITE-LOG";
    //while(xSemaphoreTake(spiffsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);

    //FILE* logFile = fopen("/spiffs/log.csv", "a");
    //if (logFile == NULL) {
    //    ESP_LOGE(fileSys, "Failed to open LOG file for writing");
    //    xSemaphoreGive(spiffsSemaphore);
    //    return;
    //}
    //else ESP_LOGI(fileSys, "LOG file opened");
    //char* timeString = (char*) malloc(sizeof(char)*64);
    //int32_t timeStringLength = timeToString(timeString);
    //char* log = (char*) malloc(sizeof(char)*(strlen(line) + timeStringLength + 4));
    //strcpy(log, timeString);
    //strcat(log, " ");
    //strcat(log, line);
    //strcat(log, "\r\n");
    //fprintf(logFile, log);
    //ESP_LOGI(fileSys, "LOG written: %s", log);
    //fclose(logFile);
    //ESP_LOGI(fileSys, "LOG file closed");
    //size_t total = 0, used = 0;
    //esp_err_t ret = esp_spiffs_info(NULL, &total, &used);
    //if (ret != ESP_OK) {
    //    ESP_LOGE(fileSys, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    //} else {
    //    ESP_LOGI(fileSys, "Partition free space: %d", total-used);
    //}
    //xSemaphoreGive(spiffsSemaphore);
    //vTaskDelay(15 / portTICK_RATE_MS);
    //free(timeString);
    //free(log);
}

void writeOfflineData(uint8_t module, uint32_t idCaminhao, uint32_t idSensor, uint16_t pressure, uint8_t temp){
    
    //TODO: add fourth sensor
    // if(!oldSensorData[0].active || !oldSensorData[1].active || !oldSensorData[2].active){
    //     char* newSensorLine = (char*) malloc(sizeof(char)*32);
    //     sprintf(newSensorLine, "New sensor id: %X", idSensor);
    //     writeLog(newSensorLine);
    //     free(newSensorLine);
    //     for(uint8_t i=0; i<4; i++){
    //         if(!oldSensorData[i].active){
    //             oldSensorData[i].active = true;
    //             oldSensorData[i].caminhaoId = idCaminhao;
    //             oldSensorData[i].sensorId = idSensor;
    //             oldSensorData[i].pressure = pressure;
    //             oldSensorData[i].temp = temp;
    //             while(xSemaphoreTake(gpsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);
    //             oldSensorData[i].lat = lat;
    //             oldSensorData[i].lon = lon;
    //             xSemaphoreGive(gpsSemaphore);
    //             while(xSemaphoreTake(tStmpSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);
    //             time_t tt = time(NULL); //Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual
    //             oldSensorData[i].dthora = *gmtime(&tt); //Converte o tempo atual e atribui na estrutura
    //             xSemaphoreGive(tStmpSemaphore);
    //             break;
    //         }
    //     }
    // }
    // else{
    //     for(uint8_t index=0; index<4; index++){
    //         if(oldSensorData[index].sensorId == idSensor){
    //             while(xSemaphoreTake(gpsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);
    //             while(xSemaphoreTake(tStmpSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);
    //             if(oldSensorData[index].lat != lat || oldSensorData[index].lon != lon || oldSensorData[index].pressure != pressure || oldSensorData[index].temp != temp || abs(tStmp.tm_min - oldSensorData[index].dthora.tm_min) >= 5){
    //                 oldSensorData[index].lat = lat;
    //                 oldSensorData[index].lon = lon;
    //                 oldSensorData[index].pressure = pressure;
    //                 oldSensorData[index].temp = temp;
    //                 time_t tt = time(NULL); //Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual
    //                 oldSensorData[index].dthora = *gmtime(&tt); //Converte o tempo atual e atribui na estrutura
    //                 char* newDataLine = (char*) malloc(sizeof(char)*64);
    //                 sprintf(newDataLine, "New data for sensor %X", idSensor);
    //                 writeLog(newDataLine);
    //                 free(newDataLine);
    //             }
    //             else{
    //                 xSemaphoreGive(gpsSemaphore);
    //                 xSemaphoreGive(tStmpSemaphore);
    //                 return;
    //             }
    //             xSemaphoreGive(gpsSemaphore);
    //             xSemaphoreGive(tStmpSemaphore);
    //             break;
    //         }
    //     }
    // }

    while(xSemaphoreTake(spiffsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE){
        vTaskDelay(10 / portTICK_RATE_MS);
    }
    static const char *fileSys = "SPIFFS";
    
    FILE* f = NULL;
    struct stat st;

    if (stat("/spiffs/offline.csv", &st) != 0) {
        ESP_LOGI(fileSys, "Creating Offline File.");
    
        f = fopen("/spiffs/offline.csv", "a");
        if (f == NULL) {
            ESP_LOGE(fileSys, "Failed to create offline file for writing");
        }
        else{
            fprintf(f, "time,modulo,idcaminhao,idsensor,pressao,temp,lat,lon\r\n");
            fclose(f);
            ESP_LOGI(fileSys, "Offline Header Written");
        }
    }
    else{
        f = fopen("/spiffs/offline.csv", "a");
        ESP_LOGI(fileSys, "Offline File Already Exists.");  
    } 

    if (f == NULL) {
        ESP_LOGE(fileSys, "Failed to open file for writing");
        xSemaphoreGive(spiffsSemaphore);
        vTaskDelay(50 / portTICK_RATE_MS);
        return;
    }
    else ESP_LOGI(fileSys, "Offline file opened");
    
    char* timeString = (char*) malloc(sizeof(char)*64);
    //char* gpsString = (char*) malloc(sizeof(char)*64);
    int32_t timeStringLength = timeToString(timeString);
    //int32_t gpsStringLength = gpsToString(gpsString);

    // char* log = (char*) malloc(sizeof(char)*(timeStringLength + 5 + 6 + 4 + gpsStringLength + 16));
    char* log = (char*) malloc(sizeof(char)*512);
    while(xSemaphoreTake(gpsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(1 / portTICK_RATE_MS);
        //"time,modulo,idcaminhao,idsensor,pressao,temp,lat,lon\r\n"
    if((lat != 0.0) && (lon != 0.0))
        sprintf(log, "%s,%d,%d,%X,%d,%d,%0.6lf,%0.6lf\r\n", timeString, module, idCaminhao, idSensor, pressure, temp, lat, lon);
    else
        sprintf(log, "%s,%d,%d,%X,%d,%d\r\n", timeString, module, idCaminhao, idSensor, pressure, temp);
    xSemaphoreGive(gpsSemaphore);
    fprintf(f, log);
    ESP_LOGI(fileSys, "Data written: %s", log);
    fclose(f);
    ESP_LOGI(fileSys, "Offline file closed");
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(fileSys, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(fileSys, "Partition free space: %d", total-used);
    }
    xSemaphoreGive(spiffsSemaphore);
    vTaskDelay(50 / portTICK_RATE_MS);
    //free(gpsString);
    free(timeString);
    free(log);
    ESP_LOGI(fileSys, "end of writeOfflineData");
}

void SDTask(void *args){
    static const char *TAG_SSD = "SDTask";
    sdmmc_card_t* card;
    esp_err_t ret = {0};
    ESP_LOGW(TAG_SSD, "SDtask INIT");
    while(true){
        while(xSemaphoreTake(SDSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(1 / portTICK_RATE_MS);
        while(xSemaphoreTake(spiffsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(1 / portTICK_RATE_MS);
        struct stat st;
        if (stat("/spiffs/log.txt", &st) == 0){
            //ret = mountSDCard(&card);
            // if (ret != ESP_OK) ESP_LOGW(TAG_SSD, "Failed to mount filesystem.");
            // else {
            FILE* SDLog;
            FILE* internLog;
            //ESP_LOGW(TAG_SSD, "SD card mounted.");
            if (stat("/sdcard/log.txt", &st) != 0) {
                ESP_LOGW(TAG_SSD, "Creating log.txt in SDcard");
            
                SDLog = fopen("/sdcard/log.txt", "a");

                if (SDLog == NULL) ESP_LOGW(TAG_SSD, "Failed to create log e for writing in SDcard");
                else{
                    fprintf(SDLog, "LOG FILE\r\n");
                    fclose(SDLog);
                    ESP_LOGW(TAG_SSD, "log header Written");
                }
            }
            else{
                ESP_LOGW(TAG_SSD, "log.txt Already Exists in SDcard.");
            }
            SDLog = fopen("/sdcard/log.txt", "a");
            internLog = fopen("/spiffs/log.txt", "r");

            char line[512];
            ESP_LOGW("SDcopy", "Starting to copy LOG file");
            while(fgets(line, sizeof(line), internLog) != NULL){
                for(int i=0;i<strlen(line);i++){
                    if(line[i] == '\n' || line[i] == '\r') line[i] = '\0';
                }
                strcat(line, "\r\n");
                fprintf(SDLog, line);
            }
            fclose(SDLog);
            fclose(internLog);
            remove("/spiffs/log.txt");
        }
        else{
            ESP_LOGW(TAG_SSD, "log file wasnt found in intern memory to copy to SDcard");
        }
        
        xSemaphoreGive(SDSemaphore);
        xSemaphoreGive(spiffsSemaphore);
        vTaskDelay(50 / portTICK_RATE_MS);
        ESP_LOGW(TAG_SSD, "SDtask gave semaphores");
        vTaskDelay(LOG_COPY_PERIOD);
    }
}

/*
**mounts the SD card
**initialize SD Card GPIOs using SPI Interface
*/
esp_err_t mountSDCard(sdmmc_card_t** card){
    // Initializing SD Card variables
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = PIN_NUM_MISO;
    slot_config.gpio_mosi = PIN_NUM_MOSI;
    slot_config.gpio_sck  = PIN_NUM_CLK;
    slot_config.gpio_cs   = PIN_NUM_CS;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    // Function to initialize SD Card GPIOs using SDMMC Interface
    return esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, card);
}

/* Initialize SD card
** verifies if offline.csv exists. if it doesnt creates it and writes its header
** gets connection string from string.txt if the file exists
*/
void initSDCard(void){
    static const char *TAG_SSD = "SD";
    sdmmc_card_t* card;
    esp_err_t ret = {0};
    
    struct stat st;
    FILE* f;
    // SDSemaphore = xSemaphoreCreateBinary();
    // if(SDSemaphore == NULL){ 
    //     ESP_LOGI(TAG_SSD, "SD semaphore create failed");
    //     writeLog("SD semaphore create failed");
    // }
    // else ESP_LOGI(TAG_SSD, "SD semaphore create successed");
    // xSemaphoreGive(SDSemaphore);
    while(xSemaphoreTake(SDSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(1 / portTICK_RATE_MS);

    ret = mountSDCard(&card);


    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG_SSD, "Failed to mount filesystem. "
                "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG_SSD, "Failed to initialize the card (%s). "
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
    }
    else{
        ESP_LOGI(TAG_SSD, "SD card mounted.");
        // Card has been initialized, print its properties
        sdmmc_card_print_info(stdout, card);

        if (stat("/sdcard/offline.csv", &st) != 0) {
            ESP_LOGI(TAG_SSD, "Creating Offline File.");
        
            f = fopen("/sdcard/offline.csv", "w");

            if (f == NULL){ 
                ESP_LOGE(TAG_SSD, "Failed to open offline file for writing");
                writeLog("Failed to open offline file for writing");
            }
            else{
                fprintf(f, "time,modulo,idcaminhao,idsensor,pressao,temp,lat,lon\r\n");
                fclose(f);
                ESP_LOGI(TAG_SSD, "Offline Header Written");
            }
        }
        else{
          ESP_LOGI(TAG_SSD, "Offline File Already Exists in SDcard.");
          //writeLog("Offline File Already Exists in SD Card.");  
        } 
        
        if (stat("/sdcard/string.txt", &st) == 0) { 

            f = fopen("/sdcard/string.txt", "r");
            if (f == NULL) {
                ESP_LOGE(TAG_SSD, "Failed to open String.txt");
            }
            else{
                //minha solução para o problema do tamanho da string
                char buf[512];
                // int i = 0;
                // for(i=0; fgetc(f)!=EOF; i++);                   //conta quantos caracteres tem no arquivo
                // fseek(f, 0, SEEK_SET);                          //volta o ponteiro ao inicio do arquivo
                // buf = (char*) malloc(sizeof(char)*(i+2));        //aloca memória suficiente para caber a string + \0
                fgets(buf, 512, f);                             //copia a string do arquivo para buf
                buf[511] = '\0';                                //adiciona \0 ao final de buf
                char* pos = strchr(buf, '\n');                  //checa se não houve nenhum \n no meio da string lida do arquivo
                if (pos != NULL) *pos = '\0';                   //se encontrou algum \n coloca um \0 no lugar
                fclose(f);
                connectionString = (char *)malloc(sizeof(char)*(strlen(buf)+1));
                strcpy(connectionString, buf);
                //free(buf);
                ESP_LOGI(TAG_SSD, "Connection String: %s", connectionString);
                //writeLog("Connection String retrieved with success.");
            }
        }
        else{
            connectionString = NULL;

            ESP_LOGI(TAG_SSD, "Unable to get Connection String. File Does Not Exist.");
            writeLog("Unable to get Connection String. File Does Not Exist.");
        }
    }
    xSemaphoreGive(SDSemaphore);
}


void writeToQueue(void *args){
    ESP_LOGE("queue", "queue task initiated");
    while(true){
        vTaskDelay(WRITE_QUEUE_PERIOD);
        while(xSemaphoreTake(spiffsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);
        while(xSemaphoreTake(SDSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);

        ESP_LOGE("queue", "queue procedure initiated");
        static const char *fileSys = "SPIFFS";
        struct stat st;
        if(stat("/spiffs/offline.csv", &st) == 0){
            FILE* SDOffline;
            FILE* internOffline;
            if (stat("/sdcard/offline.csv", &st) != 0) {
                ESP_LOGI("queue", "Creating offline.csv in SDcard");
            
                SDOffline = fopen("/sdcard/offline.csv", "a");

                if (SDOffline == NULL) ESP_LOGE("queue", "Failed to create Offline e for writing in SDcard");
                else{
                    fprintf(SDOffline, "time,modulo,idcaminhao,idsensor,pressao,temp,lat,lon\r\n");
                    fclose(SDOffline);
                    ESP_LOGI("queue", "Offline header Written");
                }
            }
            else{
                ESP_LOGI("queue", "offline.csv Already Exists in SDcard.");
            }

            SDOffline = fopen("/sdcard/offline.csv", "a");
            internOffline = fopen("/spiffs/offline.csv", "r");

            char line[512];
            ESP_LOGE("queue", "Starting to copy LOG file");
            fgets(line, sizeof(line), internOffline); // ignoring first line, it's the header
            while(fgets(line, sizeof(line), internOffline) != NULL){
                for(int i=0;i<strlen(line);i++){
                    if(line[i] == '\n' || line[i] == '\r') line[i] = '\0';
                }
                strcat(line, "\r\n");
                fprintf(SDOffline, line);
            }
            fclose(SDOffline);
            fclose(internOffline);
        }
        else{
            ESP_LOGI("queue", "offline file wasnt found in intern memory to copy to SDcard");
        }
        if (stat("/spiffs/offline.csv", &st) == 0){
            ESP_LOGE("queue", "Found offline.csv file");
            FILE* offlineFile = fopen("/spiffs/offline.csv", "r");
            FILE* queueFile = fopen("/sdcard/queue.txt", "a");
            if (queueFile == NULL) {
                ESP_LOGE(fileSys, "Failed to open queue file for writing");
                return;
            }
            char line[512];
            fgets(line, sizeof(line), offlineFile); //Discarta a primeira linha pois é o header
            if(line == NULL) ESP_LOGE("queue", "empty offline file");
            else ESP_LOGE("queue", "first line of offline file: %s", line);

            ESP_LOGE("queue", "starting queue write loop");
            while(fgets(line, sizeof(line), offlineFile) != NULL){
                ESP_LOGE("queue", "Reading line of offline.csv file");
                for(int i=0;i<strlen(line);i++){
                    if(line[i] == '\n' || line[i] == '\r') line[i] = '\0';
                }
                char* time = strtok(line, ",");
                char* module = strtok(NULL, ",");
                module--;
                module[0] = '\0';
                module++;
                char* idCaminhao = strtok(NULL, ",");
                idCaminhao--;
                idCaminhao[0] = '\0';
                idCaminhao++;
                char* idSensor = strtok(NULL, ",");
                idSensor--;
                idSensor[0] = '\0';
                idSensor++;
                char* pressure = strtok(NULL, ",");
                pressure--;
                pressure[0] = '\0';
                pressure++;
                char* temp = strtok(NULL, ",");
                temp--;
                temp[0] = '\0';
                temp++;
                char* latitude = strtok(NULL, ",");
                char* longitude = strtok(NULL, ",");
                if(latitude != NULL && longitude != NULL){
                    latitude--;
                    latitude[0] = '\0';
                    latitude++;
                    longitude--;
                    longitude[0] = '\0';
                    longitude++;
                    char* queueLine = (char*) malloc(sizeof(char)*512);
                    sprintf(queueLine, "{\"modulo\":%s,\"IdCaminhao\":%s,\"IdSensor\":\"%s\",\"Pressao\":%s,\"Temperatura\":%s,\"Latitude\":%s,\"Longitude\":%s,\"dthora\":\"%s\"}\n", module, idCaminhao, idSensor, pressure, temp, latitude, longitude, time);
                    fprintf(queueFile, queueLine);
                    free(queueLine);
                }
                else{
                    char* queueLine = (char*) malloc(sizeof(char)*512);
                    sprintf(queueLine, "{\"modulo\":%s,\"IdCaminhao\":%s,\"IdSensor\":\"%s\",\"Pressao\":%s,\"Temperatura\":%s,\"dthora\":\"%s\"}\n", module, idCaminhao, idSensor, pressure, temp, time);
                    fprintf(queueFile, queueLine);
                    free(queueLine);
                }
            }
            fclose(offlineFile);
            fclose(queueFile);
            remove("/spiffs/offline.csv");
        }
        else{
            ESP_LOGI(fileSys, "There is no offline data available!");
        }
        xSemaphoreGive(spiffsSemaphore);
        vTaskDelay(50 / portTICK_RATE_MS);
        //simCom.h
        while(xSemaphoreTake(simSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);

        ESP_LOGE("queue", "Starting iotHub run!");
        //iothub_client_esp32_run();

        //simCom.h
        xSemaphoreGive(simSemaphore);

        ESP_LOGE("queue", "queue procedure ended");
        xSemaphoreGive(SDSemaphore);
    }
}