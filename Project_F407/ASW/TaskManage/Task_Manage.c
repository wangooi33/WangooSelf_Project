/* Includes ------------------------------------------------------------------*/
#include "Task_Manage.h"
/* typedef -----------------------------------------------------------*/
wTaskHandle_st wTaskHandle;
BaseType_t xReturn = pdPASS;
/* define ------------------------------------------------------------*/

/* macro -------------------------------------------------------------*/

/* variables ---------------------------------------------------------*/

/* function prototypes -----------------------------------------------*/

/* function implementation--------------------------------------------*/
void AllTaskCreat(void *pvParameters);
void Task01(void *pvParameters);
void Error_TaskCreate(uint8_t taskid);

void Root_Task(void *pvParameters)
{
	uint8_t taskid = 0;
	xReturn = xTaskCreate((TaskFunction_t)AllTaskCreat, 
    					  (const char*   )"root_task", 
    					  (uint16_t      )512, 
    					  (void *        )NULL, 
    					  (UBaseType_t   )0,
    					  (TaskHandle_t *)&wTaskHandle.Root_TaskHandle);
	if(pdPASS != xReturn)
		Error_TaskCreate(taskid);
}
void AllTaskCreat(void *pvParameters)
{
	taskENTER_CRITICAL();

	uint8_t taskid = 1;
	xTaskCreate((TaskFunction_t)Task01, 
				(const char*   )"Task01", 
				(uint16_t      )128, 
				(void *        )NULL, 
				(UBaseType_t   )0,
				(TaskHandle_t *)&wTaskHandle.Task1_TaskHandle);
	if(pdPASS != xReturn)
		Error_TaskCreate(taskid);
	
	taskid++;

	

	vTaskDelete(wTaskHandle.Root_TaskHandle);
	taskEXIT_CRITICAL();
}

void Task01(void *pvParameters)
{
	vTaskDelay(5);
}

void Error_TaskCreate(uint8_t taskid)
{
	if (taskid == 0)
	{
		for (;;);
	}
	else
	{
		
	}
}

void vApplicationIdleHook(void)
{
	/* 空闲任务钩子 */
    /* 低功耗、统计、喂狗都可以放这 */
}
void vApplicationStackOverflowHook(TaskHandle_t xTask,char *pcTaskName)
{
	/* 栈溢出钩子 */
	(void)xTask;
	(void)pcTaskName;

	taskDISABLE_INTERRUPTS();
	for (;;);
}
void vApplicationTickHook(void)
{
	/* 当内核时钟跳动一次时，在内核上下文里，顺手执行一下钩子函数 */
}
void vApplicationMallocFailedHook(void)
{
	/* 内存分配失败钩子 */
	taskDISABLE_INTERRUPTS();
	for (;;);
}


