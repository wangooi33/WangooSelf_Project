#include "timers.h"
#include "memory.h"
#include "queue.h"

QueueHandle_t TimerQueue = NULL;
ListManager_t ActiveTimerList;


static void prvInitialiseNewTimer( uint32_t TimerPeriodInTicks,
                                    uint8_t isAutoReload,
                                    uint32_t TimerID,
                                    TimerCallBackFunction_t pCallBackFunction,
                                    Timer_t* pNewTimer );
static uint32_t prvGetNextExpireTime( uint8_t * const pListWasEmpty );
static void prvProcessTimerOrBlockTask( const uint32_t NextExpireTime, uint8_t ListWasEmpty );
static void	prvProcessReceivedCommands( void );
static void prvInsertTimerInActiveList( Timer_t * const pTimer );

void TimerManageTaskInit( void )
{
    if ( TimerQueue == NULL )
    {
        ListInitialise(&(ActiveTimerList));
        TimerQueue = QueueCreate(timerQUEUE_LENGTH,sizeof(TimerMessageRe_t));
    }
}
TimerHandle_t TimerCreate( uint32_t TimerPeriodInTicks,
                            uint8_t isAutoReload,
                            uint32_t TimerID,
                            TimerCallBackFunction_t pCallBackFunction )
{
    Timer_t *pNewTimer;
    pNewTimer = (Timer_t *)MemMalloc(sizeof(Timer_t));
    if ( pNewTimer != NULL )
    {
        prvInitialiseNewTimer(TimerPeriodInTicks,isAutoReload,TimerID,pCallBackFunction,pNewTimer);
    }
    return pNewTimer;
}
static void prvInitialiseNewTimer( uint32_t TimerPeriodInTicks,
                                    uint8_t isAutoReload,
                                    uint32_t TimerID,
                                    TimerCallBackFunction_t pCallBackFunction,
                                    Timer_t* pNewTimer )
{   
    pNewTimer->TimerPeriodInTicks = TimerPeriodInTicks;
    pNewTimer->isAutoReload = isAutoReload;
    pNewTimer->TimerID = TimerID;
    pNewTimer->pCallbackFunction = pCallBackFunction;
    ListInitialiseItem(&(pNewTimer->TimerListItem));
}

uint8_t TimerGenericCommand( TimerHandle_t Timer, const uint8_t CommandID, const uint32_t Value )
{
    uint8_t xReturn = pdFALSE;
    TimerMessageRe_t TimerMessage;
    TimerMessage.Command_ID = CommandID;
    TimerMessage.u.TimerParameters.pTimer = Timer;
    TimerMessage.u.TimerParameters.MessageValue = Value;
    xReturn = QueueSendToBack(TimerQueue,&TimerMessage);
    return xReturn;
}
uint8_t TimerPendFunctionCall( TimerHandle_t Timer )
{
    uint8_t xReturn = pdFALSE;
    TimerMessageRe_t TimerMessage;
    TimerMessage.Command_ID = timerCOMMAND_EXECUTECALLBACK;
    TimerMessage.u.CallBackParameters.pTimer = Timer;
    xReturn = QueueSendToBack(TimerQueue,&TimerMessage);
    return xReturn;
}

void TimerManagerTask( void )
{
    uint32_t NextExpireTime;
    uint8_t ListWasEmpty;
    NextExpireTime = prvGetNextExpireTime(&ListWasEmpty);
    prvProcessTimerOrBlockTask(NextExpireTime,ListWasEmpty);
    prvProcessReceivedCommands();
}
static uint32_t prvGetNextExpireTime( uint8_t * const pListWasEmpty )
{
    uint32_t NextExpireTime;

    *pListWasEmpty = (ActiveTimerList.NumberOfItems == 0);
    if ( *pListWasEmpty == pdFALSE )
    {
        //非空
        NextExpireTime = (ActiveTimerList.ListEnd.pNextItem->value);
    }
    else
    {
        NextExpireTime = 0;
    }
    return NextExpireTime;
}
static void prvProcessTimerOrBlockTask( const uint32_t NextExpireTime, uint8_t ListWasEmpty )
{
	uint32_t TimeNow = GetSysticks();
    Timer_t *pTimer = (Timer_t *)(ActiveTimerList.ListEnd.pNextItem->pOwner_Task);
    if ( (ListWasEmpty == pdFALSE) && (NextExpireTime <= TimeNow) )
    {
        ListDelete(&(pTimer->TimerListItem));
        if ( pTimer->isAutoReload == pdTRUE )
        {
            prvInsertTimerInActiveList(pTimer);
            TimerGenericCommand(pTimer,timerCOMMAND_START,NextExpireTime);
        }
    }
    pTimer->pCallbackFunction((TimerHandle_t)pTimer);
}

static void prvInsertTimerInActiveList( Timer_t * const pTimer )
{
    pTimer->TimerListItem.pOwner_Task = pTimer;
	pTimer->TimerListItem.value = GetSysticks() + pTimer->TimerPeriodInTicks;
    ListInsertSort(&ActiveTimerList,&(pTimer->TimerListItem));
}


static void	prvProcessReceivedCommands( void )
{
    TimerMessageRe_t Message;
    Timer_t *pTimer;
    uint8_t TimerListsWereSwitched;
    uint32_t TimeNow;

    while ( QueueReceive(TimerQueue,&Message) != pdFALSE )
    {
        if ( Message.Command_ID < 0 )
        {
            //执行软件定时器回调函数
            Timer_t *pTimer = Message.u.CallBackParameters.pTimer;
            pTimer->pCallbackFunction((TimerHandle_t)Message.u.CallBackParameters);
        }
        if ( Message.Command_ID > 0 )
        {
            pTimer = Message.u.TimerParameters.pTimer;
            if ( (pTimer->TimerListItem).pOwner_List != NULL )
            {
                //从链表中删除
                ListDelete(&(pTimer->TimerListItem));
            }

            {
                //此处可以做系统Tick溢出判断,
                TimeNow = GetSysticks();
            }
            
            switch (Message.Command_ID)
            {
                case timerCOMMAND_START:
                case timerCOMMAND_RESET:
                    prvInsertTimerInActiveList(pTimer);
                    break;

                case timerCOMMAND_STOP:
                    /* 计时器已从活动列表中移除,这里无需做任何处理 */
                    break;

                case timerCOMMAND_CHANGE_PERIOD:
                    pTimer->TimerPeriodInTicks = Message.u.TimerParameters.MessageValue;
                    prvInsertTimerInActiveList(pTimer);
                    break;

                case timerCOMMAND_DELETE:
                    MemFree(pTimer);
                    break;
                
                default:
                    break;
            }
        }
    }
}

uint32_t TimerGetTimerID( const TimerHandle_t Timer )
{
    Timer_t * const pTimer = (Timer_t *)Timer;
    uint32_t xReturn;
	xReturn = pTimer->TimerID;
	return xReturn;
}

void MyTestTimerCallback(TimerHandle_t Timer)
{
    uint32_t TimerID = TimerGetTimerID(Timer);
    
    if(TimerID == 1) 
    {
        //定时器1到期
    } 
    else if(TimerID == 2) 
    {
        //定时器2到期
    }
}


