/*
 * OmWebServer.h
 * 2016-11-26 dvb
 *
 * This class implements a web server built on top of
 * the excellent ESP8266WebServer.
 *
 * This code is for Arduino ESP8266, and uses ESP8266WebServer & ESP8266WiFi
 *
 * This class combines the Wifi Setup and retry loop
 * and simplifies the web server API. The result is
 * easier to use, but with less flexibility.
 *
 * It incorporates some feedback and status display on
 * the built-in LED (can be disabled).
 *
 * It also attempts to guide the user with helpful
 * serial messages and default served pages.
 *
 * EXAMPLE
 *
 * OmWebServer w;
 *
 * in setup():   w.addWifi("My Network", "password1234");
 * in loop():    w.tick();
 *
 * The IP address will be printed to the Serial console.
 * The IP address will also be "blinked" on the BUILTIN_LED.
 */

#ifndef __OmWebServer__
#define __OmWebServer__

#include <vector>
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#endif

#include "OmWebPages.h"
#include "OmNtp.h"

typedef const char *(* OmRequestHandler)(const char *request);
typedef void (* OmConnectionStatus)(const char *ssid, bool trying, bool failure, bool success);


class OmWebServerPrivates;

class OmWebServer
{
    OmWebServerPrivates *p = NULL;
    void startWifiTry();
    void maybeStatusCallback(bool trying, bool failure, bool success);
public:
    OmWebServer(int port = 80);
    ~OmWebServer();

    void setVerbose(int verbose); // turn off to print less stuff
    
    /// add to the list of known networks to try.
    void addWifi(String ssid, String password);
    
    void setBonjourName(String bonjourName);
    String getBonjourName();
    
    void setHandler(OmRequestHandler requestHandler);
    void setHandler(OmWebPages &requestHandler);
    void setStatusCallback(OmConnectionStatus statusCallback);
    
    void setNtp(OmNtp *ntp);
    
    /// defaults to 80
    void setPort(int port);
    
    /// changes or disables the blinking status LED. Use -1 to disable.
    void setStatusLedPin(int statusLedPin);
    
    void begin();
    
    /// call this in loop to give time to run.
    void tick();
    
    const char *getSsid();
    int getPort();
    unsigned int getIp();
    int getClientPort();
    unsigned int getClientIp();
    unsigned int getTicks();
    
    /// public for callback purposes, not user-useful
    void handleRequest(String request, WiFiClient &client);

    // connection in progress...
    WiFiClient client;
    long clientStartMillis = 0;
    String request;
    
};

#endif // __OmWebServer__
