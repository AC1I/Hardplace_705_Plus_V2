#if !defined TRACER_H_DEFINED
#define TRACER_H_DEFINED

#include <Stream.h>
#include <usb_serial.h>

#include "Hardplace705Plus.h"

#if defined USE_THREADS
#define autoLock() Threads::Scope _wait_(m_Mutex)
#else
#define autoLock()
#endif

class CTraceDevice : public Stream {
public:
  CTraceDevice(bool fForceEnable = false);
  CTraceDevice(const CTraceDevice&);

public:
  CTraceDevice& operator=(const CTraceDevice&);

private:
  virtual int available() {
    if (m_TraceDev) {
      return m_TraceDev.available();
    }
    return 0;
  }
  virtual int read() {
    if (m_TraceDev) {
      return m_TraceDev.read();
    }
    return 0;
  }
  virtual int peek() {
    if (m_TraceDev) {
      return m_TraceDev.peek();
    }
    return -1;
  }
  virtual void flush() {
    if (m_TraceDev) {
      m_TraceDev.flush();
    }
  }
  virtual size_t write(uint8_t b) {
    return write(&b, 1);
  }
  virtual size_t write(const uint8_t* buffer, size_t size) {
    size_t cBytes(0);
    if (Available()
        && m_TraceDev.availableForWrite() > static_cast<int>(size)) {
      cBytes = m_TraceDev.write(buffer, size);
      if (FlushOnWrite()) {
        m_TraceDev.flush();
      }
    }
    return cBytes;
  }
  void clear(void) {
    if (m_TraceDev) {
      m_TraceDev.clear();
    }
  }

public:
  void begin(uint32_t uBPS) {
    m_TraceDev.begin(uBPS);
  }

  void end(void) {
    m_TraceDev.end();
  }

  void setTimeout(unsigned uTimeout) {
    m_TraceDev.setTimeout(uTimeout);
  }

  bool dtr(void) {
    return m_TraceDev.dtr();
  }

  operator bool() {
    return m_TraceDev;
  }
  operator usb_serial_class&() {
    return m_TraceDev;
  }

  operator usb_serial_class*() {
    return &m_TraceDev;
  }

public:
  void Enable(bool bEnable = true);

  void Disable(void) {
    Enable(false);
  }

  bool Enabled(void) const;

  bool Available(void) {
    return (Enabled()
            && m_TraceDev
            && m_TraceDev.dtr()
            && m_TraceDev.availableForWrite());
  }

  void disableFlushOnWrite(bool fDisable = true);
  bool FlushOnWrite(void) const;

public:
  void Trace(unsigned char val) {
    autoLock();
    if (Available()) {
      write(val);
    }
  }
  void Trace(const char* str) {
    autoLock();
    if (Available()) {
      print(str);
    }
  }

  void Trace(unsigned char* buf, size_t len) {
    autoLock();
    if (Available()) {
      write(buf, len);
    }
  }

  void Trace(const String& rString) {
    autoLock();
    if (Available()) {
      print(rString.c_str());
    }
  }

  size_t TraceLn(const char* val) {
    autoLock();
    if (Available()) {
      return println(val);
    }
    return 0;
  }

  size_t TraceLn(const String& rStr) {
    return TraceLn(rStr.c_str());
  }

  size_t TracePrint(const char* val) {
    autoLock();
    if (Available()) {
      return print(val);
    }
    return 0;
  }

  size_t TracePrint(unsigned val, int format) {
    autoLock();
    if (Available()) {
      return print(val, format);
    }
    return 0;
  }

  size_t TraceHex(const unsigned char* pBuffer, size_t stBufLen) {
    autoLock();
    if (Available()) {
      for (size_t nIndex = 0; nIndex < stBufLen; nIndex++) {
        if (pBuffer[nIndex] <= 0x0F) {
          print("0");
        }
        print(pBuffer[nIndex], HEX);
        if (nIndex != stBufLen - 1) {
          print(" ");
        }
      }
      println("");
      return (stBufLen + 2);
    }
    return 0;
  }
  size_t TraceHex(unsigned char uchChar) {
    autoLock();
    if (Available()) {
      if (uchChar <= 0x0F) {
        print("0");
      }
      print(uchChar, HEX);
      print(" ");
      return 1;
    }
    return 0;
  }

private:
  usb_serial_class& m_TraceDev;
#if defined USE_THREADS
  static Threads::Mutex m_Mutex;
#endif
};
#undef autoLock
#endif
