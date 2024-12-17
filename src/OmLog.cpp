/*
 * OmLog.cpp
 * 2016-12-13 dvb
 * Implementation of OmLog
 */

#include "OmLog.h"
#include "OmUtil.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if NOT_ARDUINO
static long millis()
{
    return 0;
}
#else
//#define printf Serial.printf
#endif

const char *ipAddressToString(IPAddress ip)
{
    unsigned int localIpInt = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | (ip[3] << 0);
    return omIpToString(localIpInt);
}

void OmLogClass::logS(const char *file, int line, char ch, const char *format, ...)
{
    va_list args;
    va_start (args, format);
    static char s[320];
    vsnprintf (s, sizeof(s), format, args);
    s[sizeof(s) - 1] = 0;
    int t = (int)millis();
    int tS = t / 1000;
    int tH = (t / 10) % 100;
    
    // find the last slash
    int fileStringOffset = 0;
    {
        int ix = 0;
        while(file[ix])
        {
            if(file[ix] == '/')
                fileStringOffset = ix + 1;
            ix++;
        }
    }

    this->printf("%4d.%02d (%c) %s.%d: ", tS, tH, ch, file + fileStringOffset, line);
    this->printf("%s", s);

    if(this->buffer && this->bufferEnabled)
    {
        // we write at most half the buffer size.
        // when buffer passes half full we swap the halves
        // so we're never snprint-ing across the end loop.
        this->bufferW += snprintf(this->buffer + this->bufferW, this->bufferSize / 2,
                                   "%4d.%02d (%c) %s.%d: ", tS, tH, ch, file + fileStringOffset, line);
        this->bufferW += snprintf(this->buffer + this->bufferW, this->bufferSize / 2,
                                   "%s", s);
    }
    
    // add trailing CR if missing
    int len = (int)strlen(format);
    if(len == 0 || format[len - 1] > 13)
    {
        this->printf("%s", "\n");
        if(this->buffer && this->bufferEnabled)
            this->buffer[this->bufferW++] = '\n';
    }

    // double-ensure that it's zero terminated
    if(this->buffer)
        this->buffer[this->bufferW] = 0;

    // lastly, if we're printing in buffer, swap the halves when time.
    if(this->buffer)
    {
        uint32_t half = this->bufferSize / 2;
        if(this->bufferW > half)
        {
            for(uint32_t ix = 0; ix < half; ix++)
            {
                char c = this->buffer[ix];
                this->buffer[ix] = this->buffer[ix + half];
                this->buffer[ix + half] = c;
            }
            this->bufferW -= half;
        }
    }
}

void OmLogClass::setBufferSize(uint32_t bufferSize)
{
    if(this->buffer)
    {
        free((void *)this->buffer);
        this->buffer = NULL;
    }
    this->bufferSize = bufferSize;
    if(bufferSize)
        this->buffer = (char *)calloc(bufferSize+1, 1); // extra end byte is always 0.
}

// only affects the in-memory buffer writing; serial not affected by this.
bool OmLogClass::setBufferEnabled(bool enabled)
{
    bool result = this->bufferEnabled;
    this->bufferEnabled = enabled;
    return result;
}

void OmLogClass::clear()
{

}

const char *OmLogClass::getBufferA()
{
    // first buffer starts just past the current write pointer, and can be zero-len.
    // the writer pointer just finished with null terminator write there
    if(this->buffer)
        return this->buffer + this->bufferW + 1;
    else
        return "";
}

const char *OmLogClass::getBufferB()
{
    // second buffer starts at beginning of buffer, up to the write pointer
    if(this->buffer)
        return this->buffer;
    else
        return "";
}

void OmLogClass::setVPrintf(OmVPrintfHandler vPrintfHandler)
{
    this->vPrintfHandler = vPrintfHandler;
}
size_t OmLogClass::printf(const char *format, ...)
{
    va_list arg;
    va_start(arg, format);
    size_t result = 0;

    if(this->vPrintfHandler)
    {
        result = this->vPrintfHandler(format, arg);
    }
    else
    {
        result = vprintf(format, arg);
    }

    va_end(arg);
    return result;
}



//uint32_t OmLogClass::bufferSize = 0;
//uint32_t OmLogClass::bufferW = 0;
//char *OmLogClass::buffer = 0;

OmLogClass OmLog;
