/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _MOTORDATA_MGMT_CFG_H
#define _MOTORDATA_MGMT_CFG_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "w_adc.h"
/* macro ---------------------------------------------------------------------*/

/* enum ----------------------------------------------------------------------*/
typedef enum
{
	MotorData_BitVarMapIndex_00,
	MotorData_BitVarMapIndex_01,
	MotorData_BitVarMapIndex_Max,
}MotorData_BitVarMapIndex_en;

typedef enum
{
	MotorData_ByteVarMapIndex_00,
	MotorData_ByteVarMapIndex_01,
	MotorData_ByteVarMapIndex_Max,
}MotorData_ByteVarMapIndex_en;

typedef enum
{
	MotorData_HalfWordVarMapIndex_00,
	MotorData_HalfWordVarMapIndex_01,
	MotorData_HalfWordVarMapIndex_02,
	MotorData_HalfWordVarMapIndex_03,
	MotorData_HalfWordVarMapIndex_04,
	MotorData_HalfWordVarMapIndex_05,
	MotorData_HalfWordVarMapIndex_06,
	MotorData_HalfWordVarMapIndex_07,
	MotorData_HalfWordVarMapIndex_Max,
}MotorData_HalfWordVarMapIndex_en;

typedef enum
{
	MotorData_WordVarMapIndex_00,
	MotorData_WordVarMapIndex_01,
	MotorData_WordVarMapIndex_02,
	MotorData_WordVarMapIndex_03,
	MotorData_WordVarMapIndex_04,
	MotorData_WordVarMapIndex_05,
	MotorData_WordVarMapIndex_06,
	MotorData_WordVarMapIndex_07,
	MotorData_WordVarMapIndex_Max,
}MotorData_WordVarMapIndex_en;

/* types ---------------------------------------------------------------------*/
typedef struct 
{
	uint8_t byteIndex;
	uint8_t bitIndex;
	uint8_t *pMeta;
	uint8_t rxvalue;
	uint8_t reserve;
}MotorData_BitVarMap_st;

typedef struct 
{
	uint8_t Index;
	uint8_t *pMeta;
	uint8_t rxvalue;
	uint8_t reserve;
}MotorData_ByteVarMap_st;

typedef struct 
{
	uint8_t Index;
	uint16_t *pMeta;
	uint16_t rxvalue;
	uint8_t reserve;
}MotorData_HalfWordVarMap_st;

typedef struct 
{
	uint8_t Index;
	uint32_t *pMeta;
	uint32_t rxvalue;
	uint8_t reserve;
}MotorData_WordVarMap_st;

/* constants -----------------------------------------------------------------*/


/* global variable -----------------------------------------------------------*/
//extern MotorData_BitVarMap_st MotorData_BitVarMap[MotorData_BitVarMapIndex_Max];
//extern MotorData_ByteVarMap_st MotorData_ByteVarMap[MotorData_ByteVarMapIndex_Max];
extern MotorData_HalfWordVarMap_st MotorData_HalfWordVarMap[MotorData_HalfWordVarMapIndex_Max];
//extern MotorData_WordVarMap_st MotorData_WordVarMap[MotorData_WordVarMapIndex_Max];
/* functions prototypes ------------------------------------------------------*/



#endif /* _MOTORDATA_MGMT_CFG_H */




