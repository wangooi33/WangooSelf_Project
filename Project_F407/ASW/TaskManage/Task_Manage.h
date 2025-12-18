/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _TASK_MANAGE_H
#define _TASK_MANAGE_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "Task_Manage_cfg.h"
/* types ------------------------------------------------------------*/
typedef struct
{
	TaskHandle_t Root_TaskHandle;
	TaskHandle_t Task1_TaskHandle;
}wTaskHandle_st;

/* constants --------------------------------------------------------*/

/* macro ------------------------------------------------------------*/

/* functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void Root_Task(void *pvParameters);
/* defines -----------------------------------------------------------*/


#endif /* _TASK_MANAGE_H */