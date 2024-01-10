#if !defined IC705_H_DEFINED
#define IC705_H_DEFINED

/*
   https://www.fbnews.jp/202007/ww02/index.html

   Key,   Tip
   Start, Ring
   Gnd,   Shield

   https://forums.qrz.com/index.php?threads/push2tune-for-the-icom-ic-7300.596410/

   1. Key~ (the tilde ~ means that the signal is active-low)
   2. Start~
   3. Switched 13.6V power.
   4. Grd (chassis ground).

   Key, when the 'TUNE' button is pressed, it's pulled low by the tuner to tell the radio to start transmitting.

   Start, to/from the microprocessor.
   Under normal circumstances the START pin is pulled high when a remote tuner is attached to tell the radio that the tuner is present.
   When you press 'TUNE' the radio pulls the pin low telling the tuner to start the matching sequence.

   These findings are specific to the IC-7300; I haven't tested other similar ICOM radios. If you have one, you will have to repeat the tests for your radio.

   With nothing connected to the Start~ pin, it sits low (about 50mV). In this state, the front-panel
   TUNER key, the TUNE and TX icons on the LCD screen work to control the Internal Antenna tuner\
   as described in the manual. Furthermore, with Start~ floating (low), the radio ignores the Key~ pin.

   If Start~ is pulled to about 5V by an external voltage through a suitable resistor, then the IC-7300
   detects this, and begins "listening" to the Key~ pin, which is important to the rest of this discussion.

   The Start~ pin is actually bi-directional (wired-or), meaning that when pulled-up by an external resistor,
   the radio can still momentary pulse the Start~ pin low to signal an external device. This is how the radio
   normally starts the tuning process if an AH-4 remote tuner is plugged into the radio. This happens when
   the TUNER key is tapped. This is as documented in the manual in the event the radio is used with the ICOM
   remote tuner. I am not using this feature of the interface in this design.

   The Key~ pin is pulled high (about 3.3V) by an internal-to-the- IC7300 pull-up.

   With radio having been told to listen to Key~ (by externally pulling-up Start~), then if you wait a while
   while the radio discovers this, then pull the Key~ pin to ground, then the radio responds by transmitting
   the 10W carrier, regardless of mode. This happens for as long as Key~ is low, and stops when Key~ is released.
   The radio generates a tone, and shows what is happening by animating the TUNE and TX screen icons.
   So, in summary, here is what I learned:
   In order to preserve the option of using the internal antenna tuner, you cannot permanently just tie Start~ high,
   because that disables the internal tuner. Rather, with the "Push2Tune" button, you have to first generate
   the Start~ pull-up signal, wait about 70ms for the radio to discover it, and then pull Key~ low for the tuning duration.
   Upon releasing the "Push2Tune" button, you simultaneously remove the pull-up from Start~ and allow Key~ to float,
   so the radio reverts to the mode where the internal tuner can be used.
   If the rising edge of Start~ is concurrent with the down-edge of Key~, the radio ignores them.

   !!!! Check that tune signal power follows settings, reduce power if required !!!
    The other thing is the tune signal makes the ICOM output a 10W carrier to tune with which is actually too high for the ATU in the Hardrock,
    it only needs 1-2W to tune and 5W is the maximum so I can't really make use of that feature either.

    Note: Checked, Icom indeed transmits 10W carrier even if Max Power is set lower.

    Note: AH-705 uses CI-V on tuning jack (Send/Ring). 1200 baud 8-N-1 Sends <Start~> FE FE 00 00 FD
*/

#include "Hardplace705Plus.h"
#include "Teensy41.h"
#include "IC_705Master.h"
#include "ICOM.h"
#include "Hardrock.h"
#include "Tracer.h"


class CIC_705Tuner {
public:
  CIC_705Tuner(
    ITuner& rTuner, CIC_705MasterDevice& r705, CHardrock& rHardrock)
    : m_fTuning(false),
      m_r705(r705),
      m_rHardrock(rHardrock),
      m_rTuner(rTuner) {
  }
private:
  CIC_705Tuner();
  CIC_705Tuner(const CIC_705Tuner&);
  CIC_705Tuner& operator=(const CIC_705Tuner&);

public:
  void setup(void) {
  }
  void Task(void) {
    do {
      m_rTuner.TunerUpdate();
    } while (TuneOnCmd());
  }

public:
  bool Tuning(void) const {
    return m_fTuning;
  }

  bool TuneOnCmd(void) {
    if (m_rTuner.Tune()) {  // Start~ command from IC-705
      m_rTuner.Tuning(true);
      if (TuneEnabled()) {
        bool fHasAH705(false);
#if defined HAS_AH705_EMULATION
        do {
          CICOMReq Icom(m_r705.getRigAddress());
#if defined USE_THREADS
          Threads::Scope wait(m_r705);
#endif
          fHasAH705 = Icom.isTunerSelect_AH_705(m_r705);
        } while (0);
#endif
        if (fHasAH705) {  // Tuner set to AH-705, tune normally
          m_Tracer.TraceLn("Tuning Normally AH-705 mode");

          m_rHardrock.Tune();                           // Tell the Hardrock to tune. It takes somewhere between .5 and 1 second to switch over to tuning mode
          for (unsigned uTry = 0; uTry < 20; uTry++) {  // Spin awaiting switch over to tuning mode
            delay(50);
            if (m_rHardrock.isTuning()) {
              break;
            }
          }
          m_rTuner.TunerKey(true);                                                                  // Tell the IC-705 to send the tuning signal
          for (unsigned uInterval = 0; (m_rHardrock.isTuning() && uInterval < 200); uInterval++) {  // Wait for the Hardrock to complete tuning, Limit to 10 seconds
            delay(50);
          }
          if (m_rHardrock.isTuning()) {
            m_rHardrock.Tune();  // Cancel tune (Same command as tune, acts as a toggle)
          }

          m_rTuner.TunerKey(false);  // Tell the IC-705 to stop sending the tuning signal
        } else if (!m_fTuning) {     // The IC 705 transmits a 10 watt tuning signal
          m_fTuning = true;          // ... and the IC-705 won't let us change it
          m_Tracer.TraceLn("Tuning via Proxy, ICOM Phase");
          m_rTuner.TunerEnablePTT(false);  // Disable PTT
          m_rTuner.TunerKey(true);         // Tell the IC-705 to send the tuning signal
          delay(70);                       // Give the IC-705 a chance to act upon the command
          m_rTuner.TunerKey(false);        // Tell the IC-705 to stop sending the tuning signal
          m_rTuner.TunerEnablePTT(true);   // Enable PTT
        }
      } else {
        m_rTuner.TunerDisable();  // Never should get here, but if we do, tell the IC-705 we can't tune
      }
    } else if (m_fTuning  // We tune on the trailing edge. Where we can control things.
               && m_rTuner.TuneComplete()) {
      m_Tracer.TraceLn("Tuning via Proxy, Hardrock Phase");

      if (TuneEnabled()) {  // If the active antenna can be tuned
        unsigned char auchRespBuf[80];
        size_t        cRead;
        CICOMReq      Icom(m_r705.getRigAddress());
#if defined USE_THREADS
        Threads::Scope wait(m_r705);
#endif
        Icom.ReadModeFilter(m_r705);  // Cache the Mode and Filter
        if ((cRead = getResponse(auchRespBuf, sizeof auchRespBuf)) > 0) {
          CICOMResp readModeRsp(auchRespBuf, cRead);

          Icom.ReadRFPower(m_r705);  // Cache the RF Power Level
          if ((cRead = getResponse(auchRespBuf, sizeof auchRespBuf)) > 0) {
            CICOMResp readRFPowerRsp(auchRespBuf, cRead);

            // Under no circumstances proceed unless the power level is less than 5 watts!
            if (((readRFPowerRsp.RFPower() <= m_uTunePwrMax && readRFPowerRsp.RFPower() >= m_uTunePwrMin)
                 || Icom.WriteRFPower(m_r705, m_uTunePwr))
                && Icom.WriteModeFilter(m_r705, m_RTTY, m_uFilterWidthNormal)) {
              m_rHardrock.Tune();                           // It takes somewhere between .5 and 1 second to switch over to tuning mode
              for (unsigned uTry = 0; uTry < 20; uTry++) {  // Spin awaiting switch over to tuning mode
                delay(50);
                if (m_rHardrock.isTuning()) {
                  break;
                }
              }

              if (m_rHardrock.isTuning()) {  // If we are in tuning mode
                Icom.TX(m_r705, true);
                for (unsigned uInterval = 0; (m_rHardrock.isTuning() && uInterval < 200); uInterval++) {  // Limit to 10 seconds
                  delay(50);
                }
                if (m_rHardrock.isTuning()) {
                  m_rHardrock.Tune();  // Cancel tune (Same command as tune, acts as a toggle)
                }
                Icom.TX(m_r705, false);
              }
            }
            // Restore the Mode, Filter, and RF Power Level
            Icom.WriteModeFilter(m_r705, readModeRsp.Mode(), readModeRsp.Filter());
            Icom.WriteRFPower(m_r705, readRFPowerRsp.RFPower());
          }
        }
      } else {
        m_rTuner.TunerDisable();  // Never should get here, but if we do, tell the IC-705 we can't tune
      }
      m_fTuning = false;
    }
    
    m_rTuner.Tuning(m_fTuning);
    return m_fTuning;
  }

  bool TuneEnabled(void) {
    bool fAnt1Enabled(m_rHardrock.Antenna1Enabled());
    bool fAnt2Enabled(m_rHardrock.Antenna2Enabled());

    if (fAnt1Enabled || fAnt2Enabled) {
      const unsigned uActiveAntenna(m_rHardrock.getActiveAntenna());

      return ((uActiveAntenna == 1 && fAnt1Enabled)
              || (uActiveAntenna == 2 && fAnt2Enabled));
    }
    return false;
  }

protected:
  size_t getResponse(unsigned char* puchBuffer, size_t stBufLen) {
    size_t   cRead;
    CICOMReq Icom(m_r705.getRigAddress());

    for (cRead = Icom.getResponse(m_r705, puchBuffer, stBufLen);
         (cRead > 0
          && !CICOMResp(puchBuffer, cRead).isResponse());
         cRead = Icom.getResponse(m_r705, puchBuffer, stBufLen)) {
      ;
    }
    return cRead;
  }

private:
  static const unsigned m_uTunePwr;
  static const unsigned m_uTunePwrMax;
  static const unsigned m_uTunePwrMin;
  static const unsigned m_RTTY;
  static const unsigned m_uFilterWidthNormal;
  bool                  m_fTuning;
  CIC_705MasterDevice&  m_r705;
  CHardrock&            m_rHardrock;
  ITuner&               m_rTuner;
  CTraceDevice          m_Tracer;
};
#endif
