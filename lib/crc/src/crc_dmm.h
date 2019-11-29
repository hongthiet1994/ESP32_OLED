#include <stdio.h>
#include <string.h>
#include "esp_log.h"

#define CRC_DEFAULT  0xFFFF

uint16_t ChecksumBufferJson(char* jsonStringinput,uint16_t len,uint16_t DefaultChecksum);
