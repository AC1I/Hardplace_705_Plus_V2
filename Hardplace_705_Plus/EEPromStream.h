#if !defined EEPROMSTREAM_H
#define EEPROMSTREAM_H

#include <EEPROM.h>

#include "Hardplace705Plus.h"
#include "Tracer.h"

#define VER_MASTER 1

class CEEPROMStream {
public:
  CEEPROMStream(uint8_t uRecordType, uint8_t uVersion)
    : m_stHeader(sizeof m_ulMagic + sizeof m_uchRecordType
                 + sizeof m_uchVersion + sizeof(uint16_t)),
      m_ulMagic(0xC0DEF00D), m_uchRecordType(uRecordType), m_uchVersion(uVersion),
      m_iIndexHeader(0), m_iIndexNext(0), m_fHeader(false) {
    sHeader Header;

    if (end() == 0) {
      newRecord(Master, VER_MASTER);
      m_iIndexHeader = 0;
      m_iIndexNext = end();
    } else {
      while (!m_fHeader) {
        readHeader(m_iIndexHeader, Header);
        if (Header.m_ulMagic == m_ulMagic) {
          m_fHeader = Header.m_uchRecordType == m_uchRecordType;
          if (!m_fHeader) {
            int iIndexNext(nextRecord());

            if (iIndexNext >= 0) {
              m_iIndexHeader = iIndexNext;
              m_iIndexNext = m_iIndexHeader + m_stHeader;
            } else {
              m_iIndexHeader = m_iIndexNext = end();
              break;
            }
          }
        } else {
          m_iIndexHeader = m_iIndexNext = end();
          break;
        }
      }
    }
  }
  virtual ~CEEPROMStream() {
    flush();
  }

private:
  CEEPROMStream()
    : m_stHeader(sizeof m_ulMagic + sizeof m_uchRecordType
                 + sizeof m_uchVersion + sizeof(uint16_t)),
      m_ulMagic(0xC0DEF00D), m_uchRecordType(0), m_uchVersion(0),
      m_iIndexHeader(0), m_iIndexNext(0), m_fHeader(false) {}
  CEEPROMStream(const CEEPROMStream&);
  CEEPROMStream& operator=(const CEEPROMStream&);

  struct sHeader {
    sHeader()
      : m_ulMagic(0), m_uchRecordType(0), m_uchVersion(0), m_usRecordLen(0) {}
    uint32_t m_ulMagic;
    uint8_t  m_uchRecordType;
    uint8_t  m_uchVersion;
    uint16_t m_usRecordLen;
  };

public:
  enum eReservedRecordTypes {
    Master = 0,
    BeginAvailable,
    Invalid = 0xFD,
    Free = 0xFE,
    Eof = 0xFF
  };

public:
  template< typename T > T& get(T& t) {
    if (m_fHeader) {
      uint8_t* ptr((uint8_t*)&t);

      for (int count(sizeof(T)); count; --count) {
        *ptr++ = EEPROM.read(m_iIndexNext++);
      }
    }
    return t;
  }

  String& get(String& sString, size_t stMaxLength) {
    sString = String();
    for (size_t nIndex(0); nIndex < stMaxLength; nIndex++) {
      char chChar(0);

      get(chChar);
      if (chChar != '\0') {
        sString += chChar;
      }
    }
    return sString;
  }

  template< typename T > const T& put(const T& t) {
    const uint8_t* ptr((const uint8_t*)&t);

    if (!m_fHeader) {
      newRecord(m_uchRecordType, m_uchVersion);
      m_fHeader = true;
    }
    for (int count(sizeof(T)); count; --count) {
      EEPROM.update(m_iIndexNext++, *ptr++);
    }
    return t;
  }

   const String& put(String& sString, size_t stMaxLength) {
    for (size_t nIndex(0); nIndex < stMaxLength; nIndex++) {
      put((nIndex < sString.length()) ? sString.charAt(nIndex) : '\0');
    }
    return sString;
  }

  void flush(void) {
    if (m_fHeader) {
      sHeader  Header;
      uint16_t uRecordLen(m_iIndexNext - m_iIndexHeader);

      if (uRecordLen > m_stHeader) {
        readHeader(m_iIndexHeader, Header);

        if (uRecordLen > Header.m_usRecordLen) {
          EEPROM.put(
            m_iIndexHeader + sizeof m_ulMagic + sizeof m_uchRecordType + sizeof m_uchVersion,
            uRecordLen);
        }
      }
    }
  }

  void rewind(void) {
    m_iIndexNext = m_iIndexHeader + m_stHeader;
  }

  uint8_t Type(void) const {
    sHeader Header;

    readHeader(m_iIndexHeader, Header);
    return (Header.m_ulMagic == m_ulMagic) ? Header.m_uchRecordType : Invalid;
  }

  uint8_t Version(void) const {
    sHeader Header;

    readHeader(m_iIndexHeader, Header);
    return (Header.m_ulMagic == m_ulMagic) ? Header.m_uchVersion : 0;
  }

public:
  bool haveRecord(void) const {
    return m_fHeader;
  }

  int recordLength(void) {
    int iReturn(0);

    if (m_fHeader) {
      EEPROM.get(
        m_iIndexHeader + sizeof m_ulMagic + sizeof m_uchRecordType + sizeof m_uchVersion,
        iReturn);
    }
    return iReturn;
  }

  void deleteRecord(void) {
    if (m_fHeader) {
      EEPROM.update(m_iIndexHeader + sizeof m_ulMagic, Free);
    }
  }

public:
  void clear(void) {
    for (int iAddress = writeHeader(0, Master, 1); iAddress < EEPROM.length(); iAddress++) {
      EEPROM.update(iAddress, 0xFF);
    }
    m_iIndexHeader = 0;
    m_iIndexNext = recordLength();
  }

  static bool compact(void);

private:
  int end(void) const {
    int     iAddress(0);
    sHeader Header;

    for (;;) {
      readHeader(iAddress, Header);
      if (Header.m_ulMagic == m_ulMagic
          && Header.m_uchRecordType != Eof) {
        iAddress += Header.m_usRecordLen;
      } else {
        break;
      }
    }
    return iAddress;
  }

  int writeHeader(int iAddress, uint8_t uRecordType, uint8_t uVersion) {
    m_iIndexHeader = m_iIndexNext = iAddress;

    EEPROM.put(m_iIndexNext, m_ulMagic), m_iIndexNext += sizeof m_ulMagic;
    EEPROM.put(m_iIndexNext, uRecordType), m_iIndexNext += sizeof uRecordType;
    EEPROM.put(m_iIndexNext, uVersion), m_iIndexNext += sizeof uVersion;
    EEPROM.put(m_iIndexNext, uint16_t(m_stHeader)), m_iIndexNext += sizeof(uint16_t);
    return m_iIndexNext;
  }

  static int readHeader(int iAddress, sHeader& rHeader);

  int nextRecord(void) const {
    int     iAddress(m_iIndexHeader);
    sHeader Header;

    readHeader(iAddress, Header);
    if (Header.m_ulMagic == m_ulMagic) {
      iAddress += Header.m_usRecordLen;
    } else {
      iAddress = -1;
    }
    return iAddress;
  }

  void newRecord(uint8_t uRecordType, uint8_t uVersion) {
    writeHeader(end(), uRecordType, uVersion);
    flush();
  }

private:
  const size_t   m_stHeader;
  const uint32_t m_ulMagic;
  const uint8_t  m_uchRecordType;
  const uint8_t  m_uchVersion;
  int            m_iIndexHeader;
  int            m_iIndexNext;
  bool           m_fHeader;
};

class CEEPROMInitialize {
public:
  CEEPROMInitialize() {
    for (int iAddress(0); iAddress < EEPROM.length(); iAddress++) {
      EEPROM.update(iAddress, 0xFF);
    }
  }
private:
  CEEPROMInitialize(const CEEPROMInitialize&);
  CEEPROMInitialize& operator=(const CEEPROMInitialize&);
};

#endif
