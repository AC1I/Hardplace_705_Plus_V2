#if !defined HARDROCKUSB_H
#define HARDROCKUSB_H

#include <Arduino.h>
#include <USBHost_t36.h>  // Read this header first for key info
#include <elapsedMillis.h>

#include "Hardplace705Plus.h"
#include "SerialDevice.h"
#include "Hardrock.h"
#include "BoundDevice.h"
#include "Tracer.h"

class CHardrockUSB : public CSerialDevice, public CBoundDevice {
public:
  CHardrockUSB(Stream& rUSB)
    : CSerialDevice(rUSB, 19200),
      CBoundDevice(static_cast<CBoundDevice::eDeviceClass>(CTeensy::eBoundDeviceTypes::USBHost)),
      m_rUSBDevice(static_cast<USBSerial_BigBuffer&>(rUSB)),
      m_sDeviceName(m_pszUnknown),
      m_sModel(m_pszUnknown),
      m_tryInterval(10000),
      m_HardrockFound(false) {
    if (&rUSB == static_cast<const Stream*>(&SerialUSBHost1)) {
      m_sDeviceName = "SerialUSBHost1";
    } else if (&rUSB == static_cast<const Stream*>(&SerialUSBHost2)) {
      m_sDeviceName = "SerialUSBHost2";
    } else if (&rUSB == static_cast<const Stream*>(&SerialUSBHost3)) {
      m_sDeviceName = "SerialUSBHost3";
    } else if (&rUSB == static_cast<const Stream*>(&SerialUSBHost4)) {
      m_sDeviceName = "SerialUSBHost4";
    }
  }

private:
  CHardrockUSB(const CHardrockUSB&);
  CHardrockUSB& operator=(const CHardrockUSB&);

public:
  operator bool() {
    return bool(m_rUSBDevice);
  }
  uint16_t idVendor() {
    return (isUSBConnected()) ? m_rUSBDevice.idVendor() : 0;
  }
  uint16_t idProduct() {
    return (isUSBConnected()) ? m_rUSBDevice.idProduct() : 0;
  }
  const String manufacturer() {
    if (isUSBConnected()) {
      return String(reinterpret_cast<const char*>(m_rUSBDevice.manufacturer()));
    }
    return String();
  }
  const String product() {
    if (isUSBConnected()) {
      return String(reinterpret_cast<const char*>(m_rUSBDevice.product()));
    }
    return String();
  }
  const String serialNumber() {
    if (isUSBConnected()) {
      return String(reinterpret_cast<const char*>(m_rUSBDevice.serialNumber()));
    }
    return String();
  }

public:
  void setup() {
  }
  void Task(void) {
    if (!isUSBConnected()) {
      if (m_HardrockFound) {
        m_HardrockFound = false;
        const_cast<String&>(m_sModel) = m_pszUnknown;
        clear();
      }
    } else if (isHardrockConnected()) {
      CSerialDevice::Task();
    }
  }
  void bind(CBoundDevice& rDevice) {
    m_BoundDevices.bind(rDevice);
  }
  void unbind(CBoundDevice& rDevice) {
    m_BoundDevices.unbind(rDevice);
  }
  bool isBound(void) {
    return m_BoundDevices.getSize() > 0;
  }

public:
  String deviceName(void) const {
    return m_sDeviceName;
  }
  String modelName(void) const {
    return const_cast<String&>(m_sModel);
  }
  bool isUSBConnected(void) {
    return bool(*this);
  }
  bool isConnected(void) {
    return isUSBConnected() && m_HardrockFound;
  }
  bool isHardrockUSB(void) {
    return (isUSBConnected()
            && idVendor() == 0x0403
            && idProduct() == 0x6015);
  }

public:
  bool isHardrockConnected(void) {
    if (isHardrockUSB()) {
      if (!m_HardrockFound
          && m_tryInterval >= 10000) {
        m_tryInterval = 0;
#if defined XXXUSE_THREADS
        threads.addThread(getHardrockModel, this);
        threads.yield();
#else
        getHardrockModel();
#endif
      }
    } else if (m_HardrockFound) {
      m_HardrockFound = false;
      const_cast<String&>(m_sModel) = m_pszUnknown;
    }

    return m_HardrockFound;
  }

protected:
  bool getHardrockModel(void);
#if defined USE_THREADS
  static bool getHardrockModel(void* pThis);
#endif
  virtual void onAvailable(void) {
    if (m_rUSBDevice) {
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
    if (CHardrock::isHardrockPacket(rsPacket)) {
      write(rsPacket.c_str(), rsPacket.length());
    }
  }


private:
  USBSerial_BigBuffer& m_rUSBDevice;
  CBoundDeviceList     m_BoundDevices;
  const char*          m_pszUnknown = "Unknown";
  String               m_sDeviceName;
  volatile String      m_sModel;
  elapsedMillis        m_tryInterval;
  volatile bool        m_HardrockFound;
#if defined USE_THREADS
  Threads::Mutex m_Mutex;
#endif
};
#endif