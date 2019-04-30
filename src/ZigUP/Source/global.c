#include "ZComDef.h"

float EXT_Temperature = -1000;
float EXT_Humidity = -1000;
float ADC_Voltage = -1000;
float CPU_Temperature = -1000;
uint16 DIG_IN = 0;

byte zclZigUP_TaskID;
uint16 zclZigUPSeqNum=0;

volatile uint32 S0=0;
volatile uint8 STATE_LIGHT=0;
volatile uint8 STATE_LED=0;
