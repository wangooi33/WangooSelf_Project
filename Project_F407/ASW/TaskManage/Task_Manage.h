/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _TASK_MANAGE_H
#define _TASK_MANAGE_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "Task_Manage_cfg.h"

/* macro ---------------------------------------------------------------------*/

/* enum ----------------------------------------------------------------------*/

/* types ---------------------------------------------------------------------*/
typedef struct
{
	TaskHandle_t Root_TaskHandle;
	TaskHandle_t Task1_TaskHandle;
	TaskHandle_t Task2_TaskHandle;
}wTaskHandle_st;


/* constants -----------------------------------------------------------------*/


/* global variable -----------------------------------------------------------*/
extern wTaskHandle_st wTaskHandle;


/* functions prototypes ------------------------------------------------------*/
void Root_Task(void *pvParameters);
void Error_Handler(void);


#endif /* _TASK_MANAGE_H */
