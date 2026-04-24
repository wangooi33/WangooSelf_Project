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
typedef struct
{
	Phase_t PwmPhase;		//上桥PWM
	Phase_t LowPhase;		//下桥导通
}BLDCComm_t;

typedef enum
{
	MOTOR_REV = 0,
	MOTOR_FWD = 0,		//电机正转
}MotorDir_t;


/* types ---------------------------------------------------------------------*/

typedef struct
{
	MotorDir_t Direction;
	uint16_t Pulse;
	uint8_t MotorStalling;		//电机堵转
}BLDC_Info_t;

/* global variable -----------------------------------------------------------*/
extern BLDC_Info_t BLDC_Info;
extern BLDCComm_t gComFwd[8];
extern BLDCComm_t gComRev[8];
/* functions prototypes ------------------------------------------------------*/
void Hall_enable( void );
uint8_t Hall_GetState( void );
void BLDC_Disable( void );
void BLDC_Enable( void );
void BLDC_ChangeMOSstate( Phase_t PwmPhase, Phase_t LowPhase, uint16_t Duty );
MotorDir_t BLDC_GetDirection( BLDC_Info_t *pBLDC );


#endif /*__BLDC_CONTROL_H */

