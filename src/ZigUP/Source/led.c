#include "onboard.h"
#include "global.h"
#include "led.h"

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

