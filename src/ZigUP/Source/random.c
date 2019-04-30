#include "onboard.h"
#include "bitmasks.h"
#include "random.h"

uint8 GetRandomNumber(void)
{
  ADCCON1 = b00110111;     // Clock the LFSR once (13x unrolling)
  return RNDL;
}
