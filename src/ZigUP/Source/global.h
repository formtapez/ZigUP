#include "ZComDef.h"

extern uint32 float_NaN;

extern float EXT_Temperature;
extern float EXT_Humidity;
extern float ADC_Voltage;
extern float CPU_Temperature;
extern uint16 DIG_IN;

extern byte zclZigUP_TaskID;
extern uint16 zclZigUPSeqNum;

extern volatile uint32 S0;
extern volatile uint8 STATE_LIGHT;
extern volatile uint8 STATE_LED;

extern uint8 TEMP_SENSOR;

#define REPORT_REASON_TIMER     0
#define REPORT_REASON_KEY       1
#define REPORT_REASON_DIGIN     2
