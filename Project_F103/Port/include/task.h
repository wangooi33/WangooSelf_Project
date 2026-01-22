/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _TASK_H
#define _TASK_H
/* Includes ------------------------------------------------------------------*/
#include "portmacro.h"
#include "list.h"
/* macro ---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

#define pdTRUE			( ( BaseType_t ) 1 )
#define pdFALSE			( ( BaseType_t ) 0 )
#define pdPASS			( pdTRUE )
#define pdFAIL			( pdFALSE )

#define		taskMAX_NAMELEN			( 10 )
#define		taskSTACK_FILL_BYTE		( 0xA5U )


//任务优先级范围:0(最低优先级), 1, 2, 3, 4(最高优先级)
#define		taskMAX_PRIORITIES		( 5 )

//任务通信机制运行状态
#define		taskNOTOFY_NOT_WAIT			( ( uint8_t ) 0 )
#define		taskNOTOFY_WAITING			( ( uint8_t ) 1 )
#define		taskNOTOFY_RECEIVED			( ( uint8_t ) 2 )

//空闲任务相关
#define		taskIDLE_TASK_NAME			"IDLE_TASK"
#define		taskIDLE_TASK_PRIORITY		( UBaseType_t )0

#define		taskERROR_MEMORY_ALLOCTAE		( -1 )


/* enum ----------------------------------------------------------------------*/

//任务通知
typedef enum
{
	eNoAction = 0,					/* 通知任务,不修改任务通知值 */
	eSetBits,						/* 修改任务通知值bit位 */
	eIncrement,						/* 任务通知值累加 */
	eSetValueWithOverwrite,			/* 修改任务通知值为特定值 */
	eSetValueWithoutOverwrite		/* 如果之前已经读取,则进行设置 */
}eNotifyAction;

/* types ---------------------------------------------------------------------*/

typedef struct TaskControlBlock
{
	volatile StackType_t	*pTopOfStack;		/* 当前任务“运行时栈帧”的栈顶指针(PSP),必须排在结构体第一个,为了适配汇编 */

	ListItem_t		StateListItem;
	ListItem_t		EventListItem;
	UBaseType_t		Priority;
	StackType_t		*pStack;					/* 指向这个任务栈空间的起始地址(栈底) */
	char			pTaskName[ taskMAX_NAMELEN ];
	
	uint32_t		RunTimeCounter;

	//任务通知
	volatile uint32_t NotifiedValue;
	volatile uint8_t NotifyState;
}TCB_t;

/* constants -----------------------------------------------------------------*/


/* global variable -----------------------------------------------------------*/
extern volatile TickType_t TickCount;
extern TCB_t * volatile pCurrentTCB;

/* functions prototypes ------------------------------------------------------*/

/**
  * @function name: 
  * @brief        : 
  * @param        : 
  * @retval       : 
  * @version      : 
  * @note         : 
*/
BaseType_t TaskCreate( TaskFunction_t pTaskCode,
						  const char * const pName,
						  const uint16_t StackDepth,
						  void * const pParameters,
						  UBaseType_t Priority,
						  TaskHandle_t * const pHandle );
void TaskDelete(    TaskHandle_t TaskToDelete );
void TaskSuspendAll( void );
BaseType_t TaskResumeAll( void );
void TaskStartScheduler( void );
void TaskSwitchContext( void );
void TaskDelay( const TickType_t TicksToDelay );
void TaskDelayUntil( TickType_t * const pPreviousWakeTime, const TickType_t TimeIncrement );

BaseType_t TaskNotifyProduce( TaskHandle_t TaskToNotify, uint32_t Value, eNotifyAction eAction, uint32_t *pPreviousNotificationValue );
#define TaskNotify( TaskToNotify,Value,eAction ) TaskNotifyProduce( ( TaskToNotify ),( Value ),( eAction ),( NULL ) )
#define TaskNotifyAndQuery( TaskToNotify,Value,eAction,pPreviousNotifyValue ) TaskNotifyProduce( ( TaskToNotify ),( Value ),( eAction ),( pPreviousNotifyValue ) )

BaseType_t TaskNotifyWait( uint32_t BitsToClearOnEntry, uint32_t BitsToClearOnExit, uint32_t *pNotificationValue, TickType_t TicksToWait );
uint32_t TaskNotifyTake( BaseType_t ClearCountOnExit, TickType_t TicksToWait );
BaseType_t TaskNotifyStateClear( TaskHandle_t Task );


BaseType_t SysTickCount( void );



#ifdef __cplusplus
}
#endif

#endif /* _TASK_H */




