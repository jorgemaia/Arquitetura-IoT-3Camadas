#pragma once

#include <time.h>
//#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "tcpip_adapter.h"
#include "netif/ppp/pppos.h"
#include "netif/ppp/ppp.h"
#include "netif/ppp/pppapi.h"

#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"


#include "lwip/apps/sntp.h"
#include "lwip/err.h"

#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"

#include "uart.h"
#include "log.h"

#define RESET_PIN                   (5) //4
#define GPIO_OUTPUT_PIN_SIMCOM      (1ULL<<RESET_PIN)

#define GPS_TASK_STACK_SIZE         (1024*5)
#define GPS_TASK_PRIO               (2)
#define CLOCK_TASK_STACK_SIZE       (1024*5)
#define CLOCK_TASK_PRIO             (2)

#define CLOCK_UPDATE_PERIOD         (5*60000 / portTICK_RATE_MS)

extern double lat, lon;
extern struct tm tStmp;
extern SemaphoreHandle_t simSemaphore;
extern SemaphoreHandle_t gpsSemaphore;
extern SemaphoreHandle_t tStmpSemaphore;

void initSim(void);
void rstBoard(void);
void uartFlush(int uartNum);
void atTest(void);
void configSimClock(void);
void updateClock(void);
bool startGPS(void);
void setGPIO(void);
void atHandler(char* msg, char* response);
void simCom_getGPS(void);
void gpsTask(void *arg);
bool isGPSOn(void);
void clockTask(void *args);
void getGSMLOC(void);