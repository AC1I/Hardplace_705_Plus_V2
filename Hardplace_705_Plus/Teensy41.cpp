#include <stddef.h>
#include <stdint.h>
#include "Print.h"
#include "core_pins.h"
#include "Teensy41.h"
#include "ICOM.h"
#include "IC_705Master.h"

// https://github.com/FrankBoesing/T4_PowerButton
void CTeensy::reboot(void) const {
  SCB_AIRCR = 0x05FA0004;
  while (1)
    ;
}

void CTeensy::onNewPacket(const uint8_t* puPacket, size_t stPacket, CSerialDevice& rSrcDevice) {
  CIC_705MasterDevice& rIC_705(static_cast<CIC_705MasterDevice&>(rSrcDevice));
  CICOMResp            Resp(puPacket, stPacket);
  if (Resp.isFrequencyResponse()) {
    uint32_t uPrevBand(getFrequencyMeters());

    setFrequencyHz(Resp.FrequencyHz());
    setFrequencyMeters(Resp.FrequencyMeters());

    if (uPrevBand != getFrequencyMeters()) {
      if (getMetersMapIndex(getFrequencyMeters()) >= 0) {
        digitalWrite(PTT_A_Enable, PTTEnabled(eHardrock::A));
        digitalWrite(PTT_B_Enable, PTTEnabled(eHardrock::B));
        rIC_705.ReadRFPower();
      } else {
        digitalWrite(PTT_A_Enable, LOW);
        digitalWrite(PTT_B_Enable, LOW);
      }
    }
  } else if (Resp.isOperatingModeResponse()) {

  } else if (Resp.ResponseType() == 0x14  // RF Power
             && Resp >= 6
             && Resp[5] == 0x0A) {
    uint8_t uchMaxPower(
      (HardrockAvailable(eHardrock::A) && PTTEnabled(eHardrock::A))
        ? getMaxPower(eHardrock::A, m_aCurrentAntenna[eHardrock::A], getFrequencyMeters())
      : (HardrockAvailable(eHardrock::B) && PTTEnabled(eHardrock::B))
        ? getMaxPower(eHardrock::B, m_aCurrentAntenna[eHardrock::B], getFrequencyMeters())
        : getMaxPowerQRP(getFrequencyMeters()));
    if (NewBand(false)) {
      if (rIC_705.WriteRFPower((getInitialPwr() <= uchMaxPower) ? getInitialPwr() : uchMaxPower)) {
        NewBand();
      }
    } else if (Resp.RFPower() > uchMaxPower
               || getInitialPwr() > uchMaxPower) {
      rIC_705.WriteRFPower(uchMaxPower);
    }
  }
}
void CTeensy::onNewPacket(const String& rsPacket, CSerialDevice& rSrcDevice) {
  String sCmd(rsPacket);

  sCmd.toUpperCase();
  if (sCmd.length() >= 4
      && sCmd.substring(0, 2) == "HP") {
    CommandHandler Handler(
      m_CmdHandler.getValueOf(const_cast<char*>(sCmd.substring(0, 4).c_str())));
    if (Handler) {
      Handler(this, rsPacket, rSrcDevice);
    } else {
      rSrcDevice.println(sCmd.substring(0, 4) + " Command not found ");
      CommandHandler HelpHandler(m_CmdHandler.getValueOf("HPHE"));
      if (HelpHandler) {
        HelpHandler(this, rsPacket, rSrcDevice);
      }
    }
  }
}

int CTeensy::getMetersMapIndex(void) const {
  return getMetersMapIndex(getFrequencyMeters());
}

int CTeensy::getMetersMapIndex(uint32_t ulMeters) {
  int iIndex(-1);

  switch (ulMeters) {
    case 6:
      iIndex = 0;
      break;

    case 10:
      iIndex = 1;
      break;

    case 12:
      iIndex = 2;
      break;

    case 15:
      iIndex = 3;
      break;

    case 17:
      iIndex = 4;
      break;

    case 20:
      iIndex = 5;
      break;

    case 30:
      iIndex = 6;
      break;

    case 40:
      iIndex = 7;
      break;

    case 60:
      iIndex = 8;
      break;

    case 80:
      iIndex = 9;
      break;

    case 160:
      iIndex = 10;
      break;

    default:
      break;
  }
  return iIndex;
}

// Command support
extern CIC_705MasterDevice&
     IC705(void);
void CTeensy::pair_IC_705(void) {
  IC705().clearPairing();
}
uint8_t CTeensy::ReadRFPower(void) {
  uint8_t        uchPower(0);
  uint8_t        auchRespBuf[32];
  CICOMReq       Icom(IC705().getRigAddress());  // We do it this way to avoid the try lock in CIC_705MasterDevice members
  Threads::Scope wait(IC705());

  Icom.ReadRFPower(IC705());
  size_t cRead(Icom.getResponse(IC705(), auchRespBuf, sizeof auchRespBuf));

  if (cRead > 0) {
    CICOMResp readRFPowerRsp(auchRespBuf, cRead);
    uchPower = readRFPowerRsp.RFPower();
  }
  return uchPower;
}

// Commands
void CTeensy::onGetVersion(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onGetVersion(rsCmd, rSrcDevice);
}
void CTeensy::onGetVersion(const String& rsCmd, CSerialDevice& rSrcDevice) {
  rSrcDevice.println("Hardplace 705+ Version 2");
}
void CTeensy::onDebug(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onDebug(rsCmd, rSrcDevice);
}
void CTeensy::onDebug(const String& rsCmd, CSerialDevice& rSrcDevice) {
  DebugEnable(rsCmd.charAt(rsCmd.length() - 2) != '0');
  rSrcDevice.println("OK");
}
void CTeensy::onPair(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onPair(rsCmd, rSrcDevice);
}
void CTeensy::onPair(const String& rsCmd, CSerialDevice& rSrcDevice) {
  pair_IC_705();
  onReboot(rsCmd, rSrcDevice);
}
void CTeensy::onOriginal(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onOriginal(rsCmd, rSrcDevice);
}
void CTeensy::onOriginal(const String& rsCmd, CSerialDevice& rSrcDevice) {
  CEEPROMInitialize ClearEEPROM;
  onReboot(rsCmd, rSrcDevice);
}
void CTeensy::onReboot(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onReboot(rsCmd, rSrcDevice);
}
void CTeensy::onReboot(const String& rsCmd, CSerialDevice& rSrcDevice) {
  rSrcDevice.print("OK (rebooting)");
  for (int iter(0); iter < 10; iter++) {
    Delay(500);
    rSrcDevice.print(" .");
    m_Watchdog.reset();
  }
  rSrcDevice.println("");
  reboot();
}
void CTeensy::onClearMap(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onClearMap(rsCmd, rSrcDevice);
}
void CTeensy::onClearMap(const String& rsCmd, CSerialDevice& rSrcDevice) {
  eraseBindings();
  rSrcDevice.println("OK");
}
void CTeensy::onSetMaxPower(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onSetMaxPower(rsCmd, rSrcDevice);
}
void CTeensy::onSetMaxPower(const String& rsCmd, CSerialDevice& rSrcDevice) {
  uint8_t uchRFPower(ReadRFPower());

  rSrcDevice.clear();
  rSrcDevice.println("Set the new maximum power, then send any character to continue");
  while (!rSrcDevice.available()) {
    Delay(0);
  }
  rSrcDevice.clear();

  uint8_t uchMaxRFPower(ReadRFPower());

  if (uchMaxRFPower != 0
      || uchRFPower == 0) {
    if (HardrockAvailable(eHardrock::A)
        && PTTEnabled(eHardrock::A)) {
      setMaxPower(eHardrock::A, getCurrentAntenna(eHardrock::A), getFrequencyMeters(), uchMaxRFPower);
    } else if (HardrockAvailable(eHardrock::B)
               && PTTEnabled(eHardrock::B)) {
      setMaxPower(eHardrock::B, getCurrentAntenna(eHardrock::B), getFrequencyMeters(), uchMaxRFPower);
    } else {
      setMaxPowerQRP(getFrequencyMeters(), uchMaxRFPower);
    }
    rSrcDevice.println("OK");
  } else {
    rSrcDevice.println("FAIL");
  }
}
void CTeensy::onDisconnect(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onDisconnect(rsCmd, rSrcDevice);
}
void CTeensy::onDisconnect(const String& rsCmd, CSerialDevice& rSrcDevice) {
  IC705().DisconnectBoundDevice();
  rSrcDevice.println("OK");
}
void CTeensy::onATCmd(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onATCmd(rsCmd, rSrcDevice);
}
void CTeensy::onATCmd(const String& rsCmd, CSerialDevice& rSrcDevice) {
  String sCmd(rsCmd);
  sCmd.toUpperCase();
  String sATCmd(sCmd.substring(sCmd.lastIndexOf("AT+")));
  String Response;

  sATCmd.remove(sATCmd.indexOf(';'));
  IC705().SendATCmd(sATCmd, Response);
  rSrcDevice.print(Response);
}
void CTeensy::onPTTSwitchSettings(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onPTTSwitchSettings(rsCmd, rSrcDevice);
}
void CTeensy::onPTTSwitchSettings(const String& rsCmd, CSerialDevice& rSrcDevice) {
  int aiMeters[] = { 6, 10, 12, 15, 17, 20, 30, 40, 60, 80, 160 };

  for (size_t nIndex(0); nIndex < sizeof aiMeters / sizeof(int); nIndex++) {
    rSrcDevice.printf(
      "%3d Meters: Hardrock A %8s Hardrock B %s\r\n", aiMeters[nIndex],
      (digitalRead(m_aPTTEnableMap[A][getMetersMapIndex(aiMeters[nIndex])]) == LOW) ? "Enabled" : "Disabled",
      (digitalRead(m_aPTTEnableMap[B][getMetersMapIndex(aiMeters[nIndex])]) == LOW) ? "Enabled" : "Disabled");
  }
}
void CTeensy::onSetInitialPwr(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onSetInitialPwr(rsCmd, rSrcDevice);
}
void CTeensy::onSetInitialPwr(const String& rsCmd, CSerialDevice& rSrcDevice) {
  uint8_t uchPwr(ReadRFPower());

  if (uchPwr) {
    setInitialPwr(uchPwr);
    rSrcDevice.println("OK");
  } else {
    rSrcDevice.println("FAIL");
  }
}
void CTeensy::onResetInitialPwr(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onResetInitialPwr(rsCmd, rSrcDevice);
}
void CTeensy::onResetInitialPwr(const String& rsCmd, CSerialDevice& rSrcDevice) {
  if (IC705().WriteRFPower(getInitialPwr())) {
    rSrcDevice.println("OK");
  } else {
    rSrcDevice.println("FAIL");
  }
}
void CTeensy::onHelp(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onHelp(rsCmd, rSrcDevice);
}
void CTeensy::onHelp(const String& rsCmd, CSerialDevice& rSrcDevice) {
  rSrcDevice.println("HPMP - Set Max power setting for current band/antenna/amplifier"), Delay(10);
  rSrcDevice.println("HPIP - Set Initial Power for current band/antenna/amplifier"), Delay(10);
  rSrcDevice.println("HPRP - Reset power to default for current band/antenna/amplifier"), Delay(10);
  rSrcDevice.println("HPDP - Reset power to default for current band/antenna/amplifier"), Delay(10);
  rSrcDevice.println("HPVE - Get Hardplace Version"), Delay(10);
  rSrcDevice.println("HPHA - Report Hardrock availability"), Delay(10);
  rSrcDevice.println("HPDE - Debug disable \"HPDE0;\" enable \"HPDE1;\""), Delay(10);
  rSrcDevice.println("HPCP - Delete pairings"), Delay(10);
  rSrcDevice.println("HPOR - Erase all settings"), Delay(10);
  rSrcDevice.println("HPRE - Reboot"), Delay(10);
  rSrcDevice.println("HPCM - Clear USB to Hardrock assignments"), Delay(10);
  rSrcDevice.println("HPDI - Disconnect from the IC-705"), Delay(10);
  rSrcDevice.println("HPAT - Issue AT command to HC-05 \"HPAT+Version?\" or \"HPATAT+Version?\""), Delay(10);
  rSrcDevice.println("HPPT - Display PTT enable/disable settings"), Delay(10);
  rSrcDevice.println("HPPM - Print power maps"), Delay(10);
  rSrcDevice.println("HPPS - Print device status"), Delay(10);
  rSrcDevice.println("HPHE - Help");
}
void CTeensy::onFlash(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  rSrcDevice.clear();
  rSrcDevice.println(
    "Warning!!! This command will erase all memory and the device will enter bootloader mode");
  rSrcDevice.println("Send \"Y\" to continue, any other character to abort");
  while (!rSrcDevice.available()) {
    Delay(0);
  }
  if (toupper(rSrcDevice.read()) == 'Y') {
    IC705().DisconnectBoundDevice();
    if (!(HW_OCOTP_CFG5 & 0x02)) {
      asm("bkpt #251");  // run bootloader
    } else {
      __disable_irq();  // secure mode NXP ROM reboot
      USB1_USBCMD = 0;
      IOMUXC_GPR_GPR16 = 0x00200003;
      // TODO: wipe all RAM for security
      __asm__ volatile("mov sp, %0"
                       :
                       : "r"(0x20201000)
                       :);
      __asm__ volatile("dsb" ::
                         : "memory");
      volatile uint32_t* const p = (uint32_t*)0x20208000;
      *p = 0xEB120000;
      ((void (*)(volatile void*))(*(uint32_t*)(*(uint32_t*)0x0020001C + 8)))(p);
    }
    __builtin_unreachable();
  } else {
    rSrcDevice.println("Bootloader Aborted");
  }
}
void CTeensy::onPrintPwrMaps(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onPrintPwrMaps(rsCmd, rSrcDevice);
}
void CTeensy::onPrintPwrMaps(const String& rsCmd, CSerialDevice& rSrcDevice) {
  uint32_t aulMeters[] = { 6, 10, 12, 15, 17, 20, 30, 40, 60, 80, 160 };

  rSrcDevice.printf("70CM: Initial %u%% Maximum %u%%\r\n",
                    percentPower(m_InitialPwr70CM), percentPower(getInitialPower(eHardrock::QRP, 1)));
  rSrcDevice.printf("2M:   Initial %u%% Maximum %u%%\r\n\r\n",
                    percentPower(m_InitialPwr2M), percentPower(getInitialPower(eHardrock::QRP, 2)));

  for (size_t nIndex(0); nIndex < sizeof aulMeters / sizeof(uint32_t); nIndex++, Delay(10)) {
    rSrcDevice.printf(
      "Hardrock A %3luM: Initial %3u%% Maximum Ant 1/2 %3u/%3u%% QRP Initial %3u%% Max %u%%\r\n",
      aulMeters[nIndex],
      percentPower(getInitialPower(eHardrock::A, aulMeters[nIndex])),
      percentPower(getMaxPower(eHardrock::A, eAntenna::Antenna1, aulMeters[nIndex], true)),
      percentPower(getMaxPower(eHardrock::A, eAntenna::Antenna2, aulMeters[nIndex], true)),
      percentPower(getInitialPower(eHardrock::QRP, aulMeters[nIndex])),
      percentPower(getMaxPowerQRP(aulMeters[nIndex])));
    rSrcDevice.flush();
  }
  rSrcDevice.printf("\r\n");
  for (size_t nIndex(0); nIndex < sizeof aulMeters / sizeof(uint32_t); nIndex++, Delay(10)) {
    rSrcDevice.printf(
      "Hardrock B %3luM: Initial %3u%% Maximum Ant 1/2 %3u/%3u%% QRP Initial %3u%% Max %u%%\r\n",
      aulMeters[nIndex],
      percentPower(getInitialPower(eHardrock::B, aulMeters[nIndex])),
      percentPower(getMaxPower(eHardrock::B, eAntenna::Antenna1, aulMeters[nIndex], true)),
      percentPower(getMaxPower(eHardrock::B, eAntenna::Antenna2, aulMeters[nIndex], true)),
      percentPower(getInitialPower(eHardrock::QRP, aulMeters[nIndex])),
      percentPower(getMaxPowerQRP(aulMeters[nIndex])));
    rSrcDevice.flush();
  }
}
void CTeensy::onPrintStatus(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  extern void printStatus(CSerialDevice & rPrintDevice);
  printStatus(rSrcDevice);
}
void CTeensy::onHardrockAvailable(void* pthis, const String& rsCmd, CSerialDevice& rSrcDevice) {
  reinterpret_cast<CTeensy*>(pthis)->onHardrockAvailable(rsCmd, rSrcDevice);
}
void CTeensy::onHardrockAvailable(const String& rsCmd, CSerialDevice& rSrcDevice) {
  if (HardrockAvailable(eHardrock::A)) {
    rSrcDevice.print("A");
  }
  if (HardrockAvailable(eHardrock::B)) {
    rSrcDevice.print("B");
  }
  if (HardrockAvailable(eHardrock::A) || HardrockAvailable(eHardrock::B)) {
    rSrcDevice.println("");
  } else {
    rSrcDevice.println("OK");
  }
}
