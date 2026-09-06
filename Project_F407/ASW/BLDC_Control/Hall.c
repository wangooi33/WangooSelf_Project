/* includes ------------------------------------------------------------------*/
#include "hall.h"
#include "tim.h"
#include "foc.h"

/* global variable -----------------------------------------------------------*/
Hall_Info_t Hall_Info;
const uint8_t hall_sequence[6] = {0x06,0x04,0x05,0x01,0x03,0x02};
const float hall_angle_table[8] = 
{
	0.0f,
	3.0f * HALL_STEP_ANGLE,
	5.0f * HALL_STEP_ANGLE,
	4.0f * HALL_STEP_ANGLE,
	1.0f * HALL_STEP_ANGLE,
	2.0f * HALL_STEP_ANGLE,
	0.0f,
	0.0f
};

/* macro ---------------------------------------------------------------------*/
#define HALL_SPEED_FILTER_ALPHA	(0.15f)
/* 低速换向时可直接接受与命令方向一致的霍尔沿。 */
#define HALL_DIR_CHANGE_SPEED_LIMIT	(30.0f)

/* public functions ----------------------------------------------------------*/
float Angle_Normalize(float angle)
{
	while (angle >= TWO_PI)
	{
		angle -= TWO_PI;
	}
	while (angle < 0.0f)
	{
		angle += TWO_PI;
	}
	return angle;
}
static float Hall_GetStateAngle(uint8_t hall_state)
{
	return Angle_Normalize(hall_angle_table[hall_state] + PI / 6.0f);
}
int Hall_GetIndex(uint8_t state)
{
	for (int i = 0; i < 6; i++)
	{
		if (hall_sequence[i] == state)
		{
			return i;
		}
	}
	return -1;
}
int8_t Hall_GetDirection(uint8_t old_state, uint8_t new_state)
{
	int old_index = Hall_GetIndex(old_state);
	int new_index = Hall_GetIndex(new_state);

	if (old_index < 0 || new_index < 0)
	{
		return 0;
	}
	/* 正转 */
	if (((old_index + 1) % 6) == new_index)
	{
		return 1;
	}
	/* 反转 */
	if (((old_index + 5) % 6) == new_index)
	{
		return -1;
	}

	return 0;
}


void Hall_Enable(void)
{
	//__HAL_TIM_ENABLE_IT(&htim3,TIM_IT_TRIGGER);
	HAL_TIMEx_HallSensor_Start_IT(&htim3);
}
uint8_t Hall_ReadState(void)
{
	uint8_t hall_state = 0;

	hall_state  = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6);
	hall_state |= HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) << 1;
	hall_state |= HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) << 2;

	return hall_state;
}

void Hall_UpdateEdge(uint8_t hall_state, uint32_t hall_period)
{
	int8_t dir;
	int8_t commandedDirection;
	int8_t previousDirection;
	uint8_t confirmedReversal = 0;
	uint8_t commandDirectionAccepted = 0;
	uint8_t speedFilterReset = 0;
	float angleStep;

	if (hall_state == 0x0 || hall_state == 0x7)
	{
		return;
	}

	if (!Hall_Info.initialized)
	{
		Hall_Info.state = hall_state;
		Hall_Info.last_state = hall_state;
		Hall_Info.hall_angle = Hall_GetStateAngle(hall_state);
		Hall_Info.angle = Hall_Info.hall_angle;
		Hall_Info.initialized = 1;
		return;
	}
	if (hall_state == Hall_Info.state)
	{
		Hall_Info.pending_valid = 0;
		Hall_Info.pending_direction = 0;
		Hall_Info.pending_period = 0;
		return;
	}
	dir = Hall_GetDirection(Hall_Info.state,hall_state);
	if (dir == 0)
	{
		Hall_Info.pending_valid = 0;
		Hall_Info.pending_direction = 0;
		Hall_Info.pending_period = 0;
		return;
	}
	if (hall_period < HALL_MIN_PERIOD_COUNT || hall_period > 65535U)
	{
		Hall_Info.pending_valid = 0;
		Hall_Info.pending_direction = 0;
		Hall_Info.pending_period = 0;
		return;
	}

	commandedDirection = (FOC_Info.Speed_Ref < 0.0f) ? -1 : 1;
	if (dir == commandedDirection &&
	    (Hall_Info.direction == 0 ||
	     (Hall_Info.speed_filter > -HALL_DIR_CHANGE_SPEED_LIMIT && Hall_Info.speed_filter < HALL_DIR_CHANGE_SPEED_LIMIT)))
	{
		commandDirectionAccepted = 1;
	}
	if (Hall_Info.direction != 0 && dir != Hall_Info.direction && commandDirectionAccepted == 0)
	{
		if (Hall_Info.pending_valid == 0)
		{
			Hall_Info.pending_valid = 1;
			Hall_Info.pending_direction = dir;
			Hall_Info.pending_period = hall_period;
			return;
		}

		if (Hall_Info.pending_direction != dir)
		{
			Hall_Info.pending_direction = dir;
			Hall_Info.pending_period = hall_period;
			return;
		}

		/* 连续两个同向反向边沿才确认真反转。 */
		confirmedReversal = 1;
		hall_period += Hall_Info.pending_period;
	}
	previousDirection = Hall_Info.direction;
	Hall_Info.pending_valid = 0;
	Hall_Info.pending_direction = 0;
	Hall_Info.pending_period = 0;

	Hall_Info.last_state = Hall_Info.state;
	Hall_Info.state = hall_state;
	Hall_Info.direction = dir;
	Hall_Info.hall_period = hall_period;
	if (previousDirection != 0 && previousDirection != dir)
	{
		speedFilterReset = 1;
	}

	/* 计时器频率 = 1MHz */
	angleStep = confirmedReversal ? (2.0f * HALL_STEP_ANGLE) : HALL_STEP_ANGLE;
	Hall_Info.speed = (float)dir * angleStep * (float)HALL_TIMER_HZ / (float)hall_period;
	if (speedFilterReset || Hall_Info.speed_filter_init == 0)
	{
		Hall_Info.speed_filter = Hall_Info.speed;
		Hall_Info.speed_filter_init = 1;
	}
	else
	{
		Hall_Info.speed_filter += HALL_SPEED_FILTER_ALPHA *
			(Hall_Info.speed - Hall_Info.speed_filter);
	}
	Hall_Info.hall_angle = Hall_GetStateAngle(hall_state);
	/* 每个扇区重新锚定插值角度,防止加速过程中误差持续累积。 */
	Hall_Info.angle = Hall_Info.hall_angle;

	Hall_Info.new_event = 1;
}
void Hall_Interpolate(float Ts)
{
	if (!Hall_Info.initialized)
	{
		return;
	}

	/* θ(k+1) = θ(k) + ωe × Ts */
	Hall_Info.angle += Hall_Info.speed * Ts;
	Hall_Info.angle = Angle_Normalize(Hall_Info.angle);
}

