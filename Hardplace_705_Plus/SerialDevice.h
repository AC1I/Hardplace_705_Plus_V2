#if !defined SERIALDEVICE_H_DEFINED
#define SERIALDEVICE_H_DEFINED

#include <Arduino.h>
#include <cstdint>
#include <HardwareSerial.h>
#include <usb_serial.h>

#include "Hardplace705Plus.h"
#include "HardplaceUSBHost.h"

#include "Tracer.h"

#if !defined USB_DUAL_SERIAL && !defined USB_TRIPLE_SERIAL
#define SerialUSB1 Serial
typedef usb_serial_class usb_serial2_class;
#endif
#if !defined USB_TRIPLE_SERIAL
#define SerialUSB2 Serial
typedef usb_serial_class usb_serial3_class;
#endif

class CSerialStream : public Stream {
protected:
  CSerialStream(
    Stream& rStream, uint32_t uBaudrate, uint16_t uFormat = SERIAL_8N1,
    unsigned long ulTimeout = 1000)
    : m_Type(streamType(rStream)),
      m_rStream(rStream),
      m_rHsDevice(HardwareSerialDevice(rStream)),
      m_rUsb1Device(Serial),
      m_rUsb2Device(SerialUSB1),
      m_rUsb3Device(SerialUSB2),
      m_rUsbHostDevice((streamType(rStream) == USBSerialHostType) ? static_cast<USBSerialBase&>(rStream) : SerialUSBHost1),
      m_uBaudrate(uBaudrate),
      m_uFormat(uFormat) {
    setTimeout(ulTimeout);
  }
  CSerialStream(const CSerialStream& rhs)
    : m_Type(rhs.m_Type), m_rStream(rhs.m_rStream), m_rHsDevice(rhs.m_rHsDevice),
      m_rUsb1Device(rhs.m_rUsb1Device), m_rUsb2Device(rhs.m_rUsb2Device), m_rUsb3Device(rhs.m_rUsb3Device),
      m_rUsbHostDevice(rhs.m_rUsbHostDevice), m_uBaudrate(rhs.m_uBaudrate), m_uFormat(rhs.m_uFormat) {
  }
  virtual ~CSerialStream() {}

public:
  virtual void begin(void) {
    begin(m_uBaudrate, m_uFormat);
  }
  virtual void begin(uint32_t baud, uint16_t format = SERIAL_8N1) {
    if (m_Type == HardwareSerialDeviceType) {
      m_rHsDevice.begin(baud, format);
    } else if (m_Type == USBSerial1DeviceType) {
      m_rUsb1Device.begin(baud);
    } else if (m_Type == USBSerial2DeviceType) {
      m_rUsb2Device.begin(baud);
    } else if (m_Type == USBSerial3DeviceType) {
      m_rUsb3Device.begin(baud);
    } else if (m_Type == USBSerialHostType) {
      m_rUsbHostDevice.begin(baud, format);
    }
  }
  virtual void end(void) {
    if (m_Type == HardwareSerialDeviceType) {
      m_rHsDevice.end();
    } else if (m_Type == USBSerial1DeviceType) {
      m_rUsb1Device.end();
    } else if (m_Type == USBSerial2DeviceType) {
      m_rUsb2Device.end();
    } else if (m_Type == USBSerial3DeviceType) {
      m_rUsb3Device.end();
    } else if (m_Type == USBSerialHostType) {
      m_rUsbHostDevice.end();
    }
  }

  virtual int available(void) {
    return m_rStream.available();
  }
  virtual int read(void) {
    return m_rStream.read();
  }
  virtual int peek(void) {
    return m_rStream.peek();
  }
  virtual void setTimeout(unsigned long ulTimeout) {
    Stream::setTimeout(ulTimeout);
    m_rStream.setTimeout(ulTimeout);
  }
  virtual unsigned long getTimeout(void) const {
    return _timeout;
  }
  // Note: This one buffers the terminator unlike the original
  virtual size_t readBytesUntil(uint8_t terminator, uint8_t* buffer, size_t length) {
    unsigned long ulTimeout(getTimeout());
    bool          fTerminatorFound(false);
    size_t        cBytes(0);

    for (elapsedMillis now(0); (!fTerminatorFound && now < ulTimeout); Delay(1)) {
      while (!fTerminatorFound
             && available() > 0) {
        uint8_t uchRead(static_cast<uint8_t>(read()));
        buffer[cBytes++] = uchRead;
        fTerminatorFound = (uchRead == terminator) || (cBytes == length);
      }
    }
    return cBytes;
  }
  // Note: This one buffers the terminator unlike the original
  virtual String readStringUntil(char terminator) {
    String        sResponse;
    unsigned long ulTimeout(getTimeout());
    bool          fTerminatorFound(false);

    for (elapsedMillis now(0); (!fTerminatorFound && now < ulTimeout); Delay(1)) {
      while (!fTerminatorFound
             && available() > 0) {
        char chRead(static_cast<char>(read()));
        sResponse += chRead;
        fTerminatorFound = (chRead == terminator);
      }
    }
    return sResponse;
  }

public:
  virtual void transmitterEnable(uint8_t pin) {
    if (m_Type == HardwareSerialDeviceType) {
      m_rHsDevice.transmitterEnable(pin);
    }
  }
  virtual void setRX(uint8_t pin) {
    if (m_Type == HardwareSerialDeviceType) {
      m_rHsDevice.setRX(pin);
    }
  }
  virtual void setTX(uint8_t pin, bool opendrain = false) {
    if (m_Type == HardwareSerialDeviceType) {
      m_rHsDevice.setTX(pin, opendrain);
    }
  }
  virtual bool attachRts(uint8_t pin) {
    bool fReturn(false);
    if (m_Type == HardwareSerialDeviceType) {
      fReturn = m_rHsDevice.attachRts(pin);
    }
    return fReturn;
  }
  virtual bool attachCts(uint8_t pin) {
    bool fReturn(false);
    if (m_Type == HardwareSerialDeviceType) {
      fReturn = m_rHsDevice.attachCts(pin);
    }
    return fReturn;
  }
  virtual void clear(void) {
    if (m_Type == HardwareSerialDeviceType) {
      m_rHsDevice.clear();
    } else if (m_Type == USBSerial1DeviceType) {
      m_rUsb1Device.clear();
    } else if (m_Type == USBSerial2DeviceType) {
      m_rUsb2Device.clear();
    } else if (m_Type == USBSerial3DeviceType) {
      m_rUsb3Device.clear();
    } else {
      while (available()) {
        read();
      }
    }
  }
  virtual int availableForWrite(void) {
    return m_rStream.availableForWrite();
  }
  virtual void addMemoryForRead(void* buffer, size_t length) {
    if (m_Type == HardwareSerialDeviceType) {
      m_rHsDevice.addMemoryForRead(buffer, length);
    } else {
      CTraceDevice().TraceLn("CSerialStream::addMemoryForRead() Not supported for this device");
    }
  }
  virtual void addMemoryForWrite(void* buffer, size_t length) {
    if (m_Type == HardwareSerialDeviceType) {
      m_rHsDevice.addMemoryForWrite(buffer, length);
    } else {
      CTraceDevice().TraceLn("CSerialStream::addMemoryForWrite() Not supported for this device");
    }
  }
  virtual size_t write9bit(uint32_t c) {
    size_t stReturn(0);
    if (m_Type == HardwareSerialDeviceType) {
      stReturn = m_rHsDevice.write9bit(c);
    }
    return stReturn;
  }
  virtual void flush(void) {
    m_rStream.flush();
  }
  virtual size_t write(uint8_t c) {
    return m_rStream.write(c);
  }
  using Print::write;
  virtual size_t write(unsigned long n) {
    return m_rStream.write(n);
  }
  virtual size_t write(long n) {
    return m_rStream.write(n);
  }
  virtual size_t write(unsigned int n) {
    return m_rStream.write(n);
  }
  virtual size_t write(int n) {
    return m_rStream.write(n);
  }
  virtual size_t write(const String& rCmd) {
    return write(reinterpret_cast<const uint8_t*>(rCmd.c_str()), static_cast<size_t>(rCmd.length()));
  }


  operator bool() {
    bool fReturn(false);
    if (m_Type == HardwareSerialDeviceType) {
      fReturn = m_rHsDevice;
    } else if (m_Type == USBSerial1DeviceType) {
      fReturn = m_rUsb1Device;
    } else if (m_Type == USBSerial2DeviceType) {
      fReturn = m_rUsb2Device;
    } else if (m_Type == USBSerial3DeviceType) {
      fReturn = m_rUsb3Device;
    } else if (m_Type == USBSerialHostType) {
      fReturn = m_rUsbHostDevice;
    }
    return fReturn;
  }

public:
  virtual void setBaudrate(uint32_t uBaudrate) {
    m_uBaudrate = uBaudrate;
    begin();
  }
  virtual uint32_t getBaudrate(void) const {
    return m_uBaudrate;
  }

public:
  virtual void Task(void) {
    if (available() > 0) {
      onAvailable();
    }
  }
  virtual void onAvailable(void) {
  }

public:
  String getTypeString(void) const {
    String sType;
    switch (getType()) {
      case HardwareSerialDeviceType:
        sType = "HardwareSerialDeviceType";
        break;

      case USBSerial1DeviceType:
      case USBSerial2DeviceType:
      case USBSerial3DeviceType:
        sType = "USBSerialDeviceType";
        break;

      case USBSerialHostType:
        sType = "USBSerialHostType";
        break;

      default:
        sType = "UnknownDeviceType";
        break;
    }
    return sType;
  }

  virtual String deviceName(void) const {
    String sDeviceName("Unknown");
    if (&m_rStream == static_cast<const Stream*>(&Serial1)) {
      sDeviceName = "Serial1";
    } else if (&m_rStream == static_cast<const Stream*>(&Serial2)) {
      sDeviceName = "Serial2";
    } else if (&m_rStream == static_cast<const Stream*>(&Serial3)) {
      sDeviceName = "Serial3";
    } else if (&m_rStream == static_cast<const Stream*>(&Serial4)) {
      sDeviceName = "Serial4";
    } else if (&m_rStream == static_cast<const Stream*>(&Serial5)) {
      sDeviceName = "Serial5";
    } else if (&m_rStream == static_cast<const Stream*>(&Serial6)) {
      sDeviceName = "Serial6";
    } else if (&m_rStream == static_cast<const Stream*>(&Serial7)) {
      sDeviceName = "Serial7";
    } else if (&m_rStream == static_cast<const Stream*>(&Serial8)) {
      sDeviceName = "Serial8";
    } else if (&m_rStream == static_cast<const Stream*>(&Serial)) {
      sDeviceName = "Serial";
    } else if (&m_rStream == static_cast<const Stream*>(&SerialUSB1)) {
      sDeviceName = "SerialUSB1";
    } else if (&m_rStream == static_cast<const Stream*>(&SerialUSB2)) {
      sDeviceName = "SerialUSB2";
    } else if (&m_rStream == static_cast<const Stream*>(&SerialUSBHost1)) {
      sDeviceName = "SerialUSBHost1";
    } else if (&m_rStream == static_cast<const Stream*>(&SerialUSBHost2)) {
      sDeviceName = "SerialUSBHost2";
    } else if (&m_rStream == static_cast<const Stream*>(&SerialUSBHost3)) {
      sDeviceName = "SerialUSBHost3";
    } else if (&m_rStream == static_cast<const Stream*>(&SerialUSBHost4)) {
      sDeviceName = "SerialUSBHost4";
    }
    return sDeviceName;
  }

protected:
  enum Type {
    HardwareSerialDeviceType,
    USBSerial1DeviceType,
    USBSerial2DeviceType,
    USBSerial3DeviceType,
    USBSerialHostType,
    UnknownDeviceType
  };

  Type getType(void) const {
    return m_Type;
  }

private:
  static CSerialStream::Type streamType(const Stream& rStream) {  // -fno-rtti ;-(
    Type StreamType(UnknownDeviceType);
    if (&rStream == static_cast<const Stream*>(&Serial1)
        || &rStream == static_cast<const Stream*>(&Serial2)
        || &rStream == static_cast<const Stream*>(&Serial3)
        || &rStream == static_cast<const Stream*>(&Serial4)
        || &rStream == static_cast<const Stream*>(&Serial5)
        || &rStream == static_cast<const Stream*>(&Serial6)
        || &rStream == static_cast<const Stream*>(&Serial7)
        || &rStream == static_cast<const Stream*>(&Serial8)) {
      StreamType = HardwareSerialDeviceType;
    } else if (&rStream == static_cast<const Stream*>(&Serial)) {
      StreamType = USBSerial1DeviceType;
    } else if (&rStream == static_cast<const Stream*>(&SerialUSB1)) {
      StreamType = USBSerial2DeviceType;
    } else if (&rStream == static_cast<const Stream*>(&SerialUSB2)) {
      StreamType = USBSerial3DeviceType;
    } else if (&rStream == static_cast<const Stream*>(&SerialUSBHost1)
               || &rStream == static_cast<const Stream*>(&SerialUSBHost2)
               || &rStream == static_cast<const Stream*>(&SerialUSBHost3)
               || &rStream == static_cast<const Stream*>(&SerialUSBHost4)) {
      StreamType = USBSerialHostType;
    } else {
      StreamType = UnknownDeviceType;
    }
    return StreamType;
  }

  static HardwareSerial& HardwareSerialDevice(const Stream& rStream) {
    return (streamType(rStream) == HardwareSerialDeviceType)
             ? static_cast<HardwareSerial&>(const_cast<Stream&>(rStream))
             : Serial1;
  }

private:
  Type               m_Type;
  Stream&            m_rStream;
  HardwareSerial&    m_rHsDevice;
  usb_serial_class&  m_rUsb1Device;
  usb_serial2_class& m_rUsb2Device;
  usb_serial3_class& m_rUsb3Device;
  USBSerialBase&     m_rUsbHostDevice;
  uint32_t           m_uBaudrate;
  uint16_t           m_uFormat;
};

#undef SerialUSB1
#undef SerialUSB2

class CSerialDevice : public CSerialStream {
public:
  CSerialDevice(
    Stream& rDevice, uint32_t uBaudrate = 38400, unsigned long ulTimeout = 1000)
    : CSerialStream(rDevice, uBaudrate, SERIAL_8N1, ulTimeout) {
  }
  CSerialDevice(const CSerialDevice& rDevice)
    : CSerialStream(rDevice) {}
  virtual ~CSerialDevice() {}

private:
  CSerialDevice();
  CSerialDevice& operator=(const CSerialDevice&);

public:
  CTraceDevice& Tracer(void) {
    return m_Tracer;
  }
  operator CTraceDevice&() {
    return m_Tracer;
  }
  operator CTraceDevice*() {
    return &m_Tracer;
  }

protected:
  CTraceDevice m_Tracer;
};

#endif
