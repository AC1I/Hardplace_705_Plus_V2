#if !defined HARDROCK50PLUS_H_DEFINED
#define HARDROCK50PLUS_H_DEFINED

#include <elapsedMillis.h>

#include "Hardplace705Plus.h"
#include "Hardrock.h"

class CHardrock50Plus : public CHardrock {
public:
  CHardrock50Plus(CSerialDevice& rDevice)
    : CHardrock(rDevice),
      m_Interval(0), m_fConnected(false), m_fLastConnState(false), m_FreqSupported(false) {
  }
  virtual ~CHardrock50Plus() {
  }

private:
  CHardrock50Plus();
  CHardrock50Plus(const CHardrock50Plus&);
  CHardrock50Plus& operator=(const CHardrock50Plus&);

public:
  virtual const char* Model(void) const {
    return modelName();
  }
  static const char* modelName(void);

public:
  virtual void setFrequency(uint64_t ullFrequencyHz) {
    m_FreqSupported = ullFrequencyHz < 30000000;
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      char achBuf[32];
      sprintf(achBuf, "FA%011llu;", ullFrequencyHz);
      String sCmd(achBuf);
      write(sCmd);
    }
  }
  virtual bool isHardrockConnected(void) {
    // HRAA;HRAA;<CR><LF>
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRAA;");
      String Rsp;
      do {
        write(Cmd);
        Rsp = readString();
      } while (Rsp.length() && Rsp != Cmd);
      return (Rsp == Cmd);
    }
    return false;
  }
  virtual unsigned getFrequencyBand(void) {
    // HRBN;HRBN5;<CR><LF>
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRBN;");
      write(Cmd);
      String Rsp(readString());
      if (isValidResponse(Rsp, Cmd)) {
        return Rsp.charAt(4) - '0';
      }
    }
    return 99;
  }
  virtual void setFrequencyBand(unsigned uBand) {
    // HRBN7;
    CHardrock::autolock(*this);

    if (availableForWrite()) {
      String Cmd("HRBN");
      Cmd += uBand;
      Cmd += ';';
      write(Cmd);
    }
  }
  virtual void setFrequencyBand(unsigned long ulFrequency100MHz, unsigned long ulFrequencyHz) {
    unsigned uBand(99);

    if (ulFrequency100MHz == 0) {
      if (ulFrequencyHz >= 28000000
          && ulFrequencyHz <= 29700000) {
        uBand = 1;  // 1 = 10M
      } else if (ulFrequencyHz >= 24890000
                 && ulFrequencyHz <= 24990000) {
        uBand = 2;  // 2 = 12M
      } else if (ulFrequencyHz >= 21000000
                 && ulFrequencyHz <= 21450000) {
        uBand = 3;  // 4 = 15M
      } else if (ulFrequencyHz >= 18068000
                 && ulFrequencyHz <= 18168000) {
        uBand = 4;  // 3 = 17M
      } else if (ulFrequencyHz >= 14000000
                 && ulFrequencyHz <= 14350000) {
        uBand = 5;  // 5 = 20M
      } else if (ulFrequencyHz >= 10100000
                 && ulFrequencyHz <= 10150000) {
        uBand = 6;  // 6 = 30M
      } else if (ulFrequencyHz >= 7000000
                 && ulFrequencyHz <= 7300000) {
        uBand = 7;  // 7 = 40M
      } else if (ulFrequencyHz >= 5351500
                 && ulFrequencyHz <= 5366500) {
        uBand = 8;  // 8 = 60M
      } else if (ulFrequencyHz >= 3500000
                 && ulFrequencyHz <= 4000000) {
        uBand = 9;  // 9 = 80M
      } else if (ulFrequencyHz >= 1800000
                 && ulFrequencyHz <= 2000000) {
        uBand = 10;  // 10 = 160M
      }
    }
    setFrequencyBand(uBand);
  }
  virtual int getKeyingMode(void) {
    // HRMD;HRMD0;<CR><LF>
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRMD;");
      write(Cmd);
      String Rsp(readString());
      if (isValidResponse(Rsp, Cmd)  // There is some strangness where keying mode comes up invalid
          && Rsp.length() > 6) {     // on powerup (HRMD144;) or such,
        setKeyingMode(true);         // if this is the case switch it to PTT
        write(Cmd);
        Rsp = readString();
      }
      if (isValidResponse(Rsp, Cmd)
          && Rsp.length() >= 5
          && Rsp.charAt(4) >= '0'
          && Rsp.charAt(4) <= '3') {
        return Rsp.charAt(4) - '0';
      }
    }
    return -1;
  }
  virtual void setKeyingMode(bool bPTTOn) {
    // HRMD1; 0 (OFF), 1 (PTT), 2 (COR), 3 (QRP)
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRMD");
      Cmd += (bPTTOn) ? "1" : "0";
      Cmd += ";";
      write(Cmd);
    }
  }
  virtual float getCurrentPowerVSWR(char chWhich) {
    // HRPWF;HRPWF000;<CR><LF>
    // HRPWR;HRPWR000;<CR><LF>
    // HRPWD;HRPWD000;<CR><LF>
    // HRPWV;HRPWV000;<CR><LF>
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd(String(String("HRPW") + String(chWhich) + String(';')));
      write(Cmd);
      String Rsp(readString());
      if (isValidResponse(Rsp, Cmd)) {
        return atof(Rsp.substring(5, Rsp.length() - 6).c_str());
      }
    }
    return 0;
  }
  virtual bool getStatus(unsigned& ruFwdPower,
                         unsigned& ruReflectedPower,
                         unsigned& ruDrivePower,
                         unsigned& ruSWR,
                         unsigned& ruDrainVoltage,
                         unsigned& ruDrainCurrent,
                         unsigned& ruTemperature) {
    // HRST;HRST-000-000-000-000-058-000-020-;<CR><LF>
    return false;
  }
  virtual String getTemperature(void) {
    // HRTP;HRTP69F;<CR><LF>
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRTP;");
      write(Cmd);
      String Rsp(readString());
      if (isValidResponse(Rsp, Cmd)
          && (Rsp.charAt(4) == '0' || Rsp.charAt(4) == '1')) {
        return String(Rsp.substring(4, Rsp.length() - 5));
      }
    }
    return String();
  }
  virtual bool isTemperatureScaleCelsius(void) {
    // HRTS;HRTSF;<CR><LF>
    String sTemp(getTemperature());
    return sTemp.charAt(sTemp.length() - 2) == 'C';
  }
  virtual void setTemperatureScaleCelsius(bool bCelsius) {
    // HRTSC;
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String sCmd(String("HRTS") + (bCelsius) ? 'C' : 'F' + ';');
      write(sCmd);
    }
  }
  virtual float getDCInputVoltage(void) {
    // HRVT;HRVT58;<CR><LF>
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRVT;");
      write(Cmd);
      String Rsp(readString());
      if (isValidResponse(Rsp, Cmd)) {
        return atof(Rsp.substring(4, Rsp.length() - 5).c_str());
      }
    }
    return 0;
  }
  virtual bool isATUPresent(void) {
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRTMV?;");
      write(Cmd);
      String Rsp(readString());
      return (Rsp.length() > 5);
    }
    return false;
  }
  virtual void Tune(void) {
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRTMA;");
      write(Cmd);
    }
  }
  virtual bool isTuning(void) {
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRTMS?;");
      write(Cmd);
      String Rsp(readString());
      return (Rsp.length() > 4
              && Rsp.substring(0, 4) == Cmd.substring(0, 4)
              && Rsp.charAt(5) != ';');
    }
    return false;
  }
  virtual unsigned getActiveAntenna(void) {
    return 1;
  }
  virtual bool SaveATUSettings(void) {
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRTMQ;");
      return (write(Cmd) == Cmd.length());
    }
    return false;
  }
  virtual bool isTunerByPassed(void) {
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRTB;");
      write(Cmd);
      String Rsp(readString());
      if (isValidResponse(Rsp, Cmd)
          && (Rsp.charAt(4) == '0' || Rsp.charAt(4) == '1')) {
        return Rsp.charAt(4) == '0';
      }
    }
    return false;
  }
  virtual void setTunerByPass(bool bByPass) {
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRTB");
      Cmd += (bByPass) ? "0;" : "1;";
      write(Cmd);
    }
  }

public:
  bool HardrockAvailable(bool fNoDelay = false) {
    if (fNoDelay
        || m_Interval > 5000) {
      attention();
      m_fConnected = isHardrockConnected();
    }
    return m_fConnected;
  }

private:
  elapsedMillis m_Interval;
  bool          m_fConnected;
  bool          m_fLastConnState;
  bool          m_FreqSupported;
};
#endif
