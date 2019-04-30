#include <stdio.h>
#include "onboard.h"
#include "ZComDef.h"
#include "bitmasks.h"
#include "delay.h"
#include "ds18b20.h"
#include "uart.h"
#include "global.h"
#include "ws2812.h"
#include "led.h"
#include "interrupts.h"
#include "zcl_zigup.h"
#include "utils.h"

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
      UART_String("[INT] S0!");
      char buffer[100];
      sprintf(buffer, "S0-Counts: %lu", S0);
      UART_String(buffer);
      
      
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
      if (counter > 50) UART_String("lang");
      else UART_String("kurz");
      
      
      
      WS2812_SendLED(22, 55, 11);
      Relais(!STATE_LIGHT);
      UART_String("[INT] KEY!");
      char buffer[100];
      sprintf(buffer, "Light-Status: %u", STATE_LIGHT);
      UART_String(buffer); 
      
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
      UART_String("[INT] Dig-In!");
      DIG_IN = P2_0;
    }
    
    // Clear interrupt flags
    P2IFG &= ~(1<<0);	// Clear Interrupt flag for P2.0 (Dig-In)
    IRCON2 &= ~(1<<0);	// Clear Interrupt flag for Port 2
  }
}
