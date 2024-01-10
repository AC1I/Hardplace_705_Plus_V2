#if !defined COMMANDPROCESSOR_H_DEFINED
#define COMMANDPROCESSOR_H_DEFINED

#include <HashMap.h>

#include "SerialDevice.h"

typedef void (*CommandHandler)(
  void* pthis, const String& rsString, CSerialDevice& rSrcDevice);

class HashString : public String {
public:
  HashString()
    : String() {}
  HashString(const HashString& rhs)
    : String(rhs) {}
  HashString(const char* pszString)
    : String(pszString) {}
  virtual ~HashString() {}

public:
  HashString& operator=(const HashString& rhs) {
    static_cast<String&>(*this) = rhs;
    return *this;
  }
  HashString& operator=(int) {  // The reason for being, to statisfy Hashmap
    remove(0);
    return *this;
  }
};

class CCommandProcessor : public HashMap<HashString, CommandHandler> {
public:
  CCommandProcessor(size_t cEntries = 0)
    : HashMap<HashString, CommandHandler>(
      new HashType<HashString, CommandHandler>[cEntries], cEntries) {
  }
  virtual ~CCommandProcessor() {
    delete[] hashMap;
  }


private:
  CCommandProcessor(const CCommandProcessor&);
  CCommandProcessor& operator=(const CCommandProcessor&);
};
#endif