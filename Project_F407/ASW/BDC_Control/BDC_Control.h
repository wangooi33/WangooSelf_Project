#ifndef __BDC_CONTROL_H
#define __BDC_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
/* macro ---------------------------------------------------------------------*/
#define BDC_MAX_PWMDUTY     (5600 * 0.9)
#define BDC_MIN_PWMDUTY     -(5600 * 0.9)
#define ENCODER_Lines		(32)									//编码器线数
#define ENCODER_DIRSIGN   	(-1)									//编码器方向
#define BDC_SPEEDRATIO		(30)									//电机减速比
#define BDC_PPR				(BDC_SPEEDRATIO * ENCODER_Lines * 4)	//Pulse/r 每转脉冲数

#define BDC_SD_ENABLE()  	HAL_GPIO_WritePin(BDC_SD_GPIO_Port, BDC_SD_Pin, GPIO_PIN_SET)
#define BDC_SD_DISABLE()  	HAL_GPIO_WritePin(BDC_SD_GPIO_Port, BDC_SD_Pin, GPIO_PIN_RESET)
/* enum ----------------------------------------------------------------------*/

/* types ---------------------------------------------------------------------*/
typedef struct
{
    float Kp;
    float Ki;
    float Kd;
    float PreError;
    float PrePreError;
    float Output;
}PID_t;
typedef struct BDC
{
	float CurrentRPM;
	float PointRPM;
	PID_t PID_SpeedLoop;
}BDC_Info_t;
/* global variable -----------------------------------------------------------*/
extern BDC_Info_t BDC_Info;

/* functions prototypes ------------------------------------------------------*/
void BDC_Disable( void );
void BDC_Enable( void );
void BDC_PIDInit( PID_t *pPID);
void BDC_Cyclic( void );
void BDC_MotorCtrl( int16_t pulse );
void BDC_MotorSpeedCalculate( void );



#endif /*__BDC_CONTROL_H */

