/* Includes ------------------------------------------------------------------*/
#include "beep.h"
/* private variable ----------------------------------------------------------*/

/* functions implementation --------------------------------------------------*/
void Beep_StartupTone( void )
{
	static uint8_t Startup = 1;
	static uint8_t wcount = 0;
	if ( Startup )
	{
		wcount++;
		BEEP_ON;
		if ( wcount >= 100 )
		{
			wcount = 0;
			Startup = 0;
			BEEP_OFF;
		}
	}
}

