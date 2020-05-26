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

/// This macro is what you'll call. It adds the file and line number to the output.
#define OMLOG(_args...) OmLog::logS(__FILE__, __LINE__, '*', _args)
#define OMERR(_args...) OmLog::logS(__FILE__, __LINE__, 'E', _args)

/// Utility to convert an ESP8266 ip address to a printable string.
const char *ipAddressToString(IPAddress ip);

class OmLog
{
public:
    static void logS(const char *file, int line, char ch, const char *format, ...);
};

#endif __OmLog__
