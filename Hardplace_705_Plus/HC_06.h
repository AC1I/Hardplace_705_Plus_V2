#if !defined HC_06_H
#define HC_06_H

#include <Arduino.h>

#include "Hardplace705Plus.h"
#include "SerialDevice.h"
#include "Tracer.h"


class CBluetoothSlaveDevice : public CSerialDevice {
public:
  CBluetoothSlaveDevice(
    Stream& Device, const char* pszName = "HC_06", uint32_t uBaudrate = 38400,
    unsigned long ulTimeout = 1000)
    : CSerialDevice(Device, uBaudrate, ulTimeout),
      m_sName(pszName) {
  }

public:
  String Response(void) {
    String sRsp;

    for (int nIndex(0); nIndex < 100; nIndex++) {
      while (available()) {
        sRsp += static_cast<char>(read());
      }
      if (sRsp.indexOf("OK") >= 0) {
        sRsp = sRsp.substring(sRsp.indexOf("OK"));
        while (available()) {
          sRsp += static_cast<char>(read());
        }
        break;
      }
      Delay(10);
    }
    return sRsp;
  }

  virtual bool setup(void) {
    unsigned uBaud(getBaudrate());

    begin();
    if (!ping()
        && discoverBaudrate()) {
      const unsigned auRates[] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1382400 };

      for (size_t nIndex = 0; nIndex < sizeof auRates / sizeof(unsigned); nIndex++) {
        if (auRates[nIndex] == uBaud) {
          write(String((String("AT+NAME") + m_sName)).c_str());
          Response();
          Delay(1000);

          write("AT+PIN0000");
          Response();
          Delay(1000);

          write(String(String("AT+BAUD") + String(nIndex + 1, HEX)).c_str());  // Needs to match TEENSY_MAX_BAUDRATE
          Response();
          begin(uBaud);
          break;
        }
      }
    }
    return true;
  }

  bool ping(void) {
    write("AT");
    return Response().indexOf("OK") >= 0;
  }

  virtual bool discoverBaudrate(void) {
    bool           fFound(ping());
    unsigned       uBaud(getBaudrate());
    const unsigned auRates[] = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1382400 };

    for (size_t nIndex = 0; (!fFound && nIndex < sizeof auRates / sizeof(unsigned)); nIndex++) {
      begin(auRates[nIndex]);
      Delay(100);
      clear();
      fFound = ping();
    }
    if (!fFound) {
      begin(uBaud);
      Delay(100);
      clear();
    }
    return fFound;
  }
  virtual String deviceName(void) const {
    return m_sName;
  }

private:
  String m_sName;
};

#endif
