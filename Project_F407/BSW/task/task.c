/* Includes ------------------------------------------------------------------*/
#include "task.h"
#include "w_adc.h"
#include "key.h"
#include "hall.h"
#include "bldc_control.h"
#include "foc.h"
#include "../../Component/EC11/ec11.h"
#include "../../Component/com103/com103.h"

/* global variable -----------------------------------------------------------*/
volatile uint32_t SystemRunTime_1ms = 0;
volatile TaskControlMode_t TaskControlMode = TASK_CONTROL_IDLE;

/* public functions ----------------------------------------------------------*/
uint32_t GetTick_1ms(void)
{
    return SystemRunTime_1ms;
}

void Task_SetControlMode(TaskControlMode_t mode)
{
    TaskControlMode = mode;

    switch (mode)
    {
        case TASK_CONTROL_IDLE:
            FOC_Info.Speed_Ref = 0.0f;
            FOC_Info.Position_Ref = 0.0f;
            FOC_Info.Iq_Ref = 0.0f;
            break;

        case TASK_CONTROL_POSITION:
            FOC_Info.Speed_Ref = 0.0f;
            break;

        case TASK_CONTROL_SPEED:
            FOC_Info.Position_Ref = 0.0f;
            break;

        case TASK_CONTROL_CURRENT:
            FOC_Info.Speed_Ref = 0.0f;
            FOC_Info.Position_Ref = 0.0f;
            break;

        default:
            TaskControlMode = TASK_CONTROL_IDLE;
            break;
    }
}

TaskControlMode_t Task_GetControlMode(void)
{
    return TaskControlMode;
}

static void Task_Init(void)
{
    EC11_Init();
    Com103_Init();
    BLDC_SetSpeedRef(0.0f);
}

void Task_1ms(void)
{
    float positionRef;

    switch (TaskControlMode)
    {
        case TASK_CONTROL_IDLE:
            Hall_Info.commanded_direction = 1;
            break;

        case TASK_CONTROL_POSITION:
            positionRef = FOC_Info.Position_Ref;
            (void)BLDC_PositionPID();

            if (positionRef != 0.0f)
            {
                /* Also provide the startup state machine with the travel direction. */
                FOC_Info.Speed_Ref = (positionRef < 0.0f) ? -1.0f : 1.0f;
                Hall_Info.commanded_direction = (positionRef < 0.0f) ? -1 : 1;
            }
            else
            {
                Task_SetControlMode(TASK_CONTROL_IDLE);
            }
            break;

        case TASK_CONTROL_CURRENT:
            FOC_Info.Speed_Ref = (FOC_Info.Iq_Ref < 0.0f) ? -1.0f : 1.0f;
            Hall_Info.commanded_direction =
                (FOC_Info.Iq_Ref < 0.0f) ? -1 : 1;
            break;

        case TASK_CONTROL_SPEED:
        default:
            (void)BLDC_PositionPID();
            Hall_Info.commanded_direction =
                (FOC_Info.Speed_Ref < 0.0f) ? -1 : 1;
            BLDC_SpeedPID();
            break;
    }
}

void Task_2ms(void)
{
    Com103_Cyclic();
}

void Task_5ms(void)
{
    EC11_Cyclic();
}

void Task_10ms(void)
{

}

void Task_20ms(void)
{

}

void Task_50ms(void)
{
    KeyTask_Cyclic();
}

void Task_100ms(void)
{

}

void Task_500ms(void)
{
    BLDC_VBusCal();
}

void Task_1000ms(void)
{
    BLDC_TemperatureCal();
    LED4_TOGGLE;
}

void TaskSchedule(void)
{
    static uint8_t initialized = 0;
    static uint32_t t1 = 0;
    static uint32_t t2 = 0;
    static uint32_t t5 = 0;
    static uint32_t t10 = 0;
    static uint32_t t20 = 0;
    static uint32_t t50 = 0;
    static uint32_t t100 = 0;
    static uint32_t t500 = 0;
    static uint32_t t1000 = 0;

    uint32_t now = GetTick_1ms();

    if (initialized == 0U)
    {
        initialized = 1U;
        Task_Init();
    }

    if ((int32_t)(now - t1) >= TASK_PERIOD_1MS)
    {
        t1 += TASK_PERIOD_1MS;
        Task_1ms();
    }

    if ((int32_t)(now - t2) >= TASK_PERIOD_2MS)
    {
        t2 += TASK_PERIOD_2MS;
        Task_2ms();
    }

    if ((int32_t)(now - t5) >= TASK_PERIOD_5MS)
    {
        t5 += TASK_PERIOD_5MS;
        Task_5ms();
    }

    if ((int32_t)(now - t10) >= TASK_PERIOD_10MS)
    {
        t10 += TASK_PERIOD_10MS;
        Task_10ms();
    }

    if ((int32_t)(now - t20) >= TASK_PERIOD_20MS)
    {
        t20 += TASK_PERIOD_20MS;
        Task_20ms();
    }

    if ((int32_t)(now - t50) >= TASK_PERIOD_50MS)
    {
        t50 += TASK_PERIOD_50MS;
        Task_50ms();
    }

    if ((int32_t)(now - t100) >= TASK_PERIOD_100MS)
    {
        t100 += TASK_PERIOD_100MS;
        Task_100ms();
    }

    if ((int32_t)(now - t500) >= TASK_PERIOD_500MS)
    {
        t500 += TASK_PERIOD_500MS;
        Task_500ms();
    }

    if ((int32_t)(now - t1000) >= TASK_PERIOD_1000MS)
    {
        t1000 += TASK_PERIOD_1000MS;
        Task_1000ms();
    }
}
