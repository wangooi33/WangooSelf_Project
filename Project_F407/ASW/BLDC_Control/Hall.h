#ifndef __HALL_H
#define __HALL_H

#ifdef __cplusplus
extern "C" {
#endif

/* includes ------------------------------------------------------------------*/
#include "main.h"

/* macro ---------------------------------------------------------------------*/
#define HALL_STEP_ANGLE			(3.14159265358979323846f / 3.0f)
#define TWO_PI					(2.0f * 3.14159265358979323846f)
#define HALL_TIMER_HZ			(1000000U)
/* 小于1ms的扇区沿视为毛刺,对应机械转速上限约5000RPM。 */
#define HALL_MIN_PERIOD_COUNT	(1000U)
#define HALL_SPEED_TIMEOUT_MS	(100U)

/* types ---------------------------------------------------------------------*/
typedef struct
{
	uint8_t state;								/* 当前霍尔编码状态，合法值 1~6 */
	uint8_t last_state;							/* 上一次有效霍尔编码状态 */
	float angle;								/* Park使用的连续电角度 */
	float hall_angle;							/* Hall当前状态对应的离散位置 */
	volatile float speed;						/* 电角速度 rad/s */
	volatile float speed_filter;				/* 速度滤波值,由速度环读取 */
	uint8_t speed_filter_init;					/* 标记speed_filter是否已有首个有效数据 */
	uint32_t hall_period;						/* 最近有效霍尔边沿周期,单位us */
	int8_t direction;							/* 已确认的当前方向: +1正转,-1反转,0未确定 */
	int8_t pending_direction;					/* 待确认反向边沿的方向 */
	uint32_t pending_period;					/* 待确认反向边沿的周期计数值,单位us */
	uint8_t pending_valid;						/* 是否已有待确认的反向边沿,连续两个同向边沿才确认反转 */
	uint8_t initialized;						/* 是否已完成首次霍尔角度初始化 */
	volatile int8_t commanded_direction;		/* 当前控制环要求的霍尔方向 */
	volatile uint32_t last_event_ms;			/* 最近有效霍尔边沿时刻 */
	volatile int32_t hall_step_count;			/* 已确认霍尔扇区步数 */
	volatile float unwrapped_electrical_deg;	/* 解包相对电角度,单位度 */
	volatile float position_turns;				/* 相对机械圈数 */
	uint8_t new_event;							/* 是否产生了新的有效霍尔边沿 */
} Hall_Info_t;

/* global variable -----------------------------------------------------------*/
extern Hall_Info_t Hall_Info;
extern const uint8_t hall_sequence[6];
extern const float hall_angle_table[8];

/* functions prototypes ------------------------------------------------------*/
void Hall_Enable(void);
uint8_t Hall_ReadState(void);
void Hall_UpdateEdge(uint8_t hall_state, uint32_t now_time);
void Hall_Interpolate(float Ts);

float Angle_Normalize(float angle);
int Hall_GetIndex(uint8_t state);
int8_t Hall_GetDirection(uint8_t old_state, uint8_t new_state);

#ifdef __cplusplus
}
#endif

#endif /* __HALL_H */
