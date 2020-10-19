#include "simCom.h"

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

const char *TAG = "simCom";
// SemaphoreHandle_t simSemaphore = NULL;
// SemaphoreHandle_t gpsSemaphore = NULL;
// SemaphoreHandle_t tStmpSemaphore = NULL;

void initSim(void){
    // simSemaphore = xSemaphoreCreateBinary();
    // xSemaphoreGive(simSemaphore);
    // gpsSemaphore = xSemaphoreCreateBinary();
    // xSemaphoreGive(gpsSemaphore);
    // tStmpSemaphore = xSemaphoreCreateBinary();
    // xSemaphoreGive(tStmpSemaphore);
    setGPIO();
    initUart(2, false);
    //rstBoard();
    
    atTest();
    atHandler("AT+CREG=1", NULL);
    //configSimClock();
    //updateClock();
    getGSMLOC();
}

void getGSMLOC(void){
    //Messages
    char* sapbr1 = "AT+SAPBR=3,1,\"Contype\",\"GPRS\"";
    char* sapbr2 = "AT+SAPBR=3,1,\"APN\",\"claro.com.br\"";
    char* sapbr3 = "AT+SAPBR=3,1,\"USER\",\"claro\"";
    char* sapbr4 = "AT+SAPBR=3,1,\"PWD\",\"claro\"";
    char* sapbr5 = "AT+SAPBR =1,1";
    char* sapbr6 = "AT+SAPBR=2,1";
    char* sapbr7 = "AT+SAPBR=0,1";
    char* gsmloc = "AT+CIPGSMLOC=1,1";
    char* response = (char*) malloc(sizeof(char)*256);
    char* lonS;
    char* latS;
    char *date, *timeST, *year, *month, *day, *hour, *min, *sec;

    atHandler(sapbr1, response);
    atHandler(sapbr2, response);
    atHandler(sapbr3, response);
    atHandler(sapbr4, response);
    atHandler(sapbr5, response);
    atHandler(sapbr6, response);
    atHandler(gsmloc, response);
    //do atHandler(gsmloc, response);
    //while(strstr(response, "+CIPGSMLOC: 0") == NULL);
    char* normalized = strstr(response, "+CIPGSMLOC: 0");
    if(normalized != NULL){
        strtok(normalized, ",");
        //strtok(NULL, ",");
        lonS = strtok(NULL, ",");
        latS = strtok(NULL, ",");
        latS--;
        latS[0] = '\0';
        latS++;
        date = strtok(NULL, ",");
        date--;
        date[0] = '\0';
        date++;
        timeST = strtok(NULL, "O");
        timeST--;
        timeST[0] = '\0';
        timeST++;
        for(uint8_t i=0; i<strlen(timeST); i++){
            if(timeST[i] == '\r' || timeST[i] == '\n') timeST[i] = '\0';
        }
        year = strtok(date, "/");
        month = strtok(NULL, "/");
        month--;
        month[0] = '\0';
        month++;
        day = month+3;
        day--;
        day[0] = '\0';
        day++;
        
        hour = strtok(timeST, ":");
        min = strtok(NULL, ":");
        min--;
        min[0] = '\0';
        min++;
        sec = min+3;
        sec--;
        sec[0] = '\0';
        sec++;

        while(xSemaphoreTake(tStmpSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);
        tStmp.tm_year = atoi(year) -1900;
        tStmp.tm_mon = atoi(month) -1;
        tStmp.tm_mday = atoi(day);
        tStmp.tm_hour = atoi(hour);
        tStmp.tm_min = atoi(min);
        tStmp.tm_sec = atoi(sec);
        //if(tStmp.tm_hour >= 3) tStmp.tm_hour -= 3;
        //else if(tStmp.tm_hour == 2) tStmp.tm_hour = 23;
        //else if(tStmp.tm_hour == 1) tStmp.tm_hour = 22;
        //else if(tStmp.tm_hour == 0) tStmp.tm_hour = 21;

        struct timeval tv;
        tv.tv_sec = mktime(&tStmp);
        settimeofday(&tv, NULL);
        time_t tt = time(NULL);//Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual
        //tStmp = *gmtime(&tt);//Converte o tempo atual e atribui na estrutura
        tStmp = *localtime(&tt);//Converte o tempo atual e atribui na estrutura
        char data_formatada[64];
        strftime(data_formatada, 64, "%d/%m/%Y %H:%M:%S", &tStmp);
        ESP_LOGI("RTC", "now = %s", data_formatada);
        xSemaphoreGive(tStmpSemaphore);

        while(xSemaphoreTake(gpsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);
        lat = atof(latS);
        lon = atof(lonS);
        ESP_LOGI("GSMLOC", "lat: %lf \tlon: %lf", lat, lon);
        xSemaphoreGive(gpsSemaphore);
    }
    atHandler(sapbr7, response);
    free(response);
}

void gpsTask(void *arg){
    // while(xSemaphoreTake(simSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);
    // if(startGPS()) writeLog("GPS link is up!"); //log.h
    // else {
    //     writeLog("Couldnt find GPS signal!"); //log.h
    //     ESP_LOGE("GPS", "Couldnt find GPS signal!");
    // }
    // xSemaphoreGive(simSemaphore);
    while(true){
        while(xSemaphoreTake(simSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE) vTaskDelay(10 / portTICK_RATE_MS);

        getGSMLOC();

        xSemaphoreGive(simSemaphore);
        vTaskDelay(10000 / portTICK_RATE_MS); 
    }
}

void clockTask(void *args){
    while(true){
        while(xSemaphoreTake(simSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE){
            vTaskDelay(10 / portTICK_RATE_MS);
        }
        updateClock();

        xSemaphoreGive(simSemaphore);
        vTaskDelay(CLOCK_UPDATE_PERIOD);
    }
}

void atTest(void){
    //Message
    char output[50];
    char input[1024];
    char* vazio = "\0";
    int8_t try = 4;
    char* line = NULL;

    strcpy(output, "AT");
    atHandler(output, input);
    while( (strstr(input, "OK") == NULL) && (try != 0) ){
        ESP_LOGI(TAG, "No answer, reseting...");
        rstBoard();
        ESP_LOGI(TAG, "Reseted!");
        free(line);
        line = (char*) (char*) malloc(sizeof(char)*50);
        sprintf (line, "Connection with sim808 failed, board reseted!");
        writeLog(line); //log.h
        strcpy(output, "AT");
        atHandler(output, input);
        try--;
    }
    if(strstr(input, "OK") == NULL){
        ESP_LOGI(TAG, "AT comm could not be started");
        free(line);
        line = (char*) (char*) malloc(sizeof(char)*50);
        sprintf (line, "Connection with sim808 could not be established");
        writeLog(line); //log.h
        return;
    }
    ESP_LOGI(TAG, "AT comm OK!");
    free(line);
    line = (char*) (char*) malloc(sizeof(char)*50);
    sprintf (line, "Connection with sim808 established!");
    //writeLog(line); //log.h
    strcpy(output, vazio);
    strcpy(input, vazio);
    strcpy(output, "AT+GMI");
    atHandler(output, input);
    free(line);

    strcpy(output, "ATI");
    atHandler(output, input);
    strcpy(input, vazio);
}

void configSimClock(void){
    //Message
    char output[50];
    char input[1024];
    char* vazio = "\0";
    //writeLog("Configuring Sim808 clock"); //log.h

    strcpy(output, "AT+CLTS=?");
    strcpy(input, vazio);
    atHandler(output, input);
    strcpy(output, vazio);
    strcpy(input, vazio);

    strcpy(output, "AT+CLTS=1");
    strcpy(input, vazio);
    atHandler(output, input);
    strcpy(output, vazio);
    strcpy(input, vazio);

    strcpy(output, "AT+CCLK=?");
    strcpy(input, vazio);
    atHandler(output, input);
    strcpy(output, vazio);
    strcpy(input, vazio);
    //writeLog("Sim808 clock consigured"); //log.h
}

void updateClock(void){
    //Message
    char output[50];
    char input[1024];
    char* vazio = "\0";

    strcpy(output, "AT+CCLK?");
    strcpy(input, vazio);
    atHandler(output, input);
    strcpy(output, vazio);
    char *ret;
    ret = strstr(input, "\"");
    ESP_LOGI("time", "now = %s", ret);
    strcpy(input, vazio);
    char y[3], m[3], d[3], h[3], mm[3], s[3];
    y[0] = ret[1]; y[1] = ret[2]; y[2] = '\0';
    m[0] = ret[4]; m[1] = ret[5]; m[2] = '\0';
    d[0] = ret[7]; d[1] = ret[8]; d[2] = '\0';
    h[0] = ret[10]; h[1] = ret[11]; h[2] = '\0';
    mm[0] = ret[13]; mm[1] = ret[14]; mm[2] = '\0';
    s[0] = ret[16]; s[1] = ret[17]; s[2] = '\0';
    
    while(xSemaphoreTake(tStmpSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE){
        vTaskDelay(10 / portTICK_RATE_MS);
    }
    tStmp.tm_year = atoi(y) +100;
    tStmp.tm_mon = atoi(m) -1;
    tStmp.tm_mday = atoi(d);
    tStmp.tm_hour = atoi(h);
    tStmp.tm_min = atoi(mm);
    tStmp.tm_sec = atoi(s);
    struct timeval tv;
    tv.tv_sec = mktime(&tStmp);
    settimeofday(&tv, NULL);
    time_t tt = time(NULL);//Obtem o tempo atual em segundos. Utilize isso sempre que precisar obter o tempo atual
    tStmp = *gmtime(&tt);//Converte o tempo atual e atribui na estrutura
    char data_formatada[64];
    strftime(data_formatada, 64, "%d/%m/%Y %H:%M:%S", &tStmp);
    ESP_LOGI("time", "now = %d/%d/%d %d:%d:%d", tStmp.tm_mday, tStmp.tm_mon+1, tStmp.tm_year+1900, tStmp.tm_hour, tStmp.tm_min, tStmp.tm_sec);
    ESP_LOGI("RTC", "now = %s", data_formatada);
    xSemaphoreGive(tStmpSemaphore);
}

void atHandler(char* msg, char* response){
    uartSendData(UART_NUM_2, msg); //uart.h
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    if(response != NULL) uartReceiveData(UART_NUM_2, response); //uart.h
    if((response != NULL) && strstr(response, "POWER DOWN") != NULL){
        writeLog("GSM BOARD POWER DOWN");
        esp_restart();
    }
}

bool startGPS(){
    //Messages
    char* pwrGPSOn = "AT+CGPSPWR=1";
    char* GPSreset = "AT+CGPSRST=0";
    char* GPSstatus = "AT+CGPSSTATUS?";
    //data
    char* response = (char*) malloc(sizeof(char)*256);

    int8_t try = 10;

    atHandler(pwrGPSOn, response);
    atHandler(GPSreset, response);
    atHandler(GPSstatus, response);
    while((strstr(response, "Location 3D Fix") == NULL) && (try > 0)){
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        atHandler(GPSstatus, response);
        try--;
    }
    free(response);
    if(try > 0) return true;
    return false;
}

bool isGPSOn(void){
    char* GPSstatus = "AT+CGPSSTATUS?";
    char* response = (char*) malloc(sizeof(char)*256);
    atHandler(GPSstatus, response);
    if(strstr(response, "Location 3D Fix") != NULL) return true;
    if(strstr(response, "NORMAL POWER DOWN") != NULL) rstBoard();
    return false;
}

void simCom_getGPS(void){
    if(!isGPSOn()){
        writeLog("GPS not connected. Trying to connect again");
        startGPS();
        return;
    }
    char* getGPSinfo = "AT+CGPSINF=4";
    //data
    char* response = (char*) malloc(sizeof(char)*256);
    char *strLon, strLonDec[3], strLonMin[10];
    char *strLat, strLatDec[3], strLatMin[10];

    atHandler(getGPSinfo, response);
    strtok(response, ",");
    strLat = strtok(NULL, ",");
    strncpy(strLatDec, strLat, 2);
    strncpy(strLatMin, (strLat+2), 9);
    strLatMin[9] = '\0';
    strLatDec[2] = '\0';

    while(xSemaphoreTake(gpsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE){
        vTaskDelay(10 / portTICK_RATE_MS);
    }
    lat = atof(strLatDec) + (atof(strLatMin)/60.0);
    if(strstr(strtok(NULL, ","), "N") == NULL) lat = -1*lat;
    xSemaphoreGive(gpsSemaphore);

    strLon = strtok(NULL, ",");
    strncpy(strLonDec, strLon, 2);
    strncpy(strLonMin, (strLon+2), 9);
    strLonMin[9] = '\0';
    strLonDec[2] = '\0';

    while(xSemaphoreTake(gpsSemaphore, ( TickType_t ) 1 / portTICK_RATE_MS) != pdTRUE){
        vTaskDelay(10 / portTICK_RATE_MS);
    }
    lon = atof(strLonDec) + (atof(strLonMin)/60.0);
    if(strstr(strtok(NULL, ","), "E") == NULL) lon = -1*lon;
    xSemaphoreGive(gpsSemaphore);
    
    ESP_LOGI("GPS", "lat->\t(ddmm.mmmmmm):%s\t(dd.dddddd):%lf", strLat, lat);
    ESP_LOGI("GPS", "long->\t(ddmm.mmmmmm):%s\t(dd.dddddd):%lf", strLon, lon);
    free(response);
    response = (char*) malloc(sizeof(char)*64);
    sprintf(response, "GPS location updated: \tLAT = %lf \tLON = %lf", lat, lon);
    //writeLog(response); //log.h
    free(response);
}

void rstBoard(void){
    ESP_LOGI("simCom", "reseting board");
    gpio_set_level(RESET_PIN, 1);
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    gpio_set_level(RESET_PIN, 0);
    ESP_LOGI("simCom", "board reseted");
}

void setGPIO(void){
    //gpio configuration handler
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SIMCOM;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}
