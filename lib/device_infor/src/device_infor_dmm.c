#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nvs.h"
#include "esp_log.h"
#include "device_infor_dmm.h"
#include "eeprom_dmm.h"
#include "uart_dmm.h"

DEVICE_INFOR   Device_Infor;

CALIB calib[DMM_CNTSCALES];

char buffer_password_system[LEN_BUFFER_PASSWORD_SYSTEM] = {0};

void freeDevice_Infor()
{
    memset(Device_Infor.bufferDeviceinfor,0x00,LEN_BUFFER_DEVICE_INFOR);
}

void free_buffer_password_system()
{
    memset(buffer_password_system,0x00,LEN_BUFFER_PASSWORD_SYSTEM);
}

void init_start_system()
{
    size_t len;
    free_buffer_password_system();
    if(nvs_get_str_dmm(FIELD_PASSWORD_SYSTEM,buffer_password_system,&len) == ESP_ERR_NVS_NOT_FOUND)
    {               
        ESP_LOGI(EEPROM_DEBUG,"PASSWORD SYSTEM ERROR : PASSWORD_SYSTEM_DEFAULT");            
        nvs_set_str_dmm(FIELD_PASSWORD_SYSTEM, PASSWORD_SYSTEM_DEFAULT);            
    }
    else
    {
        ESP_LOGI(EEPROM_DEBUG,"PASSWORD SYSTEM : %s",buffer_password_system); 
    } 
    uart_write_bytes(UART_STM32,COMMAND_STOP_SEND_DATA_GRAPH,LEN_COMMAND_STOP_SEND_DATA_GRAPH);    
}

esp_err_t check_password_current(char* pass,uint8_t len)
{
    size_t len_get = 0;
    uint8_t i = 0;
    char password_current[LEN_BUFFER_PASSWORD_SYSTEM] = {0};
    if(nvs_get_str_dmm(FIELD_PASSWORD_SYSTEM,password_current,&len_get) == ESP_ERR_NVS_NOT_FOUND)
    {               
        ESP_LOGI(EEPROM_DEBUG,"CAN NOT GET PASSWORD CURRENT");            
        return !ESP_OK;      
    }
    else
    {
        if (len_get == len)
        {
            for (i = 0; i < len_get; i++)
            {
                if(password_current[i] != pass[i])
                {
                    return !ESP_OK;                                        
                }
            }
            ESP_LOGI(EEPROM_DEBUG,"CHECK PASSWORD SUCCESSFULLY");
            return ESP_OK;
        }
        else
        {
            return !ESP_OK;
        }
    }
    
    
}

esp_err_t change_password_system(char* passwordcurrent,uint8_t len_pass_current, char* passnew,uint8_t len_pass_new)
{
    uint8_t i=0;
    size_t len_get;
    char buffer_temp[LEN_BUFFER_PASSWORD_SYSTEM] = {0};   
    if(check_password_current(passwordcurrent,len_pass_current) != ESP_OK)
    {
        return !ESP_OK; 
    }      
    for (i = 0; i < len_pass_new; i++)
    {
        buffer_temp[i] = passnew[i];        
    }
    if(nvs_set_str_dmm(FIELD_PASSWORD_SYSTEM, buffer_temp) == ESP_OK)
    {
        if(check_password_current(passnew,len_pass_new) != ESP_OK)
        {
            return !ESP_OK; 
        }   
        else
        {
            return ESP_OK; 
        }                       
    }
    else
    {
        return !ESP_OK;        
    }    
}