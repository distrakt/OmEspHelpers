#include "OmUdp.h"
#include "OmLog.h"

void OmUdp::wifiStatus(const char *ssid, bool trying, bool failure, bool success)
{
    OMLOG("OmUdp::wifiStatus");
    if(this->portNumber == 0)
    {
        OMLOG("wifi change but port is unset");
        return;
    }
    if (success)
    {
#if 1
        this->udp.begin(this->portNumber);
#else
        // we'll join a multicast club instead, 224.1.1.1:20021
        IPAddress multicastIp;
        multicastIp[0] = 224;
        multicastIp[1] = 1;
        multicastIp[2] = 1;
        multicastIp[3] = 1;
        this->udp.beginMulticast(multicastIp, UDP_PORT);
#endif
        this->udpHere = true;
        OMLOG("udp listening on %d", this->portNumber);
    }
    if (trying || failure)
    {
        this->udpHere = false;
        OMLOG("udp stopped on %d", this->portNumber);
    }
}

// read available udp, return number of bytes if got
// or 0 for nothing
// or -1 for trouble including too big
int OmUdp::checkUdp(uint8_t *data, uint16_t maxPacket, OmUdpPacketInfo *pi)
{
    if (!this->udpHere)
        return 0;
    
    int k = this->udp.parsePacket();
    if (k == 0)
        return 0;
    
    // we have a packet, size k! lets go.dow
//    OMLOG("got udp packet size %d", k);

    if(pi)
    {
        pi->ip = udp.remoteIP();
        pi->port = udp.remotePort();
        pi->size = (uint16_t) k;
    }

    if (k > maxPacket)
    {
        static int tooBigK = 0;
        tooBigK++;
        if(tooBigK < 100 || tooBigK % 200 == 0)
            OMLOG("packet too big got %d max %d", k, maxPacket);
        return -1;
    }
    
    this->udp.read(data, k);
    return k;
}

void OmUdp::sendUdp(IPAddress ipAddress, uint16_t destinationPort, uint8_t *packetBuffer, uint16_t size)
{
    udp.beginPacket(ipAddress, destinationPort); //NTP requests are to port 123
    udp.write(packetBuffer, size);
    udp.endPacket();
}

// static head of linked list
OmUdp *OmUdp::first = NULL;

OmUdp::OmUdp(uint16_t portNumber)
{
    this->portNumber = portNumber;
    this->next = OmUdp::first;
    OmUdp::first = this;
}

