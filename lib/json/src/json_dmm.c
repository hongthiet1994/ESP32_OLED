#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cJSON.h"
#include "json_dmm.h"

#include "driver/uart.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "uart_dmm.h"
#include "udp_dmm.h"
#include "crc_dmm.h"
#include "wifi_dmm.h"
#include "eeprom_dmm.h"
#include "device_infor_dmm.h"


extern int32_t restart_counter;
char buffer_json[LEN_BUFFER_JSON] = {0};
char *ip4;
extern CALIB calib[DMM_CNTSCALES];
extern DEVICE_INFOR   Device_Infor;

extern char rx_udp_buffer_backup[LEN_RX_BUFFER_UDP];
extern size_t len_buffer_udp_backup;

extern struct sockaddr_in6 sourceAddr;
extern int sock;
/* Create a bunch of objects as demonstration. */

void free_buffer_json()
{
    memset(buffer_json,0x00,LEN_BUFFER_JSON);
}

int send_json(cJSON *root,uint8_t type_send)
{  
    size_t len = 0;   
    free_buffer_json();   
    memcpy(buffer_json,cJSON_PrintUnformatted(root),LEN_BUFFER_JSON);    
    len = strlen(buffer_json);
    //ESP_LOGI(DEBUG_JSON, " len json is: %d",len);
    //ESP_LOGI(DEBUG_JSON, "json ok : %s",buffer_json);  
    if(type_send == THROUGH_UDP)  
    {
        //ESP_LOGI(DEBUG_JSON, " SEND %d BYTES TO %s",len,sourceAddr.); 
        sendto(sock, buffer_json, len, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));                
    }   
    else
    {
        uart_write_bytes(UART_STM32,buffer_json,len);       
    }
      
    return 0;
}


void creat_json_device_infor(uint8_t type)
{
    /* declare a few. */
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *generator = NULL;
    cJSON *setting = NULL;
    cJSON *wifi = NULL;
    cJSON *other = NULL;    
    cJSON *arrraycalib = NULL;
    cJSON *arrrayadd = NULL;
    cJSON *arrraymult = NULL;

    double arr_calib_add[DMM_CNTSCALES] = {0};
    double arr_calib_mult[DMM_CNTSCALES] = {0};
    
    int i = 0;   
    wifi_mode_t mode = 0;
    /* Here we construct some JSON standards, from the JSON site. */

    /* Our "Video" datatype: */
    root = cJSON_CreateObject();
    for(i=0;i<DMM_CNTSCALES;i++)
    {
        arr_calib_add[i] = calib[i].Add;
        arr_calib_mult[i] = calib[i].Mult;
    }
    if(type == REP)
    {
        cJSON_AddNumberToObject(root, "CMD", 128);
    }
    else
    {
        cJSON_AddNumberToObject(root, "CMD", 129);
    } 
    cJSON_AddItemToObject(root, "DATA", data = cJSON_CreateObject());
    cJSON_AddItemToObject(data, "GENERATOR", generator = cJSON_CreateObject());
    cJSON_AddNumberToObject(generator, "GENERATORFUNCTION", Device_Infor.data.ui8_genFunction);
    cJSON_AddNumberToObject(generator, "STATUSGEN", Device_Infor.data.ui8_status);
    cJSON_AddNumberToObject(generator, "GENERATORTYPE", Device_Infor.data.ui8_genType);
    cJSON_AddNumberToObject(generator, "SIGNALFREQUENCY", Device_Infor.data.ui16_genFreq);
    cJSON_AddNumberToObject(generator, "SIGNALRANGECONSTANT", Device_Infor.data.f_valueConstant);
    cJSON_AddNumberToObject(generator, "SIGNALRANGEMAX", Device_Infor.data.f_valueMax);
    cJSON_AddNumberToObject(generator, "SIGNALRANGEMIN", Device_Infor.data.f_valueMin);        
    cJSON_AddItemToObject(data, "SETTING", setting = cJSON_CreateObject());
    cJSON_AddNumberToObject(setting, "AUTOPOWEROFF", Device_Infor.data.ui8_apo);
    cJSON_AddNumberToObject(setting, "BUZZER", Device_Infor.data.ui8_buzzer);
    cJSON_AddNumberToObject(setting, "TIMEAPO", Device_Infor.data.ui16_timeapo);
    cJSON_AddNumberToObject(setting, "TEMPTYPE", Device_Infor.data.ui8_temptype);
    cJSON_AddNumberToObject(setting, "NUMRESETSTM", Device_Infor.data.ui32_numofresetSTM);
    cJSON_AddNumberToObject(setting, "NUMRESETESP", restart_counter);
    cJSON_AddNumberToObject(setting, "AUTOCONNECT", Device_Infor.data.ui8_autoconnect);
    cJSON_AddItemToObject(data, "WIFI", wifi = cJSON_CreateObject());
    cJSON_AddStringToObject(wifi, "ADDRESS", "12134");
    cJSON_AddStringToObject(wifi, "PASS", "12134");
    cJSON_AddStringToObject(wifi, "SSID", "12134");     
    cJSON_AddItemToObject(data, "OTHER", other = cJSON_CreateObject());
    cJSON_AddNumberToObject(other, "MEASURINGTYPE", Device_Infor.data.ui8_measuareType);
    esp_wifi_get_mode(&mode);
    cJSON_AddNumberToObject(other, "WIFIMODE", mode); 
    cJSON_AddItemToObject(other, "CALIBRATION", arrraycalib = cJSON_CreateObject());   
    cJSON_AddItemToObject(arrraycalib, "ADD", arrrayadd = cJSON_CreateDoubleArray(arr_calib_add,DMM_CNTSCALES)); 
    cJSON_AddItemToObject(arrraycalib, "MULT", arrraymult = cJSON_CreateDoubleArray(arr_calib_mult,DMM_CNTSCALES)); 
    /* Print to text */    
    if (send_json(root,THROUGH_UDP) != 0) 
    {
        cJSON_Delete(root);
        exit(EXIT_FAILURE);
    }
    cJSON_Delete(root);

}

void parser_json_udp()
{
    cJSON *root = NULL;
    int int_cmd = 0;
    int int_ChecksumCal = 0;
	int int_ChecksumRe = 0;
    root = cJSON_Parse(rx_udp_buffer_backup);
    if(root == NULL)
    {    
       ESP_LOGI(DEBUG_JSON, "NO FORMAT");     
    }
    else
    {
        int_ChecksumRe = cJSON_GetObjectItem(root,"CRC") -> valueint;
        int_ChecksumCal = ChecksumBufferJson(rx_udp_buffer_backup,len_buffer_udp_backup,CRC_DEFAULT);
        //ESP_LOGI(DEBUG_JSON, " CRC : %d",int_ChecksumRe);
        if(int_ChecksumCal != int_ChecksumRe)
        {
            int_cmd = cJSON_GetObjectItem(root,"CMD") -> valueint;	
            ESP_LOGI(DEBUG_JSON, " CMD : %d",int_cmd);			
            processRequest(int_cmd);			
        }

    }      
    cJSON_Delete(root);
}


void jsonWiFilist()
{
    wifi_mode_t mode = 0;
    wifi_scan_config_t scan_config = {0};
    wifi_ap_record_t *ap_list_buffer; 
    esp_wifi_get_mode(&mode);
    uint16_t sta_number = 0;
    uint8_t i;
    cJSON *root = NULL;
    cJSON *data = NULL;
    cJSON *wifi = NULL;
    cJSON *wifi_infor = NULL;
    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "CMD", COMMAND_SCAN_WIFI);
    cJSON_AddItemToObject(root, "DATA", data = cJSON_CreateObject());
    cJSON_AddItemToObject(data, "WIFIS", wifi = cJSON_CreateArray()); 

    if(mode == WIFI_MODE_AP)
    {     
        ESP_LOGI(WIFI_DEBUG, "Mode AP");  
        esp_wifi_set_mode(WIFI_MODE_APSTA);        
    } 
    if(esp_wifi_scan_start(&scan_config, true) == ESP_OK)
    {
        esp_wifi_scan_get_ap_num(&sta_number);
        ESP_LOGI(WIFI_DEBUG, "sta_number : %d",sta_number);  
        ap_list_buffer = malloc(sta_number * sizeof(wifi_ap_record_t));
        if (ap_list_buffer == NULL) 
        {
            ESP_LOGI(WIFI_DEBUG, "Failed to malloc buffer to print scan results");
            return;
        }
        if (esp_wifi_scan_get_ap_records(&sta_number,(wifi_ap_record_t *)ap_list_buffer) == ESP_OK) 
        {
            if(sta_number > MAX_NUMBER_WIFI)
            {
                sta_number = MAX_NUMBER_WIFI;
            }
            for(i=0; i<sta_number; i++) 
            {  
                wifi_infor = cJSON_CreateObject() ;  
                cJSON_AddStringToObject(wifi_infor, "SSID",(char*)ap_list_buffer[i].ssid);                
                cJSON_AddStringToObject(wifi_infor, "MAC",(char*) ap_list_buffer[i].ssid);
                cJSON_AddItemToArray(wifi, wifi_infor);                      
            }
            if (send_json(root,THROUGH_UDP) != 0) 
            {
                cJSON_Delete(root);
                exit(EXIT_FAILURE);
            }
            cJSON_Delete(root);
           
        }
        free(ap_list_buffer); 
    }
    else
    {
        ESP_LOGI(WIFI_DEBUG, "ERROR SCAN WIFI");
    } 



    if(mode == WIFI_MODE_AP)
    {   
        esp_wifi_set_mode(WIFI_MODE_AP);        
    }
    
}


void parserJsonWiFiInfor()
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* ssid;
    char* pass;
    root = cJSON_Parse(rx_udp_buffer_backup);
    data = cJSON_GetObjectItem(root,"DATA");

    ssid = cJSON_GetObjectItem(data,"SSID") ->valuestring;
    pass = cJSON_GetObjectItem(data,"PASS") ->valuestring;

    sendto(sock, "{\"CMD\":126}", 11, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
    ESP_LOGI(WIFI_DEBUG, "%s",ssid);
    ESP_LOGI(WIFI_DEBUG, "%s",pass);
    nvs_set_str_dmm(FIELD_SSID_STA,ssid);
    nvs_set_str_dmm(FIELD_PASSWORD_STA,pass);
    
    setting_WiFi(WIFI_MODE_STA);
    cJSON_Delete(root);    
}

void confirmIP()
{
    wifi_mode_t mode = 0;
    cJSON *root = NULL;
    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "CMD", CONFIRM_IP);    
    esp_wifi_get_mode(&mode);
    if(mode == WIFI_MODE_AP)
    {     
        cJSON_AddStringToObject(root, "IP","192.168.4.1");         
    } 
    else
    {
        cJSON_AddStringToObject(root, "IP",ip4);
    }  
    
    if (send_json(root,THROUGH_UDP) != 0) 
    {
        cJSON_Delete(root);
        exit(EXIT_FAILURE);
    } 
    cJSON_Delete(root);	
}

void proccess_cmd_change_password_system()
{
    cJSON *root = NULL;
    cJSON *data = NULL;
    char* passcurrent;
    char* passnew;  
    root = cJSON_Parse(rx_udp_buffer_backup);
    ESP_LOGI(WIFI_DEBUG, "rx_udp_buffer_backup: %s",rx_udp_buffer_backup);  
    data = cJSON_GetObjectItem(root,"DATA");

    passcurrent = cJSON_GetObjectItem(data,"CURRENTPASSWORD")  ->valuestring;
    passnew = cJSON_GetObjectItem(data,"PASSWORD") ->valuestring;


    ESP_LOGI(WIFI_DEBUG, "PASSWORD CURRENT : %s , LEN : %d",passcurrent,strlen(passcurrent));
    ESP_LOGI(WIFI_DEBUG, "PASSWORD NEW : %s , LEN : %d",passnew,strlen(passnew));  

    if(change_password_system(passcurrent,strlen(passcurrent),passnew,strlen(passnew)) == ESP_OK)
    {
        sendto(sock, "{\"CMD\":135,\"STATUS\":0}", 22, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr)); 
        
    }
    else
    {
        sendto(sock, "{\"CMD\":135,\"STATUS\":1}", 22, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
    }
    cJSON_Delete(root);	
}