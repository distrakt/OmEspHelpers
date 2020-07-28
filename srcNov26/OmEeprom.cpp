#include "OmEeprom.h"
#include "OmUtil.h"
#include "OmLog.h"
#include <EEPROM.h>


// +------------------------------------------------
// | HELPERS
// +------------------------------------------------

/// read a string, but max string length IS size - 1.
static void eeGetString(int offset, char *out, int size)
{
    bool z = false;
    for(int ix = 0; ix < size; ix++)
    {
        char c = 0;
        if(!z)
        {
            c = EEPROM.read(offset + ix);
            z = c == 0;
        }
        out[ix] = c;
    }
    out[size - 1] = 0;
}

/// read an int from memory. Yes we assume the endianness
/// is the same as when we wrote it. :) 
static void eeGetInt(int offset, uint8_t *out, int size)
{
    bool z = false;
    for(int ix = 0; ix < size; ix++)
    {
        uint8_t c = EEPROM.read(offset + ix);
        out[ix] = c;
    }
}


// +------------------------------------------------
// | OMEEPROM CLASS
// +------------------------------------------------


OmEeprom::OmEeprom(const char *signature)
{
    this->signature = signature;
    this->signatureSize = strlen(this->signature) + 1;
}

void OmEeprom::addField(const char *fieldName, uint8_t type, uint8_t length)
{
    OmEepromField field;
    field.name = fieldName;
    field.type = type;
    field.length = length;
    this->fields.push_back(field);
}

void OmEeprom::begin()
{
    if(this->didBegin)
        return;

    this->fieldsSize = 0;
    int fieldsBeginning = this->signatureSize;
    for(OmEepromField &field : this->fields)
    {
        field.offset = fieldsBeginning + this->fieldsSize;
        this->fieldsSize += field.length;
    }

    this->dataSize = this->signatureSize = this->fieldsSize;
    
    this->data = (uint8_t *)calloc(1, this->dataSize);
    memcpy(this->data, this->signature, this->signatureSize);

    EEPROM.begin(this->dataSize); // all the fields plus the signature

    // check the signature. If it matches, read the rest.
    bool match = true;
    for(int ix = 0; ix < this->signatureSize; ix++)
    {
        char ch = EEPROM.read(ix);
        if(ch != this->signature[ix])
        {
            match = false;
            break;
        }
    }
    OMLOG("eeprom signature '%s' %s match.", this->signature, match ? "did" : "did not");
    if(match)
    {
        // great! read all them fields.
        for(OmEepromField &field : this->fields)
        {
            if(field.type == 0)
                eeGetString(field.offset, (char *)this->data + field.offset, field.length);
            else if(field.type == 1)
                eeGetInt(field.offset, this->data + field.offset, field.length);
        }
    }
    this->didBegin = true;
}

/// if string is shorter than field, zero out the rest.
/// also ensure last char is zero, for sure.
void quellStringTail(OmEepromField *field, char *data)
{
    if(field->type == 0)
    {
        // string, yes.
        bool z = false;
        for(int ix = 0; ix < field->length; ix++)
        {
            if(z)
                data[ix] = 0;
            else if(data[ix] == 0)
                z = true;
        }
        data[field->length - 1] = 0;
    }
}

bool OmEeprom::get(const char *fieldName, void *valueOut)
{
    if(!this->didBegin) return false;
    OmEepromField *field = this->findField(fieldName);
    if(!field)
        return false;

    memcpy(valueOut, this->data + field->offset, field->length);
    quellStringTail(field, (char *)valueOut);

    return true;
}

bool OmEeprom::put(const char *fieldName, const void *value)
{
    if(!this->didBegin) return false;
    OmEepromField *field = this->findField(fieldName);
    if(!field)
        return false;

    memcpy(this->data + field->offset, value, field->length);
    quellStringTail(field, (char *)this->data + field->offset);

    return true;
}

OmEepromField *OmEeprom::findField(const char *fieldName)
{
    for(OmEepromField &field : this->fields)
        if(omStringEqual(fieldName, field.name))
            return &field;
    return 0;
}

bool OmEeprom::commit()
{
    if(!this->didBegin) return false;
    bool result = true;
    bool change = false;
    for(int ix = 0; ix < this->dataSize; ix++)
    {
        if(EEPROM.read(ix) != this->data[ix])
        {
            change = true;
            result &= EEPROM.write(ix, this->data[ix]);
        }
    }
    if(change)
        result &= EEPROM.commit();

    return result;
}

void OmEeprom::dumpState()
{
    OMLOG("eeprom dataSize: %d", this->dataSize);
    uint8_t dumpBuffer[128];
    char printBuffer[256];
    OMLOG("signature: %s", this->signature);
    OMLOG("didBegin: %d", this->didBegin);
    if(this->didBegin)
    {
        OMLOG("data.signature[%d]: %s", this->signatureSize, this->data);
        for(OmEepromField &f : this->fields)
        {
            this->get(f.name, dumpBuffer);
            char *w = printBuffer;
            for(int ix = 0; ix < f.length; ix++)
            {
                w += sprintf(w, " %02x", dumpBuffer[ix]);
            }
            OMLOG("data.%s[%d@%d]: %s", f.name, f.length, f.offset, printBuffer);
        }
    }
}
