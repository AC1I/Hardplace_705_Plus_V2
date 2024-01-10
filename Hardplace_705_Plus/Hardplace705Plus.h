#if !defined HARDPLACE705PLUS_DEFINED
#define HARDPLACE705PLUS_DEFINED

#include <TeensyThreads.h>

#define TRACE_GLOBAL false
#define DO_PING

#if defined _THREADS_H
#define USE_THREADS
#endif

#if defined USB_DUAL_SERIAL || defined USB_TRIPLE_SERIAL
#define DUAL_SERIAL
#endif

void Delay(uint32_t millis);
#endif
