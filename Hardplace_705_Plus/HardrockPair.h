#include "WString.h"
#include "TeensyThreads.h"
#include "core_pins.h"
#if !defined HARDROCKPAIR_H_DEFINED
#define HARDROCKPAIR_H_DEFINED

#include <new>
#include <memory>
#include <elapsedMillis.h>

#include "Hardplace705Plus.h"
#include "SerialDevice.h"
#include "Teensy41.h"
#include "IC_705Master.h"
#include "Hardrock.h"
#include "Hardrock500.h"
#include "IC705Tuner.h"
#include "BoundDevice.h"
#include "HardrockBluetooth.h"
#include "HardrockUSB.h"
#include "EEPromStream.h"

#define VER_HARDROCKPAIR 1

class CHardrockPair : public CSerialDevice, public CBoundDevice, private CEEPROMStream {
public:
  CHardrockPair(
    CTeensy::eHardrock eWhich, HardwareSerial& rSerialDevice, CTeensy& rTeensy,
    CIC_705MasterDevice& rIC705, unsigned char uchRigAddress = 0xE0)
    : CSerialDevice(rSerialDevice, 19200),
      CBoundDevice(static_cast<CBoundDevice::eDeviceClass>(CTeensy::eBoundDeviceTypes::Hardrock)),
      CEEPROMStream((eWhich == CTeensy::eHardrock::A)
                      ? CTeensy::eEEPromRecordTypes::HardrockAType
                      : CTeensy::eEEPromRecordTypes::HardrockBType,
                    VER_HARDROCKPAIR),
      m_Port(eWhich), m_rTeensy(rTeensy),
      m_rIC705(rIC705), m_ICOM(uchRigAddress), m_pHardrock(0), m_pTuner(0),
      m_fWasAttached(false), m_fWasCreated(false), m_bFrequencyRequested(false),
      m_uActiveAntenna(1), m_pBluetooth(0), m_pUSB(0), m_ulBaudrate(CSerialDevice::getBaudrate()) {
    Serialize(haveRecord());
    m_rIC705.bindDevice(*this, uchRigAddress);
  }
  ~CHardrockPair() {
    m_rIC705.unbindDevice(*this);
    m_pTuner.reset();
    m_pHardrock.reset();
  }

private:
  CHardrockPair();
  CHardrockPair(const CHardrockPair&);
  CHardrockPair& operator=(const CHardrockPair&);

public:
  void setup(void) {
    setBaudrate(m_ulBaudrate);
  }
  void Task(void) {
    bool fHardrockAttached(m_rTeensy.HardrockAvailable(m_Port));

    if (fHardrockAttached) {
      if (!m_fWasAttached) {
        m_fWasAttached = true;
        m_StartupDelay = 0;
      } else if (!m_pHardrock
                 && m_StartupDelay > 7000
                 && !m_fWasCreated) {
#if defined USE_THREADS
        m_fWasCreated = true;
        threads.addThread(newHardrock, this);
#else
        m_fWasCreated = newHardrock();
#endif
      } else if (m_pHardrock) {
        m_ulBaudrate = m_pHardrock->getBaudrate();
        Serialize();

        if (!m_bFrequencyRequested) {
          m_ICOM.ReadOperatingFreq(m_rIC705);
          m_bFrequencyRequested = true;
          m_Poll = 0;
#if defined USE_THREADS
          if (modelName() == String(CHardrock500::modelName())) {
            threads.addThread(AntennaMonitor, this);
          }
          threads.addThread(PTTMonitor, this);
#endif
        }
        m_pHardrock->Task();
#if !defined USE_THREADS
        if (m_pTuner
            && m_rTeensy.SendEnabled(m_Port)) {
          m_pTuner->Task();
        }
        if (m_Poll >= 1000) {
          int iKeyingMode(const_cast<CHardrock*>(m_pHardrock)->getKeyingMode());

          if (iKeyingMode >= 0) {
            m_rTeensy.setKeyingMode(m_Port, iKeyingMode == 1);
          }

          if (modelName() == String(CHardrock500::modelName())) {
            m_uActiveAntenna = const_cast<CHardrock*>(m_pHardrock)->getActiveAntenna();
            m_rTeensy.setCurrentAntenna(m_Port,
                                        (m_uActiveAntenna == 1)
                                          ? CTeensy::eAntenna::Antenna1
                                          : CTeensy::eAntenna::Antenna2);
          }
          m_Poll = 0;
        }
#endif
        if (m_pUSB) {
          m_pUSB->Task();
        }
      }
    } else if (m_fWasAttached) {
#if defined USE_THREADS
      threads.yield();
#endif
      unbind();
      m_fWasAttached = m_fWasCreated = m_bFrequencyRequested = false;
      m_pTuner.reset();
      m_pHardrock.reset();
      m_uActiveAntenna = 1;
      m_rTeensy.setKeyingMode(m_Port, true);
    }
  }
  bool isConnected(void) const {
    return m_rTeensy.HardrockAvailable(m_Port) && bool(m_pHardrock);
  }
  String modelName(void) {
    if (m_pHardrock) {
      return String(m_pHardrock->Model());
    }
    return String();
  }
  CTeensy::eHardrock serialPort(void) const {
    return m_Port;
  }
  unsigned activeAntenna(void) const {
    return m_uActiveAntenna;
  }
  bool isATUPresent(void) const {
    return bool(m_pTuner);
  }
public:
  void bind(CHardrockBluetoothSlaveDevice& rBluetooth, CHardrockUSB& rUSB) {
    rBluetooth.bind(rUSB);
    rUSB.bind(rBluetooth);
    m_pBluetooth = &rBluetooth;
    m_pUSB = &rUSB;
  }
  void unbind(void) {
    if (m_pBluetooth && m_pUSB) {
      m_pBluetooth->unbind(*m_pUSB);
      m_pUSB->unbind(*m_pBluetooth);
    }
    m_pUSB = 0, m_pBluetooth = 0;
  }
  bool isBound(void) const {
    return m_pBluetooth != 0 || m_pUSB != 0;
  }
  void Serialize(bool bLoad = false) {
    CEEPROMStream::rewind();

    if (bLoad) {
      get(m_ulBaudrate);
    } else {
      put(m_ulBaudrate);
      CEEPROMStream::flush();
    }
  }

private:
  bool newHardrock(void);
#if defined USE_THREADS
  static void newHardrock(void* pThis);
  static void AntennaMonitor(void* pThis);
  static void PTTMonitor(void* pThis);
  static void TunerThread(void* pArg);
  void
  monitorActiveAntenna(void) {
    std::shared_ptr<CHardrock> pHardrock(m_pHardrock);

    for (elapsedMillis duration(0); isConnected();) {
      if (duration >= 1000
          && !m_rTeensy.isTuning()) {
        unsigned uActiveAntenna(pHardrock->getActiveAntenna());

        if (uActiveAntenna
            && uActiveAntenna != m_uActiveAntenna) {
          m_uActiveAntenna = uActiveAntenna;
          m_rTeensy.setCurrentAntenna(
            m_Port,
            (m_uActiveAntenna == 1) ? CTeensy::eAntenna::Antenna1 : CTeensy::eAntenna::Antenna2);
        }
        duration = 0;
      }
      threads.yield();
    }
  }
  void
  monitorPTT(void) {
    std::shared_ptr<CHardrock> pHardrock(m_pHardrock);

    for (elapsedMillis duration(500); isConnected();) {
      if (duration >= 1000
          && !m_rTeensy.isTuning()) {
        int iKeyingMode(pHardrock->getKeyingMode());

        if (iKeyingMode >= 0
            || (iKeyingMode = pHardrock->getKeyingMode() >= 0)) {
          m_rTeensy.setKeyingMode(m_Port, iKeyingMode == 1);
        }
        duration = 0;
      }
      threads.yield();
    }
  }
  void
  TunerThread(void) {
    while (isConnected()) {
      if (m_rTeensy.SendEnabled(m_Port)
          && m_rTeensy.TunerEnabled()) {
        m_pTuner->Task();
      }
      threads.yield();
    }
  }
#endif

public:
  virtual void onNewPacket(const uint8_t* puPacket, size_t stPacket, CSerialDevice& rSrcDevice);
  virtual void onNewPacket(const String& rsPacket, CSerialDevice& rSrcDevice);

private:
  CTeensy::eHardrock             m_Port;
  CTeensy&                       m_rTeensy;
  CIC_705MasterDevice&           m_rIC705;
  CICOMReq                       m_ICOM;
  std::shared_ptr<CHardrock>     m_pHardrock;
  std::shared_ptr<CIC_705Tuner>  m_pTuner;
  elapsedMillis                  m_StartupDelay;
  elapsedMillis                  m_Poll;
  bool                           m_fWasAttached;
  volatile bool                  m_fWasCreated;
  bool                           m_bFrequencyRequested;
  volatile unsigned              m_uActiveAntenna;
  CHardrockBluetoothSlaveDevice* m_pBluetooth;
  CHardrockUSB*                  m_pUSB;
  uint32_t                       m_ulBaudrate;
#if defined USE_THREADS
  Threads::Mutex m_Mutex;
#endif
};

#endif