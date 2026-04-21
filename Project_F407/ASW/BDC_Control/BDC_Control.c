/* includes ------------------------------------------------------------------*/
#include "BDC_Control.h"
/* global variable -----------------------------------------------------------*/
BDC_Info_t BDC_Info;
int16_t EncoderLastCnt = 0;

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
void BDC_CurrentOffsetCalibrate(BDC_Info_t *pBDC)
{
	//作为系统初始化的最后一个函数,加入蜂鸣器提示
	uint32_t Sum = 0;
	
	BEEP_ON;
	// 确保电机断电、无电流
	for ( uint8_t i = 0; i < 200; i++ )
	{
		Sum += gADC1CaptureBuffer[IN9_PB1];
		HAL_Delay(2);
	}
	BEEP_OFF;
	pBDC->CurrZeroOffsetV = ((float)Sum / 200.0f) * 3.3f / 4095.0f;
	pBDC->CurrFilt = 0.0f;
}
void BDC_PIDIncSpeedInit( PID_Inc_t *pPID )
{
	pPID->Kp = 5.5f;
	pPID->Ki = 3.2f;
	pPID->Kd = 0;
	pPID->PreError = 0;
	pPID->PrePreError = 0;
	pPID->Output = 0;
}
void BDC_PIDPosSpeedInit( PID_Pos_t *pPID )
{
	pPID->Kp = 5.5f;
	pPID->Ki = 3.2f;
	pPID->Kd = 0.5f;
	pPID->PreError = 0;
	pPID->SumError = 0;
	pPID->Output = 0;
}
void BDC_PIDPosPosInit( PID_Pos_t *pPID )
{
	pPID->Kp = 1.3f;
	pPID->Ki = 0.025f;
	pPID->Kd = 0.2f;
	pPID->PreError = 0;
	pPID->SumError = 0;
	pPID->Output = 0;
}
void BDC_PIDCurInit( PID_Pos_t *pPID )
{
	pPID->Kp = 0.8f;
	pPID->Ki = 0.05f;
	pPID->Kd = 0;
	pPID->PreError = 0;
	pPID->SumError = 0;
	pPID->Output = 0;
}

void BDC_EncoderCollects( BDC_Info_t *pBDC )
{
	//差分法
	int16_t NowCnt;
	int16_t Delta;

	NowCnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
	Delta = NowCnt - EncoderLastCnt;
	EncoderLastCnt = NowCnt;

	pBDC->PulseCnt += (float)(ENCODER_DIRSIGN * Delta);

	//rps = (delta / PPR) / 0.01    ->(10ms周期)
	//RPM = rps × 60
	pBDC->RPM = (float)(ENCODER_DIRSIGN * Delta) * 60.0f / (BDC_PPR * 0.01f);
}
void BDC_ADCCollects( BDC_Info_t *pBDC )
{
	float vbus_adc = (float)gADC1CaptureBuffer[IN8_PB0] * 3.3f / 4095.0f;
	float curr_adc = (float)gADC1CaptureBuffer[IN9_PB1] * 3.3f / 4095.0f;

	// 电压(先转电压再计算)
	pBDC->PowerVoltage = (vbus_adc - 1.24f) * 37.0f;

	// 电流(带零偏)
	float curr_raw = (curr_adc - pBDC->CurrZeroOffsetV) / (8.0f * 0.02f) * 1000.0f;

	// 一阶低通滤波
	pBDC->CurrFilt = 0.9f * pBDC->CurrFilt + 0.1f * curr_raw;

	//单向限幅
	if ( pBDC->CurrFilt < 0 )

	{
		pBDC->CurrFilt = 0;
	}

	pBDC->EleCurrent = pBDC->CurrFilt;
}

float BDC_SpeedIncPID_Cal( PID_Inc_t *pPID, float Expectation, float CurrentVal )
{
	float Error_k;    //当前误差
	float Delta;
	float Output;

	Error_k = Expectation - CurrentVal;
	Delta = pPID->Kp * (Error_k - pPID->PreError) + pPID->Ki * Error_k;

	Output = pPID->Output + Delta;
	if ( Output > BDC_MAX_CUR_TARGET )
	{
	    Output = BDC_MAX_CUR_TARGET;
	}
	else if ( Output < BDC_MIN_CUR_TARGET )
	{
	    Output = BDC_MIN_CUR_TARGET;
	}

	pPID->Output = Output;
	pPID->PrePreError = pPID->PreError;
	pPID->PreError = Error_k;

	return Output;
}
float BDC_SpeedPosPID_Cal( PID_Pos_t *pPID, float Expectation, float CurrentVal )
{
	float Error_k = Expectation - CurrentVal;
	float Output;
	float P = pPID->Kp * Error_k;
	float D = pPID->Kd * (Error_k - pPID->PreError);

	float tempOutput = P + D + pPID->Ki * pPID->SumError;

	if (!((tempOutput >= BDC_MAX_CUR_TARGET && Error_k > 0) ||
			(tempOutput <= BDC_MIN_CUR_TARGET && Error_k < 0)))
	{
	    pPID->SumError += Error_k;
	}

	Output = P + pPID->Ki * pPID->SumError + D;

	if ( Output > BDC_MAX_CUR_TARGET )
	{
		Output = BDC_MAX_CUR_TARGET;
	}
	else if ( Output < BDC_MIN_CUR_TARGET )
	{
		Output = BDC_MIN_CUR_TARGET;
	}
	pPID->Output = Output;
	pPID->PreError = Error_k;

	return Output;
}
int16_t BDC_CurrentPI_Cal( PID_Pos_t *pID, float Target, float Feedback )
{
	float Error = Target - Feedback;
	float Output;

	pID->SumError += Error;

	Output = pID->Kp * Error + pID->Ki * pID->SumError;

	if (Output > BDC_MAX_PWMDUTY)
	{
		Output = BDC_MAX_PWMDUTY;
		pID->SumError -= Error;   // 抗积分饱和
	}
	else if (Output < BDC_MIN_PWMDUTY)
	{
		Output = BDC_MIN_PWMDUTY;
		pID->SumError -= Error;
	}

	pID->Output = Output;
	pID->PreError = Error;

	return (int16_t)Output;
}

void BDC_MotorCtrl( int16_t pulse )
{
	if ( pulse > -PWM_DEADZONE && pulse < PWM_DEADZONE )
	{
		pulse = 0;
	}

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
void BDC_PIDInit( void )
{
	BDC_PIDIncSpeedInit(&BDC_Info.PIDInc_SpeedLoop);
	BDC_PIDPosSpeedInit(&BDC_Info.PIDPos_SpeedLoop);
	BDC_PIDPosPosInit(&BDC_Info.PID_PositionLoop);
	BDC_PIDCurInit(&BDC_Info.PID_CurrentLoop);
}
void BDC_EncoderReset( void )
{
	EncoderLastCnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim3);
	BDC_Info.RPM = 0.0f;
}
void BDC_ResetControlState( BDC_Info_t *pBDC )
{
	pBDC->Expectation.ExpectedCur = 0.0f;
	pBDC->EleCurrent = 0.0f;
	pBDC->CurrFilt = 0.0f;

	pBDC->PIDPos_SpeedLoop.SumError = 0.0f;
	pBDC->PIDPos_SpeedLoop.PreError  = 0.0f;
	pBDC->PIDPos_SpeedLoop.Output    = 0.0f;

	pBDC->PID_CurrentLoop.SumError = 0.0f;
	pBDC->PID_CurrentLoop.PreError  = 0.0f;
	pBDC->PID_CurrentLoop.Output    = 0.0f;

	pBDC->PID_PositionLoop.SumError = 0.0f;
	pBDC->PID_PositionLoop.PreError  = 0.0f;
	pBDC->PID_PositionLoop.Output    = 0.0f;

	pBDC->RPM = 0.0f;
	pBDC->PulseCnt = 0.0f;
}

static void BDC_RampTargetRPM( BDC_Info_t *pBDC )
{
	const float Step = 2.0f;			// 每5ms变化2RPM

	if ( pBDC->Expectation.ExpectedRPM_Ramp < pBDC->Expectation.ExpectedRPM )
	{
		pBDC->Expectation.ExpectedRPM_Ramp += Step;
		if ( pBDC->Expectation.ExpectedRPM_Ramp > pBDC->Expectation.ExpectedRPM )
		{
			pBDC->Expectation.ExpectedRPM_Ramp = pBDC->Expectation.ExpectedRPM;
		}
		
	}
	else if ( pBDC->Expectation.ExpectedRPM_Ramp > pBDC->Expectation.ExpectedRPM )
	{
		pBDC->Expectation.ExpectedRPM_Ramp -= Step;
		if ( pBDC->Expectation.ExpectedRPM_Ramp < pBDC->Expectation.ExpectedRPM )
		{
			pBDC->Expectation.ExpectedRPM_Ramp = pBDC->Expectation.ExpectedRPM;
		}
	}
}
void BDC_Cyclic(void)
{
	int16_t Pulse;
	static uint8_t SpeedLoopCnt = 0;

	BDC_ADCCollects(&BDC_Info);
	BDC_RampTargetRPM(&BDC_Info);		//斜坡增量

	//速度环降频:10ms
	if ( ++SpeedLoopCnt >= 2 )
	{
		SpeedLoopCnt = 0;

		BDC_EncoderCollects(&BDC_Info);

		BDC_Info.Expectation.ExpectedCur = BDC_SpeedPosPID_Cal(&BDC_Info.PIDPos_SpeedLoop,
																BDC_Info.Expectation.ExpectedRPM_Ramp,
																BDC_Info.RPM);
	}

	//电流环
	Pulse = BDC_CurrentPI_Cal(&BDC_Info.PID_CurrentLoop,BDC_Info.Expectation.ExpectedCur,BDC_Info.EleCurrent);

	BDC_MotorCtrl(Pulse);
}

