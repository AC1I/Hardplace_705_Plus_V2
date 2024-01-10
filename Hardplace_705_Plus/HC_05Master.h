#if !defined HC_O5_MASTER_H_DEFINED
#define HC_O5_MASTER_H_DEFINED
/*
    To return HC-05 to mfg. default settings: "AT+ORGL"
    To get version of your HC-05 enter: "AT+VERSION?"
    To change device name from the default HC-05 to let's say MYBLUE enter: "AT+NAME=MYBLUE"
    To change default security code from 1234 to 2987 enter: "AT+PSWD=2987"
    To change HC-05 baud rate from default 9600 to 115200, 1 stop bit, 0 parity enter: "AT+UART=115200,1,0"
*/

/*
    You have 2 way to enter the AT CMD mode:

  Way 1:

  Step 1: Input low level to PIN34->EN PIN. (it can also be floating)
  Step 2: Supply power to the module.
  Step 3: Input high level to the EN PIN(PIN34)(you can press the button this do this action,
          if you buy HC-05 from DSD TECH)
  Then the module will enter to AT mode. The baud rate is as same as the communication time, default is  9600

  Way 2:
  Step 1: Connect PIN34(EN PIN) to the power supply PIN.or always press the button .
  Step 2: Supply power to module
          Then the module will enter to AT module. But at this time, the baudrate is always 38400.
          In this way, user should change the baud rate at the AT mode, if they forget the communication baud rate.

          Note:
          If PIN34 is tied to HIGH Full AT Mode is enabled, the switch, or "Way" 1 will only give you AT Mode
*/

/*
   HC-05 zs-040 Status LED

  The onboard LED shows the current state of the module.
  – rapid flash (about 5 times a second) – module is on and waiting for a connection or pairing
  – single quick flash once every 2 seconds – the module has just paired with another device
  – double quick flash every 2 seconds – connected to another device

  The HC-05 remembers devices it has paired with and after cycling the power you do not need to re-pair. This means you can go from
  – rapid flash (about 5 times a second) – module is on and waiting for a connection or pairing, to
  – double quick flash every 2 seconds – connected to another device directly.
*/

/*                                 FULL AT mode
   The HC-05 has a connection from pin 34 to +3.3v. This activates “full” AT mode.
*/

/*                  Connecting from HC-06
   http://www.martyncurrey.com/connecting-2-arduinos-by-HC_05-using-a-hc-05-and-a-hc-06-pair-bind-and-link/#more-2098
*/

/*
   AT+RMAAD      clears any previously paired devices.
   AT+ROLE=1     puts the HC-05 in Master Mode
   AT+RESET      reset the HC-05. This is sometimes needed after changing roles.

   AT+CMODE=0    allows the HC-05 to connect to any device
   AT+INIT       initiates the SPP profile. If SPP is already active you will get an error(17) which you can ignore.
   AT+IAC=9e8b33 Inquire HC_05 device has an access code
   AT+CLASS=0    Device type Any (220400 for IC-705)

   AT+INQM=0,5,9 set inquiry to search for up to 5 devices for 9 seconds
   AT+INQ        searches for other HC_05 devices.

*/

#include <elapsedMillis.h>

#include "Hardplace705Plus.h"
#include "Teensy41.h"
#include "HC_05.h"
#include "EEPromStream.h"
#include "Tracer.h"

#define VER_HC_05MASTER 1

class CHC_05MasterDevice : public CHC_05Device, CEEPROMStream {
public:
  CHC_05MasterDevice(
    IBluetooth::TeensyBluetooth DeviceID, IBluetooth& rBluetooth, Stream& rDevice,
    uint32_t uBaudrate = 38400, unsigned long ulTimeout = 1000,
    const char* pszName = "H-C-2010-06-01",
    const char* pszRName = "HC-05",
    const char* pszPIN = "1234", unsigned long ulClass = 0,
    uint8_t eRecordType = CEEPROMStream::eReservedRecordTypes::Invalid)
    : CHC_05Device(DeviceID, rBluetooth, rDevice, uBaudrate, ulTimeout),
      CEEPROMStream(eRecordType, VER_HC_05MASTER),
      m_uBaudrate(uBaudrate),
      m_sName(pszName),
      m_sRName(pszRName),
      m_sPIN(pszPIN),
      m_ulClass(ulClass),
      m_ulInquireTimeSecs(30),
      m_fSetup(false),
      m_fConnected(false),
      m_LastAttempt(5000),
      m_AttemptTime(0) {
    Serialize(haveRecord());
    Tracer().TraceLn("Bind Address " + m_sBoundAddress);
  }

private:
  CHC_05MasterDevice(const CHC_05MasterDevice&);
  CHC_05MasterDevice& operator=(const CHC_05MasterDevice&);

public:
  virtual bool setup(void) {
    begin();
    if (!isPoweredOn()) {
      if (getBaudrate() == 38400) {
        ATCmdMode();
      } else {
        DataTransferMode();
      }
      PowerOn();
      m_fSetup = false;
    }
    if (!m_fSetup) {
      m_fSetup = discoverBaudrate();
      if (m_fSetup) {
        InitSPPLib();
        if (m_sBoundAddress.length()) {
          Bind(m_sBoundAddress);
        }
        m_AttemptTime = 0;
      }
    }
    return m_fSetup;
  }

  virtual void Task(void) {
    CHC_05Device::Task();

    if (m_LastAttempt >= 5000) {
      if (!isReady()) {
        setup();
      } else if (m_fConnected
                 && !isConnected()) {
        if (isPoweredOn()
            && isReady()) {
          if (m_AttemptTime > (60 * 1000)) {  // Deal with wierd state where bluetooth is connected
            PowerOff();                       // but the module doesn't recognize it
          } else {
            Disconnect();
          }
        }
        m_fConnected = false;
      } else if (isPoweredOn()
                 && !isConnected()) {
        m_fConnected = LinkBoundDevice();
      } else {
        m_AttemptTime = 0;
      }
      m_LastAttempt = 0;
    }
  }

  virtual void PowerOff(void) {
    CBluetoothDevice::PowerOff();
    setBaudrate(defaultBaudrate());
    m_fSetup = false;
  }

  virtual String deviceName(void) const {
    return m_sName;
  }

  virtual void PowerOn(void) {
    CHC_05Device::PowerOn();
    for (elapsedMillis wait(0); isConnected() && wait < 1000; delay(1))  // Wait for HC-05 to initialize
      ;
  }

  bool isReady(void) {
    return m_fSetup;
  }

  bool isConnected(void) {
    return CHC_05Device::isConnected();
  }

  bool isConnected(bool isManualConnectMode) {
    bool fisConnected(false);
    if (isManualConnectMode) {
      fisConnected = isConnected();
    } else {
      CAutoATCmdMode restoreMode(this);
      String         Response;
      SendATCmd("AT+STATE?", Response);
      fisConnected = Response.indexOf("+STATE:CONNECTED") >= 0;
    }
    return fisConnected;
  }

public:
  void DisconnectBoundDevice(void) {
    CAutoATCmdMode restoreMode(this);
    Disconnect();
    for (elapsedMillis waitTime(0); isConnected() && waitTime < 1000; Delay(1))
      ;
  }

public:
  unsigned long InquireTime(unsigned long ulSeconds) {
    return (ulSeconds * 100) / 128;
  }

  void setInquireTime(unsigned long ulTimeout) {
    m_ulInquireTimeSecs = (ulTimeout * 128) / 100;
  }

  String InquireTimeout(void) {
    return String(InquireTime(m_ulInquireTimeSecs));
  }

  unsigned long InquireTimeoutMSecs(void) {
    return m_ulInquireTimeSecs * 1000;
  }

  String InquireAccessModeCmd(bool bAccessModeRSSI = false, unsigned cMaxDevices = 1, unsigned long ulTimeoutSeconds = 30) {
    String InquireModeCmd("AT+INQM=");
    InquireModeCmd += (bAccessModeRSSI) ? "1" : "0";
    InquireModeCmd += ",";
    InquireModeCmd += cMaxDevices;
    InquireModeCmd += ",";
    InquireModeCmd += InquireTime(ulTimeoutSeconds);

    return InquireModeCmd;
  }

public:
  bool LinkToDevice(const String& sDevice) {
    CAutoATCmdMode restoreMode(this);
    bool           bReturn(false);
    unsigned long  ulHC_05Timeout(getTimeout());

    setTimeout((ulHC_05Timeout >= 10000) ? ulHC_05Timeout : 10000);  // Long timeout for connect
    Delay(0);
    String sResp;
    bReturn = SendATCmd(String("AT+LINK=") + sDevice, sResp);  // Link to device
    Delay(0);
    if (!bReturn) {
      if (sResp.indexOf("FAIL") >= 0) {
        Disconnect();
      } else if (sResp.indexOf("ERROR:(16)") >= 0) {
        InitSPPLib();
      }
    } else if (sResp.indexOf("ERROR:(0)") >= 0) {
      PowerOff();
    }
    setTimeout(ulHC_05Timeout);
    return bReturn;
  }

  bool Bind(const String& sAddress) {
    CAutoATCmdMode restoreMode(this);
    return SendATCmd(String("AT+BIND=") + sAddress);
  }

  bool GetBindAddress(String& Address) {
    CAutoATCmdMode restoreMode(this);
    String         Resp;

    if (SendATCmd("AT+BIND?", Resp)
        && Resp.length() > 6
        && Resp.indexOf(':') >= 0
        && Resp.substring(0, 6) == String("+BIND:")) {
      Resp = Resp.substring(1 + Resp.indexOf(':'), Resp.indexOf('\r'));
      Resp.replace(':', ',');
      Address = Resp;
      return true;
    }
    return false;
  }

  bool isManualConnectionMode(void) {
    CAutoATCmdMode restoreMode(this);
    bool           fisManualConnectionMode(false);

    String Resp;
    if (SendATCmd("AT+CMODE?", Resp)
        && Resp.indexOf("+CMOD") >= 0) {
      if (Resp.indexOf("+CMODE:") >= 0) {
        fisManualConnectionMode = Resp[Resp.indexOf("+CMODE:") + 7] == '0';
      } else if (Resp.indexOf("+CMOD:") >= 0) {
        fisManualConnectionMode = Resp[Resp.indexOf("+CMOD:") + 6] == '0';
      }
    }
    return fisManualConnectionMode;
  }

  bool LinkBoundDevice(void) {
    CAutoATCmdMode restoreMode(this);
    bool           bReturn(false);

    if (m_sBoundAddress.length() == 0) {
      GetBindAddress(m_sBoundAddress);
    }
    if (m_sBoundAddress.length()) {             // Device last Paired with/Bound to
      bReturn = LinkToDevice(m_sBoundAddress);  // Link to device "3031,7D,341D93"
    }
    return bReturn;
  }

  void Disconnect(void) {
    CAutoATCmdMode restoreMode(this);
    String         sResp;
    if (!SendATCmd(String("AT+DISC"), sResp)  // Disconnect
        && sResp.indexOf("ERROR:(16)") >= 0) {
      InitSPPLib();
    }
  }

  bool InitSPPLib(void) {
    CAutoATCmdMode restoreMode(this);
    return SendATCmd("AT+INIT");
  }

  bool Pair(const String& sDeviceAddr, unsigned long ulTimeout) {
    CAutoATCmdMode restoreMode(this);
    unsigned long  ulTimeoutCache(getTimeout());
    setTimeout((ulTimeout * 1000) + 1000);
    bool bReturn(SendATCmd(String("AT+PAIR=" + sDeviceAddr + String(",") + String(ulTimeout, DEC))));
    setTimeout(ulTimeoutCache);
    return bReturn;
  }

  bool Pair(void) {
    CAutoATCmdMode restoreMode(this);
    bool           bReturn(false);
    unsigned long  ulHC_05Timeout(getTimeout());

    if (!SendATCmd("AT+VERSION?")) {  // VERSION:2.0-20100601 NOT VERSIOn 3!
      discoverBaudrate();
      SendATCmd("AT+VERSION?");
    }
    if (SendATCmd(String("AT+NAME=\"")
                  + String(m_sName) + String("\""))  // Device Name
        && SendATCmd(String("AT+PSWD=\"")
                     + m_sPIN + String("\""))  // Password
        && SendATCmd(String("AT+UART=")        // Baudrate
                     + String(m_uBaudrate) + String(",0,0"))
        && SendATCmd("AT+RMAAD")                // Delete all authenticated devices (clear pair list)
        && SendATCmd("AT+ROLE=1")               // Master Device
        && SendATCmd("AT+RESET")                // Reset the device
        && (Delay(1000), SendATCmd("AT+INIT"))  // Initialize the SPP profile lib
        && SendATCmd("AT+CMODE=0")              // Manual connection mode
        && SendATCmd("AT+IAC=9e8b33")           // General Inquiries
        && SendATCmd(String("AT+CLASS=")
                     + String(m_ulClass, DEC))) {  // Class
      unsigned long ulInquireTimeoutMSecs(InquireTimeoutMSecs() + 1000);

      setTimeout((ulHC_05Timeout >= ulInquireTimeoutMSecs) ? ulHC_05Timeout : ulInquireTimeoutMSecs);  // Long timeout for Inquire, RName

      if (SendATCmd(InquireAccessModeCmd(false, 1, m_ulInquireTimeSecs))) {
        String Response;
        String RName;

        if (SendATCmd("AT+INQ", Response)) {
          if (Response.length()
              && Response.indexOf(',')) {
            Response = Response.substring(0, Response.indexOf(','));
          }
          while (Response.length() > 0
                 && Response.indexOf(':') >= 0) {
            String Device(Response.substring(1 + Response.indexOf(':'), Response.indexOf('\r')));
            Device.replace(':', ',');

            if (SendATCmd(String("AT+RNAME?") + String(Device), RName)
                && RName == String("+RNAME:") + m_sRName + String("\r\nOK\r\n")) {
              RName = String(Device);
              break;
            } else {
              RName = String();
            }
            if (Response.indexOf('\n')) {
              Response = Response.substring(Response.indexOf('\n') + 1);
            } else {
              Response = String();
            }
          }
        }

        setTimeout(ulHC_05Timeout);
        if (RName.length()
            && Pair(RName, 120)
            && Bind(RName)) {
          saveBoundAddress(RName);
          bReturn = LinkToDevice(RName);
        }
        //          SendATCmd(InquireAccessModeCmd(false, 2, m_ulInquireTimeSecs));
      }
    }
    setTimeout(ulHC_05Timeout);
    return bReturn;
  }

  virtual bool Paired(void) const {
    return m_sBoundAddress.length() != 0;
  }

  virtual void clearPairing(void) {
    m_sBoundAddress = String();
    Serialize();
  }

private:
  void Serialize(bool bLoad = false) {
    const unsigned uMaxAddressLen(14);  // 12 digit hexidecimal value (BD_ADDR)

    CEEPROMStream::rewind();

    if (bLoad) {
      get(m_sBoundAddress, uMaxAddressLen);
    } else {
      put(m_sBoundAddress, uMaxAddressLen);
      CEEPROMStream::flush();
    }
  }

  void saveBoundAddress(const String& rBoundAddress) {
    m_sBoundAddress = rBoundAddress;  //3031,7D,341D93
    Serialize();
  }

private:
  uint32_t      m_uBaudrate;
  String        m_sName;
  String        m_sRName;
  String        m_sPIN;
  unsigned long m_ulClass;
  unsigned long m_ulInquireTimeSecs;
  bool          m_fSetup;
  bool          m_fConnected;
  String        m_sBoundAddress;
  elapsedMillis m_LastAttempt;
  elapsedMillis m_AttemptTime;
};
#endif
