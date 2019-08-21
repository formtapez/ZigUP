#include <stdio.h>
#include "onboard.h"
#include "ZComDef.h"
#include "bitmasks.h"
#include "delay.h"
#include "ds18b20.h"
#include "uart.h"
#include "global.h"
#include "hal_mcu.h"


#define DS18B20_SKIP_ROM 		0xCC
#define DS18B20_CONVERT_T 		0x44
#define DS18B20_MATCH_ROM               0x55
#define DS18B20_SEARCH_ROM		0XF0
#define DS18B20_READ_SCRATCHPAD         0xBE
#define DS18B20_WRITE_SCRATCHPAD        0x4E
#define DS18B20_COPY_SCRATCHPAD         0x48


// Sends one bit to bus
void ds18b20_send_bit(uint8 bit, uint8 useInterrupts)
{
  P0_7 = 1;
  if (useInterrupts)
    HAL_DISABLE_INTERRUPTS();
  P0DIR |= (1<<7);     // output
  P0_7 = 0;
  if ( bit != 0 ) 
    _delay_us( 8 );
  else 
    _delay_us( 80 );
  P0_7 = 1;
  if (useInterrupts)
    HAL_ENABLE_INTERRUPTS();   
  if ( bit != 0 ) 
    _delay_us( 80 );
  else 
    _delay_us( 2 );
  
//  if (bit==1) P0_7 = 1;
//  _delay_us(100);
//  P0_7 = 1;
}


// Reads one bit from bus
uint8 ds18b20_read_bit(uint8 useInterrupts)
{
  uint8 i;
 
  
  P0_7 = 1;
  if (useInterrupts)
    HAL_DISABLE_INTERRUPTS();
  P0DIR |= (1<<7);     // output
  P0_7 = 0;
  _delay_us(2);
//  P0_7 = 1;
//  _delay_us(15);
  P0DIR &= ~(1<<7);     // input
  _delay_us( 5 );
  i = P0_7;
  if (useInterrupts)
    HAL_ENABLE_INTERRUPTS();
  _delay_us( 60 );
  return i;
}


// Sends one byte to bus
void ds18b20_send_byte(int8 data, uint8 useInterrupts)
{
  uint8 i,x;
  for(i=0;i<8;i++)
  {
    x = data>>i;
    x &= 0x01;
    ds18b20_send_bit(x, useInterrupts);
  }
  //_delay_us(100);
}


// Reads one byte from bus
uint8 ds18b20_read_byte(uint8 useInterrupts)
{
  uint8 i;
  uint8 data = 0;
  for (i=0;i<8;i++)
  {
    if(ds18b20_read_bit(useInterrupts)) data|=0x01<<i;
    //_delay_us(25);
  }
  return(data);
}


// Sends reset pulse
uint8 ds18b20_RST_PULSE(void)
{
  uint8 i;
  P0_7 = 0;
  P0DIR |= (1<<7);     // output
  _delay_us(600);
  P0DIR &= ~(1<<7);     // input
  _delay_us(70);
  i = P0_7;
  _delay_us(200);
  P0_7 = 1;
  P0DIR |= (1<<7);     // output
  _delay_us(600);
  return i;
}


//////////////////////////////////////////////////////////////////////////////
// ONE WIRE CRC
//
uint8 ds18b20_ow_crc( uint8 x)
{
  ds18b20_dowcrc = ds18b20_dscrc_table[ds18b20_dowcrc^x];
  return ds18b20_dowcrc;
}


// NEXT
// The Next function searches for the next device on the 1-Wire bus. If
// there are no more devices on the 1-Wire then false is returned.
//
uint8 ds18b20_Next(void)
{
  uint8 m = 1; // ROM Bit index
  uint8 n = 0; // ROM Byte index
  uint8 k = 1; // bit mask
  uint8 x = 0;
  uint8 discrepMarker = 0; // discrepancy marker
  uint8 g; // Output bit
  uint8 nxt; // return value
  int flag;
  
  nxt = FALSE; // set the next flag to false
  ds18b20_dowcrc = 0; // reset the dowcrc
  flag = ds18b20_RST_PULSE(); // reset the 1-Wire
  if(flag|| ds18b20_doneFlag) // no parts -> return false
  {
    ds18b20_lastDiscrep = 0; // reset the search
    return FALSE;
  }
  ds18b20_send_byte(0xF0, 0); // send SearchROM command
  do
    // for all eight bytes
  {
    x = 0;
    if(ds18b20_read_bit(0)==1) 
      x = 2;
    _delay_us(15);
    if(ds18b20_read_bit(0)==1) 
      x |= 1; // and its complement
    if(x ==3) // there are no devices on the 1-Wire
      break;
    else
    {
      if(x>0) // all devices coupled have 0 or 1
        g = x>>1; // bit write value for search
      else
      {
        // if this discrepancy is before the last
        // discrepancy on a previous Next then pick
        // the same as last time
        if(m<ds18b20_lastDiscrep)
          g = ((ds18b20_ROM[n]&k)>0);
        else // if equal to last pick 1
          g = (m==ds18b20_lastDiscrep); // if not then pick 0
        // if 0 was picked then record
        // position with mask k
        if (g==0) 
          discrepMarker = m;
      }
      if(g==1) // isolate bit in ROM[n] with mask k
        ds18b20_ROM[n] |= k;
      else
        ds18b20_ROM[n] &= ~k;
      ds18b20_send_bit(g, 0); // ROM search write
      m++; // increment bit counter m
      k = k<<1; // and shift the bit mask k
      if(k==0) // if the mask is 0 then go to new ROM
      { // byte n and reset mask
        ds18b20_ow_crc(ds18b20_ROM[n]); // accumulate the CRC
        n++; k++;
      }
    }
  }while(n<8); //loop until through all ROM bytes 0-7
  
  if(m<65||ds18b20_dowcrc) // if search was unsuccessful then
    ds18b20_lastDiscrep=0; // reset the last discrepancy to 0
  else
  {
    // search was successful, so set lastDiscrep,
    // lastOne, nxt
    ds18b20_lastDiscrep = discrepMarker;
    ds18b20_doneFlag = (ds18b20_lastDiscrep==0);
    nxt = TRUE; // indicates search is not complete yet, more
    // parts remain
  }
  return nxt;
}


// FIRST
// The First function resets the current state of a ROM search and calls
// Next to find the first device on the 1-Wire bus.
//
uint8 ds18b20_First(void)
{
  ds18b20_lastDiscrep = 0; // reset the rom search last discrepancy global
  ds18b20_doneFlag = FALSE;
  return ds18b20_Next(); // call Next and return its return value
}


uint8 ds18b20_find_devices(void)
{
  unsigned char m;
  ds18b20_numROMs=0;
  char buffer[100];
  
  if(!ds18b20_RST_PULSE()) //Begins when a presence is detected
  {
    if(ds18b20_First()) //Begins when at least one part is found
    {
      do
      {
        for(m=0;m<8;m++)
        {
          ds18b20_FoundROM[ds18b20_numROMs][m] = ds18b20_ROM[m]; //Identifies ROM
        } 
        sprintf(buffer, "Sensor %d ROM CODE = %02X%02X%02X%02X%02X%02X%02X%02X",  ds18b20_numROMs,
                ds18b20_FoundROM[ds18b20_numROMs][7],ds18b20_FoundROM[ds18b20_numROMs][6],ds18b20_FoundROM[ds18b20_numROMs][5],ds18b20_FoundROM[ds18b20_numROMs][4],
                ds18b20_FoundROM[ds18b20_numROMs][3],ds18b20_FoundROM[ds18b20_numROMs][2],ds18b20_FoundROM[ds18b20_numROMs][1],ds18b20_FoundROM[ds18b20_numROMs][0]);
        UART_String(buffer);
        ds18b20_numROMs++;
      }while (ds18b20_Next()&&(ds18b20_numROMs<ds18b20_MAX_DEVICES+1)); //Continues until no additional devices are found
    }
  }
  return ds18b20_numROMs;
}


void ds18b20_start_conversion(void)
{
  if(!ds18b20_RST_PULSE())
  {
    ds18b20_send_byte(DS18B20_SKIP_ROM, 1);
    ds18b20_send_byte(DS18B20_CONVERT_T, 1);
    _delay_ms(750);
  }
}


void ds18b20_SelectSensor(uint8 ID)
{
  uint8 i;
  
  if(!ds18b20_RST_PULSE())
  {
    ds18b20_send_byte(DS18B20_MATCH_ROM, 1); // DS18B20_MATCH_ROM
    for(i=0;i<8;i++)
    {
      ds18b20_send_byte(ds18b20_FoundROM[ID][i], 1); //send ROM code
    }
  }
}


float ds18b20_get_sensor_temperature(uint8 deviceId)
{
  uint8 temp1, temp2;
  float temperature = -999.99;
  //EXT_Temperature = -999.99;
  
  ds18b20_SelectSensor(deviceId);
  
  ds18b20_send_byte(DS18B20_READ_SCRATCHPAD, 1);
  temp1=ds18b20_read_byte(1);
  temp2=ds18b20_read_byte(1);
  ds18b20_RST_PULSE();
  
  if (temp1 == 0xff && temp2 == 0xff)
  {
    // cannot find sensor
    return temperature;
    //return EXT_Temperature;
  }
  
  // neg. temp
  if (temp2 & b00001000) temperature = ((uint16)temp1 | (uint16)(temp2 & b00000111) << 8) / 16.0 - 128.0;
  // pos. temp
  else temperature = ((uint16)temp1 | (uint16)(temp2 & b00000111) << 8) / 16.0;
  
  return temperature;
}