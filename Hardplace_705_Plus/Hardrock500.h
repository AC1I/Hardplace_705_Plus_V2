#if !defined HARDROCK500_H_DEFINED
#define HARDROCK500_H_DEFINED

#include <elapsedMillis.h>

#include "Hardplace705Plus.h"
#include "Hardrock.h"

/*
  SERIAL COMMANDS
  The following commands are recognized by the Hardrock-500 amplifier:
    FA   – VFO A Frequency
    HRAA – Acknowledge Message
    HRBN – Set the HR500’s frequency band
    HRFW – Forces a reset and activates the bootloader to accept new firmware
    HRMD – Set the HR500’s keying mode
    HRPW – Read the HR500’s FWD, RFL, Drive power or SWR
    HRST – Reports the complete operating status of the HR500
    HRTM – Pass data to ATU
    HRTP – Read the heatsink temperature
    HRTS – Set the temper display to ‘F’ or ‘C’
    HRVT – Read HR500’s MOSFET drain voltage, VDD
    IF   – General information

  Command Format
  Commands sent from the computer to the HR500 are considered either GETs or SETs. GET commands are used by the
  computer to get information from the HR500; the HR500 will then provide an appropriate response message (RSP). SET
  commands are sent by the computer to change the amplifier's configuration. A SET can be followed by a GET to verify
  the new settings.
  SET commands use 2 or 4 characters, optional data fields, and a terminating semicolon (;). Examples:

    HRMD0; Computer sets mode to ‘OFF’
    HRBN7; Computer selects 40M band
  Many SET commands have a corresponding GET command, which is just the command letters with no data. The
  data format of the response message from the HR500 (RSP) is usually identical to the format of the SET data.
  Exceptions are noted in the command descriptions.

  Characters sent to the HR50 can use either upper or lower case. The HR50 will always respond with upper case.

  ************************************************************************************************************************
  IF / FA (Frequency; SET)
  SET format: FAxxxxxxxxxxx; where xxxxxxxxxxx is the frequency in Hz.
                12345678901 12,345,678,901
  Example:
  FA00014060000; sets the HR50 to 14060 kHz.

  The Hz, 10Hz and 100Hz digits are ignored by the HR50 for calculating the band and ATU frequency segment. If the
  specified frequency is in a different amateur band than the present one, the HR50 will change to the new band.

  ************************************************************************************************************************
  HRAA (Acknowledge Message; GET)
  RSP format: HRAA; a simple response to indicate that the amp is present and listening on the port
  Example: HRAA; the HR500 responds HRAA;

  ************************************************************************************************************************
  HRBN (HR500 frequency Band; SET/GET)
  SET/RSP format: HRBNxx; where xx represents desired band, leading 0 not required:
    1 = 10M 6 = 30M
    2 = 12M 7 = 40M
    3 = 17M 8 = 60M
    4 = 15M 9 = 80M
    5 = 20M 10 = 160M
    other = Unknown band (reports 99)

  Example: HRBN7; sets the band to 40M

  If the HR500 is setup to use a transceiver that uses a band voltage like the FT-817 or Xiegu transceivers, the band will
  immediately change back to the band asserted by the transceiver since the HR500 is reading the band control voltage
  form the 3 or 4 times a second.

  ************************************************************************************************************************
  HRFW (Commands the HR500 to reset and listen for the bootloader; SET)
  SET format: HRBFW;
  This command is automatically send by the firmware loading app on the PC and does not have an operational function.

  ************************************************************************************************************************
  HRMD (HR500 change keying mode; SET/GET)
  SET/RSP format: HRMDx; where x = 0 (OFF), 1 (PTT)

  Example HRMD1; sets the HR500 keying mode to PTT

  ************************************************************************************************************************
  HRPWx (Get data current power/VSWR readings)
  SET/RSP format: HRPWx; where x = F, R, D, or V
  Responds with the current forward power, reflected power, drive power or VSWR depending on ‘x’.

  Values of ‘x’ are:
    • F – reads forward power
    • R – reads reflected power
    • D – reads drive power
    • V – reads VSWR (if the power is not sufficient for and accurate reading 0.0 will be returned)

  Example HRPWF; response: HRPWF350; – the amp is producing 350 watts of forward power

  ************************************************************************************************************************
  HRST (Get Hardrock-500 status)
  RSP format: HRST-fff-rrr-ddd-sss-vvv-iii-ttt-
  Responds with all current operating parameters.

    • fff – forward power in watts
    • rrr – reflected power in watts
    • ddd – drive power in 1/10th watt
    • sss – SWR in 1/10th ratio
    • vvv – Drain voltage in volts
    • iii – drain current in amps
    • ttt – temperature in °C or °F depending on setting

  ************************************************************************************************************************
  HRTM; (HR50 ATU Message/Response)
  SET/RSP format HRTM<tuner message>;
  Acts as pass through to communicate with the ATU.

  Example HRTMC?; requests the current capacitor setting in the ATU.
  HR50 responds HRTM 8; the ATU capacitor is set to step 8 out of 127

  Tuner Messages:

    A         Initiate an autotune sequence
    Bxx, B?   Set/query band setting uses bands listed above except UNK = 11
    Cxxx, C?  Set/query capacitor step value (0 - 127)
    E         Commands tuner to erase all stored tuning solutions
    Fxxx, F?  Set/query ATU memory channel
    Hx, H?    Set/query capacitor position 0 = input, 1 = output
    I         Query ATU information responds ‘HRTMHR50 ATU version x.x’
    Lxxx, L?  Set/query inductor step value (0 – 127)
    Px Query  RF power reading x = F (average fwd), R (average reverse), P (PEP forward)
    Q         Store the currently applies tuning solution in the current freq/band memory
    S         Query the status of the last autotune cycle S (success), F (fail), L (power too low), A (abort)
    V         Query the ATU firmware revision responds ‘HRTMx.x;’
    W         Query SWR reading responds 1.0 – 9.9 or --- if the power is too low
    Yx, Y?    Set/query ATU bypass 0 = engaged, 1 = bypassed

  ************************************************************************************************************************
  HRTP; (HR50 read temperature)
  RSP format HRTPnx; n = temperature value; x = F (Fahrenheit) or x = C (Celsius)

  Reads the temperature of the heat spreader in °C or °F depending on setting

  ************************************************************************************************************************
  HRTS; (HR50 read temperature/set temperature scale)
  SET format HRTSx; x = F (Fahrenheit) or x = C (Celsius)
  RSP format HRTSx; x = F (Fahrenheit) or x = C (Celsius)

  Example HRTSC; sets the temperature scale to Celsius

  ************************************************************************************************************************
  HRVT; (HR50 read DC input voltage)
  RSP format HRVTxx.xV; where xx.x is the DC input voltage.
*/
/*
   IF00014060000;
   HRAA;HRAA;<CR><LF>
   HRBN;HRBN5;<CR><LF>
   HRBN7;
   HRMD;HRMD0;<CR><LF>
   HRMD1;
   HRMD0;
   HRPWF;HRPWF000;<CR><LF>
   HRPWR;HRPWR000;<CR><LF>
   HRPWD;HRPWD000;<CR><LF>
   HRPWV;HRPWV000;<CR><LF>
   HRST;HRST-000-000-000-000-058-000-020-;<CR><LF>
   HRTMV;HRTM1.1C;<CR><LF>
   HRTP;HRTP69F;<CR><LF>
   HRTS;HRTSF;<CR><LF>
   HRTSF;
   HRVT;HRVT58;<CR><LF>
*/

class CHardrock500 : public CHardrock {
public:
  CHardrock500(CSerialDevice& rDevice)
    : CHardrock(rDevice),
      m_Interval(0), m_fConnected(false), m_fLastConnState(false), m_FreqSupported(false) {
  }
  virtual ~CHardrock500() {
  }

private:
  CHardrock500();
  CHardrock500(const CHardrock500&);
  CHardrock500& operator=(const CHardrock500&);

public:
  virtual const char* Model(void) const {
    return modelName();
  }
  static const char* modelName(void);

public:
  virtual void setFrequency(uint64_t ullFrequencyHz) {
    CHardrock::autolock(*this);
    m_FreqSupported = ullFrequencyHz < 30000000;
    if (availableForWrite()) {
      char achBuf[32];
      sprintf(achBuf, "FA%011llu;", ullFrequencyHz);
      String sCmd(achBuf);
      write(sCmd);
    }
  }

  bool isHardrockConnected(void) {
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

  unsigned getFrequencyBand(void) {
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

  void setFrequencyBand(unsigned uBand) {
    // HRBN7;
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRBN");
      Cmd += uBand;
      Cmd += ';';
      write(Cmd);
    }
  }

  void setFrequencyBand(unsigned long ulFrequency100MHz, unsigned long ulFrequencyHz) {
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

  int getKeyingMode(void) {
    // HRMD;HRMD0;<CR><LF>
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRMD;");
      write(Cmd);
      String Rsp(readString());
      if (isValidResponse(Rsp, Cmd)
          && (Rsp.charAt(4) == '0' || Rsp.charAt(4) == '1')) {
        return Rsp.charAt(4) - '0';
      }
    }
    return -1;
  }

  void setKeyingMode(bool bPTTOn) {
    // HRMD1;
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRMD");
      Cmd += (bPTTOn) ? "1" : "0";
      Cmd += ";";
      write(Cmd);
    }
  }

  float getCurrentPowerVSWR(char chWhich) {
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

  bool getStatus(unsigned& ruFwdPower,
                 unsigned& ruReflectedPower,
                 unsigned& ruDrivePower,
                 unsigned& ruSWR,
                 unsigned& ruDrainVoltage,
                 unsigned& ruDrainCurrent,
                 unsigned& ruTemperature) {
    // HRST;HRST-000-000-000-000-058-000-020-;<CR><LF>
    return false;
  }

  String getTemperature(void) {
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

  bool isTemperatureScaleCelsius(void) {
    // HRTS;HRTSF;<CR><LF>
    String sTemp(getTemperature());
    return sTemp.charAt(sTemp.length() - 2) == 'C';
  }

  void setTemperatureScaleCelsius(bool bCelsius) {
    // HRTSC;
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String sCmd(String("HRTS") + (bCelsius) ? 'C' : 'F' + ';');
      write(sCmd);
    }
  }

  float getDCInputVoltage(void) {
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

  bool isATUPresent(void) {
    // HRAP:HRAP1;/HRAP0;
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRAP;");
      write(Cmd);
      String Rsp(readString());
      return (isValidResponse(Rsp, Cmd)
              && Rsp.charAt(4) == '1');
    }
    return false;
  }

  void Tune(void) {
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRTU;");
      write(Cmd);
    }
  }

  bool isTuning(void) {
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRTT;");
      write(Cmd);
      String Rsp(readString());
      return (isValidResponse(Rsp, Cmd)
              && Rsp.charAt(4) == '1');
    }
    return false;
  }

  unsigned getActiveAntenna(void) {
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRAN;");
      write(Cmd);
      String Rsp(readString());
      if (isValidResponse(Rsp, Cmd)
          && (Rsp.charAt(4) == '1' || Rsp.charAt(4) == '2')) {
        return Rsp.charAt(4) - '0';
      }
    }
    return 0;
  }

  bool SaveATUSettings(void) {
    CHardrock::autolock(*this);
    if (availableForWrite()) {
      String Cmd("HRTMQ;");
      return (write(Cmd) == Cmd.length());
    }
    return false;
  }

  /*
       HRTB (HR500 Tuner Mode; SET/GET)
       SET/RSP format: HRTBx; where x represents desired ATU mode:
              0 = Bypass the ATU   1  = Activate ATU
       Example: HRTB0; bypasses the ATU
    */
  bool isTunerByPassed(void) {
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

  void setTunerByPass(bool bByPass) {
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

public:
  bool Antenna1Enabled(void) {
    return true;  // m_UserConfig.Antenna1Enabled();
  }
  bool Antenna2Enabled(void) {
    return true;  // m_UserConfig.Antenna2Enabled();
  }

private:
  elapsedMillis m_Interval;
  bool          m_fConnected;
  bool          m_fLastConnState;
  bool          m_FreqSupported;
};
#endif
