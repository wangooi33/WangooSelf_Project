#ifndef __KEY_H
#define __KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum KeyST
{
	KeyEvent_Idle,
	KeyEvent_Monitor,
	Key1_Active,
	Key2_Active,
	Key3_Active,
	Key4_Active,
	Key5_Active,
}KeyEventState_t;

typedef struct
{
	KeyEventState_t KeyEventState;
	uint8_t Key1ValidLevel;
	uint8_t Key2ValidLevel;
	uint8_t Key3ValidLevel;
	uint8_t Key4ValidLevel;
	uint8_t Key5ValidLevel;
}KeyInfo_t;

extern KeyInfo_t KeyInfo;
void KeyTask_Cyclic( void );




#endif /* __KEY_H */

