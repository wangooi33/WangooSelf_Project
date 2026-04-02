#ifndef LIST_H
#define LIST_H

#define listMAX_SORTVALUE 0xFFFF
typedef uint32_t ListSortValue_t;

typedef struct List_Node
{
    ListItem_t *pPreviousItem;
    ListItem_t *pNextItem;
    ListSortValue_t value;  //排序权值
    void *pOwner_Task;      //所属任务(TCB)或者Timer_t
    void *pOwner_List;      //所属链表(ListManager_t)
}ListItem_t;

typedef struct List_End
{
    uint32_t NumberOfItems;
    ListItem_t ListEnd;
    ListItem_t *pCurrentListItem;
}ListManager_t;

void ListInitialise( ListManager_t *plist );
void ListInitialiseItem( ListItem_t *pItem );
void ListInsert( ListManager_t *plist, ListItem_t *pNewItem );
void ListInsertSort( ListManager_t *plist, ListItem_t *pNewItem );
uint16_t ListDelete( ListItem_t *pItemToDelete );


#endif /* LIST_H */

