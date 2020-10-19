// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <esp_wifi.h>

#include "iothub_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "iothubtransportmqtt.h"
#include "iothub_client_options.h"
#include "azure_c_shared_utility/agenttime.h"

#include "iothub_client_core_ll.h"
#include "iothub_transport_ll.h"
#include "internal/iothubtransport.h"

#include "esp_log.h"
#include "driver/uart.h"

#include "libGSM.h"
#include "IoT_Hub_MQTT.h"
#include "tlsio_pal.h"

#include "driver/i2c.h"
#include "xi2c.h"
#include "fonts.h"
#include "ssd1306.h"
#include "nvs_flash.h"

#include "nvs_flash.h"
#include "nvs.h"

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_spiffs.h"

#include <time.h>
#include <sys/time.h>
#include "lwip/apps/sntp.h"
#include "lwip/err.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/xlogging.h"

#include "freertos/event_groups.h"

#define I2C_EXAMPLE_MASTER_SCL_IO    21    /*!< gpio number for I2C master clock */
#define I2C_EXAMPLE_MASTER_SDA_IO    22    /*!< gpio number for I2C master data  */
#define I2C_EXAMPLE_MASTER_NUM I2C_NUM_1   /*!< I2C port number for master dev */
#define I2C_EXAMPLE_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_EXAMPLE_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */

static int callbackCounter;
static bool g_continueRunning;
#define MESSAGE_COUNT 25
#define DOWORK_LOOP_NUM     3

static const char *TAG = "AZURE";
//static const char *TAG_FFS = "SPIFFS";
nvs_handle my_handle;

#define PORT_NUM_ENCRYPTED          8883

#define DEFAULT_MSG_TO_SEND         1

time_t now1 = 0;
//int flag_init = 1;
//struct tm time_new = { 0 };
//struct tm time_old = { 0 };

time_t sntp_get_current_timestamp();

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
//static const char* connectionString = "HostName=IngestorPrincipal.azure-devices.net;DeviceId=medidor1;SharedAccessKey=B3/r+q4xpKCn5Ar2592aCQ1iQkjKX+DD1b3RTveC1Bs=";
//static const char* connectionString = "HostName=poc-qg-iothub.azure-devices.net;DeviceId=caminhaopiloto;SharedAccessKey=nwUaVPIWKBCkegvGFULS7gjgLuudPc54iwdLXW2Rvxc=";

typedef struct EVENT_INSTANCE_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    size_t messageTrackingId;  // For tracking the messages within the user callback.
} EVENT_INSTANCE;

uint8_t* rx_payload_task()
{
    uint8_t* data = (uint8_t*) malloc(23);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, 23, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            if (data[0] == '<'){
                if (data[22] == '>'){
                    vTaskDelay(2000 / portTICK_PERIOD_MS);
                    return data;
                }
            }
        }else{
            return (uint8_t*) "NO_PAYLOAD_RECEIVED";
        }
    }
    free(data);
}

/**
 * @brief i2c master initialization
 */
void i2c_master_init()
{
    int i2c_master_port = I2C_EXAMPLE_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_EXAMPLE_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_EXAMPLE_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_EXAMPLE_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_EXAMPLE_MASTER_RX_BUF_DISABLE,
                       I2C_EXAMPLE_MASTER_TX_BUF_DISABLE, 0);
    ESP_LOGI("SCREEN", "i2c driver installed.");
}

static IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
    int* counter = (int*)userContextCallback;
    const char* buffer;
    size_t size;
    MAP_HANDLE mapProperties;
    const char* messageId;
    const char* correlationId;

    // Message properties
    if ((messageId = IoTHubMessage_GetMessageId(message)) == NULL)
    {
        messageId = "<null>";
    }

    if ((correlationId = IoTHubMessage_GetCorrelationId(message)) == NULL)
    {
        correlationId = "<null>";
    }

    // Message content
    if (IoTHubMessage_GetByteArray(message, (const unsigned char**)&buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        ESP_LOGE(TAG,"unable to retrieve the message data\r\n");
    }
    else
    {
        ESP_LOGE(TAG,"Received Message [%d]\r\n Message ID: %s\r\n Correlation ID: %s\r\n Data: <<<%.*s>>> & Size=%d\r\n", *counter, messageId, correlationId, (int)size, buffer, (int)size);
        // If we receive the work 'quit' then we stop running
        if (size == (strlen("quit") * sizeof(char)) && memcmp(buffer, "quit", size) == 0)
        {
            g_continueRunning = false;
        }
    }

    // Retrieve properties from the message
    mapProperties = IoTHubMessage_Properties(message);
    if (mapProperties != NULL)
    {
        const char*const* keys;
        const char*const* values;
        size_t propertyCount = 0;
        if (Map_GetInternals(mapProperties, &keys, &values, &propertyCount) == MAP_OK)
        {
            if (propertyCount > 0)
            {
                size_t index;

                ESP_LOGE(TAG," Message Properties:\r\n");
                for (index = 0; index < propertyCount; index++)
                {
                    ESP_LOGE(TAG,"\tKey: %s Value: %s\r\n", keys[index], values[index]);
                }
                ESP_LOGE(TAG,"\r\n");
            }
        }
    }

    /* Some device specific action code goes here... */
    (*counter)++;
    return IOTHUBMESSAGE_ACCEPTED;
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    EVENT_INSTANCE* eventInstance = (EVENT_INSTANCE*)userContextCallback;
    size_t id = eventInstance->messageTrackingId;

    ESP_LOGE(TAG,"Confirmation[%d] received for message tracking id = %d with result = %s\r\n", callbackCounter, (int)id, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    /* Some device specific action code goes here... */
    callbackCounter++;
    IoTHubMessage_Destroy(eventInstance->messageHandle);
}

static void log_info(const char* text){
    static char logText[256];
    struct tm time_log = { 0 };

    time(&now1);
    localtime_r(&now1, &time_log);

    sprintf_s(logText, sizeof(logText), "%d/%d/%d-%d:%d:%d\t%s\n",time_log.tm_mday, time_log.tm_mon + 1, time_log.tm_year + 1900, time_log.tm_hour, time_log.tm_min, time_log.tm_sec,text);

    FILE* f = fopen("/spiffs/log.txt", "a+");
    fprintf(f, logText);
    ESP_LOGE("LOG", "Log Written.");
    //ESP_LOGE(TAG_FFS,"%s",elemt_tmp);
    fclose(f);
    flag_pay = 1;

}

static int connect_init(){

    if (ppposStatus() == GSM_STATE_IDLE || ppposStatus() == GSM_STATE_FIRSTINIT){
        if (ppposInit() == 0){
            return 0;
        }else{
            return 1;
        }
    }else{
        return 0;
    }
    
}

void iothub_client_esp32_run()
{

    ESP_LOGE(TAG,"iothub_client_esp32_run\r\n");   
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

    EVENT_INSTANCE messages[25]; //[MESSAGE_COUNT];

    g_continueRunning = true;
    srand((unsigned int)time(NULL));
    callbackCounter = 0;
    int receiveContext = 0;
    int time_limit = 0;

    screen_msg( NULL, "Initializing", "IoT Hub Client");

    ESP_LOGE(TAG,"PPPos Status %d.\r\n", ppposStatus());

    ESP_LOGE(TAG, "Connection String: %s", connectionString);

    int connect = 0;
    for(uint8_t i=0; i<3; i++){
        connect = connect_init();
        if(connect == 1) break;
    }
    if (connect == 0)
    { //falhou
        ESP_LOGE(TAG,"Failed to initialize the platform.\r\n");

        screen_msg( NULL, "ERROR: Failed to", "Initialize Platform");

        log_info("Failed to initialize the platform.");

    }
    else
    { //iniciou
        
        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol)) == NULL)
        { //falhou
            ESP_LOGE(TAG,"ERROR: iotHubClientHandle is NULL!\r\n");

            screen_msg( NULL, "ERROR: Invalid", "Connection String");

            log_info("Invalid Connection String.");

        }
        else
        { //created from connection string
    
            // screen_msg( NULL, "PPP", "Initialized");

            bool traceOn = true;
            IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &traceOn);

            /* Setting Message call back, so we can receive Commands. */
            if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, &receiveContext) != IOTHUB_CLIENT_OK)
            { //falhou
                ESP_LOGE(TAG,"ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");

                screen_msg( NULL, "ERROR: Setting", "Message Callback");

                log_info("Error setting message Callback.");

            }
            else
            { //callback message set
                ESP_LOGE(TAG,"IoTHubClient_LL_SetMessageCallback...successful.\r\n");

                /* Now that we are ready to receive commands, let's send some messages */
                int iterator = 0;

                screen_msg( NULL, "Sending Data to", "Azure IoT Hub");

                do
                {
                    if (iterator <= callbackCounter){

                        // process the line read.
                        ESP_LOGE(TAG,"iterator: [%d], callbackCounter: [%d]. \r\n", iterator, callbackCounter);

                        // Check if destination file exists before renaming ***comment from exemple, simply checking if the file exists***
                        struct stat st;
                        if (stat("/sdcard/queue.txt", &st) == 0) {
                    
                            FILE* handle = fopen("/sdcard/queue.txt", "r");
                            char line[512];

                            if (handle) {   //handle != NULL
                                int counter = 0;
                                //TODO: otimizar leitura do arquivo
                                while (fgets(line, sizeof(line), handle)) {
                                    
                                    for(int i=0;i<strlen(line);i++){
                                        if(line[i] == '\n'){
                                            line[i] = '\0';
                                        }
                                    }

                                    if ((messages[iterator].messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)line, strlen(line))) == NULL)
                                    { //falhou
                                        ESP_LOGE(TAG,"ERROR: iotHubMessageHandle is NULL!\r\n");

                                        screen_msg( NULL, "Iot Hub Message", "Handle is Null");

                                        log_info("IoT Hub Message Handle is Null.");
                                    }
                                    else
                                    { //message created from byte array
                                        messages[iterator].messageTrackingId = iterator;
                                        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messages[iterator].messageHandle, SendConfirmationCallback, &messages[iterator]) != IOTHUB_CLIENT_OK)
                                        {//falhou
                                            ESP_LOGE(TAG,"ERROR: IoTHubClient_LL_SendEventAsync..........FAILED!\r\n");

                                            screen_msg( NULL, "Failed to Send", "Event Async.");

                                            log_info("Failed to Send Event Async.");

                                        }
                                        else
                                        {
                                            ESP_LOGE(TAG,"IoTHubClient_LL_SendEventAsync accepted message [%d] for transmission to IoT Hub.\r\n", (int)iterator);
                                        }
                                    }
                                    iterator++;
                                    counter++;

                                    if(counter >= MESSAGE_COUNT){                                            
                                        ESP_LOGE(TAG,"IoT Hub Client got overloaded, sending all Payloads");
                                        size_t index = 0;
                                        for (index = 0; index < DOWORK_LOOP_NUM; index++)
                                        {
                                            IoTHubClient_LL_DoWork(iotHubClientHandle);
                                            ThreadAPI_Sleep(1);
                                        }

                                        time_limit = 0;
                                        while((callbackCounter < iterator) && (g_continueRunning == true)){
                                            ESP_LOGE(TAG,"Waiting for Callback. %d seconds Left.\r\n", (30-time_limit));
                                            IoTHubClient_LL_DoWork(iotHubClientHandle);
                                            ThreadAPI_Sleep(1000);

                                            if(time_limit >= 30){
                                                g_continueRunning = false;

                                                screen_msg( NULL, "ERROR: Check", "Connection String");

                                                log_info("IoT Hub Device Problem! Check Connection String.");

                                                break;
                                            }
                                            time_limit++;
                                        }
                                        ESP_LOGE(TAG,"Callback = Msgs sent!\r\n");

                                        counter = 0;
                                        iterator = 0;
                                        callbackCounter = 0;
                                    }
                                }

                                time_limit = 0;
                                while((callbackCounter < iterator) && (g_continueRunning == true)){
                                    ESP_LOGE(TAG,"Waiting for Callback. %d seconds Left.\r\n", (30-time_limit));
                                    IoTHubClient_LL_DoWork(iotHubClientHandle);
                                    ThreadAPI_Sleep(1000);

                                    if(time_limit >= 30){
                                        g_continueRunning = false;

                                        screen_msg( NULL, "ERROR: Check", "Connection String");

                                        log_info("IoT Hub Device Problem! Check Connection String.");

                                        break;
                                    }
                                    time_limit++;
                                }
                                ESP_LOGE(TAG,"Callback = Msgs sent!\r\n");

                                
                                fclose(handle);
                                // Delete it if it exists
                                remove("/sdcard/queue.txt");
                                struct stat st_test;
                                if(stat("/sdcard/queue.txt", &st_test) == 0) unlink("/sdcard/queue.txt");

                            }else {
                                // error opening the file.
                                ESP_LOGE("FFS","ERROR Opening queue file!\r\n");
                                break;
                            }   
                        }else{
                            ESP_LOGE("FFS","Queue file does not exists!\r\n");
                            break;
                        }

                    }
                        
                    IoTHubClient_LL_DoWork(iotHubClientHandle);
                    ThreadAPI_Sleep(1);  

                    if (callbackCounter >= MESSAGE_COUNT)
                    {
                        ESP_LOGE(TAG,"Exit IoT Hub Routine.\r\n");
                        break;
                    }
                } while (g_continueRunning);

                ESP_LOGE(TAG,"iothub_client_sample_mqtt has gotten quit message, call DoWork %d more time to complete final sending...\r\n", DOWORK_LOOP_NUM);
                size_t index = 0;
                for (index = 0; index < DOWORK_LOOP_NUM; index++)
                {
                    IoTHubClient_LL_DoWork(iotHubClientHandle);
                    ThreadAPI_Sleep(1);
                }

            }
            IoTHubClient_LL_Destroy(iotHubClientHandle);   
        }
        ppposDisconnect(0, 0);

    }

    screen_msg(NULL, "Closed Azure", "IoT Hub Client");

}