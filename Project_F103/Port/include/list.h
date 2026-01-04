/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _LIST_H
#define _LIST_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "portmacro.h"
/* macro ---------------------------------------------------------------------*/

/* enum ----------------------------------------------------------------------*/

/* types ---------------------------------------------------------------------*/

struct LIST_ITEM
{
	configLIST_VOLATILE TickType_t ItemValue;/* 排序 */
	struct LIST_ITEM * configLIST_VOLATILE pNext;
	struct LIST_ITEM * configLIST_VOLATILE pPrevious;
	void * pOwner;							/* 定时器任务或者普通任务 */
	void * configLIST_VOLATILE pContainer;	/* 链表索引 */
};
typedef struct LIST_ITEM ListItem_t;

struct MINI_LIST_ITEM
{
	configLIST_VOLATILE TickType_t ItemValue;
	struct LIST_ITEM * configLIST_VOLATILE pNext;
	struct LIST_ITEM * configLIST_VOLATILE pPrevious;
};
typedef struct MINI_LIST_ITEM MiniListItem_t;

typedef struct LIST
{
	volatile UBaseType_t uNumberOfItems;	/* 当前链表中真实节点数,不包含 ListEnd */
	ListItem_t * configLIST_VOLATILE pIndex;/* 链表遍历时的当前位置指针 */
	MiniListItem_t ListEnd;					/* 哨兵结点 */
} List_t;


/* constants -----------------------------------------------------------------*/


/* global variable -----------------------------------------------------------*/


/* functions prototypes ------------------------------------------------------*/

/**
  * @function name: 
  * @brief        : 
  * @param        : 
  * @retval       : 
  * @version      : 
  * @note         : 
*/


#endif /* _LIST_H */




