/*
 * OmNtp.cpp
 * 2016-12-13 dvb
 * Implementation of OmNtp
 */

#include "OmNtp.h"
#include "OmUtil.h"
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266HTTPClient.h>
#endif
#ifdef ARDUINO_ARCH_ESP32
#include <HTTPClient.h>
#endif

#include "math.h"
#include "stdio.h"

#define MINUTES_PER_NTP_POLL 10 // minutes between ntp queries, randomized a little
#define NTP_PER_TIMEURL_POLL 10 // every N ntp polls, also do a timeurl poll

// fumod is handy. fumod(-5,20) is 15.
float fumod(float a, float b)
{
    if(b == 0)
        return a;

    float x = floor(a / b);
    x *= b;
    float r = a - x;
    return r;
}

#define SECONDS_IN_HOUR (60 * 60)
#define SECONDS_IN_DAY (60 * 60 * 24)

OmNtpSyncRecord::OmNtpSyncRecord(unsigned long nowMs, int hour, int minute, int second)
{
    this->acquired = true;
    this->acquiredMs = nowMs;
    this->reportedSeconds = hour * 3600 + minute * 60 + second;
}

float OmNtpSyncRecord::getSwd(unsigned long nowMs, int zone)
{
    float result = -1;
    if(this->acquired)
    {
        result = (nowMs - this->acquiredMs) / 1000.0 + this->reportedSeconds + zone * 3600.0;
        result = fumod(result, SECONDS_IN_DAY);
    }
    return result;
}

double OmNtpSyncRecord::getU(unsigned long nowMs)
{
    double result = -1;
    if(this->acquired)
    {
        result = (nowMs - this->acquiredMs) / 1000.0 + this->reportedSeconds;
    }
    return result;
}


float omGetSecondsWithinDay(unsigned long nowMs, OmNtpSyncRecord &utcTime, OmNtpSyncRecord &localTime, int zone)
{
    // here we can have either or both of utcTime (from ntp) and localTime (from a simple server).
    // we presume that ntp is more accurate, but localTime carries the correct hour.
    // if local time is missing, we use the zone value and add it.
    float utcSwd = utcTime.getSwd(nowMs, zone);
    float localSwd = localTime.getSwd(nowMs); // localtime is presumed already in-zone.
    if(utcSwd >= 0 && localSwd >= 0)
    {
        // trickiest case. we reconcile the two.
        float utcSwh = fumod(utcSwd, SECONDS_IN_HOUR);
        float localSwh = fumod(localSwd, SECONDS_IN_HOUR);
        float correctionSeconds = utcSwh - localSwh; // can range (-SECONDS_IN_HOUR, +SECONDS_IN_HOUR)
        // this is the tricky bit. Since we're taking the "hours" from local time, we
        // do have to assume that the minutes are not more than a half hour wrong from each other.
        // in practice it's always less than a second. but tricky things happen at hour crossings.
        // SO, we ensure the correction is at most + or - a half of an hour.
        if(correctionSeconds < -(SECONDS_IN_HOUR/2))
            correctionSeconds += SECONDS_IN_HOUR;
        else if(correctionSeconds > +(SECONDS_IN_HOUR/2))
            correctionSeconds -= SECONDS_IN_HOUR;
        localSwd += correctionSeconds;
        localSwd = fumod(localSwd, SECONDS_IN_DAY);
    }
    else if(utcSwd >= 0)
    {
        // we have only the utc time (which has zone already added)
        localSwd = utcSwd;
    }
    // else, either we have localSwd (yay) or we don't (boohoo) and it's -1.
    return localSwd;
}

bool omGetHms(float secondsWithinDay, int &hourOut, int &minuteOut, float &secondOut)
{
    bool result;
    if(secondsWithinDay < 0)
    {
        result = false;
        hourOut = -1;
        minuteOut = -1;
        secondOut = -1;
    }
    else
    {
        result = true;
        int secondsWithinDayI = floor(secondsWithinDay);
        hourOut = secondsWithinDayI / SECONDS_IN_HOUR;
        minuteOut = (secondsWithinDayI / 60) % 60;
        secondOut = fumod(secondsWithinDay, 60);
    }
    return result;
}

bool omGetHms(float secondsWithinDay, int &hourOut, int &minuteOut, int &secondOut)
{
    float secondF;
    bool result = omGetHms(secondsWithinDay, hourOut, minuteOut, secondF);
    secondOut = secondF;
    return result;
}

const char *omGetHmsString(float secondsWithinDay)
{
    int hour;
    int minute;
    int second;
    bool got;
    got = omGetHms(secondsWithinDay, hour, minute, second);
    static char timeString[10];
    if(got)
        sprintf(timeString, "%02d:%02d:%02d", hour, minute, second);
    else
        sprintf(timeString, "--:--:--");
    return timeString;
}

#ifndef NOT_ARDUINO
// the actual class, including arduino-specific UDP and URL code.

static const unsigned int kLocalPort = 2390;      // local port to listen for UDP packets
//const char *kNtpServerName = "time.nist.gov";

const char *ntpServerNames[] =
{
    "time.nist.gov",
    "time.google.com",
    "pool.ntp.org"
};
#define ARRAYCOUNT(_a) (sizeof(_a) / sizeof(_a[0]))

OmNtp::OmNtp()
{
    OmNtp::lastNtpBegun = this;
}

void OmNtp::setWifiAvailable(bool wifiAvailable)
{
    this->wifiAvailable = wifiAvailable;
    
    if(this->wifiAvailable)
    {
        this->countdownMilliseconds = 67;
        this->getLocalTime();
    }
}

void OmNtp::setTimeZone(int hourOffset)
{
    this->timeZone = hourOffset;
}

int OmNtp::getTimeZone()
{
    return this->timeZone;
}

void OmNtp::getLocalTime()
{
    if(this->timeUrl == 0)
    {
        this->localTime.acquired = false;
        return;
    }

    if(!this->wifiAvailable)
        return;

    HTTPClient http;
    // NOTE. The SDK's header file marks http.begin(url) as deprecated. But the
    // recommended one with a WiFiClient argument crashes. So I don't use it. dvb 2020-05-30.
#if 1
//    Crashes!
    http.begin(client, this->timeUrl);
#else
    http.begin(this->timeUrl);
#endif

    unsigned long t0 = millis();
    int httpCode = http.GET();
    unsigned long t1 = millis();
    OMLOG("%s: %d", this->timeUrl, httpCode);
    this->stats.timeUrlRequestsSent++;
    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        int localHour = omStringToInt(payload.substring(11,13).c_str()); // String is not the same as std::string, by the way.
        int localMinute = omStringToInt(payload.substring(14,16).c_str());
        int localSecond = omStringToInt(payload.substring(17,19).c_str());

        unsigned long now = t1 + ((t1 - t0) / 2); // account for round trip time
        OMLOG("got: %s", payload.c_str());
        OMLOG("got hms: %d %d %d", localHour, localMinute, localSecond);
        OMLOG("get roundrip: %ld", t1 - t0);

        localHour = (localHour + this->timeUrlOffset) % 24; // NOTE: if we were getting the date, this could make the date wrong.
        this->localTime = OmNtpSyncRecord(now, localHour, localMinute, localSecond);
        this->stats.timeUrlRequestsAnswered++;
        this->stats.timeUrlRequestMostRecentMillis = this->localTime.acquiredMs;
    }
    else
    {
        OMLOG("failed to get time from %s", this->timeUrl);
    }

    // when to try again, as a fraction of ntp-attempts
    // the overall interval is randomized, so no need to dither here
    // the overall checks are around every 5-10 minutes; these are a divider of that.
    // at 10, it will catch a time change within 2 hours. ish.
    if(this->localTime.acquired)
        this->localTimeRefetchCountdown = NTP_PER_TIMEURL_POLL; // we got it once, we can wait a long time before looking again
    else
        this->localTimeRefetchCountdown = 2; // we really want to have it when we can please.
}

bool OmNtp::getUTime(uint32_t &uTimeOut, int &uFracOut) // seconds of 1970-1-1, and milliseconds.
{
    unsigned long now = millis();
    double uTimeF = this->ntpTime.getU(now);
    uint32_t uTime = 0;
    int uMillis = 0;
    bool result = false;

    if(uTimeF >= 0)
    {
        uTime = uTimeF;
        uMillis = (uTimeF - uTime) * 1000;
        result = true;
    }

    uTimeOut = uTime;
    uFracOut = uMillis;
    return result;
}

uint32_t OmNtp::getUTime()
{
    uint32_t uTime;
    int uFrac;
    this->getUTime(uTime, uFrac);
    return uTime;
}

bool OmNtp::getTime(int &hourOut, int &minuteOut, float &secondOut)
{
    float swd = omGetSecondsWithinDay(millis(), this->ntpTime, this->localTime, this->timeZone);
    bool result = omGetHms(swd, hourOut, minuteOut, secondOut);
    return result;
}

bool OmNtp::getTime(int &hourOut, int &minuteOut, int &secondOut)
{
    float swd = omGetSecondsWithinDay(millis(), this->ntpTime, this->localTime, this->timeZone);
    bool result = omGetHms(swd, hourOut, minuteOut, secondOut);
    return result;
}

const char *OmNtp::getTimeString()
{
    float swd = omGetSecondsWithinDay(millis(), this->ntpTime, this->localTime, this->timeZone);
    const char *result = omGetHmsString(swd);
    return result;
}

static int tryOneDnsLookup(const char *serverName, IPAddress &ipAddressOut)
{
    int did = WiFi.hostByName(serverName, ipAddressOut); // And this? Blocking.
    OMLOG("(aok=%d) ip for %s: %s\n", did, serverName, ipAddressToString(ipAddressOut));
    
    return did;
}

/// Call begin when the wifi is up
void OmNtp::begin()
{
    this->udp.begin(kLocalPort); // Yeah, so, this will fail if you instantiate two OmNtp's. Yup.
    int did = false;

    for(const char *ntpServerName : ntpServerNames)
    {
        did = tryOneDnsLookup(ntpServerName, this->ntpServerIp);
        if(did)
        {
            this->stats.ntpServerName = ntpServerName;
            this->stats.ntpServerIp = this->ntpServerIp;
            break;
        }
    }

    if(did != 0)
    {
        this->begun = true;
    }

    OmNtp::lastNtpBegun = this;
}

void OmNtp::checkForPacket()
{
    int cb = this->udp.parsePacket();
    if (!cb)
    {
        return; // no incoming packet
    }

    unsigned long int now = millis();

    this->stats.ntpRequestsAnswered++;
    this->stats.ntpRequestMostRecentMillis = now;

    OMLOG("packet received, length=%d\n", cb);
    this->ntpRequestSent = 0;
    
    // We've received a packet, read the data from it
    udp.read(packetBuffer, kNtpPacketSize); // read the packet into the buffer
    
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:
    
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    OMLOG("Seconds since Jan 1 1900 = %d\n", secsSince1900);
    
    const unsigned long seventyYears = 2208988800UL;
    unsigned long secsSince1970 = secsSince1900 - seventyYears; // relative to unix epoch jan 1 1970 UTC.
    OMLOG("Unix time = %d\n", secsSince1970);
    
    int secondsWithinDay = secsSince1970 % 86400;
    int hour = secondsWithinDay / 3600;
    int secondsWithinHour = secondsWithinDay % 3600;
    int minute = secondsWithinHour / 60;
    int second = secondsWithinHour % 60;
    
    OMLOG(" UTC: %02d:%02d:%02d\n", hour, minute, second);

    this->ntpTime = OmNtpSyncRecord(now, hour, minute, second);
    OMLOG("Time: %s\n", this->getTimeString());
}

void OmNtp::tick(unsigned int deltaMillis)
{
    if(!this->wifiAvailable)
        return;

    if(this->ntpRequestSent)
        this->checkForPacket();

    this->countdownMilliseconds -= deltaMillis;

    if(this->countdownMilliseconds > 0)
        return;

    // +--------------------------------
    // | Coarsely we only do anything on the countdown.

    this->countdownMilliseconds = MINUTES_PER_NTP_POLL * 60 * 1000 + random(0,90*1000); // jittered by some seconds.
    if((!this->ntpTime.acquired) && (!this->localTime.acquired))
        this->countdownMilliseconds = random(10000, 20000); // more frequent til we get the first one.

    if(!this->begun)
        this->begin();

    if(!this->begun)
        return;

    // +-----------------------------
    // | Interval expired. Time to send out another time query.
    // | We do this blindly, and poll for any incoming time packets.
    // | Send out a utp packet
    memset(packetBuffer, 0, kNtpPacketSize);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;
    
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(this->ntpServerIp, 123); //NTP requests are to port 123
    udp.write(packetBuffer, kNtpPacketSize);
    udp.endPacket();
    this->ntpRequestSent = 1;
    this->stats.ntpRequestsSent++;
    OMLOG("sent ntp request\n", 1);

    // and try the local time zone again. In case of spring forward &c.
    if((!this->localTime.acquired) || (this->localTimeRefetchCountdown-- <= 0))
    {
        if(this->timeUrl)
        {
            this->getLocalTime();
            OMLOG("refreshing local time zone %s", this->timeUrl);
        }
    }
}

OmNtp *OmNtp::lastNtpBegun = NULL;

OmNtp *OmNtp::ntp()
{
    return OmNtp::lastNtpBegun;
}

void OmNtp::setTimeUrl(const char *timeUrl)
{
    this->timeUrl = timeUrl;
    this->stats.timeUrl = this->timeUrl;
    if(this->wifiAvailable)
        this->getLocalTime();
}

void OmNtp::setTimeUrlOffset(int hoursFromTimeUrl)
{
    this->timeUrlOffset = hoursFromTimeUrl;
    this->getLocalTime();
}

void OmNtp::sGetTimeOfDay(int &minuteWithinDayOut, float &secondWithinMinuteOut)
{
    OmNtp *ntp = OmNtp::ntp();
    if(ntp)
    {
        int h,m;
        ntp->getTime(h, m, secondWithinMinuteOut);
        minuteWithinDayOut = h * 60 + m;
    }
    else
    {
        minuteWithinDayOut = -1;
        secondWithinMinuteOut = -1;
    }
}

#endif
// end of file
