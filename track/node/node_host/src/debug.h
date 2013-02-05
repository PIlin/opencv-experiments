#ifndef DEBUG_H___
#define DEBUG_H___

#include <Arduino.h>

struct BufferPrint : public Print
{
  uint8_t* buf;
  uint8_t s;
  uint8_t written;

  BufferPrint() : buf(0), s(0), written(0) {}
  BufferPrint(uint8_t* buf, size_t s) : buf(buf), s(s), written(0) {}
  virtual size_t write(uint8_t c);
};


extern BufferPrint bufferPrint;

void reset_buffer_print(void);
void DBGP(char const* x);



#if DEBUG == 1
# define DEBUG_PRINT(x)   do { Serial.print(x);   } while (0)
# define DEBUG_PRINTLN(x) do { Serial.println(x); } while (0)
# define DEBUG_PRINTLN2(x,y) do { DEBUG_PRINT(x); DEBUG_PRINTLN(y); } while (0)
# define DEBUG_PRINTLN2H(x,y) do { DEBUG_PRINT(x); Serial.println(x, HEX); } while (0)
#elif DEBUG == 2
# define _DBG2PREP reset_buffer_print()
# define _DBG2SEND DBGP((char const*)bufferPrint.buf)
# define DEBUG_PRINT(x)   do { _DBG2PREP; bufferPrint.print(x); _DBG2SEND; } while (0)
# define DEBUG_PRINTLN(x) do { _DBG2PREP; bufferPrint.print(x); _DBG2SEND; } while (0)
# define DEBUG_PRINTLN2(x,y) do { _DBG2PREP; bufferPrint.print(x); bufferPrint.print(y); _DBG2SEND; } while (0)
# define DEBUG_PRINTLN2H(x,y) do { _DBG2PREP; bufferPrint.print(x); bufferPrint.print(y, HEX); _DBG2SEND; } while (0)
#else
# define DEBUG_PRINT(x)
# define DEBUG_PRINTLN(x)
# define DEBUG_PRINTLN2(x,y)
# define DEBUG_PRINTLN2H(x,y)
#endif






#endif