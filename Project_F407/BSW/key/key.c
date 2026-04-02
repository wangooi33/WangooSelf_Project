#include "key.h"
#include "BDC_Control.h"

KeyInfo_t KeyInfo;
uint8_t KeyBit8Map = 0;

void KeyTask_Cyclic( void )
{
	static uint8_t i = 0;
	static uint16_t wcount = 0;
	switch (KeyInfo.KeyEventState)
	{
		case KeyEvent_Idle:
			KeyInfo.KeyEventState = KeyEvent_Monitor;
			break;
		case KeyEvent_Monitor:
			if ( GPIO_PIN_SET == HAL_GPIO_ReadPin(KEY1_GPIO_Port,KEY1_Pin) )
			{
				//按键按下为高电平
				KeyInfo.Key1ValidLevel++;
				if ( KeyInfo.Key1ValidLevel >= 2 )
				{
					KeyInfo.Key1ValidLevel = 0;
					KeyInfo.KeyEventState = Key1_Active;
				}
			}
			else
			{
				KeyInfo.Key1ValidLevel = 0;
			}
			
			if ( GPIO_PIN_SET == HAL_GPIO_ReadPin(KEY2_GPIO_Port,KEY2_Pin) )
			{
				//按键按下为高电平
				KeyInfo.Key2ValidLevel++;
				if ( KeyInfo.Key2ValidLevel >= 2 )
				{
					KeyInfo.Key2ValidLevel = 0;
					KeyInfo.KeyEventState = Key2_Active;
				}
			}
			else
			{
				KeyInfo.Key2ValidLevel = 0;
			}
			
			if ( GPIO_PIN_SET == HAL_GPIO_ReadPin(KEY3_GPIO_Port,KEY3_Pin) )
			{
				//按键按下为高电平
				KeyInfo.Key3ValidLevel++;
				if ( KeyInfo.Key3ValidLevel >= 2 )
				{
					KeyInfo.Key3ValidLevel = 0;
					KeyInfo.KeyEventState = Key3_Active;
				}
			}
			else
			{
				KeyInfo.Key3ValidLevel = 0;
			}
			
			if ( GPIO_PIN_SET == HAL_GPIO_ReadPin(KEY4_GPIO_Port,KEY4_Pin) )
			{
				//按键按下为高电平
				KeyInfo.Key4ValidLevel++;
				if ( KeyInfo.Key4ValidLevel >= 2 )
				{
					KeyInfo.Key4ValidLevel = 0;
					KeyInfo.KeyEventState = Key4_Active;
				}
			}
			else
			{
				KeyInfo.Key4ValidLevel = 0;
			}
			
			if ( GPIO_PIN_SET == HAL_GPIO_ReadPin(KEY5_GPIO_Port,KEY5_Pin) )
			{
				//按键按下为高电平
				KeyInfo.Key5ValidLevel++;
				if ( KeyInfo.Key5ValidLevel >= 2 )
				{
					KeyInfo.Key5ValidLevel = 0;
					KeyInfo.KeyEventState = Key5_Active;
				}
			}
			else
			{
				KeyInfo.Key5ValidLevel = 0;
			}
			break;
		case Key1_Active:
			BDC_Info.BDC_Dutyfactor = 1000;
			BDC_SetPulse(BDC_Info.BDC_Dutyfactor);
			BDC_Enable();
			KeyInfo.KeyEventState = KeyEvent_Monitor;
			break;
		case Key2_Active:
			BDC_Disable();
			KeyInfo.KeyEventState = KeyEvent_Monitor;
			break;
		case Key3_Active:
			BDC_Info.BDC_Dutyfactor += 200;
			BDC_SetPulse(BDC_Info.BDC_Dutyfactor);
			KeyInfo.KeyEventState = KeyEvent_Monitor;
			break;
		case Key4_Active:
			BDC_Info.BDC_Dutyfactor -= 200;
			BDC_SetPulse(BDC_Info.BDC_Dutyfactor);
			KeyInfo.KeyEventState = KeyEvent_Monitor;
			break;
		case Key5_Active:
			BDC_Disable();
			wcount++;
			if ( wcount >= 3 )
			{
				wcount = 0;
				BDC_SetDirection( (++i % 2) ? MOTOR_FWD : MOTOR_REV );
				BDC_Enable();
				KeyInfo.KeyEventState = KeyEvent_Monitor;
			}
			break;

		default:
			KeyInfo.KeyEventState = KeyEvent_Idle;
			break;
		
	}
}


