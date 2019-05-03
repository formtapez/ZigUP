#include "ZComDef.h"
#include "onboard.h"
#include "ws2812.h"

uint8 WS2812_buffer[WS2812_buffersize];
uint8 WS2812_bit=0;
uint16 WS2812_byte=0;

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
  
  uint8 i,j;
  
  for (j=0; j<10; j++)  // send identical data 10 times for up to 10 LEDs in a row
  {
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
