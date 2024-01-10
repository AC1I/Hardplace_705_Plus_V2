#include "HardrockUSB.h"

#if defined USE_THREADS

bool CHardrockUSB::getHardrockModel(void* pThis) {
  CHardrockUSB*  pHardrockUSB(reinterpret_cast<CHardrockUSB*>(pThis));
  Threads::Scope wait(pHardrockUSB->m_Mutex);
  return pHardrockUSB->getHardrockModel();
}
#endif

bool CHardrockUSB::getHardrockModel(void) {
  if (!m_HardrockFound) {
    CHardrock* pHardrock(CHardrock::CHardrockFactory(*this));
    if (pHardrock) {
      const_cast<String&>(m_sModel) = pHardrock->Model();
      Tracer().TraceLn(
        deviceName()
        + " Connected to "
        + pHardrock->Model());
      delete pHardrock;
      m_HardrockFound = true;
    } else {
      Tracer().TraceLn(deviceName() + " Hardrock Not Found");
      const_cast<String&>(m_sModel) = m_pszUnknown;
    }
  }
  return m_HardrockFound;
}
