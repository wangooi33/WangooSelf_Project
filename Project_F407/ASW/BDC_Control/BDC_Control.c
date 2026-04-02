#include "BDC_Control.h"

BDC_Info_t BDC_Info;

void BDC_Disable( void )
{
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1,TIM_CHANNEL_2);
	BDC_SD_DISABLE();
}

void BDC_Enable( void )
{
	BDC_SD_ENABLE();
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
}

void BDC_MotorSpeedCalculate(void)
{
	//差分法
	static int32_t LastCnt = 0;
	int32_t NowCnt;
	int32_t Delta;			//50ms(在50ms周期内运行)内编码器脉冲数
	float NumOfRevolution; 	//50ms内转的圈数

	NowCnt = (int32_t)__HAL_TIM_GET_COUNTER(&htim3) + BDC_Info.EncoderOverflowCnt * 65536;
	Delta = NowCnt - LastCnt;
	LastCnt = NowCnt;

	NumOfRevolution = (float)Delta / PPR;
	//rps = (delta / PPR) / 0.05
	//RPM = rps × 60
	BDC_Info.RPM = NumOfRevolution / 0.05f * 60.0f;

	#if 0
	/**
	 * 速度环:一阶低通滤波
	 * 公式:
	 *    离散系统:				y[k] = (1-α) y[k-1] + α * x[k]
	 *    等效连续时间常数:	τ ≈ Ts / α
	 *      Ts = 50ms
	 *      α = 0.2
	 *      得出 → τ ≈ 0.05 / 0.2 = 0.25s:系统大约需要 0.25 秒跟随变化
	 */
	static uint8_t first = 1;
	float NewRPM;
	if (first)
	{
	    BDC_Info.RPM = NewRPM;
	    first = 0;
	}
	else
	{
	    BDC_Info.RPM = 0.8f * BDC_Info.RPM + 0.2f * NewRPM;
	}
	#endif
}


void BDC_SetPulse( uint16_t Pulse )
{
	if ( Pulse >= 5500 )
	{
		Pulse = 5500;
	}
	
	if ( BDC_Info.BDC_Direction == MOTOR_FWD )
	{
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, Pulse);
	}
	else
	{
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, Pulse);
	}
}
void BDC_SetDirection( BDC_Dir_t Dir )
{
	BDC_Info.BDC_Direction = Dir;
	if ( BDC_Info.BDC_Direction == MOTOR_FWD )
	{
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, BDC_Info.BDC_Dutyfactor);
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
	}
	else
	{
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
		__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, BDC_Info.BDC_Dutyfactor);
	}
}

