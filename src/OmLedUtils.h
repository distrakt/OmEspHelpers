

#ifndef __OmLedUtils_h__
#define __OmLedUtils_h__

#include <vector>

/// random range
float rr(float high);
float rr(float low, float high);
/// random range from low to high-1
int ir(int low, int high);
int ir(int max);
/// random range, with 50% chance of sign-flip.
float rrS(float low, float high);

int migrateI(int x, int dest, int delta);
float migrateF(float x, float dest, float delta);
float mapRange(float x, float inLow, float inHigh, float outLow, float outHigh);
float mapRangeNoPin(float x, float inLow, float inHigh, float outLow, float outHigh);
float pinRange(float x, float bound0, float bound1);
int pinRangeI(int x, int bound0, int bound1);
int umod(int a, int b);

template <class T>
void swapo(T &a, T&b)
{
    T t;
    t = a;
    a = b;
    b = t;
}

template <class T>
void remove(std::vector<T> &container, int index)
{
    if (index >= 0 && index < container.size())
        container.erase(container.begin() + index);
}

#endif // __OmLedUtils_h__
