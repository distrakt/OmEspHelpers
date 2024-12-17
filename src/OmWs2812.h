/*
 * 2020-05-17 Plague Time
 * A SPI-based implementation of WS2812 driver.
 * It's for ESP8266 and must be on the SPI data
 * pin (D7 for a Wemos D1). It also ends up using
 * the clock pin of course but oh well.
 */

#ifndef __OmWs2812_h__
#define __OmWs2812_h__

#include "OmLedT.h"
#include "OmLedTStrip.h"

class OmWs2812Writer
{
public:
    OmWs2812Writer();
#ifdef ARDUINO_ARCH_ESP32
    OmWs2812Writer(uint8_t mosi, uint8_t miso, uint8_t sclk, uint8_t cs);
#endif
//    void showLeds(OmLed8 *leds, int ledCount);
    void showLeds(OmLed8Strip *strip);
    void showStrip(OmLed8Strip *strip);
    void showLeds(OmLed16 *leds, int ledCount);
    void showLeds(OmLed16Strip *strip);
    void showStrip(OmLed16Strip *strip);
    void setGrb(bool onOff = true);
    bool getGrb();

    void setBrightness(uint8_t brightness);
    void setLimit(uint8_t limit);

    /*
     Hazard limit. I tried  1280 which is 8µS at 160mhz, and suppsedly would be ok. But experimentally
     on the Kitchen Led Sculpture found that it flickered down to 900 or so. So I use 800 as the cutoff.
     So far this leads to around 3% of frames having to be retried due to an interrupt in there.
     Still gathering data. dvb2020-05 plague time.
     */
    /// if enabled, will disable interrupts except for brief intervals, and will retry the update if hazard exceeded.
    void setInterruptWindow(bool interruptsEnabled, int cycleHazardLimit = 800); // 800 is 5µS at 160Mhz.

    // stats gathered, in machine cycles (80/160mhz)
    int irqBucketSize = 64; // categorize irq tracking buckets by multiples of this
    static const int irqBucketCount = 64; // max bucket multiple
    unsigned int cycleHazardLimit = 0;

    class IrqInfo
    {
    public:
        uint32_t pauseCyclesLongest = 0;
        float pauseCyclesAverage;
        uint32_t hazardReached = 0; // incremented each time the hazard limite exceeded.
        uint32_t buckets[irqBucketCount];
        uint32_t frames = 0;

        void clear()
        {
            for (int ix = 0; ix < OmWs2812Writer::irqBucketCount; ix++)
              this->buckets[ix] = 0;
            this->pauseCyclesLongest = 0;
            this->hazardReached = 0;
            this->frames = 0;
        }

        IrqInfo()
        {
            this->clear();
        }
    };

    IrqInfo irq;

    uint32_t spiRate = 3200000;

#ifdef ARDUINO_ARCH_ESP32
// on ESP32, we allow overriding the default SPI pins
    bool doOverrideSpiPins = false;
    uint8_t mosi;
    uint8_t miso;
    uint8_t sclk;
    uint8_t cs;
    /// we only use the first pin, Mosi, as the output, but the other three must be assigned (and are not used).
    void setSpiPins(uint8_t mosi, uint8_t miso, uint8_t sclk, uint8_t cs);
#endif
};

#endif
