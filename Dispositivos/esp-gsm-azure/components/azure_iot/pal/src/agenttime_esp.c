// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "azure_c_shared_utility/agenttime.h"
#include "azure_c_shared_utility/xlogging.h"

#include "lwip/apps/sntp.h"

#include "esp_log.h"
#include "libGSM.h"

static const char *TIME_TAG = "[SNTP]";

void initialize_sntp(void)
{
    printf("Initializing SNTP\n");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
 //    sntp_setservername(1, "202.112.29.82");        // set sntp server after got ip address, you had better to adjust the sntp server to your area
    sntp_init();
}

static void obtain_time(void)
{
	// ==== Get time from NTP server =====
	time_t now = 0;
	struct tm timeinfo = { 0 };
	int retry = 0;
	const int retry_count = 10;

	time(&now);
	localtime_r(&now, &timeinfo);

	while (1) {
		printf("\r\n");
		ESP_LOGI(TIME_TAG,"OBTAINING TIME");
	    ESP_LOGI(TIME_TAG, "Initializing SNTP");
	    sntp_setoperatingmode(SNTP_OPMODE_POLL);
	    sntp_setservername(0, "pool.ntp.org");
	    sntp_init();
		ESP_LOGI(TIME_TAG,"SNTP INITIALIZED");

		// wait for time to be set
		now = 0;
		while ((timeinfo.tm_year < (2016 - 1900)) && (++retry < retry_count)) {
			ESP_LOGI(TIME_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			time(&now);
			localtime_r(&now, &timeinfo);
			if (ppposStatus() != GSM_STATE_CONNECTED) break;
		}
		if (ppposStatus() != GSM_STATE_CONNECTED) {
			sntp_stop();
			ESP_LOGE(TIME_TAG, "Disconnected, waiting for reconnect");
			retry = 0;
			while (ppposStatus() != GSM_STATE_CONNECTED) {
				vTaskDelay(100 / portTICK_RATE_MS);
			}
			continue;
		}

		if (retry < retry_count) {
			ESP_LOGI(TIME_TAG, "TIME SET TO %s", asctime(&timeinfo));
			break;
		}
		else {
			ESP_LOGI(TIME_TAG, "ERROR OBTAINING TIME\n");
		}
		sntp_stop();
		break;
	}
}

time_t sntp_get_current_timestamp()
{
    time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	// Is time set? If not, tm_year will be (1970 - 1900).
	if (timeinfo.tm_year < (2016 - 1900)) {
		printf("Time is not set yet. Connecting to WiFi and getting time over NTP. timeinfo.tm_year:%d\n",timeinfo.tm_year);
		obtain_time();
		// update 'now' variable with current time
		time(&now);
	}
	char strftime_buf[64];

	// Set timezone to Brazil Standard Time
	//setenv("TZ", "BRT3BRST,M10.2.0/0:00,M2.3.0/0:00", 1);
	//tzset();
	localtime_r(&now, &timeinfo);
	return now;

}

time_t get_time(time_t* currentTime)
{
    return sntp_get_current_timestamp();

}

double get_difftime(time_t stopTime, time_t startTime)
{	
    return (double)stopTime - (double)startTime;
}

struct tm* get_gmtime(time_t* currentTime)
{
    return NULL;
}

char* get_ctime(time_t* timeToGet)
{
    return NULL;
}
