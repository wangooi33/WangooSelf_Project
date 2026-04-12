/* Includes ------------------------------------------------------------------*/
#include "BDC_Control.h"
/* global variable -----------------------------------------------------------*/
BDC_Info_t BDC_Info;
/* functions declaration -----------------------------------------------------*/

/* functions implementation --------------------------------------------------*/
void BDC_Disable( void )
{
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_2);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
	BDC_SD_DISABLE();
}
void BDC_Enable( void )
{
	BDC_SD_ENABLE();
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
}

void BDC_EncoderCollects( void )
{
	//差分法
	static int16_t LastCnt = 0;
	int16_t NowCnt;
	int16_t Delta;

	NowCnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
	Delta = NowCnt - LastCnt;
	LastCnt = NowCnt;

	BDC_Info.CurrentPulseCnt += (float)(ENCODER_DIRSIGN * Delta);
    BDC_Info.CurrentPosition = BDC_Info.CurrentPulseCnt;

	//rps = (delta / PPR) / 0.05    ->(50ms周期)
	//RPM = rps × 60
	BDC_Info.CurrentRPM = (float)(ENCODER_DIRSIGN * Delta) * 60.0f / (BDC_PPR * 0.05f);
}

void BDC_PIDIncInit( PID_Inc_t *pPID )
{
	pPID->Kp = 5.5f;
	pPID->Ki = 3.2f;
	pPID->Kd = 0;
	pPID->PreError = 0;
	pPID->PrePreError = 0;
	pPID->Output = 0;
}
void BDC_PIDPosInit( PID_Pos_t *pPID )
{
//	pPID->Kp = 5.5f;
//	pPID->Ki = 3.2f;
//	pPID->Kd = 0.5f;
	pPID->Kp = 1.3f;
	pPID->Ki = 0.025f;
	pPID->Kd = 0.2f;
	pPID->PreError = 0;
	pPID->SumError = 0;
	pPID->Output = 0;
}

int16_t BDC_PIDIncSpeedLoop_Cal( BDC_Info_t *pBDC )
{
	float Error_k;	//当前误差
    float Delta;
	float Output;

	Error_k = pBDC->PointRPM - pBDC->CurrentRPM;
	Delta = pBDC->PIDInc_SpeedLoop.Kp * (Error_k - pBDC->PIDInc_SpeedLoop.PreError)
		    + pBDC->PIDInc_SpeedLoop.Ki * Error_k;
	
	Output = pBDC->PIDInc_SpeedLoop.Output + Delta;
	if ( Output > BDC_MAX_PWMDUTY )
	{
		Output = BDC_MAX_PWMDUTY;
	}
	else if ( Output < BDC_MIN_PWMDUTY )
	{
		Output = BDC_MIN_PWMDUTY;
	}

	pBDC->PIDInc_SpeedLoop.Output = Output;
	pBDC->PIDInc_SpeedLoop.PrePreError = pBDC->PIDInc_SpeedLoop.PreError;
	pBDC->PIDInc_SpeedLoop.PreError = Error_k;
	
	return (int16_t)Output;
}
int16_t BDC_PIDPosSpeedLoop_Cal( BDC_Info_t *pBDC )
{
	float Error_k = pBDC->PointRPM - pBDC->CurrentRPM;
	float Output;
	float P = pBDC->PIDPos_SpeedLoop.Kp * Error_k;
	float D = pBDC->PIDPos_SpeedLoop.Kd * (Error_k - pBDC->PIDPos_SpeedLoop.PreError);

	float tempOutput = P + D + pBDC->PIDPos_SpeedLoop.Ki * pBDC->PIDPos_SpeedLoop.SumError;

	if ( !((tempOutput >= BDC_MAX_PWMDUTY && Error_k > 0) || (tempOutput <= BDC_MIN_PWMDUTY && Error_k < 0)) )
	{
		pBDC->PIDPos_SpeedLoop.SumError += Error_k;		//抗饱和
	}
	Output = P + pBDC->PIDPos_SpeedLoop.Ki * pBDC->PIDPos_SpeedLoop.SumError + D;

	if ( Output > BDC_MAX_PWMDUTY )
	{
		Output = BDC_MAX_PWMDUTY;
	}
	else if ( Output < BDC_MIN_PWMDUTY )
	{
		Output = BDC_MIN_PWMDUTY;
	}

	pBDC->PIDPos_SpeedLoop.Output = Output;
	pBDC->PIDPos_SpeedLoop.PreError = Error_k;

	return (int16_t)Output;
}

int16_t BDC_PIDPosPositionLoop_Cal( BDC_Info_t *pBDC )
{
	float Error_k = pBDC->PointPosition - pBDC->CurrentPosition;
	float Output;
	float P = pBDC->PIDPos_PositionLoop.Kp * Error_k;
	float D = pBDC->PIDPos_PositionLoop.Kd * (Error_k - pBDC->PIDPos_PositionLoop.PreError);

	float tempOutput = P + D + pBDC->PIDPos_PositionLoop.Ki * pBDC->PIDPos_PositionLoop.SumError;

	if ( !((tempOutput >= BDC_MAX_PWMDUTY && Error_k > 0) || (tempOutput <= BDC_MIN_PWMDUTY && Error_k < 0)) )
	{
		pBDC->PIDPos_PositionLoop.SumError += Error_k;		//抗饱和
	}
	Output = P + pBDC->PIDPos_PositionLoop.Ki * pBDC->PIDPos_PositionLoop.SumError + D;

	if ( Output > BDC_MAX_PWMDUTY )
	{
		Output = BDC_MAX_PWMDUTY;
	}
	else if ( Output < BDC_MIN_PWMDUTY )
	{
		Output = BDC_MIN_PWMDUTY;
	}

	pBDC->PIDPos_PositionLoop.Output = Output;
	pBDC->PIDPos_PositionLoop.PreError = Error_k;

	return (int16_t)Output;
}

void BDC_MotorCtrl( int16_t pulse )
{
	if ( pulse >= 0 )
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    }
    else
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, -pulse);
    }
}

void BDC_Cyclic( void )
{
	int16_t Pulse;
	BDC_EncoderCollects();
	Pulse = BDC_PIDIncSpeedLoop_Cal(&BDC_Info);
	//Pulse = BDC_PIDPosSpeedLoop_Cal(&BDC_Info);
	//Pulse = BDC_PIDPosPositionLoop_Cal(&BDC_Info);
	BDC_MotorCtrl(Pulse);
}

