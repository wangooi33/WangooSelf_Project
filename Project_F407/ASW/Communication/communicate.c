#include "communicate.h"



void Com_FromF103_RxEventCallback(uint8_t *pData,uint16_t len)
{
	if (pData[0] == 0x25 && pData[1] == 0x25)
	{
		if (pData[2] == 0x1C)
		{
			ToF103_RealTimeRead_HalfWord((MotorData_HalfWordVarMapIndex_en)pData[3]);
		}
		else if (pData[2] == 0x8C)
		{
			uint16_t Data = pData[8] | pData[7] << 8;
			MotorData_HalfWordVarMap[(MotorData_HalfWordVarMapIndex_en)pData[3]].rxvalue = Data;
			ToF103_RealTimeWrite_HalfWord((MotorData_HalfWordVarMapIndex_en)pData[3],Data);
		}
		else if (pData[2] == 0xF0)
		{
			/* 释放一个二值信号量,通知握手任务。握手完成后,销毁握手任务 */
			ToF103_HandShake(pData,len);
		}
		
	}
}


