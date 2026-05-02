#include "list.h"

//空表时只有链表头占用空间
void ListInitialise( ListManager_t *plist )
{
    plist->NumberOfItems = 0;
    plist->pCurrentListItem = (ListItem_t *)&(plist->ListEnd);
    plist->ListEnd.value = listMAX_SORTVALUE;
    plist->ListEnd.pPreviousItem = (ListItem_t *)&(plist->ListEnd);
    plist->ListEnd.pNextItem = (ListItem_t *)&(plist->ListEnd);
}
void ListInitialiseItem( ListItem_t *pItem )
{
    pItem->pOwner_List = NULL;
	pItem->pOwner_Task = NULL;
}
//插入到当前节点的前面
void ListInsert( ListManager_t *plist, ListItem_t *pNewItem )
{
    ListItem_t* const pIndex = plist->pCurrentListItem;	//常量指针
    
    pNewItem->pNextItem = pIndex;
    pNewItem->pPreviousItem = pIndex->pPreviousItem;
    pIndex->pPreviousItem->pNextItem = pNewItem;
    pIndex->pPreviousItem = pNewItem;

    pNewItem->pOwner_List = plist;
    plist->NumberOfItems++;
}
//按权值排序插入
void ListInsertSort( ListManager_t *plist, ListItem_t *pNewItem )
{
    ListItem_t* pIterator;
    ListSortValue_t value = pNewItem->value;

    if ( value == listMAX_SORTVALUE )
    {
        pIterator = (ListItem_t *)&(plist->ListEnd);
    }
    else
    {
        for ( pIterator = plist->ListEnd.pNextItem; pIterator->value <= value; pIterator = pIterator->pNextItem )
        {
        }
    }

    pNewItem->pNextItem = pIterator;
    pNewItem->pPreviousItem = pIterator->pPreviousItem;
    pIterator->pPreviousItem->pNextItem = pNewItem;
    pIterator->pPreviousItem = pNewItem;
    
    pNewItem->pOwner_List = plist;
    plist->NumberOfItems++;
}

uint16_t ListDelete( ListItem_t *pItemToDelete )
{
    ListManager_t *pList = (ListManager_t *)pItemToDelete->pOwner_List;

    pItemToDelete->pPreviousItem->pNextItem = pItemToDelete->pNextItem;
    pItemToDelete->pNextItem->pPreviousItem = pItemToDelete->pPreviousItem;
    if ( pList->pCurrentListItem == pItemToDelete )
    {
        pList->pCurrentListItem = pItemToDelete->pPreviousItem;
    }
    pList->NumberOfItems--;
    pItemToDelete->pOwner_List = NULL;

    return pList->NumberOfItems;
}
