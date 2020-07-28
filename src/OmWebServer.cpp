/*
 * OmWebServer.cpp
 * 2016-11-26 dvb
 * Implementation of OmWebServer
 */

#include "Arduino.h"
#include "OmWebServer.h"
#include "OmBlinker.h"


#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#endif

typedef enum
{
    OWS_BEGIN = 0,
    OWS_NO_WIFIS, // Means no ssid/pwd have been set. Just sit here, can't do anything.
    OWS_CONNECTING_TO_WIFI, // aka trying
    OWS_RUNNING,

    OWS_AP_START,
    OWS_AP_RUNNING,
} EOwsState;

/// This class reduces clutter in the public header file.
class OmWebServerPrivates
{
public:
    unsigned int ticks = 0;
    std::vector<String> ssids;
    std::vector<String> passwords;
    String ssid; // current or attempting ssid.
    String bonjourName;
    EOwsState state = OWS_BEGIN;
    long long wifiTryStartedMillis = 0; // so we know when to time-out on one of the ssid/password combos, and try the next
    int port = 80;
    OmRequestHandler requestHandler = 0;
    OmWebPages *requestHandlerPages = 0;
    OmConnectionStatus statusCallback = 0;
    WiFiServer *wifiServer = 0;
    OmNtp *ntp = 0;
    int wifiIndex = 0; // which wifi currently trying or achieved.
    int requestCount = 0;

    bool streamToClient = false; // set to true when we're open for busines, handling request.
#define STREAM_BLOCK 1200
    uint8_t streamBlock[STREAM_BLOCK + 1];
    int streamBlockIndex = 0;
    int streamCount = 0;

    int lastWifiStatus = -99;
    
    OmBlinker b = OmBlinker(OM_DEFAULT_LED);
    
    bool doSerial = true;
    int verbose = 2;

    long lastMillis;
    long long uptimeMillis;

    // connection in progress...
    WiFiClient client;
    long long clientStartMillis = 0;
    String request;

    bool accessPoint = false; // set to true if an access point is actually running.
    String accessPointSsid;
    String accessPointPassword;
    long long accessPointStartMillis = 0; // set when we fire up the ap, and any client access to prolong it.

    void printf(const char *format, ...)
    {
        if(!this->doSerial)  // serial disabled?
            return;
        
        va_list args;
        va_start (args, format);
        static char s[320];
        vsnprintf (s, sizeof(s), format, args);
        s[sizeof(s) - 1] = 0;
        int t = (int)millis();
        int tS = t / 1000;
        int tH = (t / 10) % 100;
        Serial.printf("%4d.%02d (*) OmWebServer.%d: ", tS, tH, this->port);
        Serial.print(s);
        int k = (int)strlen(s);
        if(s[k - 1] != '\n')
            Serial.print('\n');
    }
};

static OmWebServer *recentOmWebServer = 0; // dumbest trick ever, so plain-C callbacks can access. :-/
static OmWebServerPrivates *recentOmWebServerPrivates = 0;

OmWebServer::OmWebServer(int port)
{
    this->p = new OmWebServerPrivates();
    this->p->port = port;
    this->p->lastMillis = millis();

    recentOmWebServer = this;
    recentOmWebServerPrivates = this->p;
}

OmWebServer::~OmWebServer()
{
    delete this->p;
    recentOmWebServer = 0;
    recentOmWebServerPrivates = 0;
}

void OmWebServer::setAccessPoint(String ssid, String password)
{
    this->p->accessPoint = false; // will be set true if running
    this->p->accessPointSsid = ssid;
    this->p->accessPointPassword = password;
}


void OmWebServer::setBonjourName(String bonjourName)
{
    this->p->bonjourName = bonjourName;
}

String OmWebServer::getBonjourName()
{
    return this->p->bonjourName;
}

/// add to the list of known networks to try.
void OmWebServer::addWifi(String ssid, String password)
{
    this->p->ssids.push_back(ssid);
    this->p->passwords.push_back(password);
}

/// empty the wifis list.
void OmWebServer::clearWifis()
{
    this->p->ssids.clear();
    this->p->passwords.clear();
}

void OmWebServer::setHandler(OmRequestHandler requestHandler)
{
    this->p->requestHandler = requestHandler;
}

void OmWebServer::setHandler(OmWebPages &requestHandlerPages)
{
    this->p->requestHandlerPages = &requestHandlerPages;
//    requestHandlerPages.setOmWebServer(this);
}


void OmWebServer::setStatusCallback(OmConnectionStatus statusCallback)
{
    this->p->statusCallback = statusCallback;
}

/// defaults to 80
void OmWebServer::setPort(int port)
{
    this->p->port = port;
}

void OmWebServer::maybeStatusCallback(bool trying, bool failure, bool success)
{
    // Print our messages (unless disabled), and then call the user proc if any.
    const char *ssid = this->getSsid();
    
    if(trying)
        this->p->printf("Attempting to join wifi network \"%s\"\n", ssid);
    if(failure)
        this->p->printf("Failed to join wifi network \"%s\"\n", ssid);
    if(success)
    {
        this->p->printf("Joined wifi network \"%s\"\n", ssid);
        this->p->printf("Accessible at http://%s:%d\n", omIpToString(this->getIp()), this->getPort());
    }
    
    if(this->p->statusCallback)
    {
        (this->p->statusCallback)(this->getSsid(), trying, failure, success);
    }
}

void OmWebServer::initiateConnectionTry(String wifi, String password)
{
    if(this->p->ssids.size() == 0)
    {
        this->p->state = OWS_NO_WIFIS;
        return;
    }
    
    WiFi.persistent(false);
    WiFi.begin(wifi.c_str(), password.c_str());
    this->p->ssid = wifi;

    this->p->state = OWS_CONNECTING_TO_WIFI;
    this->p->wifiTryStartedMillis = this->p->uptimeMillis;
    this->maybeStatusCallback(true, false, false);
}

void OmWebServer::owsBegin()
{
    this->p->printf("On port %d\n", this->p->port);

    WiFi.persistent(false);
    WiFi.disconnect();
    if(this->p->ntp)
        this->p->ntp->setWifiAvailable(false);

    this->p->accessPoint = false; // if it was an access point, it is not any more.
    this->p->wifiIndex = 0; // start the wifi-search loop.

    // Note -- we used to "delete this->p->wifiServer;" if there was one
    // but I found it works better to just keep the old one if any, to
    // persist across wifi/ssid reconnects. Deleting and Newing crashed.

    // Normal station mode, connect to a netowrk and keep working.
    if(this->p->ssids.size() == 0)
    {
        // no ssids, so we'll try an access point.
        if(this->p->accessPointSsid.length())
        {
            this->p->state = OWS_AP_START;
        }
        else
        {
            this->p->state = OWS_NO_WIFIS;
            this->p->printf("No wifis. Try omWebServer->addWifi(ssid, password); or ->setAccessPoint(ssid, password);\n");
        }
        return;
    }

    WiFi.mode(WIFI_STA);
    if(this->p->bonjourName.length() > 0)
    {
#if ARDUINO_ARCH_ESP8266
        WiFi.hostname(this->p->bonjourName.c_str());
#endif
#if ARDUINO_ARCH_ESP32
        WiFi.setHostname(this->p->bonjourName.c_str());
#endif
    }

    // and fire up the first attempt. (later attempts will stay on OWS_CONNECTING_TO_WIFI state.)
    this->p->wifiIndex = 0;
    this->initiateConnectionTry(this->p->ssids[this->p->wifiIndex], this->p->passwords[this->p->wifiIndex]);
    this->p->state = OWS_CONNECTING_TO_WIFI;

    // flash the "3-blinks" connecting pattern.
    this->p->b.clear();
    this->p->b.addBlink(1,1);
    this->p->b.addBlink(1,1);
    this->p->b.addBlink(1,1);
    this->p->b.addOffTime(5);

}

void OmWebServer::end()
{
    WiFi.persistent(false);
    WiFi.disconnect();
    if(this->p->wifiServer)
    {
        delete this->p->wifiServer;
        this->p->wifiServer = 0;
    }
}

void OmWebServer::glitch(int k)
{
    // simulate a network trouble.
    switch(k)
    {
        case 1:
            WiFi.disconnect();
            break;
    }

}

static bool endsWithCrlfCrlf(String &s)
{
  int len = (int)s.length();
  if(len < 4)
    return false;
  if(s[len-4] == 13 && s[len-3] == 10 && s[len-2] == 13 && s[len-1] == 10)
    return true;
  return false;
}

static String urlFromRequest(String r)
{
  String url;
  int len = (int)r.length();
  int ix = 0;
  bool firstSp = false;
  while(ix < len)
  {
    char c = r[ix++];
    if(c == ' ')
    {
      if(firstSp)
        return url;
      else
        firstSp = true;
    }
    else if(firstSp)
      url += c;
  }
  return url;
}

/*
 Handy cheat sheet
 typedef enum {
 WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
 WL_IDLE_STATUS      = 0,
 WL_NO_SSID_AVAIL    = 1,
 WL_SCAN_COMPLETED   = 2,
 WL_CONNECTED        = 3,
 WL_CONNECT_FAILED   = 4,
 WL_CONNECTION_LOST  = 5,
 WL_DISCONNECTED     = 6
 } wl_status_t;
*/

const char *statusString(int wifiStatus)
{
#define SSCASE(_x) if(wifiStatus == _x) return #_x;
    SSCASE(WL_NO_SHIELD);
    SSCASE(WL_NO_SHIELD);
    SSCASE(WL_IDLE_STATUS);
    SSCASE(WL_NO_SSID_AVAIL);
    SSCASE(WL_SCAN_COMPLETED);
    SSCASE(WL_CONNECTED);
    SSCASE(WL_CONNECT_FAILED);
    SSCASE(WL_CONNECTION_LOST);
    SSCASE(WL_DISCONNECTED);
    return "?";
#undef SSCASE
}

void OmWebServer::pollForClient()
{
    // (TODO this could be a method on OmWebServerPrivates, to keep it inside.)

    // do our funny polling.

    // maybe kill an open client if it's been a little while
    // You see, this library only manages a single open client at a time. We only ask
    // for a client when the last one we got is no longer active.
    // but Chrome (for example) opens a connection immediately, to make the next
    // action snappier to the user. So here, we just shut that down. Chrome spots it,
    // and initiates a fresh connection _WHEN the TIME comes_.

#define CLIENT_DEADLINE 300
    if(this->p->client || this->p->client.connected())
    {
        if(this->p->uptimeMillis - this->p->clientStartMillis > CLIENT_DEADLINE)
        {
            this->p->client.stop();
            // Serial.printf("\nKILLED CLIENT\n"); // excrutiating verbose
        }
    }


    if(!this->p->client || !this->p->client.connected())
    {
        this->p->client = this->p->wifiServer->available();
        if(this->p->client.connected())
        {
            // Serial.printf("\nNEW CLIENT\n"); // excrutiating verbose
            this->p->request = "";
            this->p->clientStartMillis = this->p->uptimeMillis;
            this->p->accessPointStartMillis = this->p->clientStartMillis; // keep the AP mode alive longer, it is in use.
        }
    }

    if(this->p->client && this->p->client.connected())
    {
        while(this->p->client.available())
        {
            char c = this->p->client.read();
            // Serial.printf("%c", c); // excrutiating verbose
            this->p->request += c;
            if(endsWithCrlfCrlf(this->p->request))
            {
                // looks like the request finished. Ok, so:
                String url = urlFromRequest(this->p->request);

                this->handleRequest(url, this->p->client); // performs the SEND.
            }
        }
    }
}

#ifdef ARDUINO_ARCH_ESP32
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Event: client connected to access point");
    if(recentOmWebServerPrivates)
        recentOmWebServerPrivates->accessPointStartMillis = recentOmWebServerPrivates->uptimeMillis; // reset the timeout
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("Event: client disconnected from access point");
}
#endif

/// call this in loop to give time to run.
void OmWebServer::tick()
{
    {
        long nowMillis = millis();
        long deltaMillis = nowMillis - this->p->lastMillis;;
        this->p->uptimeMillis += deltaMillis;
        this->p->lastMillis = nowMillis;
    }

    this->p->ticks++;
    this->p->b.tick();
    if(this->p->ntp)
        this->p->ntp->tick(this->p->lastMillis);
#if ARDUINO_ARCH_ESP8266
    if(this->p->bonjourName.length())
        MDNS.update();
#endif

    int wifiStatus = WiFi.status();
    if(wifiStatus != this->p->lastWifiStatus)
    {
        this->p->lastWifiStatus = wifiStatus;
        this->p->printf("Wifi status became: %d:%s\n", wifiStatus, statusString(wifiStatus));
    }
    
    switch(this->p->state)
    {
        case OWS_BEGIN:
        {
            // the very first tick.
            this->owsBegin();
            break;
        }

        case OWS_NO_WIFIS:
            break;
            
        case OWS_CONNECTING_TO_WIFI:
        {
            // Wait for connection
            auto wifiStatus = WiFi.status();
            if(wifiStatus == WL_CONNECTED)
            {
                // hooray, we've achieved connection.
                this->maybeStatusCallback(false, false, true); // tell the world.
                // got ssid
                // maybe bonjour?
                if(this->p->bonjourName.length() > 0)
                {
                    MDNS.begin(this->p->bonjourName.c_str());
                    MDNS.addService("http", "tcp", 80);
                    this->p->printf("Bonjour address: http://%s.local\n", this->p->bonjourName.c_str());
                }
                // start the server, if there isnt one.
                if(!this->p->wifiServer)
                {
                    this->p->wifiServer = new WiFiServer(this->p->port);
                    this->p->wifiServer->begin();
                }

                this->p->b.clear();
                this->p->b.addNumber(this->getIp() & 0xff); // blink out the last Octet, like 192.168.0.234, blink the 234.
                
                // and tell NTP that they're open for business
                if(this->p->ntp)
                    this->p->ntp->setWifiAvailable(true);

                // Enter the Running State. This is good.
                this->p->state = OWS_RUNNING;
            }
            else
            {
                // we're not connected. if it's been enough time, try the next wifi.
                // We only care about WL_CONNECTED vs every other status. The sequence of status
                // changes isn't rigorously followed the same between ESP32 and EPS8266, so we do timeout instead.
                int howLong = this->p->uptimeMillis - this->p->wifiTryStartedMillis;
                if(howLong > 7000)
                {
                    this->maybeStatusCallback(false, true, false);
                    // connection failed, try next known network
                    this->p->wifiIndex += 1;
                    // if we've tried them all, AND there's an access point, go do that.
                    if(this->p->wifiIndex >= (int)this->p->ssids.size() && this->p->accessPointSsid.length())
                    {
                        this->p->state = OWS_AP_START;
                    }
                    else
                    {
                        this->p->wifiIndex %= this->p->ssids.size();
                        this->initiateConnectionTry(this->p->ssids[this->p->wifiIndex], this->p->passwords[this->p->wifiIndex]);
                    }
                }
            }
            break;
        }

        case OWS_RUNNING:
        {
            if(wifiStatus != WL_CONNECTED && !this->p->accessPoint)
            {
                // Lost connection, jump back into loop. Pardon the clunky state machine.
                this->p->state = OWS_BEGIN;
                // and tell NTP that they're gone fishing
                if(this->p->ntp)
                    this->p->ntp->setWifiAvailable(false);
            }
            else
            {
                this->pollForClient();
            }
            break;
        }

        case OWS_AP_START:
        {
            if(this->p->accessPointSsid.length())
            {
                // beginning is easy for access point, there's no further state to consider.
                WiFi.disconnect();
                delay(200);
                WiFi.mode(WIFI_AP);
                delay(200);
                WiFi.disconnect();
                delay(200);
                WiFi.mode(WIFI_AP);
                WiFi.softAP(this->p->accessPointSsid.c_str(), this->p->accessPointPassword.length() ? this->p->accessPointPassword.c_str() : 0);
#ifdef ARDUINO_ARCH_ESP32
                WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_AP_STACONNECTED);
                WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_AP_STADISCONNECTED);
#endif
                IPAddress apIp = WiFi.softAPIP();
                this->p->printf("Access Point IP Address: %s\n", omIpToString(apIp, true));
                // instantiate right here, now.
                this->p->wifiServer = new WiFiServer(this->p->port);
                this->p->wifiServer->begin();
                this->p->state = OWS_RUNNING;
                this->p->printf("Access point name:     %s\n", this->p->accessPointSsid.c_str());
                this->p->printf("Access point password: %s\n", this->p->accessPointPassword.length() ? this->p->accessPointPassword.c_str() : "<none>");
                this->p->state = OWS_AP_RUNNING;
                this->p->accessPoint = true;

                this->p->accessPointStartMillis = this->p->uptimeMillis; // for timing out the Access Point mode.
            }
            else
            {
                // shouldnt have got here, entered AP_START but no ap name given. so.
                this->p->state = OWS_NO_WIFIS;
            }
            break;
        }

        case OWS_AP_RUNNING:
        {
            this->pollForClient();
            // TODO: if no connections for a while, go back to OWS_BEGIN to reestablish station mode.
            // this for an appliance that's connected, then the wifi fails a while, but comes back.
            // no need to lapse into permanant AP mode.

#define AP_TIMEOUT_SECONDS 120
            // after AP_TIMEOUT_SECONDS, go ahead and retry the available wifis, if any.
            if(this->p->ssids.size())
            {
                int secondsSinceLastClientAccess = (this->p->uptimeMillis - this->p->accessPointStartMillis) / 1000;
                if(secondsSinceLastClientAccess > AP_TIMEOUT_SECONDS)
                {
                    this->p->printf("Access point %s unused for %d seconds; retrying wifis", this->p->accessPointSsid.c_str(), AP_TIMEOUT_SECONDS);
                    this->p->state = OWS_BEGIN;
                }
            }
            break;
        }
    }
}

void OmWebServer::setStatusLedPin(int statusLedPin)
{
    this->p->b.setLedPin(statusLedPin);
}

bool OmWebServer::put(uint8_t ch)
{
    // our streaming implementation.
    // used entirely for URL transmission.

    bool result = true;
    if(this->p->streamToClient)
    {
        this->p->streamCount++;
        this->p->streamBlock[this->p->streamBlockIndex++] = ch;
        if(this->p->streamBlockIndex == STREAM_BLOCK)
        {
            this->p->client.write(this->p->streamBlock, this->p->streamBlockIndex);
            this->p->streamBlockIndex = 0;
        }
    }
    else
        result = false;

    return result;
}

bool OmWebServer::put(const char *s)
{
    bool result = true;
    while(*s)
        result &= this->put(*s++);
    return result;
}
bool OmWebServer::put(uint8_t *d, int size)
{
    bool result = true;
    while(size-- > 0)
        result &= this->put(*d++);
    return result;
}


bool OmWebServer::done()
{
    this->p->client.write(this->p->streamBlock, this->p->streamBlockIndex);
    this->p->streamBlockIndex = 0;
    return true;
}

void OmWebServer::handleRequest(String request, WiFiClient &client)
{
    // TODO: this could be on OmWebServerPrivates, to hide fully.
    this->p->requestCount++;
    this->p->b.addInterjection(2,2); // blink LED to show incoming


    String uriS = request;
    
    int remotePort = client.remotePort();
    IPAddress remoteIp = client.remoteIP();
    if(this->p->verbose >= 2)
    {
      this->p->printf("Request from %s:%d %s\n", omIpToString(remoteIp), remotePort, uriS.c_str());
    }

    this->p->streamToClient = true; // streaming available!
    this->p->streamBlockIndex = 0;
    this->p->streamCount = 0;

    if(this->p->requestHandler)
    {
        // raw handler is old-fashioned. Should let client
        // determine the response code & type. should let it stream, not
        // require a const char * return.
        // TODO reconsider. dvb2019-11
        this->put("HTTP/1.1 200 OK\n"
                              "Content-type:text/html\n"
                              "Connection: close\n"
                              "\n");

        const char *response = (this->p->requestHandler)(uriS.c_str());
        this->put(response);
    }
    else if(this->p->requestHandlerPages)
    {
        OmRequestInfo ri;
        ri.clientIp = client.remoteIP();
        ri.clientPort = client.remotePort();
        ri.serverIp = this->getIp();
        ri.serverPort = this->getPort();
        ri.bonjourName = this->p->bonjourName.c_str();
        ri.uptimeMillis = this->p->uptimeMillis;
        if(this->p->accessPoint)
            ri.ap = this->p->accessPointSsid.c_str();
        else
            ri.ssid = this->getSsid();

        // callee needs to render the response header, content type, 200, &c.
        this->p->requestHandlerPages->handleRequest(this, uriS.c_str(), &ri); // first "this" is us as an OmIByteStream.
    }
    else
    {
        this->put("<pre>\n");
        const char *h1 = "No handler, try omWebServer.setHandler(yourRequestHandlerProc);\n";
        const char *h2 = "   or use OmWebPages with omWebServer.setHandler(omWebPages);\n";
        this->put("<hr />\n");
        this->put("OmWebServer\n");
        this->put(h1);
        this->put(h2);
        this->put("<hr />\n");
        this->put("</pre>\n");
    }

    this->done(); // flush the stream.
    client.stop();
    this->p->streamToClient = false;
    if(this->p->verbose >= 2)
    {
      this->p->printf("Replying %d bytes\n", this->p->streamCount);
    }
}

const char *OmWebServer::getSsid()
{
    if(this->p->state == OWS_BEGIN || this->p->state == OWS_NO_WIFIS)
        return "";
    return this->p->ssids[this->p->wifiIndex].c_str();
}

int OmWebServer::getPort()
{
    return this->p->port;
}

unsigned int OmWebServer::getIp()
{
    IPAddress localIp = WiFi.localIP();
    if(this->p->accessPoint)
    {
        // we're serving a wifi network, as an access point. Return our
        // ip address for that then.
        localIp = WiFi.softAPIP();
    }
    unsigned int localIpInt = (localIp[0] << 24) | (localIp[1] << 16) | (localIp[2] << 8) | (localIp[3] << 0);
    return localIpInt;
}

//int OmWebServer::getClientPort()
//{
//    return this->p->webServer->client().remotePort();
//}

//unsigned int OmWebServer::getClientIp()
//{
//    IPAddress clientIp = this->p->webServer->client().remoteIP();
//    unsigned int clientIpInt = (clientIp[0] << 24) | (clientIp[1] << 16) | (clientIp[2] << 8) | (clientIp[3] << 0);
//    return clientIpInt;
//}

unsigned int OmWebServer::getTicks()
{
    return this->p->ticks;
}

void OmWebServer::setNtp(OmNtp *ntp)
{
    this->p->ntp = ntp;
}

void OmWebServer::setVerbose(int verbose)
{
  this->p->verbose = verbose;
}

bool OmWebServer::isWifiConnected()
{
    bool result = this->p->state == OWS_RUNNING;
    return result;
}

long long OmWebServer::uptimeMillis()
{
    return this->p->uptimeMillis;
}

