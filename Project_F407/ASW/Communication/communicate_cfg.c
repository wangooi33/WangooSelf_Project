#include "communicate.h"
#include "Task_Manage_cfg.h"
#include <string.h>

uint16_t Bsw_Swap16(uint16_t data);
uint32_t Bsw_Swap32(uint32_t data);
uint8_t FormF103_DataCheck(uint8_t *pData,uint16_t len);


void ToF103_TxHandle(uint8_t *pData,uint16_t len)
{
	uint8_t TxBuffer[256] = {0};
	uint8_t index = 0;
	uint16_t Crc16 = 0;
	TxBuffer[index++] = FRAME_HEAD;
	TxBuffer[index++] = FRAME_HEAD;
	memcpy(&TxBuffer[index],pData,len);
	index += len;
	Crc16 = CheckCRC16(TxBuffer,index);
	TxBuffer[index++] = (uint8_t)(Crc16 >> 8);	/* Crc16[1] */
	TxBuffer[index++] = (uint8_t)(Crc16);		/* Crc16[0] */
	TxBuffer[index++] = FRAME_TAIL;
	TxBuffer[index++] = FRAME_TAIL;
	HAL_UART_Transmit(&huart1, TxBuffer, index, index*2);
}
void ToF103_HandShake(uint8_t *pData,uint16_t len)
{
	uint8_t ResBuf[32];
	uint8_t index = 0;
	ResBuf[index++] = ResBuf[index++] = (uint8_t)(0xF0u + (uint8_t)POSITIVE_ACK);
	ResBuf[index++] = 0x00;
	ResBuf[index++] = 0x00;
	ResBuf[index++] = SoftwareVer[0];
	ResBuf[index++] = SoftwareVer[1];
	ResBuf[index++] = SoftwareVer[2];
	ResBuf[index++] = SoftwareVer[3];
	ToF103_TxHandle(ResBuf,index);
}
#if 0
void ToF103_RealTimeRead_Bit(MotorData_BitVarMapIndex_en MapIndex)
{
	uint8_t indexaddr = MotorData_BitVarMap[MapIndex].byteIndex;
	uint8_t bitaddr = MotorData_BitVarMap[MapIndex].bitIndex;
	uint8_t ResBuf[32];
	uint8_t index = 0;
	ResBuf[index++] = 0x1A;
	ResBuf[index++] = indexaddr;
	ResBuf[index++] = 0x00;
	ResBuf[index++] = 0x00 | (0x01 << bitaddr);
	ResBuf[index++] = 0x00;
	ResBuf[index++] = 0x00;
	ResBuf[index++] = (*(MotorData_BitVarMap[MapIndex].pMeta) >> bitaddr) & 0x01;
	ToF103_TxHandle(ResBuf,index);
}
void ToF103_RealTimeWrite_Bit(MotorData_BitVarMapIndex_en MapIndex,uint8_t Data)
{
	uint8_t indexaddr = MotorData_BitVarMap[MapIndex].byteIndex;
	uint8_t bitaddr = MotorData_BitVarMap[MapIndex].bitIndex;
	uint8_t ResBuf[32];
	uint8_t index = 0;
	ResBuf[index++] = 0x1A;
	ResBuf[index++] = indexaddr;
	ResBuf[index++] = 0x00;
	ResBuf[index++] = 0x00 | (0x01 << bitaddr);
	ResBuf[index++] = 0x00;
	ResBuf[index++] = 0x00;
	ResBuf[index++] = Data & 0x01;
	ToF103_TxHandle(ResBuf,index);
}
#endif
void ToF103_RealTimeRead_HalfWord(MotorData_HalfWordVarMapIndex_en MapIndex)
{
	uint8_t indexaddr = MotorData_HalfWordVarMap[MapIndex].Index;
	uint8_t ResBuf[32];
	uint8_t index = 0;
	ResBuf[index++] = 0x1C;
	ResBuf[index++] = indexaddr;
	ResBuf[index++] = 0x00;
	
	ResBuf[index++] = 0x00;
	ResBuf[index++] = 0x00;
	ResBuf[index++] = *(MotorData_HalfWordVarMap[MapIndex].pMeta) >> 8;
	ResBuf[index++] = *(MotorData_HalfWordVarMap[MapIndex].pMeta);
	ToF103_TxHandle(ResBuf,index);
}
void ToF103_RealTimeWrite_HalfWord(MotorData_HalfWordVarMapIndex_en MapIndex,uint16_t Data)
{
	uint8_t indexaddr = MotorData_HalfWordVarMap[MapIndex].Index;
	uint8_t ResBuf[32];
	uint8_t index = 0;
	ResBuf[index++] = 0x1C;
	ResBuf[index++] = indexaddr;
	ResBuf[index++] = 0x00;
	
	ResBuf[index++] = 0x00;
	ResBuf[index++] = 0x00;
	ResBuf[index++] = Data >> 8;
	ResBuf[index++] = Data;
	ToF103_TxHandle(ResBuf,index);
}
uint8_t FormF103_DataCheck(uint8_t *pData,uint16_t len)
{
	uint8_t ret = 0;
	uint16_t CRC16 = (uint16_t)((pData[len-4] << 8) | pData[len-3]);
	if (CRC16 == CheckCRC16(pData,len-4))
	{
		ret = 1;
	}
	return ret;
}
uint16_t Bsw_Swap16(uint16_t data)
{
    return ((data & 0x00ff) << 8) | ((data & 0xff00) >> 8);
}

uint32_t Bsw_Swap32(uint32_t data)
{
    return ((data & 0xff000000) >> 24) | \
           ((data & 0x00ff0000) >> 8)  | \
           ((data & 0x0000ff00) << 8)  | \
           ((data & 0x000000ff) << 24 );
}


