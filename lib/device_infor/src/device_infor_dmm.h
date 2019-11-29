#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "esp_log.h"


#define DEVICE_ID_DEFAULT                           "16031994"
#define LEN_DEVICE_ID               32

#define LEN_BUFFER_DEVICE_INFOR     129

#define LEN_BUFFER_PASSWORD_SYSTEM  32
#define PASSWORD_SYSTEM_DEFAULT     "123456789"
typedef union
{
	struct device_infor
	{
		char ar_DeviceID[LEN_DEVICE_ID];
		uint8_t ui8_genFunction;
		uint8_t ui8_genType;
		uint16_t ui16_genFreq;
		float  f_valueConstant;
		float  f_valueMax;
		float  f_valueMin;
		uint8_t ui8_status;
		uint8_t ui8_showGrap;
		uint8_t ui8_apo;
		uint8_t ui8_buzzer;
		uint16_t ui16_timeapo;		
		uint8_t ui8_temptype;		
		uint8_t ui8_measuareType;		
		uint32_t ui32_numofresetSTM;
		uint32_t ui32_numofresetESP32;
		uint8_t ui8_autoconnect;
		char     arr_SSID[32];
		char     arr_PASS[32];
	} data;
	char bufferDeviceinfor[LEN_BUFFER_DEVICE_INFOR];
} DEVICE_INFOR;

#define DMM_CNTSCALES               34    // the number of scales

typedef struct _CALIB
{
  double  Mult;
  double  Add;
} CALIB;

void freeDevice_Infor();
void free_buffer_password_system();
void init_start_system();
esp_err_t check_password_current(char* pass,uint8_t len);
esp_err_t change_password_system(char* passwordcurrent,uint8_t len_pass_current, char* passnew,uint8_t len_pass_new);