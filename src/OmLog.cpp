/*
 * OmNtp.cpp
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
void OmLog::logS(const char *file, int line, const char *format, ...)
{
    va_list args;
    va_start (args, format);
    static char s[320];
    vsnprintf (s, sizeof(s), format, args);
    s[sizeof(s) - 1] = 0;
    auto t = millis();
    auto tS = t / 1000;
    auto tH = (t / 10) % 100;
    
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
    
    
    printf("%4d.%02d (*) %s.%d: ", tS, tH, file + fileStringOffset, line);
    printf("%s", s);
    
    // add trailing CR if missing
    int len = (int)strlen(format);
    if(len == 0 || format[len - 1] > 13)
        printf("%s", "\n");
}

