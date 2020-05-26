
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
    OmNtp();

    /// Call this after wifi properly established
    void setWifiAvailable(bool wifiAvailable);

    /// Call this in your loop(). Assumes about 20-50 ms interval.
    void tick(long milliseconds); // pass in the current apparent "millis", we'll pace ourselves accordingly.
    
    /// This number gets added to UTC for your time zone. It ranges
    /// from -12 to +12. California in December needs -8.
    void setTimeZone(int hourOffset);

    int getTimeZone();
    
    /// Issue a request to an http service, the url you set with setTimeUrl().
    void setLocalTimeZone();

    /*
    <?
    print date("Y-m-d H:i:s");
    print " + ";
    print date("e");
    ?>
     */
    /// URL to a server under your control that gives the current time.
    /// OmNtp will deduce the timezone relative to NTP from it, also.
    /// Https not supported, just http.
    void setTimeUrl(const char *timeUrl);

    /// Get the time of day in three handy integers.
    bool getTime(int &hourOut, int &minuteOut, int &secondOut);
    /// Get the time of day in two handy integers and a float.
    bool getTime(int &hourOut, int &minuteOut, float &secondOut);

    /// Get the time of day in a handy string like HH:MM:SS.
    /// (It's a static char[], by the way.)
    const char *getTimeString();

    bool getUTime(uint32_t &uTimeOut, int &uFracOut); // seconds of 1970-1-1, and milliseconds.
    uint32_t getUTime(); // lite version
    bool uTimeToTime(uint32_t uTime, int uFrac, int &hourOut, int &minuteOut, float &secondOut); // in local time zone

    /// Get the last-created ntp instance, if any
    static OmNtp *ntp();

private:
    // Internal implementation methods and details.
    void begin();
    void checkForPacket();

    WiFiUDP udp; // the connection
    long intervalMilliseconds = 120017; // ticks between NTP queries; 2 minutes
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
    
    bool ntpRequestSent = 0; // is set "true" after sending ntp request, and cleared when we get answer.

    const char *timeUrl = 0;
    bool localTimeGot = false; // set when we get it, and cleared when used
    bool localTimeEverGot = false; // set if it has ever ever worked.
    int localHour = -1;
    int localMinute = -1;
    int localSecond = -1;
    unsigned long int localTimeGotMillis = 0;

    int localTimeRefetchCountdown = 0; // now and then, reget local time.

    long countdownMilliseconds = -1;
    long lastMilliseconds = 0;
};

#endif // __OmNtp__
