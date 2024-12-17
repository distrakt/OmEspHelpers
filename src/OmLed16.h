// For Arduino UNO, trying out "hi-res" Sk9822 color control
// Which has 8 bits of r,g,b, and a 5 bit multiplier per pixel
// datasheet https://www.pololu.com/file/0J1234/sk9822_datasheet.pdf
// more good notes: https://cpldcpu.wordpress.com/2016/12/13/sk9822-a-clone-of-the-apa102/

#ifndef __OmLed16_h__
#define __OmLed16_h__

#include <stdlib.h>
#include <math.h>
#include "OmLedT.h"

#define USE_OMLEDT 1
// class represents 16-bit rgb components, and
// conversions suitable for Sk9822 or Apa102 display.
// "Origi" is the old self-contained one, used for testing against old behavior with the templated one.
class OmLed16Origi
{
public:
    unsigned short r;
    unsigned short g;
    unsigned short b;

    static float hexGamma;  // gamma
    static float hexGamma1; // inverse gamma
    static void setHexGamma(float hexGamma);

    OmLed16Origi operator +(OmLed16Origi other);

    OmLed16Origi();

    OmLed16Origi(unsigned short r,unsigned short g,unsigned short b);

    OmLed16Origi(int hex);

    int toHex();
    static OmLed16Origi hsv(unsigned short h, unsigned short s, unsigned short v);

    /// return simple 8-bit dotstar/apa102/sk9822, no low-value scaling.
    uint32_t dot8();

    /// return the dotstar/apa102/sk9822 32 bit value for this pixel with 1, 5, or 31 scaling
    uint32_t dot();

    /// like dot() with more accurate brigthness, but slower.
    /// I find that regular dot() is just fine.
    uint32_t dotY();

    void setHsv( unsigned short h,  long s,  long v);
    void operator *=(float x);
    OmLed16Origi operator *(float x);
    void operator *=(OmLed16Origi other);
    OmLed16Origi operator *(OmLed16Origi other);
//#define ADD_SAT(_a,_b) _a = _a + _b; if(_a < _b) _a = -1;
    void operator +=(OmLed16Origi other);
};


#if USE_OMLEDT
#else
typedef OmLed16Origi OmLed16;

class OmLed16Strip
{
public:
    int ledCount;
    OmLed16 *leds;
    bool needsDispose;

    OmLed16Strip(int ledCount, OmLed16 *leds = NULL);
    ~OmLed16Strip();
    void clear();

    void fillRange(float low, float high, OmLed16 co);

    int umod(int a, int b);

    void fillRangeRing(float low, float high, OmLed16 co);

};
#endif

#endif
