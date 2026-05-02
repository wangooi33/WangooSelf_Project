#ifndef _MEMORY_C
#define _MEMORY_C

#include <stdio.h>

#define memHEAP_SIZE             ((size_t)(10 * 1024))
#define memBYTE_ALIGMENT_MASK    (0x0007)
#define memHEAP_MIN_SIZE         ((size_t)(5 * HeapStructSize))

typedef struct FreeBlock 
{
    struct FreeBlock *pNextFreeBlock;
    size_t BlockSize;
}Block_t;

extern size_t RemainBytes;

void* MemMalloc( size_t WantedSize );
void MemFree( void *p );

#endif /* _MEMORY_C */