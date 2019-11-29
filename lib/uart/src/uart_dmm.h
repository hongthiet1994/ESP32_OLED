#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "driver/uart.h"

#define UART_DEBUG "UART_DEBUG"
#define BAUD_RATE_UART_STM       1843200


#define ECHO_TEST_TXD_2  (GPIO_NUM_17)
#define ECHO_TEST_RXD_2  (GPIO_NUM_16)
#define ECHO_TEST_TXD_0  (GPIO_NUM_1)
#define ECHO_TEST_RXD_0  (GPIO_NUM_3)

#define ECHO_TEST_RTS  (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS  (UART_PIN_NO_CHANGE)


#define BUF_SIZE                 (1536)
#define LEN_BUFFER_RX_UART        1536

#define QUEUE_SIZE               10240 
#define TIME_OUT_RECEIVE_UART   3
#define EX_UART_NUM   UART_NUM_0
#define UART_STM32    UART_NUM_0

#define UART_DEBUG_COM    UART_NUM_2

#define PATTERN_CHR_NUM      3         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define RD_BUF_SIZE       33280  //(BUF_SIZE)



#define   COMMAND_REQUEST_AP_MODE_ESP32         1
#define   COMMAND_SYNC_DEVICE_INFOR_REP         2
#define   COMMAND_SYNC_DEVICE_INFOR_SEND        4

#define   COMMAND_CONFIRM_WIFI_STATUS           3


#define   COMMAND_STOP_SEND_DATA_GRAPH          "{\"CMD\":999,\"STATUS\":0,\"CRC\":54209}"
#define   LEN_COMMAND_STOP_SEND_DATA_GRAPH      34
void config_uart_0();
void config_uart_2();
void config_uart_0_queue();
void free_buffer_RX_uart();
void free_buffer_RX_uart_backup();
void uart_event_task(void *pvParameters);
void process_data_uart_task(void *pvParameters);
void check_time_out_receive_uart();
void proccess_Command_STM(uint8_t command);