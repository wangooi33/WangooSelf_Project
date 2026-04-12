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

void BDC_MotorSpeedCalculate( void )
{
	//差分法
    static int16_t LastCnt = 0;
    int16_t NowCnt;
    int16_t Delta;

    NowCnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
    Delta = NowCnt - LastCnt;
    LastCnt = NowCnt;

	//rps = (delta / PPR) / 0.05    ->(50ms周期)
	//RPM = rps × 60
    float rpm = (float)Delta / BDC_PPR / 0.05f * 60.0f;
    BDC_Info.CurrentRPM = ENCODER_DIRSIGN * rpm;
}

void BDC_PIDInit( PID_t *pPID )
{
	pPID->Kp = 5.5f;
	pPID->Ki = 3.2f;
	pPID->PreError = 0;
	pPID->PrePreError = 0;
	pPID->Output = 0;
}
int16_t BDC_IncrementalPID_Cal( BDC_Info_t *pBDC )
{
	//公式:Δu(k)=K_p[e(k)−e(k−1)]+K_i * e(k)+K_d[e(k)−2e(k−1)+e(k−2)]
	float Error_k;	//当前误差
    float Delta;
	float Output;

	Error_k = pBDC->PointRPM - pBDC->CurrentRPM;
	Delta = pBDC->PID_SpeedLoop.Kp * (Error_k - pBDC->PID_SpeedLoop.PreError)
		    + pBDC->PID_SpeedLoop.Ki * Error_k;
	
	Output = pBDC->PID_SpeedLoop.Output + Delta;
	if ( Output > BDC_MAX_PWMDUTY )
	{
		Output = BDC_MAX_PWMDUTY;
	}
	else if ( Output < BDC_MIN_PWMDUTY )
	{
		Output = BDC_MIN_PWMDUTY;
	}

	pBDC->PID_SpeedLoop.Output = Output;
	pBDC->PID_SpeedLoop.PrePreError = pBDC->PID_SpeedLoop.PreError;
	pBDC->PID_SpeedLoop.PreError = Error_k;
	
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
	BDC_MotorSpeedCalculate();
	Pulse = BDC_IncrementalPID_Cal(&BDC_Info);
	BDC_MotorCtrl(Pulse);
}

