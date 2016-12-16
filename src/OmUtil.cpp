//
//  OmUtil.h
//  OmEspHelpers
//
//  Created by David Van Brink on 11/22/16.
//  Copyright (c) 2016 omino.com. All rights reserved.
//

#include "OmUtil.h"
#include <stdio.h>

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

const char *omTime(unsigned long int millis)
{
    static char s[100];
    unsigned long int seconds = millis / 1000;
    int minutes = (int)seconds / 60;
    int hours = minutes / 60;
    int days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    char *w = s;
    if(days)
        w += sprintf(w, "%dd", days);
    if(hours)
        w += sprintf(w, "%dh", hours);
    if(minutes)
        w += sprintf(w, "%dm", minutes);
    if(seconds)
        w += sprintf(w, "%ds", (int)seconds);
    
    return s;
}


int omStringToInt(const char *s)
{
    int x = 0;
    bool sign = false;
    while(char c = *s++)
    {
        if(c == '-')
            sign = !sign;
        else
        {
            x = x * 10;
            x += c - '0';
        }
    }
    if(sign)
        x = -x;
    return x;
}


const char *omIpToString(unsigned long ip)
{
    static char s[20];
    sprintf(s, "%d.%d.%d.%d", (int)(ip >> 24) & 0xff, (int)(ip >> 16) & 0xff, (int)(ip >> 8) & 0xff, (int)(ip >> 0) & 0xff);
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
    else if(h <= 255)
    {
        rgbOut[0] = v;
        rgbOut[1] = s;
        rgbOut[2] = otrMap(h, 213, 256, v, s);
    }
}
