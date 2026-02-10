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

#define listSET_LIST_ITEM_OWNER( pListItem, pOwner )	( ( pListItem )->pOwner = ( void * )( pOwner ) )
#define listGET_LIST_ITEM_OWNER( pListItem )			( ( pListItem )->pOwner )

#define listSET_LIST_ITEM_VALUE( pListItem, Value )		( ( pListItem )->ItemValue = ( Value ) )
#define listGET_LIST_ITEM_VALUE( pListItem )			( ( pListItem )->ItemValue )

#define listGET_ITEM_VALUE_OF_HEAD_ENTRY( pList )		( ( ( pList )->ListEnd ).pNext->ItemValue )


#define listLIST_IS_EMPTY( pList )						( ( BaseType_t )( ( pList )->NumberOfItems == ( UBaseType_t )0 ) )

#define listCURRENT_LIST_LENGTH( pList )				( ( pList )->NumberOfItems )

//跳过ListEnd节点,找打到下一个TCB
#define listGET_OWNER_OF_NEXT_ENTRY( pTCB, pList )											\
{																							\
	List_t * const pConstList = ( pList );													\
	( pConstList )->pIndex = ( pConstList )->pIndex->pNext;									\
	if ( ( void * )( pConstList )->pIndex == ( void * )&( ( pConstList )->ListEnd ) )		\
	{																						\
		( pConstList )->pIndex = ( pConstList )->pIndex->pNext;								\
	}																						\
	( pTCB ) = ( pConstList )->pIndex->pOwner;												\
}

#define listGET_OWNER_OF_HEAD_ENTRY( pList )  			( (&( ( pList )->ListEnd ))->Next->pOwner )

#define listIS_CONTAINED_WITHIN( pList, pListItem ) 	( ( BaseType_t )( ( pListItem )->pContainer == ( void * )( pList ) ) )

#define listLIST_ITEM_CONTAINER( pListItem ) 			( ( pListItem )->pContainer )


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




