#include <memory>
#include <cstdint>
#include "HardrockPair.h"
#include "Tracer.h"

#if defined USE_THREADS
void        CHardrockPair::newHardrock(void* pThis) {
         CHardrockPair& rHardrockPair(*reinterpret_cast<CHardrockPair*>(pThis));
         Threads::Scope wait(rHardrockPair.m_Mutex);

         rHardrockPair.m_fWasCreated = rHardrockPair.newHardrock();
}
void CHardrockPair::AntennaMonitor(void* pThis) {
  reinterpret_cast<CHardrockPair*>(pThis)->monitorActiveAntenna();
}

void CHardrockPair::PTTMonitor(void* pThis) {
  reinterpret_cast<CHardrockPair*>(pThis)->monitorPTT();
}

void CHardrockPair::TunerThread(void* pThis) {  // pArgs must be allocated memory
  reinterpret_cast<CHardrockPair*>(pThis)->TunerThread();
}
#endif
bool CHardrockPair::newHardrock(void) {
  m_pHardrock.reset(CHardrock::CHardrockFactory(*this));

  if (m_pHardrock) {
    m_pHardrock->Tracer().TraceLn(
      "Found new Hardrock "
      + String(m_pHardrock->Model()));

    m_pHardrock->setup();
    if (m_rTeensy.getFrequencyHz()) {
      m_pHardrock->setFrequency(m_rTeensy.getFrequencyHz());
    }
    
    m_rTeensy.setKeyingMode(m_Port, true);

    if (m_pHardrock->isATUPresent()
        || m_pHardrock->isATUPresent()) {
      m_pHardrock->Tracer().TraceLn(
        String(m_pHardrock->Model())
        + " has ATU installed");
      m_pTuner = std::make_shared<CIC_705Tuner>(m_rTeensy, m_rIC705, *m_pHardrock);
      if (m_pTuner) {
        m_pTuner->setup();
#if defined USE_THREADS
        threads.addThread(TunerThread, this);
#endif
      } else {
        m_pHardrock.reset();
      }
    } else {
      Delay(250);
    }
  }
  return bool(m_pHardrock);
}
void CHardrockPair::onNewPacket(const uint8_t* puPacket, size_t stPacket, CSerialDevice& rSrcDevice) {
  if (m_pHardrock) {
    CICOMResp Resp(puPacket, stPacket);

    if (Resp.isFrequencyResponse()) {
      m_pHardrock->setFrequency(Resp.FrequencyHz());
    }
  }
}
void CHardrockPair::onNewPacket(const String& rsPacket, CSerialDevice& rSrcDevice) {
  if (m_pHardrock
      && CHardrock::isHardrockPacket(rsPacket)) {
    m_pHardrock->write(rsPacket);

    if (m_pHardrock->isResponseExpected(rsPacket)) {
      String sRsp(m_pHardrock->readStringUntil(';'));
      if (sRsp.length()) {
        rSrcDevice.write(sRsp.c_str(), sRsp.length());
      }
    }
  }
}
