//
//  OmLedTPatterns.h
//  OmLedHelpersDev
//
//  Created by David Van Brink on 5/10/2020 Plague Time.
//  Copyright © 2020 David Van Brink. All rights reserved.
//

#ifndef __OmLedTPatterns_h__
#define __OmLedTPatterns_h__

#include "OmLedT.h"
#include "OmLedTStrip.h"
#include "OmLedUtils.h"
#include <stdio.h>
#include <vector>

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

typedef enum EOmLedPatternParamType
{
    PPT_NONE = 0,
    PPT_INT = 1,
    PPT_CHECKBOX = 2, // bitfield int
    PPT_COLOR = 3,
    PPT_BUTTON = 4 // button or action
} EOmLedPatternParamType;

template <typename LEDT>
class OmLedTPattern
{
protected:
    const char *name = NULL;
    char nameX[16];
    int ledCount;
    uint32_t totalMs;
    bool changed = true;
public:

    class OmLedPatternParam
    {
    public:
        OmLedPatternParam()
        {
            this->name = "";
            this->colorValue = LEDT(0,0,0);
            this->intValue = 0;
        }

        EOmLedPatternParamType type = PPT_INT;
        const char *name;
        const char *checkboxNames = "";
        union
        {
            LEDT colorValue;
            int32_t intValue;
        };
    };

    std::vector<OmLedPatternParam> params;

    OmLedTPattern()
    {
        return;
    }

    virtual ~OmLedTPattern()
    {
        return;
    }

    OmLedTPattern<LEDT> *setName(const char *name)
    {
        this->name = name;
        return this;
    }

    void init(int ledCount)
    {
        this->ledCount = ledCount;
        this->totalMs = 0;
        this->innerInit();
    }
    void tick(uint32_t deltaMs, OmLedTStrip<LEDT> *strip)
    {
        if(deltaMs > 500)
            deltaMs = 100;

        this->totalMs += deltaMs;
        if(strip)
            strip->clear();
        if(this->changed)
        {
            this->innerChanged();
            this->changed = false;
        }
        this->innerTick(deltaMs, strip);
    }

    const char *getName()
    {
        if(this->name)
            return name;
        return "unnamed";
    }

    /// for future expansion -- use bit 3 to mean, Supports MIDI
    virtual uint16_t getFlags()
    {
        return 0;
    }

    int getParamCount()
    {
        return (int)(this->params.size());
    }

    const char *getParamName(int ix)
    {
        if(ix < 0 || ix >= this->params.size())
            return NULL;
        return this->params[ix].name;
    }

    const char *getParamCheckboxNames(int ix)
    {
        if(ix < 0 || ix >= this->params.size())
            return NULL;
        return this->params[ix].checkboxNames;
    }

    EOmLedPatternParamType getParamType(int ix)
    {
        if(ix < 0 || ix >= this->params.size())
            return PPT_NONE;
        return this->params[ix].type;
    }

    LEDT getParamValueColor(int ix)
    {
        if(ix < 0 || ix >= this->params.size())
            return LEDT(0,0,0);
        OmLedPatternParam *pp = &this->params[ix];
        if(pp->type != PPT_COLOR)
            return LEDT(0,0,0);
        return pp->colorValue;
    }

    int getParamValueInt(int ix)
    {
        if(ix < 0 || ix >= this->params.size())
            return 0;

        OmLedPatternParam *pp = &this->params[ix];
        if(pp->type == PPT_COLOR)
            return pp->colorValue.toHex();
        if(pp->type == PPT_INT || pp->type == PPT_CHECKBOX)
            return pp->intValue;
        return 0;
    }

    void setParamValueColor(int ix, LEDT co)
    {
        if(ix < 0 || ix >= this->params.size())
            return;
        OmLedPatternParam *pp = &this->params[ix];
        if(pp->type == PPT_COLOR)
            pp->colorValue = co;
        if(pp->type == PPT_INT)
            pp->intValue = co.toHex();
        this->changed = true;
    }

    void setParamValueInt(int ix, int x)
    {
        if(ix < 0 || ix >= this->params.size())
            return;
        OmLedPatternParam *pp = &this->params[ix];
        if(pp->type == PPT_COLOR)
            pp->colorValue = LEDT(x);
        if(pp->type == PPT_INT || pp->type == PPT_CHECKBOX)
            pp->intValue = x;
        this->changed = true;
    }

    void doAction(unsigned int ix, bool buttonDown)
    {
        if(ix < this->params.size())
            this->innerDoAction(ix, buttonDown);
    }

    void doRotaryControl(int incDec, bool buttonDown)
    {
        if(incDec < 0)
            incDec = -1;
        else if(incDec > 0)
            incDec = +1;
        this->innerDoRotaryControl(incDec, buttonDown);
    }

    /// Save the params into an array of ints, you could write them to flash for later maybe
    void getParams(int firstParam, int paramCount, uint32_t *storeThemHere)
    {
        // set them all to zero, so excess storage wont cause unneeded eeprom writes if saved
        for(int ix = 0; ix < paramCount; ix++)
            storeThemHere[ix] = 0;

        for(int ix = firstParam; ix < firstParam + paramCount; ix++)
        {
            if(ix < 0 || ix >= this->params.size())
                break;
            OmLedPatternParam &pa = this->params[ix];
            uint32_t v = 0;
            switch(pa.type)
            {
                case PPT_INT:
                {
                    v = pa.intValue;
                    break;
                }
                case PPT_COLOR:
                {
                    v = pa.colorValue.toHex();
                    break;
                }
                default:
                    break;
            }
            *storeThemHere++ = v;
        }
    }

    /// Set some or all the params from an array of ints, perhaps restored from flash. Note --
    /// 16-bit colors won't come back identically, just 8 bits worth.
    void setParams(int firstParam, int paramCount, uint32_t *restoreFromHere)
    {
        for(int ix = firstParam; ix < firstParam + paramCount; ix++)
        {
            if(ix < 0 || ix >= this->params.size())
                break;
            OmLedPatternParam &pa = this->params[ix];
            uint32_t v = *restoreFromHere++;
            switch(pa.type)
            {
                case PPT_INT:
                {
                    pa.intValue = v;
                    break;
                }
                case PPT_COLOR:
                {
                    pa.colorValue = LEDT(v);
                    break;
                }
                default:
                    break;
            }
        }
        this->changed = true;
    }

    // +------------------------------------------
    // | 2024 Midi Actions You Could Implement
    virtual void midiKey(uint8_t pitch, uint8_t velocity)
    {
        return;
    }

    virtual void midiControl(uint8_t control, uint8_t value)
    {
        return;
    }

    // do all notes off
    virtual void midiPanic()
    {
        return;
    }

protected:
    void addColorParam(const char *name, LEDT value)
    {
        OmLedPatternParam p;
        p.type = PPT_COLOR;
        p.name = name;
        p.colorValue = value;
        this->params.push_back(p);
    }

    void addIntParam(const char *name, int value)
    {
        OmLedPatternParam p;
        p.type = PPT_INT;
        p.name = name;
        p.intValue = value;
        this->params.push_back(p);
    }

    void addAction(const char *name)
    {
        OmLedPatternParam p;
        p.type = PPT_BUTTON;
        p.name = name;
        this->params.push_back(p);
    }
    
    void addCheckboxParam(const char *name, const char *checkboxNames, int value)
    {
        OmLedPatternParam p;
        p.type = PPT_CHECKBOX;
        p.name = name;
        p.checkboxNames = checkboxNames;
        p.intValue = value;
        this->params.push_back(p);
    }

    virtual void innerInit() = 0;
    virtual void innerChanged() {};
    virtual void innerTick(uint32_t milliseconds, OmLedTStrip<LEDT> *strip) = 0;
    virtual void innerDoAction(unsigned int ix, bool buttonDown) { UNUSED(ix); UNUSED(buttonDown); };
    
    bool rotaryButtonIsDown = false;
    virtual void innerDoRotaryControl(int incDec, bool buttonDown)
    {
        // default implementation changes the first slider or second slider.
        if(incDec)
        {
            unsigned int paramIx = this->rotaryButtonIsDown ? 1 : 0;
            // look for the 0th or 1st int param, if any...
            int pCount = this->getParamCount();
            int intsFound = 0;
            for(int ix = 0; ix < pCount; ix++)
            {
                if(this->getParamType(ix) == PPT_INT)
                {
                    if(intsFound == paramIx)
                    {
                        int v = this->getParamValueInt(ix);
                        v = pinRange(v + incDec, 0, 100);
                        this->setParamValueInt(ix, v);
                    }
                    intsFound++;
                }
            }
        }
        else
        {
            this->rotaryButtonIsDown = buttonDown;
        }
    }
};

typedef OmLedTPattern<OmLed16> OmLed16Pattern;
typedef OmLedTPattern<OmLed8> OmLed8Pattern;

class OmLed16PatternCrossfade
{
public:
    OmLed16Strip *strip0;
    OmLed16Strip *strip1;
    unsigned int transitionTime;
    unsigned int currentTime = 0;

    OmLed16PatternCrossfade(OmLed16Strip *strip0, OmLed16Strip *strip1, unsigned int transitionTime)
    {
        this->strip0 = strip0;
        this->strip1 = strip1;
        if(transitionTime < 1)
            transitionTime = 1;
        this->transitionTime = transitionTime;
    }

    /// return "true" if it's still crossfading.
    bool tick(uint32_t ms, OmLed16Strip *strip)
    {
        bool result = true;
        this->currentTime += ms;
        if(this->currentTime >= this->transitionTime)
            result = false;

        float w1 = (float)this->currentTime / this->transitionTime;
        float w0 = 1.0 - w1;

        for(int ix = 0; ix < strip->ledCount; ix++)
        {
            strip->leds[ix] = strip0->leds[ix] * w0 + strip1->leds[ix] * w1;
            //            strip->leds[ix] = OmLed16(0,0,0);
        }

        return result;
    }
};

class SimplePattern : public OmLed16Pattern
{
public:
    float k = 0;
    bool b0 = false;
    void innerInit() override
    {
        this->addColorParam("color", OmLed16(0x8000, 0x4000, 0x4000));
        this->addIntParam("rate", 1);
        this->addAction("go");

//        auto a = this->getColorParamNames();
//        auto b = this->getColorParamValues();
//        auto c = this->getIntParamNames();
//        auto d = this->getIntParamValues();
//        auto e = this->getActionNames();
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        float rate = this->getParamValueInt(1);// this->getIntParamValues()[0];
        OmLed16 color = this->getParamValueColor(0); // this->getColorParamValues()[0];

        this->k += ms * rate / 1000.0;

        if(strip)
        {
            strip->fillRange(this->k, this->k + 1, color);
        }
    }

    void innerDoAction(unsigned int ix, bool buttonDown) override
    {
        this->b0 = buttonDown;
    }
};

#endif // __OmLedTPatterns_h__
