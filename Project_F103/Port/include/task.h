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

/* types ---------------------------------------------------------------------*/

typedef struct TaskControlBlock
{
	volatile StackType_t	*pTopOfStack;		/* 当前任务的栈顶,必须排在结构体第一个,为了适配汇编 */

	ListItem_t		StateListItem;
	ListItem_t		EventListItem;
	UBaseType_t		Priority;
	StackType_t		*pStack;					/* 指向这个任务栈空间的起始地址 */
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
BaseType_t TaskCreate( 	TaskFunction_t pTaskCode,const char * const pName,const uint16_t StackDepth,void * const pParameters,UBaseType_t Priority,TaskHandle_t * const pHandle );
void TaskSuspendAll( void );
BaseType_t TaskResumeAll( void );
void TaskStartScheduler( void );


#ifdef __cplusplus
}
#endif

#endif /* _TASK_H */




