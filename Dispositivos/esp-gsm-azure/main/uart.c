#include "uart.h"

void initUart(uint8_t uartNum, bool rs485){
    static const char* TAG = "UART-INIT";
    int32_t UART;
    int8_t uartTX;
    int8_t uartRX;
    int8_t uartRTS;
    int8_t uartCTS;
    //verifies wich uart to initialize
    if(uartNum == 0){
        UART = UART0_PORT;
        uartTX = UART0_TX;
        uartRX = UART0_RX;
        uartRTS = UART0_RTS;
        uartCTS = UART0_CTS;
    }
    else if(uartNum == 1){
        UART = UART1_PORT;
        uartTX = UART1_TX;
        uartRX = UART1_RX;
        uartRTS = UART1_RTS;
        uartCTS = UART1_CTS;
    }
    else{
        UART = UART2_PORT;
        uartTX = UART2_TX;
        uartRX = UART2_RX;
        uartRTS = UART2_RTS;
        uartCTS = UART2_CTS;
    }
    
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    
    // Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Uart%d start configure", uartNum);

    // Configure UART parameters
    uart_param_config(UART, &uart_config);
    
    ESP_LOGI(TAG, "UART%d set pins, mode and install driver.", uartNum);
    // Set UART1 pins(TX: IO23, RX: I022, RTS: IO18, CTS: IO19)
    uart_set_pin(UART, uartTX, uartRX, uartRTS, uartCTS);

    // Install UART driver (we don't need an event queue here)
    // In this example we don't even use a buffer for sending data.
    uart_driver_install(UART, UART_BUF_SIZE * 2, UART_BUF_SIZE * 2, 0, NULL, 0);

    // Set RS485 half duplex mode
    if(rs485) uart_set_mode(UART, UART_MODE_RS485_HALF_DUPLEX);
    else uart_set_mode(UART, UART_MODE_UART);
}

int uartSendData(uart_port_t uartNum, char* data){
    char *output = (char*) malloc(sizeof(char)*(strlen(data)+5));
    char *TAG = "sender";
    strcpy(output, data);
    strcat(output, "\r\n");
    uart_write_bytes(uartNum, (const char*)output, strlen(output));
    ESP_LOGI(TAG, "message sent = %s", data);
    uart_wait_tx_done(uartNum, 100 / portTICK_PERIOD_MS);
    free(output);
    return 0;
}

int uartReceiveData(uart_port_t uartNum, char* data){
    int length = 0, i=0;
    char *TAG = "receiver";
    while(length == 0 && i <10){
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_2, (size_t*)&length));
        uart_read_bytes(uartNum, (uint8_t*)data, length, 1000 / portTICK_PERIOD_MS);
        i++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    data[length] = '\0';
    ESP_LOGI(TAG, "received message = %s", data);
    return length;
}

void uartFlush(int uartNum){
    //discarta o conteudo do buffer do RX antes de enviar
    uart_flush(uartNum);
    //discarta o conteudo do buffer do TX antes de enviar
    uart_flush_input(uartNum);
}