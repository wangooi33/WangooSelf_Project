#include "timers.h"
#include "queue.h"
/*--------------------------------------------------------------------------------------*/
static List_t ActiveTimerList1;
static List_t ActiveTimerList2;
static List_t *pCurrentTimerList;
static List_t *pOverflowTimerList;

static QueueHandle_t TimerQueue = NULL;
static TaskHandle_t TimerManageHandle = NULL;

/*--------------------------------------------------------------------------------------*/
static void prvInitialiseNewTimer(	const char * const pTimerName,	
								const TickType_t TimerPeriodInTicks,
								const UBaseType_t AutoReload,
								void * const pTimerID,
								TimerCallbackFunction_t pCallbackFunction,
								Timer_t *pNewTimer );
static void prvCheckForValidListAndQueue( void );
static void prvTimerTask( void *pParameters );
static void prvProcessTimerOrBlockTask( const TickType_t NextExpireTime, BaseType_t ListWasEmpty );

/*--------------------------------------------------------------------------------------*/
BaseType_t TimerCreateManageTask( void )
{
	BaseType_t xReturn = pdFAIL;

	prvCheckForValidListAndQueue();
	if( TimerQueue != NULL )
	{
		xReturn = TaskCreate( prvTimerTask,
							  timMANAGE_TASK_NAME,
							  timMANAGE_TASK_STACK_DEPTH,
							  NULL,
							  ( UBaseType_t )timPRIORITY_TASK,
							  &TimerManageHandle );
	}
	return xReturn;
}
static void prvCheckForValidListAndQueue( void )
{
	PortEnterCritical();
	{
		if( TimerQueue == NULL )
		{
			ListInitialise( &ActiveTimerList1 );
			ListInitialise( &ActiveTimerList2 );
			pCurrentTimerList = &ActiveTimerList1;
			pOverflowTimerList = &ActiveTimerList2;

			TimerQueue = QueueCreate( ( UBaseType_t )timQUEUE_LENGTH, sizeof( DaemonTaskMessage_t ) );
			if ( TimerQueue != NULL )
			{
				QueueAddToRegistry( TimerQueue, "TmrQ" );
			}
		}

	}
	PortExitCritical();
}

static void prvTimerTask( void *pParameters )
{
	TickType_t NextExpireTime;
	BaseType_t ListWasEmpty;

	for ( ;; )
	{
		NextExpireTime = prvGetNextExpireTime( &ListWasEmpty );
		prvProcessTimerOrBlockTask( NextExpireTime, ListWasEmpty );
		prvProcessReceivedCommands();
	}
}
static TickType_t prvGetNextExpireTime( BaseType_t * const pListWasEmpty )
{
	TickType_t NextExpireTime;

	*pListWasEmpty = listLIST_IS_EMPTY( pCurrentTimerList );
	if( *pListWasEmpty == pdFALSE )
	{
		NextExpireTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pCurrentTimerList );
	}
	else
	{
		NextExpireTime = ( TickType_t )0U;
	}

	return NextExpireTime;
}
static void prvProcessTimerOrBlockTask( const TickType_t NextExpireTime, BaseType_t ListWasEmpty )
{
	TickType_t TimeNow;
	BaseType_t TimerListsWereSwitched;

	TaskSuspendAll();
	{
		//采样当前时间
		TimeNow = prvSampleTimeNow( &TimerListsWereSwitched );
		//TickList没有切换,则说明没有溢出
		if( TimerListsWereSwitched == pdFALSE )
		{
			//是否到期
			if( ( ListWasEmpty == pdFALSE ) && ( NextExpireTime <= TimeNow ) )
			{
				( void )TaskResumeAll();
				prvProcessExpiredTimer( NextExpireTime, TimeNow );
			}
			else
			{
				if( ListWasEmpty != pdFALSE )
				{
					ListWasEmpty = listLIST_IS_EMPTY( pOverflowTimerList );
				}

				QueueWaitForMessageRestricted( TimerQueue, ( NextExpireTime - TimeNow ), ListWasEmpty );

				if( TaskResumeAll() == pdFALSE )
				{
					portREQUEST_TASK_SWITCH();
				}
			}
		}
		else
		{
			( void )TaskResumeAll();
		}
	}
}
static TickType_t prvSampleTimeNow( BaseType_t * const pTimerListsWereSwitched )
{
	TickType_t TimeNow;
	static TickType_t LastTime = ( TickType_t )0U;
	
	TimeNow = TaskGetTickCount();

	if( TimeNow < LastTime )
	{
		/* 当前时间 < 之前时间,溢出
		若溢出则切换链表 */
		prvSwitchTimerLists();
		*pTimerListsWereSwitched = pdTRUE;
	}
	else
	{
		*pTimerListsWereSwitched = pdFALSE;
	}

	LastTime = TimeNow;

	return TimeNow;
}
static void prvSwitchTimerLists( void )
{
	TickType_t NextExpireTime, ReloadTime;
	List_t *pTemp;
	Timer_t *pTimer;
	BaseType_t Result;

	/* 计数器的数值已超出范围。需要切换计时器列表。
	如果当前计时器列表中仍有任何计时器被引用,
	那么这些计时器肯定已经过期，应在切换列表之前对其进行处理。 */
	while( listLIST_IS_EMPTY( pCurrentTimerList ) == pdFALSE )
	{
		NextExpireTime = listGET_ITEM_VALUE_OF_HEAD_ENTRY( pCurrentTimerList );

		pTimer = ( Timer_t * ) listGET_OWNER_OF_HEAD_ENTRY( pCurrentTimerList );
		( void )ListRemove( &( pTimer->TimerListItem ) );
		//Timer到期,执行回调
		pTimer->pCallbackFunction( ( TimerHandle_t )pTimer );

		if( pTimer->AutoReload == ( UBaseType_t )pdTRUE )
		{
			//下一次溢出的Tick,小值
			ReloadTime = ( NextExpireTime + pTimer->TimerPeriodInTicks );
			//再次溢出
			if( ReloadTime > NextExpireTime )
			{
				listSET_LIST_ITEM_VALUE( &( pTimer->TimerListItem ), ReloadTime );
				listSET_LIST_ITEM_OWNER( &( pTimer->TimerListItem ), pTimer );
				ListInsert( pCurrentTimerList, &( pTimer->TimerListItem ) );
			}
			else
			{
				Result = TimerGenericCommand( pTimer, tmrCOMMAND_START_DONT_TRACE, NextExpireTime, NULL, ( TickType_t )0U );
				( void )Result;
			}
		}
	}

	pTemp = pCurrentTimerList;
	pCurrentTimerList = pOverflowTimerList;
	pOverflowTimerList = pTemp;
}
static void prvProcessExpiredTimer( const TickType_t NextExpireTime, const TickType_t TimeNow )
{
	//处理一个到期的软件定时器
	
	BaseType_t Result;
	//取出最早到期的软件定时器
	Timer_t * const pTimer = ( Timer_t * ) listGET_OWNER_OF_HEAD_ENTRY( pCurrentTimerList );
	( void )ListRemove( &( pTimer->TimerListItem ) );

	//自动重载
	if( pTimer->AutoReload == ( UBaseType_t ) pdTRUE )
	{
		//计算下一个到期时间,插入链表
		if( prvInsertTimerInActiveList( pTimer, ( NextExpireTime + pTimer->TimerPeriodInTicks ), TimeNow, NextExpireTime ) != pdFALSE )
		{
			//超过理论启动Tick,重新安排重新启动
			Result = TimerGenericCommand( pTimer, tmrCOMMAND_START_DONT_TRACE, NextExpireTime, NULL, ( TickType_t )0U );
		}
	}

	pTimer->pCallbackFunction( ( TimerHandle_t )pTimer );
}
static BaseType_t prvInsertTimerInActiveList( Timer_t * const pTimer, const TickType_t NextExpiryTime, const TickType_t TimeNow, const TickType_t CommandTime )
{
	BaseType_t ProcessTimerNow = pdFALSE;

	listSET_LIST_ITEM_VALUE( &( pTimer->TimerListItem ), NextExpiryTime );
	listSET_LIST_ITEM_OWNER( &( pTimer->TimerListItem ), pTimer );

	//已经到期
	if( NextExpiryTime <= TimeNow )
	{
		if( ( ( TickType_t )( TimeNow - CommandTime ) ) >= pTimer->TimerPeriodInTicks )
		{
			//实际处理Tick > 理论处理Tick
			ProcessTimerNow = pdTRUE;
		}
		else
		{
			//溢出
			ListInsert( pOverflowTimerList, &( pTimer->TimerListItem ) );
		}
	}
	else
	{
		if( ( TimeNow < CommandTime ) && ( NextExpiryTime >= CommandTime ) )
		{
			//Tick回绕
			ProcessTimerNow = pdTRUE;
		}
		else
		{
			ListInsert( pCurrentTimerList, &( pTimer->TimerListItem ) );
		}
	}

	return ProcessTimerNow;
}

static void prvProcessReceivedCommands( void )
{
	DaemonTaskMessage_t Message;
	Timer_t *pTimer;
	BaseType_t TimerListsWereSwitched, Result;
	TickType_t TimeNow;

	while( QueueReceive( TimerQueue, &Message, ( TickType_t )0U ) != pdFAIL )
	{
		if( Message.MessageID >= ( BaseType_t ) 0 )
		{
			pTimer = Message.u.TimerParameters.pTimer;

			if( listIS_CONTAINED_WITHIN( NULL, &( pTimer->TimerListItem ) ) == pdFALSE ) 
			{
				( void )ListRemove( &( pTimer->TimerListItem ) );
			}

			TimeNow = prvSampleTimeNow( &TimerListsWereSwitched );

			switch( Message.MessageID )
			{
				case tmrCOMMAND_START :
			    case tmrCOMMAND_RESET :
				case tmrCOMMAND_START_DONT_TRACE :
					if( prvInsertTimerInActiveList( pTimer,  Message.u.TimerParameters.MessageValue + pTimer->TimerPeriodInTicks, TimeNow, Message.u.TimerParameters.MessageValue ) != pdFALSE )
					{
						pTimer->pCallbackFunction( ( TimerHandle_t )pTimer );

						if( pTimer->AutoReload == ( UBaseType_t ) pdTRUE )
						{
							Result = TimerGenericCommand( pTimer, tmrCOMMAND_START_DONT_TRACE, Message.u.TimerParameters.MessageValue + pTimer->TimerPeriodInTicks, NULL, ( TickType_t )0U );
						}
					}
					break;

				case tmrCOMMAND_STOP :
					break;

				case tmrCOMMAND_CHANGE_PERIOD :
					pTimer->TimerPeriodInTicks = Message.u.TimerParameters.MessageValue;
					( void ) prvInsertTimerInActiveList( pTimer, ( TimeNow + pTimer->TimerPeriodInTicks ), TimeNow, TimeNow );
					break;

				case tmrCOMMAND_DELETE :
					vHeapFree( pTimer );
					break;

				default	:
					break;
			}
		}
	}
}



TimerHandle_t TimerCreate(	const char * const pTimerName,	
                               const TickType_t TimerPeriodInTicks,
                               const UBaseType_t AutoReload,
                               void * const pTimerID,
                               TimerCallbackFunction_t pCallbackFunction )
{
    Timer_t *pNewTimer;
    pNewTimer = ( Timer_t * )pHeapMalloc( sizeof( Timer_t ) );

    if ( pNewTimer != NULL )
    {
		prvInitialiseNewTimer( pTimerName, TimerPeriodInTicks, AutoReload, pTimerID, pCallbackFunction, pNewTimer );
    }

    return pNewTimer;
}
static void prvInitialiseNewTimer( const char * const pTimerName,
										   const TickType_t TimerPeriodInTicks,
										   const UBaseType_t AutoReload,
										   void * const pTimerID,
										   TimerCallbackFunction_t pCallbackFunction,
										   Timer_t *pNewTimer )
{
	if( pNewTimer != NULL )
	{
		prvCheckForValidListAndQueue();

		pNewTimer->pTimerName = pTimerName;
		pNewTimer->TimerPeriodInTicks = TimerPeriodInTicks;
		pNewTimer->AutoReload = AutoReload;
		pNewTimer->pTimerID = pTimerID;
		pNewTimer->pCallbackFunction = pCallbackFunction;

		ListInitialiseItem( &( pNewTimer->TimerListItem ) );
	}
}
								
//把“对定时器的操作命令”封装成消息,通过 xTimerQueue 发送给“定时器服务任务”。
BaseType_t TimerGenericCommand( TimerHandle_t Timer, const BaseType_t CommandID, const TickType_t OptionalValue, BaseType_t * const pHigherPriorityTaskWoken, const TickType_t TicksToWait )
{
	BaseType_t xReturn = pdFAIL;
	DaemonTaskMessage_t xMessage;

	if( TimerQueue != NULL )
	{
		xMessage.MessageID = CommandID;
		xMessage.u.TimerParameters.MessageValue = OptionalValue;
		xMessage.u.TimerParameters.pTimer = ( Timer_t * )Timer;

		if ( SchedulerRunning != pdFALSE && SchedulerSuspended == ( UBaseType_t )pdFALSE )
		{
			xReturn = QueueSendToBack( TimerQueue, &xMessage, TicksToWait );
		}
		else
		{
			xReturn = QueueSendToBack( TimerQueue, &xMessage, ( TickType_t )0 );
		}
	}
	
	return xReturn;
}


