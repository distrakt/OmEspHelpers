/*
  Blink with OTA Update

  This sketch shows a very minimal web server that controls
  the rate of the LED blinking, and a web control to reboot
  into "update mode"

  Or enter update mode by pulling a pin D1 low.

  HOW TO USE IT.
  0. Modify the sketch with your WIFI credentials, and led/button pins.
  1. Upload the sketch. See the LED blink.
  2. In serial monitor, watch the device connect to your WIFI, and note the IP address.
  3. Enter the address to your browser. Change the LED speed.

  OK... Now let'd update the device over the air, not USB
  4. In Arduino, do "Sketch:Export compiled binary" to generate a .bin file.
  5. On the web page, set the "Update Code" slider to the update code.
  6. Click "Update", and wait for the update page to appear.
  7. In the update page, select the .bin file (in project directory) and upload.
  8. After the upload completes, it will reboot with the new code.

  Of course, it just rebooted to the same code. But you could now move
  the device somewhere else, powered from a USB adapter or whatnot.
  And then you could change the sketch, and update it from afar. No
  USB serial required!
 */

#include "OmEspHelpers.h"

int led = LED_BUILTIN; // for ESP32 set to something else
int button = D1; // or any pin
int gBlinkRate = 10;

#define WIFI_SSID "omino warp" // my home network
#define WIFI_PASSWORD "0123456789" // my home network

OmWebServer s;
OmWebPages p;

void setup() 
{
  if(OmOta.setup(WIFI_SSID, WIFI_PASSWORD)) return;

  // if OmOta didnt return, we're in regular mode.
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  pinMode(button, INPUT_PULLUP);

  // minimal web server bringup
  s.addWifi(WIFI_SSID, WIFI_PASSWORD);
  s.setHandler(p);
  p.beginPage("main");
  p.addSlider("blinkRate", [] (const char *pageName, const char *parameterName, int value, int ref1, void *ref2)
  {
    gBlinkRate = value;
  });

  p.addStaticHtml("<pre>Hello, World!</pre>"); // you could change this, to prove you got a new version.
  
  OmOta.addUpdateControl(p);
}

void loop()
{
  if(OmOta.loop()) return;

  // if OmOta didn't return, we're in regular mode. Blink, and pump the web server.
  static int blinkPhase = 0;
  static int blinkState = 0; // 0 off, 1 on
  
  delay(10);
  s.tick();
  blinkPhase += gBlinkRate; // adding more blinks faster
  if(blinkPhase >= 100)
  {
    blinkPhase = 0;
    blinkState = !blinkState;
  }
  digitalWrite(led, blinkState);

  // you can invoke update mode from the web interface, or from a push button.
  int buttonState = digitalRead(button);
  if(buttonState == 0)
    OmOta.rebootToOta();
}
