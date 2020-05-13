#include <vector>
#include <stdint.h>

class OmEepromField
{
public:
  const char *name = 0;
  uint8_t type = 0; // 0 string, 1 int.
  uint8_t length = 0; // string container size (1 more than max length) or int size 1,2,4,8.
  int offset = 0;
};

class OmEeprom
{
public:
  OmEeprom(const char *signature);
  void addField(const char *fieldName, uint8_t type, uint8_t length);

  void begin();

  bool get(const char *fieldName, void *valueOut);
  bool put(const char *fieldName, const void *value);
  bool commit();

  /// To help debug.
  void dumpState();

private:
  int signatureSize;
  const char *signature;
  
  std::vector<OmEepromField> fields;
  bool didBegin = false;

  int dataSize = 0; // sum of signature & fields.
  uint8_t *data = 0; // malloc'd by fields.
  OmEepromField *findField(const char *fieldName);
};

