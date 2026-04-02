#ifndef _TIMERS_H
#define _TIMERS_H

#include "list.h"
#include "port.h"

typedef void (*TimerCallBackFunction_t)( TimerHandle_t Timer );
typedef void (*PendedFunction_t)( void *, uint32_t );
typedef void* TimerHandle_t;

#define timerQUEUE_LENGTH               (10)

#define timerCOMMAND_EXECUTECALLBACK    (-1)
#define timerCOMMAND_START              (1)
#define timerCOMMAND_RESET              (2)
#define timerCOMMAND_STOP               (3)
#define timerCOMMAND_CHANGE_PERIOD      (4)
#define timerCOMMAND_DELETE             (5)

typedef struct TimersControl
{
    const char *pTimerName;
    ListItem_t TimerListItem;
    uint32_t TimerPeriodInTicks;
    uint8_t isAutoReload;
    uint32_t TimerID;
    TimerCallBackFunction_t pCallbackFunction; //回调函数执行:表示定时器到期
}Timer_t;

typedef struct TimerCounter
{
    uint32_t MessageValue;  //软件定时器开启时的系统Tick  或者作为  定时器周期的修改值
    Timer_t *pTimer;
}TimerParameter_t;

typedef struct TimerCallBack
{
    Timer_t *pTimer;
}CallBackParameter_t;

typedef struct TimerQueueMessage
{
    int8_t Command_ID;
    union
    {
        TimerParameter_t TimerParameters;
        CallBackParameter_t CallBackParameters;
    }u;
}TimerMessageRe_t;

extern QueueHandle_t TimerQueue;

void TimerManageTaskInit( void );
TimerHandle_t TimerCreate( uint32_t TimerPeriodInTicks,
                            uint8_t isAutoReload,
                            uint32_t TimerID,
                            TimerCallBackFunction_t pCallBackFunction );
uint8_t TimerGenericCommand( TimerHandle_t Timer, const uint8_t CommandID, const uint32_t Value );
uint8_t TimerPendFunctionCall( TimerHandle_t Timer );
void TimerManagerTask( void );
void TimerGetTimerID( const TimerHandle_t xTimer );                         

#endif /* _TIMERS_H */
