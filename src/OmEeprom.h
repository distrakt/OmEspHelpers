#include <vector>
#include <string>
#include <stdint.h>

#ifndef OMEEPROM_H
#define OMEEPROM_H

#if NOT_ARDUINO
#include <string>
#define String std::string
#else
#include "Arduino.h"
#endif

typedef enum
{
    OME_TYPE_STRING = 0,
    OME_TYPE_INT = 1
} EOmEepromFieldType;

class OmEepromField
{
public:
    const char *name = 0;
    EOmEepromFieldType type = OME_TYPE_STRING; // 0 string, 1 int.
    uint8_t length = 0; // string container size (1 more than max length) or int size 1,2,4,8.
    int offset = 0;
};

/*! @brief Wrapper for eeprom, lets you structure fields and check signature */
class OmEeprom
{
public:
    OmEeprom(const char *signature);
    void addField(const char *fieldName, EOmEepromFieldType type, uint8_t length);

    void begin();

    bool get(const char *fieldName, void *valueOut);
    bool put(const char *fieldName, const void *value);
    bool commit();

    /*! To help debug. */
    void dumpState();

    // And now, the friendlier API calls.
    void addString(const char *fieldName, uint8_t length);
    void addInt8(const char *fieldName);
    void addInt16(const char *fieldName);
    void addInt32(const char *fieldName);

    void set(const char *fieldName, String stringValue);
    void set(const char *fieldName, int32_t intValue);
    String getString(const char *fieldName);
    int getInt(const char *fieldName);

private:
    int signatureSize;
    const char *signature;

    std::vector<OmEepromField> fields;
    bool didBegin = false;

    int dataSize = 0; // sum of signature & fields.
    uint8_t *data = 0; // malloc'd by fields.
    OmEepromField *findField(const char *fieldName);
};

#endif // OMEEPROM_H
