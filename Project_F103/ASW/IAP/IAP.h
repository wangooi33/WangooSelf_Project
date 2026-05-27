#ifndef _IAP_H
#define _IAP_H
/* includes -----------------------------------------------------------------*/
#include "main.h"
#include "AT24Cxx.h"
#include "NM25Qxx.h"
#include "check.h"
#include <stdbool.h>
#include <string.h>

/* macro --------------------------------------------------------------------*/
#define BOOT_FALSE						0
#define BOOT_TRUE						1

#define APP_START_ADDRESS				0x08008000u
#define APP_END_ADDRESS					0x0807FFFFu
#define EXTFLASH_HEADER_ADDR			0x000000u
#define EXTFLASH_APP_ADDR				0x010000u
#define EXTEE_BOOT_UPDATEFLAG_ADDR		0x00

#define EXTEE_BOOT_UPDATEFLAG_KEY		0xA82002A8u
#define APP_PACKEXIST_KEY				0x5AA55AA5u

/* enum ---------------------------------------------------------------------*/
typedef enum
{
    Upgrade_OK = 0,
    Upgrade_Error,
    Upgrade_CrcError,
    Upgrade_SizeError,
    Upgrade_HeaderError,
} UpgradeResult_t;

typedef enum
{
    Update_Idle = 0,
    Update_Started,
    Update_Writing,
    Update_Finished,
} UpdateState_t;
	
/* types --------------------------------------------------------------------*/
typedef struct
{
    uint32_t ExistFlag;
    uint32_t AppSize;
    uint32_t CRC32;
    uint32_t AppVersion;
} AppHeader_t;

typedef struct
{
    AppHeader_t Header;
    uint32_t RecvSize;
    uint32_t WriteAddr;
	UpdateState_t UpdateState;
	uint8_t AppWriteFinally;
} IAP_Info_t;

/* functions prototypes -----------------------------------------------------*/
void IAP_PacketUpdateProcess(uint8_t *PacketBuf, uint16_t Size);
void IAP_RxProcess(uint8_t *pData, uint16_t Size);

#endif
