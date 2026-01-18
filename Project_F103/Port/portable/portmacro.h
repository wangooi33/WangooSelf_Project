/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _PORTMACRO_H
#define _PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
/* global variable -----------------------------------------------------------*/
extern  uint32_t SystemCoreClock;

/* types ---------------------------------------------------------------------*/
typedef uint32_t TickType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef void * TaskHandle_t;
typedef void (*TaskFunction_t)( void * );
typedef uint32_t StackType_t;
/* macro --------------------------------------------------------------------*/

#define PortSVC_Handler				SVC_Handler
#define PortSysTick_Handler			SysTick_Handler

#define port_INLINE					__forceinline

//任务调用周期:1Hz->1s调用1000次
#define portCPU_CLOCK_HZ			( SystemCoreClock )
#define portTICK_RATE_HZ			( 1000 )
#define portMAX_DEALY				( TickType_t ) 0xFFFFFFFFUL

#define portHEAP_MAXSIZE			( ( size_t ) ( 75 * 1024 ) )
#define portBYTE_ALIGNED			8
#define portBYTE_ALIGNED_MASK		0x0007



//Cortex-M优先级位数
#ifdef __NVIC_PRIO_BITS
	#define portPIRO_BITS					__NVIC_PRIO_BITS
#else
	#define portPIRO_BITS					4
#endif


//Systick
#define portSYSTICK_CTRL					( * ( ( volatile uint32_t * ) 0xE000E010 ) )
#define portCTRL_ENABLE						( 1UL << 0UL )
#define portCTRL_TICKINT					( 1UL << 1UL )
#define portCTRL_CLKSOURCET					( 1UL << 2UL )
#define portSYSTICK_VAL						( * ( ( volatile uint32_t * ) 0xE000E018 ) )
#define portSYSTICK_LOAD					( * ( ( volatile uint32_t * ) 0xE000E014 ) )


//中断优先级(值越小,优先级越高)
#define portNVIC_IPR						( 0xE000E3F0 )
#define portUSE_FIREST_INTERRUPT_NUMBER		( 16 )
#define portMASK_AIRCR_PRIGROUP				( 0x07 << 8UL )
#define portAIRCR_PRIGROUP_SHIFT			( 8UL )
//能安全调用API函数的“最高中断优先级阈值”, > 5 可以调用。
#define portPRIORITY_MAX_INTERRUPT_SYSTEMCALL	5
#define portPRIORITY_MAX_SYSTEMCALL_LIMIT 	( portPRIORITY_MAX_INTERRUPT_SYSTEMCALL << (8 - portPIRO_BITS) )
//SHPR3:SCB->SHPR3基址:0xE000ED20,通过查询Cortex-M内核文档得知:PendSV优先级的地址在0xE000_ED22,则
#define portSCB_SYSPRI3_REG					( * ( ( volatile uint32_t * ) 0xE000ED20 ) )
//中断最低优先级(4位优先级系统):15,保证PendSV和Systick的优先级最低(值越大,优先级越低)
#define portPRIORITY_LOWEST_INTERRUPT  		0xF
#define portPRIORITY_PENDSV_SYSTICK			( portPRIORITY_LOWEST_INTERRUPT << ( 8 - portPIRO_BITS ) )
#define porSHPR3_PENDSV_PRIORITY			( ( ( uint32_t )portPRIORITY_PENDSV_SYSTICK ) << 16UL )
#define porSHPR3_SYSTICK_PRIORITY			( ( ( uint32_t )portPRIORITY_PENDSV_SYSTICK ) << 24UL )


//任务切换
#define portSCB_ICSR						( * ( ( volatile uint32_t * ) 0xE000ED04 ) )
#define portICSR_PENDSV_BITSET				( 1UL << 28UL )
#define portMASK_ICSR_VECTACTIVE			( 0xFFUL )


//FPU
#define portFPU_FPCCR						( * ( ( volatile uint32_t * ) 0xE000EF34 ) )
#define portFPCCR_ASPEN_AND_LSPEN_BITS		( 0x3UL << 30UL )


/* macro.功能使能开关 --------------------------------------------------------------*/

//Tick溢出处理
#define portENABLE_TICK_OVERFLOW_CHECK 		0
//低功耗模式
#define portENABLE_TICKLESS_IDLE 			0
//优先级校准
#define portENABLE_PRIORITY_CALIBRATION		1
//浮点计算功能(注意:Cortex-M3不带FPU)
#define portENABLE_FPU 						0


/* function -------------------------------------------------------------------------*/

//请求一次 PendSV 上下文切换,仅做请求,并不立刻切换。15->等待全系统范围内的读 + 写完成。
#define		portREQUEST_TASK_SWITCH()			\
{												\
	portSCB_ICSR = portICSR_PENDSV_BITSET;		\
	__dsb(15);									\
	__isb(15);									\
}


/* functions prototypes ------------------------------------------------------------*/
void *pHeapMalloc( size_t WantedSize );
void vHeapFree( void * pDel );

void PortRaiseBASEPRI( void );
void PortSetBASEPRI( uint32_t ulBASEPRI );

void PortEnterCritical( void );
void PortExitCritical( void );
StackType_t *pPortInitialiseStack( StackType_t *pTopOfStack, TaskFunction_t pCode, void *pParameters );
UBaseType_t PortStartScheduler( void );


#ifdef __cplusplus
}
#endif

#endif /* _PORTMACRO_H */





