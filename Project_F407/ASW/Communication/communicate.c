#include "communicate.h"

void ToF103_NoData(uint8_t funid, uint8_t subfunid, uint8_t serialnum)
{
	uint8_t TxBuffer[16] = {0};
	uint8_t index = 0;
	TxBuffer[index++] = 0x25;
	TxBuffer[index++] = 0x25;
	TxBuffer[index++] = serialnum;
	TxBuffer[index++] = funid;
	TxBuffer[index++] = subfunid;
	TxBuffer[index++] = 0x00;
	TxBuffer[index++] = 0x00;
	TxBuffer[index]   = CheckCRC16(TxBuffer[2], index - 2);
	TxBuffer[index++] = (CheckCRC16(TxBuffer[2], index - 2)) >> 8;
	TxBuffer[index++] = 0x25;
	TxBuffer[index++] = 0x25;
}



