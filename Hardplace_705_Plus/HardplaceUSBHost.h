#if !defined HARDPLACEUSBHOST_H
#define HARDPLACEUSBHOST_H

#include <Arduino.h>
#include <USBHost_t36.h>  // Read this header first for key info

#include "Hardplace705Plus.h"
#include "Tracer.h"

class CHardplaceUSBHost : public USBHost {
public:
  CHardplaceUSBHost(uint32_t uBaud = 115200, uint32_t uFormat = USBHOST_SERIAL_8N1);

private:
  CHardplaceUSBHost(const CHardplaceUSBHost&);
  CHardplaceUSBHost& operator=(const CHardplaceUSBHost&);

public:
  enum USBDevice {
    HUB,
    USB1,
    USB2,
    USB3,
    USB4
  };

public:
  USBHub*              USBSerialHub(void) const;
  USBSerial_BigBuffer* USBSerialDevice(USBDevice which) const;

public:
  static void Task(void);

private:
  void _Task(void);

private:
  uint32_t            m_baud;
  uint32_t            m_format;
  USBHub              m_Hub;
  USBSerial_BigBuffer m_USB1;
  USBSerial_BigBuffer m_USB2;
  USBSerial_BigBuffer m_USB3;
  USBSerial_BigBuffer m_USB4;
  USBDriver*          m_aDrivers[5] = { &m_Hub, &m_USB1, &m_USB2, &m_USB3, &m_USB4 };
  const char*         m_apszDriverNames[5] = { "Hub", "SerialUSBHost1", "SerialUSBHost2", "SerialUSBHost3", "SerialUSBHost4" };
  bool                m_afDriverActive[5] = { false, false, false, false, false };
  CTraceDevice        m_DebugMonitor;
#if defined USE_THREADS
  Threads::Mutex      m_Mutex;
#endif
};

extern USBHub&              SerialUSBHostHub;
extern USBSerial_BigBuffer& SerialUSBHost1;
extern USBSerial_BigBuffer& SerialUSBHost2;
extern USBSerial_BigBuffer& SerialUSBHost3;
extern USBSerial_BigBuffer& SerialUSBHost4;

#endif
