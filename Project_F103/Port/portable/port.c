#include "portmacro.h"
#include "task.h"

/*--------------------------------------------------------------------------------------*/

//临界区嵌套计数,=0视为退出临界区,未开启调度器时默认为0xa5,开启调度器后,会把它置为0
static uint8_t CritialNestCount = 0xa5;
static uint32_t IPR_PriorityBitCnt = 0;
static uint8_t MaxSysCallPriority = 0;
extern TCB_t * volatile pCurrentTCB;

/*--------------------------------------------------------------------------------------*/
static void PortSetupTimerInterrupt( void );
static void TaskExitError( void );

/* 汇编 */
void PortEnableVFP( void );
void PortStartFirstTask( void );

/* 中断 */
void PortSysTick_Handler( void );
void PortSVC_Handler( void );
void PortPendSV_Handler(void);

/*--------------------------------------------------------------------------------------*/
#define		portASSERT( x )		if( ( x ) == 0 ) { PortRaiseBASEPRI();for( ;; );}

/*--------------------------------------------------------------------------------------*/

void PortRaiseBASEPRI( void )
{
	uint32_t Newbasepri = portPRIORITY_MAX_SYSTEMCALL_LIMIT;
	__asm
	{
		msr basepri, Newbasepri
		dsb
		isb
	}
}
void PortSetBASEPRI( uint32_t ulBASEPRI )
{
	__asm
	{
		msr basepri, ulBASEPRI
	}
}

void PortEnterCritical( void )
{
	//关中断;任务不能切换
	PortRaiseBASEPRI();
	CritialNestCount++;//CritialNestCount == 0时,认为离开临界区,并恢复中断状态
	if( CritialNestCount == 1 )
	{
		//portASSERT( ( portSCB_ICSR & portMASK_ICSR_VECTACTIVE ) == 0 );
	}
}
void PortExitCritical( void )
{
	//portASSERT( CritialNestCount );
	CritialNestCount--;
	if ( CritialNestCount == 0 )
	{
		PortSetBASEPRI( 0 );
	}
}

StackType_t *pPortInitialiseStack( StackType_t *pTopOfStack, TaskFunction_t pCode, void *pParameters )
{
	/*------ 硬件压栈 ------    */
	//xPSR:bit24=1,CPU接下来用Thumb状态执行任务。
	pTopOfStack--;
	*pTopOfStack = 0x01000000;
	
	//PC:在Cortex-M上,函数指针的bit0 = 1,表示Thumb函数。PC的bit0不能等于1,必须为偶地址。
	pTopOfStack--;
	*pTopOfStack = ( ( StackType_t )pCode ) & ( ( StackType_t )0xFFFFFFFEUL );

	//LR
	pTopOfStack--;
	*pTopOfStack = ( StackType_t )TaskExitError;

	//r12,r3,r2,r1
	pTopOfStack -= 5;
	
	//r0
	*pTopOfStack = ( StackType_t )pParameters;


	/*------ 软件压栈 ------    */
	//LR:返回到 Thread mode
	pTopOfStack--;
	*pTopOfStack = 0xFFFFFFFD;

	//r11,r10,r9,r8,r7,r6,r5
	pTopOfStack -= 8;

	//r4
	return pTopOfStack;
}

UBaseType_t PortStartScheduler( void )
{
	//1.优先级校准
	#if ( portENABLE_PRIORITY_CALIBRATION == 1 )
	{
		//0(__initial_sp) ~ 15(SysTick_Handler)
		volatile uint8_t * const FirstUserRegisterPriority = ( uint8_t * )( portNVIC_IPR + portUSE_FIREST_INTERRUPT_NUMBER );
		volatile uint8_t PriorityBitMask;
		uint32_t OriginPrioity;

		/* 1.计算优先级有效位个数 */
		//计算NVIC->IPR[x]中的有效位(写回读)
		OriginPrioity = *FirstUserRegisterPriority;
		*FirstUserRegisterPriority = ( uint8_t )0xFF;
		PriorityBitMask = *FirstUserRegisterPriority;
		//portASSERT( PriorityBitMask == ( PriorityBitMask & portPRIORITY_PENDSV_SYSTICK ) );
		MaxSysCallPriority = PriorityBitMask & portPRIORITY_MAX_INTERRUPT_SYSTEMCALL;
		
		//手动计算优先级有效位
		IPR_PriorityBitCnt = 7;
		while ( 0x80 == ( PriorityBitMask & 0x80 ) )
		{
			IPR_PriorityBitCnt--;
			PriorityBitMask <<= ( uint8_t )0x01;
		}
		//两次的计算结果进行对比
		//portASSERT( ( 7 - IPR_PriorityBitCnt ) == __NVIC_PRIO_BITS );
		//portASSERT( ( 7 - IPR_PriorityBitCnt ) == portPIRO_BITS );
		
		//对齐 + 掩码
		IPR_PriorityBitCnt <<= portAIRCR_PRIGROUP_SHIFT;
		IPR_PriorityBitCnt &= portMASK_AIRCR_PRIGROUP;
		//还原
		*FirstUserRegisterPriority = OriginPrioity;
	}
	#endif

	//2.调整PendSV、Systick优先级
	portSCB_SHPR3 |= porSHPR3_PENDSV_PRIORITY;
	portSCB_SHPR3 |= porSHPR3_SYSTICK_PRIORITY;

	//3.系统时钟使能
	PortSetupTimerInterrupt();

	//4.进入临界区嵌套次数
	CritialNestCount = 0;

	//5.启动FPU(FPU:浮点运算单元(硬件),VFP:浮点运算架构)
	#if ( portENABLE_FPU == 1 )
	{
		PortEnableVFP();
		portFPU_FPCCR |= portFPCCR_ASPEN_AND_LSPEN_BITS;
	}
	#endif

	//6.启动第一个任务
	PortStartFirstTask();

	/* Should not get here! */
	return 0;
}

void PortSysTick_Handler( void )
{
	PortRaiseBASEPRI();
	{
		if ( SysTickCount() != pdFALSE )
		{
			//请求一次PendSV中断
			portSCB_ICSR = portICSR_PENDSV_BITSET;
		}
	}
	PortSetBASEPRI(0);
}

/*--------------------------------------------------------------------------------------*/

static void PortSetupTimerInterrupt( void )
{
	#if ( portENABLE_TICKLESS_IDLE == 1 )
	{
		//低功耗
	}
	#endif

	portSYSTICK_CTRL = 0UL;
	portSYSTICK_VAL = 0UL;

	portSYSTICK_LOAD = ( portCPU_CLOCK_HZ / portTICK_RATE_HZ ) - 1UL;
	portSYSTICK_CTRL = ( portCTRL_ENABLE | portCTRL_TICKINT | portCTRL_CLKSOURCET );
}

static void TaskExitError( void )
{
	/* 实现任务的函数不能退出或试图返回给调用者,如果任务想要退出,应调用TaskDelete。
	所以,LR中存储这个函数地址,来查看任务是否返回 */
	portASSERT( CritialNestCount == ~0UL );
	PortRaiseBASEPRI();
	for( ;; );
}

/*--------------------------------------------------------------------------------------*/

/**
 * @note:
 */

__asm void PortEnableVFP( void )
{
	PRESERVE8
	ldr.w r0, =0xE000ED88
	ldr r1, [r0]
	orr r1, r1, #( 0xf << 20 )
	str r1, [r0]
	bx r14
	nop
}

/**
 * @note:
 *    把主栈指针 MSP 恢复成“芯片刚上电时应有的状态”
 *    (1)ldr r0, =0xE000ED08	:寄存器地址
 *    (2)ldr r0, [r0]			:向量表基地址
 *    (3)ldr r0, [r0]			:vector[0]
 */
__asm void PortStartFirstTask( void )
{
	ldr r0, =0xE000ED08
	ldr r0, [r0]
	ldr r0, [r0]
	msr msp, r0

	mov r0, #0
	msr control, r0

	cpsie i
	cpsie f
	dsb
	isb

	svc 0
	nop
	nop
}

/**
 * @note:
 *    (1)r3 = &pxCurrentTCB
 *    (2)r1 = pxCurrentTCB
 *    (3)r0 = pxCurrentTCB->pxTopOfStack
 */
__asm void PortSVC_Handler( void )
{
	IMPORT pCurrentTCB
		
	ldr r3, =pCurrentTCB
	ldr r1, [r3]
	ldr r0, [r1]

	ldmia r0!, {r4-r11, r14}
	msr psp, r0
	isb
	
	mov r0, #0
	msr basepri, r0

	bx r14
}

/**
 * @brief:上下文保存,上下文切换
 * @note:
 *    (1)ldr r3, =pxCurrentTCB	:&pCurrentTCB
 *    (2)ldr r2, [r3]			:*(&pCurrentTCB) = pCurrentTCB
 *
 *    (3)ldr r1, [r3]			:pCurrentTCB
 *    (4)ldr r0, [r1]			:pCurrentTCB->pTopOfStack
 */
__asm void PortPendSV_Handler(void)
{
	extern CritialNestCount;
	extern pCurrentTCB;
	extern TaskSwitchContext;

	
	PRESERVE8
	
	mrs r0, psp
	isb

	ldr r3, =pCurrentTCB
	ldr r2, [r3]

	#if portENABLE_FPU
		tst r14, #0x10
		it eq
		vstmdbeq r0!, {s16-s31}
	#endif

	stmdb r0!, {r4-r11, r14}

	str r0, [r2]

	stmdb sp!, {r0, r3}
	mov r0, #portPRIORITY_MAX_SYSTEMCALL_LIMIT
	msr basepri, r0
	dsb
	isb
	bl TaskSwitchContext
	mov r0, #0
	msr basepri, r0
	ldmia sp!, {r0, r3}

	ldr r1, [r3]
	ldr r0, [r1]

	ldmia r0!, {r4-r11, r14}

	#if portENABLE_FPU
		tst r14, #0x10
		it eq
		vldmiaeq r0!, {s16-s31}
	#endif

	msr psp, r0
	isb

	bx r14
}

