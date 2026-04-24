/* includes ------------------------------------------------------------------*/
#include "BLDC_Control.h"
/* global variable -----------------------------------------------------------*/
BLDC_Info_t BLDC_Info;

BLDCComm_t gComFwd[8] =
{
	[0] = {PHASE_NONE,	PHASE_NONE},	//000:无效
	[1] = {PHASE_U,		PHASE_W},		//001:U+ W-
	[2] = {PHASE_V,		PHASE_U},		//010:V+ U-
	[3] = {PHASE_V,		PHASE_W},		//011:V+ W-
	[4] = {PHASE_W,		PHASE_V},		//100:W+ V-
	[5] = {PHASE_U,		PHASE_V},		//101:U+ V-
	[6] = {PHASE_W,		PHASE_U},		//110:W+ U-
	[7] = {PHASE_NONE,	PHASE_NONE}, 	//111:无效
};
BLDCComm_t gComRev[8] =
{
	[0] = {PHASE_NONE,	PHASE_NONE},
	[1] = {PHASE_W,		PHASE_U},		//001
	[2] = {PHASE_U,		PHASE_V},		//010
	[3] = {PHASE_W,		PHASE_V},		//011
	[4] = {PHASE_V,		PHASE_W},		//100
	[5] = {PHASE_V,		PHASE_U},		//101
	[6] = {PHASE_U,		PHASE_W},		//110
	[7] = {PHASE_NONE,	PHASE_NONE},
};

/* functions declaration -----------------------------------------------------*/

/* functions implementation --------------------------------------------------*/
void BLDC_Disable( void )
{
	HAL_TIM_PWM_Stop(&htim8,TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim8,TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim8,TIM_CHANNEL_3);
	__HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, 0);
	__HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, 0);
	__HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, 0);
	HAL_GPIO_WritePin(BLDC_CH1N_GPIO_Port, BLDC_CH1N_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BLDC_CH2N_GPIO_Port, BLDC_CH2N_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BLDC_CH3N_GPIO_Port, BLDC_CH3N_Pin, GPIO_PIN_RESET);
	BLDC_SD_DISABLE();
}
void BLDC_Enable( void )
{
	BLDC_SD_ENABLE();
	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_3);
}
void Hall_enable( void )
{
	__HAL_TIM_ENABLE_IT(&htim5, TIM_IT_TRIGGER);
	__HAL_TIM_ENABLE_IT(&htim5, TIM_IT_UPDATE);
	HAL_TIMEx_HallSensor_Start(&htim5);

	HAL_TIM_TriggerCallback(&htim5);	// 执行一次换相
}
void Hall_Disable(void)
{
	__HAL_TIM_DISABLE_IT(&htim5, TIM_IT_TRIGGER);
	__HAL_TIM_DISABLE_IT(&htim5, TIM_IT_UPDATE);
	HAL_TIMEx_HallSensor_Stop(&htim5);
}
uint8_t Hall_GetState( void )
{
	uint8_t State = 0;

	//U相
	if( HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_10) != GPIO_PIN_RESET )
	{
		State |= 0x01U << 0;
	}

	//V相
	if( HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_11) != GPIO_PIN_RESET )
	{
		State |= 0x01U << 1;
	}

	//W相
	if( HAL_GPIO_ReadPin(GPIOH, GPIO_PIN_12) != GPIO_PIN_RESET )
	{
		State |= 0x01U << 2;
	}
	
	return State;
}
void BLDC_ChangeMOSstate( Phase_t PwmPhase, Phase_t LowPhase, uint16_t Duty )
{
	__HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, 0);
	__HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, 0);
	__HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3, 0);
	HAL_GPIO_WritePin(BLDC_CH1N_GPIO_Port, BLDC_CH1N_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BLDC_CH2N_GPIO_Port, BLDC_CH2N_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(BLDC_CH3N_GPIO_Port, BLDC_CH3N_Pin, GPIO_PIN_RESET);
	
	switch (PwmPhase)
	{
		case PHASE_U:
			__HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1,Duty);
			break;
		case PHASE_V:
			__HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2,Duty);
			break;
		case PHASE_W:
			__HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_3,Duty);
			break;
		default:
			return;
	}
	switch (LowPhase)
	{
		case PHASE_U:
			HAL_GPIO_WritePin(BLDC_CH1N_GPIO_Port, BLDC_CH1N_Pin, GPIO_PIN_SET);
			break;
		case PHASE_V:
			HAL_GPIO_WritePin(BLDC_CH2N_GPIO_Port, BLDC_CH2N_Pin, GPIO_PIN_SET);
			break;
		case PHASE_W:
			HAL_GPIO_WritePin(BLDC_CH3N_GPIO_Port, BLDC_CH3N_Pin, GPIO_PIN_SET);
			break;
		default:
			return;
	}

	HAL_TIM_GenerateEvent(&htim8, TIM_EVENTSOURCE_COM);
}

MotorDir_t BLDC_GetDirection( BLDC_Info_t *pBLDC )
{
	return pBLDC->Direction;
}


