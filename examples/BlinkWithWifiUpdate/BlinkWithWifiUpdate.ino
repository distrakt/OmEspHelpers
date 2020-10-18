/*
  Blink with OTA Update

  This sketch shows how an otherwise non-Wifi-enabled project can
  still benefit from a wireless update.

  For an ESP8266, blink the built-in LED.
  When a pin D1 is pulled low, enter Wifi Over The Air Update mode, and await
  a new binary image.

  HOW TO USE IT.
  1. Upload the sketch. See the LED blink a ...- pattern.
  2. Open serial monitor
  3. Ground pin D1, this will cause a reboot, entering OTA UPDATE 
     mode. Then unground the pin.
  4. Observe on serial monitor the IP address that the wifi takes
  5. Write it down -- this will be the same IP address forever, most
     likely. So it won't need to be attached to a serial monitor forever.
  6. Change the blink pattern if you like
  7. Use Sketch:Export Compiled Binary
  8. In browser, open the IP address you wrote down
  9. Choose File to the new binary, and Upload on the page.
  10. It will upload & reboot with new code.
 */

#include "OmEspHelpers.h"

int led = LED_BUILTIN;
int button = D1;
#define WIFI_SSID "omino warp" // my home network
#define WIFI_PASSWORD "0123456789" // my home network

void setup() 
{
  if(OmOta.setup(WIFI_SSID, WIFI_PASSWORD)) return;
  pinMode(led, OUTPUT);
  pinMode(button, INPUT_PULLUP);
}

void blink(int tOn, int tOff)
{
  digitalWrite(led, LOW); // led On
  delay(tOn);
  digitalWrite(led, HIGH); // led Off
  delay(tOff);
}

void loop()
{
  if(OmOta.loop()) return;

  // three short blinks and a long blink, why not.
  blink(100,100);
  blink(100,100);
  blink(100,200);
  blink(400, 500);
  int buttonState = digitalRead(button);
  if(buttonState == 0)
    OmOta.rebootToOta();
}
