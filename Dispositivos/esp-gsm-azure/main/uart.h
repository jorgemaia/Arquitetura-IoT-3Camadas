#pragma once

#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

// Note: UART2 default pins IO16, IO17 do not work on ESP32-WROVER module 
// because these pins connected to PSRAM
#define UART0_TX    (19) //2
#define UART0_RX    (18) //4
// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define UART0_RTS   UART_PIN_NO_CHANGE
// CTS is not used in RS485 Half-Duplex Mode
#define UART0_CTS   UART_PIN_NO_CHANGE

//UART1
#define UART1_TX    (4) //19
#define UART1_RX    (34) //18
// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define UART1_RTS   UART_PIN_NO_CHANGE
// CTS is not used in RS485 Half-Duplex Mode
#define UART1_CTS   UART_PIN_NO_CHANGE

//UART2
#define UART2_TX    (17)
#define UART2_RX    (16)
// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define UART2_RTS   UART_PIN_NO_CHANGE
// CTS is not used in RS485 Half-Duplex Mode
#define UART2_CTS   UART_PIN_NO_CHANGE

#define UART_BUF_SIZE   (256)
#define BAUD_RATE       (115200)

// Read packet timeout
#define PACKET_READ_TICS        (100 / portTICK_RATE_MS)
#define UART_TASK_STACK_SIZE    (1024*10)
#define UART_TASK_PRIO          (1)
#define UART0_PORT              (UART_NUM_0)
#define UART1_PORT              (UART_NUM_1)
#define UART2_PORT              (UART_NUM_2)

void initUart(uint8_t uartNum, bool rs485);
int uartSendData(uart_port_t uartNum, char* data);
int uartReceiveData(uart_port_t uartNum, char* data);
void uartFlush(int uartNum);