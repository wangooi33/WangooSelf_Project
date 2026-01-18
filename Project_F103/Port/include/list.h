/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _LIST_H
#define _LIST_H
/* Includes ------------------------------------------------------------------*/
#include "portmacro.h"
/* macro ---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/* enum ----------------------------------------------------------------------*/

/* types ---------------------------------------------------------------------*/

struct LIST_ITEM
{
	TickType_t ItemValue;			/* 排序权值 */
	struct LIST_ITEM *pNext;
	struct LIST_ITEM *pPrevious;
	void * pOwner;					/* TCB */
	void * pContainer;				/* 挂载的链表 */
};
typedef struct LIST_ITEM ListItem_t;

struct MINI_LIST_ITEM
{
	TickType_t ItemValue;
	struct LIST_ITEM *pNext;
	struct LIST_ITEM *pPrevious;
};
typedef struct MINI_LIST_ITEM MiniListItem_t;

typedef struct LIST
{
	volatile UBaseType_t NumberOfItems;			/* 当前链表中真实节点数,不包含 ListEnd */
	ListItem_t *pIndex;							/* 链表遍历时的当前位置指针 */
	MiniListItem_t ListEnd;						/* 哨兵结点 */
} List_t;


/* constants -----------------------------------------------------------------*/


/* global variable -----------------------------------------------------------*/


/* functions prototypes ------------------------------------------------------*/

#define  listIS_EMPTY( pList )                      ( ( BaseType_t )( ( pList )->NumberOfItems == 0 ) )

#define  listSET_LISTITEM_VALUE( pListItem,value )  ( ( pListItem )->ItemValue = value )
#define  listSET_LISTITEM_OWNER( pListItem,owner )  ( ( pListItem )->pOwner = ( void * )owner )

#define  listGET_LISTITEM_VALUE( pListItem )        ( ( pListItem )->ItemValue )
#define  listGET_LISTITEM_OWNER( pListItem )        ( ( pListItem )->pOwner )
#define  listGET_LISTITEM_CONTAINER( pListItem )    ( ( pListItem )->pContainer )
#define  listGET_ENDNEXT_LISTITEM_OWNER( pList )    ( ( &( ( pList )->ListEnd ) )->pNext->pOwner )
#define  listGET_CURRENTLIST_LENTH( pList )         ( ( pList )->NumberOfItems )
//跳过哨兵节点，把下一个 TCB 拿出来
#define listGET_OWNER_OF_NEXT_ENTRY( pTCB, pList )											\
{																							\
	List_t * const pConstList = ( pList );													\
	( pConstList )->pIndex = ( pConstList )->pIndex->pNext;									\
	if ( ( void * ) ( pConstList )->pIndex == ( void * ) &( ( pConstList )->ListEnd ) )		\
	{																						\
		( pConstList )->pIndex = ( pConstList )->pIndex->pNext;								\
	}																						\
	( pTCB ) = ( pConstList )->pIndex->pOwner;												\
}


/**
  * @function name: 
  * @brief        : 
  * @param        : 
  * @retval       : 
  * @version      : 
  * @note         : 
*/
void ListInitialise( List_t * const pList );
void ListInitialiseItem( ListItem_t * const pItem );
void ListInsert( List_t * const pList, ListItem_t * const pNewListItem );
void ListInsertCurPrevious( List_t * const pList, ListItem_t * const pNewListItem );
UBaseType_t ListRemove( ListItem_t * const pListItemToRemove );




#ifdef __cplusplus
}
#endif

#endif /* _LIST_H */




