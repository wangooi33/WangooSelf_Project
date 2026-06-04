#include "IAP.h"
#include <stdio.h>

IAP_Info_t IAP_Info;

void IAP_ChangeUpdateFlag(uint8_t isEnable)
{
    uint32_t Val = (isEnable) ? APP_UPDATE_FLAG : 0;
    AT24Cxx_Write(0,(uint8_t *)&Val,4);
}
void IAP_Cyclic(void)
{
    switch (IAP_Info.IAPRunSt)
    {
        case IAPRunSt_Idle:

            break;
        case IAPRunSt_Change:
            IAP_ChangeUpdateFlag(1);
			AT24Cxx_Read(0,IAP_Info.UpdateFlag,4);
            IAP_Info.IAPRunSt = IAPRunSt_JumpToBoot;
            break;
        case IAPRunSt_JumpToBoot:
            NVIC_SystemReset();
            break;
    }
}
void IAP_RxProcess(uint8_t *pData, uint16_t Size)
{
    if (pData[0] == 0x3A && pData[1] == 0x3A)
    {
        switch (pData[2])
        {
            case 0xF1:
                IAP_Info.IAPRunSt = IAPRunSt_Change;
                break;
        }
    }
}
