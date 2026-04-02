#include "memory.h"

//存储在.bss段
uint8_t Heap[memHEAP_SIZE];

//8字节对齐
const size_t HeapStructSize = ( sizeof(Block_t) + (memBYTE_ALIGMENT_MASK - 1) ) & ~(memBYTE_ALIGMENT_MASK - 1);
Block_t *pHeapStart = NULL;
Block_t *pHeapEnd = NULL;
size_t RemainBytes = 0;
//BlockSize的最高位为0,表示空闲
size_t FreeFlagBitMask = 0;

static void prvHeapInit( void );
static void prvBlockInsertFreeList( Block_t *pBlockToInsert );

void* MemMalloc( size_t WantedSize )
{
    Block_t *pPreviousBlock,*pCurrentBlock,*pNewBlock;
    void *pReturn = NULL;

    if ( pHeapEnd == NULL )
    {
        prvHeapInit();
    }

    if( WantedSize > 0 )
	{
        //计算实际占用空间大小
        WantedSize += HeapStructSize;
        if ( (WantedSize & memBYTE_ALIGMENT_MASK) != 0 )
        {
            //向上对齐:五入
            WantedSize = ( WantedSize + (memBYTE_ALIGMENT_MASK - 1) ) & ~(memBYTE_ALIGMENT_MASK - 1);
        }

        if ( WantedSize > 0 && WantedSize <= RemainBytes )
        {
            //遍历堆
            pPreviousBlock = pHeapStart;
            pCurrentBlock = pPreviousBlock->pNextFreeBlock;
            while ( pCurrentBlock->pNextFreeBlock != NULL && pCurrentBlock->BlockSize < WantedSize )
            {
                pPreviousBlock = pCurrentBlock;
                pCurrentBlock = pCurrentBlock->pNextFreeBlock;
            }

            //内存分配
            if ( pCurrentBlock != pHeapEnd )
            {
                //刨除结构体,偏移到数据区地址
                pReturn = (void *)((uint8_t *)pCurrentBlock + HeapStructSize);
                pPreviousBlock->pNextFreeBlock = pCurrentBlock->pNextFreeBlock;
                //块拆分(易产生内存碎片)
                if ( pCurrentBlock->BlockSize - WantedSize >= memHEAP_MIN_SIZE )
                {
                    pNewBlock = (void *)((uint8_t *)pCurrentBlock + WantedSize);
                    pNewBlock->BlockSize = pCurrentBlock->BlockSize - WantedSize;
                    pCurrentBlock->BlockSize = WantedSize;
                    prvBlockInsertFreeList(pNewBlock);
                }
                pCurrentBlock->BlockSize |= FreeFlagBitMask;
                pCurrentBlock->pNextFreeBlock = NULL;
                RemainBytes -= pCurrentBlock->BlockSize;
            }
        }
    }
    return pReturn;
}
void MemFree( void *p )
{
    Block_t *pBlock;

    if ( p != NULL )
    {
        //p指向块的数据区,回收时需向下偏移一个结构体的大小
        pBlock = (void *)((uint8_t *)p - HeapStructSize);
        if ( pBlock->BlockSize & FreeFlagBitMask )
        {
            //分配出去的块不在空闲块链表中,pxNextFreeBlock应该为NULL
            if ( pBlock->pNextFreeBlock == NULL )
            {
                //置位
                pBlock->BlockSize &= ~FreeFlagBitMask;
                prvBlockInsertFreeList(pBlock);
            }
        }
    }
}

static void prvHeapInit( void )
{
    /* 对齐方向: ->  <-  */
    size_t Address = (size_t)Heap;
    size_t TotalHeapSize = memHEAP_SIZE;

    //堆实际的起始地址
    if ( (Address & memBYTE_ALIGMENT_MASK) != 0 )
    {
        //向上对齐:x = (x + (A - 1)) & ~(A - 1)   ->
        Address = ( Address + (memBYTE_ALIGMENT_MASK - 1) ) & ~(memBYTE_ALIGMENT_MASK - 1);
        //削减后堆的大小
        TotalHeapSize = TotalHeapSize - (Address - (size_t)Heap);
    }
	pHeapStart = (Block_t *)Address;
    pHeapStart->BlockSize = 0;
    pHeapStart->pNextFreeBlock = (void *)(Address + HeapStructSize);

    //堆尾地址
    Address = (size_t)pHeapStart + TotalHeapSize;
    Address -= HeapStructSize;
    //向下对齐:x = x & ~(A - 1)    <-
    Address &= ~( memBYTE_ALIGMENT_MASK - 1 );
    pHeapEnd = (void *)Address;
	pHeapEnd->pNextFreeBlock = NULL;

    RemainBytes = (size_t)pHeapEnd - (size_t)pHeapStart->pNextFreeBlock;
    pHeapStart->pNextFreeBlock->BlockSize = RemainBytes;
    pHeapStart->pNextFreeBlock->pNextFreeBlock = pHeapEnd;

    FreeFlagBitMask = ((size_t)1) << (sizeof(size_t) * 8 - 1);
}
static void prvBlockInsertFreeList( Block_t *pBlockToInsert )
{
    Block_t *pIterator;
    for ( pIterator = pHeapStart; pIterator->pNextFreeBlock < pBlockToInsert; pIterator = pIterator->pNextFreeBlock )
    {
    	//按地址排序
    }
    //块合并(向前、向后)
    if ( (uint8_t *)pBlockToInsert == (uint8_t *)pIterator + (pIterator->BlockSize) )
    {
        pBlockToInsert->BlockSize += pIterator->BlockSize;
        pBlockToInsert = pIterator;
    }
    //块合并(向后)
    if ( (uint8_t *)pBlockToInsert + (pBlockToInsert->BlockSize) == pIterator->pNextFreeBlock )
    {
        if ( pIterator->pNextFreeBlock != pHeapEnd )
        {
            pBlockToInsert->BlockSize += pIterator->pNextFreeBlock->BlockSize;
            pBlockToInsert->pNextFreeBlock = pIterator->pNextFreeBlock->pNextFreeBlock;
        }
        else
        {
            pBlockToInsert->pNextFreeBlock = pHeapEnd;
        }
    }
    
    if ( pIterator != pBlockToInsert )
    {
        //没有进行过向前合并
        pIterator->pNextFreeBlock = pBlockToInsert;
    }
}

