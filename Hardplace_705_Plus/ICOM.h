#if !defined ICOM_H_DEFINED
#define ICOM_H_DEFINED

#include "Hardplace705Plus.h"
#include "Teensy41.h"
#include "SerialDevice.h"

class CICOMResp {
public:
  CICOMResp(uint8_t uchRigAddress = 0xE0)
    : m_Type(Unknown),
      m_uchRigAddress(uchRigAddress),
      m_pPacket(0),
      m_stPacket(0) {
  }
  CICOMResp(const uint8_t* pResp, size_t stRespLen, uint8_t uchRigAddress = 0xE0)
    : m_Type(Unknown),
      m_uchRigAddress(uchRigAddress),
      m_pPacket(new uint8_t[stRespLen]),
      m_stPacket(stRespLen) {
    while (m_stPacket > 1
           && (pResp[0] != 0xFE || pResp[1] != 0xFE)) {
      pResp++, m_stPacket--;
    }
    memcpy(m_pPacket, pResp, m_stPacket);
    if (stRespLen > 4) {
      m_Type = static_cast<RespType>(m_pPacket[4]);
    }
    /*
         FE FE 00 A4 - 01 05 01 FD
         FE FE 00 A4 - 00 00 50 68 46 01 FD
      */
  }
  ~CICOMResp() {
    delete[] m_pPacket, m_pPacket = 0, m_stPacket = 0;
  }
private:
  CICOMResp(const CICOMResp&);
  CICOMResp& operator=(const CICOMResp&);

public:
  typedef enum RespType {
    Unknown = -1,
    SetFrequencyRig = 0,
    SetModeFilterRig,
    ReadBandEdges,
    ReadOperatingFreq,
    ReadModeFilter,
    SetFrequency,
    SetModeFilter,
    SelectVFOMode,
    SelectMemoryMode,
    WriteMemory,
    TransferMemoryToVFO,
    ClearMemory,
    ReadDuplexOffset,
    WriteDuplexOffset,
    Scan,
    SplitAndDuplex,
    SelectTuningSteps
  } RespType;

  bool isResponse(uint8_t uchRigAddress = 0xA4) const {
    return (m_stPacket > 3
            && (m_pPacket[3] == uchRigAddress || m_pPacket[2] == '\0'));
  }

  CICOMResp::RespType ResponseType(void) const {
    return m_Type;
  }

  uint8_t RigAddress(void) const {
    return m_stPacket > 3 ? m_pPacket[3] : 0xA4;
  }

  uint8_t ToAddress(void) const {
    return m_stPacket > 2 ? m_pPacket[2] : 0xE0;
  }

  bool isBroadcast(void) const {
    return ToAddress() == '\0';
  }

  static bool isBroadcast(const uint8_t* pPacket, size_t stPacketLen) {
    return stPacketLen > 2 && pPacket[2] == '\0';
  }

  static bool isClonePacket(const uint8_t* pPacket, size_t stPacketLen) {
    return stPacketLen > 5
           && pPacket[0] == 0xFE
           && pPacket[1] == 0xFE
           && ((pPacket[2] == 0xEF && pPacket[3] == 0xEE)
               || (pPacket[2] == 0xEE && pPacket[3] == 0xEF))
           && pPacket[4] >= 0xE0
           && pPacket[4] <= 0xE5;
  }

  static bool isCloneResponse(const uint8_t* pPacket, size_t stPacketLen) {
    return isClonePacket(pPacket, stPacketLen);
  }

  bool isCloneResponse(void) const {
    return isClonePacket(m_pPacket, m_stPacket);
  }

  bool isFrequencyResponse(void) const {
    return (m_Type == SetFrequencyRig
            || m_Type == ReadBandEdges
            || m_Type == ReadOperatingFreq
            || m_Type == SetFrequency);
  }

  bool isOperatingModeResponse(void) const {
    return (m_Type == SetModeFilterRig
            || m_Type == ReadModeFilter
            || m_Type == SetModeFilter);
  }

  uint64_t FrequencyHz(void) const {
    uint64_t ullFrequency(0);

    if (isFrequencyResponse()
        && m_stPacket > 10) {
      uint64_t ullMultiplier(1);

      for (size_t nIndex(5); nIndex < m_stPacket - 1; nIndex++) {
        uint8_t uchValue((((m_pPacket[nIndex] & 0xF0) >> 4) * 10) + (m_pPacket[nIndex] & 0x0F));

        ullFrequency += static_cast<uint64_t>(uchValue) * ullMultiplier;
        ullMultiplier *= 100;
      }
    }
    return ullFrequency;
  }

  uint32_t FrequencyMeters(void) const {
    if (isFrequencyResponse()) {
      return FrequencyMeters(FrequencyHz());
    }
    return 0;
  }

  static uint32_t FrequencyMeters(uint64_t ullFrequencyHz);

  bool isHF(void) const {
    if (isFrequencyResponse()) {
      return FrequencyHz() >= 3000000 && FrequencyHz() <= 30000000;
    }
    return false;
  }

  bool isVHF(void) const {
    if (isFrequencyResponse()) {
      return FrequencyHz() >= 30000000 && FrequencyHz() <= 300000000;
    }
    return false;
  }

  bool isUHF(void) const {
    if (isFrequencyResponse()) {
      return FrequencyHz() >= 300000000L && FrequencyHz() <= 3000000000;
    }
    return false;
  }

  int Mode(void) const {
    int iMode(-1);
    if (ResponseType() == ReadModeFilter
        && m_stPacket >= 6) {
      iMode = static_cast<unsigned>(m_pPacket[5]);
    }
    return iMode;
  }

  int Filter(void) const {
    int iFilter(-1);
    if (ResponseType() == ReadModeFilter
        && m_stPacket >= 7) {
      iFilter = static_cast<unsigned>(m_pPacket[6]);
    }
    return iFilter;
  }

  unsigned RFPower(void) const {
    unsigned uPower(0);

    if (ResponseType() == 0x14
        && m_stPacket > 6
        && m_pPacket[5] == 0x0A) {
      unsigned uMultiplier(1);
      for (size_t nIndex(m_stPacket - 2); nIndex >= 6; nIndex--) {
        uint8_t uchValue((((m_pPacket[nIndex] & 0xF0) >> 4) * 10) + (m_pPacket[nIndex] & 0x0F));

        uPower += static_cast<unsigned>(uchValue) * uMultiplier;
        uMultiplier *= 100;
      }
    }
    return uPower;
  }


public:
  operator const uint8_t*() {
    return m_pPacket;
  }
  operator const uint8_t*() const {
    return m_pPacket;
  }
  operator size_t() {
    return m_stPacket;
  }
  operator size_t() const {
    return m_stPacket;
  }

public:
  void    setICOMAddress(uint8_t* pPacket, size_t stPacketLen);
  void    setICOMAddress(uint8_t uchAddress);
  void    setICOMAddress(void);
  uint8_t getICOMAddress(void) const;

public:
  byte write(Stream& rOutputDev, const uint8_t* pbBuffer, size_t stBufLen) {
    return rOutputDev.write(pbBuffer, stBufLen);
  }

private:
  CICOMResp::RespType m_Type;
  uint8_t             m_uchRigAddress;
  uint8_t*            m_pPacket;
  size_t              m_stPacket;
};

class CICOMReq {
public:
  CICOMReq(uint8_t uchRigAddress = 0xE0)
    : m_uchRigAddress(uchRigAddress) {
  }

public:
  static bool isCloneRequest(const uint8_t* pPacket, size_t stPacketLen) {
    return CICOMResp::isClonePacket(pPacket, stPacketLen);
  }

public:
  size_t ReadOperatingFreq(Stream& rOutputDev) {
    const uint8_t OperatingFrequencyReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x03, 0xFD };
    return write(rOutputDev, OperatingFrequencyReq, sizeof OperatingFrequencyReq);
  }

  size_t ReadModeFilter(Stream& rOutputDev) {
    static const uint8_t ReadModeFilterReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x04, 0xFD };
    return write(rOutputDev, ReadModeFilterReq, sizeof ReadModeFilterReq);
  }

  size_t ReadRFPower(Stream& rOutputDev) {
    static const uint8_t ReadRFLevelReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x14, 0x0A, 0xFD };
    return write(rOutputDev, ReadRFLevelReq, sizeof ReadRFLevelReq);
  }

  size_t WriteModeFilter(Stream& rOutputDev, unsigned uMode, unsigned uFilter) {
    uint8_t WriteModeFilterReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x01, 0x00, 0x00, 0xFD };
    WriteModeFilterReq[5] = static_cast<uint8_t>(uMode);
    WriteModeFilterReq[6] = static_cast<uint8_t>(uFilter);
    return (write(rOutputDev, WriteModeFilterReq, sizeof WriteModeFilterReq) > 0);
  }

  size_t WriteRFPower(CSerialDevice& rOutputDev, unsigned uLevel) {
    uint8_t  WriteRFPowerReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x14, 0x0A, 0x00, 0x00, 0xFD };
    unsigned uLevelLow(uLevel % 100);
    unsigned uLevelHigh(uLevel / 100);

    WriteRFPowerReq[6] = ((uLevelHigh / 10) << 4) + uLevelHigh % 10;
    WriteRFPowerReq[7] = ((uLevelLow / 10) << 4) + uLevelLow % 10;

    return (write(rOutputDev, WriteRFPowerReq, sizeof WriteRFPowerReq) > 0
            && getRigResponse(rOutputDev));
  }

  bool TX(CSerialDevice& rOutputDev, bool bOn) {
    uint8_t TXOnOffReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x1C, 0x00, 0x00, 0xFD };

    TXOnOffReq[6] = (bOn) ? 0x01 : 0x00;
    return (write(rOutputDev, TXOnOffReq, sizeof TXOnOffReq) > 0
            && getRigResponse(rOutputDev));
  }

  bool isTransmitting(CSerialDevice& rOutputDev) {
    uint8_t TXOnOffReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x1C, 0x00, 0xFD };
    if (write(rOutputDev, TXOnOffReq, sizeof TXOnOffReq) > 0) {
      uint8_t auchResp[80];
      size_t  cRead;
      do {
        cRead = getResponse(rOutputDev, auchResp, sizeof auchResp);
      } while (cRead
               && (cRead < 8
                   || !CICOMResp(auchResp, cRead).isResponse()
                   || auchResp[4] != 0x1C
                   || auchResp[5] != 0x00));
      return (auchResp[6] == 0x01);
    }
    return false;
  }

  bool Tuner(CSerialDevice& rOutputDev, bool bOn) {
    uint8_t TunerOnOffReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x1C, 0x01, 0x00, 0xFD };

    TunerOnOffReq[6] = (bOn) ? 0x01 : 0x00;
    return (write(rOutputDev, TunerOnOffReq, sizeof TunerOnOffReq) > 0
            && getRigResponse(rOutputDev));
  }

  bool isTunerSelect_AH_705(CSerialDevice& rOutputDev) {
    uint8_t TunerTypeReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x1A, 0x05, 0x03, 0x65, 0xFD };

    if (write(rOutputDev, TunerTypeReq, sizeof TunerTypeReq) > 0) {
      unsigned char auchResp[80];
      size_t        cRead;
      do {
        cRead = getResponse(rOutputDev, auchResp, sizeof auchResp);
      } while (cRead
               && (cRead < 10
                   || !CICOMResp(auchResp, cRead).isResponse()
                   || auchResp[4] != 0x1A
                   || auchResp[5] != 0x05
                   || auchResp[6] != 0x03
                   || auchResp[7] != 0x65));
      return (cRead >= 10
              && auchResp[8] == 0x00);
    }
    return false;
  }

  bool CI_V_Transcieve(CSerialDevice& rOutputDev) {
    if (getICOMAddress() == 0xA4) {  // The 1A commands differ by Transciever, don't issue if not IC-705
      uint8_t TranscieveReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x1A, 0x05, 0x01, 0x31, 0xFD };

      if (write(rOutputDev, TranscieveReq, sizeof TranscieveReq) > 0) {
        unsigned char auchResp[80];
        size_t        cRead;
        do {
          cRead = getResponse(rOutputDev, auchResp, sizeof auchResp);
        } while (cRead
                 && (cRead < 10
                     || !CICOMResp(auchResp, cRead).isResponse()
                     || auchResp[4] != 0x1A
                     || auchResp[5] != 0x05
                     || auchResp[6] != 0x01
                     || auchResp[7] != 0x31));
        return (cRead >= 10
                && auchResp[8] == 0x01);
      }
    }
    return false;
  }

  bool CI_V_Transcieve(CSerialDevice& rOutputDev, bool bOn) {
    if (getICOMAddress() == 0xA4) {  // The 1A commands differ by Transciever, don't issue if not IC-705
      uint8_t TranscieveReq[] = { 0xFE, 0xFE, getICOMAddress(), getRigAddress(), 0x1A, 0x05, 0x01, 0x31, 0x01, 0xFD };

      TranscieveReq[8] = (bOn) ? 0x01 : 0x00;
      return (write(rOutputDev, TranscieveReq, sizeof TranscieveReq) > 0
              && getRigResponse(rOutputDev));
    }
    return false;
  }

public:
  void setICOMAddress(uint8_t uchAddress) {
    m_uchICOMAddr = uchAddress;
  }

  uint8_t getICOMAddress(void) const {
    return m_uchICOMAddr;
  }

  uint8_t getRigAddress(void) const {
    return m_uchRigAddress;
  }

public:
  static size_t getResponse(CSerialDevice& rStream, uint8_t* puchBuffer, size_t stLen) {
    return rStream.readBytesUntil(0xFD, puchBuffer, stLen);
  }

  bool getRigResponse(CSerialDevice& rStream) {
    uint8_t auchBuffer[32];
    size_t  stBytes;
    do {
      stBytes = getResponse(rStream, auchBuffer, sizeof auchBuffer);
    } while (stBytes > 0
             && stBytes != 6
             && auchBuffer[2] != m_uchRigAddress);

    return (stBytes == 6 && auchBuffer[4] == 0xFB);
  }

public:
  byte write(Stream& rOutputDev, const uint8_t* pbBuffer, size_t stBufLen) {
    return rOutputDev.write(pbBuffer, stBufLen);
  }

private:
  static uint8_t m_uchICOMAddr;
  uint8_t        m_uchRigAddress;
};

#endif
