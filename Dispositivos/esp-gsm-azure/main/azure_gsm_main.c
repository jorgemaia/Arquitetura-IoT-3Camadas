/* esp-azure example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_event.h"
#include "esp_http_client.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "esp_sntp.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "ping/ping_sock.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "iothub_client_sample_mqtt.h"

#include "libGSM.h"
#include "simCom.h"
#include <time.h>

static const char *TAG = "sim_example";

#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD
#define	PIN_PHY_POWER	12

#define AZURE_CORE  0

SemaphoreHandle_t simSemaphore = NULL;
SemaphoreHandle_t gpsSemaphore = NULL;
SemaphoreHandle_t tStmpSemaphore = NULL;
SemaphoreHandle_t spiffsSemaphore = NULL;
SemaphoreHandle_t SDSemaphore = NULL;

double lat=0, lon=0;
struct tm tStmp;

// Variaveis globais para o status da internet
static uint8_t ContadorDeTimeouts =0;
static uint8_t flagConexaoInternetAtiva;// em 0 a conexao esta ativa, em 1 a conexao caiu
static uint8_t flagConexaoTimeout =0;



// funcoes para gerenciar o ping
void test_on_ping_success(esp_ping_handle_t hdl, void *args)
{
    /* Invocada quando recebe uma respostas */
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    printf("%d bytes from %s icmp_seq=%d ttl=%d time=%d ms\n",
           recv_len, inet_ntoa(target_addr.u_addr.ip4), seqno, ttl, elapsed_time);
    // se ok ligar led, se nao desligar
    //acendeLed();
    ESP_LOGI("PING", "ping success");
    ContadorDeTimeouts = 0; // conexao esta ok, zera o contador de timeout
    flagConexaoInternetAtiva = 0; // coloca a flag para a conexao estar ok
    flagConexaoTimeout = 0;
}
void test_on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    /* Invocada quando recebe o reply de packet timeout */
    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    printf("From %s icmp_seq=%d timeout\n", inet_ntoa(target_addr.u_addr.ip4), seqno);
    //apagaLed(); // conexao deu timeout
     ESP_LOGI("PING", "TIMEOUT");
    ContadorDeTimeouts++; // acumula um timeout
    if( ContadorDeTimeouts == 5 ) {
        flagConexaoInternetAtiva = 1; //conexao dropou
    }
    flagConexaoTimeout = 1;
    
}
void test_on_ping_end(esp_ping_handle_t hdl, void *args)
{
    /* Invocada quando uma sessao de ping termina */
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    printf("%d packets transmitted, %d received, time %dms\n", transmitted, received, total_time_ms);
    esp_ping_stop(hdl); // o ping deve ser parado para poder deletar
    esp_ping_delete_session(hdl); // importante deletar a sessao
}
void initialize_ping()
{
    /* convert URL to IP address */
    ip_addr_t target_addr;
    struct addrinfo hint;
    struct addrinfo *res = NULL;
    memset(&hint, 0, sizeof(hint));
    memset(&target_addr, 0, sizeof(target_addr));
    getaddrinfo("8.8.8.8", NULL, &hint, &res);
    struct in_addr addr4 = ((struct sockaddr_in *) (res->ai_addr))->sin_addr;
    inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    freeaddrinfo(res);
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;          // target IP address
    ping_config.count = 1;    // ping in infinite mode, esp_ping_stop can stop it
    /* set callback functions */
    esp_ping_callbacks_t cbs;
    cbs.on_ping_success = test_on_ping_success;
    cbs.on_ping_timeout = test_on_ping_timeout;
    cbs.on_ping_end = test_on_ping_end;
    
    esp_ping_handle_t ping;
    esp_ping_new_session(&ping_config, &cbs, &ping);
    esp_ping_start(ping);
}

void azure_task(void *pvParameter)
{
    // xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
    //                     false, true, portMAX_DELAY);
    
    ESP_LOGI(TAG, "Connected to AP success!");

    iothub_client_sample_mqtt_run();

    vTaskDelete(NULL);
}

void createSemaphores(void){

    spiffsSemaphore = xSemaphoreCreateBinary();
    if(spiffsSemaphore == NULL) {
        ESP_LOGI("SEMPAHORE CREATION", "spiffs semaphore create failed");
        writeLog("spiffs semaphore create failed");
    }
    else ESP_LOGI("SEMPAHORE CREATION", "spiffs semaphore create successed");
    xSemaphoreGive(spiffsSemaphore);

    SDSemaphore = xSemaphoreCreateBinary();
    if(SDSemaphore == NULL){ 
        ESP_LOGI("SEMPAHORE CREATION", "SD semaphore create failed");
        writeLog("SD semaphore create failed");
    }
    else ESP_LOGI("SEMPAHORE CREATION", "SD semaphore create successed");
    xSemaphoreGive(SDSemaphore);

    simSemaphore = xSemaphoreCreateBinary();
    if(simSemaphore == NULL){ 
        ESP_LOGI("SEMPAHORE CREATION", "GSM semaphore create failed");
        writeLog("GSM semaphore create failed");
    }
    else ESP_LOGI("SEMPAHORE CREATION", "GSM semaphore create successed");
    xSemaphoreGive(simSemaphore);

    gpsSemaphore = xSemaphoreCreateBinary();
    if(gpsSemaphore == NULL){ 
        ESP_LOGI("SEMPAHORE CREATION", "GPS semaphore create failed");
        writeLog("GPS semaphore create failed");
    }
    else ESP_LOGI("SEMPAHORE CREATION", "GPS semaphore create successed");
    xSemaphoreGive(gpsSemaphore);

    tStmpSemaphore = xSemaphoreCreateBinary();
    if(tStmpSemaphore == NULL){ 
        ESP_LOGI("SEMPAHORE CREATION", "tSTMP semaphore create failed");
        writeLog("tSTMP semaphore create failed");
    }
    else ESP_LOGI("SEMPAHORE CREATION", "tSTMP semaphore create successed");
    xSemaphoreGive(tStmpSemaphore);
}

void app_main()
{
    const TickType_t wait = 60000 / portTICK_PERIOD_MS;
    //const char* RS_ZERO = "RS485_0";
    //const char* RS_ONE = "RS485_1";
    createSemaphores();
    //initInternMem(); //log.h
    //initSDCard(); //log.h

    initSim(); //simCom.h

    // uint8_t a = 0;
    // uint8_t b = 1;
    while(ppposInit() != 1);
    //if(ppposInit() == 1) {
    //ppposDisconnect(0, 0);// when using GPS
    writeLog("PPP Task Initialized.");
    //}
    //configLED();

    //setenv("TZ","GMT+3",1);
    //tzset();

    //initialize_ping();
    //display_buff = (struct display *)malloc(sizeof(struct display));
    //display_queue = xQueueCreate( 10, sizeof( struct display ) );
    //if(display_queue == NULL){
    //    ESP_LOGE("SCR", "Error creating the display queue");
    //}
    //i2c_master_init(); //IoT_Hub_MQTT.h
    //SSD1306_Init();
    //xTaskCreatePinnedToCore(screenTask, "screen", SCREEN_STACK_SIZE, NULL, SCREEN_TASK_PRIO, NULL, AZURE_CORE);
    //screen_msg( NULL, "   System" , "Initialized.");

    //each task watches a different uart port (UART0 and UART1)
    //xTaskCreatePinnedToCore(rsTask, RS_ZERO, UART_TASK_STACK_SIZE, NULL, UART_TASK_PRIO, NULL, LOCAL_CORE);
    //xTaskCreatePinnedToCore(rsTaskOne, RS_ONE, UART_TASK_STACK_SIZE, NULL, UART_TASK_PRIO, NULL, LOCAL_CORE);
    //sim tasks
    //xTaskCreatePinnedToCore(gpsTask, "getGPS_task", GPS_TASK_STACK_SIZE, NULL, GPS_TASK_PRIO, NULL, AZURE_CORE);
    // xTaskCreatePinnedToCore(clockTask, "clock_task", CLOCK_TASK_STACK_SIZE, NULL, CLOCK_TASK_PRIO, NULL, AZURE_CORE);
    //payload shift task
    //xTaskCreatePinnedToCore(writeToQueue, "payload", CLOCK_TASK_STACK_SIZE, NULL, CLOCK_TASK_PRIO, NULL, AZURE_CORE);
    //xTaskCreatePinnedToCore(SDTask, "SD_COPY", CLOCK_TASK_STACK_SIZE, NULL, CLOCK_TASK_PRIO, NULL, AZURE_CORE);
    
    
    //vTaskDelay(wait);
    //printf("FIM DELAY\r\n");

    if ( xTaskCreate(&azure_task, "azure_task", 1024 * 5, NULL, 10, NULL) != pdPASS ) {
        printf("create azure task failed\r\n");
    }


}
