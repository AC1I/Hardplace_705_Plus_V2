#include <elapsedMillis.h>
#include <InternalTemperature.h>

#include "Hardplace705Plus.h"
#include "Tracer.h"
#include "Teensy41.h"
#include "IC_705Master.h"
#include "IC705Tuner.h"
#include "Hardrock.h"
#include "HardrockBluetooth.h"
#include "HardrockUSB.h"
#include "HardrockPair.h"
#include "SerialProcessor.h"

extern "C" uint32_t set_arm_clock(uint32_t frequency);

namespace {

void HardplaceTask(void);
void ManageBindings(void);
void ReadRFPower(bool fNow = false);
bool BindByMap(CHardrockPair& rHardrock);
void BindByPort(CHardrockPair& rHardrock);
void HighAlarmISR(void);
void LowAlarmISR(void);

/*
   Serial1 -> Hardrock A
   Serial2 -> Hardrock B
   Serial3 -> Hardrock Bluetooth B
   Serial4 -> Hardrock Bluetooth A
   Serial5 -> Unused
   Serial6 -> Unused
   Serial7 -> Unused
   Serial8 -> Icom Bluetooth

   The Teensy 4.1 has a logic controlled power switch on the USB Host 5V power pin
   so the connector will not have 5V power until the USBHost begin() function is called.
*/

// CEEPROMInitialize          ClearEEPROM;           // For initializing EEPROM before serialization occurs
const float                   fHighTempAlarmC(80);   // Keep junction temperature below 95C for longest life
CTraceDevice                  Tracer(TRACE_GLOBAL);  // Global for tracing constuctors of global/objects
CTeensy                       Teensy;
CIC_705MasterDevice           IC_705(IBluetooth::TeensyBluetooth::IC_705, Teensy, 0xE1, Serial8, 115200);
CHardrockBluetoothSlaveDevice BluetoothA(Serial4, "Hardplace A", 115200);
CHardrockBluetoothSlaveDevice BluetoothB(Serial3, "Hardplace B", 115200);
CHardrockPair                 HardrockA(CTeensy::eHardrock::A, Serial1, Teensy, IC_705, 0xE2);
CHardrockPair                 HardrockB(CTeensy::eHardrock::B, Serial2, Teensy, IC_705, 0xE3);
CHardrockUSB                  HardrockUSB1(SerialUSBHost1);
CHardrockUSB                  HardrockUSB2(SerialUSBHost2);
CHardrockUSB                  HardrockUSB3(SerialUSBHost3);
CHardrockUSB                  HardrockUSB4(SerialUSBHost4);
CSerialProcessor              CmdProcessor(Serial, 115200);
#if defined                   DUAL_SERIAL
CSerialProcessor              CmdProcessorB(SerialUSB1, 115200);
#endif
}

void setup() {
  if (!Tracer.Enabled()) {
    if (Teensy.DebugEnabled()) {
      Tracer.Enable();
      Tracer.begin(CmdProcessor.getBaudrate());
      Tracer.setTimeout(1);
      while (!Tracer && millis() < 10000)
        ;  // wait for Arduino Serial Monitor
    }
  }
  if (Tracer.Enabled()) {
    if (CrashReport) {            // CrashReport.breadcrumb((1-6), uint32_t identifier)
      Tracer.TraceLn("");         // CrashReport.breadcrumb( 1, 0x5000000 | __LINE__ );
      Tracer.print(CrashReport);  // Upper bits hold '5' perhaps indicating func() for ref, lower bits show line #
    }
    if (Teensy.WatchdogTripped()) {
      Tracer.TraceLn("Reboot due to Watchdog timeout");
    }
  }

  Teensy.setup();
  IC_705.setup();
  HardrockA.setup();
  HardrockB.setup();
  BluetoothA.setup();  // Note: if you reboot with the Bluetooth connected discoverBaudrate will fail
  BluetoothB.setup();  // due to the inability to enter command mode and if will take a long time to complete the setup
  HardrockUSB1.setup();
  HardrockUSB2.setup();
  HardrockUSB3.setup();
  HardrockUSB4.setup();
  CmdProcessor.setup();
#if defined DUAL_SERIAL
  CmdProcessorB.setup();
#endif

  while (!IC_705.Paired()) {
    if (Serial) {
      Serial.println("On the IC-705 Bluetooth menu, select, <<Pairing Reception>>");
      Serial.println("When the IC-705 prompts for a password, enter 0000");
    }
    IC_705.Pair();
  }

  CmdProcessor.bind(Teensy);     // Command Processor
  CmdProcessor.bind(IC_705);     // IC-705 passthru
  CmdProcessor.bind(HardrockA);  // Hardrock-A passthru
#if defined DUAL_SERIAL
  CmdProcessorB.bind(Teensy);     // Command Processor
  CmdProcessorB.bind(IC_705);     // IC-705 passthru
  CmdProcessorB.bind(HardrockB);  // Hardrock-B passthru
#endif
  BluetoothA.bind(Teensy);  // Command Processor
  BluetoothA.bind(IC_705);  // IC-705 passthru

  BluetoothB.bind(Teensy);  // Command Processor
  BluetoothB.bind(IC_705);  // IC-705 passthru

  IC_705.bindDevice(Teensy, IC_705.getRigAddress());  // Route IC-705 Broadcasts to Teensy

  InternalTemperature.attachHighTempInterruptCelsius(fHighTempAlarmC, &HighAlarmISR);
  Teensy.enableWatchdog();
  threads.setDefaultTimeSlice(10);
}

void        loop() {
#if defined DO_PING
  elapsedMillis loopTime(0);
#endif

  Teensy.Task();
  IC_705.Task();
  HardrockA.Task();
  HardrockB.Task();
  BluetoothA.Task();
  BluetoothB.Task();
  CmdProcessor.Task();
#if defined DUAL_SERIAL
  CmdProcessorB.Task();
#endif
  HardplaceTask();

#if defined DO_PING
#define PING_INTERVAL 10000
  static elapsedMillis _ping(PING_INTERVAL);
  static unsigned      uMaxLoopTime(0);

  if (loopTime > uMaxLoopTime) {
    uMaxLoopTime = loopTime;
  }
  if (_ping >= PING_INTERVAL) {
    if (Tracer.Enabled()) {
      const char* pszConnected("Connected");
      const char* pszDisconnected("Disconnected");
      const char* pszEnabled("Enabled");
      const char* pszDisabled("Disabled");

      Tracer.TraceLn(
        "Hardplace 705+ IC-705 " + String((IC_705.isConnected()) ? pszConnected : pszDisconnected)
        + " - Hardrock-A " + String((Teensy.HardrockAvailable(CTeensy::eHardrock::A)) ? pszConnected : pszDisconnected)
        + " - Hardrock-B " + String((Teensy.HardrockAvailable(CTeensy::eHardrock::B)) ? pszConnected : pszDisconnected)
        + " - Hub " + String((SerialUSBHostHub) ? pszConnected : pszDisconnected)
        + " - HardrockUSB1 " + String((HardrockUSB1) ? pszConnected : pszDisconnected)
        + " - HardrockUSB2 " + String((HardrockUSB2) ? pszConnected : pszDisconnected)
        + " - HardrockUSB3 " + String((HardrockUSB3) ? pszConnected : pszDisconnected)
        + " - HardrockUSB4 " + String((HardrockUSB4) ? pszConnected : pszDisconnected)
        + " - PTT-A " + String((Teensy.PTTEnabled(CTeensy::eHardrock::A)) ? pszEnabled : pszDisabled)
        + " - PTT-B " + String((Teensy.PTTEnabled(CTeensy::eHardrock::B)) ? pszEnabled : pszDisabled)
        + " - Tuner " + String((Teensy.TunerEnabled()) ? pszEnabled : pszDisabled)
        + " - CPU Temperature " + String(InternalTemperature.readTemperatureC(), 1) + "C"
        + " - Loop time " + String(loopTime) + "/" + String(uMaxLoopTime) + " milliseconds");
    }
    _ping = 0;
  }
#endif
#if defined USE_THREADS
  threads.yield();
#endif
}

void Delay(uint32_t millis) {
  if (millis == 0) {
    Teensy.resetWatchdog();
  }

  for (elapsedMillis eDelay(0); eDelay < millis; CHardplaceUSBHost::Task()) {
#if defined USE_THREADS
    threads.delay(1);
#else
    delay(1);
#endif
  }
}

CIC_705MasterDevice& IC705(void) {  // accessor for command processor
  return IC_705;
}

void printStatus(CSerialDevice& rPrintDevice) {
  const char* pszConnected("Connected");
  const char* pszDisconnected("Disconnected");
  const char* pszEnabled("Enabled");
  const char* pszDisabled("Disabled");

  rPrintDevice.println("IC-705          " + String((IC_705.isConnected()) ? pszConnected : pszDisconnected));
  rPrintDevice.println("Hardrock-A      " + String((Teensy.HardrockAvailable(CTeensy::eHardrock::A)) ? pszConnected : pszDisconnected));
  rPrintDevice.println("Hardrock-B      " + String((Teensy.HardrockAvailable(CTeensy::eHardrock::B)) ? pszConnected : pszDisconnected));
  rPrintDevice.println("Hub             " + String((SerialUSBHostHub) ? pszConnected : pszDisconnected));
  rPrintDevice.println("HardrockUSB1    " + String((HardrockUSB1) ? pszConnected : pszDisconnected));
  rPrintDevice.println("HardrockUSB2    " + String((HardrockUSB2) ? pszConnected : pszDisconnected));
  rPrintDevice.println("HardrockUSB3    " + String((HardrockUSB3) ? pszConnected : pszDisconnected));
  rPrintDevice.println("HardrockUSB4    " + String((HardrockUSB4) ? pszConnected : pszDisconnected));
  rPrintDevice.println("PTT-A           " + String((Teensy.PTTEnabled(CTeensy::eHardrock::A)) ? pszEnabled : pszDisabled));
  rPrintDevice.println("PTT-B           " + String((Teensy.PTTEnabled(CTeensy::eHardrock::B)) ? pszEnabled : pszDisabled));
  rPrintDevice.println("Tuner           " + String((Teensy.TunerEnabled()) ? pszEnabled : pszDisabled));
  rPrintDevice.println("CPU Temperature " + String(InternalTemperature.readTemperatureC(), 1) + "C");
}

namespace {

void HardplaceTask(void) {
  bool                     isConnected(IC_705.isConnected());
  static bool              wasConnected(false);
  static bool              wasHardrockConnected(false);
  static CTeensy::eAntenna AntennaA(CTeensy::eAntenna::Antenna1);
  static CTeensy::eAntenna AntennaB(CTeensy::eAntenna::Antenna1);
  CTeensy::eAntenna        currentAntennaA(Teensy.getCurrentAntenna(CTeensy::eHardrock::A));
  CTeensy::eAntenna        currentAntennaB(Teensy.getCurrentAntenna(CTeensy::eHardrock::B));
  static elapsedMillis     eLastInit(0);

  if (isConnected) {
    if (!wasConnected) {
      Teensy.onConnect();
      if (!IC_705.getCI_V_Transcieve()) {
        IC_705.setCI_V_Transcieve(true);
      }
      eLastInit = 0;
    } else if (eLastInit > 250) {
      if (!Teensy.getFrequencyMeters()) {
        IC_705.ReadOperatingFreq();
        eLastInit = 0;
      } else if (!Teensy.getInitialPwr()) {
        IC_705.ReadRFPower(true);
        eLastInit = 0;
      }
    }
    if (currentAntennaA != AntennaA
        || currentAntennaB != AntennaB) {
      AntennaA = currentAntennaA;
      AntennaB = currentAntennaB;
      if (Teensy.getInitialPwr()) {
        IC_705.ReadRFPower(true);
      }
    }
    ReadRFPower();
  } else if (wasConnected) {
    Teensy.onDisconnect();
  }
  wasConnected = isConnected;

  ManageBindings();

  Teensy.TunerEnable(
    (HardrockA && HardrockA.isATUPresent() && Teensy.SendEnabled(CTeensy::eHardrock::A))
    || (HardrockB && HardrockB.isATUPresent() && Teensy.SendEnabled(CTeensy::eHardrock::B)));

  if (!wasHardrockConnected
      && ((Teensy.HardrockAvailable(CTeensy::eHardrock::A) && Teensy.PTTEnabled(CTeensy::eHardrock::A))
          || (Teensy.HardrockAvailable(CTeensy::eHardrock::B) && Teensy.PTTEnabled(CTeensy::eHardrock::B)))) {
    wasHardrockConnected = true;
    if (isConnected) {
      if (Teensy.getInitialPwr()) {
        IC_705.WriteRFPower(Teensy.getInitialPwr(), true);
      }
      IC_705.ReadRFPower(true);
    }
  } else if (wasHardrockConnected
             && (!Teensy.HardrockAvailable(CTeensy::eHardrock::A) || !Teensy.PTTEnabled(CTeensy::eHardrock::A))
             && (!Teensy.HardrockAvailable(CTeensy::eHardrock::B) || !Teensy.PTTEnabled(CTeensy::eHardrock::B))) {
    wasHardrockConnected = false;
    if (isConnected) {
      if (Teensy.getInitialPwr()) {
        IC_705.WriteRFPower(Teensy.getInitialPwr(), true);
      }
      IC_705.ReadRFPower(true);
    }
  }
}

void ReadRFPower(bool fNow) {
  static elapsedMillis eLastRFPowerQuery(1000);

  if (fNow
      || eLastRFPowerQuery >= 1000) {
    eLastRFPowerQuery = 0;
    IC_705.ReadRFPower();
  }
}

void ManageBindings(void) {
  if (HardrockA.isConnected()) {
    if (!HardrockA.isBound()) {
      if (!BindByMap(HardrockA)) {
        BindByPort(HardrockA);
      }
      if (IC_705.isConnected()) {
        ReadRFPower();
      }
    }
  } else if (HardrockA.isBound()) {
    HardrockA.unbind();
  }

  if (HardrockB.isConnected()) {
    if (!HardrockB.isBound()) {
      if (!BindByMap(HardrockB)) {
        BindByPort(HardrockB);
      }
      if (IC_705.isConnected()) {
        ReadRFPower();
      }
    }
  } else if (HardrockB.isBound()) {
    HardrockB.unbind();
  }
}

bool BindByMap(CHardrockPair& rHardrock) {
  bool              fBound(false);
  CTeensy::eBinding eTarget((rHardrock.serialPort() == CTeensy::eHardrock::A)
                              ? CTeensy::eBinding::HardrockA
                              : CTeensy::eBinding::HardrockB);

  if (rHardrock.isConnected() && !rHardrock.isBound()) {
    CHardrockUSB* aHardrockUSB[4] = { &HardrockUSB1, &HardrockUSB2, &HardrockUSB3, &HardrockUSB4 };

    for (size_t nIndex(0);
         (!fBound && nIndex < sizeof aHardrockUSB / sizeof(CHardrockUSB*)); nIndex++) {
      if (aHardrockUSB[nIndex]->isUSBConnected()) {
        CTeensy::eBinding Binding(Teensy.getBinding(
          aHardrockUSB[nIndex]->idProduct(), aHardrockUSB[nIndex]->idVendor(),
          aHardrockUSB[nIndex]->serialNumber(), rHardrock.modelName()));
        if (Binding == eTarget && !aHardrockUSB[nIndex]->isBound()) {
          rHardrock.bind((rHardrock.serialPort() == CTeensy::eHardrock::A) ? BluetoothA : BluetoothB,
                         *aHardrockUSB[nIndex]);
          Tracer.TraceLn(
            rHardrock.deviceName() + " Bound by map to "
            + String((rHardrock.serialPort() == CTeensy::eHardrock::A) ? "BluetoothA" : "BluetoothB"));
          fBound = true;
        }
      }
    }
  }
  return fBound;
}

void BindByPort(CHardrockPair& rHardrock) {
  CHardrockUSB*                  aHardrock[4] = { &HardrockUSB1, &HardrockUSB2, &HardrockUSB3, &HardrockUSB4 };
  CHardrockBluetoothSlaveDevice& rBluetooth(
    (rHardrock.serialPort() == CTeensy::eHardrock::A) ? BluetoothA : BluetoothB);

  if (rHardrock.isConnected() && !rHardrock.isBound()
      && !rBluetooth.isBound(rHardrock.deviceClass())) {
    for (size_t nIndex(0); nIndex < sizeof aHardrock / sizeof(CHardrockUSB*); nIndex++) {
      if (aHardrock[nIndex]->isHardrockConnected()
          && aHardrock[nIndex]->isConnected() && !aHardrock[nIndex]->isBound()
          && rHardrock.modelName() == aHardrock[nIndex]->modelName()) {
        rHardrock.bind(rBluetooth, *aHardrock[nIndex]);
        Teensy.bind(
          (rHardrock.serialPort() == CTeensy::eHardrock::A)
            ? CTeensy::eBinding::HardrockA
            : CTeensy::eBinding::HardrockB,
          aHardrock[nIndex]->idProduct(), aHardrock[nIndex]->idVendor(),
          aHardrock[nIndex]->serialNumber(), aHardrock[nIndex]->modelName());
        Tracer.TraceLn(
          rHardrock.deviceName() + " Bound by port to "
          + String((rHardrock.serialPort() == CTeensy::eHardrock::A) ? "BluetoothA" : "BluetoothB"));
        break;
      }
    }
  }
}

void HighAlarmISR(void) {
  set_arm_clock(396000000);  // Reduce speed to 396 MHz
  // set an alarm at low temperature
  InternalTemperature.attachLowTempInterruptCelsius(fHighTempAlarmC - 10, &LowAlarmISR);
}

void LowAlarmISR(void) {
  set_arm_clock(528000000);  // Restore speed to 528 MHz
  // set an alarm at high temperature
  InternalTemperature.attachHighTempInterruptCelsius(fHighTempAlarmC, &HighAlarmISR);
}
}
