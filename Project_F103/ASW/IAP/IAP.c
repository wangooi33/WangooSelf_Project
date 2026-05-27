#include "IAP.h"
#include <stdio.h>

IAP_Info_t IAP_Info;

static void prvIAPSetUpdateFlag(uint8_t Enable)
{
	uint32_t Val = (Enable) ? EXTEE_BOOT_UPDATEFLAG_KEY : 0;
	AT24Cxx_Write(EXTEE_BOOT_UPDATEFLAG_ADDR,(uint8_t *)&Val,4);
}
static UpgradeResult_t prvIAPStart(AppHeader_t *pAppHeader)
{
	uint32_t SectorCount;
	if (pAppHeader == NULL)
	{
		return Upgrade_Error;
	}
	if (pAppHeader->ExistFlag != APP_PACKEXIST_KEY)
	{
		return Upgrade_HeaderError;
	}
	if (pAppHeader->AppSize == 0 || pAppHeader->AppSize > (APP_END_ADDRESS - APP_START_ADDRESS + 1))
	{
		return Upgrade_SizeError;
	}

	/* APP文件头写入 */
	memset(&IAP_Info,0,sizeof(IAP_Info));
	IAP_Info.Header = *pAppHeader;
	IAP_Info.WriteAddr = EXTFLASH_APP_ADDR;
	Flash_Write((uint8_t *)pAppHeader,EXTFLASH_HEADER_ADDR,sizeof(AppHeader_t));

	/* 擦除APP区域 */
	SectorCount = pAppHeader->AppSize / 4096;
	if (pAppHeader->AppSize % 4096 > 0 )
	{
		SectorCount++;
	}
	for (uint32_t i = 0; i < SectorCount; i++)
	{
		Flash_EraseSector((EXTFLASH_APP_ADDR / 4096) + i);
	}

	IAP_Info.UpdateState = Update_Writing;
	return Upgrade_OK;
}
static UpgradeResult_t prvIAPWrite(uint8_t *Data, uint32_t Length)
{
	if (!Data || Length == 0)
	{
		return Upgrade_Error;
	}
	if (IAP_Info.RecvSize + Length > IAP_Info.Header.AppSize)
	{
		return Upgrade_SizeError;
	}
	
	Flash_Write(Data,IAP_Info.WriteAddr,Length);
	IAP_Info.WriteAddr += Length;
	IAP_Info.RecvSize  += Length;

	return Upgrade_OK;
}
static void prvIAPCalcCRC()
{
	
}
static UpgradeResult_t prvIAPFinish(void)
{
	uint32_t CRC32 = 0;
	uint8_t Buf[4];
	
	if (IAP_Info.RecvSize != IAP_Info.Header.AppSize)
	{
		return Upgrade_SizeError;
	}
	
//	CRC32 = prvIAPCalcCRC();
//	if (CRC32 != IAP_Info.Header.CRC)
//	{
//		 return Upgrade_CrcError;
//	}

	prvIAPSetUpdateFlag(BOOT_TRUE);
	AT24Cxx_Read(EXTEE_BOOT_UPDATEFLAG_ADDR,Buf,4);
	
	NVIC_SystemReset();

	return Upgrade_OK;
}

void IAP_PacketUpdateProcess(uint8_t *PacketBuf, uint16_t Size)
{
	static AppHeader_t header;

	if (Size == sizeof(AppHeader_t))
	{
		IAP_Info.UpdateState = Update_Started;
	}
	else
	{
		IAP_Info.UpdateState = Update_Writing;
	}
	switch (IAP_Info.UpdateState)
	{
		case Update_Started:
			memcpy(&header, PacketBuf, sizeof(AppHeader_t));
			prvIAPStart(&header);
			printf("Header Writed Finally \r\n");
			IAP_Info.UpdateState = Update_Idle;
			break;
			
		case Update_Writing:
			prvIAPWrite(PacketBuf, Size);
			printf("Writed Length:%d\r\n",IAP_Info.RecvSize);
			IAP_Info.UpdateState = Update_Idle;
			break;

		case Update_Finished:
			prvIAPFinish();
			IAP_Info.UpdateState = Update_Idle;
			break;

		default:
			IAP_Info.UpdateState = Update_Idle;
			break;
	}
}
void IAP_RxProcess(uint8_t *pData, uint16_t Size)
{
	if (pData[0] == 0xAA && pData[1] == 0xAA)
	{
		printf("Boot wil run \r\n");
		prvIAPFinish();
	}
}
