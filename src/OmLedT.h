
#ifndef __OmLedT_h__
#define __OmLedT_h__

#include "OmLedUtils.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

template <typename T, unsigned int MAX>
class OmLedT
{
public:

    union
    {
        T v[3];
        struct
        {
            T r;
            T g;
            T b;
        };
    };

    static const unsigned int _MAX = MAX;

    static OmLedT fromRgb(int r, int g, int b)
    {
        OmLedT led;
        led.r = r;
        led.g = g;
        led.b = b;
        return led;
    }

    static float hexGamma;
    static float hexGammaInverse;

    static void setHexGamma(float hexGamma)
    {
        OmLedT<T, MAX>::hexGamma = hexGamma;
        OmLedT<T, MAX>::hexGammaInverse = 1.0 / OmLedT<T, MAX>::hexGamma;
    }

    OmLedT()
    {
        this->r = 0;
        this->g = 0;
        this->b = 0;
        return;
    }

    OmLedT(T r, T g, T b)
    {
        this->r = r;
        this->g = g;
        this->b = b;
    }

    OmLedT<T, MAX> operator *(float n) const
    {
        OmLedT result;
        result.r = this->r * n;
        result.g = this->g * n;
        result.b = this->b * n;
        return result;
    }

    void operator *=(float n)
    {
        this->r *= n;
        this->g *= n;
        this->b *= n;
    }

    void operator *=(const OmLedT<T, MAX> &other)
    {
        OmLedT<T, MAX> result;
        this->r = this->r * other.r / MAX;
        this->g = this->g * other.g / MAX;
        this->b = this->b * other.b / MAX;
    }

    OmLedT operator *(const OmLedT &other) const
    {
        OmLedT<T, MAX> result;
        result.r = this->r * other.r / MAX;
        result.g = this->g * other.g / MAX;
        result.b = this->b * other.b / MAX;
        return result;
    }


    void operator +=(const OmLedT &other)
    {
#define ADD_SAT(_a,_b) _a = _a + _b; if(_a < _b) _a = MAX;
        ADD_SAT(this->r, other.r);
        ADD_SAT(this->g, other.g);
        ADD_SAT(this->b, other.b);
#undef ADD_SAT
    }

    OmLedT operator +(const OmLedT &other)
    {
        OmLedT result = *this;
        result += other;
        return result;
    }

    void operator -=(const OmLedT &other)
    {
#define SUB_SAT(_a,_b) if(_a < _b) _a = 0; else _a = _a - _b;
        SUB_SAT(this->r, other.r);
        SUB_SAT(this->g, other.g);
        SUB_SAT(this->b, other.b);
#undef SUB_SAT
    }

    OmLedT operator -(const OmLedT &other)
    {
        OmLedT result = *this;
        result -= other;
        return result;
    }

    bool operator ==(const OmLedT &other)
    {
        return (this->r == other.r) && (this->g == other.g) && (this->b == other.b);
    }

    /// mix between two colors. f is how much of the OTHER color. 0 means all of this-> stays same.
    OmLedT mix(const OmLedT &other, float f)
    {
        OmLedT result;
        float f1 = 1.0 - f;

        result.r = this->r * f1 + other.r * f + 0.5;
        result.g = this->g * f1 + other.g * f + 0.5;
        result.b = this->b * f1 + other.b * f + 0.5;
        return result;
    }


    static OmLedT hsv(T h, T s, T v)
    {
        OmLedT result;
        result.setHsv(h, s, v);
        return result;
    }

    void setRgb(T r, T g, T b)
    {
        this->r = r;
        this->g = g;
        this->b = b;
    }

#define otrMap(_x, _inLow, _inHigh, _outLow, _outHigh) (_x - _inLow) * (_outHigh - _outLow) / (_inHigh - _inLow) + _outLow

    void setHsvFloat(float h, float s, float v)
    {
#define hsvHextant0 ((float)(0.0 * MAX / 6.0))
#define hsvHextant1 ((float)(1.0 * MAX / 6.0))
#define hsvHextant2 ((float)(2.0 * MAX / 6.0))
#define hsvHextant3 ((float)(3.0 * MAX / 6.0))
#define hsvHextant4 ((float)(4.0 * MAX / 6.0))
#define hsvHextant5 ((float)(5.0 * MAX / 6.0))
#define hsvHextant6 ((float)(6.0 * MAX / 6.0))

        s = 1.0 - s; // now it's more like the "whiteness level", or min per component
        s = s * v; // scaled by brightness.

        if (h < hsvHextant1)
        {
            this->r = MAX * v;
            this->g = MAX * otrMap(h, hsvHextant0, hsvHextant1, s, v);
            this->b = MAX * s;
        }
        else if (h < hsvHextant2)
        {
            this->r = MAX * otrMap(h, hsvHextant1, hsvHextant2, v, s);
            this->g = MAX * v;
            this->b = MAX * s;
        }
        else if (h < hsvHextant3)
        {
            this->r = MAX * s;
            this->g = MAX * v;
            this->b = MAX * otrMap(h, hsvHextant2, hsvHextant3, s, v);
        }
        else if (h < hsvHextant4)
        {
            this->r = MAX * s;
            this->g = MAX * otrMap(h, hsvHextant3, hsvHextant4, v, s);
            this->b = MAX * v;
        }
        else if (h < hsvHextant5)
        {
            this->r = MAX * otrMap(h, hsvHextant4, hsvHextant5, s, v);
            this->g = MAX * s;
            this->b = MAX * v;
        }
        else
        {
            this->r = MAX * v;
            this->g = MAX * s;
            this->b = MAX * otrMap(h, hsvHextant5, hsvHextant6, v, s);
        }

#undef hsvHextant0
#undef hsvHextant1
#undef hsvHextant2
#undef hsvHextant3
#undef hsvHextant4
#undef hsvHextant5
#undef hsvHextant6

    }

    void setHsv(T h,  T s,  T v)
    {
        s = MAX - s; // now it's more like the "whiteness level", or min per component
        s = (long)s * v / MAX; // scaled by brightness.


#define hsvHextant0 ((int)(0.0 * (MAX+1) / 6 + 0.5))
#define hsvHextant1 ((int)(1.0 * (MAX+1) / 6 + 0.5))
#define hsvHextant2 ((int)(2.0 * (MAX+1) / 6 + 0.5))
#define hsvHextant3 ((int)(3.0 * (MAX+1) / 6 + 0.5))
#define hsvHextant4 ((int)(4.0 * (MAX+1) / 6 + 0.5))
#define hsvHextant5 ((int)(5.0 * (MAX+1) / 6 + 0.5))
#define hsvHextant6 ((int)(6.0 * (MAX+1) / 6 + 0.5))
        if (h < hsvHextant1)
        {
            this->r = v;
            this->g = otrMap(h, hsvHextant0, hsvHextant1, s, v);
            this->b = s;
        }
        else if (h < hsvHextant2)
        {
            this->r = otrMap(h, hsvHextant1, hsvHextant2, v, s);
            this->g = v;
            this->b = s;
        }
        else if (h < hsvHextant3)
        {
            this->r = s;
            this->g = v;
            this->b = otrMap(h, hsvHextant2, hsvHextant3, s, v);
        }
        else if (h < hsvHextant4)
        {
            this->r = s;
            this->g = otrMap(h, hsvHextant3, hsvHextant4, v, s);
            this->b = v;
        }
        else if (h < hsvHextant5)
        {
            this->r = otrMap(h, hsvHextant4, hsvHextant5, s, v);
            this->g = s;
            this->b = v;
        }
        else
        {
            this->r = v;
            this->g = s;
            this->b = otrMap(h, hsvHextant5, hsvHextant6, v, s);
        }
    }

#define _MAX(_a, _b) _a > _b ? _a : _b
#define _MIN(_a, _b) _a <= _b ? _a : _b
    void toHsv(T &hOut,  T &sOut,  T &vOut)
    {
        uint8_t maxComponent; // 0:r, 1:g, 2:b
        uint8_t minComponent; // 0:r, 1:g, 2:b

        if(this->r > this->g)
        {
            maxComponent = this->r > this->b ? 0 : 2;
            minComponent = this->g < this->b ? 1 : 2;
        }
        else
        {
            maxComponent = this->g > this->b ? 1 : 2;
            minComponent = this->r < this->b ? 0 : 2;
        }
        uint8_t hextantHex = (maxComponent << 4) | minComponent;

        vOut = this->v[maxComponent];
        sOut = mapRange(this->v[minComponent], 0, vOut, MAX, 0);

        switch(hextantHex)
        {
            case 0x02: // red max, blue min, so from red to yellow
                hOut = mapRange(this->g, 0, this->r, hsvHextant0, hsvHextant1);
                break;
            case 0x12: // green max, blue min, so from yellow to green
                hOut = mapRange(this->r, 0, this->g, hsvHextant2, hsvHextant1);
                break;
            case 0x10: // green max, red min, so from green to cyan
                hOut = mapRange(this->b, 0, this->g, hsvHextant2, hsvHextant3);
                break;
            case 0x20: // blue max, red min, so from cyan to blue
                hOut = mapRange(this->g, 0, this->b, hsvHextant4, hsvHextant3);
                break;
            case 0x21: // blue max, green min, so from blue to magents
                hOut = mapRange(this->r, 0, this->b, hsvHextant4, hsvHextant5);
                break;
            case 0x01: // red max, green min, so from magenta to red
                hOut = mapRange(this->b, 0, this->r, hsvHextant6, hsvHextant5);
                break;
            default:
                hOut = 0; // no clear winner, hue is 0.
        }
    }


    void printHextants()
    {
        printf("hextant0: %d\n", hsvHextant0);
        printf("hextant1: %d\n", hsvHextant1);
        printf("hextant2: %d\n", hsvHextant2);
        printf("hextant3: %d\n", hsvHextant3);
        printf("hextant4: %d\n", hsvHextant4);
        printf("hextant5: %d\n", hsvHextant5);
        printf("hextant6: %d\n", hsvHextant6);
    }
#undef hsvHextant0
#undef hsvHextant1
#undef hsvHextant2
#undef hsvHextant3
#undef hsvHextant4
#undef hsvHextant5
#undef hsvHextant6

    /// create LED with gamma-corrected web-color 0x00rrggbb
    OmLedT(int hex)
    {
        uint16_t r = (hex >> 16) & 0xff;
        uint16_t g = (hex >> 8) & 0xff;
        uint16_t b = hex & 0xff;

        float rF = r / 255.0;
        float gF = g / 255.0;
        float bF = b / 255.0;

        this->r = pow(rF, hexGamma) * MAX;;
        this->g = pow(gF, hexGamma) * MAX;;
        this->b = pow(bF, hexGamma) * MAX;;
    }

    int toHex()
    {
        float rF = (float)this->r / MAX;
        float gF = (float)this->g / MAX;
        float bF = (float)this->b / MAX;

        rF = pow(rF, hexGammaInverse);
        gF = pow(gF, hexGammaInverse);
        bF = pow(bF, hexGammaInverse);
        rF = rF * 255.0;
        gF = gF * 255.0;
        bF = bF * 255.0;
        int r = rF + 0.5;
        int g = gF + 0.5;;
        int b = bF + 0.5;;
        int result = (r << 16) | (g << 8) | b;
        return result;
    }

    OmLedT<uint8_t, 255> getLed8()
    {
        if(MAX == 255)
        {
            return *(OmLedT<uint8_t, 255> *)this; // compiler will optimize out if it's not, in fact, MAX255.
        }
        else if(MAX == 65535)
        {
            OmLedT<uint8_t, 255> result;
            result.r = this->r >> 8;
            result.g = this->g >> 8;
            result.b = this->b >> 8;
            return result;
        }
        else
        {
            OmLedT<uint8_t, 255> result;
            return result;
        }
    }

    OmLedT<uint16_t, 65535> getLed16()
    {
        if(MAX == 255)
        {
            OmLedT<uint16_t, 65535> result;
            result.r = (this->r << 8) | this->r;
            result.g = (this->g << 8) | this->g;
            result.b = (this->b << 8) | this->b;
            return result;
        }
        else if(MAX == 65535)
        {
            return *(OmLedT<uint16_t, 65535> *)this; // compiler will optimize out if it's not, in fact, MAX255.
        }
        else
        {
            OmLedT<uint16_t, 65535> result;
            return result;
        }
    }

    template <typename LEDT>
    LEDT getLedT()
    {
        LEDT result;
        if(LEDT::_MAX >= MAX)
        {
            // going to same or larger bit size, we divide so 0x12 --> 0x1212
            result.r = this->r * LEDT::_MAX / MAX;
            result.g = this->g * LEDT::_MAX / MAX;
            result.b = this->b * LEDT::_MAX / MAX;
        }
        else
        {
            // going to smaller bit size, we let it truncate, so (255+1) / (65535+1)
            result.r = this->r * (LEDT::_MAX+1) / (MAX+1);
            result.g = this->g * (LEDT::_MAX+1) / (MAX+1);
            result.b = this->b * (LEDT::_MAX+1) / (MAX+1);
        }
        return result;
    }

    // return the dot star 32 bit value for this pixel
    // dot() uses one of three Brightness multipliers, 31, 5, or 1.
    uint32_t dot()
    {
        uint32_t x;

        if(MAX == 255)
        {
            x = 0xff000000 | (this->r) | (((uint32_t)this->g) << 8) | (((uint32_t)this->b) << 16);
        }
        else if(MAX == 65535)
        {
            // cheap estimate of max component
            uint16_t maxComponent = this->r | this->g | this->b;

            // we kick in the 31 bit scalar in 3 regimes: 31, 5, and 1.
            // the full 16 bit ranges are:
            // 31/31 * 0xffff --> 65535.00
            //  5/31 * 0xffff --> 10570.16
            //  1/31 * 0xffff -->  2114.03

            if (maxComponent >= 10570)
            {
                // use brightness 31, and rgb directly.
                x = 0xff000000 | (this->r >> 8) | (this->g & 0xff00) | (((uint32_t)this->b & 0xff00) << 8);
            }
            else if (maxComponent >= 2114)
            {
                // use brightness 5, and scale up
                uint16_t rr = this->r * 31L / 5;
                uint16_t gg = this->g * 31L / 5;
                uint32_t bb = this->b * 31L / 5;
                x = 0xe5000000 | (rr >> 8) | (gg & 0xff00) | ((bb & 0xff00) << 8);
            }
            else
            {
                // dimmest regime, brightness 1, and scale up
                uint32_t rr = this->r * 31;
                uint32_t gg = this->g * 31;
                uint32_t bb = this->b * 31;
                x = 0xe1000000 | (rr >> 8) | (gg & 0xff00) | ((bb & 0xff00) << 8);
            }
        }
        else
            x = 0;
        return x;
    }

    // the highest color component
    int getBrightness()
    {
        int result = this->r;
        if(this->g > result)
            result = this->g;
        if(this->b > result)
            result = this->b;
        return result;
    }
};

template <typename T, unsigned int MAX>
OmLedT<T, MAX> operator *(float n, OmLedT<T, MAX> &led)
{
    OmLedT<T, MAX> result;
    result.r = n * led.r;
    result.g = n * led.g;
    result.b = n * led.b;
    return result;
}

// static variables
template <typename T, unsigned int MAX>
float OmLedT<T, MAX>::hexGamma = 1.0;  // gamma
template <typename T, unsigned int MAX>
float OmLedT<T, MAX>::hexGammaInverse = 1.0; // inverse gamma


typedef OmLedT<uint8_t, 255> OmLed8;
typedef OmLedT<uint16_t, 65535> OmLed16;
#define OmHsv8(h,s,v) OmLed8::hsv(h,s,v)

#endif // __OmLedT_h__
