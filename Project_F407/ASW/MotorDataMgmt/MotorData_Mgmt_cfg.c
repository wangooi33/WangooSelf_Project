#include "MotorData_Mgmt_cfg.h"


MotorData_HalfWordVarMap_st MotorData_HalfWordVarMap[MotorData_HalfWordVarMapIndex_Max] =
{
	/*									  Index			pMeta			       rxvalue  reserve*/
	[MotorData_HalfWordVarMapIndex_00] = {0x00,  &MotorData_FromADC.Motor_IU	,0x00    ,0x00},
	[MotorData_HalfWordVarMapIndex_01] = {0x01,  &MotorData_FromADC.Motor_IV	,0x00    ,0x00}, 
	[MotorData_HalfWordVarMapIndex_02] = {0x02,  &MotorData_FromADC.Motor_IW	,0x00    ,0x00}, 
	[MotorData_HalfWordVarMapIndex_03] = {0x03,  &MotorData_FromADC.Motor_VBUS	,0x00    ,0x00}, 
	[MotorData_HalfWordVarMapIndex_04] = {0x04,  &MotorData_FromADC.Motor_IBUS	,0x00    ,0x00}, 
	[MotorData_HalfWordVarMapIndex_05] = {0x05,  &MotorData_FromADC.Motor_EMFU	,0x00    ,0x00}, 
	[MotorData_HalfWordVarMapIndex_06] = {0x06,  &MotorData_FromADC.Motor_EMFV	,0x00    ,0x00},
	[MotorData_HalfWordVarMapIndex_07] = {0x07,  &MotorData_FromADC.Motor_EMFW	,0x00    ,0x00}, 

};
#if 0
MotorData_BitVarMap_st MotorData_BitVarMap[MotorData_BitVarMapIndex_Max] = 
{

};
MotorData_ByteVarMap_st MotorData_ByteVarMap[MotorData_ByteVarMapIndex_Max] =
{

};

MotorData_WordVarMap_st MotorData_WordVarMap[MotorData_WordVarMapIndex_Max] =
{

};
#endif


