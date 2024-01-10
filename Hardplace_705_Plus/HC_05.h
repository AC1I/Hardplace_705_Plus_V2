#if !defined HC_O5_H_DEFINED
#define HC_O5_H_DEFINED

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
#include "Bluetooth.h"
#include "Tracer.h"

class CHC_05Device : public CBluetoothDevice {
public:
  CHC_05Device(
    IBluetooth::TeensyBluetooth DeviceID, IBluetooth& rBluetooth, Stream& rDevice,
    uint32_t uBaudrate = 38400, unsigned long ulTimeout = 1000)
    : CBluetoothDevice(DeviceID, rBluetooth, rDevice, uBaudrate, ulTimeout),
      m_DefaultBaudrate(uBaudrate),
      m_LastComm(0) {
  }

private:
  CHC_05Device(const CHC_05Device&);
  CHC_05Device& operator=(const CHC_05Device&);

public:
  virtual int read() {
    int iReturn(CBluetoothDevice::read());
    if (iReturn >= 0) {
      m_LastComm = 0;
    }
    return iReturn;
  }
  virtual String deviceName(void) const {
    return String("HC-05");
  }
  virtual void ensureAvailablility(void) {
    if (isPoweredOn()
        && isReady()
        && isDataTransferMode()
        && !ping()) {
      PowerOff();
      Delay(250);
      PowerOn();
    }
  }
  virtual uint32_t defaultBaudrate(void) const {
    return m_DefaultBaudrate;
  }

public:
  virtual bool ping(void) {
    if (m_LastComm > 10000) {
      bool fDebugEnabled(m_Tracer.Enabled());
      m_Tracer.Disable();

      bool fReturn(ping(true));

      m_Tracer.Enable(fDebugEnabled);
      return fReturn;
    }
    return true;
  }

  virtual bool ping(bool bNow) {
    bool fAlive(true);
    if (bNow
        || m_LastComm > 10000) {
      const char* pszTestCmd = "AT";
      fAlive = SendATCmd(pszTestCmd) || SendATCmd(pszTestCmd);
    }
    return fAlive;
  }

  virtual bool reset(void) {
    CAutoATCmdMode restoreMode(this);
    bool           bReturn(SendATCmd("AT+RESET"));
    PowerOff();
    setBaudrate(defaultBaudrate());
    Delay(250);
    PowerOn();
    return bReturn;
  }

public:
  virtual bool discoverBaudrate(void) {
    CAutoATCmdMode restoreMode(this);
    bool           fFound(ping(true));
    unsigned       uBaud(getBaudrate());
    const unsigned auRates[] = { 38400, 115200, 9600, 57600, 19200, 4800, 230400, 921600, 1382400 };

    for (size_t nIndex = 0; (!fFound && nIndex < sizeof auRates / sizeof(unsigned)); nIndex++) {
      begin(auRates[nIndex]);
      Delay(100);
      clear();
      fFound = ping(true);
    }
    if (!fFound) {
      begin(uBaud);
      Delay(100);
      clear();
    }
    if (fFound
        && Tracer().Enabled()) {
      SendATCmd("AT+UART");
      Tracer().TraceLn("Baudrate discovered at " + String(getBaudrate()));
    }
    return fFound;
  }

public:
  void ATCmdMode(void) {
    bool fATCmdMode(isATCmdMode());
    m_rBluetooth.BluetoothATCmdMode(m_DeviceID);
    if (!fATCmdMode) {
      Delay(100);
    }
  }
  bool isATCmdMode(void) {
    return m_rBluetooth.BluetoothIsATCmdMode(m_DeviceID);
  }
  void DataTransferMode(void) {
    bool fDataTransferMode(isDataTransferMode());
    m_rBluetooth.BluetoothDataTransferMode(m_DeviceID);
    if (!fDataTransferMode) {
      Delay(100);
    }
  }
  virtual bool isDataTransferMode(void) {
    return m_rBluetooth.BluetoothIsDataTransferMode(m_DeviceID);
  }
  virtual bool isConnected(void) {
    return m_rBluetooth.BluetoothConnected(m_DeviceID);
  }
  virtual bool connectionStateChanged(void) {
    return m_rBluetooth.BluetoothConnectionStateChanged(m_DeviceID);
  }
  virtual bool remoteAvailable(void) {
    return (isConnected() && isDataTransferMode());
  }

public:
  bool SendATCmd(String Cmd, String& rRsp) {
    return SendATCmd(Cmd.c_str(), rRsp);
  }

  bool SendATCmd(String Cmd) {
    String Rsp;
    return SendATCmd(Cmd, Rsp);
  }

  bool SendATCmd(const char* pszCmd, char* pszRspBuf = 0, size_t stBufLen = 0) {
    String Rsp;

    bool bStatus(SendATCmd(pszCmd, Rsp));
    if (pszRspBuf
        && Rsp.length()) {
      size_t stToCopy(stBufLen > Rsp.length() ? Rsp.length() : stBufLen - 1);
      strncpy(pszRspBuf, Rsp.c_str(), stToCopy);
      pszRspBuf[stToCopy] = '\0';
    }
    return bStatus;
  }

  bool SendATCmd(const char* pszCmd, String& Rsp) {
    CAutoATCmdMode restoreMode(this);

    clear();
    println(pszCmd);

    for (bool bComplete(false); !bComplete; Delay(1)) {
      String Response(readStringUntil('\n'));
      Rsp += Response;
      if (Response.indexOf('+') >= 0) {
        continue;
      }
      bComplete = Response.length() == 0
                  || Response[Response.length() - 1] != '\n'
                  || Rsp.indexOf("OK\r\n") >= 0
                  || Rsp.indexOf("FAIL\r\n") >= 0
                  || Rsp.indexOf("ERROR") >= 0;
    }
    if (Rsp.length()) {
      m_Tracer.Trace(String(pszCmd) + " - " + Rsp);
    } else {
      m_Tracer.TraceLn(pszCmd);
    }
    return (Rsp.indexOf("OK\r\n") >= 0);
  }

  class CAutoATCmdMode {
  public:
    CAutoATCmdMode(CHC_05Device& rDevice)
      : m_rDevice(rDevice),
        m_fisDataTransferMode(m_rDevice.isDataTransferMode()) {
      m_rDevice.ATCmdMode();
    }
    CAutoATCmdMode(CHC_05Device* pDevice)
      : m_rDevice(*pDevice),
        m_fisDataTransferMode(m_rDevice.isDataTransferMode()) {
      m_rDevice.ATCmdMode();
    }
    ~CAutoATCmdMode() {
      if (m_fisDataTransferMode) {
        m_rDevice.DataTransferMode();
      }
    }

  private:
    CAutoATCmdMode();
    CAutoATCmdMode(const CAutoATCmdMode&);
    CAutoATCmdMode& operator=(const CAutoATCmdMode&);

  private:
    CHC_05Device& m_rDevice;
    bool          m_fisDataTransferMode;
  };

private:
  uint32_t      m_DefaultBaudrate;
  elapsedMillis m_LastComm;
};
#endif
