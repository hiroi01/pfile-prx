#ifndef ___LOG__H___
#define ___LOG__H___


#if DEBUG == 2

void logToFile(const char* format, ...);
#define dbgprintf(format, ... ) logToFile(format, ##__VA_ARGS__)

#elif DEBUG == 1

#include <stdio.h>

#define dbgprintf(format, ... ) printf(format, ##__VA_ARGS__)

#else

#define dbgprintf(format, ... ) do{}while(0)

#endif





#endif
