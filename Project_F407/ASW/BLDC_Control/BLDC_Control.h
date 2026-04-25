#ifndef __BLDC_CONTROL_H
#define __BLDC_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "adc.h"
/* macro ---------------------------------------------------------------------*/
#define BLDC_CURRENT_SOFT_LIMIT_mA		(5900.0f)						//软限流点,电机额定电流5.9A
#define BLDC_CURRENT_RELEASE_mA			(5400.0f)						//回差释放
#define BLDC_CURRENT_TRIP_mA			(7500.0f)						//硬保护点
#define BLDC_CURRENT_LIMIT_KP			(0.08f)							//相电流超限削减系数
#define BLDC_PWM_MIN_DUTY				(50U)

#define BLDC_POLE_PAIRS					(2U)							//电机极对数
#define BLDC_HALL_TIMER_HZ				(84000000UL / 128UL)
#define BLDC_HALL_MIN_TICKS				(8U)							//毛刺
#define BLDC_HALL_TIMEOUT_MS			(300U)							//堵转计数


#define BLDC_SD_ENABLE()  		HAL_GPIO_WritePin(BLDC_SD_GPIO_Port, BLDC_SD_Pin, GPIO_PIN_SET)
#define BLDC_SD_DISABLE()  		HAL_GPIO_WritePin(BLDC_SD_GPIO_Port, BLDC_SD_Pin, GPIO_PIN_RESET)

/* enum ----------------------------------------------------------------------*/
typedef enum
{
	PHASE_U = 0,
	PHASE_V,
	PHASE_W,
	PHASE_NONE
}Phase_t;

typedef enum
{
	MOTOR_REV = 0,
	MOTOR_FWD,					//电机正转
}MotorDir_t;

typedef enum
{
	BLDC_U_Current,				//Channel_4:U相电流
	BLDC_V_Current,				//Channel_5:V相电流
	BLDC_W_Current,				//Channel_6:W相电流
	BLDC_PowerVoltage,			//Channel_7:电源电压
	BLDC_MotorTemperature,		//Channel_8:电机温度
}ADC3_ChannelIndex_t;
/* types ---------------------------------------------------------------------*/
typedef struct
{
	Phase_t PwmPhase;		//上桥PWM
	Phase_t LowPhase;		//下桥导通
}BLDCMosCom_t;
typedef struct
{
	float U_PhaseCurrent;
	float V_PhaseCurrent;
	float W_PhaseCurrent;
}CurrentPhase_t;
typedef struct
{
	float U_PhaseSetV;
	float V_PhaseSetV;
	float W_PhaseSetV;
}PhaseSetV_t;
typedef struct
{
	float U_CurrFilt;
	float V_CurrFilt;
	float W_CurrFilt;
}PhaseCurrFilt_t;

typedef struct
{
	float PowerVoltage;					//电源电压
	CurrentPhase_t CurrentPhase;		//相电流
	PhaseSetV_t CurrZeroOffsetV;		//电流零偏 (单位:V)
	PhaseCurrFilt_t CurrFilt;			//电流滤波值
	float MotorTemperature;				//电机温度
	
	float RPM;
	MotorDir_t Direction;				//方向
	uint16_t Pulse;						//占空比
	uint8_t MotorStalling;				//电机堵转
}BLDC_Info_t;

/* global variable -----------------------------------------------------------*/
extern BLDC_Info_t BLDC_Info;
extern BLDCMosCom_t *pHallTable;
extern volatile uint32_t gHallLastEdgeMs;
extern uint8_t gHallFirstEdge;

/* functions prototypes ------------------------------------------------------*/
void Hall_enable( void );
void Hall_Disable( void );
uint8_t Hall_GetState( void );
void BLDC_Disable( void );
void BLDC_Enable( void );
void BLDC_HallTableSelect( MotorDir_t Dir );
void BLDC_ChangeMOSstate( Phase_t PwmPhase, Phase_t LowPhase, uint16_t Duty );
void BLDC_HallSpeedReset(void);
void BLDC_HallCollects(BLDC_Info_t *pBLDC);

MotorDir_t BLDC_GetDirection( BLDC_Info_t *pBLDC );

void BLDC_Cyclic( void );

#endif /*__BLDC_CONTROL_H */

