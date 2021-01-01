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
#include "OmXmlWriter.h" // for the streaming api

typedef const char *(* OmRequestHandler)(const char *request);
typedef void (* OmConnectionStatus)(const char *ssid, bool trying, bool failure, bool success);

#ifdef LED_BUILTIN
#define OM_DEFAULT_LED LED_BUILTIN
#else
#define OM_DEFAULT_LED -1
#endif

class OmWebServerPrivates;

/*! @brief Manages wifi connection, and forwarding http requests to a handler, typically OmWebPages */
class OmWebServer : public OmIByteStream
{
    OmWebServerPrivates *p = NULL;
    void initiateConnectionTry(String wifi, String password);
    void maybeStatusCallback(bool trying, bool failure, bool success);
public:
    OmWebServer(int port = 80);
    ~OmWebServer();

    /*! @brief OmWebServer by default prints much status to serial; set to 0 to cut that out. */
    void setVerbose(int verbose); // turn off to print less stuff

    /*! must be set before begin(), and cannot be revoked.
    Creates a wifi network access point with the name shown.
    You'll have to communicate the IP address to the user by your
    own means, on screen display or something.
    set "" for no access point.
    NOTE: 2019-12-14 works sometimes. I dont highly recommend. :( */
    void setAccessPoint(String ssid, String password);
    
    /*! @brief add to the list of known networks to try. */
    void addWifi(String ssid, String password);
    /*! @brief reset the list of known networks to try, to empty again. */
    void clearWifis();

    /*! @brief advertises on local network as bonjourName.local */
    void setBonjourName(String bonjourName);
    /*! @brief get the current bonjour name */
    String getBonjourName();
    
    void setHandler(OmRequestHandler requestHandler);

    /*! @brief Introduce an OmWebPages to the server, done and done! */
    void setHandler(OmWebPages &requestHandler);
    /*! @brief receive notifications of changes to wifi status */
    void setStatusCallback(OmConnectionStatus statusCallback);

    /*! @brief Introduce an NTP object to the server */
    void setNtp(OmNtp *ntp);
    
    /*! @brief defaults to 80 */
    void setPort(int port);
    
    /*! @brief changes or disables the blinking status LED. Use -1 to disable. */
    void setStatusLedPin(int statusLedPin);
    
    void end();

    /*! @brief simulate a network trouble. 1==disconnect the wifi */
    void glitch(int k);
    
    /*! @brief You must call this in loop() to give time to run.
     This allows networks to be joined and rejoined, and is when requests are served.
     Call it often!
     return the number of requests handled, if any.
     */
    int tick();

    const char *getSsid();
    int getPort();
    unsigned int getIp();
    int getClientPort();
    unsigned int getClientIp();
    unsigned int getTicks();

    /*! @brief is it? */
    bool isWifiConnected();
    /*! @brief arduino's millis() will overflow after 50 days. Not this baby. */
    long long uptimeMillis();

    bool put(uint8_t) override;
    bool done() override;
    bool put(const char *s); // helpers to send longer amounts & strings
    bool put(uint8_t *d, int size); // helpers to send longer amounts & strings
private:
    /* public for callback purposes, not user-useful */
    void handleRequest(String request, WiFiClient &client);

    /* state machine business. */
    void owsBegin();

    int pollForClient();


};

#endif // __OmWebServer__
