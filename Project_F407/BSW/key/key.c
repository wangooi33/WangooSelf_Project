/* Includes ------------------------------------------------------------------*/
#include "key.h"
#include "BDC_Control.h"
/* private variable ----------------------------------------------------------*/
static uint8_t key_cnt[KEY_NUM] = {0};
static uint8_t key_state[KEY_NUM] = {0}; // 0:松开 1:按下
// GPIO映射
static GPIO_TypeDef* KEY_PORT[KEY_NUM] = 
{
    KEY1_GPIO_Port, 
	KEY2_GPIO_Port, 
	KEY3_GPIO_Port,
    KEY4_GPIO_Port, 
    KEY5_GPIO_Port
};
static uint16_t KEY_PIN[KEY_NUM] = 
{
    KEY1_Pin, 
	KEY2_Pin, 
	KEY3_Pin, 
	KEY4_Pin, 
	KEY5_Pin
};
/* functions implementation --------------------------------------------------*/
static KeyEvent_t KeyScan(void)
{
	for (int i = 0; i < KEY_NUM; i++)
	{
		if ( HAL_GPIO_ReadPin(KEY_PORT[i], KEY_PIN[i]) == GPIO_PIN_SET )
		{
			if ( key_cnt[i] < DEBOUNCE_CNT )
			{
				key_cnt[i]++;
			}
			if ( key_cnt[i] >= DEBOUNCE_CNT && key_state[i] == 0 )
			{
				key_state[i] = 1;
				return (KeyEvent_t)(KEY1_PRESS + i);
			}
		}
		else
		{
			key_cnt[i] = 0;
			key_state[i] = 0;
		}
	}
	return KEY_NONE;
}

void KeyTask_Cyclic(void)
{
    KeyEvent_t key = KeyScan();
    switch ( key )
    {
        case KEY1_PRESS:   //启动(正转)
			BDC_Info.PointRPM = 60;
			//BDC_Info.PointPosition += BDC_PPR;
			BDC_Enable();
            break;

        case KEY2_PRESS:   //停止
            BDC_Disable();
            break;

        case KEY3_PRESS:   //加速
			BDC_Info.PointRPM += 5;
            break;

        case KEY4_PRESS:   //减速
			BDC_Info.PointRPM -= 5;
            break;

        case KEY5_PRESS:   //反转
			BDC_Info.PointRPM = -BDC_Info.PointRPM;
            break;

        default:
            break;
    }
}

