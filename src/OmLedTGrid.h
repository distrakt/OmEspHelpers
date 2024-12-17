#ifndef __OmLedTGrid_h__
#define __OmLedTGrid_h__

#include "OmLedT.h"
#include "OmLedTStrip.h"

/// callback to return an LED index for a coordinate, or -1 if out of range in any way
typedef int (* OmPixelFinder)(int x, int y, void *ref);


template <class LEDT, bool WRAPX = false, bool WRAPY = false>
class OmLedTGrid
{
public:
    OmLedTStrip<LEDT> *strip = NULL;
    uint16_t width = 0;
    uint16_t height = 0;
    OmPixelFinder pixelFinder = NULL;
    void *pixelFinderRef = NULL;

    void setPixelFinder(OmPixelFinder pixelFinder, void *pixelFinderRef)
    {
        this->pixelFinder = pixelFinder;
        this->pixelFinderRef = pixelFinderRef;
    }

    int getLedIndex(int x, int y)
    {
        if(this->pixelFinder)
        {
            int result = this->pixelFinder(x, y, this->pixelFinderRef);
            return result;
        }
        
        if(x < 0 || x >= this->width || y < 0)
            return -1;

        int ix = this->width * y + x;
        if(ix >= this->strip->ledCount)
            ix = -1;
        return ix;
    }

    /// simplest arrangement is take a strip, and assume it's left to right, top to bottom, by width
    OmLedTGrid(OmLedTStrip<LEDT> *strip, uint16_t width, uint16_t height = 0)
    {
        if(width < 1)
            width = 1;

        this->strip = strip;
        this->width = width;
        if(height == 0)
            height = (this->strip->ledCount + this->width - 1)/this->width;
        this->height = height;
    }

    void setLed(const LEDT co, int x, int y)
    {
        int ix = this->getLedIndex(x, y);
        if(ix >= 0)
            strip->setLed(ix, co);
    }

    void clear()
    {
        this->strip->clear();
    }
    void clear(LEDT co)
    {
        this->strip->clear(co);
    }

    template <int MODE>
    void fillRow(LEDT co, int y, float x0, float x1)
    {
        if(x0 >= this->width)
            return;
        if(x1 <= 0)
            return;
        if(x0 < 0)
            x0 = 0;
        if(x1 > this->width)
            x1 = this->width;

        int xi = floor(x0);
        int ei = floor(x1);

        LEDT *leds = this->strip->leds;

#define IF_LEDIX_IN_RANGE if(ledIx >= 0 && ledIx < this->strip->ledCount)
        if (xi == ei)
        {
            // only one LED lit
            int ledIx = this->getLedIndex(xi, y);
            IF_LEDIX_IN_RANGE
            {
                float f = x1 - x0;
                if(MODE == MODE_REPLACE)
                    leds[ledIx] *= (1.0 - f);
                leds[ledIx] += co * f;
            }
        }
        else
        {
            LEDT coF;
            float f;
            int ledIx;

            // leftmost pixel
            ledIx = this->getLedIndex(xi, y);
            IF_LEDIX_IN_RANGE
            {
                f = xi + 1 - x0;
                if(MODE == MODE_REPLACE)
                    leds[ledIx] *= (1.0 - f);
                leds[ledIx] += co * f;
            }

            // middle pixels, if any
            for (int k = xi + 1; k < ei; k++)
            {
                ledIx = this->getLedIndex(k, y);
                IF_LEDIX_IN_RANGE
                {
                    if(MODE == MODE_REPLACE)
                        leds[ledIx] = co;
                    else
                        leds[ledIx] += co;
                }
            }

            // rightmost pixel
            f = x1 - ei;
            if(f > 0.0)
            {
                ledIx = this->getLedIndex(ei, y);
                IF_LEDIX_IN_RANGE
                {
                    if(MODE == MODE_REPLACE)
                        leds[ledIx] *= (1.0 - f);
                    leds[ledIx] += co * f;
                }
            }
        }
    }

    static float fu(float n, float d)
    {
        n = fmod(n, d);
        if(n < 0)
            n += d;
        return n;
    }

    void fill2(LEDT co, float x0, float y0, float x1, float y1)
    {
        if(x0 > x1)
        {
            this->fill2(co, 0, y0, x1, y1);
            this->fill2(co, x0, y0, this->width, y1);
            return;
        }

        if(y0 > y1)
        {
            this->fill2(co, x0, 0, x1, y1);
            this->fill2(co, x0, y0, x1, this->height);
            return;
        }

        int y0i = floor(y0);
        int y1i = floor(y1);

        if (y0i == y1i)
        {
            // only one LED row lit
            float f = y1 - y0;
            LEDT coF = co * f;
            this->fillRow<0>(coF, y0i, x0, x1);
        }
        else
        {
            LEDT coF;
            float f;

            // topmost row
            f = y0i + 1 - y0;
            coF = co * f;
            this->fillRow<0>(coF, y0i, x0, x1);

            // middle rows, if any
            for (int k = y0i + 1; k < y1i; k++)
            {
                this->fillRow<0>(co, k, x0, x1);
            }

            // bottom pixel row
            f = y1 - y1i;
            coF = co * f;
            this->fillRow<0>(coF, y1i, x0, x1);
        }
    }

    void fill(LEDT co, float x0, float y0, float x1, float y1)
    {
        if(WRAPX)
        {
            x0 = fu(x0, this->width);
            x1 = fu(x1, this->width);
        }
        else
        {
            x0 = pinRange(x0, 0, this->width);
            x1 = pinRange(x1, 0, this->width);
            if(x0 > x1)
                swapo(x0, x1);
        }

        if(WRAPY)
        {
            y0 = fu(y0, this->height);
            y1 = fu(y1, this->height);
        }
        else
        {
            y0 = pinRange(y0, 0, this->height);
            y1 = pinRange(y1, 0, this->height);
            if(y0 > y1)
                swapo(y0, y1);
        }

        this->fill2(co, x0, y0, x1, y1);
    }

};

typedef OmLedTGrid<OmLed8> OmLed8Grid;
typedef OmLedTGrid<OmLed16> OmLed16Grid;

#endif // __OmLedTGrid_h__

