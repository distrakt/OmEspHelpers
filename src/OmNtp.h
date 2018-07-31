
/*
 * OmNtp.h
 * 2016-12-13
 *
 * This class implements a background time service.
 *
 * HOW DOES IT WORK
 * 
 * It is not very accurate. Every couple of minutes it queries a well-known
 * time server (time.nist.gov), and adds on-board millis() when you ask
 * for the time. It gives you the time in Hours, Minutes, and Seconds.
 *
 * If Wifi is not available, the time is "unknown" or (-1,-1,-1).
 *
 * EXAMPLE
 *
 *     OmNtp ntp;
 *
 * in setup():
 *
 *     ntp.setTimeZone(-8); // for California in the winter
 *
 * when wifi is established:
 *
 *     ntp.setWifiAvailable(true);
 *
 * in loop():
 *
 *     ntp.tick();
 *
 * when you want the time:
 *
 *     int hour, minute, second;
 *     bool gotTheTime = ntp.getTime(hour, minute, second);
 *     const char *string = ntp.getTimeString(); // HH:MM:SS or "unknown"
 *
 * Adapted from https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/NTPClient/NTPClient.ino
 */

#ifndef __OmNtp__
#define __OmNtp__

#include "OmLog.h"
#include "WiFiUdp.h"

class OmNtp
{
public:
    /// Call this after wifi properly established
    void setWifiAvailable(bool wifiAvailable);

    /// Call this in your loop(). Assumes about 20-50 ms interval.
    void tick();
    
    /// This number gets added to UTC for your time zone. It ranges
    /// from -12 to +12. California in December needs -8.
    void setTimeZone(int hourOffset);

    /// Get the time of day in three handy integers.
    bool getTime(int &hourOut, int &minuteOut, int &secondOut);

    /// Get the time of day in a handy string like HH:MM:SS.
    /// (It's a static char[], by the way.)
    const char *getTimeString();

    /// Get the last-created ntp instance, if any
    static OmNtp *ntp();

private:
    // Internal implementation methods and details.
    void begin();
    void checkForPacket();

    WiFiUDP udp; // the connection
    int interval = 6000; // ticks between NTP queries; 2 minutes at 20ms/tick.
    int countdown = -1;
    int timeZone; // offset the hour reported by this much.
    unsigned long int uTimeAcquiredMillis = 0; // the millis() when this last time was got
    unsigned long int uTime = 0; // time since 1970-01-01
    int hour = -1;
    int minute = -1;
    int second = -1;
    bool begun = false;
    bool wifiAvailable;
    static const int kNtpPacketSize = 48;
    byte packetBuffer[kNtpPacketSize]; //buffer to hold incoming and outgoing packets
    IPAddress ntpServerIp; // resolved IP address (time.nist.gov)

    static OmNtp *lastNtpBegun;
};

#endif __OmNtp__