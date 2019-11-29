#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_event_loop.h"
#include "esp_log.h"

#define  DEBUG_UDP                         "UDP_DEBUG"
#define LEN_RX_BUFFER_UDP                  2048
#define LEN_BUFFER_IP_ADDRESS              128


#define TIME_SEND_UDP_DEBUG        10000

#define UDP_PORT                   50001
#define CONFIG_EXAMPLE_IPV4

#define CMD_CHANGE_PASSWORD_SYSTEM  135
#define CMD_SETTING_AP_MODE         133
#define COMMAND_SCAN_WIFI           130
#define COMMAND_SET_WIFI            126
#define CONFIRM_WIFI_INFOR          11
#define CONFIRM_IP                  122
#define SEND_BUFFER_ADC             102
#define STOP_SEND_BUFFER_ADC        999
#define SYNC_DEVICE_INFOR           12
#define GENERATOR_INFOR             123
#define REQUEST_CONNECT             101
#define REQUEST_VOLTAGE             100
#define REQUEST_CURRENT             200
#define CONFIRM_CURRENT_INFOR       202
#define REQUEST_RESISTOR            131
#define CONFIRM_RESISTOR_INFOR      132
#define REQUEST_ERRASE_EEPROM       12 
#define REQUEST_NUM_OF_RESET        13
#define REQUEST_DEFAULT             0
#define REQUEST_WIFI_LIST           3
#define RESPONSE_SSID_PASS          4
#define REQUEST_GENERATOR           123



 esp_err_t event_handler(void *ctx, system_event_t *event);

 void udp_server_task(void *pvParameters);
 void process_data_udp_task(void *pvParameters);
 void parser_json_udp();
 void processRequest(uint16_t cmd) ;
 void send_udp_packet(void *pvParameters);
 void send_udp_to_IP(char* data, size_t len,char* ip_addr);