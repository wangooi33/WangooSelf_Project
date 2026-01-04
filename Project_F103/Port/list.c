#include "list.h"





void ListInitialise( List_t * const pList )
{
	pList->pIndex = (ListItem_t *)(pList->ListEnd);
	pList->uNumberOfItems = (UBaseType_t)0U;
	
	pList->ListEnd.ItemValue = MAX_DELAY;
	pList->ListEnd.pNext = (ListItem_t *)(pList->ListEnd);
	pList->ListEnd.pPrevious = (ListItem_t *)(pList->ListEnd);
}
void ListInitialiseItem( ListItem_t * const pItem )
{
	pItem->pContainer == NULL;
}

/* xListEnd -> 最小 -> ... -> 最大 -> xListEnd */
void ListInsert( List_t * const pList, ListItem_t * const pNewListItem )
{
	ListItem_t *pIterator;
	const TickType_t ValueOfInsertion = pNewListItem->ItemValue;

	if( ValueOfInsertion == portMAX_DELAY )
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

	( pList->uNumberOfItems )++;
}
void ListInsertCur( List_t * const pList, ListItem_t * const pNewListItem )
{
	ListItem_t * const pIndex = pList->pIndex;

	pNewListItem->pNext = pIndex;
	pNewListItem->pPrevious = pIndex->pPrevious;

	pIndex->pPrevious->pNext = pNewListItem;
	pIndex->pPrevious = pNewListItem;

	pNewListItem->pContainer = ( void * ) pList;

	( pList->uNumberOfItems )++;
}
UBaseType_t ListRemove( ListItem_t * const pListItemToRemove )
{
	List_t * const pList = ( List_t * ) pListItemToRemove->pContainer;

	pListItemToRemove->pNext->pPrevious = pListItemToRemove->pPrevious;
	pListItemToRemove->pPrevious->pNext = pListItemToRemove->pNext;

	if( pList->pIndex == pListItemToRemove )
	{
		pList->pIndex = pListItemToRemove->pPrevious;
	}

	pList->uNumberOfItems--;
	pListItemToRemove->pContainer = NULL;

	return pList->uNumberOfItems;
}


