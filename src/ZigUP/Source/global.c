#include "ZComDef.h"

uint32 float_NaN = 0x7F800001;

float EXT_Temperature;
float EXT_Humidity;
float ADC_Voltage;
float CPU_Temperature;
uint16 DIG_IN = 0;

byte zclZigUP_TaskID;
uint16 zclZigUPSeqNum=0;

volatile uint32 S0=0;
volatile uint8 STATE_LIGHT=0;
volatile uint8 STATE_LED=0;

uint8 TEMP_SENSOR=0;
