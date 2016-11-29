# OmEspHelpers #

This library presents a very simple API for bringing up a web interface on your 
*Arduino ESP8266* project. In its present form it is appropriate for:

* Quick experiments
* Beginner exercises
* Simple projects and products

It is perhaps too limited for serious production, but you can decide.

## Platforms ##

I have only run this library on a Wemos D1 Mini. It is expected to work on other Arduino ESP8266 boards.

## Example ##

Here is an example sketch which brings up a Web Controlled LED.

```
#include "OmEspHelpers.h"
#include "OmWebServer.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

void buttonProc(const char *page, const char *item, int value, int ref1, void *ref2)
{
    digitalWrite(LED_BUILTIN, ref1);
}

void buttonMomentaryProc(const char *page, const char *item, int value, int ref1, void *ref2)
{
    digitalWrite(LED_BUILTIN, !value);
}

OmWebPages p;
OmWebServer s;

void setup() 
{
  p.beginPage("Home");
  p.addButton("ledOn", buttonProc, 0);
  p.addButton("ledOff", buttonProc, 1);
  p.addButton("ledMomentary", buttonMomentaryProc);
  
  s.addWifi("Omino Impulse", ""); // my home network, no password
  s.setHandler(p);
  s.setStatusLedPin(-1); // tell the server not to blink the led; this app uses it.

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() 
{
  s.tick(); // in turn calls OmWebPages
  delay(20);
}
```
