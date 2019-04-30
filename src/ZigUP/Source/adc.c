#include "ZComDef.h"
#include "onboard.h"
#include "bitmasks.h"
#include "adc.h"

uint16 ADC_Read(void)
{
  int16 result = 0;
  ADCCON3 = b00110000; // internal reference, decimation 512, channel 0, start conversation
  while (!(ADCCON1 & (1<<7)));	// wait for conversation to finish
  result = (uint16)ADCL;
  result |= (uint16)(ADCH << 8);
  if (result < 0) result = 0;
  result >>= 2;
  return result;
}

uint16 ADC_Read_Avg(void)
{
  uint32 result = 0;
  for (uint8 i=0; i<32; i++) result += ADC_Read();
  return (uint16)(result / 32);
}

float ADC_GetVoltage(void)
{
  return ADC_Read_Avg() * 0.003949905;
}

uint16 ADC_Temperature(void)
{
  int16 result = 0;
  ADCCON3 = b00111110; // internal reference, decimation 512, temperature, start conversation
  while (!(ADCCON1 & (1<<7)));	// wait for conversation to finish
  result = (uint16)ADCL;
  result |= (uint16)(ADCH << 8);
  if (result < 0) result = 0;
  result >>= 2;
  return result;
}

uint16 ADC_Temperature_Avg(void)
{
  TR0 = 1;        // connect the temperature sensor to the SOC_ADC
  ATEST = 1;      // Enables the temperature sensor
  
  uint32 result = 0;
  for (uint8 i=0; i<32; i++) result += ADC_Temperature();
  
  ATEST = 0;      // Disables the temperature sensor
  TR0 = 0;        // disconnect the temperature sensor from the SOC_ADC
  
  return (uint16)(result / 32);
}

float ADC_GetTemperature(void)
{
  return ADC_Temperature_Avg() * 0.0556 - 303.89;
}
