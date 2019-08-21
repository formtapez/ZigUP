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

extern int8 TEMP_SENSOR;

#define REPORT_REASON_TIMER     0
#define REPORT_REASON_KEY       1
#define REPORT_REASON_DIGIN     2

#define ds18b20_MAX_DEVICES               16


extern uint8 ds18b20_ROM[8]; // ROM Bit
extern uint8 ds18b20_lastDiscrep; // last discrepancy
extern uint8 ds18b20_doneFlag; // Done flag
extern uint8 ds18b20_FoundROM[ds18b20_MAX_DEVICES][8]; // table of found ROM codes
extern uint8 ds18b20_numROMs;
extern uint8 ds18b20_dowcrc;
extern volatile uint8 ds18b20_dscrc_table[];
extern char EXT_Temperature_string[1 + 8*2 + 1 + 7 + 1];
extern float EXT_Temperatures[ds18b20_MAX_DEVICES];
extern uint8 ds18b20_current;
