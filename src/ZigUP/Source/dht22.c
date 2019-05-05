#include <stdio.h>
#include "onboard.h"
#include "bitmasks.h"
#include "delay.h"
#include "dht22.h"
#include "uart.h"
#include "global.h"

int DHT22_Measure(void)
{
  uint8 last_state = 0xFF;
  uint8 i;
  uint8 j = 0;
  uint8 counter = 0;
  uint8 checksum = 0;
  uint8 dht22_data[5];

//#define ZIGUP_DHT22_DEBUG  

#ifdef ZIGUP_DHT22_DEBUG
  uint8 dht22_debug[100];
  uint8 debugcnt;
  for(debugcnt = 0; debugcnt < 100; debugcnt++) dht22_debug[debugcnt] = 0;
  debugcnt = 0;
#endif    
  
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
#ifdef ZIGUP_DHT22_DEBUG
      dht22_debug[debugcnt++] = counter;
#endif
      if(counter > 9)  // detect "1" bit time
      {
        dht22_data[j / 8] |= 1;
      }
      j++;
    }
  }

  char buffer[100];

#ifdef ZIGUP_DHT22_DEBUG
  sprintf(buffer, "j: %u", j);
  UART_String(buffer); 
  
  for(i = 0; i < 5; i++)
  {
    sprintf(buffer, "DHT22: (%u) %u\n", i, dht22_data[i]);
    UART_String(buffer); 
  }
  
  for(debugcnt = 0; debugcnt < 100; debugcnt++)
  {
    sprintf(buffer, "DHT22 Debug: (%u) %u\n", debugcnt, dht22_debug[debugcnt]);
    UART_String(buffer); 
  }
#endif    
  
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
      UART_String(buffer); 
      return (1);
    }
    else
    {
      UART_String("DHT22: CRC-Fail"); 
      return (0);
    }
  }
  else
  {
    UART_String("DHT22: Timeout"); 
    return (0);
  }
}
