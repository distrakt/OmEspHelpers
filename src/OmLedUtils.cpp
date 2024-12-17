#include "OmLedUtils.h"
#include <stdlib.h>

#ifdef _WIN32
#define FRAND ((float)rand() / (float)RAND_MAX)
#else
#define FRAND drand48()
#endif

int ir(int low, int high)
{
    if(low == high)
        return low;
    return rand() % (high - low) + low;
}

int ir(int max)
{
    return ir(0, max);
}

/// random range
float rr(float high)
{
    return rr(0, high);
}

float rr(float low, float high)
{
    return low + drand48() * (high - low);
}

/// random range, with 50% chance of sign-flip.
float rrS(float low, float high)
{
    float r = rr(low, high);
    if(ir(0,2))
        r = -r;
    return r;
}

int migrateI(int x, int dest, int delta)
{
    if(x < dest)
    {
        x += delta;
        if(x > dest)
            x = dest;
    }
    else if(x > dest)
    {
        x -= delta;
        if(x < dest)
            x = dest;
    }
    return x;
}

float migrateF(float x, float dest, float delta)
{
    if(x < dest)
    {
        x += delta;
        if(x > dest)
            x = dest;
    }
    else if(x > dest)
    {
        x -= delta;
        if(x < dest)
            x = dest;
    }
    return x;
}

int pinRangeI(int x, int bound0, int bound1)
{
    if(bound0 > bound1)
    {
        int t = bound0;
        bound0 = bound1;
        bound1 = t;
    }
    if(x < bound0)
        return bound0;
    if(x >= bound1)
        return bound1 - 1;
    return x;
}

float pinRange(float x, float bound0, float bound1)
{
    if(bound0 > bound1)
    {
        float t = bound0;
        bound0 = bound1;
        bound1 = t;
    }
    if(x < bound0)
        return bound0;
    if(x > bound1)
        return bound1;
    return x;
}

float mapRange(float x, float inLow, float inHigh, float outLow, float outHigh)
{
    x -= inLow;
    x = x * (outHigh - outLow) / (inHigh - inLow);
    x += outLow;

    x = pinRange(x, outLow, outHigh);
    return x;
}

float mapRangeNoPin(float x, float inLow, float inHigh, float outLow, float outHigh)
{
    x -= inLow;
    x = x * (outHigh - outLow) / (inHigh - inLow);
    x += outLow;
    return x;
}

int umod(int a, int b)
{
    int result = a % b;
    if(result < 0)
        result += b;
    return result;
}
