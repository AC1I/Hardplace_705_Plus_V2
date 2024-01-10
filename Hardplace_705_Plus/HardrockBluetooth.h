#if !defined HARDROCKBLUETOOTH_H_DEFINED
#define HARDROCKBLUETOOTH_H_DEFINED

#include <Arduino.h>

#include "Hardplace705Plus.h"
#include "HC_06.h"
#include "BoundDevice.h"
#include "Tracer.h"


class CHardrockBluetoothSlaveDevice : public CBluetoothSlaveDevice, public CBoundDevice {
public:
  CHardrockBluetoothSlaveDevice(
    Stream& rDevice, const char* pszName = "Hardplace Slave", uint32_t uBaudrate = 38400,
    unsigned long ulTimeout = 1000)
    : CBluetoothSlaveDevice(
      rDevice, pszName, uBaudrate, ulTimeout),
      CBoundDevice(static_cast<CBoundDevice::eDeviceClass>(CTeensy::eBoundDeviceTypes::Bluetooth)) {
  }

private:
  CHardrockBluetoothSlaveDevice();
  CHardrockBluetoothSlaveDevice(const CHardrockBluetoothSlaveDevice&);
  CHardrockBluetoothSlaveDevice& operator=(const CHardrockBluetoothSlaveDevice&);

public:
  virtual void Task(void) {
    CSerialDevice::Task();
  }
  void bind(CBoundDevice& rDevice) {
    m_BoundDevices.bind(rDevice);
  }
  void unbind(CBoundDevice& rDevice) {
    m_BoundDevices.unbind(rDevice);
  }
  bool isBound(CBoundDevice::eDeviceClass eDevice) {
    bool fBound(false);

    for (int nIndex(0); (!fBound && nIndex < m_BoundDevices.getSize()); nIndex++) {
      fBound = (eDevice == m_BoundDevices.get(nIndex)->deviceClass());
    }
    return fBound;
  }
  virtual void onAvailable(void) {
    if (peek() == 0xFE) {
      const size_t stBuf(128);
      uint8_t*     pauchBuf(new uint8_t[stBuf]);
      size_t       stRead(readBytesUntil(0xFD, pauchBuf, stBuf));

      for (int nIndex(0); nIndex < m_BoundDevices.getSize(); nIndex++) {
        m_BoundDevices.get(nIndex)->onNewPacket(pauchBuf, stRead, *this);
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
        for (int nIndex(0); nIndex < m_BoundDevices.getSize(); nIndex++) {
          m_BoundDevices.get(nIndex)->onNewPacket(sData, *this);
        }
      }
    }
  }
  virtual void onNewPacket(const String& rsPacket, CSerialDevice& rSrcDevice) {
    write(rsPacket.c_str(), rsPacket.length());
  }

private:
  CBoundDeviceList m_BoundDevices;
};
#endif
