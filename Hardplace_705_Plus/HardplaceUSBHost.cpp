/*
  You might be able to uncomment the line: //#define USBHOST_PRINT_DEBUG
  serial.cpp if device not support
  USBSerial::product_vendor_mapping_t USBSerial::pid_vid_mapping[] = {
  // FTDI mappings.
  {0x0403, 0x6001, USBSerialBase::FTDI, 0},
  {0x0403, 0x8088, USBSerialBase::FTDI, 1},  // 2 devices try to claim at interface level
  {0x0403, 0x6010, USBSerialBase::FTDI, 1},  // Also Dual Serial, so claim at interface level
  {0x0403, 0x6015, USBSerialBase::FTDI, 0},  // Hardrock Seral
*/

#include "HardplaceUSBHost.h"

#include "Tracer.h"

static CHardplaceUSBHost _sUSBHost(19200);

USBHub&
  SerialUSBHostHub(*_sUSBHost.USBSerialHub());
USBSerial_BigBuffer&
  SerialUSBHost1(*_sUSBHost.USBSerialDevice(CHardplaceUSBHost::USBDevice::USB1));
USBSerial_BigBuffer&
  SerialUSBHost2(*_sUSBHost.USBSerialDevice(CHardplaceUSBHost::USBDevice::USB2));
USBSerial_BigBuffer&
  SerialUSBHost3(*_sUSBHost.USBSerialDevice(CHardplaceUSBHost::USBDevice::USB3));
USBSerial_BigBuffer&
  SerialUSBHost4(*_sUSBHost.USBSerialDevice(CHardplaceUSBHost::USBDevice::USB4));

CHardplaceUSBHost::CHardplaceUSBHost(uint32_t uBaud, uint32_t uFormat)
  : m_baud(uBaud),
    m_format(uFormat),
    m_Hub(*this),
    m_USB1(*this, 1),
    m_USB2(*this, 1),
    m_USB3(*this, 1),
    m_USB4(*this, 1) {
  m_USB1.writeTimeOut(100);
  m_USB2.writeTimeOut(100);
  m_USB3.writeTimeOut(100);
  m_USB4.writeTimeOut(100);
}


USBHub*
CHardplaceUSBHost::USBSerialHub(void) const {
  return static_cast<USBHub*>(m_aDrivers[USBDevice::HUB]);
}
USBSerial_BigBuffer*
CHardplaceUSBHost::USBSerialDevice(USBDevice which) const {
  if (which >= USB1
      && which <= USB4) {
    return static_cast<USBSerial_BigBuffer*>(m_aDrivers[which]);
  }
  return 0;
}
void CHardplaceUSBHost::Task(void) {
  _sUSBHost._Task();
}
void CHardplaceUSBHost::_Task(void) {
  const size_t cDevices(sizeof m_aDrivers / sizeof(USBDriver*));

#if defined USE_THREADS
  Threads::Scope wait(m_Mutex);
#endif

  USBHost::Task();

  // Print out information about different devices.
  for (uint8_t nIndex(0); nIndex < cDevices; nIndex++) {
    if (*m_aDrivers[nIndex] != m_afDriverActive[nIndex]) {
      m_afDriverActive[nIndex] = *m_aDrivers[nIndex];
      
      if (m_afDriverActive[nIndex]) {
        USBSerial_BigBuffer* curDevice(USBSerialDevice(USBDevice(nIndex)));

        if (curDevice) {
          curDevice->begin(m_baud, m_format);
        }

        if (m_DebugMonitor) {
          m_DebugMonitor.printf("*** Device %s %x:%x - connected ***\r\n", m_apszDriverNames[nIndex], m_aDrivers[nIndex]->idVendor(), m_aDrivers[nIndex]->idProduct());

          const uint8_t* psz(m_aDrivers[nIndex]->manufacturer());
          if (psz && *psz) m_DebugMonitor.printf("  manufacturer: %s\r\n", psz);
          psz = m_aDrivers[nIndex]->product();
          if (psz && *psz) m_DebugMonitor.printf("  product: %s\r\n", psz);
          psz = m_aDrivers[nIndex]->serialNumber();
          if (psz && *psz) m_DebugMonitor.printf("  Serial: %s\r\n", psz);
        }
      } else {
        m_DebugMonitor.printf("*** Device %s - disconnected ***\r\n", m_apszDriverNames[nIndex]);
      }
    }
  }
}
