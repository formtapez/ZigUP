#include "ZComDef.h"
#include "onboard.h"
#include "uart.h"

void UART_Init(void)
{
  U0CSR |= (1<<7);	// Mode = UART, Receiver disabled
  // U0UCR defaults ok (8,N,1)
  
  U0GCR = 11;	// 115200 Baud
  U0BAUD = 216;	// 115200 Baud
}

void UART_Transmit(char data)
{
  U0DBUF = data;
  while (U0CSR & (1<<0)); // wait until byte has been sent
}

void UART_String(const char *s)
{
  while (*s)
  {
    UART_Transmit(*s++);
  }
  UART_Transmit('\r');
  UART_Transmit('\n');
}
