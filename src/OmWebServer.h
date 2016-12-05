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
#include <ESP8266WebServer.h>

#include "OmWebPages.h"

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
    
    /// add to the list of known networks to try.
    void addWifi(String ssid, String password);
    
    void setHandler(OmRequestHandler requestHandler);
    void setHandler(OmWebPages &requestHandler);
    void setStatusCallback(OmConnectionStatus statusCallback);
    
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
    void handleRequest();
};

#endif // __OmWebServer__