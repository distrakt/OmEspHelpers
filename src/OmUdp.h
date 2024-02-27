#ifndef __OmUdp__
#define __OmUdp__

#include <stdint.h>
#include "WiFiUdp.h"

class OmUdpPacketInfo
{
public:
    IPAddress ip;
    uint16_t port;
    uint16_t size;
};
class OmUdp
{
public:
    WiFiUDP udp;
    OmUdp *next = NULL; // we maintain a linked list of all udp instances
    bool udpHere = false;
    uint16_t portNumber = 0;

    OmUdp(uint16_t portNumber); // constructor

    /// called by OmWebServer
    void wifiStatus(const char *ssid, bool trying, bool failure, bool success);

    // read available udp, return number of bytes if got
    // or 0 for nothing
    // or -1 for trouble including too big
    int checkUdp(uint8_t *data, uint16_t maxPacket, OmUdpPacketInfo *pi = NULL);

    void sendUdp(IPAddress ipAddress, uint16_t destinationPort, uint8_t *packetBuffer, uint16_t size);

    static OmUdp *first;
};


#endif // __OmUdp__
