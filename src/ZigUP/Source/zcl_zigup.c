/*********************************************************************
* INCLUDES
*/
#include "zcl_zigup.h"

#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "MT_SYS.h"

#include "nwk_util.h"

#include "zcl.h"
#include "zcl_ha.h"
#include "zcl_ms.h"
#include "zcl_ezmode.h"
#include "zcl_diagnostic.h"

#include "onboard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"

#include "NLMEDE.h"
#include "ZDSecMgr.h"
#include "hal_flash.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*********************************************************************
* MACROS
*/

/*********************************************************************
* CONSTANTS
*/
#define ZIGUP_REPORTING_INTERVAL 5000
/*********************************************************************
* TYPEDEFS
*/

/*********************************************************************
* GLOBAL VARIABLES
*/
byte zclZigUP_TaskID;
uint16 zclZigUPSeqNum=0;

volatile uint32 S0=0;
volatile uint8 STATE_LIGHT=0;
volatile uint8 STATE_LED=0;

#define WS2812_buffersize 9
uint8 WS2812_buffer[WS2812_buffersize];
uint8 WS2812_bit=0;
uint16 WS2812_byte=0;

/*********************************************************************
* GLOBAL FUNCTIONS
*/

/*********************************************************************
* LOCAL VARIABLES
*/
afAddrType_t zclZigUP_DstAddr;

uint16 bindingInClusters[] =
{
  ZCL_CLUSTER_ID_GEN_ON_OFF
};
#define ZCLZIGUP_BINDINGLIST (sizeof(bindingInClusters) / sizeof(bindingInClusters[0]))

// Test Endpoint to allow SYS_APP_MSGs
static endPointDesc_t ZigUP_TestEp =
{
  ZIGUP_ENDPOINT,
  &zclZigUP_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

devStates_t zclZigUP_NwkState = DEV_INIT;

float ADC_Voltage = -1000;;
float CPU_Temperature = -1000;
float EXT_Temperature = -1000;
float EXT_Humidity = -1000;
uint16 DIG_IN = 0;

/*********************************************************************
* LOCAL FUNCTIONS
*/

#pragma inline=forced
void _delay_ns_400(void)
{
  /* 13 NOPs == 400nsecs */
  asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
  asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
  asm("nop"); asm("nop"); asm("nop");
}

#pragma inline=forced
void _delay_ns_800(void)
{
  /* 26 NOPs == 800nsecs */
  asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
  asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
  asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
  asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
  asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
  asm("nop");
}

#pragma inline=forced
void _delay_us(uint16 microSecs)
{
  while(microSecs--)
  {
    asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
  }
}

void _delay_ms(uint16 milliSecs)
{
  while(milliSecs--)
  {
    _delay_us(1000);
  }
}

void FactoryReset(void)
{
  UART0_String("Performing factory reset...");
  for (int i = HAL_NV_PAGE_BEG; i <= (HAL_NV_PAGE_BEG + HAL_NV_PAGE_CNT); i++)
  {
    HalFlashErase(i);
  }
  
  UART0_String("Performing system reset...");
  SystemReset();
}

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

void UART0_Init(void)
{
  U0CSR |= (1<<7);	// Mode = UART, Receiver disabled
  // U0UCR defaults ok (8,N,1)
  
  U0GCR = 11;	// 115200 Baud
  U0BAUD = 216;	// 115200 Baud
}

void UART0_Transmit(char data)
{
  
  U0DBUF = data;
  while (U0CSR & (1<<0)); // wait until byte has been sent
}

void UART0_String(const char *s)
{
  while (*s)
  {
    UART0_Transmit(*s++);
  }
  UART0_Transmit('\r');
  UART0_Transmit('\n');
}

void Relais(uint8 state)
{
  if (state)	// Switch light on
  {
    P1_1 = 1; // ON
    P1_2 = 0;	// OFF
    _delay_ms(250);
    P1_1 = 0; // ON
  }
  else	// Switch light off
  {
    P1_1 = 0; // ON
    P1_2 = 1;	// OFF
    _delay_ms(250);
    P1_2 = 0; // OFF
  }
  
  STATE_LIGHT = state;
}

void LED(uint8 state)
{
  P1SEL &= ~BV(6); // LED = GPIO
  
  if (state)	// Switch LED on
  {
    P1_6 = 1; // ON
  }
  else	// Switch LED off
  {
    P1_6 = 0; // OFF
  }
  
  STATE_LED = state;
}

void WS2812_StoreBit(uint8 bit)
{
  // store bit (only the 1 bits)
  if (bit) WS2812_buffer[WS2812_byte] |= 1 << WS2812_bit;
  WS2812_bit++;
  
  // 8 bits per byte...
  if (WS2812_bit > 7)
  {
    WS2812_bit = 0;
    WS2812_byte++;
  }
}

void WS2812_SendLED(uint8 red, uint8 green, uint8 blue)
{
  P1SEL |= BV(6); // LED = Peripheral
  
  WS2812_bit = 0;
  WS2812_byte = 0;
  
  // delete buffer
  for (uint16 i=0; i<WS2812_buffersize; i++) WS2812_buffer[i] = 0;
  
  uint8 i;
  
  for (i=8; i; i--)
  {
    if ((green >> (i-1)) & 1)
    {
      WS2812_StoreBit(1);
      WS2812_StoreBit(1);
      WS2812_StoreBit(0);
    }
    else
    {
      WS2812_StoreBit(1);
      WS2812_StoreBit(0);
      WS2812_StoreBit(0);
    }
  }
  for (i=8; i; i--)
  {
    if ((red >> (i-1)) & 1)
    {
      WS2812_StoreBit(1);
      WS2812_StoreBit(1);
      WS2812_StoreBit(0);
    }
    else
    {
      WS2812_StoreBit(1);
      WS2812_StoreBit(0);
      WS2812_StoreBit(0);
    }
  }
  for (i=8; i; i--)
  {
    if ((blue >> (i-1)) & 1)
    {
      WS2812_StoreBit(1);
      WS2812_StoreBit(1);
      WS2812_StoreBit(0);
    }
    else
    {
      WS2812_StoreBit(1);
      WS2812_StoreBit(0);
      WS2812_StoreBit(0);
    }
  }        
  
  // SPI method - a little bit jittery, but LED-Stripes seem to be okay with it
  for (uint16 j=0; j<WS2812_buffersize; j++)
  {
    U1DBUF = WS2812_buffer[j];
    while((U1CSR & (1<<0)));      // wait until byte is sent
  }
  
  /*        
  // DMA method - slower, timing broken
  
  uint8 DMACONFIG[8];
  
  // DMA source address
  DMACONFIG[0] = (uint16)(&WS2812_buffer) >> 8;
  DMACONFIG[1] = (uint16)(&WS2812_buffer) & 0xff;
  
  // DMA destination address
  DMACONFIG[2] = 0x70;
  DMACONFIG[3] = 0xf9;    // Address of U1DBUF
  
  // DMA length
  DMACONFIG[4] = WS2812_buffersize;
  DMACONFIG[5] = WS2812_buffersize;
  
  // DMA block transfer, no trigger
  DMACONFIG[6] = b00100000;
  
  // DMA address increment for source only, high priority
  DMACONFIG[7] = b01000010;
  
  DMA0CFGH = (uint16)(&DMACONFIG) >> 8;    // DMA Channel-0 Configuration Address High Byte
  DMA0CFGL = (uint16)(&DMACONFIG) & 0xff;  // DMA Channel-0 Configuration Address Low Byte
  
  DMAARM = b00000001;     // DMA Arm channel 0
  DMAREQ = b00000001;     // DMA transfer request channel 0
  */     
}

uint8 GetRandomNumber(void)
{
  ADCCON1 = b00110111;     // Clock the LFSR once (13x unrolling)
  return RNDL;
}

#pragma vector=P0INT_VECTOR 
__interrupt void IRQ_S0(void)
{
  if (P0IFG & (1<<6))	// Interrupt flag for P0.6 (S0)?
  {
    // debounce
    _delay_ms(5);
    if (!P0_6) // still pressed?
    {
      /*
      WS2812_SendLED(255, 0, 0);
      _delay_ms(500);
      WS2812_SendLED(127, 0, 0);
      _delay_ms(500);
      WS2812_SendLED(63, 0, 0);
      _delay_ms(500);
      
      WS2812_SendLED(0, 255, 0);
      _delay_ms(500);
      WS2812_SendLED(0, 127, 0);
      _delay_ms(500);
      WS2812_SendLED(0, 63, 0);
      _delay_ms(500);
      
      WS2812_SendLED(0, 0, 255);
      _delay_ms(500);
      WS2812_SendLED(0, 0, 127);
      _delay_ms(500);
      WS2812_SendLED(0, 0, 63);
      _delay_ms(500);
      */        
      S0++;
      UART0_String("[INT] S0!");
      char buffer[100];
      sprintf(buffer, "S0-Counts: %lu", S0);
      UART0_String(buffer);
      
      
      //          FactoryReset();
      
      
      
      LED(!STATE_LED);
      
    }
    
    // Clear interrupt flags
    P0IFG &= ~(1<<6);	// Clear Interrupt flag for P0.6 (S0)
    IRCON &= ~(1<<5);	// Clear Interrupt flag for Port 0
  }
}

#pragma vector=P1INT_VECTOR 
__interrupt void IRQ_KEY(void)
{
  if (P1IFG & (1<<3))	// Interrupt flag for P1.3 (KEY)?
  {
    // debounce
    _delay_ms(20);
    if (!P1_3) // still pressed?
    {
      uint8 counter = 0;
      while (!P1_3 && counter++ < 100)
      {
        _delay_ms(10);
        counter++;
      }
      if (counter > 50) UART0_String("lang");
      else UART0_String("kurz");
      
      
      
      WS2812_SendLED(22, 55, 11);
      Relais(!STATE_LIGHT);
      UART0_String("[INT] KEY!");
      char buffer[100];
      sprintf(buffer, "Light-Status: %u", STATE_LIGHT);
      UART0_String(buffer); 
      
      Measure();
      zclZigUP_Reporting();
    }
    
    // Clear interrupt flags
    P1IFG &= ~(1<<3);	// Clear Interrupt flag for P1.3 (KEY)
    IRCON2 &= ~(1<<3);	// Clear Interrupt flag for Port 1
  }
}

#pragma vector=P2INT_VECTOR 
__interrupt void IRQ_DIGIN(void)
{
  if (P2IFG & (1<<0))	// Interrupt flag for P2.0 (Dig-In)?
  {
    // debounce
    _delay_ms(20);
    if (!P2_0) // still pressed?
    {
      UART0_String("[INT] Dig-In!");
      DIG_IN = P2_0;
    }
    
    // Clear interrupt flags
    P2IFG &= ~(1<<0);	// Clear Interrupt flag for P2.0 (Dig-In)
    IRCON2 &= ~(1<<0);	// Clear Interrupt flag for Port 2
  }
}

void Measure(void)
{
  ADC_Voltage = ADC_GetVoltage();
  CPU_Temperature = ADC_GetTemperature();
  
//  if (!DHT22_Measure())
//  {
    EXT_Humidity = -1000;
    if (!ds18b20_get_temp())
    {
      EXT_Temperature = -1000;
    }
//  }
}

void zclZigUP_Reporting(void)
{
#define NUM_ATTRIBUTES  7
  
  // send report
  zclReportCmd_t *pReportCmd;
  
  pReportCmd = osal_mem_alloc( sizeof(zclReportCmd_t) + ( NUM_ATTRIBUTES * sizeof(zclReport_t) ) );
  if ( pReportCmd != NULL )
  {
    pReportCmd->numAttr = NUM_ATTRIBUTES;
    
    pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
    pReportCmd->attrList[0].attrData = (void *)(&STATE_LIGHT);
    
    pReportCmd->attrList[1].attrID = ATTRID_CPU_TEMP;
    pReportCmd->attrList[1].dataType = ZCL_DATATYPE_SINGLE_PREC;
    pReportCmd->attrList[1].attrData = (void *)(&CPU_Temperature);
    
    pReportCmd->attrList[2].attrID = ATTRID_EXT_TEMP;
    pReportCmd->attrList[2].dataType = ZCL_DATATYPE_SINGLE_PREC;
    pReportCmd->attrList[2].attrData = (void *)(&EXT_Temperature);
    
    pReportCmd->attrList[3].attrID = ATTRID_EXT_HUMI;
    pReportCmd->attrList[3].dataType = ZCL_DATATYPE_SINGLE_PREC;
    pReportCmd->attrList[3].attrData = (void *)(&EXT_Humidity);
    
    pReportCmd->attrList[4].attrID = ATTRID_S0_COUNTS;
    pReportCmd->attrList[4].dataType = ZCL_DATATYPE_UINT32;
    pReportCmd->attrList[4].attrData = (void *)(&S0);
    
    pReportCmd->attrList[5].attrID = ATTRID_ADC_VOLT;
    pReportCmd->attrList[5].dataType = ZCL_DATATYPE_SINGLE_PREC;
    pReportCmd->attrList[5].attrData = (void *)(&ADC_Voltage);
    
    pReportCmd->attrList[6].attrID = ATTRID_DIG_INPUT;
    pReportCmd->attrList[6].dataType = ZCL_DATATYPE_UINT16; // boolean or uint8 causes every second report to hang...
    pReportCmd->attrList[6].attrData = (void *)(&DIG_IN);
    
    zclZigUP_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
    zclZigUP_DstAddr.addr.shortAddr = 0;
    zclZigUP_DstAddr.endPoint=1;
    
    zcl_SendReportCmd( ZIGUP_ENDPOINT, &zclZigUP_DstAddr, ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, false, zclZigUPSeqNum++ );
  }
  
  osal_mem_free( pReportCmd );
}

int DHT22_Measure(void)
{
  uint8 last_state = 0xFF;
  uint8 i;
  uint8 j = 0;
  uint8 counter = 0;
  uint8 checksum = 0;
  uint8 dht22_data[5];
  
  /*  
  uint8 dht22_debug[100];
  uint8 debugcnt;
  for(debugcnt = 0; debugcnt < 100; debugcnt++) dht22_debug[debugcnt] = 0;
  debugcnt = 0;
  */  
  
  P0DIR |= (1<<7);     // output
  P0_7 = 1;
  _delay_ms(250);
  P0_7 = 0;
  _delay_ms(1);
  P0DIR &= ~(1<<7);     // input
  
  for(i = 0; i < 85; i++)
  {
    counter = 0;
    while(P0_7 == last_state)
    {
      counter++;
      _delay_us(1);
      
      if(counter > 60) break; // Exit if not responsive
    }
    last_state = P0_7;
    
    if(counter > 60) break; // Double check for stray sensor
    
    // Ignore the first 3 transitions (the 80us x 2 start condition plus the first ready-to-send-bit state), and discard ready-to-send-bit counts
    if((i >= 4) && ((i % 2) != 0))
    {
      dht22_data[j / 8] <<= 1;
      //      dht22_debug[debugcnt++] = counter;
      if(counter > 20)  // detect "1" bit time
      {
        dht22_data[j / 8] |= 1;
      }
      j++;
    }
  }
  
  char buffer[100];
  /*
  sprintf(buffer, "j: %u", j);
  UART0_String(buffer); 
  
  for(i = 0; i < 5; i++)
  {
  sprintf(buffer, "DHT22: (%u) %u\n", i, dht22_data[i]);
  UART0_String(buffer); 
}
  
  for(debugcnt = 0; debugcnt < 100; debugcnt++)
  {
  sprintf(buffer, "DHT22 Debug: (%u) %u\n", debugcnt, dht22_debug[debugcnt]);
  UART0_String(buffer); 
}
  */  
  
  // If we have 5 bytes (40 bits), wrap-up and end
  if(j >= 40)
  {
    // The first 2 bytes are humidity values, the next 2 are temperature, the final byte is the checksum
    checksum = dht22_data[0] + dht22_data[1] + dht22_data[2] + dht22_data[3];
    checksum &= 0xFF;
    if(dht22_data[4] == checksum)
    {
      float humidity = (dht22_data[0] * 256 + dht22_data[1]) / 10.0;
      float temperature = ((dht22_data[2] & b01111111)* 256 + dht22_data[3]) / 10.0;
      if (dht22_data[2] & b10000000) temperature = -temperature;        // negative temperatures when MSB is set
      
      EXT_Temperature = temperature;
      EXT_Humidity = humidity;
      
      sprintf(buffer, "DHT22: %.1f °C  / %.1f %%\n", temperature, humidity);
      UART0_String(buffer); 
      return (1);
    }
    else
    {
      UART0_String("DHT22: CRC-Fail"); 
      return (0);
    }
  }
  else
  {
    UART0_String("DHT22: Timeout"); 
    return (0);
  }
}

int DS1820_Measure(void)
{
  return (0);
}

// Sends one bit to bus
void ds18b20_send(uint8 bit)
{
  P0DIR |= (1<<7);     // output
  P0_7 = 0;
  _delay_us(5);
  if (bit==1) P0_7 = 1;
  _delay_us(80);
  P0_7 = 1;
}

// Reads one bit from bus
uint8 ds18b20_read(void)
{
  P0DIR |= (1<<7);     // output
  P0_7 = 0;
  _delay_us(2);
  P0_7 = 1;
  _delay_us(15);
  P0DIR &= ~(1<<7);     // input
  return P0_7;
}

// Sends one byte to bus
void ds18b20_send_byte(int8 data)
{
  uint8 i,x;
  for(i=0;i<8;i++)
  {
    x = data>>i;
    x &= 0x01;
    ds18b20_send(x);
  }
  _delay_us(100);
}

// Reads one byte from bus
uint8 ds18b20_read_byte(void)
{
  uint8 i;
  uint8 data = 0;
  for (i=0;i<8;i++)
  {
    if(ds18b20_read()) data|=0x01<<i;
    _delay_us(15);
  }
  return(data);
}

// Sends reset pulse
uint8 ds18b20_RST_PULSE(void)
{
  P0DIR |= (1<<7);     // output
  P0_7 = 0;
  _delay_us(500);
  P0_7 = 1;
  P0DIR &= ~(1<<7);     // input
  _delay_us(500);
  return P0_7;
}

// Returns temperature from sensor
uint8 ds18b20_get_temp(void)
{
  char buffer[100];
  uint8 temp1, temp2;
  if(ds18b20_RST_PULSE())
  {
    ds18b20_send_byte(0xCC);
    ds18b20_send_byte(0x44);
    _delay_ms(750);
    ds18b20_RST_PULSE();
    ds18b20_send_byte(0xCC);
    ds18b20_send_byte(0xBE);
    temp1=ds18b20_read_byte();
    temp2=ds18b20_read_byte();
    ds18b20_RST_PULSE();
    EXT_Temperature = ((uint16)temp1 | (uint16)(temp2 & b00000111) << 8) / 16.0;
    
    // neg. temp
    if (temp2 & b00001000) EXT_Temperature = ((uint16)temp1 | (uint16)(temp2 & b00000111) << 8) / 16.0 - 128.0;
    // pos. temp
    else EXT_Temperature = ((uint16)temp1 | (uint16)(temp2 & b00000111) << 8) / 16.0;
    
    sprintf(buffer, "DS18B20: %.2f °C\n", EXT_Temperature);
    UART0_String(buffer); 
    return 1;
  }
  else
  {
    UART0_String("DS18B20: Fail."); 
    return 0;
  }
}


/*********************************************************************
* ZCL General Profile Callback table
*/
static zclGeneral_AppCallbacks_t zclZigUP_CmdCallbacks =
{
  zclZigUP_BasicResetCB,            // Basic Cluster Reset command
  zclZigUP_IdentifyCB,              // Identify command
  NULL,                                   // Identify Trigger Effect command
  zclZigUP_IdentifyQueryRspCB,      // Identify Query Response command
  zclZigUP_OnOffCB,                 // On/Off cluster commands
  NULL,                                   // On/Off cluster enhanced command Off with Effect
  NULL,                                   // On/Off cluster enhanced command On with Recall Global Scene
  NULL,                                   // On/Off cluster enhanced command On with Timed Off
#ifdef ZCL_GROUPS
  NULL,                                   // Group Response commands
#endif
#ifdef ZCL_SCENES
  NULL,                                  // Scene Store Request command
  NULL,                                  // Scene Recall Request command
  NULL,                                  // Scene Response command
#endif
#ifdef ZCL_ALARMS
  NULL,                                  // Alarm (Response) commands
#endif
#ifdef SE_UK_EXT
  NULL,                                  // Get Event Log command
  NULL,                                  // Publish Event Log command
#endif
  NULL,                                  // RSSI Location command
  NULL                                   // RSSI Location Response command
};

/*********************************************************************
* @fn          zclZigUP_Init
*
* @brief       Initialization function for the zclGeneral layer.
*
* @param       none
*
* @return      none
*/
void zclZigUP_Init( byte task_id )
{
  //  BindRestoreFromNV();
  zclZigUP_TaskID = task_id;
  
  // Set destination address to indirect
  zclZigUP_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  zclZigUP_DstAddr.endPoint = 0;
  zclZigUP_DstAddr.addr.shortAddr = 0;
  
  // This app is part of the Home Automation Profile
  zclHA_Init( &zclZigUP_SimpleDesc[0] );
  
  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( ZIGUP_ENDPOINT, &zclZigUP_CmdCallbacks );
  
  // Register the application's attribute list
  zcl_registerAttrList( ZIGUP_ENDPOINT, zclZigUP_NumAttributes, zclZigUP_Attrs );
  
  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( zclZigUP_TaskID );
  
#ifdef ZCL_DISCOVER
  // Register the application's command list
  zcl_registerCmdList( ZIGUP_ENDPOINT, zclCmdsArraySize, zclZigUP_Cmds );
#endif
  
  // Register for all key events - This app will handle all key events
  RegisterForKeys( zclZigUP_TaskID );
  
  // Register for a test endpoint
  afRegister( &ZigUP_TestEp );
  
#ifdef ZCL_DIAGNOSTIC
  // Register the application's callback function to read/write attribute data.
  // This is only required when the attribute data format is unknown to ZCL.
  zcl_registerReadWriteCB( ZIGUP_ENDPOINT, zclDiagnostic_ReadWriteAttrCB, NULL );
  
  if ( zclDiagnostic_InitStats() == ZSuccess )
  {
    // Here the user could start the timer to save Diagnostics to NV
  }
#endif
  
  
  
  
  
  P0SEL = b00001101;                    // 0=GPIO 1=Peripheral (ADC, UART)
  P0DIR = b00001000;                    // 1=output
  P0INP = b00000001;                    // 1=no pulling
  
  P1SEL = b01000000;                    // 0=GPIO 1=Peripheral (SPI)
  P1DIR = b01000110;                    // 1=output
  P1INP = b00000000;                    // 1=no pulling
  
  P2SEL = b00000000;                    // 0=GPIO 1=Peripheral, Priorities
  P2DIR = b00000000;                    // 1=output, Priorities
  P2INP = b00000000;                    // 1=no pulling, Ports P0-P2 all Pull-UP
  
  PICTL |= BV(0) | BV(1) | BV(3);       // Falling Edge INT P0 (S0) + P1.0-P1.3 (KEY) + P2.0 (Dig-In)
  IEN0 |= BV(7);                        // EA - Global interrupt enable
  IEN1 |= BV(5);                        // Port 0 interrupt enable
  IEN2 |= BV(1) | BV(4);                // Port 1+2 interrupt enable
  
  P0IEN |= BV(6);                       // S0 (P0.6) interrupt enable
  P1IEN |= BV(3);                       // KEY (P1.3) interrupt enable
  P2IEN |= BV(0);                       // Dig-In (P2.0) interrupt enable
  
  
  PERCFG |= BV(1);                      // UART1 SPI Alternative #2 Pins
  U1CSR = b00000000;                    // UART1 SPI Master
  U1GCR = b00010000;                    // UART1 Baud_E
  U1BAUD = b01000000;                   // UART1 Baud_M
  
  
  _delay_ms(GetRandomNumber()); // Random delay
  
  Relais(0);
  //  WS2812_SendLED(0, 0, 0);
  UART0_Init();
  
  if (P0_7) UART0_String("Sensor: High.");
  else UART0_String("Sensor: Low.");
  
  
  osal_start_reload_timer( zclZigUP_TaskID, ZIGUP_REPORTING_EVT, ZIGUP_REPORTING_INTERVAL );
  
  
  UART0_String("Init done.");
  
  
  
}

/*********************************************************************
* @fn          zclSample_event_loop
*
* @brief       Event Loop Processor for zclGeneral.
*
* @param       none
*
* @return      none
*/
uint16 zclZigUP_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  
  (void)task_id;  // Intentionally unreferenced parameter
  
  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( zclZigUP_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
      case ZCL_INCOMING_MSG:
        // Incoming ZCL Foundation command/response messages
        zclZigUP_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
        break;
        
      case ZDO_STATE_CHANGE:
        zclZigUP_NwkState = (devStates_t)(MSGpkt->hdr.status);
        
        // now on the network
        if ( (zclZigUP_NwkState == DEV_ZB_COORD) || (zclZigUP_NwkState == DEV_ROUTER) || (zclZigUP_NwkState == DEV_END_DEVICE) )
        {
          // now on network
        }
        break;
        
      default:
        break;
      }
      
      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }
    
    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  
  if ( events & ZIGUP_IDENTIFY_TIMEOUT_EVT )
  {
    if ( zclZigUP_IdentifyTime > 0 ) zclZigUP_IdentifyTime--;
    zclZigUP_ProcessIdentifyTimeChange();
    
    return ( events ^ ZIGUP_IDENTIFY_TIMEOUT_EVT );
  }
  
  if ( events & ZIGUP_REPORTING_EVT )
  {
    // update measurements
    Measure();
    
    // report states
    zclZigUP_Reporting();
    
    return ( events ^ ZIGUP_REPORTING_EVT );
  }    
  
  // Discard unknown events
  return 0;
}

/*********************************************************************
* @fn      zclZigUP_ProcessIdentifyTimeChange
*
* @brief   Called to process any change to the IdentifyTime attribute.
*
* @param   none
*
* @return  none
*/
static void zclZigUP_ProcessIdentifyTimeChange( void )
{
  if ( zclZigUP_IdentifyTime > 0 )
  {
    osal_start_timerEx( zclZigUP_TaskID, ZIGUP_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    osal_stop_timerEx( zclZigUP_TaskID, ZIGUP_IDENTIFY_TIMEOUT_EVT );
  }
}

/*********************************************************************
* @fn      zclZigUP_BasicResetCB
*
* @brief   Callback from the ZCL General Cluster Library
*          to set all the Basic Cluster attributes to default values.
*
* @param   none
*
* @return  none
*/
static void zclZigUP_BasicResetCB( void )
{
  NLME_LeaveReq_t leaveReq;
  // Set every field to 0
  osal_memset( &leaveReq, 0, sizeof( NLME_LeaveReq_t ) );
  
  // This will enable the device to rejoin the network after reset.
  leaveReq.rejoin = TRUE;
  
  // Set the NV startup option to force a "new" join.
  zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );
  
  // Leave the network, and reset afterwards
  if ( NLME_LeaveReq( &leaveReq ) != ZSuccess )
  {
    // Couldn't send out leave; prepare to reset anyway
    ZDApp_LeaveReset( FALSE );
  }
}

/*********************************************************************
* @fn      zclZigUP_IdentifyCB
*
* @brief   Callback from the ZCL General Cluster Library when
*          it received an Identity Command for this application.
*
* @param   srcAddr - source address and endpoint of the response message
* @param   identifyTime - the number of seconds to identify yourself
*
* @return  none
*/
static void zclZigUP_IdentifyCB( zclIdentify_t *pCmd )
{
  zclZigUP_IdentifyTime = pCmd->identifyTime;
  zclZigUP_ProcessIdentifyTimeChange();
}

/*********************************************************************
* @fn      zclZigUP_IdentifyQueryRspCB
*
* @brief   Callback from the ZCL General Cluster Library when
*          it received an Identity Query Response Command for this application.
*
* @param   srcAddr - requestor's address
* @param   timeout - number of seconds to identify yourself (valid for query response)
*
* @return  none
*/
static void zclZigUP_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp )
{
  (void)pRsp;
}

/*********************************************************************
* @fn      zclZigUP_OnOffCB
*
* @brief   Callback from the ZCL General Cluster Library when
*          it received an On/Off Command for this application.
*
* @param   cmd - COMMAND_ON, COMMAND_OFF or COMMAND_TOGGLE
*
* @return  none
*/
static void zclZigUP_OnOffCB( uint8 cmd )
{
  afIncomingMSGPacket_t *pPtr = zcl_getRawAFMsg();
  zclZigUP_DstAddr.addr.shortAddr = pPtr->srcAddr.addr.shortAddr;
  
  // Turn on the light
  if ( cmd == COMMAND_ON )
  {
    Relais(LIGHT_ON);
  }
  // Turn off the light
  else if ( cmd == COMMAND_OFF )
  {
    Relais(LIGHT_OFF);
  }
  // Toggle the light
  else if ( cmd == COMMAND_TOGGLE )
  {
    Relais(!STATE_LIGHT);
  }
}

/******************************************************************************
*
*  Functions for processing ZCL Foundation incoming Command/Response messages
*
*****************************************************************************/

/*********************************************************************
* @fn      zclZigUP_ProcessIncomingMsg
*
* @brief   Process ZCL Foundation incoming message
*
* @param   pInMsg - pointer to the received message
*
* @return  none
*/
static void zclZigUP_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg )
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
  case ZCL_CMD_READ_RSP:
    zclZigUP_ProcessInReadRspCmd( pInMsg );
    break;
#endif
#ifdef ZCL_WRITE
  case ZCL_CMD_WRITE_RSP:
    zclZigUP_ProcessInWriteRspCmd( pInMsg );
    break;
#endif
#ifdef ZCL_REPORT
    // Attribute Reporting implementation should be added here
  case ZCL_CMD_CONFIG_REPORT:
    // zclZigUP_ProcessInConfigReportCmd( pInMsg );
    break;
    
  case ZCL_CMD_CONFIG_REPORT_RSP:
    // zclZigUP_ProcessInConfigReportRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_READ_REPORT_CFG:
    // zclZigUP_ProcessInReadReportCfgCmd( pInMsg );
    break;
    
  case ZCL_CMD_READ_REPORT_CFG_RSP:
    // zclZigUP_ProcessInReadReportCfgRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_REPORT:
    // zclZigUP_ProcessInReportCmd( pInMsg );
    break;
#endif
  case ZCL_CMD_DEFAULT_RSP:
    zclZigUP_ProcessInDefaultRspCmd( pInMsg );
    break;
#ifdef ZCL_DISCOVER
  case ZCL_CMD_DISCOVER_CMDS_RECEIVED_RSP:
    zclZigUP_ProcessInDiscCmdsRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_DISCOVER_CMDS_GEN_RSP:
    zclZigUP_ProcessInDiscCmdsRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_DISCOVER_ATTRS_RSP:
    zclZigUP_ProcessInDiscAttrsRspCmd( pInMsg );
    break;
    
  case ZCL_CMD_DISCOVER_ATTRS_EXT_RSP:
    zclZigUP_ProcessInDiscAttrsExtRspCmd( pInMsg );
    break;
#endif
    
  default:
    break;
  }
  
  if ( pInMsg->attrCmd ) osal_mem_free( pInMsg->attrCmd );
}

#ifdef ZCL_READ
/*********************************************************************
* @fn      zclZigUP_ProcessInReadRspCmd
*
* @brief   Process the "Profile" Read Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;
  
  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
    // Notify the originator of the results of the original read attributes
    // attempt and, for each successfull request, the value of the requested
    // attribute
  }
  
  return ( TRUE );
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
* @fn      zclZigUP_ProcessInWriteRspCmd
*
* @brief   Process the "Profile" Write Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;
  
  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < writeRspCmd->numAttr; i++ )
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }
  
  return ( TRUE );
}
#endif // ZCL_WRITE

/*********************************************************************
* @fn      zclZigUP_ProcessInDefaultRspCmd
*
* @brief   Process the "Profile" Default Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
  
  // Device is notified of the Default Response command.
  (void)pInMsg;
  
  return ( TRUE );
}

#ifdef ZCL_DISCOVER
/*********************************************************************
* @fn      zclZigUP_ProcessInDiscCmdsRspCmd
*
* @brief   Process the Discover Commands Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInDiscCmdsRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverCmdsCmdRsp_t *discoverRspCmd;
  uint8 i;
  
  discoverRspCmd = (zclDiscoverCmdsCmdRsp_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numCmd; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }
  
  return ( TRUE );
}

/*********************************************************************
* @fn      zclZigUP_ProcessInDiscAttrsRspCmd
*
* @brief   Process the "Profile" Discover Attributes Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInDiscAttrsRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsRspCmd_t *discoverRspCmd;
  uint8 i;
  
  discoverRspCmd = (zclDiscoverAttrsRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }
  
  return ( TRUE );
}

/*********************************************************************
* @fn      zclZigUP_ProcessInDiscAttrsExtRspCmd
*
* @brief   Process the "Profile" Discover Attributes Extended Response Command
*
* @param   pInMsg - incoming message to process
*
* @return  none
*/
static uint8 zclZigUP_ProcessInDiscAttrsExtRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverAttrsExtRsp_t *discoverRspCmd;
  uint8 i;
  
  discoverRspCmd = (zclDiscoverAttrsExtRsp_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }
  
  return ( TRUE );
}
#endif // ZCL_DISCOVER
