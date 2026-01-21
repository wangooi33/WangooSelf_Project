#include <stdlib.h>
#include "portmacro.h"
#include "task.h"

typedef struct Free_BLOCK
{
	struct Free_BLOCK *pNextFreeBlock;
	size_t BlockSize;	//要算上结构体大小
} BlockLink_t;
/*------------------------------------------------------------------*/

#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( HeapStructSize << 1 ) )

/*------------------------------------------------------------------*/

static uint8_t Heap[ portHEAP_MAXSIZE ];
static size_t FreeBytesRemain = 0U;					/* 剩余的字节数 */
static size_t BlockAllocatedBit = 0;				/* 块分配位掩码 */

/* xStart 的“地址和值”在整个系统运行期间都不变;
   pxEnd  的“指向哪个地址”只在堆初始化时确定,之后也不再变。*/
static BlockLink_t HeapStart,*pHeapEnd = NULL;
static const size_t HeapStructSize = ( sizeof( BlockLink_t ) + ( ( size_t )( portBYTE_ALIGNED - 1 ) ) ) & ~( ( size_t )( portBYTE_ALIGNED - 1 ) );

/*------------------------------------------------------------------*/
static void HeapInit(void);
static void InsertBlockIntoFreeList( BlockLink_t *pBlockToInsert );
/*------------------------------------------------------------------*/

void *pHeapMalloc( size_t WantedSize )
{
	BlockLink_t *pBlock, *pPreviousBlock, *pNewBlockLink;
	void *pReturn = NULL;

	TaskSuspendAll();
	{
		//1.初始化
		if ( pHeapEnd == NULL )
		{
			HeapInit();
		}

		//2.牺牲WantedSize最高位, 用作空闲标志位(0:表示空闲)
		if ( ( WantedSize & BlockAllocatedBit ) == 0 )
		{
			if ( WantedSize > 0 )
			{
				//计算实际占用空间
				WantedSize += HeapStructSize;
				if ( ( WantedSize & portBYTE_ALIGNED_MASK ) != 0x00 )
				{
					//让空间补足到8字节的整数倍
					WantedSize += portBYTE_ALIGNED - ( WantedSize & portBYTE_ALIGNED_MASK );
				}
			}

			//3.分配链表项
			if ( ( WantedSize > 0 ) && ( WantedSize <= FreeBytesRemain ) )
			{
				//遍历
				pPreviousBlock = &HeapStart;
				pBlock = HeapStart.pNextFreeBlock;
				while ( ( pBlock->pNextFreeBlock != NULL ) && ( pBlock->BlockSize < WantedSize ) )
				{
					pPreviousBlock = pBlock;
					pBlock = pBlock->pNextFreeBlock;
				}
				if ( pBlock != pHeapEnd )
				{
					//返回“空闲块头”后的“数据区地址”,按字节计算需要强转
					pReturn = ( void * )( ( ( uint8_t * )pPreviousBlock->pNextFreeBlock ) + HeapStructSize );
					//移出空闲链表
					pPreviousBlock->pNextFreeBlock = pBlock->pNextFreeBlock;
					//拆分
					if ( ( pBlock->BlockSize ) - WantedSize > heapMINIMUM_BLOCK_SIZE )
					{
						pNewBlockLink = ( void * )( ( uint8_t * )pBlock + WantedSize );
						pNewBlockLink->BlockSize =  ( pBlock->BlockSize ) - WantedSize;
						pBlock->BlockSize = WantedSize;
						InsertBlockIntoFreeList( pNewBlockLink );		//拆分后的空闲块插入链表
					}
				}
				FreeBytesRemain -= WantedSize;
				pBlock->BlockSize |= BlockAllocatedBit;	//已分配
				pBlock->pNextFreeBlock = NULL;
			}
		}
	}
	TaskResumeAll();
	
	return pReturn;
}
void vHeapFree( void * pDel )
{
	/*	指针类型转换:
		| BlockLink_t | 用户数据 |
		^			  ^
		|			  |
		pLink		  pDel
	*/
	BlockLink_t *pLink;
	uint8_t *pData = ( uint8_t * )pDel;

	pData -= HeapStructSize;	//按字节回退,需强转类型
	pLink = ( void * )pData;	//(uint8_t*) → (void*)防报错 → (BlockLink_t*)
	
	if ( ( pLink->BlockSize & BlockAllocatedBit ) != 0 )
	{
		if ( pLink->pNextFreeBlock != NULL )
		{
			pLink->BlockSize &= ~BlockAllocatedBit;
			
			TaskSuspendAll();
			{
				FreeBytesRemain += pLink->BlockSize;
				InsertBlockIntoFreeList( ( BlockLink_t * )pLink );
			}
			TaskResumeAll();
		}
	}		
}
void StackOverflowHook(TaskHandle_t Task,char *pTaskName)
{
	
}

/*------------------------------------------------------------------*/

static void HeapInit( void )
{
	BlockLink_t *pFirstFreeBlock;
	uint8_t *AlignedHeap;
	size_t Address;
	size_t HeapRealSize = portHEAP_MAXSIZE;

	Address = ( size_t )Heap;

	/* 如果堆起始地址没有按要求对齐,就把它“推到下一个对齐边界”,并相应缩小可用堆大小,确保后续内存全部对齐 */
	if ( ( Address & portBYTE_ALIGNED_MASK ) != 0 )
	{
		Address += ( portBYTE_ALIGNED - 1 );
		Address &= ~( ( size_t )( portBYTE_ALIGNED - 1 ) );		//向下对齐(低地址)
		HeapRealSize -= Address - ( size_t )Heap;
	}
	//堆起始地址
	AlignedHeap = ( uint8_t * )Address;


	HeapStart.pNextFreeBlock = ( void * )AlignedHeap;
	HeapStart.BlockSize = ( size_t )0;

	Address = ( ( size_t )AlignedHeap ) + HeapRealSize;
	Address -= HeapStructSize;
	Address &= ~( ( size_t )portBYTE_ALIGNED_MASK );			//portBYTE_ALIGNED_MASK == portBYTE_ALIGNED - 1
	pHeapEnd = ( void * )Address;
	pHeapEnd->BlockSize = 0;
	pHeapEnd->pNextFreeBlock = NULL;

	pFirstFreeBlock = ( void * )AlignedHeap;
	pFirstFreeBlock->BlockSize = Address - ( size_t )pFirstFreeBlock;
	pFirstFreeBlock->pNextFreeBlock = pHeapEnd;

	FreeBytesRemain = pFirstFreeBlock->BlockSize;
	BlockAllocatedBit = ( ( size_t )1 ) << ( ( sizeof( size_t ) * 8 ) - 1 );		//位掩码校准(左移4 * 8位)
}

static void InsertBlockIntoFreeList( BlockLink_t *pBlockToInsert )
{
	BlockLink_t *pIterator;
	uint8_t *pt;
		
	for ( pIterator = &HeapStart ; pIterator->pNextFreeBlock < pBlockToInsert ; pIterator = pIterator->pNextFreeBlock )
	{
		/*
			地址升序:
		   _____________________________________	  ___________________________
		   |pIterator   --->   (pBlockToInsert)|  ——> |pIterator->pNextFreeBlock| 
		   —————————————————————————————————————	  ———————————————————————————
		*/
	}

	/* 向下(低地址)合并:pBlockToInsert紧挨pIterator块 */
	pt = ( uint8_t * )pIterator;//让“地址加法”按“字节”为单位进行,而不是按结构体大小,需要强转
	if ( pt + pIterator->BlockSize == ( uint8_t * )pBlockToInsert )
	{
		pIterator->BlockSize += pBlockToInsert->BlockSize;
		pBlockToInsert = pIterator;
	}

	/* 向上(高地址)合并 */
	pt = ( uint8_t * )pBlockToInsert;
	if ( pt + pBlockToInsert->BlockSize == ( uint8_t * )pIterator->pNextFreeBlock )
	{
		if ( pIterator->pNextFreeBlock != pHeapEnd )
		{
			pBlockToInsert->BlockSize +=  pIterator->BlockSize;
			pBlockToInsert->pNextFreeBlock = pIterator->pNextFreeBlock->pNextFreeBlock;
		}
		else
		{
			pBlockToInsert->pNextFreeBlock = pHeapEnd;
		}
	}
	else
	{
		pBlockToInsert->pNextFreeBlock = pIterator->pNextFreeBlock;
	}
	
	/* 未进行过向下合并 */
	if ( pBlockToInsert != pIterator )
	{
		pIterator->pNextFreeBlock = pBlockToInsert;
	}
}

