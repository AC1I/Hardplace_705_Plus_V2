#if !defined BOUNDDEVICE_H
#define BOUNDDEVICE_H

#include <List.hpp>

#include "Hardplace705Plus.h"
#include "SerialDevice.h"

class CBoundDevice {
public:
  enum eDeviceClass {
    Unknown
  };

public:
  CBoundDevice(eDeviceClass eDevice)
    : m_eDeviceClass(eDevice) {}
  CBoundDevice(const CBoundDevice& rhs)
    : m_eDeviceClass(rhs.m_eDeviceClass) {}
  virtual ~CBoundDevice() {}
private:
  CBoundDevice()
    : m_eDeviceClass(Unknown) {}

public:
  eDeviceClass deviceClass(void) const {
    return m_eDeviceClass;
  }

public:
  CBoundDevice& operator=(const CBoundDevice& rhs) {
    m_eDeviceClass = rhs.m_eDeviceClass;
    return *this;
  }

public:
  virtual void onNewPacket(const uint8_t* puPacket, size_t stPacket, CSerialDevice& rSrcDevice) {}
  virtual void onNewPacket(const String& rsPacket, CSerialDevice& rSrcDevice) {}
  virtual void onReceive(uint8_t uchChar, CSerialDevice& rSrcDevice) {}

private:
  eDeviceClass m_eDeviceClass;
};

class CBoundDeviceList : public List<CBoundDevice*> {
public:
  virtual ~CBoundDeviceList() {
    clear();
  }

public:
  CBoundDeviceList(){};

private:
  CBoundDeviceList(const CBoundDeviceList&);
  CBoundDeviceList& operator=(const CBoundDeviceList&);

public:
  void bind(CBoundDevice& rDevice) {
    if (!isBound(rDevice)) {
      add(&rDevice);
    } else {
      CTraceDevice().TraceLn("!!!Device is already bound!!!");
    }
  }
  void unbind(CBoundDevice& rDevice) {
    for (int nIndex(0); nIndex < getSize(); nIndex++) {
      if (get(nIndex) == &rDevice) {
        remove(nIndex);
        break;
      }
    }
  }
  bool isBound(CBoundDevice& rDevice) {
    bool fBound(false);
    for (int nIndex(0); (!fBound && nIndex < getSize()); nIndex++) {
      fBound = get(nIndex) == &rDevice;
    }
    return fBound;
  }
  void clear(void) {
    while (!isEmpty()) {
      remove(0);
    }
  }
};
#endif
