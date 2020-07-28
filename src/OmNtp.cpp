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
static const unsigned int kLocalPort = 2390;      // local port to listen for UDP packets
//const char *kNtpServerName = "time.nist.gov";
const char *kNtpServerName2 = "time.nist.gov";
const char *kNtpServerName1 = "time.google.com";
const char *kNtpServerName3 = "pool.ntp.org";

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
        this->setLocalTimeZone();
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

void OmNtp::setLocalTimeZone()
{
    if(this->timeUrl == 0)
        return;
    if(!this->wifiAvailable)
        return;
    HTTPClient http;
    // NOTE. The SDK's header file marks http.begin(url) as deprecated. But the
    // recommended one with a WiFiClient argument crashes. So I don't use it. dvb 2020-05-30.
#if 0
    Crashes!
//    WiFiClient client; // esp32 for this?
//    http.begin(client, this->timeUrl);
#endif
    http.begin(this->timeUrl);
    int httpCode = http.GET();
    OMLOG("%s: %d", this->timeUrl, httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();
        OMLOG("got: %s", payload.c_str());
        this->localHour = omStringToInt(payload.substring(11,13).c_str()); // String is not the same as std::string, by the way.
        this->localMinute = omStringToInt(payload.substring(14,16).c_str());
        this->localSecond = omStringToInt(payload.substring(17,19).c_str());
        OMLOG("got hms: %d %d %d", this->localHour, this->localMinute, this->localSecond);
        this->localTimeGot = true;
        this->localTimeGotMillis = millis();
        this->localTimeEverGot = true;
    }
    else
    {
        OMLOG("failed to get time from %s", this->timeUrl);
    }
    this->localTimeRefetchCountdown = random(30,80);
}

bool OmNtp::getUTime(uint32_t &uTimeOut, int &uFracOut) // seconds of 1970-1-1, and milliseconds.
{
    uint32_t uTime = this->uTime;

    if(uTime == 0)
    {
        // unknown.
        uTimeOut = 0;
        uFracOut = 0;
        return false;
    }

    // time since our last update...
    int milliDelta = millis() - this->uTimeAcquiredMillis;
    uTime += milliDelta / 1000;
    int uFrac = milliDelta % 1000;

    uTimeOut = uTime;
    uFracOut = uFrac;
    return true;
}

uint32_t OmNtp::getUTime()
{
    uint32_t uTime;
    int uFrac;
    this->getUTime(uTime, uFrac);
    return uTime;
}

bool OmNtp::uTimeToTime(uint32_t uTime, int uFrac, int &hourOut, int &minuteOut, float &secondOut)
{
    uTime += this->timeZone * 3600; // bump the time zone.

    int secondsWithinDay = uTime % 86400;
    int hour = secondsWithinDay / 3600;
    int secondsWithinHour = secondsWithinDay % 3600;
    int minute = secondsWithinHour / 60;
    int second = secondsWithinHour % 60;

    hourOut = hour;
    minuteOut = minute;
    secondOut = second + uFrac / 1000.0;
    return true;
}


bool OmNtp::getTime(int &hourOut, int &minuteOut, float &secondOut)
{
    uint32_t uTime;
    int uFrac;
    this->getUTime(uTime, uFrac);

    if(uTime == 0)
    {
        // unknown.
        hourOut = 0;
        minuteOut = 0;
        secondOut = 0;
        return false;
    }
    this->uTimeToTime(uTime, uFrac, hourOut, minuteOut, secondOut);
    return true;
}

bool OmNtp::getTime(int &hourOut, int &minuteOut, int &secondOut)
{
    float secondF;
    bool result = this->getTime(hourOut, minuteOut, secondF);
    secondOut = (int)secondF;
    return result;
}

const char *OmNtp::getTimeString()
{
    int hour;
    int minute;
    int second;
    bool got;
    got = this->getTime(hour, minute, second);
    static char timeString[10];
    if(got)
        sprintf(timeString, "%02d:%02d:%02d", hour, minute, second);
    else
        sprintf(timeString, "--:--:--");
    return timeString;
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
    int did;
    did = tryOneDnsLookup(kNtpServerName1, this->ntpServerIp);
    if(!did)
        did = tryOneDnsLookup(kNtpServerName2, this->ntpServerIp);
    if(!did)
        did = tryOneDnsLookup(kNtpServerName3, this->ntpServerIp);
    
    if(did != 0)
    {
        this->begun = true;
    }

    OmNtp::lastNtpBegun = this;
}

void OmNtp::checkForPacket()
{
    int cb = udp.parsePacket();
    if (!cb)
    {
        return; // no incoming packet
    }

    unsigned long int now = millis();

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
    unsigned long epoch = secsSince1900 - seventyYears; // classic unix epoch to jan 1 1970.
    OMLOG("Unix time = %d\n", epoch);
    
    int secondsWithinDay = epoch % 86400;
    int hour = secondsWithinDay / 3600;
    int secondsWithinHour = secondsWithinDay % 3600;
    int minute = secondsWithinHour / 60;
    int second = secondsWithinHour % 60;
    
    OMLOG(" UTC: %02d:%02d:%02d\n", hour, minute, second);
    this->hour = hour;
    this->minute = minute;
    this->second = second;
    this->uTime = epoch;
    this->uTimeAcquiredMillis = now;
    OMLOG("Time: %s\n", this->getTimeString());

    // and, if we've recently got a CA time, derive the apparent time zone.
#define TWELVEHOURSMILLIS (12*60*60*1000)
    if(this->localTimeGot && ((now - this->localTimeGotMillis) < TWELVEHOURSMILLIS))
    {
        int uSeconds = this->hour * 3600 + this->minute * 60 + this->second;
        int localSeconds = this->localHour * 3600 + this->localMinute * 60 + this->localSecond;
        OMLOG("utc seconds: %d, ca seconds: %d", uSeconds, localSeconds);

        localSeconds += (now - this->localTimeGotMillis) / 1000;
        
        int diffSeconds = localSeconds - uSeconds;
        if(diffSeconds < 0)
            diffSeconds += (24 * 3600);
        int diffHours = (diffSeconds + 1800) / 3600;
        diffHours = (diffHours + 36) % 24 - 12; // -12 to +12
        this->timeZone = diffHours;
        OMLOG("CA Time Zone appears to be: %d", this->timeZone);
    }
    else
    {
        this->localTimeGot = false;
    }
}

void OmNtp::tick(long milliseconds)
{
    if(!this->wifiAvailable)
        return;

    if(this->ntpRequestSent)
        this->checkForPacket();

    long millisecondsDelta = milliseconds - this->lastMilliseconds;
    this->lastMilliseconds = milliseconds;
    this->countdownMilliseconds -= millisecondsDelta;

    if(this->countdownMilliseconds > 0)
        return;
    
    this->countdownMilliseconds = this->intervalMilliseconds + random(100, 200) * 1000;
    if(this->hour < 0)
        this->countdownMilliseconds = random(10000, 20000); // more frequent til we get the first one.

    if(!this->begun)
        this->begin();

    if(!this->begun)
        return;

    // Interval expired. Time to send out another time query.
    // We do this blindly, and poll for any incoming time packets.
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
    OMLOG("sent ntp request\n", 1);

    // and try the local time zone again. In case of spring forward &c.
    if(!this->localTimeEverGot || (this->localTimeRefetchCountdown-- == 0))
    {
        if(this->timeUrl)
        {
            this->setLocalTimeZone();
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
    if(this->wifiAvailable)
        this->setLocalTimeZone();
}

// end of file
