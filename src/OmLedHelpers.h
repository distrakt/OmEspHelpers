#ifndef _OMLEDS_H_
#define _OMLEDS_H_
//
// for desktop test builds, preprocessor defines these.
// for arduino, here's where they arrive.
#ifndef DIGITAL_WRITE
#define DIGITAL_WRITE(_pin, _value) digitalWrite(_pin, _value)
#define PIN_MODE(_pin, _mode) pinMode(_pin, _mode)
#endif
//
#include "OmLedT.h"
#include "OmLedTStrip.h"
#include "OmLedTGrid.h"
#include "OmLedTPatterns.h"
#include "OmLedTPatternManager.hpp"

#include "OmSk9822.h"
#include "OmWs2812.h"
#include "OmLedUtils.h"

#endif // _OMLEDS_H_
