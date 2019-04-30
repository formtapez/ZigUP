// buffersize = num_leds * 24 bits * 3 bits per bit / 8 bit
#define WS2812_buffersize 9

void WS2812_StoreBit(uint8 bit);
void WS2812_SendLED(uint8 red, uint8 green, uint8 blue);
