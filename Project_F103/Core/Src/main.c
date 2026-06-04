/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "AT24Cxx.h"
#include "NM25Qxx.h"
#include "IAP.h"
#include "delay.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
const char SoftWareID[] = "W010";
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TASK_PERIOD_500US     1
#define TASK_PERIOD_1MS       1
#define TASK_PERIOD_2MS       2
#define TASK_PERIOD_5MS       5
#define TASK_PERIOD_10MS      10

#define TASK_PERIOD_20MS      20
#define TASK_PERIOD_50MS      50
#define TASK_PERIOD_100MS     100
#define TASK_PERIOD_500MS     500
#define TASK_PERIOD_1000MS    1000


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define IWDG_ENBALE 0
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t SystemRunTime_1ms = 0;
volatile uint64_t SystemRunTime_500us = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Error_Handler(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint32_t GetTick_1ms(void)
{
  return SystemRunTime_1ms;
}
uint64_t GetTick_500us(void)
{
  return SystemRunTime_500us;
}

void Task_500us()
{
	
}
void Task_1ms()
{
	IAP_Cyclic();
}
void Task_2ms()
{

}
void Task_5ms()
{

}
void Task_10ms()
{

}
void Task_20ms()
{	

}
void Task_50ms()
{
	
}
void Task_100ms()
{
	
}
void Task_500ms()
{
	LED1_TOGGLE;
}
void Task_1000ms()
{
	LED0_TOGGLE;
	//BEEP_TOGGLE;
#if IWDG_ENBALE
	HAL_IWDG_Refresh(&hiwdg);
#endif
}

void TaskSchedule()
{
	static uint64_t startValue_500us = 0;
    static uint32_t startValue_1ms = 0;
    static uint32_t startValue_2ms = 0;
    static uint32_t startValue_5ms = 0;
    static uint32_t startValue_10ms = 0;
    static uint32_t startValue_20ms = 0;
    static uint32_t startValue_50ms = 0;
    static uint32_t startValue_100ms = 0;
    static uint32_t startValue_500ms = 0;
    static uint32_t startValue_1000ms = 0;
    if (GetTick_500us() - startValue_500us >= TASK_PERIOD_500US)
    {
        startValue_500us = GetTick_500us();
        Task_500us();
    }
    if (GetTick_1ms() - startValue_1ms >= TASK_PERIOD_1MS)
    {
        startValue_1ms = GetTick_1ms();
        Task_1ms();
    }
    if (GetTick_1ms() - startValue_2ms >= TASK_PERIOD_2MS)
    {
    	startValue_2ms = GetTick_1ms();
        Task_2ms();
    }
    if (GetTick_1ms() - startValue_5ms >= TASK_PERIOD_5MS)
    {
    	startValue_5ms = GetTick_1ms();
        Task_5ms();
    }
    if (GetTick_1ms() - startValue_10ms >= TASK_PERIOD_10MS)
    {
    	startValue_10ms = GetTick_1ms();
        Task_10ms();
    }
    if (GetTick_1ms() - startValue_20ms >= TASK_PERIOD_20MS)
    {
    	startValue_20ms = GetTick_1ms();
        Task_20ms();
    }
    if (GetTick_1ms() - startValue_50ms >= TASK_PERIOD_50MS)
    {
    	startValue_50ms = GetTick_1ms();
        Task_50ms();
    }
    if (GetTick_1ms() - startValue_100ms >= TASK_PERIOD_100MS)
    {
    	startValue_100ms = GetTick_1ms();
        Task_100ms();
    }
    if (GetTick_1ms() - startValue_500ms >= TASK_PERIOD_500MS)
    {
    	startValue_500ms = GetTick_1ms();
        Task_500ms();
    }
    if (GetTick_1ms() - startValue_1000ms >= TASK_PERIOD_1000MS)
    {
        startValue_1000ms = GetTick_1ms();
	    Task_1000ms();
    }

}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  SCB->VTOR = APP_START_ADDRESS;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_IWDG_Init();
  MX_SPI2_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim1);
  HAL_TIM_Base_Start(&htim2);
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1,gU1RxBuf,U1BUF_MAXSIZE);
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2,gU2RxBuf,U2BUF_MAXSIZE);
  DWT_Init();
  AT24Cxx_Init();
  Flash_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    TaskSchedule();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
