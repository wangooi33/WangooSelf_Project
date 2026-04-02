#include "queue.h"
#include "memory.h"

/**
 * @类型:
 *  1.队列
 *  2.二值信号量(任务间的同步:数值变化时用作通知)
 *  3.计数信号量(P\V操作)
 *  4.互斥量(任务间的互斥:确保同一时间只有一个任务访问该资源)
 *  5.递归互斥量(每次获取+1,释放-1。=0时其他任务才能获取,防止自死锁)
 *
 * @注意:
 *  1.无论是二值信号量还是计数信号量,只占用一个结构体大小的内存,无数据区
 *  2.在引入优先级后,互斥量才有实际作用。
 */

static void prvInitialiseNewQueue( const uint32_t QueueLength, const uint32_t ItemSize, const uint8_t QueueType, uint8_t *pQueueDataArea, Queue_t *pNewQueue );
QueueHandle_t QueueGenericCreate( const uint32_t QueueLength, const uint32_t ItemSize, const uint8_t QueueType );
uint8_t QueueGenericSend( QueueHandle_t Queue, const void * const pItemToQueue, const uint8_t CopyPosition );
static void prvCopyDataToQueue( Queue_t * const pQueue, const void *pItemToQueue, const uint8_t Position );
static void prvCopyDataFromQueue( Queue_t * const pQueue, void * const pBuffer );


void AsmRaiseBasepri( void )
{
    uint32_t LimitValue = 5;
    _asm
    {
        msr basepri, LimitValue
        dsb
        isb
    }
}
void AsmSetBasepri( void )
{
    _asm
    {
       msr basepri, 0
       dsb
       isb
    }
}

QueueHandle_t QueueGenericCreate( const uint32_t QueueLength, const uint32_t ItemSize, const uint8_t QueueType )
{
    Queue_t *pNewQueue;
    size_t QueueSizeInBytes;
    uint8_t *pQueueDataArea;

    if ( ItemSize == 0 )
    {
        //二值信号量
        QueueSizeInBytes = 0;
    }
    else
    {
        QueueSizeInBytes = (size_t)(ItemSize * QueueLength);
    }
    pNewQueue = (Queue_t *)MemMalloc(sizeof(Queue_t) + QueueSizeInBytes);

    if ( pNewQueue != NULL )
    {
        //指针偏移到数据区
        pQueueDataArea = (uint8_t *)pNewQueue + sizeof(Queue_t);
        prvInitialiseNewQueue(QueueLength,ItemSize,QueueType,pQueueDataArea,pNewQueue);
    }
    return pNewQueue;
}
static void prvInitialiseNewQueue( const uint32_t QueueLength, const uint32_t ItemSize, const uint8_t QueueType, uint8_t *pQueueDataArea, Queue_t *pNewQueue )
{
    if ( ItemSize == 0 )
    {
        pNewQueue->pHead = (uint8_t *)pNewQueue;
    }
    else
    {
        pNewQueue->pHead = (uint8_t *)pQueueDataArea;
    }
    pNewQueue->MaxLength = QueueLength;
	pNewQueue->ItemSize = ItemSize;
    pNewQueue->QueueType = QueueType;

    pNewQueue->pTail = pNewQueue->pHead + (pNewQueue->MaxLength * pNewQueue->ItemSize);//数据区“末尾后一字节”,不是最后一个元素
    pNewQueue->NumOfResources = 0;
    pNewQueue->pWriteTo = pNewQueue->pHead;
    pNewQueue->pReadFrom = pNewQueue->pHead + ((pNewQueue->MaxLength - 1) * pNewQueue->ItemSize);//队列的最后一个元素的起始地址
    ListInitialise(&(pNewQueue->TasksWaitingToReceive));
	ListInitialise(&(pNewQueue->TasksWaitingToSend));
}

QueueHandle_t QueueCreate( uint32_t QueueLength,uint32_t ItemSize )
{
    QueueHandle_t pNewQueue;
    pNewQueue = QueueGenericCreate((QueueLength),(ItemSize),(queueTYPE_QUEUE));
    return pNewQueue;
}
QueueHandle_t BinarySemaphoreCreate()
{
    QueueHandle_t pNewBinarySemaphore;
    pNewBinarySemaphore = QueueGenericCreate((1),(0),(queueTYPE_BINARY_SEMAPHORE));
    return pNewBinarySemaphore;
}
QueueHandle_t CountingSemaphoreCreate( uint32_t MaxCount,uint32_t InitialConut )
{
    QueueHandle_t pNewCountingSemaphore;
    pNewCountingSemaphore = QueueGenericCreate((MaxCount),(0),(queueTYPE_COUNTING_SEMAPHORE));
    if ( pNewCountingSemaphore != NULL)
    {
        ((Queue_t *)pNewCountingSemaphore)->NumOfResources = InitialConut;
    }
    return pNewCountingSemaphore;
}

uint8_t QueueGenericSend( QueueHandle_t Queue, const void * const pItemToQueue, const uint8_t CopyPosition )
{
    //一次仅生产一个资源
    uint8_t xReturn;
    Queue_t *pQueue = (Queue_t *)Queue;
    AsmRaiseBasepri();
    {
        //生产
        if ( pQueue->NumOfResources < pQueue->MaxLength || CopyPosition == queueSEND_OVERWRITE )
        {
            prvCopyDataToQueue(pQueue,pItemToQueue,CopyPosition);
            if ( (pQueue->TasksWaitingToReceive).NumberOfItems != 0 )
            {
                //唤醒阻塞的生产者任务
            }
            xReturn = pdTRUE;
        }
        else
        {
            //队列已满
            xReturn = pdFALSE;
        }
    }
    AsmSetBasepri();
	return xReturn;
}
static void prvCopyDataToQueue( Queue_t * const pQueue, const void *pItemToQueue, const uint8_t Position )
{
    uint32_t NumOfResources;
    NumOfResources = pQueue->NumOfResources;

    //二值信号量
    if ( pQueue->ItemSize == 0 && pQueue->QueueType == queueTYPE_BINARY_SEMAPHORE )
    {
    }

    if ( Position == queueSEND_TOBACK )
    {
        //先拷贝数据,在移动指针.此时,这个指针指向下次的写地址,这个动作是在为下次操作铺路
        (void)memcpy((void *)pQueue->pWriteTo,pItemToQueue,(size_t)pQueue->ItemSize);
        pQueue->pWriteTo += pQueue->ItemSize;
        if ( pQueue->pWriteTo >= pQueue->pTail )
        {
        	//数据并不会写入不属于队列的内存,因为上面已经做出限制if ( pQueue->NumOfResources < pQueue->MaxLength || CopyPosition == queueSEND_OVERWRITE )
            pQueue->pWriteTo = pQueue->pHead;
        }
    }
    else
    {
        (void)memcpy((void *)pQueue->pReadFrom,pItemToQueue,(size_t)pQueue->ItemSize);
        pQueue->pReadFrom -= pQueue->ItemSize;//位置校准
        if ( pQueue->pReadFrom < pQueue->pHead )
        {
            //最后一个元素的起始地址
            pQueue->pReadFrom = (pQueue->pTail - pQueue->ItemSize);
        }

        if ( Position == queueSEND_OVERWRITE )
        {
            if ( NumOfResources > 0 )
			{
				//覆盖掉了,因为pQueue->NumOfResources,下面+1所以这里要-1
				--NumOfResources;
			}
        }
    }

    pQueue->NumOfResources = NumOfResources + 1;
}

uint8_t QueueSendToBack( QueueHandle_t Queue, const void * const pItemToQueue )
{
    uint8_t xReturn = pdFALSE;
    xReturn = QueueGenericSend(Queue,pItemToQueue,queueSEND_TOBACK);
    return xReturn;
}
uint8_t QueueSendToFront( QueueHandle_t Queue, const void * const pItemToQueue )
{
    uint8_t xReturn = pdFALSE;
    xReturn = QueueGenericSend(Queue,pItemToQueue,queueSEND_TOFRONT);
    return xReturn;
}
uint8_t QueueOverWrite( QueueHandle_t Queue, const void * const pItemToQueue )
{
    uint8_t xReturn = pdFALSE;
    xReturn = QueueGenericSend(Queue,pItemToQueue,queueSEND_OVERWRITE);
    return xReturn;
}


uint8_t QueueReceive( QueueHandle_t Queue, void * const pBuffer )
{   
	uint8_t xReturn;
    Queue_t *pQueue = (Queue_t *)Queue;
    uint32_t NumOfResources;
    NumOfResources = pQueue->NumOfResources;

    AsmRaiseBasepri();
    {
        if ( NumOfResources > 0 )
        {
            prvCopyDataFromQueue(pQueue,pBuffer);
            pQueue->NumOfResources = NumOfResources - 1;
            if ( (pQueue->TasksWaitingToSend).NumberOfItems != 0 )
            {
                //唤醒阻塞的消费者任务
            }
            xReturn = pdTRUE;
        }
        else
        {
            xReturn = pdFALSE;
        }
    }
    AsmSetBasepri();
	return xReturn;
}
static void prvCopyDataFromQueue( Queue_t * const pQueue, void * const pBuffer )
{
    if( pQueue->ItemSize != 0 )
	{
		pQueue->pReadFrom += pQueue->ItemSize;
		if( pQueue->pReadFrom >= pQueue->pTail )
		{
			pQueue->pReadFrom = pQueue->pHead;
		}
		(void)memcpy((void *)pBuffer,(void *)pQueue->pReadFrom,(size_t)pQueue->ItemSize);
	}
}


