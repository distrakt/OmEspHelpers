#include "OmEeprom.h"
#include "OmUtil.h"
#include "OmLog.h"

#ifdef NOT_ARDUINO
#include "EepromTesting.h"
#else
#include <EEPROM.h>
#endif

// +------------------------------------------------
// | HELPERS
// +------------------------------------------------

#if 0
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
    for(int ix = 0; ix < size; ix++)
    {
        uint8_t c = EEPROM.read(offset + ix);
        out[ix] = c;      
    }
}
#endif

// +------------------------------------------------
// | OMEEPROM CLASS
// +------------------------------------------------

static uint8_t *putInt(uint32_t value, int length, uint8_t *data)
{
    while(length-- > 0)
    {
        *data++ = value & 0xff;
        value >>= 8;
    }
    return data;
}

OmEepromClass::OmEepromClass()
{
    this->signature = "ee";
    this->signatureSize = (int)strlen(this->signature) + 1;
    OmEepromClass::active = true;
}

OmEepromField *OmEepromClass::addField(const char *fieldName, EOmEepromFieldType type, uint8_t length, int omeFlags, const char *label)
{
    OmEepromField field;
    field.name = fieldName;
    field.type = type;
    field.length = length;
    field.omeFlags = omeFlags;
    field.label = label;
    this->fields.push_back(field);
    return &this->fields[this->fields.size() - 1];
}

void OmEepromClass::end()
{
    this->didBegin = false;
    this->signature = "";
    this->signatureSize = (int)strlen(this->signature) + 1;
    this->fields.clear();
    free(this->data);
    this->data = NULL;
    OmEepromClass::active = false;
}

void OmEepromClass::begin(const char *signature)
{
    this->dumpState("begin");
    if(this->didBegin)
    {
        OMERR("already did begin");
        return;
    }

    // new 2021-07-26 way -- the data footprint includes
    // field descriptions, so we can read them back all out of order and such.
    int len = 0;

    /*
     layout is like this:
     0x23 0x42 <-- new sig 2
     lengthLo, lengthHi <-- total size of eeprom data including first 4 bytes header goodies

     each field:
      type, size <-- 2 bytes
      name <-- null terminated string (strlen+1 bytes)
      data <-- size bytes of the actual payload.
     */
    len += 4; // header
    for(OmEepromField &field : this->fields)
    {
        len += 2; // type & size
        len += strlen(field.name) + 1; // space for the field name...
        field.offset = len; // here's the data
        len += field.length;
    }

    // we know how big it is. Allocate our in-mem copy,
    // and populate it with useful info frome the field descriptions.
    this->dataSize = len;
    this->data = (uint8_t *)calloc(1, this->dataSize);
    uint8_t *w = this->data;
    *w++ = 0x23;
    *w++ = 0x42;
    w = putInt(this->dataSize, 2, w);

    for(OmEepromField &field : this->fields)
    {
        *w++ = field.type;
        *w++ = field.length;
        int nameLength = (int)strlen(field.name) + 1;
        memcpy(w, field.name, nameLength);
        w += nameLength;
        for(int ix = 0; ix < field.length; ix++)
            *w++ = 0;
    }

    // we set didBegin here, knowing we're going to use only the SetValue functions.
    // commit won't really work til the end of this proc.
    this->didBegin = true;

    // NOW, we walk the actual EEPROM and pull out fields where we can, to populate in-memory.
    EEPROM.begin(4);
#define EER(_ix) ((uint8_t)EEPROM.read(_ix))
    if(EER(0) != 0x23 || EER(1) != 0x42)
        goto doneReadingEeprom;

    {
        // check the length, then march it all
        int eepromLength = EER(2) + 0x100 * EER(3);
        if(eepromLength < 0 || eepromLength >= 0x1000)
            goto doneReadingEeprom;
        EEPROM.begin(eepromLength);
        int ix = 4;
    readNextEepromField:
#define EIX(_var) if(ix >= eepromLength) goto doneReadingEeprom; _var = EER(ix); ix++;
        EIX(int type)
        EIX(int size)
        String fieldName;
        uint8_t ch;
        do {
            EIX(ch)
            if(ch != 0)
                fieldName += (char)ch;
        } while(ch);
        // we have a field name!
        printf("found field '%s', type %d, size %d\n", fieldName.c_str(), type, size);

        if(type == OME_TYPE_INT)
        {
            uint32_t v = 0;
            for(int vx = 0; vx < size; vx++)
            {
                EIX(uint8_t byte);
                v += byte << (8 * vx);
            }
            this->set(fieldName.c_str(), v);
        }
        else if(type == OME_TYPE_STRING)
        {
            String s;
            for(int vx = 0; vx < size; vx++)
            {
                EIX(uint8_t byte);
                if(byte)
                    s += (char)byte;
            }
            this->setString(fieldName.c_str(), s);
        }
        else if(type == OME_TYPE_BYTES)
        {
            for(int vx = 0; vx < size; vx++)
            {
                EIX(uint8_t byte);
                this->set(fieldName.c_str(), vx, 1, &byte);
            }
        }
        else
            ix += size;
        goto readNextEepromField;
    }

doneReadingEeprom:
    // We've walked the eeprom, and pulled our data in as needed.
    // Now we know what size it should be, and we rewrite the data in this form.
    // Of course thankfully no actual writes occur if nothing has changed.
    EEPROM.begin(this->dataSize);
    this->commit();
}

/// if string is shorter than field, zero out the rest.
/// also ensure last char is zero, for sure.
void quellStringTail(OmEepromField *field, char *data)
{
    if(field->type == OME_TYPE_STRING)
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

bool OmEepromClass::get(const char *fieldName, void *valueOut)
{
    if(!this->didBegin)
    {
        OMERR("get: did not begin");
        return false;
    }
    OmEepromField *field = this->findField(fieldName);
    if(!field)
        return false;
    
    memcpy(valueOut, this->data + field->offset, field->length);
    quellStringTail(field, (char *)valueOut);
    
    return true;
}

bool OmEepromClass::put(const char *fieldName, const void *value)
{
    if(!this->didBegin)
    {
        OMERR("put: did not begin");
        return false;
    }
    OmEepromField *field = this->findField(fieldName);
    if(!field)
        return false;
    
    memcpy(this->data + field->offset, value, field->length);
    quellStringTail(field, (char *)this->data + field->offset);

    // if it's a bonjour string, fix up the text.
    if(field->type == OME_TYPE_STRING && (field->omeFlags & OME_FLAG_BONJOUR))
    {
        bool lastWasHyphen = false;
        bool anyNonHyphens = false;
        int wx = field->offset;
        for(int ix = 0; ix < field->length - 1; ix++)
        {
            int fx = field->offset + ix;
            char ch = this->data[fx];
            OMLOG("ix=%d fx=%d wx=%d ch=%d", ix, fx, wx, ch);
            if(ch == 0)
                break;
            if( (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || (ch == '-'))
            { /* it's legal */}
            else
            {
                ch = '-';
            }
            bool isHyphen = ch == '-';
            if(!isHyphen)
            {
                if(lastWasHyphen && anyNonHyphens)
                    this->data[wx++] = '-';
                this->data[wx++] = ch;
                anyNonHyphens = true;
                OMLOG("--> %d %c", ch, ch);
            }
            lastWasHyphen = isHyphen;
        }
        while(wx < field->offset + field->length)
              this->data[wx++] = 0;
    }
    
    return true;
}

OmEepromField *OmEepromClass::findField(const char *fieldName)
{
    if(!this->didBegin)
    {
        OMERR("findField: did not begin");
        return NULL;
    }
    for(OmEepromField &field : this->fields)
        if(omStringEqual(fieldName, field.name))
            return &field;
    OMERR("no field '%s'", fieldName);
    return 0;
}

OmEepromField *OmEepromClass::findField(int ix)
{
    if(!this->didBegin)
    {
        OMERR("findField: did not begin");
        return NULL;
    }
    if(ix < 0 || ix >= this->fields.size())
    {
        OMERR("no field %d", ix);
        return NULL;
    }
    OmEepromField *field = &this->fields[ix];
    return field;
}

int OmEepromClass::commit()
{
    this->dumpState("commit");
    if(!this->didBegin) return -1;
    int k = 0;
    for(int ix = 0; ix < this->dataSize; ix++)
    {
        if(EEPROM.read(ix) != this->data[ix])
        {
            k++;
            EEPROM.write(ix, this->data[ix]);
        }
    }
    if(k > 0)
        EEPROM.commit();
    
    return k;
}

void OmEepromClass::dumpState(const char *note)
{
    return;
    if(note)
        OMLOG(note);
    OMLOG("eeprom dataSize: %d", this->dataSize);
    uint8_t dumpBuffer[128];
    char printBuffer[256];
    char *printBufferEnd = printBuffer + 240;
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
                if(w < printBufferEnd)
                    w += sprintf(w, " %02x", dumpBuffer[ix]);
            }
            OMLOG("data.%s[%d@%d]: %s", f.name, f.length, f.offset, printBuffer);
        }
    }
}


char *OmEepromClass::addString(const char *fieldName, uint8_t length, int omeFlags, const char *label)
{
    OmEepromField *f = this->addField(fieldName, OME_TYPE_STRING, length, omeFlags, label);
    char *result = (char *)(this->data + f->offset);
    return result;
}

char *OmEepromClass::addString(const char *fieldName, uint8_t length)
{
    return this->addString(fieldName, length, 0, NULL);
}

void OmEepromClass::addBytes(const char *fieldName, uint8_t length, int omeFlags, const char *label)
{
    this->addField(fieldName, OME_TYPE_BYTES, length, omeFlags, label);
}

int8_t *OmEepromClass::addInt8(const char *fieldName, int omeFlags, const char *label)
{
    OmEepromField *f = this->addField(fieldName, OME_TYPE_INT, 1, omeFlags, label);
    int8_t *result = (int8_t *)(this->data + f->offset);
    return result;
}
int16_t *OmEepromClass::addInt16(const char *fieldName, int omeFlags, const char *label)
{
    OmEepromField *f = this->addField(fieldName, OME_TYPE_INT, 2, omeFlags, label);
    int16_t *result = (int16_t *)(this->data + f->offset);
    return result;
}
int32_t *OmEepromClass::addInt32(const char *fieldName, int omeFlags, const char *label)
{
    OmEepromField *f = this->addField(fieldName, OME_TYPE_INT, 4, omeFlags, label);
    int32_t *result = (int32_t *)(this->data + f->offset);
    return result;
}

void OmEepromClass::set(const char *fieldName, String stringValue)
{
    this->put(fieldName, stringValue.c_str());
}
void OmEepromClass::set(const char *fieldName, int32_t intValue)
{
    this->put(fieldName, &intValue); // depends on being littleendian.
}
String OmEepromClass::getString(const char *fieldName)
{
    
    OmEepromField *field = this->findField(fieldName);
    if(!field)
        return "";
    String s;
    switch(field->type)
    {
        case OME_TYPE_INT:
        {
            char cc[16];
            int v = this->getInt(fieldName);
            if(field->omeFlags & OME_FLAG_HUNDREDTHS)
            {
                const char *sign = "";
                if(v < 0)
                {
                    sign = "-";
                    v = -v;
                }
                sprintf(cc, "%s%d.%02d", sign, v / 100, v %100);
            }
            else
            {
                sprintf(cc, "%d", v);
            }
            s = cc;
            break;
        }
        case OME_TYPE_STRING:
        {
            this->data[field->offset + field->length - 1] = 0;
            s = ((char *)this->data) + field->offset;
            break;
        }
    }
    return s;
}
int OmEepromClass::getInt(const char *fieldName)
{
    int i = 0;
    OmEepromField *f = this->findField(fieldName);
    if(f && f->type == OME_TYPE_INT && f->length <= 4)
    {
        this->get(fieldName, &i);
        bool isNegative = (i & 1 << (f->length * 8 - 1)) != 0;
        if(isNegative)
        {
            for(int ix = f->length; ix < 4; ix++)
                ((char *)(&i))[ix] = 0xff;
        }
    }
    return i;
}

void OmEepromClass::set(const char *fieldName, int first, int count, uint8_t *bytes)
{
    OmEepromField *f = this->findField(fieldName);
    if(f && f->type == OME_TYPE_BYTES)
    {
        while(count--)
        {
            if(first < 0 || first >= f->length)
                break;
            this->data[f->offset + first] = *bytes++;
            first++;
        }
    }
}
void OmEepromClass::getBytes(const char *fieldName, int first, int count, uint8_t *bytes)
{
    // clear first...
    for(int ix = 0; ix < count; ix++)
        bytes[ix] = 0;

    OmEepromField *f = this->findField(fieldName);
    if(f && f->type == OME_TYPE_BYTES)
    {
        while(count--)
        {
            if(first < 0 || first >= f->length)
                break;
             *bytes++ = this->data[f->offset + first];
            first++;
        }
    }
}

uint8_t OmEepromClass::getByte(const char *fieldName, int index)
{
    uint8_t result = 0;
    this->getBytes(fieldName, index, 1, &result);
    return result;
}


int OmEepromClass::getFieldCount()
{
    return (int)this->fields.size();
}

const char *OmEepromClass::getSignature()
{
    return this->signature;
}
int OmEepromClass::getDataSize()
{
    return this->dataSize;
}

const char *OmEepromClass::getFieldName(int ix)
{
    if(ix < 0 || ix > this->fields.size())
        return NULL;
    return this->fields[ix].name;
}

int OmEepromClass::getFieldLength(int ix)
{
    if(ix < 0 || ix > this->fields.size())
        return -1;
    return this->fields[ix].length;
}

int OmEepromClass::getFieldType(int ix)
{
    if(ix < 0 || ix > this->fields.size())
        return -1;
    return this->fields[ix].type;
}

void OmEepromClass::setString(const char *fieldName, String value)
{
    OmEepromField *f = this->findField(fieldName);
    if(f)
    {
        switch(f->type)
        {
            case OME_TYPE_INT:
            {
                int decimals = (f->omeFlags & OME_FLAG_HUNDREDTHS) ? 2 : 0;
                int x = omStringToInt(value.c_str(), decimals);
                this->put(fieldName, &x);
                break;
            }

            case OME_TYPE_STRING:
                this->put(fieldName, value.c_str());
                break;
        }
    }
}

/** This global OmEeprom magics into existence if and only if you use it. Arduino-style. */
bool OmEepromClass::active = false;
OmEepromClass OmEeprom;
