/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _COMMUNICATE_CFG__H
#define _COMMUNICATE_CFG__H
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "Task_Manage_cfg.h"
#include "MotorData_Mgmt_cfg.h"
#include "check.h"
#include "timers.h"
#include "usart.h"
/* macro ---------------------------------------------------------------------*/
#define FRAME_HEAD			0x25
#define FRAME_TAIL			0x25
#define POSITIVE_ACK		0x80
#define NEGATIVE_ACK		0x40

/* enum ----------------------------------------------------------------------*/

/* types ---------------------------------------------------------------------*/


/* constants -----------------------------------------------------------------*/


/* global variable -----------------------------------------------------------*/

/* functions prototypes ------------------------------------------------------*/
uint8_t FormF103_DataCheck(uint8_t *pData,uint16_t len);
void ToF103_HandShake(uint8_t *pData,uint16_t len);
void ToF103_RealTimeRead_Bit(MotorData_BitVarMapIndex_en MapIndex);
void ToF103_RealTimeRead_Byte(MotorData_ByteVarMapIndex_en MapIndex);
void ToF103_RealTimeRead_HalfWord(MotorData_HalfWordVarMapIndex_en MapIndex);
void ToF103_RealTimeRead_Word(MotorData_WordVarMapIndex_en MapIndex);
void ToF103_RealTimeWrite_Bit(MotorData_BitVarMapIndex_en MapIndex,uint8_t Data);
void ToF103_RealTimeWrite_HalfWord(MotorData_HalfWordVarMapIndex_en MapIndex,uint16_t Data);




#endif /* _COMMUNICATE_CFG__H */
