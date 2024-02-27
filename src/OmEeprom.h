#include <vector>
#include <string>
#include <stdint.h>

#ifndef OMEEPROM_H
#define OMEEPROM_H

#ifdef NOT_ARDUINO
#include <string>
#define String std::string
#else
#include "Arduino.h"
#endif

typedef enum
{
    OME_TYPE_STRING = 0,
    OME_TYPE_INT = 1,
    OME_TYPE_BYTES = 2, // bytes array
} EOmEepromFieldType;

typedef enum
{
    /// low 8 bits form an enum of categories
    /// omit from easy user form UI
    OME_GROUP_OTA = 201,
    OME_GROUP_WIFI_SETUP = 202,
    /// 8th bit and up are format flags
    /// a string which should be constrained to FQDN characters a-z, 0-9, -.
    OME_FLAG_BONJOUR = 0x0100,
    /// for an int, present 1234 as 12.34 entry text. used when rendering html.
    OME_FLAG_HUNDREDTHS = 0x0200,
} EOmEepromFlags;

class OmEepromField
{
public:
    const char *name = 0;
    EOmEepromFieldType type = OME_TYPE_STRING; // 0 string, 1 int.
    uint16_t length = 0; // string container size (1 more than max length) or int size 1,2,4,8.

    int omeFlags = 0;
    const char *label = 0; // typically a prettier version of the field name
    const char *description = 0;

    int offset = 0;
};

/*! @brief Wrapper for eeprom, lets you structure fields and check signature */
class OmEepromClass
{
public:
    OmEepromClass();
    static bool active;
    OmEepromField *addField(const char *fieldName, EOmEepromFieldType type, uint8_t length, int omeFlags, const char *label);

    void begin(const char *signature = "x"); // signature is ignored.
    void end();

    bool get(const char *fieldName, void *valueOut, int valueLength = -1); // value length if provided limits write length
    bool put(const char *fieldName, const void *value, int valueLength = -1); // valueLength if provided limits bytes copied, pads with 0
    /** Commit the current in-memory eeprom to persistent eeprom/flash. Return number of bytes written, or -1 for failure. */
    int commit();

    /// Print out the in-memory contents of the Eeprom and other misc. Helpful for debugging.
    void dumpState(const char *note = NULL);

    // And now, the friendlier API calls.

    /// Add a string field, in group 0
    char *addString(const char *fieldName, uint8_t length);
    /// Add a string field.
    /// @param fieldName name stored with the field
    /// @param length maximum length of string
    /// @param omeFlags lowest 8 bits specifies a "group" for this field, which can be presented as a Form on a page
    /// @param label presentation name for this field -- not saved in eeprom so you can change it when needed.
    char *addString(const char *fieldName, uint8_t length, int omeFlags, const char *label);

    int8_t *addInt8(const char *fieldName, int omeFlags = 0, const char *label = NULL);
    int16_t *addInt16(const char *fieldName, int omeFlags = 0, const char *label = NULL);
    int32_t *addInt32(const char *fieldName, int omeFlags = 0, const char *label = NULL);

    void addBytes(const char *fieldName, uint8_t length, int omeFlags = 0, const char *label = NULL);

    void set(const char *fieldName, String stringValue);
    void set(const char *fieldName, int32_t intValue);
    bool set(const char *fieldName, int first, int count, uint8_t *bytes);

    String getString(const char *fieldName);
    void setString(const char *fieldName, String value);
    int getInt(const char *fieldName);
    void getBytes(const char *fieldName, int first, int count, uint8_t *bytes);
    uint8_t getByte(const char *fieldName, int index);

    int getFieldCount();
    int getDataSize();
    const char *getFieldName(int ix);
    int getFieldLength(int ix);
    int getFieldType(int ix);

    OmEepromField *findField(const char *fieldName);
    OmEepromField *findField(int ix);

    /// set a value from a string. convert to int for int type.
    void fieldFromString(const char *fieldName, String value);

    /// retrieve value as string. convert from int for int type. (todo -- format styles? flags?)
    String fieldToString(const char *fieldName);

    bool verbose = false;

private:

    std::vector<OmEepromField> fields;
    bool didBegin = false;

    int dataSize = 0; // sum of signature & fields.
    uint8_t *data = 0; // malloc'd by fields.
};

extern OmEepromClass OmEeprom;

#endif // OMEEPROM_H
