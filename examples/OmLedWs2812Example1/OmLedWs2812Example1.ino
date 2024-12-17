#include "OmLedHelpers.h"


#define LED_COUNT 64
// Ws2812 writer always uses the SPI pin, D7 on ESP8266

OmLed8 leds[LED_COUNT];
OmLed8Strip s(LED_COUNT, leds);

OmWs2812Writer ledWriter;

void setup()
{
    Serial.begin(115200);
    delay(400);
    Serial.printf("\n\n\n");
    Serial.printf("hello from\n%s\n", __FILE__);
      // show bad config
  if (F_CPU < 160000000)
    Serial.printf("CPU is %d; works better at 160mhz and up\n", F_CPU);
}

void loop()
{
    static int ticks = 0;
    static float x = 0;
    static float hue = 0;
    delay(16.666);

    ticks++;
    if(ticks % 123 == 0)
    {
        Serial.print("ticks ");
        Serial.println(ticks);
    }

    s.clear();
    OmLed8 co = OmLed8::hsv(hue, 255, 255);
    hue += 1.8;
    if(hue >= 256)
    hue -= 256;
    x += 0.04;
    if(x > LED_COUNT)
    x -= LED_COUNT;
    if(ticks % 3 == 0)
      s.fillRange(0, 100, OmLed8(10,0,0));
    s.setLed(x, co);
    s.fillRangeRing(x + LED_COUNT / 2.0, x + LED_COUNT / 2.0 + 2.5, co);

    ledWriter.showStrip(&s);
}
