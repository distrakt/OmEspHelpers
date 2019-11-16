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
    OWS_NO_WIFIS, // just sit here, can't do anything.
    OWS_CONNECTING_TO_WIFI,
    OWS_RUNNING
} EOwsState;

/// This class reduces clutter in the public header file.
class OmWebServerPrivates
{
public:
    unsigned int ticks = 0;
    std::vector<String> ssids;
    std::vector<String> passwords;
    String bonjourName;
    EOwsState state = OWS_BEGIN;
    int port = 80;
    OmRequestHandler requestHandler = 0;
    OmWebPages *requestHandlerPages = 0;
    OmConnectionStatus statusCallback = 0;
//    ESP8266WebServer *webServer = 0;
    WiFiServer *wifiServer = 0;
    OmNtp *ntp = 0;
    int wifiIndex = 0; // which wifi currently trying or achieved.
    int requestCount = 0;
    
    int lastWifiStatus = -99;
    
    OmBlinker b = OmBlinker(LED_BUILTIN);
    
    bool doSerial = true;
    int verbose = 2;
    
    void printf(const char *format, ...)
    {
        if(!this->doSerial)  // serial disabled?
            return;
        
        va_list args;
        va_start (args, format);
        static char s[320];
        vsnprintf (s, sizeof(s), format, args);
        s[sizeof(s) - 1] = 0;
        auto t = millis();
        auto tS = t / 1000;
        auto tH = (t / 10) % 100;
        Serial.printf("%4d.%02d (*) OmWebServer.%d: ", tS, tH, this->port);
        Serial.print(s);
    }
};

OmWebServer::OmWebServer(int port)
{
    this->p = new OmWebServerPrivates();
    this->p->port = port;
}

OmWebServer::~OmWebServer()
{
    delete this->p;
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

void OmWebServer::startWifiTry()
{
    if(this->p->ssids.size() == 0)
    {
        this->p->state = OWS_NO_WIFIS;
        return;
    }
    
    this->p->wifiIndex %= this->p->ssids.size();
    this->maybeStatusCallback(true, false, false);
    WiFi.persistent(false);
    WiFi.begin(this->p->ssids[this->p->wifiIndex].c_str(), this->p->passwords[this->p->wifiIndex].c_str());
}

void OmWebServer::begin()
{
    this->p->printf("On port %d\n", this->p->port);
    if(this->p->ssids.size() == 0)
    {
        this->p->state = OWS_NO_WIFIS;
        this->p->printf("No wifis. Try omWebServer->addWifi(ssid, password);\n");
        return;
    }
    
    WiFi.persistent(false);
    WiFi.disconnect();
    if(this->p->wifiServer)
    {
        delete this->p->wifiServer;
        this->p->wifiServer = 0;
    }
//    if(this->p->webServer)
//    {
//        delete this->p->webServer;
//        this->p->webServer = 0;
//    }
    
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
    this->p->state = OWS_CONNECTING_TO_WIFI;
    this->startWifiTry();
    
    // flash the "3-blinks" connecting pattern.
    this->p->b.clear();
    this->p->b.addBlink(1,1);
    this->p->b.addBlink(1,1);
    this->p->b.addBlink(1,1);
    this->p->b.addOffTime(5);
}

//// bindable non-member function, for callback purpose
//static void handleRequest0(OmWebServer *instance)
//{
//    instance->handleRequest();
//}

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


/// call this in loop to give time to run.
void OmWebServer::tick()
{
    long now = millis();
    this->p->ticks++;
    this->p->b.tick();
    if(this->p->ntp)
        this->p->ntp->tick(1); // TODO pass me millis
#if ARDUINO_ARCH_ESP8266
    if(this->p->bonjourName.length())
        MDNS.update();
#endif

    int wifiStatus = WiFi.status();
    if(wifiStatus != this->p->lastWifiStatus)
    {
        this->p->lastWifiStatus = wifiStatus;
        this->p->printf("Wifi status became: %d\n", wifiStatus);
    }
    
    switch(this->p->state)
    {
        case OWS_BEGIN:
        {
            // the very first tick.
            this->begin();
        }
            break;
        case OWS_NO_WIFIS:
            break;
            
        case OWS_CONNECTING_TO_WIFI:
        {
            // Wait for connection
            auto wifiStatus = WiFi.status();
            if(wifiStatus == WL_IDLE_STATUS || wifiStatus == WL_DISCONNECTED)
                break;
            else if(wifiStatus == WL_CONNECTED)
            {
                this->maybeStatusCallback(false, false, true);
                // got ssid
                // maybe bonjour?
                if(this->p->bonjourName.length() > 0)
                {
                    MDNS.begin(this->p->bonjourName.c_str());
                    MDNS.addService("http", "tcp", 80);
                    this->p->printf("Bonjour address: http://%s.local\n", this->p->bonjourName.c_str());
                }
                // start the server.
                this->p->wifiServer = new WiFiServer(this->p->port);
                this->p->wifiServer->begin();
                
//                this->p->webServer = 0;
//                this->p->webServer = new ESP8266WebServer(this->p->port);
//                auto hr0 = std::bind(handleRequest0, this);
//                this->p->webServer->onNotFound(hr0);
//                this->p->webServer->begin();
                this->p->state = OWS_RUNNING;
                this->p->b.clear();
                this->p->b.addNumber(this->getIp() & 0xff); // blink out the last Octet, like 192.168.0.234, blink the 234.
                
                // and tell NTP that they're open for business
                if(this->p->ntp)
                    this->p->ntp->setWifiAvailable(true);
            }
            else
            {
                this->maybeStatusCallback(false, true, false);
                // connection failed, try next known network
                this->p->wifiIndex = (this->p->wifiIndex + 1) % this->p->ssids.size();
                this->startWifiTry();
            }
            break;
        }
            
        case OWS_RUNNING:
        {
            if(wifiStatus != WL_CONNECTED)
            {
                // Lost connection, jump back into loop. Pardon the clunky state machine.
                this->p->state = OWS_BEGIN;
                // and tell NTP that they're gone fishing
                if(this->p->ntp)
                    this->p->ntp->setWifiAvailable(false);
            }
            else
            {
              // do our funny polling.
//              static WiFiClient client;
//              static String request = "";
            
              // maybe kill an open client if it's been a little while
              // You see, this library only manages a single open client at a time. We only ask
              // for a client when the last one we got is no longer active.
              // but Chrome (for example) opens a connection immediately, to make the next
              // action snappier to the user. So here, we just shut that down. Chrome spots it,
              // and initiates a fresh connection _WHEN the TIME comes_.

              #define CLIENT_DEADLINE 300
              if(client || client.connected())
              {
                if(now - this->clientStartMillis > CLIENT_DEADLINE)
                {
                  client.stop();
                  // Serial.printf("\nKILLED CLIENT\n"); // excrutiating verbose
                }
              }
            

              if(!this->client || !this->client.connected())
              {
                client = this->p->wifiServer->available();
                if(client.connected())
                {
                  // Serial.printf("\nNEW CLIENT\n"); // excrutiating verbose
                  request = "";
                  this->clientStartMillis = now;
                }
              }

              if(client && client.connected())
              {
                while(client.available())
                {
                  char c = client.read();
                  // Serial.printf("%c", c); // excrutiating verbose
                  request += c;
                  if(endsWithCrlfCrlf(request))
                  {
                    // looks like the request finished. Ok, so:
                    // Serial.printf("request: %s\n", request.c_str());
                    String url = urlFromRequest(request);
                    // Serial.printf("url: %s\n", url.c_str());
//                    Serial.printf("remoteIp: %s, remotePort: %d\n", ipAddressToString(client.remoteIP()), client.remotePort());

                    handleRequest(url, client); // performs the SEND.
                  }
                }
              }



              
            }

//            this->p->webServer->handleClient();
        }
    }
}

void OmWebServer::setStatusLedPin(int statusLedPin)
{
    this->p->b.setLedPin(statusLedPin);
}

void OmWebServer::handleRequest(String request, WiFiClient &client)
{
    this->p->requestCount++;
    this->p->b.addInterjection(2,2); // blink LED to show incoming


    String uriS = request;
    
    int remotePort = client.remotePort();
    IPAddress remoteIp = client.remoteIP();
    if(this->p->verbose >= 2)
    {
      this->p->printf("Request from %s:%d %s\n", omIpToString(remoteIp), remotePort, uriS.c_str());
    }
    
    String responseS = "HTTP/1.1 200 OK\n"
              "Content-type:text/html\n"
              "Connection: close\n"
              "\n";

    if(this->p->requestHandler)
    {
        const char *response = (this->p->requestHandler)(uriS.c_str());
        responseS += String(response);
    }
    else if(this->p->requestHandlerPages)
    {
        OmRequestInfo ri;
        ri.clientIp = client.remoteIP();
        ri.clientPort = client.remotePort();
        ri.serverIp = this->getIp();
        ri.serverPort = this->getPort();
        ri.bonjourName = this->p->bonjourName.c_str();

        static char response[6500];
        this->p->requestHandlerPages->handleRequest(response, sizeof(response), uriS.c_str(), &ri);
        responseS += String(response);
    }
    else
    {
        responseS += "<pre>\n";
        const char *h1 = "No handler, try omWebServer.setHandler(yourRequestHandlerProc);\n";
        const char *h2 = "   or use OmWebPages with omWebServer.setHandler(omWebPages);\n";
        responseS += "<hr />\n";
        responseS += "OmWebServer\n";
        responseS += h1;
        responseS += h2;
        responseS += "<hr />\n";
        responseS += "</pre>\n";
//        this->p->printf(h1);
//        this->p->printf(h2);
    }

    client.print(responseS);
    //Serial.printf("sending response[%d\]\n\n\n", r.length());
    //Serial.printf("%s", r.c_str());
    client.stop();
    if(this->p->verbose >= 2)
    {
      this->p->printf("Replying %d bytes\n", responseS.length());
    }



    
//    String uriS = this->p->webServer->uri();
//    for(int ix = 0; ix < this->p->webServer->args(); ix++)
//    {
//        if(ix == 0)
//            uriS += '?';
//        else
//            uriS += '&';
//        uriS += this->p->webServer->argName(ix);
//        uriS += '=';
//        uriS += this->p->webServer->arg(ix);
//    }
//    
//    int remotePort = this->p->webServer->client().remotePort();
//    IPAddress remoteIp = this->getClientIp();
//    this->p->printf("Request from %s:%d %s\n", omIpToString(remoteIp), remotePort, uriS.c_str());
//    
//    if(this->p->requestHandler)
//    {
//        const char *response = (this->p->requestHandler)(uriS.c_str());
//        String responseS = String(response);
//        this->p->webServer->send(200, "text/html", responseS);
//        this->p->printf("Replying %d bytes\n", responseS.length());
//    }
//    else if(this->p->requestHandlerPages)
//    {
//        static char response[6500];
//        this->p->requestHandlerPages->handleRequest(response, sizeof(response), uriS.c_str());
//        String responseS = String(response);
//        this->p->webServer->send(200, "text/html", responseS);
//        this->p->printf("Replying %d bytes\n", responseS.length());
//    }
//    else
//    {
//        String s = "<pre>\n";
//        const char *h1 = "No handler, try omWebServer.setHandler(yourRequestHandlerProc);\n";
//        const char *h2 = "   or use OmWebPages with omWebServer.setHandler(omWebPages);\n";
//        s += "<hr />\n";
//        s += "OmWebServer\n";
//        s += h1;
//        s += h2;
//        s += "<hr />\n";
//        s += "</pre>\n";
//        this->p->webServer->send(200, "text/html", s);
//        this->p->printf(h1);
//        this->p->printf(h2);
//    }
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

