#if !defined HARDROCK_H_DEFINED
#define HARDROCK_H_DEFINED

/*
         Hardrock 50                                                    |        Hardrock 50+                                       |        Hardrock 500
  FA   – VFO A Frequency                                                | FA   – VFO A Frequency                                    | FA   – VFO A Frequency
                                                                        | HRAA – Acknowledge Message                                | HRAA – Acknowledge Message
                                                                        |                                                           | HRAN – Select which antenna is selected
                                                                        |                                                           | HRAP – Ask of the ATU is present
  HRAT – Set ATU to bypass or active                                    |                                                           | HRAT – Set ATU to bypass or active
  HRBN – Set HR50’s operating band                                      | HRBN – Set HR50+’s operating band                         | HRBN – Set the HR500’s frequency band
  HRBR – Set the ACC port baud rate                                     |                                                           |
                                                                        |                                                           | HRFW – Forces a reset and activates the bootloader to accept new firmware
  HRKX – Set ACC port to inverted data for the KX3                      |                                                           |
  HRMD – Set the HR50’s keying mode                                     | HRMD – Set the HR50+’s keying mode                        | HRMD – Set the HR500’s keying mode
  HRMX – Report the maximum PEP, Average power and SWR from the last TX |                                                           |
  HRRP – turn on/off an automatic HRMX message after every key          |                                                           |
                                                                        | HRPW – Read the HR50+’s FWD, RFL, Drive power or SWR      | HRPW – Read the HR500’s FWD, RFL, Drive power or SWR
                                                                        | HRST – Reports the complete operating status of the HR50+ | HRST – Reports the complete operating status of the HR500
  HRRX – Read HR50 status                                               |                                                           |
                                                                        |                                                           | HRTB – Bypass or activate the ATU
  HRTM – Pass data to ATU                                               | HRTM – Pass data to ATU                                   | HRTM – Pass data to ATU
  HRTP – Set temperature to F or C                                      | HRTP – Read the heatsink temperature                      | HRTP – Read the heatsink temperature
  HRTU – Forces the ATU to tune the next time the amp is keyed          |                                                           |
                                                                        |                                                           | HRTR – Report result of last tuning attempt
  HRTS – Report the status of the last tune cycle                       | HRTS – Set the temperature display to ‘F’ or ‘C’          | HRTS – Set the temperature display to ‘F’ or ‘C’
                                                                        |                                                           | HRTT – Ask if the ATU is tuning
                                                                        |                                                           | HRTU – Put the ATU in TUNE mode
  HRVT – Read HR50’s DC input voltage                                   | HRVT – Read HR50+’s MOSFET drain voltage, VDD             | HRVT – Read HR500’s MOSFET drain voltage, VDD
  IF   – General information                                            | IF – General information                                  | IF – General information
*/

#include <Arduino.h>
#include <elapsedMillis.h>

#include "Hardplace705Plus.h"
#include "SerialDevice.h"
#include "Teensy41.h"
#include "Tracer.h"

class CHardrock : public CSerialDevice {
public:
  CHardrock(
    CSerialDevice& rDevice)
    : CSerialDevice(rDevice),
      m_IntercommandPeriod(200), m_LastReadWrite(0) {
  }

  virtual ~CHardrock() {
  }

public:
  static CHardrock* CHardrockFactory(CSerialDevice& rDevice);
  static bool       isHardrockPacket(const String& rPacket);

private:
  CHardrock();
  CHardrock(const CHardrock&);
  CHardrock& operator=(const CHardrock&);

public:
  virtual void setup(void) {
    attention();
  }
  virtual void Task(void) {
  }

protected:
  uint32_t intercommandPeriod(void) const {
    return m_IntercommandPeriod;
  }

public:
  bool isValidResponse(String& rRsp, String& rCmd, unsigned nCmdLen = 4) {
    return (rRsp.length() > nCmdLen
            && rRsp.substring(0, nCmdLen) == rCmd.substring(0, nCmdLen)
            && rRsp.charAt(rRsp.length() - 1) == ';');
  }

public:
  int read(String& rDest, unsigned uTimeoutMilliSecs = 250) {
    unsigned long ulTimeout(getTimeout());

    setTimeout(uTimeoutMilliSecs);
    rDest = readStringUntil(';');
    setTimeout(ulTimeout);

    int nIndex(rDest.indexOf("HR"));
    if (nIndex > 0) {
      rDest = rDest.substring(nIndex);
    } else if (nIndex < 0) {
      rDest.remove(0);
    }
    m_LastReadWrite = 0;
    return rDest.length();
  }

  String readString(unsigned uTimeoutMilliSecs = 250) {
    String Read;

    read(Read, uTimeoutMilliSecs);
    // Tracer().TraceLn("Hardrock::readString: " + Read);
    return Read;
  }
  String readString(String& rCmd, unsigned uTimeoutMilliSecs = 250) {
    elapsedMillis Timeout;
    String        Response(readString(uTimeoutMilliSecs));

    while (Response.length()
           && !isValidResponse(rCmd, Response)) {
      Response.remove(0);
      unsigned TimeRemaining(uTimeoutMilliSecs - Timeout);
      if (Timeout >= uTimeoutMilliSecs) {
        break;
      }
      Response += readString(TimeRemaining);
    }
    m_LastReadWrite = 0;
    return Response;
  }

  size_t write(const String& rCmd) {
    while (m_LastReadWrite <= m_IntercommandPeriod) {
      Delay(1);
    }
    clear();

    size_t stWritten(CSerialDevice::write(rCmd));
    // Tracer().TraceLn("Hardrock::write: " + rCmd);
    m_LastReadWrite = 0;
    return stWritten;
  }
  String readStringUntil(char terminator) {
    String Rsp(CSerialDevice::readStringUntil(terminator));
    for (size_t nIndex(0); nIndex < Rsp.length();) {
      if (!isAlphaNumeric(Rsp[nIndex])
          && Rsp[nIndex] != ';') {
        Rsp.remove(nIndex, 1);
        continue;
      }
      nIndex++;
    }
    m_LastReadWrite = 0;
    return Rsp;
  }

public:
  void attention(void) {
    String sCmd(';');
    write(sCmd);
  }

  virtual void        setFrequency(uint64_t ullFrequencyHz) = 0;
  virtual bool        isHardrockConnected(void) = 0;
  virtual unsigned    getFrequencyBand(void) = 0;
  virtual void        setFrequencyBand(unsigned uBand) = 0;
  virtual void        setFrequencyBand(unsigned long ulFrequency100MHz, unsigned long ulFrequencyHz) = 0;
  virtual int         getKeyingMode(void) = 0;
  virtual void        setKeyingMode(bool bPTTOn) = 0;
  virtual float       getCurrentPowerVSWR(char chWhich) = 0;
  virtual bool        getStatus(unsigned& ruFwdPower,
                                unsigned& ruReflectedPower,
                                unsigned& ruDrivePower,
                                unsigned& ruSWR,
                                unsigned& ruDrainVoltage,
                                unsigned& ruDrainCurrent,
                                unsigned& ruTemperature) = 0;
  virtual String      getTemperature(void) = 0;
  virtual bool        isTemperatureScaleCelsius(void) = 0;
  virtual void        setTemperatureScaleCelsius(bool bCelsius) = 0;
  virtual float       getDCInputVoltage(void) = 0;
  virtual bool        isATUPresent(void) = 0;
  virtual void        Tune(void) = 0;
  virtual bool        isTuning(void) = 0;
  virtual unsigned    getActiveAntenna(void) = 0;
  virtual bool        SaveATUSettings(void) = 0;
  virtual bool        isTunerByPassed(void) = 0;
  virtual void        setTunerByPass(bool bByPass) = 0;
  virtual const char* Model(void) const = 0;

public:
  virtual bool Antenna1Enabled(void) {
    return true;
  }
  virtual bool Antenna2Enabled(void) {
    return false;
  }
  virtual bool isResponseExpected(const String& rsCmd) {
    return isHardrockPacket(rsCmd)
           && (rsCmd.indexOf(';') == 4
               || rsCmd[rsCmd.length() - 2] == '?')
           && toupper(rsCmd[0]) == 'H'
           && toupper(rsCmd[1]) == 'R';
  }

#if defined USE_THREADS
public:
  operator Threads::Mutex&() {
    return m_Mutex;
  }

protected:
  class autolock {
  public:
    autolock(Threads::Mutex& rMutex)
      : m_rMutex(rMutex) {
      rMutex.lock();
    }
    ~autolock() {
      m_rMutex.unlock();
    }
  private:
    Threads::Mutex& m_rMutex;
  };

protected:
  Threads::Mutex m_Mutex;
#else
protected:
  class autolock {
  public:
    autolock(CHardrock&) {
    }
    ~autolock() {
    }
  };
#endif

private:
  const uint32_t m_IntercommandPeriod;
  elapsedMillis  m_LastReadWrite;
};
#endif
