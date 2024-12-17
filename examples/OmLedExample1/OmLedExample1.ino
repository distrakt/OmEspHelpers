/*
  Example of OmLedHelpers 
  either: ESP8266 or ESP32 CPU
  either: ws2812 or apa102 LED
*/
#include "OmLedHelpers.h"

#define OMLED_2812 0  // any of ws2811, ws2812, ws2815, sk6812...
#define OMLED_102 1   // apa102, sk9822...

#ifdef ARDUINO_ARCH_ESP32
#define DO_OLED 1  // if set, also draw some junk on the oled provided on certain ESP32 boards. kind-of specific.
#endif

#ifdef ARDUINO_ARCH_ESP8266

#if OMLED_2812

OmWs2812Writer ledWriter;  // always uses the default SPI pins, gpio13 (called D7 on D1 mini)

#elif OMLED_102

#define LED_PIN_D D7
#define LED_PIN_C D6
OmSk9822Writer<LED_PIN_C, LED_PIN_D> ledWriter;

#else
#warning "Only WS2812 or Apa102 type leds supported"
#endif

#elif ARDUINO_ARCH_ESP32

#if OMLED_2812

// On Esp32, you can specify the SPI pins, we only use the first argument output pin.

OmWs2812Writer ledWriter(12, 13, 14, 15);  // override the SPI output pin to be 12. The other three pins must be assigned and are unused for this.
                                           // OmWs2812Writer ledWriter; // or just use the default vspi SPI pins.

#elif OMLED_102
#define LED_PIN_C 13
#define LED_PIN_D 12
OmSk9822Writer<LED_PIN_C, LED_PIN_D> ledWriter;
#else
#warning "Only WS2812 or Apa102 type leds supported"
#endif

#else
#error "Only Esp8266 and Esp32 supported"
#endif


#define DATA_P
#define LED_COUNT 16

OmLed16 leds[LED_COUNT];
OmLed16Strip strip(LED_COUNT, leds);
//OmSk9822Writer<LED_PIN_C, LED_PIN_D, 0> sk9822;

#if DO_OLED
#include "SSD1306.h"          // alias for `#include "SSD1306Wire.h"`
SSD1306 display(0x3c, 5, 4);  // pins 5 and 4.
#endif

void setup() {
  Serial.begin(115200);
  delay(350);

#if DO_OLED
  display.init();
#endif
  // Serial.printf("\n\n%s %s\n", __FILE__, __DATE__);

  // Serial.printf("MOSI: %d\n", MOSI);
  // Serial.printf("MISO: %d\n", MISO);
  // Serial.printf("SCK: %d\n", SCK);
  // Serial.printf("SS: %d\n", SS);
  // Serial.printf("\n");

  // ledWriter.setSpiPins(12,13,14,15);
}

void loop() {
  static int ticks = 0;
  static float x = 0;
  static float hue = 0;
  delay(16.666);

  ticks++;
  if (ticks % 123 == 0) {
    Serial.print("ticks ");
    Serial.println(ticks);
  }

  strip.clear();
  OmLed16 co = OmLed16::hsv(hue * 0x0101, 0xffff, 0xa000);
  hue += 1.8;
  if (hue >= 256)
    hue -= 256;
  x += 0.04;
  if (x > LED_COUNT)
    x -= LED_COUNT;
  // strip.setLed(x, co);
  strip.fillRangeRing(x, x + 1.5, co);
  ledWriter.showStrip(&strip);

#if DO_OLED
  display.setColor(BLACK);
  display.clear();
  display.setColor(WHITE);
  display.drawLine(ir(0, 50), ir(0, 50), ir(0, 50), ir(0, 50));
  display.display();
#endif
}
