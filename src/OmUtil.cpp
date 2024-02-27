//
//  OmUtil.h
//  OmEspHelpers
//
//  Created by David Van Brink on 11/22/16.
//  Copyright (c) 2016 omino.com. All rights reserved.
//

#include "OmUtil.h"
#include <stdio.h>
#include <string.h>

bool omStringEqual(const char *s1, const char *s2, int maxLen)
{
    if(!s1 || !s2)
        return false;

    while(maxLen-- > 0)
    {
        char c1 = *s1++;
        char c2 = *s2++;
        if(c1 != c2)
        {
            return false;
        }
        if(c1 == 0)
        {
            return true;
        }
    }
    return false; // reached the end. 
}

const char *omTime(long long millis, int secondsDecimals)
{
    static char s[100];
    if(secondsDecimals < 0)
        secondsDecimals = 0;
    if(secondsDecimals > 3)
        secondsDecimals = 3;
    long long seconds = millis / 1000;
    long long minutes = (int)seconds / 60;
    long long hours = minutes / 60;
    long long days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    char *w = s;
    w[0] = 0;

    bool showSeconds = seconds || secondsDecimals || (millis < 1000);

    if(days)
        w += sprintf(w, "%lldd", days);
    if(hours)
        w += sprintf(w, "%lldh", hours);
    if(minutes)
        w += sprintf(w, "%lldm", minutes);
    if(showSeconds)
        w += sprintf(w, "%lld", seconds);
    if(secondsDecimals)
    {
        char f[6];
        strcpy(f,".%0^d");
        f[3] = '0' + secondsDecimals;
        int x = millis % 1000;
        for(int ix = secondsDecimals; ix < 3; ix++)
            x /= 10;
        w += sprintf(w, f, x);
    }
    if(showSeconds)
        w += sprintf(w, "s");

    return s;
}


int omStringToInt(const char *s, int fixedDecimals)
{
    int k = (int)strlen(s);
    int radix = 10;
    if(k >= 3 && s[0] == '0' && s[1] == 'x') // detect 0x for hex.
    {
        radix = 16;
        s += 2;
    }
    int x = 0;
    bool sign = false;
    bool pastDecimal = false;
    while(char c = *s++)
    {
        if(c == '-')
            sign = !sign;
        else if(c == '.')
            pastDecimal = true;
        else
        {
            if(pastDecimal)
            {
                fixedDecimals--;
                if(fixedDecimals < 0)
                    break;
            }
            int d = 0;
            if(c >= 'A' && c <= 'Z')
                c += 'a' - 'A';
            else if(c >= 'a' && c <= 'f')
                d = c - 'a' + 10;
            else if(c >= '0' && c <= '9')
                d = c - '0';
            x = x * radix;
            x += d;
        }
    }
    while(fixedDecimals-- > 0)
        x *= radix;

    if(sign)
        x = -x;
    return x;
}


const char *omIpToString(unsigned int ip, bool flip)
{
    static char s[20];
    if(!flip)
        sprintf(s, "%d.%d.%d.%d", (int)(ip >> 24) & 0xff, (int)(ip >> 16) & 0xff, (int)(ip >> 8) & 0xff, (int)(ip >> 0) & 0xff);
    else
        sprintf(s, "%d.%d.%d.%d", (int)(ip >> 0) & 0xff, (int)(ip >> 8) & 0xff, (int)(ip >> 16) & 0xff, (int)(ip >> 24) & 0xff);
    return s;
}

const char *omIpToString(unsigned char ip[4])
{
    static char s[20];
    sprintf(s, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return s;
}

int omHexToInt(const char *s, int digitCount)
{
    int result = 0;
    while(digitCount--)
    {
        int d;
        char c = *s++;
        if(c >= '0' && c <= '9')
            d = c - '0';
        else if(c >= 'a' && c <= 'f')
            d = c - 'a' + 10;
        else if(c >= 'A' && c <= 'F')
            d = c - 'A' + 10;
        else break;
        
        result = result * 16 + d;
    }
    return result;
}

void omHsvToRgb(unsigned char *hsvIn, unsigned char *rgbOut)
{
    unsigned char h = hsvIn[0];
    unsigned char s = hsvIn[1];
    unsigned char v = hsvIn[2];
    
    s = 255 - s; // now it's more like the "whiteness level", or min per component
    s = s * v / 255; // scaled by brightness.

#define otrMap(_x, _inLow, _inHigh, _outLow, _outHigh) (_x - _inLow) * (_outHigh - _outLow) / (_inHigh - _inLow) + _outLow
    if(h < 43)
    {
        rgbOut[0] = v;
        rgbOut[1] = otrMap(h, 0, 43, s, v);
        rgbOut[2] = s;
    }
    else if(h < 85)
    {
        rgbOut[0] = otrMap(h, 43, 85, v, s);
        rgbOut[1] = v;
        rgbOut[2] = s;
    }
    else if(h < 128)
    {
        rgbOut[0] = s;
        rgbOut[1] = v;
        rgbOut[2] = otrMap(h, 85, 128, s, v);
    }
    else if(h < 170)
    {
        rgbOut[0] = s;
        rgbOut[1] = otrMap(h, 128, 170, v, s);
        rgbOut[2] = v;
    }
    else if(h < 213)
    {
        rgbOut[0] = otrMap(h, 170, 217, s, v);
        rgbOut[1] = s;
        rgbOut[2] = v;
    }
    else /*if(h <= 255) always is */
    {
        rgbOut[0] = v;
        rgbOut[1] = s;
        rgbOut[2] = otrMap(h, 213, 256, v, s);
    }
}

void omRgbToHsv(unsigned char *rgbIn, unsigned char *hsvOut)
{
#define rgbR rgbIn[0]
#define rgbG rgbIn[1]
#define rgbB rgbIn[2]
#define hsvH hsvOut[0]
#define hsvS hsvOut[1]
#define hsvV hsvOut[2]
    unsigned char rgbMin, rgbMax;

    rgbMin = rgbR < rgbG ? (rgbR < rgbB ? rgbR : rgbB) : (rgbG < rgbB ? rgbG : rgbB);
    rgbMax = rgbR > rgbG ? (rgbR > rgbB ? rgbR : rgbB) : (rgbG > rgbB ? rgbG : rgbB);

    hsvV = rgbMax;
    if (hsvV == 0)
    {
        hsvH = 0;
        hsvS = 0;
        return;
    }

    hsvS = 255 * long(rgbMax - rgbMin) / hsvV;
    if (hsvS == 0)
    {
        hsvH = 0;
        return;
    }

    if (rgbMax == rgbR)
        hsvH = 0 + 43 * (rgbG - rgbB) / (rgbMax - rgbMin);
    else if (rgbMax == rgbG)
        hsvH = 85 + 43 * (rgbB - rgbR) / (rgbMax - rgbMin);
    else
        hsvH = 171 + 43 * (rgbR - rgbG) / (rgbMax - rgbMin);

    return;
}

int omMigrate(int x, int dest, int delta)
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


/// pin range EXCLUSIVE of top
int omPinRange(int x, int low, int high)
{
    if(low > high)
    {
        int t = low;
        low = high;
        high = t;
    }
    if(x < low)
        x = low;
    else if(x >= high)
        x = high - 1;
    return x;
}
