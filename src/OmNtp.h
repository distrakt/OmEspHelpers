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

// for testing, the utilities can be built on mac. But the main class is Arduino only.
class OmNtpSyncRecord
{
public:
    bool acquired = false;
    unsigned long acquiredMs = 0; // when it was acquired
    unsigned long reportedSeconds = 0; // seconds since 1970 or seconds since midnight.

    // constructors
    OmNtpSyncRecord(){};
    OmNtpSyncRecord(unsigned long nowMs, int hour, int minute, int second);

    // seconds within day
    float getSwd(unsigned long nowMs, int zone = 0);

    // get the raw seconds, only sensible for a UTC time. no zone.
    double getU(unsigned long nowMs);
};

float omGetSecondsWithinDay(unsigned long nowMs, OmNtpSyncRecord &utcTime, OmNtpSyncRecord &localTime, int zone);
bool omGetHms(float secondsWithinDay, int &hourOut, int &minuteOut, float &secondOut);
bool omGetHms(float secondsWithinDay, int &hourOut, int &minuteOut, int &secondOut);

#ifndef NOT_ARDUINO
#include "WiFiUdp.h"

/*! class to keep a local clock synchronized from a server */
class OmNtp
{
public:
    OmNtp();

    /*! Call this after wifi properly established */
    void setWifiAvailable(bool wifiAvailable);

    /*! Call this in your loop(). Assumes about 20-50 ms interval. */
    void tick(unsigned int deltaMillis); // millis since last tick, mediated by the owner.
    
    /*! This number gets added to UTC for your time zone. It ranges */
    /*! from -12 to +12. California in December needs -8. */
    void setTimeZone(int hourOffset);


    int getTimeZone();
    
    /*! Issue a request to an http service, the url you set with setTimeUrl(). */
    void getLocalTime();

    /*
    <?
    print date("Y-m-d H:i:s");
    print " + ";
    print date("e");
    ?>
     */
    /*! URL to a server under your control that gives the current time. */
    /*! OmNtp will deduce the timezone relative to NTP from it, also. */
    /*! Https not supported, just http. */
    void setTimeUrl(const char *timeUrl);

    /*! if the time is calculated from a TimeUrl, add this offset to it. */
    /*! for example, I use a CA timeUrl for my lamps in Arkansas so I set */
    /*! it to 2. */
    void setTimeUrlOffset(int hoursFromTimeUrl);

    /*! Get the time of day in three handy integers. */
    bool getTime(int &hourOut, int &minuteOut, int &secondOut);
    /*! Get the time of day in two handy integers and a float. */
    bool getTime(int &hourOut, int &minuteOut, float &secondOut);

    /*! Get the time of day in a handy string like HH:MM:SS. */
    /*! (It's a static char[], by the way.) */
    const char *getTimeString();

    bool getUTime(uint32_t &uTimeOut, int &uFracOut); // seconds of 1970-1-1, and milliseconds.
    uint32_t getUTime(); // lite version

    /*! Get the last-created ntp instance, if any */
    static OmNtp *ntp();

    class OmNtpStats
    {
    public:
        const char *ntpServerName = "";
        IPAddress ntpServerIp;
        const char *timeUrl = NULL;
        unsigned int ntpRequestsSent = 0;
        unsigned int ntpRequestsAnswered = 0;
        long ntpRequestMostRecentMillis = -1;
        unsigned int timeUrlRequestsSent = 0;
        unsigned int timeUrlRequestsAnswered = 0;
        long timeUrlRequestMostRecentMillis = 0;
    };

    OmNtpStats stats;

    /// Static methods to get the time, and return -1's if time does not exist in space.
    static void sGetTimeOfDay(int &minuteWithinDayOut, float &secondWithinMinuteOut);

private:
    // Internal implementation methods and details.
    void begin();
    void checkForPacket();

    WiFiUDP udp; // the connection
    int timeZone; // offset the hour reported by this much.
    bool wifiAvailable;
    const char *timeUrl = 0;
    int timeUrlOffset = 0;

    OmNtpSyncRecord ntpTime;
    OmNtpSyncRecord localTime;

    bool begun = false;
    static const int kNtpPacketSize = 48;
    byte packetBuffer[kNtpPacketSize]; //buffer to hold incoming and outgoing packets
    IPAddress ntpServerIp; // resolved IP address (time.nist.gov)

    bool ntpRequestSent = 0; // is set "true" after sending ntp request, and cleared when we get answer.

    int localTimeRefetchCountdown = 0; // now and then, reget local time.
    long countdownMilliseconds = -1;

    static OmNtp *lastNtpBegun; // handy global of if instance exists.
    
    WiFiClient client; // for sending requests
};
#endif //NOT_ARDUINO

#endif // __OmNtp__
