#include "portmacro.h"

typedef struct Free_BLOCK
{
	struct Free_BLOCK *pNextFreeBlock;
	size_t BlockSize;
} BlockLink_t;
/*------------------------------------------------------------------*/

static uint8_t Heap[portHEAP_MAXSIZE];
static size_t FreeBytesRemain = 0U;			/* 剩余的字节数 */
static size_t BlockAllocatedBit = 0;		/* 位掩码 */

static BlockLink_t *pHeapStart,*pHeapEnd = NULL;
static const size_t BlockStructSize = ( sizeof ( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNED - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNED - 1 );

/*------------------------------------------------------------------*/

static void HeapInit(void)
{
	BlockLink_t *pFirstFreeBlock;
	uint8_t *AlignedHeap;
	size_t Address;
	size_t HeapRealSize = portHEAP_MAXSIZE;

	Address = ( size_t ) Heap;

	/* 如果堆起始地址没有按要求对齐,就把它“推到下一个对齐边界”,并相应缩小可用堆大小,确保后续内存全部对齐 */
	if( ( Address & portBYTE_ALIGNED_MASK ) != 0 )
	{
		Address += ( portBYTE_ALIGNED - 1 );
		Address &= ~( ( size_t ) portBYTE_ALIGNED - 1 );		//向下对齐(低地址)
		HeapRealSize -= Address - ( size_t ) Heap;
	}

	/* 堆起始地址 */
	AlignedHeap = ( uint8_t * ) Address;

	/* Heap Start */
	pHeapStart.pNextFreeBlock = ( void * ) AlignedHeap;
	pHeapStart.BlockSize = ( size_t ) 0;
	/* Heap End */
	Address = ( ( size_t ) AlignedHeap ) + HeapRealSize;
	Address -= BlockStructSize;
	Address &= ~( ( size_t ) portBYTE_ALIGNED_MASK );		 //portBYTE_ALIGNED_MASK == portBYTE_ALIGNED - 1
	pHeapEnd = ( void * ) Address;
	pHeapEnd->BlockSize = 0;
	pHeapEnd->pNextFreeBlock = NULL;

	pFirstFreeBlock = ( void * ) AlignedHeap;
	pFirstFreeBlock->BlockSize = Address - ( size_t ) pFirstFreeBlock;
	pFirstFreeBlock->pNextFreeBlock = pHeapEnd;

	FreeBytesRemain = pFirstFreeBlock->BlockSize;

	BlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * 8 ) - 1 );		//左移4 * 8位
}


