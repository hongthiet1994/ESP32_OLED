#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "udp_dmm.h"
#include "wifi_dmm.h"
#include "http_server_dmm.h"
#include "eeprom_dmm.h"
#include "uart_dmm.h"
#include "ssd1306_oled.h"


char *ip4;

uint32_t ui32_WiFi_status = WIFI_DISCONNECT;

WIFI_INFOR    WiFi_Infor;
wifi_config_t wifi_config_sta;
wifi_config_t wifi_config_ap;

uint32_t ui32_counter_disconnected = 0;
/* FreeRTOS event group to signal when we are connected & ready to make a request */
 EventGroupHandle_t wifi_event_group;
//static const char *WIFI_DEBUG = "WiFi";
int CONNECTED_BIT = BIT0;
// FOR STRING LIST WIFI 
char buffer_wifi_list[LEN_BUFFER_WIFI_LIST] = {0};

uint32_t ui32_deleted_task_reconnect = false;

TaskHandle_t xTask_Check_reconnect = NULL;

uint32_t ui32_Send_WiFi_status_to_STM = false;

esp_err_t event_handler(void *ctx, system_event_t *event)
{   
    wifi_mode_t wifi_mode;
    switch (event->event_id) 
    {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(WIFI_DEBUG, "SYSTEM_EVENT_STA_START");
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(WIFI_DEBUG, "SYSTEM_EVENT_STA_CONNECTED");
            tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
            
            ui32_counter_disconnected = 0;
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(WIFI_DEBUG, "SYSTEM_EVENT_STA_GOT_IP");
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);      
                ip4 = ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip);
                ESP_LOGI(WIFI_DEBUG, "IP: %s",	ip4);
            display_string_debug(ip4,15,2);
            ui32_WiFi_status = WIFI_CONNECTED;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
            auto-reassociate. */
            ui32_counter_disconnected++;              
            ESP_LOGI(WIFI_DEBUG, "SYSTEM_EVENT_STA_DISCONNECTED : %d",ui32_counter_disconnected);   
            display_value_debug("Connecting..",12,ui32_counter_disconnected,2);
            //display_string_debug("Connecting..",12,2); 
            esp_wifi_connect();
            ui32_WiFi_status = WIFI_DISCONNECT;
            //xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            esp_wifi_get_mode(&wifi_mode);                                  
            break;
        case SYSTEM_EVENT_AP_STA_GOT_IP6:
            ESP_LOGI(WIFI_DEBUG, "SYSTEM_EVENT_STA_GOT_IP6");
            xEventGroupSetBits(wifi_event_group, IPV6_GOTIP_BIT);            
            char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
            ESP_LOGI(WIFI_DEBUG, "IPv6: %s", ip6);
            break;
        case SYSTEM_EVENT_AP_START:
            ESP_LOGI(WIFI_DEBUG, "AP START");
            display_string_debug("AP Mode",7,2);
            ui32_WiFi_status = WIFI_DISCONNECT; 
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(WIFI_DEBUG, "SYSTEM_EVENT_AP_STACONNECTED");               
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:             
            ESP_LOGI(WIFI_DEBUG, "SYSTEM_EVENT_AP_STADISCONNECTED");  
            xTaskCreatePinnedToCore(check_WiFi_reconnect_task, "check_WiFi_reconnect_task", 4096, NULL, 4, &xTask_Check_reconnect,1);          
            ui32_deleted_task_reconnect = false;
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            ui32_Send_WiFi_status_to_STM = false;
            break;
        default:
            break;
    }
    return ESP_OK;
}


void check_WiFi_reconnect_task(void *pvParameters)
{  
    wifi_mode_t  wifi_mode;
    wifi_sta_list_t wifi_sta_list;
    wifi_ap_record_t wifi_ap_record;
    for(;;)
    { 
        esp_wifi_get_mode(&wifi_mode);
        if(wifi_mode == WIFI_MODE_APSTA)
        {           
            esp_wifi_ap_get_sta_list(&wifi_sta_list);  
            ESP_LOGI(WIFI_DEBUG, "wifi_sta_list: %d",wifi_sta_list.num);           
            if(wifi_sta_list.num <=0)
            {
                if(esp_wifi_sta_get_ap_info(&wifi_ap_record) == ESP_OK)
                {                   
                    
                    esp_wifi_set_mode(WIFI_MODE_STA);   
                    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta);   
                    esp_wifi_start();
                    vTaskDelete(xTask_Check_reconnect);
                }
                else
                {                    
                    esp_wifi_connect();
                    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);        
                    xEventGroupClearBits(wifi_event_group, IPV4_GOTIP_BIT);
                    xEventGroupClearBits(wifi_event_group, IPV6_GOTIP_BIT);                    
                }                
            }
            else
            {              
                                                
            }
            uart_write_bytes(UART_STM32,COMMAND_STOP_SEND_DATA_GRAPH,LEN_COMMAND_STOP_SEND_DATA_GRAPH);            
        }
        ESP_LOGI(WIFI_DEBUG, "CHECK WIFI RECONNET"); 
        vTaskDelay(10000/ portTICK_PERIOD_MS);   //10s
    }
    vTaskDelete(NULL);
}

void initialise_WiFi(uint8_t mode)
{       
    wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    esp_event_loop_init(event_handler, NULL);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);     
    setting_WiFi(mode);
    
    /*
    switch (mode)
    {
        case WIFI_MODE_STA:
            ESP_LOGI(WIFI_DEBUG, "STA MODE");     
            memset(wifi_config_sta.sta.ssid,0,LEN_BUFFER_SSID);
            memset(wifi_config_sta.sta.password,0,LEN_BUFFER_PASS);
            nvs_get_str_dmm(FIELD_SSID_STA,(char*)wifi_config_sta.sta.ssid,&len);
            nvs_get_str_dmm(FIELD_PASSWORD_STA,(char*)wifi_config_sta.sta.password,&len);                          
            ESP_LOGI(WIFI_DEBUG, "STA: SSID %s...", wifi_config_sta.sta.ssid);
            ESP_LOGI(WIFI_DEBUG, "STA: PASSWORD %s...", wifi_config_sta.sta.password);      
            esp_wifi_set_mode(WIFI_MODE_STA);   
            esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta);   
            esp_wifi_start();
            break;
        case WIFI_MODE_AP:
            ESP_LOGI(WIFI_DEBUG, "AP MODE");   
            memset(wifi_config_ap.ap.ssid,0,LEN_BUFFER_SSID);
            memset(wifi_config_ap.ap.password,0,LEN_BUFFER_PASS);
            nvs_get_str_dmm(FIELD_SSID_AP,(char*)wifi_config_ap.ap.ssid,&len);
            nvs_get_str_dmm(FIELD_PASSWORD_AP,(char*)wifi_config_ap.ap.password,&len);          
            ESP_LOGI(WIFI_DEBUG, "AP MODE : SSID %s...", wifi_config_ap.ap.ssid);
            ESP_LOGI(WIFI_DEBUG, "AP MODE : PASSWORD %s...", wifi_config_ap.ap.password);     
            wifi_config_ap.ap.max_connection = MAX_CONNECTION;
            wifi_config_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
            wifi_config_ap.ap.channel = 6;              
            esp_wifi_set_mode(WIFI_MODE_APSTA);   
            esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap);   
            esp_wifi_start();   
        case WIFI_MODE_APSTA:            
            break;        
        default:
            break;
    }
    */
    
    
}

void setting_WiFi(uint8_t mode)
{    
    size_t len;
    switch (mode)
    {
        case WIFI_MODE_STA:
            ESP_LOGI(WIFI_DEBUG, "STA MODE");     
            memset(wifi_config_sta.sta.ssid,0,LEN_BUFFER_SSID);
            memset(wifi_config_sta.sta.password,0,LEN_BUFFER_PASS);
            nvs_get_str_dmm(FIELD_SSID_STA,(char*)wifi_config_sta.sta.ssid,&len);
            nvs_get_str_dmm(FIELD_PASSWORD_STA,(char*)wifi_config_sta.sta.password,&len);    
            ESP_LOGI(WIFI_DEBUG, "STA: SSID %s...", wifi_config_sta.sta.ssid);
            ESP_LOGI(WIFI_DEBUG, "STA: PASSWORD %s...", wifi_config_sta.sta.password);      
            esp_wifi_stop();
            esp_wifi_set_mode(WIFI_MODE_STA);   
            esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta);   
            esp_wifi_start();
            break;
        case WIFI_MODE_AP:
            esp_wifi_restore();
            ESP_LOGI(WIFI_DEBUG, "AP MODE");   
            memset(wifi_config_ap.ap.ssid,0,LEN_BUFFER_SSID);
            memset(wifi_config_ap.ap.password,0,LEN_BUFFER_PASS);
            nvs_get_str_dmm(FIELD_SSID_AP,(char*)wifi_config_ap.ap.ssid,&len);
            nvs_get_str_dmm(FIELD_PASSWORD_AP,(char*)wifi_config_ap.ap.password,&len);             
            ESP_LOGI(WIFI_DEBUG, "AP: SSID %s...", wifi_config_ap.ap.ssid);
            ESP_LOGI(WIFI_DEBUG, "AP: PASSWORD %s...", wifi_config_ap.ap.password);      
            esp_wifi_stop(); 
            wifi_config_ap.ap.max_connection = MAX_CONNECTION;
            wifi_config_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
            wifi_config_ap.ap.channel = 6;                
            esp_wifi_set_mode(WIFI_MODE_AP);   
            esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap);   
            esp_wifi_start();
            break;
        case WIFI_MODE_APSTA:
            //esp_wifi_restore();
            ESP_LOGI(WIFI_DEBUG, "APSTA MODE");   
            memset(wifi_config_ap.ap.ssid,0,LEN_BUFFER_SSID);
            memset(wifi_config_ap.ap.password,0,LEN_BUFFER_PASS);
            memset(wifi_config_sta.sta.ssid,0,LEN_BUFFER_SSID);
            memset(wifi_config_sta.sta.password,0,LEN_BUFFER_PASS);
            nvs_get_str_dmm(FIELD_SSID_STA,(char*)wifi_config_sta.sta.ssid,&len);
            nvs_get_str_dmm(FIELD_PASSWORD_STA,(char*)wifi_config_sta.sta.password,&len);
            nvs_get_str_dmm(FIELD_SSID_AP,(char*)wifi_config_ap.ap.ssid,&len);
            nvs_get_str_dmm(FIELD_PASSWORD_AP,(char*)wifi_config_ap.ap.password,&len);              
            ESP_LOGI(WIFI_DEBUG, "AP: SSID %s...", wifi_config_ap.ap.ssid);
            ESP_LOGI(WIFI_DEBUG, "AP: PASSWORD %s...", wifi_config_ap.ap.password);   
            ESP_LOGI(WIFI_DEBUG, "STA: SSID %s...", wifi_config_sta.ap.ssid);
            ESP_LOGI(WIFI_DEBUG, "STA: PASSWORD %s...", wifi_config_sta.ap.password);     
            esp_wifi_stop();
            wifi_config_ap.ap.max_connection = MAX_CONNECTION;
            wifi_config_ap.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
            wifi_config_ap.ap.channel = 6;     
            esp_wifi_set_mode(WIFI_MODE_APSTA);   
            esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap);   
            esp_wifi_start();
            break; 
        default:
            break;
    }   
}

void wait_for_ip()
{
    uint32_t bits = IPV4_GOTIP_BIT | IPV6_GOTIP_BIT ;
    ESP_LOGI(WIFI_DEBUG, "Waiting for AP connection...");
    xEventGroupWaitBits(wifi_event_group, bits, false, true, portMAX_DELAY);
    ESP_LOGI(WIFI_DEBUG, "Connected to AP");
}



/* Initialize Wi-Fi as sta and set scan method */
void wifi_scan(void)
{
    uint16_t ui16_len_total = 0;
    uint16_t ui16_len_temp = 0;
    uint16_t sta_number = 0;
    uint8_t i;
    wifi_mode_t mode = 0;
    wifi_scan_config_t scan_config = {0};  
    wifi_ap_record_t *ap_list_buffer;
    ESP_LOGI(WIFI_DEBUG, "Scaning WiFi...........");    
    esp_wifi_get_mode(&mode);
    if(mode == WIFI_MODE_AP)
    {     
        ESP_LOGI(WIFI_DEBUG, "MODE AP  - > MODE APSTA");  
        esp_wifi_set_mode(WIFI_MODE_APSTA);           
        //esp_wifi_start();     
    }        
    free_Buffer_WiFi_list();     
    if(esp_wifi_scan_start(&scan_config, true) == ESP_OK)   
    {
        
        esp_wifi_scan_get_ap_num(&sta_number);
        ESP_LOGI(WIFI_DEBUG, "sta_number : %d",sta_number);
        ap_list_buffer = malloc(sta_number * sizeof(wifi_ap_record_t));
        if (ap_list_buffer == NULL) 
        {
            ESP_LOGI(WIFI_DEBUG, "Failed to malloc buffer to print scan results");
            //return;
        }
        if (esp_wifi_scan_get_ap_records(&sta_number,(wifi_ap_record_t *)ap_list_buffer) == ESP_OK) 
        {
            if(sta_number > MAX_NUMBER_WIFI)
            {
                sta_number = MAX_NUMBER_WIFI;
            }
            for(i=0; i<sta_number; i++) 
            {             
                ui16_len_total = strlen(buffer_wifi_list);
                ui16_len_temp = strlen((char*)ap_list_buffer[i].ssid);             
                memcpy(&buffer_wifi_list[ui16_len_total],ap_list_buffer[i].ssid,ui16_len_temp);
                memcpy(&buffer_wifi_list[ui16_len_total+ui16_len_temp],",",1);            
            }
            ESP_LOGI(WIFI_DEBUG, "[%s]", buffer_wifi_list);
        }
        free(ap_list_buffer);   
    }
    else
    {
        ESP_LOGI(WIFI_DEBUG, "ERROR SCAN WIFI");
    } 
    if(mode == WIFI_MODE_AP)
    {   
        ESP_LOGI(WIFI_DEBUG, "APSTA - > AP");
        esp_wifi_set_mode(WIFI_MODE_AP);        
    }
        
}

void freeWiFi_Infor()
{
    memset(WiFi_Infor.Buffer_WiFi_Infor,0,LEN_BUFFER_WIFI_INFOR);
}
void free_Buffer_WiFi_list()
{
    memset(buffer_wifi_list,0,LEN_BUFFER_WIFI_LIST);
}


void config_WiFi_infor_default()
{
    size_t len;
    memset(wifi_config_sta.sta.ssid,0,LEN_BUFFER_SSID);
    memset(wifi_config_sta.sta.password,0,LEN_BUFFER_PASS);
    memset(wifi_config_ap.ap.ssid,0,LEN_BUFFER_SSID);
    memset(wifi_config_ap.ap.password,0,LEN_BUFFER_PASS);
    ESP_LOGI(WIFI_DEBUG," START CONFIG WIFI INFOR DEFAULT");
    if(nvs_get_str_dmm(FIELD_SSID_STA,(char*)wifi_config_sta.sta.ssid,&len) == ESP_ERR_NVS_NOT_FOUND)
    {               
        ESP_LOGI(WIFI_DEBUG,"SSID ERROR : SETTING SSID STA DEFAULT");            
        nvs_set_str_dmm(FIELD_SSID_STA, WIFI_SSID_STA_DEFAULT);
            
    }
    if(nvs_get_str_dmm(FIELD_PASSWORD_STA,(char*)wifi_config_sta.sta.password,&len) == ESP_ERR_NVS_NOT_FOUND)
    {               
        ESP_LOGI(WIFI_DEBUG,"PASSWORD ERROR : SETTING PASSWORD STA DEFAULT");  
        nvs_set_str_dmm(FIELD_PASSWORD_STA, WIFI_PASS_STA_DEFAULT);
       
    }
    if(nvs_get_str_dmm(FIELD_SSID_AP,(char*)wifi_config_ap.ap.ssid,&len) == ESP_ERR_NVS_NOT_FOUND)
    {               
        ESP_LOGI(WIFI_DEBUG,"SSID ERROR : SETTING SSID AP DEFAULT");  
        nvs_set_str_dmm(FIELD_SSID_AP, WIFI_SSID_AP_DEFAULT);
          
    }   
    if(nvs_get_str_dmm(FIELD_PASSWORD_AP,(char*)wifi_config_ap.ap.password,&len) == ESP_ERR_NVS_NOT_FOUND)
    {               
        ESP_LOGI(WIFI_DEBUG,"SSID ERROR : SETTING PASS AP DEFAULT");  
        nvs_set_str_dmm(FIELD_PASSWORD_AP, WIFI_PASS_AP_DEFAULT);
              
    } 
    ESP_LOGI(WIFI_DEBUG," END CONFIG WIFI INFOR DEFAULT");
}