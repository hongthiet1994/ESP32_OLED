

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "crc_dmm.h"


uint16_t calculCRC(uint8_t crcData, uint16_t crcReg) 
{  	
	uint32_t i;
	for (i = 0; i < 8; i++) 
	{
		if (((crcReg & 0x8000) >> 8) ^ (crcData & 0x80))
		{
			crcReg = (crcReg << 1) ^ 0x8005;
		}
		else
		{
			crcReg = (crcReg << 1);
		}		
		crcData <<= 1;
	}
	return crcReg;
}// culCalcCRC

uint16_t ChecksumBufferJson(char* jsonStringinput,uint16_t len,uint16_t DefaultChecksum)
{
  uint16_t ChecksumCal=DefaultChecksum;
  uint16_t i=0;
  uint16_t iStart,iEnd;
  char buffertemp[200] = {0};
  for(i = 0;i<len;i++)
  {
    buffertemp[i] = jsonStringinput[i];
  }
  i = len -1;
  while(i>=1 && jsonStringinput[i] != ':')
  {
    i--;
  }
  buffertemp[i+1] = '0'; 
  buffertemp[i+2] = '}';
  iStart = 0;
  iEnd = i + 2;
  for(i=iStart;i<=iEnd;i++)
  {
    ChecksumCal = calculCRC(buffertemp[i], ChecksumCal); 
  }  
  return ChecksumCal;
}