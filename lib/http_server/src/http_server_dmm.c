/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "http_server_dmm.h"
#include "wifi_dmm.h"
#include "eeprom_dmm.h"
#include <math.h>
#include "ssd1306_oled.h"
#include "debug.h"

extern wifi_config_t wifi_config_sta;
extern wifi_config_t wifi_config_ap;


extern uint32_t ui32_num_send_udp;
extern uint32_t ui32_num_receive_udp;
extern int32_t restart_counter;

wifi_config_t wifi_config_web;

extern WIFI_INFOR    WiFi_Infor;
char web_setting_WiFi[LEN_BUFFER_WEB_OPEN] = {0};
extern char buffer_wifi_list[330];

uint32_t ui32_allow_upload_ota = false;
uint32_t ui32_check_get_setting_wifi = 0;

httpd_handle_t web_server = NULL;

static void __attribute__((noreturn)) task_fatal_error(void)
{
    ESP_LOGE(HTTP_DEBUG, "Exiting task due to fatal error...");
    (void)vTaskDelete(NULL);
    while (1) 
    {
        ;
    }
}


esp_err_t home_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/html");
    ESP_LOGI(HTTP_DEBUG,"Home page");
    ui32_num_send_udp = 0;
    ui32_num_receive_udp = 0;
    restart_counter = 0;
    nvs_set_number_dmm(FIELD_RESTART_COUNTER,0);  
    display_value_debug(KEY_NUM_OF_RESET,strlen(KEY_NUM_OF_RESET),restart_counter,1);
    nvs_set_number_dmm(FIELD_NUM_REV,0);  
    display_value_debug("Rev: ",5,0,3); 
    nvs_set_number_dmm(FIELD_NUM_SEND,0);   
    display_value_debug("Send: ",6,0,4);   
	httpd_resp_send(req, HOME_HTML, strlen(HOME_HTML));
	return ESP_OK;
}
esp_err_t scanwifi_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/html");
	//httpd_resp_send(req, SCAN_WIFI_HTML, strlen(SCAN_WIFI_HTML));    
	wifi_scan();
    snprintf(web_setting_WiFi,LEN_BUFFER_WEB_OPEN,HTML_WIFI_SETTING,wifi_config_ap.ap.ssid,buffer_wifi_list);
    httpd_resp_send(req, web_setting_WiFi, strlen(web_setting_WiFi));
    return ESP_OK;
}

esp_err_t scanwifi_handler_post(httpd_req_t *req)
{    
    size_t len_ssid = 0 , len_password = 0 , len_deviceid = 0;
    httpd_resp_set_type(req, "text/html");  
    len_ssid = httpd_req_get_hdr_value_len(req, "SSID") + 1;  
    len_password = httpd_req_get_hdr_value_len(req, "PASSWORD") + 1;
    len_deviceid = httpd_req_get_hdr_value_len(req, "DEVICEID") + 1;
    ESP_LOGI(WIFI_DEBUG, "len_ssid: %d ",len_ssid);
    ESP_LOGI(WIFI_DEBUG, "len_password: %d ",len_password);
    ESP_LOGI(WIFI_DEBUG, "len_deviceid: %d ",len_deviceid);
    memset(wifi_config_ap.ap.ssid,0,LEN_BUFFER_SSID);
    memset(wifi_config_ap.ap.password,0,LEN_BUFFER_PASS);
    memset(wifi_config_sta.sta.ssid,0,LEN_BUFFER_SSID);
    memset(wifi_config_sta.sta.password,0,LEN_BUFFER_PASS);  
    if((len_ssid >1) && (len_password>1) && (len_password<LEN_BUFFER_PASS))
    {
        httpd_req_get_hdr_value_str(req, "SSID",(char*)wifi_config_sta.sta.ssid, len_ssid);
        httpd_req_get_hdr_value_str(req, "PASSWORD",(char*)wifi_config_sta.sta.password, len_password); 
        httpd_req_get_hdr_value_str(req, "DEVICEID",(char*)wifi_config_ap.ap.ssid, len_deviceid); 

        httpd_resp_send(req, HTML_WIFI_CONNECTED, strlen(HTML_WIFI_CONNECTED));   
        ESP_LOGI(WIFI_DEBUG, "SSID: %s ",wifi_config_sta.sta.ssid);                
        ESP_LOGI(WIFI_DEBUG, "PASSWORD: %s ",wifi_config_sta.sta.password);          
        nvs_set_str_dmm(FIELD_SSID_STA,(char*) wifi_config_sta.sta.ssid);
        nvs_set_str_dmm(FIELD_PASSWORD_STA,(char*)  wifi_config_sta.sta.password);
        nvs_set_str_dmm(FIELD_SSID_AP,(char*)  wifi_config_ap.ap.ssid);
        esp_wifi_stop();         
        ESP_LOGI(WIFI_DEBUG, "Setting WiFi configuration SSID: %s...", wifi_config_sta.sta.ssid);   
        ESP_LOGI(WIFI_DEBUG, "Setting WiFi configuration PASS: %s...", wifi_config_sta.sta.password);   
        esp_wifi_set_mode(WIFI_MODE_STA);   
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config_sta) );    
        ESP_ERROR_CHECK( esp_wifi_start());
    }
    else
    {
        if((len_ssid == 1) && (len_password == 1) && (len_deviceid > 1))
        {
            httpd_req_get_hdr_value_str(req, "DEVICEID",(char*)wifi_config_ap.ap.ssid, len_deviceid); 
            nvs_set_str_dmm(FIELD_SSID_AP,(char*)  wifi_config_ap.ap.ssid);
            httpd_resp_send(req, HTML_CHANGE_DEVICEID_SUCCESSFULLY, strlen(HTML_CHANGE_DEVICEID_SUCCESSFULLY));
        }
        else
        {
            httpd_resp_send(req, HTML_WIFI_DISCONNETED, strlen(HTML_WIFI_DISCONNETED));            
        }       
           
    }    
    
    return ESP_OK;
}

esp_err_t upload_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, UPLOAD_HTML, strlen(UPLOAD_HTML));
    ESP_LOGI(HTTP_DEBUG,"started"); 
    ui32_allow_upload_ota = true;
	return ESP_OK;
}


int strln(char *ar)
{
   char *p = strstr(ar, "\r\n\r\n");
   if (p)
      return p - ar + 4;
   return 0;
}


esp_err_t upload_post_handler(httpd_req_t *req)
{
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    if(ui32_allow_upload_ota == false)
    {
        ESP_LOGI(HTTP_DEBUG, "Error auto upload");
        return 0;
    }
    ui32_allow_upload_ota = false;
    ESP_LOGI(HTTP_DEBUG, "Starting OTA example");
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) 
    {
        ESP_LOGW(HTTP_DEBUG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(HTTP_DEBUG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }

    ESP_LOGI(HTTP_DEBUG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(HTTP_DEBUG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    int binary_file_length = 0;
    /*deal with all receive packet*/
    bool image_header_was_checked = false;
    char buffer_data[BUFFER_SIZE];
    while (1) 
    {
        int data_read = httpd_req_recv(req, buffer_data, BUFFER_SIZE);
        if(data_read < 0) 
        {
            ESP_LOGE(HTTP_DEBUG, "Error: SSL data read error");
            ESP_LOGE(HTTP_DEBUG, "data read : %d",data_read);
            task_fatal_error();
        }
        else if (data_read > 0) 
        {
            if (image_header_was_checked == false) {
                    image_header_was_checked = true;
                    int h = strln(buffer_data);
                    int len = data_read - h;
                    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(HTTP_DEBUG, "OTA begin failed! (%s)", esp_err_to_name(err));
                        task_fatal_error();
                    }

                    ESP_LOGI(HTTP_DEBUG, "OTA begin upload");
                    err = esp_ota_write(update_handle, (const void *)&buffer_data[h], len);
                    if (err != ESP_OK) {
                        task_fatal_error();
                    }
            }  else {
                    err = esp_ota_write(update_handle, (const void *)buffer_data, data_read);
                    if (err != ESP_OK) {
                        task_fatal_error();
                    }
            }
            binary_file_length += data_read;
            ESP_LOGI(HTTP_DEBUG, "Written length %d", binary_file_length);
        } else if (data_read == 0) 
        {
            ESP_LOGI(HTTP_DEBUG, "Connection closed, all data received");
            break;
        }
    }
    ESP_LOGI(HTTP_DEBUG, "Total write binary data length : %d", binary_file_length);

    if (esp_ota_end(update_handle) != ESP_OK) 
    {
        ESP_LOGE(HTTP_DEBUG, "esp_ota_end failed!");
        task_fatal_error();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(HTTP_DEBUG, "Esp boot parttition failed (%s)!", esp_err_to_name(err));
        task_fatal_error();
    } 
    ESP_LOGI(HTTP_DEBUG, "Prepare to restart system!");
    esp_restart();
    return ESP_OK;

    // Note : recv_len <= req->content_len always (-ve on error)
    // In a typical scenario httpd_req_recv() should 
    // be called in a loop till the complete file is received


    // Send HTTP response with success message
    httpd_resp_send(req, "File uploaded successfully", -1);
	return ESP_OK;
}

httpd_uri_t home_url = 
{
	.uri = "/",
	.method = HTTP_GET,
	.handler = home_handler,
	.user_ctx = NULL
};
httpd_uri_t settingwifi_url = 
{
	.uri = "/settingwifi",
	.method = HTTP_GET,
	.handler = scanwifi_handler,
	.user_ctx = NULL
};

httpd_uri_t settingwifi_post_url = 
{
	.uri = "/settingwifi",
	.method = HTTP_POST,
	.handler = scanwifi_handler_post,
	.user_ctx = NULL
};

httpd_uri_t upload_url = 
{
	.uri = "/upload",
	.method = HTTP_GET,
	.handler = upload_handler,
	.user_ctx = NULL
};

httpd_uri_t upload_post_url = 
{
	.uri = "/upload",
	.method = HTTP_POST,
	.handler = upload_post_handler,
	.user_ctx = NULL
};


void start_web() 
{
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.stack_size = 8192;	
    config.send_wait_timeout = 20;
    config.recv_wait_timeout = 20;
	if (httpd_start(&web_server, &config) == ESP_OK) 
	{
		httpd_register_uri_handler(web_server, &home_url);
        httpd_register_uri_handler(web_server, &settingwifi_url);
        httpd_register_uri_handler(web_server, &settingwifi_post_url);
		httpd_register_uri_handler(web_server, &upload_url);
		httpd_register_uri_handler(web_server, &upload_post_url);
	}
	return NULL;
}

