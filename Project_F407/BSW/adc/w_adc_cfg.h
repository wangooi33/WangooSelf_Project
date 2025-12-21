/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _W_ADC_CFG_H
#define _W_ADC_CFG_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* macro ---------------------------------------------------------------------*/

/* enum ----------------------------------------------------------------------*/

/* types ---------------------------------------------------------------------*/
typedef struct 
{
	uint16_t Motor_IU;		/* 电机U相电流 */
	uint16_t Motor_IV;		/* 电机V相电流 */
	uint16_t Motor_IW;		/* 电机W相电流 */
	uint16_t Motor_VBUS;	/* 电机母线电压 */
	uint16_t Motor_IBUS;	/* 电机母线电流 */
	uint16_t Motor_EMFU;	/* 电机U相反向电动势 */
	uint16_t Motor_EMFV;	/* 电机V相反向电动势 */
	uint16_t Motor_EMFW;	/* 电机W相反向电动势 */
}MotorData_FromADC_st;


/* constants -----------------------------------------------------------------*/


/* global variable -----------------------------------------------------------*/
extern MotorData_FromADC_st MotorData_FromADC;

/* functions prototypes ------------------------------------------------------*/


#endif /* _W_ADC_CFG_H */



