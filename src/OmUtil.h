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
const char *omTime(unsigned long int millis);
int omStringToInt(const char *s);
const char *omIpToString(unsigned long ip);

#endif /* defined(__OmUtil__) */
