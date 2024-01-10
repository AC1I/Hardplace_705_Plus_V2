#include <sys/types.h>
#if !defined TEENSY41_H
#define TEENSY41_H

#include <cstdint>
#include <core_pins.h>
#include <elapsedMillis.h>
#include <Bounce2.h>

#include "Hardplace705Plus.h"
#include "EEPromStream.h"
#include "BoundDevice.h"
#include "HardplaceUSBHost.h"
#include "CommandProcessor.h"
#include "Tracer.h"
#include "Watchdog.h"

#define Interface struct

Interface IBluetooth {
  enum TeensyBluetooth {
    IC_705,
    EndOfList,
    Unknown = EndOfList
  };

  virtual void BluetoothPower(TeensyBluetooth uWhich, bool bOn) const = 0;
  virtual void BluetoothPowerOn(TeensyBluetooth uWhich) const = 0;
  virtual void BluetoothPowerOff(TeensyBluetooth uWhich) const = 0;
  virtual bool isBluetoothPowerOn(TeensyBluetooth uWhich) const = 0;

  virtual bool BluetoothConnected(TeensyBluetooth uWhich) = 0;
  virtual bool BluetoothConnectionStateChanged(TeensyBluetooth uWhich) = 0;

  virtual void BluetoothATCmdMode(TeensyBluetooth uWhich) const = 0;
  virtual bool BluetoothIsATCmdMode(TeensyBluetooth uWhich) const = 0;
  virtual void BluetoothDataTransferMode(TeensyBluetooth uWhich) const = 0;
  virtual bool BluetoothIsDataTransferMode(TeensyBluetooth uWhich) const = 0;
};

Interface ITuner {
  virtual bool          TunerUpdate(void) = 0;
  virtual void          TunerEnable(bool fEnable = true) = 0;
  virtual bool          TunerEnabled(void) = 0;
  virtual void          TunerDisable(void) = 0;
  virtual bool          Tune(void) = 0;
  virtual void          Tuning(bool isTuning) = 0;
  virtual bool          isTuning(void) = 0;
  virtual bool          TuneComplete(void) = 0;
  virtual void          TunerKey(bool bOn) = 0;
  virtual void          TunerEnablePTT(bool fEnable) = 0;
  virtual unsigned long TuneCmdDuration(void) = 0;
};

#undef Interface

#define VER_TEENSY 1

class CTeensy : public IBluetooth, public ITuner, private CEEPROMStream, public CBoundDevice {
public:
  enum eBoundDeviceTypes {
    Unknown = CBoundDevice::eDeviceClass::Unknown,
    Teensy,
    IC_705Master,
    Hardrock,
    Bluetooth,
    USB,
    USBHost
  };

public:
  CTeensy()
    : CEEPROMStream(TeensyType, VER_TEENSY),
      CBoundDevice(static_cast<CBoundDevice::eDeviceClass>(eBoundDeviceTypes::Teensy)),
      m_uDebounceInterval(5), m_ulFrequencyMeters(0), m_ullFrequency(0), m_InitialPwr2M(255),
      m_InitialPwr70CM(255), m_fDebugEnable(false), m_fTunerEnabled(false), m_isTuning(false),
      m_fBandChanged(false), m_CmdHandler(18) {
    // Don't forget to specify the number of commands in the constructor
    // Command specifiers must be unique for the first 4 characters
    uint uCmd(0);
    m_CmdHandler[uCmd++]("HPMP", onSetMaxPower);        // Set Max power setting for current band/antenna/amplifier
    m_CmdHandler[uCmd++]("HPIP", onSetInitialPwr);      // Set Initial Power for current band/antenna/amplifier
    m_CmdHandler[uCmd++]("HPRP", onResetInitialPwr);    // Reset power to default for current band/antenna/amplifier
    m_CmdHandler[uCmd++]("HPDP", onResetInitialPwr);    // Reset power to default for current band/antenna/amplifier
    m_CmdHandler[uCmd++]("HPVE", onGetVersion);         // Get Hardplace Version
    m_CmdHandler[uCmd++]("HPDE", onDebug);              // Debug disable "HPDE0;" enable "HPDE1;"
    m_CmdHandler[uCmd++]("HPCP", onPair);               // Delete pairings
    m_CmdHandler[uCmd++]("HPOR", onOriginal);           // Erase all settings
    m_CmdHandler[uCmd++]("HPRE", onReboot);             // Reboot
    m_CmdHandler[uCmd++]("HPCM", onClearMap);           // Clear USB to Hardrock assignments
    m_CmdHandler[uCmd++]("HPDI", onDisconnect);         // Disconnect from the IC-705
    m_CmdHandler[uCmd++]("HPAT", onATCmd);              // Issue AT command to HC-05 "HPAT+Version?" or HPATAT+Version?""
    m_CmdHandler[uCmd++]("HPPT", onPTTSwitchSettings);  // Display PTT enable/disable settings
    m_CmdHandler[uCmd++]("HPHE", onHelp);               // Help
    m_CmdHandler[uCmd++]("HPPM", onPrintPwrMaps);       // Print power maps
    m_CmdHandler[uCmd++]("HPPS", onPrintStatus);        // Print device status
    m_CmdHandler[uCmd++]("HPBL", onFlash);              // Activate bootloader (Same as pushing button)
    m_CmdHandler[uCmd++]("HPHA", onHardrockAvailable);  // Report Hardrock availability

    Serialize(haveRecord());
  }

private:
  CTeensy(const CTeensy&);
  CTeensy& operator=(const CTeensy&);

public:
  enum eTeensy41Pins {
    HR_TX_A,  // 0
    HR_RX_A,
    HR_Available_A,
    HR_Available_B,
    Tuner_Key,

    TunerStart,  // 5
    HR_A_6M_PTT_Enable,
    HR_TX_B,
    HR_RX_B,
    HR_A_10M_PTT_Enable,

    PTT_PWR,  // 10
    PTT_A_Enable,
    PTT_B_Enable,  // 12
    BT_ICOM_PWR,   // INTERNAL_LED
    BT_HR_B_RXD,

    BT_HR_B_TXD,  // 15
    BT_HR_A_TXD,
    BT_HR_A_RXD,
    HR_B_20M_PTT_Enable,  // SDA
    HR_B_17M_PTT_Enable,  // SCL

    HR_B_15M_PTT_Enable,  // 20
    HR_B_12M_PTT_Enable,
    HR_B_10M_PTT_Enable,
    HR_B_6M_PTT_Enable,  // 23
    HR_A_12M_PTT_Enable,

    HR_A_15M_PTT_Enable,  // 25
    HR_A_17M_PTT_Enable,
    HR_A_20M_PTT_Enable,
    HR_A_30M_PTT_Enable,
    HR_A_40M_PTT_Enable,

    HR_A_60M_PTT_Enable,  // 30
    HR_A_80M_PTT_Enable,
    HR_A_160M_PTT_Enable,
    HR_B_160M_PTT_Enable,
    BT_ICOM_TXD,

    BT_ICOM_RXD,  // 35
    BT_ICOM_STATE,
    BT_ICOM_KEY,
    HR_B_80M_PTT_Enable,
    HR_B_60M_PTT_Enable,

    HR_B_40M_PTT_Enable,  // 40
    HR_B_30M_PTT_Enable
  };
  /*
       Serial1 -> Hardrock A
       Serial2 -> Hardrock B
       Serial3 -> Hardrock Bluetooth B
       Serial4 -> Hardrock Bluetooth A
       Serial5 -> Unused
       Serial6 -> Unused
       Serial7 -> Unused
       Serial8 -> Icom Bluetooth
    */

  enum eHardrock {
    A,
    B,
    QRP
  };

  enum eAntenna {
    Antenna1,
    Antenna2
  };

  enum eEEPromRecordTypes {
    TeensyType = CEEPROMStream::BeginAvailable,
    IC_705Type,
    HardrockAType,
    HardrockBType
  };

  enum eBinding {
    HardrockA = eHardrock::A,
    HardrockB = eHardrock::B,
    Unbound
  };

  void setup(void) {
    Serial5.end();
    Serial6.end();
    Serial7.end();

    // HR_TX_A,                 // 0
    // HR_RX_A,
    pinMode(HR_Available_A, INPUT);
    pinMode(HR_Available_B, INPUT);
    pinMode(Tuner_Key, OUTPUT), digitalWrite(Tuner_Key, HIGH);

    pinMode(TunerStart, OUTPUT), digitalWrite(TunerStart, LOW);  // 5
    pinMode(HR_A_6M_PTT_Enable, INPUT_PULLUP);
    // HR_TX_B,
    // HR_RX_B,
    pinMode(HR_A_10M_PTT_Enable, INPUT_PULLUP);

    pinMode(PTT_PWR, OUTPUT), digitalWrite(PTT_PWR, LOW);  // 10
    pinMode(PTT_A_Enable, OUTPUT), digitalWrite(PTT_A_Enable, LOW);
    pinMode(PTT_B_Enable, OUTPUT), digitalWrite(PTT_B_Enable, LOW);
    pinMode(BT_ICOM_PWR, OUTPUT), digitalWrite(BT_ICOM_PWR, LOW);
    // BT_HR_B_RXD,

    // BT_HR_B_TXD, // 15
    // BT_HR_A_TXD,
    // BT_HR_A_RXD,
    pinMode(HR_B_20M_PTT_Enable, INPUT_PULLUP);
    pinMode(HR_B_17M_PTT_Enable, INPUT_PULLUP);

    pinMode(HR_B_15M_PTT_Enable, INPUT_PULLUP);  // 20
    pinMode(HR_B_12M_PTT_Enable, INPUT_PULLUP);
    pinMode(HR_B_10M_PTT_Enable, INPUT_PULLUP);
    pinMode(HR_B_6M_PTT_Enable, INPUT_PULLUP);  // 23
    pinMode(HR_A_12M_PTT_Enable, INPUT_PULLUP);

    pinMode(HR_A_15M_PTT_Enable, INPUT_PULLUP);  // 25
    pinMode(HR_A_17M_PTT_Enable, INPUT_PULLUP);
    pinMode(HR_A_20M_PTT_Enable, INPUT_PULLUP);
    pinMode(HR_A_30M_PTT_Enable, INPUT_PULLUP);
    pinMode(HR_A_40M_PTT_Enable, INPUT_PULLUP);

    pinMode(HR_A_60M_PTT_Enable, INPUT_PULLUP);  // 30
    pinMode(HR_A_80M_PTT_Enable, INPUT_PULLUP);
    pinMode(HR_A_160M_PTT_Enable, INPUT_PULLUP);
    pinMode(HR_B_160M_PTT_Enable, INPUT_PULLUP);
    // BT_ICOM_TXD,

    // BT_ICOM_RXD,         // 35
    pinMode(BT_ICOM_STATE, INPUT);
    pinMode(BT_ICOM_KEY, OUTPUT), digitalWrite(BT_ICOM_KEY, LOW);
    pinMode(HR_B_80M_PTT_Enable, INPUT_PULLUP);
    pinMode(HR_B_60M_PTT_Enable, INPUT_PULLUP);

    pinMode(HR_B_40M_PTT_Enable, INPUT_PULLUP);  // 40
    pinMode(HR_B_30M_PTT_Enable, INPUT_PULLUP);

    m_Tune.attach(TunerStart, OUTPUT);
    m_Tune.interval(m_uDebounceInterval);
    digitalWrite(TunerStart, LOW);

    m_bStateMap[IC_705].attach(BT_ICOM_STATE, INPUT);
    m_bStateMap[IC_705].interval(m_uDebounceInterval);

    m_bHardrockMap[A].attach(HR_Available_A, INPUT);
    m_bHardrockMap[A].interval(500);
    m_bHardrockMap[B].attach(HR_Available_B, INPUT);
    m_bHardrockMap[B].interval(500);

    delay(100);
    CHardplaceUSBHost::begin();
  }

  void Task(void) {
    update();
    CHardplaceUSBHost::Task();
    m_Watchdog.reset();
  }

public:
  virtual void BluetoothPower(TeensyBluetooth uWhich, bool bOn) const {
    if (uWhich >= 0 && uWhich < EndOfList) {
      digitalWrite(m_uPowerMap[uWhich], (bOn) ? HIGH : LOW);
    }
  }
  virtual void BluetoothPowerOn(TeensyBluetooth uWhich) const {
    BluetoothPower(uWhich, true);
  }
  virtual void BluetoothPowerOff(TeensyBluetooth uWhich) const {
    BluetoothPower(uWhich, false);
  }
  virtual bool isBluetoothPowerOn(TeensyBluetooth uWhich) const {
    return digitalRead(m_uPowerMap[uWhich]) != LOW;
  }

  virtual bool BluetoothConnected(TeensyBluetooth uWhich) {
    update();
    if (uWhich >= 0 && uWhich < EndOfList) {
      m_bStateMap[uWhich].update();
      return (isBluetoothPowerOn(uWhich)
              && m_bStateMap[uWhich].read() != LOW);
    }
    return false;
  }
  virtual bool BluetoothConnectionStateChanged(TeensyBluetooth uWhich) {
    update();
    if (uWhich >= 0 && uWhich < EndOfList) {
      return m_bStateMap[uWhich].changed();
    }
    return false;
  }
  virtual void BluetoothATCmdMode(TeensyBluetooth uWhich) const {
    if (uWhich >= 0 && uWhich <= EndOfList) {
      digitalWrite(m_uKeyMap[uWhich], HIGH);
    }
  }
  virtual bool BluetoothIsATCmdMode(TeensyBluetooth uWhich) const {
    if (uWhich >= 0 && uWhich < EndOfList) {
      return (digitalRead(m_uKeyMap[uWhich]) != LOW);
    }
    return false;
  }
  virtual void BluetoothDataTransferMode(TeensyBluetooth uWhich) const {
    if (uWhich >= 0 && uWhich < EndOfList) {
      digitalWrite(m_uKeyMap[uWhich], LOW);
    }
  }
  virtual bool BluetoothIsDataTransferMode(TeensyBluetooth uWhich) const {
    if (uWhich >= 0 && uWhich < EndOfList) {
      return (digitalRead(m_uKeyMap[uWhich]) == LOW);
    }
    return false;
  }

public:
  virtual bool TunerUpdate(void) {
    return m_Tune.update();
  }
  virtual void TunerEnable(bool fEnable = true) {
    if (m_fTunerEnabled != fEnable) {
      if (fEnable) {
        m_Tune.attach(TunerStart, INPUT_PULLUP);
      } else {
        m_Tune.attach(TunerStart, OUTPUT);
        digitalWrite(TunerStart, LOW);
      }
      m_fTunerEnabled = fEnable;
    }
  }
  virtual bool TunerEnabled(void) {
    return m_fTunerEnabled;
  }
  virtual void TunerDisable(void) {
    TunerEnable(false);
  }
  virtual bool Tune(void) {
    return m_Tune.fell();
  }
  virtual bool TuneComplete(void) {
    return m_Tune.rose();
  }
  virtual void TunerKey(bool bOn) {
    digitalWrite(Tuner_Key, (bOn) ? LOW : HIGH);
  }
  virtual unsigned long TuneCmdDuration(void) {
    return m_Tune.currentDuration();
  }
  virtual void TunerEnablePTT(bool fEnable) {
    if (fEnable
        && getMetersMapIndex(getFrequencyMeters()) >= 0) {
      digitalWrite(PTT_A_Enable, PTTEnabled(eHardrock::A));
      digitalWrite(PTT_B_Enable, PTTEnabled(eHardrock::B));
    } else {
      digitalWrite(PTT_A_Enable, LOW);
      digitalWrite(PTT_B_Enable, LOW);
    }
  }
  virtual void Tuning(bool isTuning) {
    m_isTuning = isTuning;
  }
  virtual bool isTuning(void) {
    return m_isTuning;
  }

public:
  bool HardrockAvailable(eHardrock uWhich) {
    if (uWhich >= 0 && uWhich < sizeof m_bHardrockMap / sizeof(Bounce)) {
      return m_bHardrockMap[uWhich].read() != LOW;
    }
    return false;
  }

  bool HardrockAvailableChanged(eHardrock uWhich) {
    if (uWhich >= 0 && uWhich < sizeof m_bHardrockMap / sizeof(Bounce)) {
      return m_bHardrockMap[uWhich].changed();
    }
    return false;
  }

  bool HardrockAvailableRose(eHardrock uWhich) {
    if (uWhich >= 0 && uWhich < sizeof m_bHardrockMap / sizeof(Bounce)) {
      return m_bHardrockMap[uWhich].rose();
    }
    return false;
  }

  bool TunerEnabled(eHardrock uWhich, eAntenna uWhichAnt) const {
    return (uWhich >= eHardrock::A && uWhich <= eHardrock::B
            && uWhichAnt >= Antenna1 && uWhichAnt <= Antenna2)
             ? m_fTuningEnabled[uWhich][uWhichAnt]
             : false;
  }

  bool DebugEnabled(void) const {
    return m_fDebugEnable;
  }
  void DebugEnable(bool fEnable = true) {
    m_fDebugEnable = fEnable;
    CTraceDevice().Enable(fEnable);
    Serialize();
  }

public:
  uint16_t deBounceInterval(void) const {
    return m_uDebounceInterval;
  }

public:
  void reboot(void) const;
  void Serialize(bool bLoad = false) {
    rewind();

    if (bLoad) {
      get(m_fDebugEnable);
      get(m_fTuningEnabled[0][0]);
      get(m_fTuningEnabled[0][1]);
      get(m_fTuningEnabled[1][0]);
      get(m_fTuningEnabled[1][1]);
      for (size_t stIndex(0); stIndex < sizeof m_aInitialPwr[eHardrock::A] / sizeof(uint8_t); stIndex++) {
        get(m_aInitialPwr[eHardrock::A][stIndex]);
      }
      for (size_t stIndex(0); stIndex < sizeof m_aInitialPwr[eHardrock::B] / sizeof(uint8_t); stIndex++) {
        get(m_aInitialPwr[eHardrock::B][stIndex]);
      }
      for (size_t stIndex(0); stIndex < sizeof m_aInitialPwr[eHardrock::QRP] / sizeof(uint8_t); stIndex++) {
        get(m_aInitialPwr[eHardrock::QRP][stIndex]);
      }
      get(m_InitialPwr2M);
      get(m_InitialPwr70CM);
    } else {
      put(m_fDebugEnable);
      put(m_fTuningEnabled[0][0]);
      put(m_fTuningEnabled[0][1]);
      put(m_fTuningEnabled[1][0]);
      put(m_fTuningEnabled[1][1]);
      for (size_t stIndex(0); stIndex < sizeof m_aInitialPwr[eHardrock::A] / sizeof(uint8_t); stIndex++) {
        put(m_aInitialPwr[eHardrock::A][stIndex]);
      }
      for (size_t stIndex(0); stIndex < sizeof m_aInitialPwr[eHardrock::B] / sizeof(uint8_t); stIndex++) {
        put(m_aInitialPwr[eHardrock::B][stIndex]);
      }
      for (size_t stIndex(0); stIndex < sizeof m_aInitialPwr[eHardrock::QRP] / sizeof(uint8_t); stIndex++) {
        put(m_aInitialPwr[eHardrock::QRP][stIndex]);
      }
      put(m_InitialPwr2M);
      put(m_InitialPwr70CM);
    }

    m_RFPowerMap.Serialize(*this, bLoad);

    for (size_t iter(0); iter < sizeof m_USBMap / sizeof(SHardrockUSBMap); iter++) {
      m_USBMap[iter].Serialize(*this, bLoad);
    }

    if (!bLoad) {
      flush();
    }
  }

  uint32_t getFrequencyMeters(void) const {
    return m_ulFrequencyMeters;
  }
  uint64_t getFrequencyHz(void) const {
    return m_ullFrequency;
  }
public:
  void onConnect(void) {
    m_ulFrequencyMeters = 0;
    m_ullFrequency = 0;
    digitalWrite(PTT_PWR, HIGH);
  }
  void onDisconnect(void) {
    m_ulFrequencyMeters = 0;
    m_ullFrequency = 0;
    digitalWrite(PTT_PWR, LOW);
  }
  void setCurrentAntenna(eHardrock Hardrock, eAntenna Antenna) {
    m_aCurrentAntenna[Hardrock] = Antenna;
  }
  eAntenna getCurrentAntenna(eHardrock Hardrock) {
    return m_aCurrentAntenna[Hardrock];
  }
  void setKeyingMode(eHardrock Hardrock, bool fEnabled) {
    m_aKeyingMode[Hardrock] = fEnabled;
    if (Hardrock == eHardrock::A || Hardrock == eHardrock::B) {
      digitalWrite((Hardrock == eHardrock::A) ? PTT_A_Enable : PTT_B_Enable, PTTEnabled(Hardrock));
    }
  }
  bool getKeyingMode(eHardrock Hardrock) {
    return m_aKeyingMode[Hardrock];
  }
private:
  virtual void onNewPacket(const uint8_t* puPacket, size_t stPacket, CSerialDevice& rSrcDevice);
  virtual void onNewPacket(const String& rsPacket, CSerialDevice& rSrcDevice);
  void         setFrequencyMeters(uint32_t ulFrequencyMeters) {
            m_fBandChanged = m_ulFrequencyMeters != ulFrequencyMeters;
            m_ulFrequencyMeters = ulFrequencyMeters;
  }
  void setFrequencyHz(uint64_t ullFrequency) {
    m_ullFrequency = ullFrequency;
  }
  bool NewBand(bool fReset = true) {
    bool fNewBand(m_fBandChanged);
    if (fReset
        && fNewBand) {
      m_fBandChanged = false;
    }
    return fNewBand;
  }
  int        getMetersMapIndex(void) const;
  static int getMetersMapIndex(uint32_t ulMeters);

  struct SRFPowerMap {
    struct SHardrockPowerMap {
      void Serialize(CEEPROMStream& rSrc, bool bLoad) {
        for (size_t stIndex(0);
             stIndex < sizeof m_uchMaxPwrAnt[0] / sizeof(uint8_t);
             stIndex++) {
          (bLoad) ? rSrc.get(m_uchMaxPwrAnt[0][stIndex]) : rSrc.put(m_uchMaxPwrAnt[0][stIndex]);
        }
        for (size_t stIndex(0);
             stIndex < sizeof m_uchMaxPwrAnt[1] / sizeof(uint8_t);
             stIndex++) {
          (bLoad) ? rSrc.get(m_uchMaxPwrAnt[1][stIndex]) : rSrc.put(m_uchMaxPwrAnt[1][stIndex]);
        }
      }
#define percentToHex(_value_) ((_value_ * 255) / 100)
      uint8_t m_uchMaxPwrAnt[2][11] = {
        { percentToHex(50), percentToHex(94), percentToHex(77), percentToHex(87),  // 6 10 12 15
          percentToHex(98), percentToHex(80), percentToHex(37), percentToHex(70),  // 17 20 30 40
          percentToHex(14), percentToHex(50), percentToHex(00) },                  // 60 80 160
        { percentToHex(10), percentToHex(18), percentToHex(15), percentToHex(17),
          percentToHex(19), percentToHex(16), percentToHex(18), percentToHex(14),
          percentToHex(14), percentToHex(10), percentToHex(00) }
      };
    };

    void Serialize(CEEPROMStream& rSrc, bool bLoad) {
      for (size_t stIndex(0);
           stIndex < sizeof m_RFPowerMap.m_uchQRPPwr / sizeof(uint8_t);
           stIndex++) {
        (bLoad) ? rSrc.get(m_uchQRPPwr[stIndex]) : rSrc.put(m_uchQRPPwr[stIndex]);
      }
      m_HRPwrMap[0].Serialize(rSrc, bLoad);
      m_HRPwrMap[1].Serialize(rSrc, bLoad);
    }

    uint8_t getMaxPower(eHardrock eWhichHardrock, eAntenna eWhichAntenna, uint32_t ulMeters) const {
      int nIndex(getMetersMapIndex(ulMeters));

      if (nIndex >= 0) {
        return (eWhichHardrock != eHardrock::QRP)
                 ? m_HRPwrMap[eWhichHardrock].m_uchMaxPwrAnt[eWhichAntenna][nIndex]
                 : getMaxPowerQRP(ulMeters);
      }
      return 255;
    }
    void setMaxPower(eHardrock eWhichHardrock, eAntenna eWhichAntenna, uint32_t ulMeters, uint8_t uchMaxPower) {
      int nIndex(getMetersMapIndex(ulMeters));

      if (nIndex >= 0) {
        if (eWhichHardrock != eHardrock::QRP) {
          m_HRPwrMap[eWhichHardrock].m_uchMaxPwrAnt[eWhichAntenna][nIndex] = uchMaxPower;
        } else {
          setMaxPowerQRP(ulMeters, uchMaxPower);
        }
      }
    }
    uint8_t getMaxPowerQRP(uint32_t ulMeters) const {
      int nIndex(getMetersMapIndex(ulMeters));

      if (nIndex >= 0) {
        return m_uchQRPPwr[nIndex];
      }
      return 255;
    }
    void setMaxPowerQRP(uint32_t ulMeters, uint8_t uchMaxPower) {
      int nIndex(getMetersMapIndex(ulMeters));

      if (nIndex >= 0) {
        m_uchQRPPwr[nIndex] = uchMaxPower;
      }
    }

    uint8_t           m_uchQRPPwr[11] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
    SHardrockPowerMap m_HRPwrMap[2];
  };

  struct SHardrockUSBMap {
    SHardrockUSBMap()
      : m_eBinding(Unbound), m_idVendor(0), m_idProduct(0), m_ulBaudrate(0) {}

    eBinding binding(void) const {
      return m_eBinding;  // Serial Port
    }
    uint16_t vendor(void) const {
      return m_idVendor;  // idVendor()
    }
    uint16_t product(void) const {
      return m_idProduct;  // idProduct()
    }
    String serialNumber(void) const {
      return m_sSerialNumber;  // serialNumber()
    }
    String modelName(void) const {
      return m_sModelName;
    }
    uint32_t getBaudrate(void) const {
      return m_ulBaudrate;
    }
    void setBaudrate(uint32_t ulBaudrate) {
      m_ulBaudrate = ulBaudrate;
    }
    void clear(void) {
      m_eBinding = Unbound;
      m_idVendor = 0;
      m_idProduct = 0;
      m_ulBaudrate = 0;
    }

    void updateBinding(eBinding binding) {
      m_eBinding = binding;
    }
    void bind(eBinding eBinding, uint16_t idProduct, uint16_t idVendor, const String& sSerialNumber, const String& sModelName) {
      m_eBinding = eBinding;
      m_idVendor = idVendor;
      m_idProduct = idProduct;
      m_sSerialNumber = sSerialNumber;
      m_sModelName = sModelName;
    }
    void Serialize(CEEPROMStream& rSrc, bool bLoad) {
      const size_t uMaxSerialNumber(8);
      const size_t uMaxModelName(14);

      if (bLoad) {
        rSrc.get(m_eBinding);
        rSrc.get(m_idVendor);
        rSrc.get(m_idProduct);
        rSrc.get(m_ulBaudrate);
        rSrc.get(m_sSerialNumber, uMaxSerialNumber);
        rSrc.get(m_sModelName, uMaxModelName);
      } else {
        rSrc.put(m_eBinding);
        rSrc.put(m_idVendor);
        rSrc.put(m_idProduct);
        rSrc.put(m_ulBaudrate);
        rSrc.put(m_sSerialNumber, uMaxSerialNumber);
        rSrc.put(m_sModelName, uMaxModelName);
      }
    }

  private:
    eBinding m_eBinding;       // Serial Port
    uint16_t m_idVendor;       // idVendor()
    uint16_t m_idProduct;      // idProduct()
    String   m_sSerialNumber;  // serialNumber()
    String   m_sModelName;     // Hardrock Model
    uint32_t m_ulBaudrate;     // Baudrate
  };

public:
  void bind(eBinding eBinding, uint16_t idProduct, uint16_t idVendor, const String& sSerialNumber, const String& sModelName) {
    bool fFound(false);
    for (size_t nIndex(0); (!fFound && nIndex < sizeof m_USBMap / sizeof(SHardrockUSBMap)); nIndex++) {
      fFound = (m_USBMap[nIndex].vendor() == idVendor
                && m_USBMap[nIndex].product() == idProduct
                && m_USBMap[nIndex].serialNumber() == sSerialNumber
                && m_USBMap[nIndex].modelName() == sModelName);
      if (fFound && m_USBMap[nIndex].binding() != eBinding) {
        m_USBMap[nIndex].updateBinding(eBinding);
        Serialize();
      }
    }
    if (!fFound) {
      for (size_t nIndex(0); nIndex < sizeof m_USBMap / sizeof(SHardrockUSBMap); nIndex++) {
        if (m_USBMap[nIndex].binding() == eBinding::Unbound) {
          m_USBMap[nIndex].bind(eBinding, idProduct, idVendor, sSerialNumber, sModelName);
          Serialize();
          break;
        }
      }
    }
  }
  eBinding getBinding(
    uint16_t idProduct, uint16_t idVendor, const String& sSerialNumber, const String& sModelName) {
    eBinding Binding(eBinding::Unbound);

    for (size_t nIndex(0); nIndex < sizeof m_USBMap / sizeof(SHardrockUSBMap); nIndex++) {
      if (m_USBMap[nIndex].vendor() == idVendor
          && m_USBMap[nIndex].product() == idProduct
          && m_USBMap[nIndex].serialNumber() == sSerialNumber
          && m_USBMap[nIndex].modelName() == sModelName) {
        Binding = m_USBMap[nIndex].binding();
        break;
      }
    }
    return Binding;
  }
  void setBaudrate(
    uint16_t idProduct, uint16_t idVendor, const String& sSerialNumber, uint32_t uBaudrate) {

    for (size_t nIndex(0); nIndex < sizeof m_USBMap / sizeof(SHardrockUSBMap); nIndex++) {
      if (m_USBMap[nIndex].vendor() == idVendor
          && m_USBMap[nIndex].product() == idProduct
          && m_USBMap[nIndex].serialNumber() == sSerialNumber) {
        m_USBMap[nIndex].setBaudrate(uBaudrate);
        Serialize();
        break;
      }
    }
  }
  uint32_t getBaudrate(
    uint16_t idProduct, uint16_t idVendor, const String& sSerialNumber, uint32_t ulDefaultBaudrate = 19200) const {
    uint32_t ulBaudrate(ulDefaultBaudrate);

    for (size_t nIndex(0); nIndex < sizeof m_USBMap / sizeof(SHardrockUSBMap); nIndex++) {
      if (m_USBMap[nIndex].vendor() == idVendor
          && m_USBMap[nIndex].product() == idProduct
          && m_USBMap[nIndex].serialNumber() == sSerialNumber) {
        ulBaudrate = m_USBMap[nIndex].getBaudrate();
        break;
      }
    }
    return (ulBaudrate) ? ulBaudrate : ulDefaultBaudrate;
  }
  void eraseBindings(void) {
    for (size_t nIndex(0); nIndex < sizeof m_USBMap / sizeof(SHardrockUSBMap); nIndex++) {
      m_USBMap[nIndex].clear();
    }
    Serialize();
  }
  bool PTTEnabled(eHardrock eWhichHardrock) const {
    int nIndex(getMetersMapIndex());

    if (nIndex >= 0) {
      return (eWhichHardrock == eHardrock::QRP
              || (m_aKeyingMode[eWhichHardrock]
                  && SendEnabled(eWhichHardrock)));
    }
    return false;
  }
  bool SendEnabled(eHardrock eWhichHardrock) const {
    int nIndex(getMetersMapIndex());

    if (nIndex >= 0) {
      return eWhichHardrock == eHardrock::QRP || digitalRead(m_aPTTEnableMap[eWhichHardrock][nIndex]) == LOW;
    }
    return false;
  }
  uint8_t getMaxPower(eHardrock eWhichHardrock, eAntenna eWhichAntenna, uint32_t ulMeters, bool fAbsolute = false) const {
    return ((fAbsolute || PTTEnabled(eWhichHardrock)) && eWhichHardrock != eHardrock::QRP)
             ? m_RFPowerMap.getMaxPower(eWhichHardrock, eWhichAntenna, ulMeters)
             : getMaxPowerQRP(ulMeters);
  }
  void setMaxPower(eHardrock eWhichHardrock, eAntenna eWhichAntenna, uint32_t ulMeters, uint8_t uchMaxPower) {
    (PTTEnabled(eWhichHardrock) && eWhichHardrock != eHardrock::QRP)
      ? m_RFPowerMap.setMaxPower(eWhichHardrock, eWhichAntenna, ulMeters, uchMaxPower)
      : setMaxPowerQRP(ulMeters, uchMaxPower);
    Serialize();
  }
  uint8_t getMaxPowerQRP(uint32_t ulMeters) const {
    return m_RFPowerMap.getMaxPowerQRP(ulMeters);
  }
  void setMaxPowerQRP(uint32_t ulMeters, uint8_t uchMaxPower) {
    m_RFPowerMap.setMaxPowerQRP(ulMeters, uchMaxPower);
    Serialize();
  }
  uint8_t getInitialPower(eHardrock eWhichHardrock, uint32_t ulMeters) const {
    if (ulMeters == 1) {
      return m_InitialPwr70CM;
    } else if (ulMeters == 2) {
      return m_InitialPwr2M;
    }
    return m_aInitialPwr[eWhichHardrock][getMetersMapIndex(ulMeters)];
  }
  uint8_t getInitialPwr(void) {
    uint8_t uchRFLevel(0);

    switch (getFrequencyMeters()) {
      case 1:
        uchRFLevel = m_InitialPwr70CM;
        break;
      case 2:
        uchRFLevel = m_InitialPwr2M;
        break;
      default:
        int       nIndex(getMetersMapIndex());
        eHardrock whichHardrock((HardrockAvailable(eHardrock::A)
                                 && SendEnabled(eHardrock::A))
                                  ? eHardrock::A
                                : (HardrockAvailable(eHardrock::B)
                                   && SendEnabled(eHardrock::B))
                                  ? eHardrock::B
                                  : eHardrock::QRP);
        if (whichHardrock != eHardrock::QRP && !PTTEnabled(whichHardrock)) {
          whichHardrock = eHardrock::QRP;
        }
        if (nIndex >= 0) {
          uchRFLevel = m_aInitialPwr[whichHardrock][nIndex];
        }
        break;
    }
    return uchRFLevel;
  }
  void setInitialPwr(uint8_t uchLevel) {
    switch (getFrequencyMeters()) {
      case 1:
        m_InitialPwr70CM = uchLevel;
        break;
      case 2:
        m_InitialPwr2M = uchLevel;
        break;
      default:
        int       nIndex(getMetersMapIndex());
        eHardrock whichHardrock((HardrockAvailable(eHardrock::A)
                                 && PTTEnabled(eHardrock::A))
                                  ? eHardrock::A
                                : (HardrockAvailable(eHardrock::B)
                                   && PTTEnabled(eHardrock::B))
                                  ? eHardrock::B
                                  : eHardrock::QRP);
        if (whichHardrock != eHardrock::QRP && !PTTEnabled(whichHardrock)) {
          whichHardrock = eHardrock::QRP;
        }
        if (nIndex >= 0) {
          m_aInitialPwr[whichHardrock][nIndex] = uchLevel;
        }
        break;
    }
    Serialize();
  }

public:
  void enableWatchdog(Watchdog::Timeout eTimeout = Watchdog::Timeout(12 * Watchdog::TIMEOUT_1S)) {
    m_Watchdog.enable(eTimeout);
    m_eTimeout = eTimeout;
  }
  void resetWatchdog(void) {
    m_Watchdog.reset();
  }
  bool WatchdogTripped(void) {
    return m_Watchdog.tripped();
  }
  uint32_t WatchdogTimeout(void) const {
    return (m_eTimeout * Watchdog::Timeout::TIMEOUT_1S) / 1000;
  }

protected:
  void update(void) {
    for (size_t nIndex(0); nIndex < sizeof m_bHardrockMap / sizeof(Bounce); nIndex++) {
      m_bHardrockMap[nIndex].update();
    }
    for (size_t nIndex(0); nIndex < sizeof m_bStateMap / sizeof(Bounce); nIndex++) {
      m_bStateMap[nIndex].update();
    }
  }

private:
  uint percentPower(uint8_t uchPwr) {
    return lround((100.0 / 255.0) * float(uchPwr));
  }
  static void    pair_IC_705(void);
  static uint8_t ReadRFPower(void);

  static void onGetVersion(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onGetVersion(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onDebug(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onDebug(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onPair(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onPair(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onOriginal(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onOriginal(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onReboot(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onReboot(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onClearMap(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onClearMap(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onSetMaxPower(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onSetMaxPower(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onDisconnect(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onDisconnect(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onATCmd(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onATCmd(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onPTTSwitchSettings(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onPTTSwitchSettings(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onSetInitialPwr(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onSetInitialPwr(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onResetInitialPwr(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onResetInitialPwr(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onHelp(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onHelp(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onFlash(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onPrintPwrMaps(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onPrintPwrMaps(const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onPrintStatus(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  static void onHardrockAvailable(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice);
  void        onHardrockAvailable(const String& rsCmd, CSerialDevice& rSrcDevice);

protected:
  const uint16_t m_uDebounceInterval;
  Bounce         m_Tune;
  Bounce         m_bStateMap[1];
  Bounce         m_bHardrockMap[2];
  const unsigned m_uPowerMap[1]{ BT_ICOM_PWR };
  const unsigned m_uKeyMap[1]{ BT_ICOM_KEY };
  uint32_t       m_ulFrequencyMeters;
  uint64_t       m_ullFrequency;
  uint8_t        m_aInitialPwr[3][11] = {
           { percentToHex(10), percentToHex(18), percentToHex(15), percentToHex(17),  // 6 10 12 15
             percentToHex(19), percentToHex(16), percentToHex(18), percentToHex(14),  // 17 20 30 40
             percentToHex(14), percentToHex(10), percentToHex(00) },                  // 60 80 160
           { percentToHex(10), percentToHex(18), percentToHex(15), percentToHex(17),  // 6 10 12 15
             percentToHex(19), percentToHex(16), percentToHex(18), percentToHex(14),  // 17 20 30 40
             percentToHex(14), percentToHex(10), percentToHex(00) },                  // 60 80 160
           { 255, 255, 255, 255,                                                      // 6 10 12 15
             255, 255, 255, 255,                                                      // 17 20 30 40
             255, 255, 0 }                                                            // 60 80 160
  };
#undef percentToHex
  uint8_t           m_InitialPwr2M;
  uint8_t           m_InitialPwr70CM;
  bool              m_fDebugEnable;
  bool              m_fTunerEnabled;
  bool              m_isTuning;
  bool              m_fBandChanged;
  CCommandProcessor m_CmdHandler;
  volatile eAntenna m_aCurrentAntenna[2] = { eAntenna::Antenna1, eAntenna::Antenna1 };
  bool              m_fTuningEnabled[2][2] = { { true, true }, { true, true } };
  bool              m_aKeyingMode[2] = { true, true };
  SRFPowerMap       m_RFPowerMap;
  SHardrockUSBMap   m_USBMap[4];
  const int         m_aPTTEnableMap[2][11] = {
            { HR_A_6M_PTT_Enable, HR_A_10M_PTT_Enable, HR_A_12M_PTT_Enable,
              HR_A_15M_PTT_Enable, HR_A_17M_PTT_Enable, HR_A_20M_PTT_Enable,
              HR_A_30M_PTT_Enable, HR_A_40M_PTT_Enable, HR_A_60M_PTT_Enable,
              HR_A_80M_PTT_Enable, HR_A_160M_PTT_Enable },
            { HR_B_6M_PTT_Enable, HR_B_10M_PTT_Enable, HR_B_12M_PTT_Enable,
              HR_B_15M_PTT_Enable, HR_B_17M_PTT_Enable, HR_B_20M_PTT_Enable,
              HR_B_30M_PTT_Enable, HR_B_40M_PTT_Enable, HR_B_60M_PTT_Enable,
              HR_B_80M_PTT_Enable, HR_B_160M_PTT_Enable }
  };
  Watchdog          m_Watchdog;
  Watchdog::Timeout m_eTimeout;
};
#endif
