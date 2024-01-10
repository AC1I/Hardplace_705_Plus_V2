#include "EEPromStream.h"

static class CEEPROMSCompactor : public CEEPROMStream {
  public:
  CEEPROMSCompactor() : CEEPROMStream(Master, VER_MASTER) {
    compact();
  }
  
} _compact;

bool CEEPROMStream::compact(void) {
  bool fCompacted(false);
  size_t stLength(CEEPROMStream().end());

  if (stLength) {
    uint8_t* pBuffer(new uint8_t[stLength]);
    uint8_t* pDest(pBuffer);
    sHeader Header;

    memset(pBuffer, 0xFF, stLength);
    readHeader(0, Header);
    if (Header.m_uchRecordType != Master) {
      CEEPROMStream().clear();
      stLength = CEEPROMStream().end();
    }
    for (int iAddress(0); iAddress < static_cast<int>(stLength);) {
      readHeader(iAddress, Header);
      if (Header.m_uchRecordType != Invalid
          && Header.m_uchRecordType != Free
          && Header.m_uchRecordType != Eof) {
        for (int iIter(0); iIter < static_cast<int>(Header.m_usRecordLen); iIter++) {
          *pDest++ = EEPROM.read(iAddress++);
        }
      } else {
        fCompacted = true;
        iAddress += static_cast<int>(Header.m_usRecordLen);
      }
    }
    if (fCompacted) {
      for (int iAddress(0); iAddress < static_cast<int>(stLength); iAddress++) {
        EEPROM.update(iAddress, pBuffer[iAddress]);
      }
    }

    delete[] pBuffer;
  }
  return fCompacted;
}

int CEEPROMStream::readHeader(int iAddress, sHeader& rHeader) {
  EEPROM.get(iAddress, rHeader.m_ulMagic),       iAddress += sizeof (uint32_t);
  EEPROM.get(iAddress, rHeader.m_uchRecordType), iAddress += sizeof (uint8_t);
  EEPROM.get(iAddress, rHeader.m_uchVersion),    iAddress += sizeof (uint8_t);
  EEPROM.get(iAddress, rHeader.m_usRecordLen),   iAddress += sizeof (uint16_t);
  return iAddress;
}
