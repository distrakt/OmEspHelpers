/*
 * This sketch demonstrates using OmEspHelpers to serve a Web UI
 * for controlling the built-in LED. To try it out, put in your
 * own wifi network and password, and watch the serial monitor for
 * it to print out the local IP address. Then browse that IP address.
 */

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

void sliderProc(const char *page, const char *item, int value, int ref1, void *ref2)
{
    value = 100 - value; // output is inverted.
    value = value * 1023 / 100; // map 0..100 onto 0..1023 on esp8266.
    analogWrite(LED_BUILTIN, value);
}

OmWebPages p;
OmWebServer s;

void setup() 
{
  Serial.begin(115200);
  Serial.print("\n\nHello LedController\n");

  p.beginPage("Home");
  p.addButton("ledOn", buttonProc, 0); // ref=0
  p.addButton("ledOff", buttonProc, 1); // ref=1
  p.addButton("ledMomentary", buttonMomentaryProc);
  p.addSlider("ledBrightness", sliderProc);

  s.addWifi("omino warp", "0123456789"); // my home network, no password
  s.setHandler(p);
  s.setStatusLedPin(-1); // tell the server not to blink the led; this app uses it.

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1); // off
}

void loop() 
{
  s.tick(); // in turn calls OmWebPages
  delay(20);
}

