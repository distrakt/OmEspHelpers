//
//  OmUtil.h
//  OmEspHelpers
//
//  Created by David Van Brink on 11/22/16.
//  Copyright (c) 2016 omino.com. All rights reserved.
//

#ifndef __OmUtil__
#define __OmUtil__

bool omStringEqual(const char *s1, const char *s2, int maxLen = 100);
/// Represent a number of milliseconds as a duration string, good for "uptime" displays. 1d2h3m4s like.
const char *omTime(long long millis, int secondsDecimals = 0);
int omStringToInt(const char *s);
const char *omIpToString(unsigned long ip, bool flip = false); // flip=true for ESP library IPAddress object.);
const char *omIpToString(unsigned char ip[4]);
int omHexToInt(const char *s, int digitCount);
void omHsvToRgb(unsigned char *hsvIn, unsigned char *rgbOut);
void omRgbToHsv(unsigned char *rgbIn, unsigned char *hsvOut);
int omMigrate(int x, int dest, int delta);
int omPinRange(int x, int low, int high);



#endif /* defined(__OmUtil__) */
