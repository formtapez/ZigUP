#include "ZComDef.h"
#include "delay.h"

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
