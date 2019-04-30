#include "hal_flash.h"
#include "onboard.h"
#include "delay.h"
#include "uart.h"
#include "global.h"
#include "utils.h"

void FactoryReset(void)
{
  UART_String("Performing factory reset...");
  for (int i = HAL_NV_PAGE_BEG; i <= (HAL_NV_PAGE_BEG + HAL_NV_PAGE_CNT); i++)
  {
    HalFlashErase(i);
  }
  
  UART_String("Performing system reset...");
  SystemReset();
}

void Relais(uint8 state)
{
  if (state)	// Switch light on
  {
    P1_1 = 1;   // activate ON-solenoid
    P1_2 = 0;   // deactivate OFF-solenoid
    _delay_ms(250);
    P1_1 = 0;   // deactivate ON-solenoid
  }
  else	// Switch light off
  {
    P1_1 = 0;   // deactivate ON-solenoid
    P1_2 = 1;   // activate OFF-solenoid
    _delay_ms(250);
    P1_2 = 0;   // deactivate OFF-solenoid
  }
  
  STATE_LIGHT = state;
}
