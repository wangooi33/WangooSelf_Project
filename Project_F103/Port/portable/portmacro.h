/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _PORTMACRO_H
#define _PORTMACRO_H
/* Includes ------------------------------------------------------------------*/

/* macro ---------------------------------------------------------------------*/
#ifndef configLIST_VOLATILE
	#define		configLIST_VOLATILE
#endif 

#define		portMAX_DELAY ( TickType_t )		0xffffffffUL
#define		PRIVILEGED_DATA			__attribute__((section("privileged_data")))
#define		PRIVILEGED_FUNCTION		__attribute__((section("privileged_functions")))



#define		portHEAP_MAXSIZE			(size_t (75 * 1024))
#define		portBYTE_ALIGNED			8
#define		portBYTE_ALIGNED_MASK		0x000f

/* enum ----------------------------------------------------------------------*/

/* types ---------------------------------------------------------------------*/
typedef uint32_t TickType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;


/* constants -----------------------------------------------------------------*/


/* global variable -----------------------------------------------------------*/


/* functions prototypes ------------------------------------------------------*/


#endif /* _PORTMACRO_H */





