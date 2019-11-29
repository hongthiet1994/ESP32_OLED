#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ssd1306_oled.h"
#include "i2c_lib.h"
#include "main.h"
 #include "nvs.h"
 #include "nvs_flash.h"
#include "uart_dmm.h"
#include "wifi_dmm.h"
#include "ota_dmm.h"
#include "udp_dmm.h"
#include "http_server_dmm.h"
#include "eeprom_dmm.h"
#include "json_dmm.h"
#include "device_infor_dmm.h"
#include "debug.h"

extern int32_t restart_counter;
extern uint32_t ui32_num_send_udp;
extern uint32_t ui32_num_receive_udp;


/* Handles for the tasks create by main(). */
TaskHandle_t xTask_UDP_server = NULL,xTask_UDP_process_data = NULL,xTask_send_udp_packet = NULL,xTask_display_value_debug = NULL;


void app_main()
{
	//Initialize NVS
	esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {        
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);   
	count_restart(); 
    ui32_num_receive_udp = nvs_get_number_dmm(FIELD_NUM_REV);
    ui32_num_send_udp = nvs_get_number_dmm(FIELD_NUM_SEND);
	ESP_LOGI(DEBUG_DATE_TIME,"COMPILED AT : %s",__TIME__);
	init_start_system();
    config_WiFi_infor_default();   
    initialise_WiFi(WIFI_MODE_STA);   
    start_web();    
	i2c_master_init();
	ssd1306_init();
	task_ssd1306_display_clear();
	
    //xTaskCreatePinnedToCore(task_ssd1306_display_text , "ssd1306_display_text", 4096, (void *)"Num_of_reset: ", 4,NULL,0);    
    xTaskCreatePinnedToCore(udp_server_task, "udp_server_task", 4096, NULL, 3, &xTask_UDP_server,1); 
    xTaskCreatePinnedToCore(process_data_udp_task, "process_data_udp_task", 4096, NULL, 2, &xTask_UDP_process_data,1); 
    //xTaskCreatePinnedToCore(send_udp_packet, "send_udp_packet", 4096, NULL, 4, &xTask_send_udp_packet,1);   // For sta mode
    xTaskCreatePinnedToCore(process_display, "process_display", 4096, NULL, 5, &xTask_display_value_debug,1); 
}
