#include "list.h"

void ListInitialise( List_t * const pList )
{
	pList->pIndex = ( ListItem_t * )&( pList->ListEnd );
	pList->NumberOfItems = ( UBaseType_t )0U;
	
	pList->ListEnd.ItemValue = portMAX_DEALY;
	pList->ListEnd.pNext = ( ListItem_t * )&( pList->ListEnd );
	pList->ListEnd.pPrevious = ( ListItem_t * )&( pList->ListEnd );
}

void ListInitialiseItem( ListItem_t * const pItem )
{
	pItem->pContainer = NULL;
}

/* 链表升序排列:ListEnd -> value最小 -> ... -> value最大 -> ListEnd */
void ListInsert( List_t * const pList, ListItem_t * const pNewListItem )
{
	ListItem_t *pIterator;//迭代器
	const TickType_t ValueOfInsertion = pNewListItem->ItemValue;

	if ( ValueOfInsertion == portMAX_DEALY )
	{
		pIterator = pList->ListEnd.pPrevious;
	}
	else
	{
		for( pIterator = ( ListItem_t * ) &( pList->ListEnd ); pIterator->pNext->ItemValue <= ValueOfInsertion; pIterator = pIterator->pNext )
		{
			/* There is nothing to do here. */
		}
	}

	pNewListItem->pNext = pIterator->pNext;
	pNewListItem->pNext->pPrevious = pNewListItem;
	pNewListItem->pPrevious = pIterator;
	pIterator->pNext = pNewListItem;


	pNewListItem->pContainer = ( void * ) pList;

	( pList->NumberOfItems )++;
}
void ListInsertCurPrevious( List_t * const pList, ListItem_t * const pNewListItem )
{
	/* 插入到pIndex的前面,不在pIndex后面插入,好处是:不破坏当前轮转顺序 */
	ListItem_t * const pIndex = pList->pIndex;

	pNewListItem->pNext = pIndex;
	pNewListItem->pPrevious = pIndex->pPrevious;

	pIndex->pPrevious->pNext = pNewListItem;
	pIndex->pPrevious = pNewListItem;

	pNewListItem->pContainer = ( void * ) pList;

	( pList->NumberOfItems )++;
}
UBaseType_t ListRemove( ListItem_t * const pListItemToRemove )
{
	List_t * const pList = ( List_t * ) pListItemToRemove->pContainer;

	pListItemToRemove->pNext->pPrevious = pListItemToRemove->pPrevious;
	pListItemToRemove->pPrevious->pNext = pListItemToRemove->pNext;

	if ( pList->pIndex == pListItemToRemove )
	{
		pList->pIndex = pListItemToRemove->pPrevious;
	}

	pList->NumberOfItems--;
	pListItemToRemove->pContainer = NULL;

	return pList->NumberOfItems;
}


