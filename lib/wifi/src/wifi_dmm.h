#include <stdio.h>
#include <stdint.h>
#include <string.h>


enum WIFI_STATUS_ENUM
{  
  WIFI_DISCONNECT,
  WIFI_CONNECTED,
  WIFI_CONNECTING 
};



#define WIFI_DEBUG  "WIFI_DEBUG"

#define MAX_CONNECTION                               10 

#define LEN_BUFFER_SSID                              32
#define LEN_BUFFER_PASS                              64
#define LEN_BUFFER_WIFI_INFOR                        96

#define LEN_BUFFER_WIFI_LIST              330
#define IPV4_GOTIP_BIT   BIT0
#define IPV6_GOTIP_BIT   BIT1

#define WIFI_SSID_STA_DEFAULT          "TEST_DMM"
#define WIFI_PASS_STA_DEFAULT          "12345679"

#define WIFI_SSID_AP_DEFAULT            "DMM_DEVICE"
#define WIFI_PASS_AP_DEFAULT            "12345679"




#define MAX_NUMBER_WIFI   10

#define MAX_COUNTER_DISCONNECTED    7

typedef union      
{
    struct WiFiInfor
    {
        char Buffer_SSID[LEN_BUFFER_SSID]; 
        char Buffer_PASS[LEN_BUFFER_PASS];               
    }data;
    char Buffer_WiFi_Infor[LEN_BUFFER_WIFI_INFOR];    
}WIFI_INFOR;
 
void check_WiFi_reconnect_task(void *pvParameters);
void initialise_WiFi(uint8_t mode);
void setting_WiFi(uint8_t mode);
void wifi_scan(void);
void freeWiFi_Infor();
void free_Buffer_WiFi_list();
void config_WiFi_infor_default();