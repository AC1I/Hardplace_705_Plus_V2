#include <Arduino.h>

#include "Hardplace705Plus.h"
#include "Tracer.h"

static bool    fEnableSingleton(false);
static bool    fDisableFlushOnWrite(true);
#if defined    USE_THREADS
Threads::Mutex CTraceDevice::m_Mutex;
#endif


CTraceDevice::CTraceDevice(bool fForceEnable)
  : m_TraceDev(SERIAL_PORT_MONITOR) {
  if (!Enabled()
      && fForceEnable) {
    Enable();
    begin(115200);
    setTimeout(1);
    while (!m_TraceDev
           && millis() < 10000)
      ;
  }
}
CTraceDevice::CTraceDevice(const CTraceDevice&)
  : m_TraceDev(SERIAL_PORT_MONITOR) {
}
CTraceDevice& CTraceDevice::operator=(const CTraceDevice&) {
  m_TraceDev = SERIAL_PORT_MONITOR;
  return *this;
}
void CTraceDevice::Enable(bool bEnable) {
  if (!bEnable) {
    flush();
  }
  fEnableSingleton = bEnable;
}
bool CTraceDevice::Enabled(void) const {
  return fEnableSingleton;
}
void CTraceDevice::disableFlushOnWrite(bool fDisable) {
  fDisableFlushOnWrite = fDisable;
}
bool CTraceDevice::FlushOnWrite(void) const {
  return !fDisableFlushOnWrite;
}
