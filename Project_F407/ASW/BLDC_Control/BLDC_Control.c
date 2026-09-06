/* includes ------------------------------------------------------------------*/
#include "bldc_control.h"
#include <math.h>
#include "tim.h"
#include "w_adc.h"
#include "foc.h"
#include "pid.h"
#include "hall.h"

/* macro ---------------------------------------------------------------------*/
#define SPEED_RAMP_RPM_PER_MS		(0.05f)		/* 50 RPM/s */
#define SPEED_PID_KP				(0.01f)
#define SPEED_PID_KI				(0.005f)
#define SPEED_PID_LIMIT				(3.0f)

/* global variable -----------------------------------------------------------*/
BLDC_Info_t BLDC_Info;
PID_t d_pid;
PID_t q_pid;
PID_t speed_pid;

/* public functions ----------------------------------------------------------*/
void BLDC_Enable(void)
{
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
	HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
	BLDC_SD_ENABLE();
}
void BLDC_Disable(void)
{
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Stop(&htim1, TIM_CHANNEL_3);

	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);

	BLDC_SD_DISABLE();
}
void BLDC_PidInit(void)
{
	PID_Init(&d_pid,1.0f,0.5f,0,(24.0f * SQRT3 / 3.0f),0,0.0001f);
	PID_Init(&q_pid,1.0f,0.5f,0,(24.0f * SQRT3 / 3.0f),0,0.0001f);
	
	PID_Init(&speed_pid,SPEED_PID_KP,SPEED_PID_KI,0,
	         SPEED_PID_LIMIT,0.0f,0.001f);
	BLDC_SetSpeedRef(400.0f);
}

void BLDC_SetSpeedRef(float speedRpm)
{
	FOC_Info.Speed_Ref = Clampf(speedRpm,
	                            -BLDC_MAX_SPEED_RPM, BLDC_MAX_SPEED_RPM);
}

float BLDC_GetSpeedRef(void)
{
	return FOC_Info.Speed_Ref;
}

void BLDC_SpeedPID(void)
{
    static uint8_t speedLoopInited = 0;
    static int8_t speedLoopDirection = 0;
    static float rampRef = 0.0f;

    float speedRpmSigned;
    float speedRpm;
    float speedRefAbs;
    float error;
    float outUnlimited;
    float out;
    float outMin;
    float outMax;
    int8_t targetDirection;
    uint8_t integrate;

    if (BLDC_Info.MotorRunStage != Motor_Run)
    {
        speedLoopInited = 0;
        return;
    }

    /* 速度反馈使用绝对值，输出电流始终为正。 */
    speedRpmSigned = Hall_Info.speed_filter * 60.0f /
                     (2.0f * PI * (float)BLDC_POLE_PAIRS);
    speedRpm = fabsf(speedRpmSigned);
    speedRefAbs = fabsf(FOC_Info.Speed_Ref);
    targetDirection = (FOC_Info.Speed_Ref < 0.0f) ? -1 : 1;
    if (speedLoopDirection != 0 && speedLoopDirection != targetDirection)
    {
        /* 需要换向时重新走开环启动，避免霍尔闭环下负电流失步。 */
        speedLoopDirection = targetDirection;
        speedLoopInited = 0;
        FOC_Info.Iq_Ref = 0.0f;
        BLDC_Info.MotorRunStage = Motor_Start_Idle;
        return;
    }
    speedLoopDirection = targetDirection;

    if (speedLoopInited == 0)
    {
        speedLoopInited = 1;
        rampRef = speedRpm;
        speed_pid.Integral = (speedRpm < speedRefAbs) ?
                             FOC_Info.Iq_Ref : 0.0f;
        speed_pid.PrevErr = 0.0f;
    }

    BLDC_Info.RPM = speedRpmSigned;

    if (rampRef < speedRefAbs)
    {
        rampRef += SPEED_RAMP_RPM_PER_MS;
        if (rampRef > speedRefAbs)
        {
            rampRef = speedRefAbs;
        }
    }
    else if (rampRef > speedRefAbs)
    {
        rampRef -= SPEED_RAMP_RPM_PER_MS;
        if (rampRef < speedRefAbs)
        {
            rampRef = speedRefAbs;
        }
    }

    error = rampRef - speedRpm;
    outUnlimited = speed_pid.Kp * error + speed_pid.Integral;
    outMin = 0.0f;
    outMax = SPEED_PID_LIMIT;
    out = Clampf(outUnlimited, outMin, outMax);

    /* 只输出正向电流；减速时输出为零，让电机自然滑行。 */
    integrate = ((outUnlimited > outMin) &&
                 (outUnlimited < outMax)) ||
                ((error > 0.0f) && (outUnlimited <= outMin)) ||
                ((error < 0.0f) && (outUnlimited >= outMax)) ||
                ((error < 0.0f) && (outUnlimited <= outMin));
    if (integrate != 0)
    {
        speed_pid.Integral += speed_pid.Ki * error * speed_pid.Ts;
    }
    speed_pid.Integral = Clampf(speed_pid.Integral, outMin, outMax);
    speed_pid.PrevErr = error;

    FOC_Info.Iq_Ref = out;
}

void BLDC_CurrentPID(void)
{
	float Udc,Uref_max,Uref,scale;

	FOC_Info.Vd = PID_Update(&d_pid,FOC_Info.Id_Ref,FOC_Info.Id);
	FOC_Info.Vq = PID_Update(&q_pid,FOC_Info.Iq_Ref,FOC_Info.Iq);
	
	/* 电压矢量限幅 */
	Udc = 24.0f;
	Uref_max = Udc * SQRT3 / 3.0f;
	Uref = sqrtf(FOC_Info.Vd * FOC_Info.Vd + FOC_Info.Vq * FOC_Info.Vq);
	if (Uref > Uref_max)
	{
		scale = Uref_max / Uref;
		FOC_Info.Vd *= scale;
		FOC_Info.Vq *= scale;
	}
}
static void FOC_Run(void)
{
	BLDC_Info.Theta = Hall_Info.angle;
	
	Clark(BLDC_Info.PhaseCurrent[0],BLDC_Info.PhaseCurrent[1],&FOC_Info.Ialpha,&FOC_Info.Ibeta);
	Park(FOC_Info.Ialpha,FOC_Info.Ibeta,BLDC_Info.Theta,&FOC_Info.Id,&FOC_Info.Iq);

	/* 电流环 */
	BLDC_CurrentPID();

	RevPark(FOC_Info.Vd,FOC_Info.Vq,BLDC_Info.Theta,&FOC_Info.Valpha,&FOC_Info.Vbeta);
	SVPWM(FOC_Info.Valpha,FOC_Info.Vbeta,24.0f,(8400.0f * 2.0f),&FOC_Info.Tcm1,&FOC_Info.Tcm2,&FOC_Info.Tcm3);

	TIM1->CCR1 = FOC_Info.Tcm1;
	TIM1->CCR2 = FOC_Info.Tcm2;
	TIM1->CCR3 = FOC_Info.Tcm3;
}
void BLDC_Run(void)
{
	uint8_t hall_state;
	float startupDirection;
	
	/* 三相电流采集 */
	BLDC_PhaseCurrentCal();

	switch (BLDC_Info.MotorRunStage)
	{
		case Motor_Start_Idle:
			FOC_Info.Id_Ref = 0.0f;
			FOC_Info.Iq_Ref = 0.0f;
			BLDC_Info.MotorRunStage = Motor_Start_CheckHall;
			break;
			
		case Motor_Start_CheckHall:
			hall_state = Hall_ReadState();
			if (hall_state == 0 || hall_state == 7)
			{
				BLDC_Info.MotorRunStage = Motor_Stop;
			}
			else
			{
				Hall_Info.state = hall_state;
				BLDC_Info.MotorRunStage = Motor_Start_HallValid;
			}
			break;
			
		case Motor_Start_HallValid:
			/* 每次启动都从干净的霍尔状态开始，方向由 Speed_Ref 决定。 */
			Hall_Info.initialized = 0;
			Hall_Info.direction = 0;
			Hall_Info.pending_valid = 0;
			Hall_Info.pending_direction = 0;
			Hall_Info.pending_period = 0;
			Hall_Info.speed = 0.0f;
			Hall_Info.speed_filter = 0.0f;
			Hall_Info.speed_filter_init = 0;
			Hall_Info.hall_period = 0;
			Hall_Info.new_event = 0;
			/* hall中心角: 60°区间 + 30° */
			Hall_Info.hall_angle = hall_angle_table[Hall_Info.state] + PI / 6.0f;
			Hall_Info.hall_angle = Angle_Normalize(Hall_Info.hall_angle);
			/* 初始电角度 */
			Hall_Info.angle = Hall_Info.hall_angle;
			BLDC_Info.MotorRunStage = Motor_Start_Run;
			break;
			
		case Motor_Start_Run:
			FOC_Info.Id_Ref = 0.0f;
			startupDirection = (FOC_Info.Speed_Ref < 0.0f) ? -1.0f : 1.0f;
			FOC_Info.Iq_Ref = 1.0f;

			/* 启动阶段开环推进电角度 */
			Hall_Info.angle = Angle_Normalize(Hall_Info.angle +
			                                   startupDirection * 0.005f);
			
			if (Hall_Info.new_event == 1)
			{
				Hall_Info.new_event = 0;
				BLDC_Info.MotorRunStage = Motor_Start_Interpolation;
			}
			break;
			
		case Motor_Start_Interpolation:
			/* 获取两次Hall跳变边沿之间的时间间隔 */
			if (Hall_Info.hall_period > 0)
			{
				BLDC_Info.MotorRunStage = Motor_Run;
			}
			break;

		case Motor_Run:
			/* 霍尔角度插值 */
			Hall_Interpolate(0.0001f);
		
//		    Hall_Info.angle += 0.005f;
//			if (Hall_Info.angle >= TWO_PI)
//			{
//				Hall_Info.angle -= TWO_PI;
//			}
			break;
			
		case Motor_Stop:
			FOC_Info.Id_Ref = 0.0f;
			FOC_Info.Iq_Ref = 0.0f;
			BLDC_Disable();
			break;
		
		default:
			BLDC_Info.MotorRunStage = Motor_Start_Idle;
			break;
	}
	
	FOC_Run();
}

