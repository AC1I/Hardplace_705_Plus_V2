#if !defined IC_705_MASTER_H_DEFINED
#define IC_705_MASTER_H_DEFINED

#include <cstdint>
#include <cstddef>
#include <memory>
#include <elapsedMillis.h>
#include "core_pins.h"
#include "List.hpp"

#include "Hardplace705Plus.h"
#include "HC_05Master.h"
#include "ICOM.h"
#include "BoundDevice.h"

class CIC_705MasterDevice : public CHC_05MasterDevice, private CICOMReq, public CBoundDevice {
public:
  CIC_705MasterDevice(
    IBluetooth::TeensyBluetooth DeviceID, IBluetooth& rBluetooth, unsigned char uchRigAddress,
    Stream& rDevice, uint32_t uBaudrate = 38400, unsigned long ulTimeout = 1000,
    const char* pszName = "Hardplace 705+", const char* pszRName = "ICOM BT(IC-705)",
    const char* pszPIN = "0000", unsigned long ulClass = 220400,
    uint8_t eRecordType = CTeensy::eEEPromRecordTypes::IC_705Type)
    : CHC_05MasterDevice(
      DeviceID, rBluetooth, rDevice, uBaudrate, ulTimeout, pszName, pszRName, pszPIN,
      ulClass, eRecordType),
      CICOMReq(uchRigAddress),
      CBoundDevice(static_cast<CBoundDevice::eDeviceClass>(CTeensy::eBoundDeviceTypes::IC_705Master)) {
  }
  ~CIC_705MasterDevice() {
    while (!m_BoundDevices.isEmpty()) {
      delete m_BoundDevices.get(0);
      m_BoundDevices.remove(0);
    }
  }

public:
  virtual bool setup(void) {
    return CHC_05MasterDevice::setup();
  }
  virtual void Task(void) {
    CHC_05MasterDevice::Task();
  }

public:
  static bool isCloneRequest(const uint8_t* pPacket, size_t stPacketLen) {
    return CICOMResp::isClonePacket(pPacket, stPacketLen);
  }

public:
  bool ReadOperatingFreq(bool fWait = false) {
    bool fReturn(false);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      fReturn = CICOMReq::ReadOperatingFreq(*this);
      m_Mutex.unlock();
      Task();
    }
    return fReturn;
  }

  size_t ReadModeFilter(bool fWait = false) {
    size_t stReturn(0);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      stReturn = CICOMReq::ReadModeFilter(*this);
      m_Mutex.unlock();
    }
    return stReturn;
  }

  size_t ReadRFPower(bool fWait = false) {
    size_t stReturn(0);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      stReturn = CICOMReq::ReadRFPower(*this);
      m_Mutex.unlock();
    }
    return stReturn;
  }

  size_t WriteModeFilter(unsigned uMode, unsigned uFilter, bool fWait = false) {
    size_t stReturn(0);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      stReturn = CICOMReq::WriteModeFilter(*this, uMode, uFilter);
      m_Mutex.unlock();
    }
    return stReturn;
  }

  size_t WriteRFPower(unsigned uLevel, bool fWait = false) {
    size_t stReturn(0);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      stReturn = CICOMReq::WriteRFPower(*this, uLevel);
      m_Mutex.unlock();
    }
    return stReturn;
  }

  bool TX(bool bOn, bool fWait = false) {
    bool fReturn(false);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      fReturn = CICOMReq::TX(*this, bOn);
      m_Mutex.unlock();
    }
    return fReturn;
  }

  bool isTransmitting(bool fWait = false) {
    bool fReturn(false);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      fReturn = CICOMReq::isTransmitting(*this);
      m_Mutex.unlock();
    }
    return fReturn;
  }

  bool Tuner(bool bOn, bool fWait = false) {
    bool fReturn(false);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      fReturn = CICOMReq::Tuner(*this, bOn);
      m_Mutex.unlock();
    }
    return fReturn;
  }

  bool isTunerSelect_AH_705(bool fWait = false) {
    bool fReturn(false);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      fReturn = CICOMReq::isTunerSelect_AH_705(*this);
      m_Mutex.unlock();
    }
    return fReturn;
  }

  bool getCI_V_Transcieve(bool fWait = false) {
    bool fReturn(false);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      fReturn = CICOMReq::CI_V_Transcieve(*this);
      m_Mutex.unlock();
      Task();
    }
    return fReturn;
  }

  bool setCI_V_Transcieve(bool bOn, bool fWait = false) {
    bool fReturn(false);
    if ((fWait && m_Mutex.lock())
        || m_Mutex.try_lock()) {
      fReturn = CICOMReq::CI_V_Transcieve(*this, bOn);
      m_Mutex.unlock();
      Task();
    }
    return fReturn;
  }

public:
  void setICOMAddress(uint8_t uchAddress) {
    return CICOMReq::setICOMAddress(uchAddress);
  }

  uint8_t getICOMAddress(void) const {
    return CICOMReq::getICOMAddress();
  }

  uint8_t getRigAddress(void) const {
    return CICOMReq::getRigAddress();
  }

public:
  size_t getResponse(uint8_t* puchBuffer, size_t stLen) {
    return CICOMReq::getResponse(*this, puchBuffer, stLen);
  }

  bool getRigResponse(void) {
    return CICOMReq::getRigResponse(*this);
  }

private:
  class CICOMBoundDevice : public CBoundDevice {
  public:
    CICOMBoundDevice(CBoundDevice& rDevice, uint8_t uchAddress = 0xE0)
      : CBoundDevice(rDevice), m_rBoundDevice(rDevice), m_uchAddress(uchAddress) {}
    CICOMBoundDevice(const CICOMBoundDevice& rSrc)
      : CBoundDevice(rSrc), m_rBoundDevice(rSrc.m_rBoundDevice), m_uchAddress(rSrc.m_uchAddress) {}
    virtual ~CICOMBoundDevice() {}

  private:
    CICOMBoundDevice();
    CICOMBoundDevice& operator=(const CICOMBoundDevice&);

  public:
    virtual void onNewPacket(const uint8_t* puPacket, size_t stPacket, CSerialDevice& rSrcDevice) {
      m_rBoundDevice.onNewPacket(puPacket, stPacket, rSrcDevice);
    }
    virtual void onNewPacket(const String& rsPacket, CSerialDevice& rSrcDevice) {
      m_rBoundDevice.onNewPacket(rsPacket, rSrcDevice);
    };
    virtual void onReceive(uint8_t uchChar, CSerialDevice& rSrcDevice) {
      m_rBoundDevice.onReceive(uchChar, rSrcDevice);
    };

  public:
    uint8_t getAddress(void) const {
      return m_uchAddress;
    }
  private:
    CBoundDevice& m_rBoundDevice;
    uint8_t       m_uchAddress;
  };

public:
  void bindDevice(CBoundDevice& rDevice, uint8_t uchAddress) {
    m_BoundDevices.add(new CICOMBoundDevice(rDevice, uchAddress));
  }
  void unbindDevice(CBoundDevice& rDevice) {
    for (int nIndex(0); nIndex < m_BoundDevices.getSize(); nIndex++) {
      if (&static_cast<CBoundDevice&>(*m_BoundDevices.get(nIndex)) == &rDevice) {
        delete m_BoundDevices.get(nIndex);
        m_BoundDevices.remove(nIndex);
        break;
      }
    }
  }
  virtual void onAvailable(void) {
    if (available() >= 6) {
      size_t    stBuf(128);
      uint8_t*  pauchBuf(new uint8_t[stBuf]);
      size_t    cBytes(readBytesUntil(0xFD, pauchBuf, stBuf));
      CICOMResp Resp(pauchBuf, cBytes);

      if (Resp.isBroadcast()
          || Resp.isFrequencyResponse()
          || Resp.isOperatingModeResponse()) {
        for (int nIndex(0); nIndex < m_BoundDevices.getSize(); nIndex++) {
          m_BoundDevices.get(nIndex)->onNewPacket(pauchBuf, cBytes, *this);
        }
      } else {
        for (int nIndex(0); nIndex < m_BoundDevices.getSize(); nIndex++) {
          CICOMBoundDevice* pDevice(m_BoundDevices.get(nIndex));
          if (pDevice && pDevice->getAddress() == Resp.ToAddress()) {
            pDevice->onNewPacket(pauchBuf, cBytes, *this);
            break;
          }
        }
      }
      delete[] pauchBuf;
    }
  }
  virtual void onNewPacket(const uint8_t* puPacket, size_t stPacket, CSerialDevice& rSrcDevice) {
    const size_t               stBuf(128);
    std::shared_ptr<uint8_t[]> pauchBuf(new uint8_t[stBuf]);

    enum cloanCmds {
      getCloneInfo = 0xE0,
      CloneInfo,
      CloanRead,
      CloanWrite,
      CloanRecord,
      CloanEnd
    };

    if (isCloneRequest(puPacket, stPacket)
        && stPacket >= 5
        && (puPacket[4] == CloanRead
            || puPacket[4] == CloanWrite)) {

      memcpy(pauchBuf.get(), puPacket, stPacket);
      if (puPacket[4] == CloanRead) {
#if 0 && defined(USE_THREADS)
      threads.addThread(onCloanRead, new cloanArgs(this, rSrcDevice, pauchBuf, stPacket, pauchBuf, stBuf));
      threads.yield();
#else
        onCloanRead(rSrcDevice, pauchBuf, stPacket, pauchBuf, stBuf);
#endif
      } else {
#if 0 && defined(USE_THREADS)
      threads.addThread(onCloanWrite, new cloanArgs(this, rSrcDevice, pauchBuf, stPacket, pauchBuf, stBuf));
      threads.yield();
#else
        onCloanWrite(rSrcDevice, pauchBuf, stPacket, pauchBuf, stBuf);
#endif
      }
    } else {
      write(puPacket, stPacket);
      size_t cBytes(getResponse(pauchBuf.get(), stBuf));
      rSrcDevice.write(pauchBuf.get(), cBytes);
    }
  }

private:
  size_t write(const uint8_t* pauchBuf, size_t stBuf) {
    return CSerialDevice::write(pauchBuf, stBuf);
  }

  struct cloanArgs {
    cloanArgs(CIC_705MasterDevice*       pClass,
              CSerialDevice&             rSrcDevice,
              std::shared_ptr<uint8_t[]> pauchCmd, const size_t stCmd,
              std::shared_ptr<uint8_t[]> pauchBuf, const size_t stBuf)
      : m_pClass(pClass), m_rSrcDevice(rSrcDevice),
        m_pauchCmd(pauchCmd), m_stCmd(stCmd),
        m_pauchBuf(pauchBuf), m_stBuf(stBuf) {}
    virtual ~cloanArgs() {}

    CIC_705MasterDevice*       m_pClass;
    CSerialDevice&             m_rSrcDevice;
    std::shared_ptr<uint8_t[]> m_pauchCmd;
    const size_t               m_stCmd;
    std::shared_ptr<uint8_t[]> m_pauchBuf;
    const size_t               m_stBuf;

  private:
    cloanArgs();
    cloanArgs(const cloanArgs&);
    cloanArgs& operator=(const cloanArgs&);
  };
  static void onCloanRead(void* Args) {  // Args must be allocated memory
    std::shared_ptr<cloanArgs> pArgs(reinterpret_cast<cloanArgs*>(Args));
    Threads::Scope             wait(pArgs->m_pClass->m_Mutex);

    pArgs->m_pClass->onCloanRead(
      pArgs->m_rSrcDevice, pArgs->m_pauchCmd, pArgs->m_stCmd, pArgs->m_pauchBuf, pArgs->m_stBuf);
  }
  void onCloanRead(
    CSerialDevice&             rSrcDevice,
    std::shared_ptr<uint8_t[]> pauchCmd, const size_t stCmd,
    std::shared_ptr<uint8_t[]> pauchBuf, const size_t stBuf) {
    bool fFlushOnWrite(!Tracer().FlushOnWrite());

    Tracer().disableFlushOnWrite(true);

    write(pauchCmd.get(), stCmd);
    size_t cBytes(getResponse(pauchBuf.get(), stBuf));
    rSrcDevice.write(pauchBuf.get(), cBytes);

    for (size_t cBytes(getResponse(pauchBuf.get(), stBuf)); (cBytes || available()); cBytes = getResponse(pauchBuf.get(), stBuf)) {
      if (cBytes) {
        rSrcDevice.write(pauchBuf.get(), cBytes);
        if (Tracer().availableForWrite() >= int(cBytes * 3)) {
          Tracer().TraceHex(pauchBuf.get(), cBytes);
        }
      }
      Delay(0);
    }
    Tracer().disableFlushOnWrite(fFlushOnWrite);
  }
  static void onCloanWrite(void* Args) {  // Args must be allocated memory
    std::shared_ptr<cloanArgs> pArgs(reinterpret_cast<cloanArgs*>(Args));
    Threads::Scope             wait(pArgs->m_pClass->m_Mutex);

    pArgs->m_pClass->onCloanWrite(
      pArgs->m_rSrcDevice, pArgs->m_pauchCmd, pArgs->m_stCmd, pArgs->m_pauchBuf, pArgs->m_stBuf);
  }
  void onCloanWrite(
    CSerialDevice&             rSrcDevice,
    std::shared_ptr<uint8_t[]> pauchCmd, const size_t stCmd,
    std::shared_ptr<uint8_t[]> pauchBuf, const size_t stBuf) {
    bool fFlushOnWrite(!Tracer().FlushOnWrite());

    Tracer().disableFlushOnWrite(true);

    write(pauchCmd.get(), stCmd);

    for (size_t cBytes(rSrcDevice.readBytesUntil(0xFD, pauchBuf.get(), stBuf));
         (cBytes || rSrcDevice.available());
         cBytes = rSrcDevice.readBytesUntil(0xFD, pauchBuf.get(), stBuf)) {
      if (cBytes) {
        write(pauchBuf.get(), cBytes);
        if (Tracer().availableForWrite() >= int(cBytes * 3)) {
          Tracer().TraceHex(pauchBuf.get(), cBytes);
        }
      }
      Delay(0);
    }
    Tracer().disableFlushOnWrite(fFlushOnWrite);
  }
public:
  operator Threads::Mutex&() {
    return m_Mutex;
  }

private:
  CIC_705MasterDevice(const CIC_705MasterDevice&);
  CIC_705MasterDevice& operator=(const CIC_705MasterDevice&);

private:
  List<CICOMBoundDevice*> m_BoundDevices;
  Threads::Mutex          m_Mutex;
};
#endif
