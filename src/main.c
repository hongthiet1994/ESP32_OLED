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
	ESP_LOGI(DEBUG_DATE_TIME,"COMPILED AT : %s",__TIME__);
	i2c_master_init();
	ssd1306_init();
	task_ssd1306_display_clear();
	xTaskCreatePinnedToCore(task_ssd1306_display_text , "ssd1306_display_text", 4096, (void *)"multimter2", 4,NULL,0);    
}
