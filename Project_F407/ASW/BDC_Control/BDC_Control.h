#ifndef __BDC_CONTROL_H
#define __BDC_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tim.h"

#define ENCODER			32		//编码器线数
#define SPEEDRATIO		30		//电机减速比
#define PPR				(SPEEDRATIO * ENCODER * 4)	// Pulse/r 每转脉冲数


#define BDC_SD_ENABLE()  HAL_GPIO_WritePin(BDC_SD_GPIO_Port, BDC_SD_Pin, GPIO_PIN_SET)
#define BDC_SD_DISABLE()  HAL_GPIO_WritePin(BDC_SD_GPIO_Port, BDC_SD_Pin, GPIO_PIN_RESET)

typedef enum
{
	MOTOR_FWD = 0,
	MOTOR_REV,
}BDC_Dir_t;

typedef struct BDC
{
	BDC_Dir_t BDC_Direction;
	uint16_t BDC_Dutyfactor;
	int32_t EncoderOverflowCnt;
	float RPM;
}BDC_Info_t;

extern BDC_Info_t BDC_Info;

void BDC_Disable( void );
void BDC_Enable( void );
void BDC_SetPulse( uint16_t Pulse );
void BDC_SetDirection( BDC_Dir_t Dir );
void BDC_MotorSpeedCalculate(void);



#endif /*__BDC_CONTROL_H */

