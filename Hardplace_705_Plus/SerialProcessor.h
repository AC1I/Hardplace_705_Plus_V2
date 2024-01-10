#if !defined SERIALPROCESSOR_H_DEFINED
#define SERIALPROCESSOR_H_DEFINED

#include <cstdint>

#include "Hardplace705Plus.h"
#include "SerialDevice.h"
#include "BoundDevice.h"
#include "Tracer.h"

class CSerialProcessor : public CSerialDevice, public CBoundDeviceList {
public:
  CSerialProcessor(Stream& rDevice, uint32_t uBaudrate = 115200)
    : CSerialDevice(rDevice, uBaudrate) {
  }

private:
  CSerialProcessor();
  CSerialProcessor(const CSerialProcessor&);
  CSerialProcessor& operator=(const CSerialProcessor&);

public:
  virtual void setup(void) {
    begin();
  }
  virtual void Task(void) {
    CSerialDevice::Task();
  }
  virtual void onAvailable(void) {
    if (peek() == 0xFE) {
      const size_t stBuf(128);
      uint8_t*     pauchBuf(new uint8_t[stBuf]);
      size_t       stRead(readBytesUntil(0xFD, pauchBuf, stBuf));

      for (int nIndex(0); nIndex < getSize(); nIndex++) {
        get(nIndex)->onNewPacket(pauchBuf, stRead, *this);
      }
      delete[] pauchBuf;
    } else {
      while (available()
             && !isAlphaNumeric(peek())
             && peek() != ';') {
        read();
      }
      if (available()) {
        String sData(readStringUntil(';'));
        for (int nIndex(0); nIndex < getSize(); nIndex++) {
          get(nIndex)->onNewPacket(sData, *this);
        }
      }
    }
  }
};
#endif
