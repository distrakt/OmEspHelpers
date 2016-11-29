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
    while(char c = *s++)
    {
        x = x * 10;
        x += c - '0';
    }
    return x;
}


const char *omIpToString(unsigned long ip)
{
    static char s[20];
    sprintf(s, "%d.%d.%d.%d", (int)(ip >> 24) & 0xff, (int)(ip >> 16) & 0xff, (int)(ip >> 8) & 0xff, (int)(ip >> 0) & 0xff);
    return s;
}
