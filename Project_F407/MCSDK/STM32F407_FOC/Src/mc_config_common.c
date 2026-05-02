
/**
  ******************************************************************************
  * @file    mc_config_common.c
  * @author  Motor Control SDK Team,ST Microelectronics
  * @brief   Motor Control Subsystem components configuration and handler structures.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044,the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

//cstat -MISRAC2012-Rule-21.1
#include "main.h" //cstat !MISRAC2012-Rule-21.1
//cstat +MISRAC2012-Rule-21.1
#include "mc_type.h"
#include "parameters_conversion.h"
#include "mc_parameters.h"
#include "mc_config_common.h"

/* USER CODE BEGIN Additional include */

/* USER CODE END Additional include */

/* USER CODE BEGIN Additional define */

/* USER CODE END Additional define */

/**
  * Virtual temperature sensor parameters Motor 1.
  */
NTC_Handle_t TempSensor_M1 =
{
  .bSensorType     = VIRTUAL_SENSOR,
  .hExpectedTemp_d = 555,
  .hExpectedTemp_C = M1_VIRTUAL_HEAT_SINK_TEMPERATURE_VALUE,
};

/**
  * Bus voltage sensor parameters Motor 1.
  */
RegConv_t VbusRegConv_M1 =
{
  .regADC                   = ADC3,
  .channel                  = MC_ADC_CHANNEL_7,
  .samplingTime             = M1_VBUS_SAMPLING_TIME,
  .data                     = 1 + (uint16_t)((NOMINAL_BUS_VOLTAGE_V * 65536) / (ADC_REFERENCE_VOLTAGE / VBUS_PARTITIONING_FACTOR))
};

RDivider_Handle_t BusVoltageSensor_M1 =
{
  ._Super =
  {
    .SensorType               = REAL_SENSOR,
    .ConversionFactor         = (uint16_t)(ADC_REFERENCE_VOLTAGE / VBUS_PARTITIONING_FACTOR),
  },

  .OverVoltageThreshold       = OVERVOLTAGE_THRESHOLD_d,
  .OverVoltageThresholdLow    = OVERVOLTAGE_THRESHOLD_d,
  .OverVoltageHysteresisUpDir = true,
  .UnderVoltageThreshold      =  UNDERVOLTAGE_THRESHOLD_d,
};

PWMC_Handle_t *pwmcHandle[NBR_OF_MOTORS];

/* USER CODE BEGIN Additional configuration */

/* USER CODE END Additional configuration */

/******************* (C) COPYRIGHT 2026 STMicroelectronics *****END OF FILE****/

