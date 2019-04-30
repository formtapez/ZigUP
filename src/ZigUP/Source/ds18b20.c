#include <stdio.h>
#include "onboard.h"
#include "ZComDef.h"
#include "bitmasks.h"
#include "delay.h"
#include "ds18b20.h"
#include "uart.h"
#include "global.h"

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
    UART_String(buffer); 
    return 1;
  }
  else
  {
    UART_String("DS18B20: Fail."); 
    return 0;
  }
}
