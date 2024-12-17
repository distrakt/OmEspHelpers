//
//  OmUtil.h
//  OmEspHelpers
//
//  Created by David Van Brink on 11/22/16.
//  Copyright (c) 2016 omino.com. All rights reserved.
//

#ifndef __OmUtil__
#define __OmUtil__

#include <string>
#include <vector>

bool omStringEqual(const char *s1, const char *s2, int maxLen = 100);
/*! @brief Represent a number of milliseconds as a duration string, good for "uptime" displays. 1d2h3m4s like.
 result is in a shared static char[], so use or copy soon.
 */
const char *omTime(long long millis, int secondsDecimals = 0);
/*! @brief string pointer to int, very forgiving
    results undefined for nonnumeric characters
    reads 0xAbCd hex also. Can also specify a sort of fixed point,
    so omStringToInt("1.234", 3) gives 1234.
 */
int omStringToInt(const char *s, int fixedDecimals = 0);
/*! @brief ip to string
 flip=true for ESP library IPAddress object.);
 */
const char *omIpToString(unsigned int, bool flip = false);
/*! @brief ip to string */
const char *omIpToString(unsigned char ip[4]);
int omHexToInt(const char *s, int digitCount);
void omHsvToRgb(unsigned char *hsvIn, unsigned char *rgbOut);
void omRgbToHsv(unsigned char *rgbIn, unsigned char *hsvOut);
int omMigrate(int x, int dest, int delta);
int omPinRange(int x, int low, int high);
std::vector<std::string> omSplit(std::string s, std::string breaker, bool doStringTrim = true, bool discardEmpties = true);
bool omContains(std::string s, std::string middle);
bool omStartsWith(std::string s, std::string starter);
std::string ssnprintf(const char *fmt, va_list v);
std::string ssprintf(std::string fmt, ...);
std::string ssprintf(const char *fmt, ...);



#endif /* defined(__OmUtil__) */
