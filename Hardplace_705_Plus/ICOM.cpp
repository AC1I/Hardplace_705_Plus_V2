#include "ICOM.h"

unsigned char CICOMReq::m_uchICOMAddr = 0xA4;

void CICOMResp::setICOMAddress(void) {
  setICOMAddress(m_pPacket, m_stPacket);
}

void CICOMResp::setICOMAddress(unsigned char* pPacket, size_t stPacketLen) {
  if (pPacket && stPacketLen >= 4) {
    setICOMAddress(pPacket[3]);
  }
}

void CICOMResp::setICOMAddress(unsigned char uchAddress) {
  if (CICOMReq().getICOMAddress() != uchAddress) {
    CICOMReq().setICOMAddress(uchAddress);
  }
}

unsigned char CICOMResp::getICOMAddress(void) const {
  return CICOMReq().getICOMAddress();
}

uint32_t CICOMResp::FrequencyMeters(uint64_t ullFrequencyHz) {
  uint32_t uMeters(lround(299792458.0 / ullFrequencyHz));

  if (uMeters >= 150) {
    uMeters = 160;
  } else if (uMeters >= 80) {
    uMeters = 80;
  } else if (uMeters >= 50) {
    uMeters = 60;
  } else if (uMeters >= 40) {
    uMeters = 40;
  } else if (uMeters >= 29) {
    uMeters = 30;
  } else if (uMeters >= 20) {
    uMeters = 20;
  } else if (uMeters >= 16) {
    uMeters = 17;
  } else if (uMeters >= 14) {
    uMeters = 15;
  }  else if (uMeters >= 12) {
    uMeters = 12;
  } else if (uMeters >= 10) {
    uMeters = 10;
  } else if (uMeters >= 5) {
    uMeters = 6;
  } else if (uMeters >= 2) {
    uMeters = 2;
  } else {
    uMeters = 1;
  }
  return uMeters;
}
