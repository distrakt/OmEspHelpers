/*
 * OmLog.h
 * 2016-12-13
 *
 * This class implements a simple console log
 * utility. It's just like printf or
 * Serial.printf, with some pretty printing.
 *
 * EXAMPLE
 *
 * OMLOG("a button was pressed: %d\n", buttonNumber);
 *
 * Which prints out like:
 *
 *      1200.22 (*) MySketch.ino.105: a button was pressed: 3
 *
 * The numbers on the left is a timestamp in seconds and hundredths.
 */

#ifndef __OmLog__
#define __OmLog__

#include <stdint.h>
#include <stdlib.h>
// just for IPAddress type.
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#endif
#if NOT_ARDUINO
typedef unsigned char IPAddress[4];
#endif

#include <stdarg.h>

/*! @brief A formatted printing helper. It adds the file and line number to the output. */
#define OMLOG(_args...) OmLog.logS(__FILE__, __LINE__, '*', _args)
/*! @brief A formatted printing helper. Like OMLOG, but has E for error. */
#define OMERR(_args...) OmLog.logS(__FILE__, __LINE__, 'E', _args)

/*! @brief Utility to convert an ESP8266 ip address to a printable string. */
const char *ipAddressToString(IPAddress ip);

/// all of OmEspHelpers printing goes through here
typedef size_t (* OmVPrintfHandler)(const char *format, va_list args);


class OmLogClass
{
public:
    uint32_t bufferSize = 0;
    uint32_t bufferW = 0;
    char *buffer = NULL;
    OmVPrintfHandler vPrintfHandler = NULL;

    void logS(const char *file, int line, char ch, const char *format, ...);

    void setBufferSize(uint32_t bufferSize);
    void clear();
    const char *getBufferA();
    const char *getBufferB();

    void setVPrintf(OmVPrintfHandler vPrintfHandler);
    size_t printf(const char *format, ...);

    bool bufferEnabled = true;
    bool setBufferEnabled(bool enabled);
};

extern OmLogClass OmLog;

#endif // __OmLog__
