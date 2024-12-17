/*
 * 2020-05-06 Plague Time
 * Low-level code to write bits to APA-102/Sk9822 led strips
 * based on 16-bit pixels
 */

// TODO roll back in to digitalWriteFast.

#if !NOT_ARDUINO
#ifndef __OmSk9822_h__
#define __OmSk9822_h__

#include "SPI.h"
#include "Arduino.h"

#include "OmLedT.h"
#include "OmLedTStrip.h"

/// Create a writer for certain pins, or SPI. On the ESP8266, the SPI pins MUST be Clock D5, Data D7, and it reserves the other two SPI pins.
/// TODO: respect the pin requests on ESP32.
template <int CLOCK_PIN, int DATA_PIN, int USE_SPI = 0>
class OmSk9822Writer
{
public:
    inline void w1(int bit)
    {
        DIGITAL_WRITE(CLOCK_PIN, 0);
        //  delay(10);
        DIGITAL_WRITE(DATA_PIN, bit);
        //  delay(10);
        DIGITAL_WRITE(CLOCK_PIN, 1);
        //  delay(10);
    }

    // send N bits down the wire, remembering that it's MSB first.
    void wN(unsigned long value, int bitCount)
    {
        unsigned long mask = 1L << (bitCount - 1);
        while (bitCount--)
        {
            int bit = (value & mask) ? 1 : 0;
            w1(bit);
            value <<= 1;
        }
    }

    OmSk9822Writer()
    {
        if(!USE_SPI)
        {
            PIN_MODE(CLOCK_PIN, OUTPUT);
            PIN_MODE(DATA_PIN, OUTPUT);
        }
    }

    int ww = 0;
    uint16_t shows = 0; // for throttling our error printout
    uint32_t spiRate = 12000000;

    void setSpiRate(uint32_t spiRate)
    {
        this->spiRate = spiRate;
    }

    void w32(unsigned long x)
    {
        if(USE_SPI)
        {
            SPI.transfer (x >> 24);
            SPI.transfer (x >> 16);
            SPI.transfer (x >>  8);
            SPI.transfer (x >>  0);
            ww += x;
        }
        else
        {
            for(int ix = 0; ix < 32; ix++)
            {
                DIGITAL_WRITE(CLOCK_PIN, 0);
                if(x & 0x80000000)
                {
                    DIGITAL_WRITE(DATA_PIN, 1);
                }
                else
                {
                    DIGITAL_WRITE(DATA_PIN, 0);
                }
                DIGITAL_WRITE(CLOCK_PIN, 1);
                x = x + x;
            }
        }
    }

    void blacks(int n)
    {
        while(n-- > 0)
            w32(0xe0000000);
    }

    /// Display the strip onto the hardware.
    /// 
    void showStrip(OmLed16Strip *strip)
    {
        strip->applyMilliampsLimit();
        if(USE_SPI)
        {
            this->shows++;
            if(shows == 1000)
            {
                // possibly print out our warning.
                if(CLOCK_PIN != SCK || DATA_PIN != MOSI)
                {
                    // TODO: on esp32, allow remapping the SPI pins. low-ish priority as frame rate is pretty good anyway.
                    Serial.printf("spi mode must use pins clock %d/data %d, cannot use %d/%d\n", SCK, MOSI, CLOCK_PIN, DATA_PIN);
                }
            }
            SPI.begin();
            SPI.beginTransaction(SPISettings(this->spiRate, MSBFIRST, SPI_MODE0)); // 12M really seems like the right speed. that's how it is.
        }
        for(int ix = 0; ix < strip->ledCount; ix++)
            w32(strip->leds[ix].dot());
        w32(0);
        w32(0); // urr probably needs more of these. or at least the correct number.
        if(USE_SPI)
        {
            SPI.endTransaction();
        }
    }
};

#endif
#endif
