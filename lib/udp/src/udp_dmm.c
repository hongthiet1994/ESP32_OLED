#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "driver/uart.h"
#include "lwip/sockets.h"
#include "udp_dmm.h"
#include "wifi_dmm.h"
#include "cJSON.h"
#include "json_dmm.h"
#include "uart_dmm.h"
#include "ssd1306_oled.h"
#include "eeprom_dmm.h"

extern uint32_t ui32_WiFi_status;
uint32_t ui32_num_send_udp = 0;
uint32_t ui32_num_receive_udp = 0;

extern TaskHandle_t xTask_UDP_server, xTask_UDP_process_data;

struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6
int sock;

uint32_t ui32_receive_cmd_stop = false;

extern EventGroupHandle_t wifi_event_group;

char rx_udp_buffer_backup[LEN_RX_BUFFER_UDP] = {0};
size_t len_buffer_udp_backup = 0;

void free_udp_buffer_backup()
{
    memset(rx_udp_buffer_backup, 0x00, LEN_RX_BUFFER_UDP);
}

void udp_server_task(void *pvParameters)
{
    char rx_buffer[LEN_RX_BUFFER_UDP];
    char addr_str[LEN_BUFFER_IP_ADDRESS];
    int addr_family;
    int ip_protocol;
    while (1)
    {
        #ifdef CONFIG_EXAMPLE_IPV4
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(UDP_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
        #else // IPV6
        struct sockaddr_in6 destAddr;
        bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
        destAddr.sin6_family = AF_INET6;
        destAddr.sin6_port = htons(PORT);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
        #endif
        sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(DEBUG_UDP, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(DEBUG_UDP, "Socket created");
        int err = bind(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0)
        {
            ESP_LOGE(DEBUG_UDP, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(DEBUG_UDP, "Socket binded");
        while (1)
        {
            //struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(sourceAddr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);
            // Error occured during receiving
            if (len < 0)
            {
                ESP_LOGI(DEBUG_UDP, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else
            {
                // Get the sender's ip address as string
                if (sourceAddr.sin6_family == PF_INET)
                {
                    inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                }
                else if (sourceAddr.sin6_family == PF_INET6)
                {
                    inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
                len_buffer_udp_backup = len;
                memcpy(rx_udp_buffer_backup, rx_buffer, len_buffer_udp_backup);
                xTaskNotifyGive(xTask_UDP_process_data);
                ESP_LOGI(DEBUG_UDP, "Received %d bytes from %s:", len, addr_str);
                //ESP_LOGI(DEBUG_UDP, "%s", rx_buffer);
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(DEBUG_UDP, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void process_data_udp_task(void *pvParameters)
{
    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        //ESP_LOGI(DEBUG_UDP, "PROCESS DATA UDP");
        parser_json_udp();
        free_udp_buffer_backup();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

struct sockaddr_in ip;

void send_udp_packet(void *pvParameters)
{
    memset((char *) &ip, 0, sizeof(ip));   
    ip.sin_family = PF_INET;
    inet_aton("192.168.4.1", &(ip.sin_addr));
    ip.sin_port = htons(UDP_PORT);
    for (;;)
    {
        if (ui32_WiFi_status == WIFI_CONNECTED)
        {
            ESP_LOGI(DEBUG_UDP, "SEND UDP PACKET");
            ui32_num_send_udp++;  
            nvs_set_number_dmm(FIELD_NUM_SEND,ui32_num_send_udp);   
            display_value_debug("Send: ",6,ui32_num_send_udp,4);       
            sendto(sock, "{\"CMD\":123,\"CRC\":12}",20, 0, (struct sockaddr*)&ip, sizeof(ip));
            //sendto(sock, "{\"CMD\":126,\"CRC\":12}",20, 0,"192.168.137.1", 13);

        }
        vTaskDelay(TIME_SEND_UDP_DEBUG / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void processRequest(uint16_t cmd)
{
    switch (cmd)
    {
    case COMMAND_SCAN_WIFI:
        jsonWiFilist();
        break;
    case COMMAND_SET_WIFI:
        parserJsonWiFiInfor();
        break;
    case CONFIRM_IP:
        confirmIP();
        break;
    case CMD_SETTING_AP_MODE:
        ESP_LOGI(DEBUG_UDP, "CMD_SETTING_AP_MODE");
        sendto(sock, "{\"CMD\":133}", 11, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
        setting_WiFi(WIFI_MODE_AP);
        break;
    case STOP_SEND_BUFFER_ADC:
        ui32_receive_cmd_stop = true;
        uart_write_bytes(UART_STM32, "{\"CMD\":999,\"CRC\":34091}", 23);
        break;
    case CMD_CHANGE_PASSWORD_SYSTEM:
        proccess_cmd_change_password_system();
        break;
    case 123:
        ESP_LOGI(DEBUG_UDP, "rev : 123");
        ui32_num_receive_udp++;
        ui32_num_send_udp++;  
        sendto(sock, "{\"CMD\":123,\"CRC\":12}", 20, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
        nvs_set_number_dmm(FIELD_NUM_REV,ui32_num_receive_udp);  
        display_value_debug("Rev: ",5,ui32_num_receive_udp,3);           
        nvs_set_number_dmm(FIELD_NUM_SEND,ui32_num_send_udp);   
        display_value_debug("Send: ",6,ui32_num_send_udp,4);        
        break;
    default:
        ESP_LOGI(DEBUG_UDP, "SEND TO ESP32");
        uart_write_bytes(UART_STM32, rx_udp_buffer_backup, strlen(rx_udp_buffer_backup));
        break;
    }
}
