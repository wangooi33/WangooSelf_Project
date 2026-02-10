/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _QUEUE_H
#define _QUEUE_H
/* Includes ------------------------------------------------------------------*/
#include "portmacro.h"
#include "list.h"
#include "task.h"
/* macro ---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

//队列类型
#define queueQUEUE_TYPE_BASE				( ( uint8_t ) 0U )
#define queueQUEUE_TYPE_MUTEX 				( ( uint8_t ) 1U )
#define queueQUEUE_TYPE_COUNTING_SEMAPHORE	( ( uint8_t ) 2U )
#define queueQUEUE_TYPE_BINARY_SEMAPHORE	( ( uint8_t ) 3U )
#define queueQUEUE_TYPE_RECURSIVE_MUTEX		( ( uint8_t ) 4U )

//状态信息
#define queueUNLOCKED			( ( int8_t ) -1 )
#define queueLOCKED_UNMODIFIED	( ( int8_t ) 0 )
#define errQUEUE_EMPTY			( ( BaseType_t ) 0 )
#define errQUEUE_FULL			( ( BaseType_t ) 0 )


//尾插
#define queueSEND_TO_BACK     ( ( BaseType_t ) 0 )
//头插
#define queueSEND_TO_FRONT    ( ( BaseType_t ) 1 )
//覆盖( len == 1 )
#define queueOVERWRITE        ( ( BaseType_t ) 2 )


/* enum ----------------------------------------------------------------------*/

/* types ---------------------------------------------------------------------*/
typedef struct QueueDefinition
{
	int8_t *pHead;
	int8_t *pTail;
	int8_t *pWriteTo;
	
	union
	{
		int8_t *pReadFrom;
		UBaseType_t RecursiveCallCount; /* 互斥量嵌套次数 */
	} u;

	List_t TasksWaitingToSend;			/* V操作阻塞事件(资源爆满 full) */
	List_t TasksWaitingToReceive;		/* P操作阻塞事件(资源枯竭 empty) */

	volatile UBaseType_t MessagesWaiting;

	UBaseType_t Length;
	UBaseType_t ItemSize;

	volatile int8_t RxLock;
	volatile int8_t TxLock;

	TaskHandle_t pMutexHolder;
	uint8_t QueueType;
} QUEUE;
typedef struct QUEUE Queue_t;


typedef struct QUEUE_REGISTRY_ITEM
{
    const char *pQueueName;
    QueueHandle_t Handle;
} QueueRegistryItem;
typedef QueueRegistryItem QueueRegistryItem_t;

typedef void * QueueHandle_t;
typedef QueueHandle_t SemaphoreHandle_t;

/* constants -----------------------------------------------------------------*/


/* global variable -----------------------------------------------------------*/


/* functions prototypes ------------------------------------------------------*/
#define QueueCreate( QueueLength, ItemSize ) QueueGenericCreate( ( QueueLength ), ( ItemSize ), ( queueQUEUE_TYPE_BASE ) )

#define SemaphoreCreateMutex() QueueCreateMutex( queueQUEUE_TYPE_MUTEX )

#define SemaphoreCreateCounting( MaxCount, InitialCount ) QueueCreateCountingSemaphore( ( MaxCount ), ( InitialCount ) )

#define SemaphoreCreateBinary() QueueGenericCreate( ( UBaseType_t ) 1, ( uint8_t )0, queueQUEUE_TYPE_BINARY_SEMAPHORE )

#define SemaphoreCreateRecursiveMutex() QueueCreateMutex( queueQUEUE_TYPE_RECURSIVE_MUTEX )

#define SemaphoreGive( Semaphore )		QueueGenericSend( ( QueueHandle_t )( Semaphore ), NULL, ( TickType_t )0U , queueSEND_TO_BACK )

#define QueueSend( Queue, pItemToQueue, TicksToWait ) QueueGenericSend( ( Queue ), ( pItemToQueue ), ( TicksToWait ), queueSEND_TO_BACK )
#define QueueSendToBack( Queue, pItemToQueue, TicksToWait ) QueueGenericSend( ( Queue ), ( pItemToQueue ), ( TicksToWait ), queueSEND_TO_BACK )
#define QueueSendToFront( Queue, pItemToQueue, TicksToWait ) QueueGenericSend( ( Queue ), ( pItemToQueue ), ( TicksToWait ), queueSEND_TO_FRONT )
#define QueueOverwrite( Queue, pItemToQueue ) QueueGenericSend( ( Queue ), ( pItemToQueue ), 0, queueOVERWRITE )



void QueueDelete( QueueHandle_t Queue );
void QueueAddToRegistry( QueueHandle_t Queue, const char *pQueueName );
void QueueUnregisterQueue( QueueHandle_t Queue );
void QueueWaitForMessageRestricted( QueueHandle_t Queue, TickType_t TicksToWait, const BaseType_t WaitIndefinitely );



#ifdef __cplusplus
}
#endif

#endif /* _QUEUE_H */





