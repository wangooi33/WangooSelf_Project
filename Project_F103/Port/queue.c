#include "queue.h"

/*--------------------------------------------------------------------------------------*/
#define queueREGISTRY_SIZE 8
/*--------------------------------------------------------------------------------------*/

QueueRegistryItem_t QueueRegistry[ queueREGISTRY_SIZE ];

/*--------------------------------------------------------------------------------------*/
static void prvInitialiseNewMutex( Queue_t * const pNewQueue );
static BaseType_t prvIsQueueFull( const Queue_t *pQueue );
static BaseType_t prvIsQueueEmpty( const Queue_t *pQueue );
static void prvLockQueue( Queue_t * const pQueue );
static void prvUnlockQueue(Queue_t * const pQueue);
static BaseType_t prvCopyDataToQueue( Queue_t * const pQueue, const void *pItemToQueue, const BaseType_t Position );

/*--------------------------------------------------------------------------------------*/

QueueHandle_t QueueGenericCreate( const UBaseType_t QueueLength, const UBaseType_t ItemSize, const uint8_t QueueType )
{
    Queue_t *pNewQueue;
    size_t QueueSizeInBytes;
    uint8_t *pQueueStorage;

    if ( ItemSize == ( UBaseType_t )0 )
    {
        QueueSizeInBytes = ( size_t )0;
    }
    else
    {
        QueueSizeInBytes = ( size_t )( ItemSize * QueueLength );
    }

    pNewQueue = ( Queue_t * )pHeapMalloc( sizeof( Queue_t ) + QueueSizeInBytes );
    if ( pNewQueue != NULL )
    {
        pQueueStorage = ( uint8_t * )pNewQueue + sizeof( QueueSizeInBytes );
        prvInitialiseNewQueue( QueueLength, ItemSize, pQueueStorage, QueueType, pNewQueue );
    }

    return pNewQueue;
}
static void prvInitialiseNewQueue( const UBaseType_t QueueLength, const UBaseType_t ItemSize, uint8_t *pQueueStorage, const uint8_t QueueType, Queue_t *pNewQueue )
{
    if ( ItemSize == ( UBaseType_t )0 )
    {
        pNewQueue->pHead = ( uint8_t * )pNewQueue;
    }
    else
    {
        pNewQueue->pHead = ( int8_t * )pQueueStorage;
    }

    pNewQueue->Length = QueueLength;
    pNewQueue->ItemSize = ItemSize;
    pNewQueue->QueueType = QueueType;
    QueueGenericReset( pNewQueue,pdTRUE );
}

void QueueGenericReset( QueueHandle_t Queue, BaseType_t isNewQueue )
{
    Queue_t * const pQueue = ( Queue_t * )Queue;
    PortEnterCritical();
    {
        //数据区最后一个“字节”
        pQueue->pTail = pQueue->pHead + ( pQueue->Length * pQueue->ItemSize );
        pQueue->MessagesWaiting = ( UBaseType_t )0U;
        pQueue->RxLock = queueUNLOCKED;
        pQueue->TxLock = queueUNLOCKED;

        if ( isNewQueue == pdFALSE )
        {
        	//检查有没有任务在等“发送这个队列”,如果有,就唤醒其中优先级最高的那个
            if ( listLIST_IS_EMPTY( pQueue->TasksWaitingToSend ) == pdFALSE )
            {
                if ( TaskRemoveFromEventList( &( pCurrentTCB->EventListItem ) ) != pdFALSE )
                {
                    portREQUEST_TASK_SWITCH();
                }
            }
        }
        else
        {
            //新队列
            ListRemove( &( pQueue->TasksWaitingToSend ) );
            ListRemove( &( pQueue->TasksWaitingToReceive ) );
        }
    }
    PortExitCritical();
}

QueueHandle_t QueueCreateMutex( const uint8_t QueueType )
{
    Queue_t * const pNewQueue;
    const UBaseType_t Length = ( UBaseType_t )1, ItemSize = ( UBaseType_t )0;
    pNewQueue = QueueGenericCreate( Length, ItemSize, QueueType );
    prvInitialiseNewMutex( pNewQueue );
    return pNewQueue;
}
static void prvInitialiseNewMutex( Queue_t * const pNewQueue )
{
    pNewQueue->pMutexHolder = NULL;
    pNewQueue->u.RecursiveCallCount = 0;
    QueueGenericSend( pNewQueue, NULL, ( TickType_t )0, queueSEND_TO_BACK );
}

QueueHandle_t QueueCreateCountingSemaphore( const UBaseType_t MaxCount, const UBaseType_t InitialCount )
{
    QueueHandle_t Handle;

    Handle = QueueGenericCreate( MaxCount, ( UBaseType_t )0, queueQUEUE_TYPE_COUNTING_SEMAPHORE );
    if( Handle != NULL )
    {
        ( ( Queue_t * )Handle )->MessagesWaiting = InitialCount;
    }

    return Handle;
}



BaseType_t QueueGenericSend( QueueHandle_t Queue, const void * const ItemToQueue, TickType_t TicksToWait, const BaseType_t CopyPosition )
{
	BaseType_t EntryTimeSet = pdFALSE,YieldRequest;
    Queue_t *pQueue = ( Queue_t * )Queue;
	TimeOut_t TimeOut;
    for ( ;; )
    {
        PortEnterCritical();
        {
            //队列未满
            if ( pQueue->MessagesWaiting < pQueue->Length || CopyPosition == queueOVERWRITE )
            {
            	//是否解锁了更高优先级的任务
            	YieldRequest = prvCopyDataToQueue( pQueue, ItemToQueue, CopyPosition );

                if ( listLIST_IS_EMPTY( &( pQueue->TasksWaitingToReceive ) ) == pdFALSE )
                {
					if ( TaskRemoveFromEventList( &( pQueue->TasksWaitingToReceive) ) != pdFALSE )
					{
						portREQUEST_TASK_SWITCH();
					}
					else if ( YieldRequest == pdTRUE )
					{
						portREQUEST_TASK_SWITCH();
					}
                }
				PortExitCritical();
				return pdPASS;
            }
            else
            {
				//超时或未设置超时时间
				if ( TicksToWait == ( TickType_t ) 0 )
				{
					PortExitCritical();
					return errQUEUE_FULL;
				}
				else if ( EntryTimeSet pdFALSE )
				{
					//队列已满,这时开始等待,记录开始等待的时间
					EntryTimeSet = pdTRUE;
					TaskInternalSetTimeOutState( &TimeOut );
				}
            }
        }
		PortExitCritical();

		TaskSuspendAll();
		prvLockQueue( pQueue );

		if( TaskCheckForTimeOut( &TimeOut, &TicksToWait ) == pdFALSE )
		{
			//重新检查队列是否满
			if ( prvIsQueueFull( pQueue) != pdFALSE )
			{
                //设置任务归属
				TaskPlaceOnEventList( &( pQueue->TasksWaitingToSend ), TicksToWait );
                prvUnlockQueue( pQueue );
                //防止当前任务占用CPU
                if( TaskResumeAll() == pdFALSE )
				{
					portREQUEST_TASK_SWITCH();
				}
            }
			else
			{
				prvUnlockQueue( pQueue );
				TaskResumeAll();
			}
		}
		else
		{
			//超时
            prvUnlockQueue( pQueue );
			TaskResumeAll();
            return errQUEUE_FULL;
		}

    }
}
static BaseType_t prvIsQueueFull( const Queue_t *pQueue )
{
    BaseType_t xReturn;

	PortEnterCritical();
	{
		if( pQueue->MessagesWaiting == pQueue->Length )
		{
			xReturn = pdTRUE;
		}
		else
		{
			xReturn = pdFALSE;
		}
	}
	PortExitCritical();

	return xReturn;
}
static BaseType_t prvIsQueueEmpty( const Queue_t *pQueue )
{
    BaseType_t xReturn;

	PortEnterCritical();
	{
		if( pQueue->MessagesWaiting == ( UBaseType_t )0 )
		{
			xReturn = pdTRUE;
		}
		else
		{
			xReturn = pdFALSE;
		}
	}
	PortExitCritical();

	return xReturn;
}
static void prvLockQueue( Queue_t * const pQueue )
{
	PortEnterCritical();
	{
		if( ( pQueue )->RxLock == queueUNLOCKED )
		{
			( pQueue )->RxLock = queueLOCKED_UNMODIFIED;
		}
		if( ( pQueue )->TxLock == queueUNLOCKED )
		{
			( pQueue )->TxLock = queueLOCKED_UNMODIFIED;
		}
	}
	PortExitCritical();
}
static void prvUnlockQueue( Queue_t * const pQueue )
{
    PortEnterCritical();
	{
        //在队列被锁住期间,有多少次“发送到队列”的行为发生
		int8_t TxLock = pQueue->TxLock;
        while ( TxLock > queueLOCKED_UNMODIFIED )
        {
            //检查有没有任务在等“发送这个队列”,如果有,就唤醒其中优先级最高的那个
            if ( listLIST_IS_EMPTY( pQueue->TasksWaitingToReceive ) == pdFALSE )
            {
                if ( TaskRemoveFromEventList( &( pCurrentTCB->EventListItem ) ) != pdFALSE )
                {
                    YieldPending = pdTRUE;
                }
                else
                {
                    break;
                }
            }
            --TxLock;
        }
        pQueue->TxLock = queueUNLOCKED;
	}
	PortExitCritical();

    PortEnterCritical();
	{
    	int8_t RxLock = pQueue->RxLock;
        while ( RxLock > queueLOCKED_UNMODIFIED )
        {
            //检查有没有任务在等“发送这个队列”,如果有,就唤醒其中优先级最高的那个
            if ( listLIST_IS_EMPTY( pQueue->TasksWaitingToSend ) == pdFALSE )
            {
                if ( TaskRemoveFromEventList( &( pCurrentTCB->EventListItem ) ) != pdFALSE )
                {
                    YieldPending = pdTRUE;
                }
                else
                {
                    break;
                }
            }
            --RxLock;
        }
        pQueue->RxLock = queueUNLOCKED;
	}
	PortExitCritical();
}

static BaseType_t prvCopyDataToQueue( Queue_t * const pQueue, const void *pItemToQueue, const BaseType_t Position )
{
    UBaseType_t xReturn = pdFALSE;
    UBaseType_t MessagesWaiting;
    MessagesWaiting = pQueue->MessagesWaiting;
    
    if ( pQueue->ItemSize == ( UBaseType_t )0 )
    {
    	if( pQueue->QueueType == queueQUEUE_TYPE_MUTEX )
		{
	        //解除优先级继承
	        xReturn = TaskPriorityDisinherit( ( void * )pQueue->pMutexHolder );
	        pQueue->pMutexHolder = NULL; 
    	}
    }
    else if ( Position == queueSEND_TO_BACK )
    {
        ( void )memcpy( ( void * )pQueue->pWriteTo, pItemToQueue, ( size_t )pQueue->ItemSize );
        //指针校准
        pQueue->pWriteTo += pQueue->ItemSize;
        if ( pQueue->pWriteTo >= pQueue->pTail )
        {
            /* 注意:因为是环形队列,头尾指针不可变,所以不需要 pQueue->pTail = pQueue->pWriteTo,
            队列的物理存储是一段连续内存(线性),逻辑上是一个环,所以指针到达末尾需要转到头 */
            pQueue->pWriteTo = pQueue->pHead;
        }
    }
    else
    {
        ( void )memcpy( ( void * )pQueue->u.pReadFrom, pItemToQueue, ( size_t )pQueue->ItemSize );
        //指针校准
        pQueue->u.pReadFrom -= pQueue->ItemSize;
        if ( pQueue->pWriteTo < pQueue->pHead )
        {
            /* 同上:指针偏移到头指针前面,需要进行校准 */
            pQueue->u.pReadFrom = pQueue->pTail - pQueue->ItemSize;
        }

        if ( Position == queueOVERWRITE )
        {
            --MessagesWaiting;
        }
    }
    pQueue->MessagesWaiting = MessagesWaiting + ( UBaseType_t )1;

    return xReturn;
}
BaseType_t QueueReceive( QueueHandle_t Queue, void * const pBuffer, TickType_t TicksToWait )
{
    BaseType_t EntryTimeSet = pdFALSE;
    TimeOut_t TimeOut;
    Queue_t * const pQueue = ( Queue_t * )Queue;

    for ( ;; )
    {
        const BaseType_t MessagesWaiting = pQueue->MessagesWaiting;
        PortEnterCritical();
        {
            if ( MessagesWaiting > ( UBaseType_t )0 )
            {
                prvCopyDataFromQueue( pQueue, pBuffer );
                pQueue->MessagesWaiting = MessagesWaiting - ( UBaseType_t )1;

                //P操作完成一次,可以进行一次V操作
                if ( listLIST_IS_EMPTY( &( pQueue->TasksWaitingToSend ) ) == pdFALSE )
                {
					if ( TaskRemoveFromEventList( &( pQueue->TasksWaitingToSend ) ) != pdFALSE )
					{
						portREQUEST_TASK_SWITCH();
					}
                }
                PortExitCritical();
                return pdPASS;
            }
            else
            {
                //没有资源可以使用
                if( TicksToWait == ( TickType_t )0 )
				{
                    PortExitCritical();
					return errQUEUE_EMPTY;
				}
				else if( EntryTimeSet == pdFALSE )
				{
					TaskInternalSetTimeOutState( &TimeOut );
					EntryTimeSet = pdTRUE;
				}
				else
				{
				}
            }
        }
        PortExitCritical();

        TaskSuspendAll();
        prvLockQueue( pQueue );

        if( TaskCheckForTimeOut( &TimeOut, &TicksToWait ) == pdFALSE )
		{
			//重新检查队列是否满
			if ( prvIsQueueEmpty( pQueue ) != pdFALSE )
			{
                //设置任务归属
				TaskPlaceOnEventList( &( pQueue->TasksWaitingToReceive ), TicksToWait );
                prvUnlockQueue( pQueue );
                //防止当前任务占用CPU
                if( TaskResumeAll() == pdFALSE )
				{
					portREQUEST_TASK_SWITCH();
				}
            }
			else
			{
				prvUnlockQueue( pQueue );
				TaskResumeAll();
			}
		}
		else
		{
			//超时
            prvUnlockQueue( pQueue );
			TaskResumeAll();
            if( IsQueueEmpty( pQueue ) != pdFALSE )
			{
				return errQUEUE_EMPTY;
			}
		}

    }

}
static void prvCopyDataFromQueue( Queue_t * const pQueue, void * const pBuffer )
{
    /*
      pcHead ---------------------------->  pcTail
        | item0 | item1 | item2 | ... | itemN |
    */
    if( pQueue->ItemSize != ( UBaseType_t )0 )
	{
        pQueue->u.pReadFrom += pQueue->ItemSize;
        /* 先偏移再拷贝,pcReadFrom 指向的是“上一次已经读过的元素位置” 
        偏移一个数据段大小.
        */
        if( pQueue->u.pReadFrom >= pQueue->pTail )
        {
            pQueue->u.pReadFrom = pQueue->pHead;
        }
        ( void )memcpy( ( void * )pBuffer, ( void * )pQueue->u.pReadFrom, ( size_t )pQueue->ItemSize );
    }
    
}

void QueueDelete( QueueHandle_t Queue )
{
    Queue_t * const pQueue = ( Queue_t * )Queue;
    //QueueUnregisterQueue( pQueue );
    vHeapFree( pQueue );
}

void QueueAddToRegistry( QueueHandle_t Queue, const char *pQueueName )
{
    UBaseType_t ux;
    for( ux = ( UBaseType_t )0U; ux < ( UBaseType_t )queueREGISTRY_SIZE; ux++ )
    {
        if( QueueRegistry[ ux ].pQueueName == NULL )
        {
            QueueRegistry[ ux ].pQueueName = pQueueName;
            QueueRegistry[ ux ].Handle = Queue;
            break;
        }
    }
}
void QueueUnregisterQueue( QueueHandle_t Queue )
{
    UBaseType_t ux;
    for( ux = ( UBaseType_t )0U; ux < ( UBaseType_t )queueREGISTRY_SIZE; ux++ )
    {
        if( QueueRegistry[ ux ].Handle == Queue )
        {
            QueueRegistry[ ux ].pQueueName = NULL;
            QueueRegistry[ ux ].Handle = ( QueueHandle_t )0;
            break;
        }
    }
}

void QueueWaitForMessageRestricted( QueueHandle_t Queue, TickType_t TicksToWait, const BaseType_t WaitIndefinitely )
{
	Queue_t * const pQueue = ( Queue_t * )Queue;

	prvLockQueue( pQueue );
	if( pQueue->MessagesWaiting == ( UBaseType_t ) 0U )
	{
		TaskPlaceOnEventListRestricted( &( pQueue->TasksWaitingToReceive ), TicksToWait, WaitIndefinitely );
	}
	prvUnlockQueue( pQueue );
}


