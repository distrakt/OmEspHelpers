/*
 * OmNtp.cpp
 * 2016-12-13 dvb
 * Implementation of OmNtp
 */

#include "OmNtp.h"

static const unsigned int kLocalPort = 2390;      // local port to listen for UDP packets
const char *kNtpServerName = "time.nist.gov";

void OmNtp::setWifiAvailable(bool wifiAvailable)
{
    this->wifiAvailable = wifiAvailable;
    
    if(this->wifiAvailable)
        this->countdown = 100;
}

void OmNtp::setTimeZone(int hourOffset)
{
    this->timeZone = hourOffset;
}

bool OmNtp::getTime(int &hourOut, int &minuteOut, int &secondOut)
{
    unsigned long int uTime = this->uTime;
    
    if(uTime == 0)
    {
        // unknown.
        hourOut = 0;
        minuteOut = 0;
        secondOut = 0;
        return false;
    }
    
    // time since our last update...
    uTime += (millis() - this->uTimeAcquiredMillis) / 1000;
    
    int secondsWithinDay = uTime % 86400;
    int hour = secondsWithinDay / 3600;
    int secondsWithinHour = secondsWithinDay % 3600;
    int minute = secondsWithinHour / 60;
    int second = secondsWithinHour % 60;
    
    hour = (hour + this->timeZone + 24) % 24;
    
    hourOut = hour;
    minuteOut = minute;
    secondOut = second;
    return true;
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
        sprintf(timeString, "unknown");
    return timeString;
}

/// Call begin when the wifi is up
void OmNtp::begin()
{
    this->udp.begin(kLocalPort); // Yeah, so, this will fail if you instantiate two OmNtp's. Yup.
    int did = WiFi.hostByName(kNtpServerName, this->ntpServerIp); // And this? Blocking.
    OMLOG("ip for %s: %s\n", kNtpServerName, ipAddressToString(this->ntpServerIp));
    if(did == 1)
    {
        this->begun = true;
    }
}

void OmNtp::checkForPacket()
{
    int cb = udp.parsePacket();
    if (!cb)
    {
        return; // no incoming packet
    }
    
    OMLOG("packet received, length=%d\n", cb);
    
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
    this->uTimeAcquiredMillis = millis();
    OMLOG("Time: %s\n", this->getTimeString());
}

void OmNtp::tick()
{
    if(!this->wifiAvailable)
        return;
    
    if(!this->begun)
        this->begin();
    
    this->checkForPacket();
    
    this->countdown--;
    if(this->countdown > 0)
        return;
    
    this->countdown = this->interval;
    
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
    OMLOG("sent ntp request\n", 1);
}
