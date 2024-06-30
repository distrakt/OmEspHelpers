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
#define printf Serial.printf
#endif

const char *ipAddressToString(IPAddress ip)
{
    unsigned int localIpInt = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | (ip[3] << 0);
    return omIpToString(localIpInt);
}

/* static method */
void OmLog::logS(const char *file, int line, char ch, const char *format, ...)
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

    printf("%4d.%02d (%c) %s.%d: ", tS, tH, ch, file + fileStringOffset, line);
    printf("%s", s);

    if(OmLog::buffer)
    {
        // we write at most half the buffer size.
        // when buffer passes half full we swap the halves
        // so we're never snprint-ing across the end loop.
        OmLog::bufferW += snprintf(OmLog::buffer + OmLog::bufferW, OmLog::bufferSize / 2,
                                   "%4d.%02d (%c) %s.%d: ", tS, tH, ch, file + fileStringOffset, line);
        OmLog::bufferW += snprintf(OmLog::buffer + OmLog::bufferW, OmLog::bufferSize / 2,
                                   "%s", s);
    }
    
    // add trailing CR if missing
    int len = (int)strlen(format);
    if(len == 0 || format[len - 1] > 13)
    {
        printf("%s", "\n");
        if(OmLog::buffer)
            OmLog::buffer[OmLog::bufferW++] = '\n';
    }

    // double-ensure that it's zero terminated
    if(OmLog::buffer)
        OmLog::buffer[OmLog::bufferW] = 0;

    // lastly, if we're printing in buffer, swap the halves when time.
    if(OmLog::buffer)
    {
        uint32_t half = OmLog::bufferSize / 2;
        if(OmLog::bufferW > half)
        {
            for(uint32_t ix = 0; ix < half; ix++)
            {
                char c = OmLog::buffer[ix];
                OmLog::buffer[ix] = OmLog::buffer[ix + half];
                OmLog::buffer[ix + half] = c;
            }
            OmLog::bufferW -= half;
        }
    }


}

void OmLog::setBufferSize(uint32_t bufferSize)
{
    if(OmLog::buffer)
    {
        free((void *)OmLog::buffer);
        OmLog::buffer = NULL;
    }
    OmLog::bufferSize = bufferSize;
    if(bufferSize)
        OmLog::buffer = (char *)calloc(bufferSize+1, 1); // extra end byte is always 0.
}

void OmLog::clear()
{

}

const char *OmLog::getBufferA()
{
    // first buffer starts just past the current write pointer, and can be zero-len.
    // the writer pointer just finished with null terminator write there
    if(OmLog::buffer)
        return OmLog::buffer + OmLog::bufferW + 1;
    else
        return "";
}

const char *OmLog::getBufferB()
{
    // second buffer starts at beginning of buffer, up to the write pointer
    if(OmLog::buffer)
        return OmLog::buffer;
    else
        return "";
}



uint32_t OmLog::bufferSize = 0;
uint32_t OmLog::bufferW = 0;
char *OmLog::buffer = 0;
