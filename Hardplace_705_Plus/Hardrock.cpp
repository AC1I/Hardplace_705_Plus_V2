#include <new>

#include "Hardrock.h"
#include "Hardrock50.h"
#include "Hardrock50Plus.h"
#include "Hardrock500.h"
#include "SerialDevice.h"

const char* CHardrock50::modelName() {
  return "Hardrock50";
}
const char* CHardrock500::modelName() {
  return "Hardrock500";
}
const char* CHardrock50Plus::modelName() {
  return "Hardrock50+";
}

class CHardrockStub : private CHardrock {
public:
  CHardrockStub(CSerialDevice& rDevice)
    : CHardrock(rDevice) {
    begin();
  }

private:
  CHardrockStub();
  CHardrockStub(const CHardrockStub&);
  CHardrockStub& operator=(const CHardrockStub&);

public:
  bool HardrockCommand(const char* pszCmd) {
    String Cmd(pszCmd);
    String Rsp;

    clear();
    do {
      attention();
      write(Cmd);
      Rsp = readStringUntil(';');
    } while (Rsp.length()
             && Rsp.indexOf(Cmd.substring(0, Cmd.length() - 1).c_str()) < 0);
    return Rsp.length() > 0;
  }

  bool discoverBaudrate(void) {
    const char*    pszBandCmd("HRBN;");
    bool           fHaveComms(HardrockCommand(pszBandCmd));
    const uint32_t ulDefaultBaudrate(getBaudrate());
    const uint32_t aulBaudrates[] = { 19200, 115200, 38400, 57600, 9600, 4800 };

    for (size_t nIndex(0);
         (!fHaveComms && nIndex < sizeof aulBaudrates / sizeof(uint32_t));
         nIndex++, Delay(0)) {
      setBaudrate(aulBaudrates[nIndex]);
      fHaveComms = HardrockCommand(pszBandCmd);
      if (fHaveComms) {
        Tracer().TraceLn(
            "Hardrock found at "
            + String(aulBaudrates[nIndex])
            + " baud");
      }
    }
    if (!fHaveComms) {
      setBaudrate(ulDefaultBaudrate);
    }
    return fHaveComms;
  }

  CHardrock* CHardrockFactory(void) {
    CHardrock* pHardrock(0);
    bool       fHaveComms(discoverBaudrate());

    if (fHaveComms) {
      if (HardrockCommand("HRAA;")) {
        if (HardrockCommand("HRAN;")) {
          pHardrock =
            new CHardrock500(*this);  // Hardrock 500
        } else {
          pHardrock =
            new CHardrock50Plus(*this);  // Hardrock 50+
        }
      } else {
        pHardrock =
          new CHardrock50(*this);  // Hardrock 50
      }
    }
    return pHardrock;
  }

private:
  virtual void setFrequency(uint64_t ullFrequencyHz) {}
  virtual bool isHardrockConnected(void) {
    return false;
  }
  virtual unsigned getFrequencyBand(void) {
    return 0;
  }
  virtual void setFrequencyBand(unsigned uBand) {}
  virtual void setFrequencyBand(unsigned long ulFrequency100MHz, unsigned long ulFrequencyHz) {}
  virtual int  getKeyingMode(void) {
     return 0;
  }
  virtual void  setKeyingMode(bool bPTTOn) {}
  virtual float getCurrentPowerVSWR(char chWhich) {
    return 0.0;
  }
  virtual bool getStatus(unsigned& ruFwdPower,
                         unsigned& ruReflectedPower,
                         unsigned& ruDrivePower,
                         unsigned& ruSWR,
                         unsigned& ruDrainVoltage,
                         unsigned& ruDrainCurrent,
                         unsigned& ruTemperature) {
    return true;
  }
  virtual String getTemperature(void) {
    return String();
  }
  virtual bool isTemperatureScaleCelsius(void) {
    return true;
  }
  virtual void  setTemperatureScaleCelsius(bool bCelsius) {}
  virtual float getDCInputVoltage(void) {
    return 0.0;
  }
  virtual bool isATUPresent(void) {
    return true;
  }
  virtual void Tune(void) {}
  virtual bool isTuning(void) {
    return false;
  }
  virtual unsigned getActiveAntenna(void) {
    return 1;
  }
  virtual bool SaveATUSettings(void) {
    return true;
  }
  virtual bool isTunerByPassed(void) {
    return false;
  }
  virtual void        setTunerByPass(bool bByPass) {}
  virtual const char* Model(void) const {
    return "HardrockStub";
  }
};

CHardrock* CHardrock::CHardrockFactory(CSerialDevice& rDevice) {
  return CHardrockStub(rDevice).CHardrockFactory();
}
bool CHardrock::isHardrockPacket(const String& rPacket) {
  String sPacket(rPacket.substring(0, 2));

  sPacket.toUpperCase();
  return ((sPacket == "HR"
           || sPacket == "FA"
           || sPacket == "IF")
          && rPacket.indexOf(';') > 0);
}
