#include <stdio.h>
#include <stdint.h>
#include <string.h>


#define DEBUG_JSON   "DEBUG_JSON"
#define THROUGH_UDP    0
#define THROUGH_UART   1
#define   REP   0
#define   SEND  1

#define LEN_BUFFER_JSON    2048
void creat_json_device_infor(uint8_t type);
void parser_json_udp();
void jsonWiFilist();
void parserJsonWiFiInfor();
void confirmIP();
void proccess_cmd_change_password_system();
