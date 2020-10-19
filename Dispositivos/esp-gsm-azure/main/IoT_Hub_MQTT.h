// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef IOTHUB_GSM_MQTT_H
#define IOTHUB_GSM_MQTT_H

#define SCREEN_STACK_SIZE 	(2*1024)
#define SCREEN_TASK_PRIO 	(2)
#ifdef __cplusplus
extern "C" {
#endif

	struct display{
		char top[16];
		char tag[16];
		char buffer[16];
	};

	extern int flag_pay;
	extern char lat_main[12];
	extern char lon_main[12];
	extern char* connectionString;
	// extern QueueHandle_t display_queue;

	void i2c_master_init();
	void screen_msg(const char *top, const char* tag, const char* buffer);
	// void screenTask( void * parameter );
	void iothub_client_esp32_run();
	int selectID(uint8_t* sensor_id);
	int compareIDs (int sensor_id);
	int new_tireSelect(int tire_ID, uint8_t *Payload);
	int transf_Payload(uint8_t *Payload_T, uint8_t *Payload);
	int comparePayload(uint8_t *Payload_T, uint8_t *Payload);
	int compareTime(struct tm time_T, struct tm time_O);
	int compareMinute(struct tm time_T, struct tm time_O);
	//int intervTime(struct tm time_T, struct tm time_O, int interv);
	//uint8_t* sendError(uint8_t num);
	//void sensorIdle(uint8_t *Payload);
	void sendAzure(double lat, double lon);
	int compareCases(int sensor_int, uint8_t *Payload);
	int send_Payload(uint8_t* Payload, double lat, double lon);
	int char2int (uint8_t *array, int n);


#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_CLIENT_SAMPLE_MQTT_H */

