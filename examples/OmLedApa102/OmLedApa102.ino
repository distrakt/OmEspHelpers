/*
Coded for Esp32 Dev Module, assumes we have Serial.printf
*/

#include "OmLedHelpers.h"


#define LED_PIN_C 16
#define LED_PIN_D 17

// Information about the LED strip itself
#define LED_COUNT 144

OmLed16 leds[LED_COUNT];
OmLed16Strip strip(LED_COUNT, leds);
OmSk9822Writer<LED_PIN_C, LED_PIN_D, 0> sk9822;


void setup() {
  delay(200);  // power-up safety delay

  OmLed16::setHexGamma(3.5);

  Serial.begin(115200);
  Serial.printf("%s built %s", __FILE__, __DATE__);
}

void loop() {
  delay(10);

  static uint16_t hue = 0;
  static float k = 0.0;  // current head of moving snakey
  float pixelsPerSecond = 10;

  static uint32_t lastMillis = 0;
  int nowMillis = millis();
  int deltaMillis = nowMillis - lastMillis;
  lastMillis = nowMillis;

  strip.clear();

  hue += 100;
  float pixelsAdvanced = pixelsPerSecond * deltaMillis / 1000.0;
  k = k + pixelsAdvanced;
  if (k > LED_COUNT)
    k -= LED_COUNT;
  uint16_t br = 65535;  // leading pixel is FULL bright
  for (int ix = 0; ix < 20; ix++) {
    OmLed16 le = OmLed16::hsv(hue, 65535, br);
    br = br * 0.7;
    strip.fillRangeRing(k - ix - 1, k - ix, le);
  }

  sk9822.showStrip(&strip);
}
