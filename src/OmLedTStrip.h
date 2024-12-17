
#ifndef __OmLedTStrip_h__
#define __OmLedTStrip_h__

#include "OmLedT.h"

template <class LEDT>
class OmLedTStrip
{
public:
    int ledCount = 0;
    int ringLed0 = 0; // Offset to zeroth LED, only used in ring-drawing
    LEDT *leds = NULL;
    bool needsDispose = false;
    float maLimit = 0; // 0 means dont limit.

    OmLedTStrip()
    {
        return;
    }

    void init(int ledCount, int zeroPoint = 0, LEDT *leds = NULL)
    {
        this->ledCount = ledCount;
        this->ringLed0 = zeroPoint;
        if(leds)
        {
            this->leds = leds;
            this->needsDispose = false;
        }
        else
        {
            this->leds = (LEDT *)calloc(ledCount, sizeof(LEDT));
            this->needsDispose = true;
        }
    }

    OmLedTStrip(int ledCount, LEDT *leds, int zeroPoint)
    {
        this->init(ledCount, zeroPoint, leds);
    }

    OmLedTStrip(int ledCount, LEDT *leds = NULL)
    {
        this->init(ledCount, 0, leds);
    }

    ~OmLedTStrip()
    {
        if(this->needsDispose && this->leds)
        {
            free(this->leds);
        }
        this->leds = NULL;
    }

    void clear()
    {
        LEDT *w = this->leds;
        for(int ix = 0; ix < this->ledCount; ix++)
        {
            w->r = 0;
            w->g = 0;
            w->b = 0;
            w++;
        }
    }

    void clear(LEDT co)
    {
        LEDT *w = this->leds;
        for(int ix = 0; ix < this->ledCount; ix++)
            *w++ = co;
    }

    void operator *=(float n)
    {
        LEDT *w = this->leds;
        for(int ix = 0; ix < this->ledCount; ix++)
        {
            w->r = w->r * n + 0.5;
            w->g = w->g * n + 0.5;
            w->b = w->b * n + 0.5;
            w++;
        }
    }
    
    void operator +=(OmLedTStrip<LEDT> *other)
    {
        int k = this->ledCount;
        if(other->ledCount < k)
            k = other->ledCount;
        LEDT *w = this->leds;
        LEDT *wOther = other->leds;
        for(int ix = 0; ix < k; ix++)
            *w++ += *wOther++;
    }
    
    bool setLed(int x, const LEDT &co)
    {
        if(x < 0 || x >= this->ledCount)
            return false;
        this->leds[x] = co;
        return true;
    }

    bool setLed(float x, const LEDT &co)
    {
        this->fillRange(x, x + 1, co);
        return true;
    }


#define MODE_ADD 0
#define MODE_REPLACE 1

    template <int MODE>
    void fillRangeM(float low, float high, LEDT co)
    {
        if(low >= this->ledCount)
            return;
        if(high <= 0)
            return;
        if(low < 0)
            low = 0;
        if(high > ledCount)
            high = ledCount;

        int xi = floor(low);
        int ei = floor(high);

        if (xi == ei)
        {
            // only one LED lit
            float f = high - low;
            if(MODE == MODE_REPLACE)
                this->leds[xi] *= (1.0 - f);
            this->leds[xi] += co * f;
        }
        else
        {
            LEDT coF;
            float f;

            // leftmost pixel
            f = xi + 1 - low;
            if(MODE == MODE_REPLACE)
                this->leds[xi] *= (1.0 - f);
            this->leds[xi] += co * f;

            // middle pixels, if any
            for (int k = xi + 1; k < ei; k++)
            {
                if(MODE == MODE_REPLACE)
                    this->leds[k] = co;
                else
                    this->leds[k] += co;
            }

            // rightmost pixel
            f = high - ei;
            if(f > 0.0)
            {
                if(MODE == MODE_REPLACE)
                    this->leds[ei] *= (1.0 - f);
                this->leds[ei] += co * f;
            }
        }
    }

    /// variant of fillRange takes two arguments, and does RGB gradient
    template <int MODE, bool RING>
    void fillRangeM(float low, float high, LEDT co0, LEDT co1)
    {
        if(!RING)
        {
            if(low >= this->ledCount)
                return;
            if(high <= 0)
                return;
            if(low < 0)
                low = 0;
            if(high > ledCount)
                high = ledCount;
        }

        float ledCount = this->ledCount;

        int xi = floor(low);
        int ei = floor(high);

        LEDT coG;

        if (xi == ei)
        {
            // only one LED lit
            float f = high - low;

            int kk = xi;
            if(RING)
                kk = umod(kk, ledCount);
            if(MODE == MODE_REPLACE)
                this->leds[kk] *= (1.0 - f);
            coG = co0.mix(co1, 0.5);
            this->leds[kk] += coG * f; //one pixel, use middle of gradient.
        }
        else
        {
            // as we walk the pixels, we compute the position between low and high
            // of the center of the pixel under consideration.

            LEDT coF;
            float f;
            float spanX;

            // leftmost partial OR full pixel
            f = xi + 1.0 - low; // how much of the pixel we cover, from the right-edge
            spanX = xi + 1.0 - f / 2.0; // halfway into that portion of the pixel, 1-f from left edge
            spanX = mapRange(spanX, low, high, 0.0, 1.0);
            coG = co0.mix(co1, spanX);

            int kk = xi;
            if(RING)
                kk = umod(kk, ledCount);
            if(MODE == MODE_REPLACE)
                this->leds[kk] *= (1.0 - f);
            this->leds[kk] += coG * f;

            // middle pixels, if any
            for (int k = xi + 1; k < ei; k++)
            {
                spanX = k + 0.5; // center of pixel
                spanX = mapRange(spanX, low, high, 0.0, 1.0);
                coG = co0.mix(co1, spanX);
                int kk = k;
                if(RING)
                    kk = umod(kk, ledCount);
                if(MODE == MODE_REPLACE)
                    this->leds[kk] = coG;
                else
                    this->leds[kk] += coG;
            }

            // rightmost pixel
            f = high - ei;
            if(f > 0.0)
            {
                spanX = ei + f / 2.0; // center of remaining pixel
                spanX = mapRange(spanX, low, high, 0.0, 1.0);
                coG = co0.mix(co1, spanX);
                int kk = ei;
                if(RING)
                    kk = umod(kk, ledCount);
                if(MODE == MODE_REPLACE)
                    this->leds[kk] *= (1.0 - f);
                this->leds[kk] += coG * f;
            }
        }
    }

    void fillRange(float low, float high, LEDT co, bool replaceDontAdd = false)
    {
        if(replaceDontAdd)
            this->fillRangeM<1>(low, high, co);
        else
            this->fillRangeM<0>(low, high, co);
    }

    void fillRangeRing(float low, float high, LEDT co)
    {
        low += this->ringLed0;
        high += this->ringLed0;
        int xi = floor(low);
        int ei = floor(high);

#define U(_x) umod(_x, this->ledCount)

        if (xi == ei)
        {
            // only one LED lit
            float f = high - low;
            this->leds[U(xi)] += co * f;
        }
        else
        {
            LEDT coF;
            float f;

            // leftmost pixel
            f = xi + 1 - low;
            this->leds[U(xi)] += co * f;

            // middle pixels, if any
            for (int k = xi + 1; k < ei; k++)
                this->leds[U(k)] += co;

            // rightmost pixel
            f = high - ei;
            this->leds[U(ei)] += co * f;
        }
    }

    void fillRange(float low, float high, LEDT co0,LEDT co1, bool replaceDontAdd = false)
    {
        if(low > high)
        {
            swapo(low, high);
            swapo(co0, co1);
        }
        if(replaceDontAdd)
            this->fillRangeM<1, false>(low, high, co0, co1);
        else
            this->fillRangeM<0, false>(low, high, co0, co1);
    }

    void fillRangeRing(float low, float high, LEDT co0,LEDT co1, bool replaceDontAdd = false)
    {
        if(low > high)
        {
            swapo(low, high);
            swapo(co0, co1);
        }
        if(replaceDontAdd)
            this->fillRangeM<1, true>(low, high, co0, co1);
        else
            this->fillRangeM<0, true>(low, high, co0, co1);
    }

    void draw(float x, float w, LEDT co)
    {
        this->fillRangeRing(x, x + w, co);
    }


    int oltMin(int a, int b)
    {
        return a < b ? a : b;
    }

    void copyFrom(OmLedTStrip *other)
    {
        int k = oltMin(this->ledCount, other->ledCount);
        for (int ix = 0; ix < k; ix++)
            this->leds[ix] = other->leds[ix];
        while (k < this->ledCount)
            this->leds[k++] = LEDT(0, 0, 0);
    }

    int getSize()
    {
        return this->ledCount;
    }

    int getZeroPoint()
    {
        return this->ringLed0;
    }

    LEDT *getLeds()
    {
        return this->leds;
    }
    
    uint32_t getMilliamps()
    {
        uint32_t t = 0;
        for(int ix = 0; ix < this->ledCount; ix++)
            t += this->leds[ix].r + this->leds[ix].g + this->leds[ix].b;
        t = t * 20 / LEDT::_MAX;
        return t;
    }

    // apply the milliamp limiter
    void applyMilliampsLimit()
    {
        if(this->maLimit > 1)
        {
            uint32_t ma = this->getMilliamps();
            if(ma > this->maLimit)
            {
                float t = (float)this->maLimit / ma;
                for(int ix = 0; ix < this->ledCount; ix++)
                    this->leds[ix] *= t;
            }
        }
    }

    /// set the milliamp limiter, applied by calling applyMilliampsLimit()
    void limitMilliamps(uint32_t maLimit)
    {
        this->maLimit = maLimit;
    }
};

typedef OmLedTStrip<OmLed8> OmLed8Strip;
typedef OmLedTStrip<OmLed16> OmLed16Strip;

#endif // __OmLedTStrip_h__
