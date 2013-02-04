#ifndef DEBUG_H___
#define DEBUG_H___


#ifdef DEBUG
# define DEBUG_PRINT(x)   do { Serial.print(x);   } while (0)
# define DEBUG_PRINTLN(x) do { Serial.println(x); } while (0)
# define DEBUG_PRINTLN2(x,y) do { DEBUG_PRINT(x); DEBUG_PRINTLN(y); } while (0)
#else
# define DEBUG_PRINT(x)
# define DEBUG_PRINTLN(x)
# define DEBUG_PRINTLN2(x,y)
#endif



#endif