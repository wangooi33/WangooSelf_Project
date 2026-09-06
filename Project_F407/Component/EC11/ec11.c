/* Includes ------------------------------------------------------------------*/
#include "ec11.h"
#include "bldc_control.h"
#include "foc.h"
#include "../../BSW/task/task.h"

/* global variable -----------------------------------------------------------*/
int32_t EC11_EncoderLastCnt = 0;
float EC11_PulseCnt = 0.0f;
float EC11_TargetAngleDeg = 0.0f;
float EC11_AbsoluteAngleDeg = 0.0f;
float EC11_SpeedDegPerSec = 0.0f;

/* local variable ------------------------------------------------------------*/
static int32_t EC11_CountRemainder = 0;
static uint8_t EC11_Initialized = 0;

/* public functions ----------------------------------------------------------*/

void EC11_Init(void)
{
    if (HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL) != HAL_OK)
    {
        EC11_Initialized = 0;
        return;
    }

    __HAL_TIM_SET_COUNTER(&htim5, 0U);

    EC11_EncoderLastCnt = 0;
    EC11_CountRemainder = 0;
    EC11_PulseCnt = 0.0f;
    EC11_SpeedDegPerSec = 0.0f;
    EC11_AbsoluteAngleDeg = BLDC_GetCurrentTurns() * 360.0f;
    EC11_TargetAngleDeg = EC11_AbsoluteAngleDeg;
    EC11_Initialized = 1;
}

void EC11_Cyclic(void)
{
    float currentTurns;
    int32_t nowCnt;
    int32_t delta;
    int32_t signedDelta;
    int32_t stepDelta = 0;

    if (EC11_Initialized == 0U)
    {
        return;
    }

    nowCnt = (int32_t)__HAL_TIM_GET_COUNTER(&htim5);
    delta = nowCnt - EC11_EncoderLastCnt;
    EC11_EncoderLastCnt = nowCnt;

    if (delta == 0)
    {
        EC11_SpeedDegPerSec = 0.0f;
        return;
    }

    if (delta > EC11_MAX_DELTA_PER_CYCLE ||
        delta < -EC11_MAX_DELTA_PER_CYCLE)
    {
        EC11_CountRemainder = 0;
        EC11_SpeedDegPerSec = 0.0f;
        return;
    }

    signedDelta = delta * (int32_t)EC11_DIR_SIGN;
    EC11_PulseCnt += (float)signedDelta;
    EC11_CountRemainder += signedDelta;

    while (EC11_CountRemainder >= (int32_t)EC11_COUNTS_PER_DETENT)
    {
        stepDelta++;
        EC11_CountRemainder -= (int32_t)EC11_COUNTS_PER_DETENT;
    }

    while (EC11_CountRemainder <= -(int32_t)EC11_COUNTS_PER_DETENT)
    {
        stepDelta--;
        EC11_CountRemainder += (int32_t)EC11_COUNTS_PER_DETENT;
    }

    EC11_SpeedDegPerSec = (float)signedDelta * EC11_DEG_PER_COUNT
                        * (1000.0f / (float)EC11_CYCLE_PERIOD_MS);

    if (stepDelta == 0)
    {
        return;
    }

    /*
     * 位置环接口使用相对机械圈数。保持旋钮的绝对目标角度，
     * 每次将“目标 - 当前反馈”写入 Position_Ref。
     */
    currentTurns = BLDC_GetCurrentTurns();

    if (Task_GetControlMode() != TASK_CONTROL_POSITION ||
        FOC_Info.Position_Ref == 0.0f)
    {
        EC11_AbsoluteAngleDeg = currentTurns * 360.0f;
    }

    EC11_AbsoluteAngleDeg += (float)stepDelta * EC11_DEG_PER_STEP;
    Task_SetControlMode(TASK_CONTROL_POSITION);
    BLDC_SetPositionRef((EC11_AbsoluteAngleDeg / 360.0f) -
                        currentTurns);
    EC11_TargetAngleDeg = EC11_AbsoluteAngleDeg;
}
