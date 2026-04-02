#ifndef _QUEUE_H
#define _QUEUE_H

#include "list.h" 

#define pdFALSE   (0)
#define pdTRUE    (1)

typedef void* QueueHandle_t;

typedef struct Queue
{
    uint8_t *pHead;
    uint8_t *pTail;
    uint8_t *pWriteTo;  //下一个要写入的地址.先拷贝数据,再移动指针(队列秉承先进先出的原则,即从一端写入数据)
    uint8_t *pReadFrom; //上一次被读取的地址.先移动指针,再拷贝数据(从另一端读数据)
    ListManager_t TasksWaitingToSend;   //当队列已满时,尝试发送的任务会阻塞在此
    ListManager_t TasksWaitingToReceive;
    volatile uint32_t NumOfResources;
    uint32_t MaxLength;
    uint32_t ItemSize;
    uint8_t QueueType;
}Queue_t;


//入队到尾部（FIFO:先进先出）
#define	queueSEND_TOBACK		(0xF0)
//入队到头部,类似“高优先级消息插队”
#define	queueSEND_TOFRONT		(0xF1)
//只允许用于长度为 1 的队列,队列满时:覆盖已有元素
#define queueSEND_OVERWRITE	    (0xF2)

#define queueTYPE_QUEUE                 (0xA0)
#define queueTYPE_BINARY_SEMAPHORE      (0xA1)
#define queueTYPE_COUNTING_SEMAPHORE    (0xA2)

void AsmRaiseBasepri( void );
void AsmSetBasepri( void );
QueueHandle_t QueueCreate( uint32_t QueueLength,uint32_t ItemSize );
QueueHandle_t BinarySemaphoreCreate();
QueueHandle_t CountingSemaphoreCreate( uint16_t MaxCount,uint16_t InitialConut );
uint8_t QueueSendToBack( QueueHandle_t Queue, const void * const pItemToQueue );
uint8_t QueueSendToFront( QueueHandle_t Queue, const void * const pItemToQueue );
uint8_t QueueOverWrite( QueueHandle_t Queue, const void * const pItemToQueue );
uint8_t QueueReceive( QueueHandle_t Queue, void * const pBuffer );



#endif /* _QUEUE_H */
