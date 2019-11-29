#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "ssd1306_oled.h"
#include "font8x8_basic.h"
#include "driver/gpio.h"
#include "eeprom_dmm.h"
#include "debug.h"


extern wifi_config_t wifi_config_ap;
extern uint32_t ui32_num_send_udp;
extern uint32_t ui32_num_receive_udp ;
extern int32_t restart_counter;
void reset_OLED_lcd()
{
	gpio_set_direction(RST_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(RST_PIN, 0);
	vTaskDelay(50/portTICK_PERIOD_MS);
	gpio_set_level(RST_PIN, 1);
}
void ssd1306_init() 
{
	esp_err_t espRc;
	reset_OLED_lcd();
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);

	i2c_master_write_byte(cmd, OLED_CMD_SET_CHARGE_PUMP, true);
	i2c_master_write_byte(cmd, 0x14, true);

	i2c_master_write_byte(cmd, OLED_CMD_SET_SEGMENT_REMAP, true); 
	i2c_master_write_byte(cmd, OLED_CMD_SET_COM_SCAN_MODE, true); 

	i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_ON, true);
	i2c_master_stop(cmd);

	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	if (espRc == ESP_OK) 
	{
		ESP_LOGI(DUBUG_OLED, "OLED configured successfully");
	} else 
	{
		ESP_LOGE(DUBUG_OLED, "OLED configuration failed. code: 0x%.2X", espRc);
	}
	i2c_cmd_link_delete(cmd);
}



void task_ssd1306_display_pattern(void *ignore) 
{
	i2c_cmd_handle_t cmd;
	for (uint8_t i = 0; i < 6; i++) 
	{
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_SINGLE, true);
		i2c_master_write_byte(cmd, 0xB0 | i, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
		for (uint8_t j = 0; j < 2; j++) 
		{
			i2c_master_write_byte(cmd, 0xFF >> (j % 8), true);
		}
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	}
	vTaskDelete(NULL);
}

void task_ssd1306_display_clear() 
{
	i2c_cmd_handle_t cmd;
	//uint8_t zero[128];
	for (uint8_t i = 0; i < 8; i++) 
	{
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_SINGLE, true);
		i2c_master_write_byte(cmd, 0xB0 | i, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
		//i2c_master_write(cmd, zero, 128, true);
		for (uint8_t j = 0; j < 128; j++) 
		{
			i2c_master_write_byte(cmd, 0 >> (j % 8), true);
		}
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	}

	//vTaskDelete(NULL);
}


void task_ssd1306_contrast(void *ignore) 
{
	i2c_cmd_handle_t cmd;
	uint8_t contrast = 0;
	uint8_t direction = 1;
	while (true) 
	{
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
		i2c_master_write_byte(cmd, OLED_CMD_SET_CONTRAST, true);
		i2c_master_write_byte(cmd, contrast, true);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
		vTaskDelay(1/portTICK_PERIOD_MS);
		contrast += direction;
		if (contrast == 0xFF) 
		{ 
			direction = -1; 
		}
		if (contrast == 0x0) 
		{ 
			direction = 1; 
		}
	}
	vTaskDelete(NULL);
}


void task_ssd1306_display_text(void *arg_text)
{
	char *text = (char*)arg_text;
	uint8_t text_len = strlen(text);

	i2c_cmd_handle_t cmd;
	uint8_t cur_page = 0;
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
	i2c_master_write_byte(cmd, 0x00, true);            // reset column
	i2c_master_write_byte(cmd, 0x10, true);
	i2c_master_write_byte(cmd, 0xB0 | cur_page, true); // reset page
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	for (uint8_t i = 0; i < text_len; i++) 
	//uint8_t i = 2;
	{
		if (text[i] == '\n') 
		{
			cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
			i2c_master_write_byte(cmd, 0x00, true); // reset column
			i2c_master_write_byte(cmd, 0x10, true);
			i2c_master_write_byte(cmd, 0xB0 | ++cur_page, true); // increment page

			i2c_master_stop(cmd);
			i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
			i2c_cmd_link_delete(cmd);
		} 
		else 
		{
			cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
			//i2c_master_write(cmd, font8x8_basic_tr[0], 8, true);
			i2c_master_write(cmd, font8x8_basic_tr[(uint8_t)text[i]], 8, true);
			i2c_master_stop(cmd);
			i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
			i2c_cmd_link_delete(cmd);
			//ESP_LOGI("OLED", "Display string");
		}
	}
	vTaskDelete(NULL);
}

void display_value_debug(char* key, uint8_t len_key,uint32_t num_of_reset,uint8_t cur_page)
{
	char buf_temp[16] = {0};
	char buffer[8] = {0};	
	uint8_t i = 0;
	uint8_t text_len = 0;	
	i2c_cmd_handle_t cmd;
	itoa(num_of_reset,buffer,10);
	memcpy(buf_temp,key,len_key);
	memcpy(&buf_temp[len_key],buffer,9);	
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
	i2c_master_write_byte(cmd, 0x00, true);            // reset column
	i2c_master_write_byte(cmd, 0x10, true);
	i2c_master_write_byte(cmd, 0xB0 | cur_page, true); // reset page
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	for ( i = 0; i < 16; i++) 
	{		
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);	
		i2c_master_write(cmd, font8x8_basic_tr[32], 8, true);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	}
	text_len = strlen(buf_temp);
	for ( i = 0; i < text_len; i++) 
	{		
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
		//i2c_master_write(cmd, font8x8_basic_tr[0], 8, true);
		i2c_master_write(cmd, font8x8_basic_tr[(uint8_t)buf_temp[i]], 8, true);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	}

}

void display_string_debug(char* buffer, uint8_t lenbuffer,uint8_t cur_page)
{
	uint8_t i = 0;
	i2c_cmd_handle_t cmd;	
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
	i2c_master_write_byte(cmd, 0x00, true);            // reset column
	i2c_master_write_byte(cmd, 0x10, true);
	i2c_master_write_byte(cmd, 0xB0 | cur_page, true); // reset page
	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);	
	for ( i = 0; i < 16; i++) 
	{		
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);	
		i2c_master_write(cmd, font8x8_basic_tr[32], 8, true);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	}
	for ( i = 0; i < lenbuffer; i++) 
	{		
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);	
		i2c_master_write(cmd, font8x8_basic_tr[(uint8_t)buffer[i]], 8, true);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	}

}


void process_display(void *pvParameters)
{
	uint32_t i=0;
	display_value_debug(KEY_NUM_OF_RESET,strlen(KEY_NUM_OF_RESET),restart_counter,1);
	display_value_debug("Rev: ",5,ui32_num_receive_udp,3); 
	display_value_debug("Send: ",6,ui32_num_send_udp,4); 
    for (;;)
    {
		i++;
		if(i%2 ==0 )
		{			
			display_string_debug((char*)wifi_config_ap.ap.ssid,strlen((char*)wifi_config_ap.ap.ssid),6);
			display_string_debug("----------------",16,7);
		}
		else
		{
			display_string_debug((char*)wifi_config_ap.ap.ssid,strlen((char*)wifi_config_ap.ap.ssid),6);
			display_string_debug("                ",16,7);			
		}
		
		// display_value_debug(KEY_NUM_OF_RESET,strlen(KEY_NUM_OF_RESET),restart_counter,1);
		// display_value_debug("Rev: ",5,ui32_num_receive_udp,3); 
        // display_value_debug("Send: ",6,ui32_num_send_udp,4); 
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
