
/*
 * 2016-11-26 dvb
 * Work on OmServer, so that its messages tend to
 * help a new user bootstrap to it.
 */
#include "Arduino.h"
#include "OmBlinker.h"
#include "OmWebServer.h"
#include <OmEspHelpers.h>

// These includes arent used in this file, but tells Arduino IDE that this project uses them.
#include <ESP8266WebServer.h> 
#include <ESP8266WiFi.h> 

OmBlinker ob(-1);
OmWebServer w;
OmWebPages p;

void setup() 
{
  Serial.begin(115200);
  Serial.printf("\n\n\nWelcome to the Sketch\n\n");

  ob.addNumber(1234);

  w.addWifi("noope", "haha");
  w.addWifi("omino warp", "0123456789");

  w.setHandler(p);
}

int ticks = 0;
void loop() 
{
  delay(10);
  ob.tick();
  w.tick();

  ticks++;
  if(ticks % 1000 == 0)
  {
    ob.addInterjection(10, 2);
  }
}
