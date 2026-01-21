#include "task.h"
#include <string.h>
#include <stdint.h>

/*--------------------------------------------------------------------------------------*/

TCB_t * volatile pCurrentTCB = NULL;										/**/
volatile UBaseType_t CurrentNumberOfTasks = ( UBaseType_t )0U;				/* 当前任务数量 */
volatile UBaseType_t DeletedTasksWaitingCleanUp = ( UBaseType_t )0;			/* 需要清除的任务数量 */
static volatile BaseType_t YieldPending = pdFALSE;							/* 请求切换任务,但此刻不能切,需等到安全点再进行切换 */

static volatile UBaseType_t SchedulerRunning = pdFALSE;						/* 调度器运行状态 */
static volatile UBaseType_t SchedulerSuspended	= ( UBaseType_t )pdFALSE;	/* 调度器挂起状态 */

volatile TickType_t TickCount = ( TickType_t )0;							/* 系统滴答计数 */
static volatile UBaseType_t PendedTicks = ( UBaseType_t )0U;				/* 挂起时产生的tick */
static volatile TickType_t NextTaskUnblockTime = ( TickType_t )0U;
static volatile BaseType_t NumOfOverflows = ( BaseType_t )0;

static volatile UBaseType_t TopReadyPriority = ( UBaseType_t )0U;			/* 就绪链表中最高优先级 */
static List_t pReadyTasksLists[ taskMAX_PRIORITIES ];
static List_t DelayedTaskList1;
static List_t DelayedTaskList2;
static List_t * volatile pDelayedTaskList;
static List_t * volatile pOverflowDelayedTaskList;
static List_t PendingReadyList;												/* 装载在中断中被“唤醒”的任务 */
static List_t SuspendedTaskList;
static List_t WaitingDeleteTaskList;										/* 等待被删除的任务 */


static TaskHandle_t IdleTaskHandle = NULL;
/*--------------------------------------------------------------------------------------*/

static void InitialiseNewTask( TaskFunction_t pTaskCode,
									 const char * const pName,
									 const uint16_t StackDepth,
									 void * const pParameters,
									 UBaseType_t Priority,
									 TaskHandle_t * const pHandle, 
									 TCB_t *pNewTCB );
static void InitialiseTaskLists( void );

static void AddNewTaskToReadyList( TCB_t *pNewTCB );
static void AddCurrentTaskToDelayList( TickType_t TicksToWait, const BaseType_t CanBlockIndefinitely );


static void ResetNextTaskUnblockTime( void );
BaseType_t SysTickCount( void );
static void DeleteTCB( TCB_t *pTCB );
static void IdleTask( void * pParameters );

/*--------------------------------------------------------------------------------------*/
#define taskSET_READY_PRIORITY( Priority, ReadyPriorities ) ( ReadyPriorities ) |= ( 1UL << ( Priority ) )
#define taskRESET_READY_PRIORITY( Priority, ReadyPriorities ) ( ReadyPriorities ) &= ~( 1UL << ( Priority ) )
//最高置位 bit 的索引(数值最大优先级) = 31 - 前导零数量
#define taskGET_HIGHEST_PRIORITY( TopPriority, ReadyPriorities ) TopPriority = ( 31UL - ( uint32_t ) __clz( ( ReadyPriorities ) ) )

//1:优先级使用位图法
#if 1
	#define  taskREADYLIST_PRIORITY_ITERATE( Priority )			\
	{															\
		taskSET_READY_PRIORITY( Priority,TopReadyPriority ); 	\
	}

	#define taskREADYLIST_PRIORITY_RESET( Priority )			\
	{															\
		taskRESET_READY_PRIORITY( Priority,TopReadyPriority );  \
	}

	//取出最高优先级链表中的下一个任务
	#define taskSELECT_HIGHEST_PRIORITY_TASK()												\
	{																						\
		UBaseType_t TopPriority;															\
																							\
		taskGET_HIGHEST_PRIORITY( TopPriority, TopReadyPriority );							\
		listGET_OWNER_OF_NEXT_ENTRY( pCurrentTCB, &( pReadyTasksLists[ TopPriority ] ) );	\
	}
#else
	#define  taskREADYLIST_PRIORITY_ITERATE( Priority )			\
	{															\
		if ( TopReadyPriority < Priority )						\
		{														\
			TopReadyPriority = Priority;						\
		}														\
	}

	#define taskREADYLIST_PRIORITY_RESET( Priority )

	#define taskSELECT_HIGHEST_PRIORITY_TASK()												\
	{																						\
		UBaseType_t TopPriority = TopReadyPriority;											\
																							\
		while( listIS_EMPTY( &( pReadyTasksLists[ TopPriority ] ) ) )						\
		{																					\
			--TopPriority;																	\
		}																					\
																							\
		listGET_OWNER_OF_NEXT_ENTRY( pCurrentTCB, &( pReadyTasksLists[ TopPriority ] ) );	\
		TopReadyPriority = TopPriority;														\
	}
#endif


#define taskREADYLIST_ADDTASK( pNewTCB )															 \
{																									 \
	taskREADYLIST_PRIORITY_ITERATE( pNewTCB->Priority );											 \
	ListInsertCurPrevious( &( pReadyTasksLists[ pNewTCB->Priority ] ), &( pNewTCB->StateListItem ) );\
}

#ifdef portENABLE_TICK_OVERFLOW_CHECK
	#define taskSWITCH_DELAYED_LISTS()									\
		do {															\
			const TickType_t NextTick = TickCount + ( TickType_t ) 1;	\
																		\
			/* 溢出只能发生在“加 1 的那一刻”,不加 1 无法判断 */			                \
			if( NextTick == ( TickType_t ) 0U )							\
			{															\
				List_t *pTemp;											\
				pTemp = pDelayedTaskList;								\
				pDelayedTaskList = pOverflowDelayedTaskList;			\
				pOverflowDelayedTaskList = pTemp;						\
																		\
				NumOfOverflows++;										\
				ResetNextTaskUnblockTime();								\
			}															\
		} while( 0 )
#endif

/*--------------------------------------------------------------------------------------*/

BaseType_t TaskCreate( 	TaskFunction_t pTaskCode,
							const char * const pName,
							const uint16_t StackDepth,
							void * const pParameters,
							UBaseType_t Priority,
							TaskHandle_t * const pHandle )
{
	BaseType_t xReturn;
	TCB_t *pxNewTCB;
	StackType_t *pStack;

	//1.(地址向下生长)先分配任务栈,再分配TCB
	pStack = ( StackType_t * )pHeapMalloc(( ( size_t )StackDepth ) * sizeof( StackType_t ) );
	if( pStack != NULL )
	{
		pxNewTCB = ( TCB_t * )pHeapMalloc( sizeof( TCB_t ) );
		if( pxNewTCB != NULL )
		{
			pxNewTCB->pStack = pStack;
		}
		else
		{
			vHeapFree( pStack );
		}
	}
	else
	{
		pxNewTCB = NULL;
	}

	//2.初始化
	if( pxNewTCB != NULL )
	{
		InitialiseNewTask( pTaskCode, pName, ( uint32_t )StackDepth, pParameters, Priority, pHandle, pxNewTCB );
		AddNewTaskToReadyList( pxNewTCB );
		xReturn = pdPASS;
	}
	else
	{
		xReturn = taskERROR_MEMORY_ALLOCTAE;
	}
	return xReturn;
}

void TaskSuspendAll( void )
{
	//任务不能切换
	++SchedulerSuspended;
}
BaseType_t TaskResumeAll( void )
{
	TCB_t *pTCB = NULL;
	UBaseType_t AlreadyRequest;
	PortEnterCritical();
	{
		--SchedulerRunning;
		if ( SchedulerRunning == ( UBaseType_t )pdFALSE )
		{
			if ( CurrentNumberOfTasks > 0 )
			{
				while ( listIS_EMPTY( &PendingReadyList ) )
				{
					pTCB = ( TCB_t * )listGET_ENDNEXT_LISTITEM_OWNER( &PendingReadyList );
					ListRemove( &( pTCB->StateListItem ) );
					ListRemove( &( pTCB->EventListItem ) );
					AddNewTaskToReadyList( pTCB );
					if ( pTCB->Priority > pCurrentTCB->Priority )
					{
						/* (特殊情况)虽然满足条件:1.在临界区内;2.高优先级抢占。
						但是还在while循环里;PendingReadyList还没处理完;ready list 正在被修改,
						突然触发PendSV,调度器在一个半更新状态下运行 */
						YieldPending = pdTRUE;
					}
					
				}
				if ( pTCB != NULL )
				{
					ResetNextTaskUnblockTime();
				}

				UBaseType_t CurrentPendTick = PendedTicks;
				if ( CurrentPendTick > ( UBaseType_t )0 )
				{
					do
					{
						if ( SysTickCount() != pdFALSE )
						{
							YieldPending = pdTRUE;
						}
						CurrentPendTick--;
					}while ( CurrentPendTick > 0U );
					PendedTicks = 0;
				}
					
				
				//挂起期间欠的所有切换，合并成一次 PendSV
				if ( YieldPending != pdFALSE )
				{
					AlreadyRequest = pdTRUE;
					portREQUEST_TASK_SWITCH();
				}
			}
		}
	}
	PortExitCritical();

	return AlreadyRequest;
}
void TaskStartScheduler( void )
{
	BaseType_t xReturn;
	
	//1.创建空闲任务
	xReturn = TaskCreate(IdleTask, 
						 taskIDLE_TASK_NAME,
						 ( uint16_t )127,
						 ( void * )NULL, 
						 taskIDLE_TASK_PRIORITY, 
						 &IdleTaskHandle);

	if ( xReturn == pdPASS )
	{
		//2.创建软件定时器起始任务
		

		//3.初始化系统
		PortSetBASEPRI(0);
		NextTaskUnblockTime = portMAX_DEALY;
		SchedulerRunning = pdTRUE;
		TickCount = ( TickType_t ) 0U;

		//4.启动
		if( PortStartScheduler() != pdFALSE )
		{
			/* Should not reach here as if the scheduler is running the
			function will not return. */
		}
		else
		{
			/* Should only reach here if a task calls TaskEndScheduler(). */
		}
	}

}
void TaskDelete(    TaskHandle_t TaskToDelete )
{
	TCB_t *pTCB;
	PortEnterCritical();
	{
		pTCB = ( TaskToDelete == NULL ) ? ( TCB_t * )pCurrentTCB : ( TCB_t * )TaskToDelete;

		//1.从链表中删除
		if ( ListRemove( &( pTCB->StateListItem ) ) == ( UBaseType_t )0 )
		{
			//当前无任务
			taskREADYLIST_PRIORITY_RESET( pTCB->Priority );
		}
		if ( listGET_LISTITEM_CONTAINER( &( pTCB->EventListItem ) ) != NULL )
		{
			ListRemove( &( pTCB->EventListItem ) );
		}

		//2.任务切换
		if ( pCurrentTCB == pTCB )
		{
			//移到待删除链表
			ListInsertCurPrevious( &( WaitingDeleteTaskList ), &( pTCB->StateListItem ));
			++DeletedTasksWaitingCleanUp;
		}
		else
		{
			DeleteTCB( pTCB );
			//如果下次预计解锁时间指的是刚刚被删除的任务,则重置该时间
			ResetNextTaskUnblockTime();
		}

	}
	PortExitCritical();

	//2.如果当前正在运行的任务刚刚被删除,则强制重新安排任务时间
	if ( pCurrentTCB == pTCB )
	{
		portREQUEST_TASK_SWITCH();
	}
}
void TaskSwitchContext( void )
{
	//这个函数本质上就切换pCurrentTCB

	//切换只能在调度器运行时进行
	if ( SchedulerSuspended != ( UBaseType_t )pdFALSE )
	{
		YieldPending = pdTRUE;
	}
	else
	{
		YieldPending = pdFALSE;

		//1.计算pCurrentTCB->RunTimeCounter,需要借助定时器
		
		
		//2.检查栈溢出
		portCHEAK_STACK_OVERFLOW();

		//3.任务切换
		taskSELECT_HIGHEST_PRIORITY_TASK();
	}
}
void TaskSuspend( TaskHandle_t TaskToSuspend )
{
	TCB_t *pTCB;
	PortEnterCritical();
	{
		//1.传参判断,为NULL则挂起pCurrentTCB
		pTCB = ( TaskToSuspend != NULL ) ? TaskToSuspend : pCurrentTCB;

		//2.处理状态链表、事件链表
		if ( ListRemove( &( pTCB->StateListItem ) ) == ( UBaseType_t )0 )
		{
			//优先级迭代
			taskREADYLIST_PRIORITY_ITERATE( pTCB->Priority );
		}
		if ( listGET_LISTITEM_CONTAINER( &( pTCB->EventListItem ) ) != NULL )
		{
			ListRemove( &( pTCB->EventListItem ) );
		}
		//3.插入挂起链表
		ListInsert( &SuspendedTaskList, &( pTCB->EventListItem ) );
		//4.复位任务通知
		pTCB->NotifyState = taskNOTOFY_NOT_WAIT;
	}
	PortExitCritical();

	//5.下一个解锁时间更新
	if ( SchedulerRunning != pdFALSE )
	{
		PortEnterCritical();
		{
			ResetNextTaskUnblockTime();
		}
		PortExitCritical();
	}
	
	//6.任务切换
	if ( pCurrentTCB == pTCB )
	{
		portREQUEST_TASK_SWITCH();
	}
	else
	{
		if ( SchedulerRunning != pdFALSE )
		{
			portREQUEST_TASK_SWITCH();
		}
		else
		{
			if ( listGET_CURRENTLIST_LENTH( &SuspendedTaskList ) == CurrentNumberOfTasks )
			{
				pCurrentTCB = NULL;
			}
			else
			{
				//切换pCurrentTCB,原因是pCurrentTCB在任务时刻必须是自治的。
				TaskSwitchContext();
			}
		}
	}
}
/*---------------------------------Tick计数----------------------------------------------*/

BaseType_t SysTickCount( void )
{
	TCB_t *pTCB;
	UBaseType_t TickValue;
	BaseType_t SwitchRequired;

	/*tick++:
		1.进行整个延时链表到就绪链表转换,直到当前tick < 延时时间.
		2.时间片轮询
	*/
	if ( SchedulerSuspended == ( UBaseType_t )pdFALSE )
	{
		//采用模2ⁿ来计算Tick
		TickType_t CurrentTick = TickCount + ( TickType_t )1;
		TickCount = CurrentTick;
		if ( TickCount == ( TickType_t )0 )
		{
			//溢出
			taskSWITCH_DELAYED_LISTS();
		}
		
		if ( CurrentTick > NextTaskUnblockTime )
		{
			for ( ;; )
			{
				if ( listIS_EMPTY( pDelayedTaskList ) == pdTRUE )
				{
					NextTaskUnblockTime = portMAX_DEALY;
					break;
				}
				else
				{
					pTCB = ( TCB_t * )listGET_ENDNEXT_LISTITEM_OWNER( pDelayedTaskList );
					TickValue = listGET_LISTITEM_VALUE( &( pTCB->StateListItem ) );
					if ( CurrentTick < TickValue )
					{
						//链表按升序排列,若值已经小于ItemValue,则视为结束
						NextTaskUnblockTime = TickValue;
						break;
					}

					//切换链表
					( void )ListRemove( &( pTCB->StateListItem ) );
					if ( listGET_LISTITEM_CONTAINER( &( pTCB->EventListItem ) ) != NULL  )
					{
						( void )ListRemove( &( pTCB->EventListItem ) );
					}
					AddNewTaskToReadyList( pTCB );

					//优先级抢占
					if ( pTCB->Priority > pCurrentTCB->Priority )
					{
						SwitchRequired = pdTRUE;
					}
				}
			}
			
		}
		
		//同优先级时间片轮询
		if ( listGET_CURRENTLIST_LENTH( &( pReadyTasksLists[ pCurrentTCB->Priority ] ) ) > ( UBaseType_t )1 )
		{
			SwitchRequired = pdTRUE;
		}
	}
	//亏欠时间++
	else
	{
		++PendedTicks;
	}

	//防止重复触发PendSV
	if ( YieldPending == pdTRUE )
	{
		SwitchRequired = pdTRUE;
	}

	return SwitchRequired;
}

/*---------------------------------任务通知----------------------------------------------*/

BaseType_t TaskNotifyProduce( TaskHandle_t TaskToNotify, uint32_t Value, eNotifyAction eAction, uint32_t *pPreviousNotificationValue )
{
	//可以抽象成V操作

	BaseType_t xReturn = pdPASS;
	TCB_t *pTCB;
	uint32_t OriginalNotifyState;
	PortEnterCritical();
	{
		//1.任务变量修改
		pTCB = ( TCB_t * )TaskToNotify;
		//任务通知值的快照(被修改掉之前的原始值)
		if ( pPreviousNotificationValue != NULL )
		{
			*pPreviousNotificationValue = pTCB->NotifiedValue;
		}
		OriginalNotifyState = pTCB->NotifiedValue;
		pTCB->NotifyState = taskNOTOFY_RECEIVED;

		//2.操作通知值
		switch ( eAction )
		{
			case eNoAction:
				break;

			case eSetBits:
				pTCB->NotifiedValue = Value;
				break;

			case eIncrement:
				pTCB->NotifiedValue += Value;
				break;

			case eSetValueWithOverwrite:
				pTCB->NotifiedValue = Value;
				break;

			case eSetValueWithoutOverwrite:
				if ( OriginalNotifyState != taskNOTOFY_RECEIVED )
				{
					pTCB->NotifiedValue = Value;
				}
				else
				{
					xReturn = pdFAIL;
				}
				break;
		}
		
		if ( OriginalNotifyState == taskNOTOFY_WAITING )
		{
			//3.从延时链表中删除(等待任务通知设有超时时间),移入就绪链表,等待CPU重新切回任务
			ListRemove( &( pTCB->StateListItem ) );
			AddNewTaskToReadyList( pTCB );

			/*  */
			#if ( portENABLE_TICKLESS_IDLE == 1 )
			{
				ResetNextTaskUnblockTime();
			}
			#endif

			
			//4.任务切换
			if ( pTCB->Priority > pCurrentTCB->Priority )
			{
				portREQUEST_TASK_SWITCH();
			}
		}
		
	}
	PortExitCritical();

	return xReturn;
}
BaseType_t TaskNotifyWait( uint32_t BitsToClearOnEntry, uint32_t BitsToClearOnExit, uint32_t *pNotificationValue, TickType_t TicksToWait )
{
	/* 任务三种解除阻塞的时机:
	(1)通知早就到了(没真正阻塞):
		1.任务调用TaskNotifyWait()
		2.进入临界区
		3.发现:pCurrentTCB->NotifyState == taskNOTOFY_RECEIVED
	(2)通知在等待过程中到达:
		1.设置NotifyState = taskNOTOFY_NOT_WAIT 
		2.任务被挂进延时链表
		3.ISR / 其他任务调用TaskNotify()
		4.NotifyState = taskNOTOFY_RECEIVED,并把任务从阻塞态拉回ready list
		5.任务继续执行,到进入“第二个临界区”函数那里 
	(3)超时醒来:
		1.任务进入taskNOTOFY_WAITING
		2.没有人notify,Tick到期,调度器把任务从延时链表挪回ready list
		3.状态仍然不是taskNOTOFY_RECEIVED,返回值为xReturn = pdFALSE */

	BaseType_t xReturn;
	
	//NotifyState和NotifiedValue可能被中断修改,所以要进入临界区
	PortEnterCritical();
	{
		if ( pCurrentTCB->NotifyState != taskNOTOFY_RECEIVED )
		{
			/* 清除指定bit位.
			若BitsToClearOnEntry = 0xFFFFFFFF,表示要保存原值,
			若BitsToClearOnEntry = 0,表示要清空原值 */
			pCurrentTCB->NotifiedValue &= ~BitsToClearOnEntry;

			//状态机实时同步,防止ISR在中间切入
			pCurrentTCB->NotifyState = taskNOTOFY_WAITING;
			
			if ( TicksToWait > ( TickType_t )0 )
			{
				//插入延时链表
				AddCurrentTaskToDelayList( TicksToWait, pdTRUE );
				//任务切走,等待这个任务回到就绪链表,才继续执行下面的函数语句。
				portREQUEST_TASK_SWITCH();
			}
		}
	}
	PortExitCritical();

	PortEnterCritical();
	{
		//返回原值
		if ( pNotificationValue != NULL )
		{
			*pNotificationValue = pCurrentTCB->NotifiedValue;
		}

		if ( pCurrentTCB->NotifyState != taskNOTOFY_RECEIVED )
		{
			xReturn = pdFAIL;
		}
		else
		{
			//BitsToClearOnExit通常写为 UINT32_MAX
			pCurrentTCB->NotifiedValue &= ~BitsToClearOnExit;
			xReturn = pdTRUE;
		}

		pCurrentTCB->NotifyState = taskNOTOFY_WAITING;
	}
	PortExitCritical();

	return xReturn;
}
uint32_t TaskNotifyTake( BaseType_t ClearCountOnExit, TickType_t TicksToWait )
{
	//类似于PV操作中的P,获取一个任务通知(不论是谁产生的)

	uint32_t ulReturn;
	
	PortEnterCritical();
	{
		if ( pCurrentTCB->NotifiedValue == 0UL )
		{
			pCurrentTCB->NotifyState = taskNOTOFY_WAITING;
			if ( TicksToWait > ( TickType_t )0 )
			{
				AddCurrentTaskToDelayList( TicksToWait, pdTRUE );
				portREQUEST_TASK_SWITCH();
			}
		}

	}
	PortExitCritical();

	PortEnterCritical();
	{
		ulReturn = pCurrentTCB->NotifiedValue;
		if ( ulReturn != 0UL )
		{
			//ClearCountOnExit仅有两种选择:pdTrue		or		pdFALSE
			if ( ClearCountOnExit != pdFALSE )
			{
				//清零通知值
				pCurrentTCB->NotifiedValue = 0UL;
			}
			else
			{
				pCurrentTCB->NotifiedValue = ulReturn - ( uint32_t )1;
			}
		}
		pCurrentTCB->NotifyState = taskNOTOFY_NOT_WAIT;
	}
	PortExitCritical();

	return ulReturn;
}
BaseType_t TaskNotifyStateClear( TaskHandle_t Task )
{
	//收到了通知,但是不做处理,仅仅只复位状态
	UBaseType_t xReturn;
	TCB_t *pTCB;

	pTCB = ( Task == NULL ) ? ( TCB_t * )pCurrentTCB : ( TCB_t * )Task;
	PortEnterCritical();

	{
		if ( pTCB->NotifyState == taskNOTOFY_RECEIVED )
		{
			pTCB->NotifyState == taskNOTOFY_NOT_WAIT;
			xReturn = pdTRUE;
		}
		else
		{
			xReturn = pdFALSE;
		}
	}
	PortExitCritical();

	return xReturn;
}
/*-----------------------------------静态函数---------------------------------------------------*/

static void InitialiseNewTask( TaskFunction_t pTaskCode,
									 const char * const pName,
									 const uint16_t StackDepth,
									 void * const pParameters,
									 UBaseType_t Priority,
									 TaskHandle_t * const pHandle, 
									 TCB_t *pNewTCB )
{
	StackType_t *pTopOfStack;
	UBaseType_t x;

	//从任务栈起始地址开始,把整个栈空间全部填成同一个字节值(0xA5)
	#if( portENABLE_STACK_OVERFLOW_CHECK == 1 )
	{
		( void ) memset( pNewTCB->pStack, ( int )taskSTACK_FILL_BYTE, ( size_t )StackDepth * sizeof( StackType_t ) );
	}
	#endif 

	//1.字节对齐(向下)
	pTopOfStack = pNewTCB->pStack + ( StackDepth - ( uint32_t )1 );
	pTopOfStack = ( StackType_t * )( ( ( uint32_t )pTopOfStack ) & ( ~( ( uint32_t )( portBYTE_ALIGNED - 1 ))) );

	//2.将任务名称存储在控制块中
	for ( x = ( UBaseType_t )0; x < ( UBaseType_t )taskMAX_NAMELEN; x++ )
	{
		pNewTCB->pTaskName[ x ] = pName[ x ];

		if( pName[ x ] == 0x00 )
		{
			break;
		}
	}
	pNewTCB->pTaskName[ taskMAX_NAMELEN - 1 ] = '\0';

	//3.优先级范围判断
	if ( Priority >= ( UBaseType_t )taskMAX_PRIORITIES )
	{
		Priority = ( UBaseType_t )taskMAX_PRIORITIES - ( UBaseType_t )1;
	}
	pNewTCB->Priority = Priority;

	
	//4.链表指针初始化
	ListInitialiseItem( &( pNewTCB->StateListItem ) );
	ListInitialiseItem( &( pNewTCB->EventListItem ) );
	listSET_LISTITEM_OWNER( &( pNewTCB->StateListItem ), pNewTCB );
	//优先级高的任务，排在事件链表前面,但 ListInsert() 是按value从小到大排序,所以要进行反转
	listSET_LISTITEM_VALUE( &( pNewTCB->EventListItem ), ( TickType_t )taskMAX_PRIORITIES - ( TickType_t )Priority );
	listSET_LISTITEM_OWNER( &( pNewTCB->EventListItem ), pNewTCB );

	//5.初始化其他变量
	pNewTCB->RunTimeCounter = 0UL;
	pNewTCB->NotifiedValue = 0;
	pNewTCB->NotifyState = taskNOTOFY_NOT_WAIT;

	/* 6.伪造一次异常返回(为了任务能被抢占、挂起、恢复),
	也就是说按照Cortex-M认识的格式布局(xPSR PC r0 LR r4-r11 )*/
	pNewTCB->pTopOfStack = pPortInitialiseStack( pTopOfStack, pTaskCode, pParameters );

	//7.生成句柄(指向TCB首地址)
	if ( ( void * )pHandle != NULL )
	{
		*pHandle = ( TaskHandle_t )pNewTCB;
	}
}
static void InitialiseTaskLists( void )
{
	UBaseType_t Priority;
	for ( Priority = ( UBaseType_t )0; Priority < ( UBaseType_t )taskMAX_PRIORITIES; Priority++ )
	{
		//不同优先级拥有各自的链表
		ListInitialise( &( pReadyTasksLists[Priority] ) );
	}
	ListInitialise( &DelayedTaskList1 );
	ListInitialise( &DelayedTaskList2 );
	ListInitialise( &PendingReadyList );
	ListInitialise( &WaitingDeleteTaskList );
	ListInitialise( &SuspendedTaskList );

	pDelayedTaskList = &DelayedTaskList1;
	pOverflowDelayedTaskList = &DelayedTaskList2;
}


static void AddNewTaskToReadyList( TCB_t *pNewTCB )
{
	PortEnterCritical();
	{
		//1.系统运行状态调整
		CurrentNumberOfTasks++;
		if ( pCurrentTCB == NULL )
		{
			pCurrentTCB = pNewTCB;
			if ( CurrentNumberOfTasks == ( UBaseType_t )1 )
			{
				InitialiseTaskLists();
			}
		}
		else
		{
			if ( SchedulerRunning == pdFALSE )
			{
				//抢占
				if ( pCurrentTCB->Priority <= pNewTCB->Priority )
				{
					pCurrentTCB = pNewTCB;
				}
			}
		}
		
		//2.插入就绪链表
		taskREADYLIST_ADDTASK( pNewTCB );
	}
	PortExitCritical();

	//3.上下文切换
	if ( SchedulerRunning != pdFALSE )
	{
		if ( pCurrentTCB->Priority < pNewTCB->Priority )
		{
			portREQUEST_TASK_SWITCH();
		}
	}
	
}
static void AddCurrentTaskToDelayList( TickType_t TicksToWait, const BaseType_t CanBlockIndefinitely )
{
	TickType_t TimeToWake;
	const TickType_t ConstTickCount = TickCount;
	//1.链表切换,优先级迭代
	if ( ListRemove( &( pCurrentTCB->StateListItem ) ) == ( UBaseType_t )0  )
	{
		taskREADYLIST_PRIORITY_ITERATE( pCurrentTCB->Priority );
	}
	
	//2.根据延时时间决定插入位置
	if ( TicksToWait == portMAX_DEALY && CanBlockIndefinitely == pdTRUE )
	{
		ListInsertCurPrevious( &SuspendedTaskList, &( pCurrentTCB->StateListItem ) );
	}
	else
	{
		TimeToWake = TickCount + TicksToWait;
		//溢出判断
		if ( TimeToWake < TickCount )
		{
			ListInsert( pOverflowDelayedTaskList, &( pCurrentTCB->StateListItem ));
		}
		else
		{
			ListInsert( pDelayedTaskList, &( pCurrentTCB->StateListItem ) );
			ResetNextTaskUnblockTime();
		}
	}
}

static void IdleTask( void * pParameters )
{
	for ( ;; )
	{
		TCB_t *pTCB;
		//1.释放被删除的任务资源
		while ( DeletedTasksWaitingCleanUp > 0 )
		{
			PortEnterCritical();
			{
				pTCB = listGET_ENDNEXT_LISTITEM_OWNER( &WaitingDeleteTaskList );
				( void )ListRemove( &( pTCB->StateListItem ) );
				--DeletedTasksWaitingCleanUp;
				--CurrentNumberOfTasks;
			}
			PortExitCritical();
			DeleteTCB( pTCB );
		}

		//2.同优先级任务时间片轮询
		if ( listGET_CURRENTLIST_LENTH( &( pReadyTasksLists[ taskIDLE_TASK_PRIORITY ] ) ) > ( UBaseType_t )1 )
		{
			portREQUEST_TASK_SWITCH();
		}
		
		//3.低功耗
		#if ( portENABLE_TICKLESS_IDLE == 1 )
		{
			//低功耗
		}
		#endif
	}
}

static void ResetNextTaskUnblockTime( void )
{
	/* 如果在 TaskSuspendAll()期间,有任务被解阻过,那么原先算好的“下一次最早唤醒时间”已经不可信,
	必须立刻重算,否则 tickless idle 可能会提前、甚至白白醒来。
	找出系统中所有“仍在延时中的任务”的最早唤醒 tick。*/
	TCB_t *pTCB;
	if ( listIS_EMPTY( pDelayedTaskList ) != pdFALSE )
	{
		NextTaskUnblockTime = portMAX_DEALY;
	}
	else
	{
		pTCB = ( TCB_t * )listGET_ENDNEXT_LISTITEM_OWNER( pDelayedTaskList );
		NextTaskUnblockTime = listGET_LISTITEM_VALUE( &( pTCB->StateListItem ) );
	}
}

static void DeleteTCB( TCB_t *pTCB )
{
	vHeapFree( pTCB->pStack );
	vHeapFree( pTCB );
}

