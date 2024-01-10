#if !defined BLUETOOTH_H_DEFINED
#define BLUETOOTH_H_DEFINED

#include <Arduino.h>
#include <elapsedMillis.h>

#include "Hardplace705Plus.h"
#include "SerialDevice.h"
#include "Teensy41.h"
#include "Tracer.h"

class CBluetoothDevice : public CSerialDevice {
public:
  CBluetoothDevice(
    IBluetooth::TeensyBluetooth DeviceID, IBluetooth& rBluetooth, Stream& Device, uint32_t uBaudrate = 38400,
    unsigned long ulTimeout = 1000)
    : CSerialDevice(Device, uBaudrate, ulTimeout),
      m_DeviceID(DeviceID), m_rBluetooth(rBluetooth) {
  }
private:
  CBluetoothDevice();
  CBluetoothDevice(const CBluetoothDevice&);
  CBluetoothDevice& operator=(const CBluetoothDevice&);

public:
  virtual bool setup(void) {
    return true;
  }
  virtual void Task(void) {
    CSerialDevice::Task();
    //      setup();
    //      ensureAvailablility();
  }

  virtual bool isReady(void) {
    return true;
  }

  virtual bool reset(void) {
    return true;
  }
  virtual bool   discoverBaudrate(void) = 0;
  virtual String deviceName(void) const = 0;
  virtual void   ensureAvailablility(void) = 0;

public:
  virtual void PowerOn(void) {
    m_rBluetooth.BluetoothPowerOn(m_DeviceID);
    Delay(100);
  }
  virtual void PowerOff(void) {
    m_rBluetooth.BluetoothPowerOff(m_DeviceID);
  }
  virtual bool isPoweredOn(void) {
    return m_rBluetooth.isBluetoothPowerOn(m_DeviceID);
  }

protected:
  IBluetooth::TeensyBluetooth m_DeviceID;
  IBluetooth&                 m_rBluetooth;
};
#endif
