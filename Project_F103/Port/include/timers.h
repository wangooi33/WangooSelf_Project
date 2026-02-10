/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _TIMERS_H
#define _TIMERS_H
/* Includes ------------------------------------------------------------------*/
#include "portmacro.h"
#include "task.h"
/* macro ---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif


#define timQUEUE_LENGTH				10

#define timPRIORITY_TASK 			2
#define timMANAGE_TASK_NAME 		"Tmr Svc"
#define timMANAGE_TASK_STACK_DEPTH	( portMINIMAL_STACK_SIZE * 2 )

#define tmrCOMMAND_EXECUTE_CALLBACK_FROM_ISR 	( ( BaseType_t ) -2 )
//让TimerTask执行一个普通上下文callback
#define tmrCOMMAND_EXECUTE_CALLBACK				( ( BaseType_t ) -1 )
//启动定时器
#define tmrCOMMAND_START_DONT_TRACE				( ( BaseType_t ) 0 )
// timer插入ActiveList
#define tmrCOMMAND_START					    ( ( BaseType_t ) 1 )
//重新从当前Tick开始计时
#define tmrCOMMAND_RESET						( ( BaseType_t ) 2 )
//从TimerList删除
#define tmrCOMMAND_STOP							( ( BaseType_t ) 3 )
//修改周期 + 重新调度
#define tmrCOMMAND_CHANGE_PERIOD				( ( BaseType_t ) 4 )
//删除 Timer 对象
#define tmrCOMMAND_DELETE						( ( BaseType_t ) 5 )

/* enum ----------------------------------------------------------------------*/

/* types ---------------------------------------------------------------------*/

typedef struct tmrTimerControl
{
	const char				*pTimerName;
	ListItem_t				TimerListItem;
	TickType_t				TimerPeriodInTicks;	/* 重装载值 */
	UBaseType_t				AutoReload;
	void 					*pTimerID;
	TimerCallbackFunction_t	pCallbackFunction;
	UBaseType_t			    TimerNumber;
}TIMER;
typedef TIMER Timer_t;

typedef struct tmrTimerParameters
{
	TickType_t			MessageValue;
	Timer_t *			pTimer;			/* 定时器对象 */
} TimerParameter_t;

typedef struct tmrTimerQueueMessage
{
	BaseType_t			 MessageID;		/* 命令 */
	union
	{
		TimerParameter_t TimerParameters;
	} u;
} DaemonTaskMessage_t;

typedef void * TimerHandle_t;
typedef void (*TimerCallbackFunction_t)( TimerHandle_t Timer );

/* constants -----------------------------------------------------------------*/


/* global variable -----------------------------------------------------------*/


/* functions prototypes ------------------------------------------------------*/


#define TimerStop( Timer, TicksToWait ) TimerGenericCommand( ( Timer ), tmrCOMMAND_STOP, 0U, NULL, ( TicksToWait ) )
#define TimerStart( Timer, TicksToWait ) TimerGenericCommand( ( Timer ), tmrCOMMAND_START, ( TaskGetTickCount() ), NULL, ( TicksToWait ) )
#define TimerReset( Timer, TicksToWait ) TimerGenericCommand( ( Timer ), tmrCOMMAND_RESET, ( TaskGetTickCount() ), NULL, ( TicksToWait ) )
#define TimerDelete( Timer, TicksToWait ) TimerGenericCommand( ( Timer ), tmrCOMMAND_DELETE, 0U, NULL, ( TicksToWait ) )
#define TimerChangePeriod( Timer, NewPeriod, TicksToWait ) TimerGenericCommand( ( Timer ), tmrCOMMAND_CHANGE_PERIOD, ( NewPeriod ), NULL, ( TicksToWait ) )


#ifdef __cplusplus
}
#endif
#endif /* TIMERS_H */


