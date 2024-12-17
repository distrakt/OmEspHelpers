//
//  OmLedTPatternManager.hpp
//  PatternsTester
//
//  Created by David Van Brink on 3/23/21.
//  Copyright © 2021 David Van Brink. All rights reserved.
//

#ifndef OmLedTPatternManager_hpp
#define OmLedTPatternManager_hpp

#include "OmLedTPatterns.h"

#define CROSSFADE_PARAM_MAX 12 // we allocate space for this many cross-fadeable params, max.

template <typename LEDT>
class OmLedTPatternManager
{
public:
    typedef OmLedTPattern<LEDT> PATTERNT;
    typedef OmLedTStrip<LEDT> STRIPT;

    int ledCount = 0;
    std::vector<PATTERNT *> patterns;
    int currentPatternIndex = -1;
    PATTERNT *currentPattern = NULL;
#define PM_HISTORY_SIZE 20
    int msHistoryIx = 0;
    int msHistory[PM_HISTORY_SIZE];

    // the cross fade pattern is the OLD pattern.
    // we set the new one immediately so the parameters are current.
    unsigned int crossfadeMs = 0;
    unsigned int crossfadeMsSoFar = 0;
    PATTERNT *crossfadePattern = NULL;
    int crossfadePatternIndex;
    STRIPT *crossfadeStrip = NULL;
    
    int crossfadeFirstParam = 0;
    int crossfadeParamCount = 0;
    uint32_t crossfadeParamsStart[CROSSFADE_PARAM_MAX];
    uint32_t crossfadeParamsTarget[CROSSFADE_PARAM_MAX];

    PATTERNT *addPattern(PATTERNT *pattern)
    {
        this->patterns.push_back(pattern);
        return pattern;
    }

    void initPatterns(int ledCount)
    {
        this->ledCount = ledCount;
        for(PATTERNT *pattern : this->patterns)
        {
            pattern->init(ledCount);
            pattern->midiPanic();
        }

        for(int ix = 0; ix < PM_HISTORY_SIZE; ix++)
            this->msHistory[ix] = 0;
    }

    int getPatternIndex(const char *patternName)
    {
        for(int px = 0; px < this->patterns.size(); px++)
        {
            if(strcmp(patternName, this->patterns[px]->getName()) == 0)
                return px;
        }
        return -1;
    }

    int getPatternCount()
    {
        return (int)this->patterns.size();
    }

    PATTERNT *getPattern(int ix)
    {
        if(ix < 0 || ix >= this->patterns.size())
            return NULL;
        return this->patterns[ix];
    }

    PATTERNT *getPattern()
    {
        return this->getPattern(this->currentPatternIndex);
    }

    int getPatternIndex()
    {
        return this->currentPatternIndex;
    }
    
    void setPattern(int nextPatternIx, unsigned int ms)
    {
        this->setPattern(nextPatternIx, 0,0,NULL, ms);
    }

    /// Change to pattern, crossfaded over some milliseconds.
    /// TODO: 2024-04-02 jeez, check params for NULL
    /// TODO: 2024-07-07 remove first parameter... it's always zero. only fade up to CROSSFADE_PARAM_MAX, any after just set immediately. done and done. yih.
    void setPattern(int nextPatternIx, int firstParam, int paramCount, uint32_t *params, unsigned int ms)
    {
        if(!params)
            paramCount = 0; /// TODO: 2024-04-02 is this sufficient? or do we need default params
#if 0
        // bypass the same-pattern case below til we get it right
        if(this->currentPatternIndex == nextPatternIx)
        {
            if(params && this->currentPattern)
                this->currentPattern->setParams(firstParam, paramCount, params);
            return;
        }
#endif
        // make copy of the current params and target params.
        this->crossfadeFirstParam = firstParam;
        if(paramCount > CROSSFADE_PARAM_MAX)
            paramCount = CROSSFADE_PARAM_MAX;
        this->crossfadeParamCount = paramCount;
        for(int px = 0; px < crossfadeParamCount; px++)
        {
            this->crossfadeParamsStart[px] = 0; // zero-out the current/start values
            this->crossfadeParamsTarget[px] = params[px];
        }
        if(this->currentPattern) // if there's a pattern, populate the current values
            this->currentPattern->getParams(firstParam, paramCount, this->crossfadeParamsStart);

        PATTERNT *nextPattern = NULL;
        if(nextPatternIx < 0 || nextPatternIx >= this->patterns.size())
            nextPatternIx = 0;
        nextPattern = this->patterns[nextPatternIx];
//        {
//            nextPattern = NULL;
//            nextPatternIx = -1;
//        }
//        else
//        {
//            nextPattern = this->patterns[nextPatternIx];
//        }

        if(nextPatternIx != this->currentPatternIndex)
        {
            // if the patterns are different, set the params (if any) and clear the param pointer.
            // they wont be needed
            if(params != NULL && nextPattern != NULL)
            {
                nextPattern->setParams(firstParam, paramCount, params);
                this->crossfadeParamCount = 0;
            }
        }
    
        this->crossfadeMs = ms;
        this->crossfadeMsSoFar = 0;
        this->crossfadePattern = this->currentPattern;
        this->crossfadePatternIndex = this->currentPatternIndex;

        // change it right now, in place
        this->currentPattern = nextPattern;
        this->currentPatternIndex = nextPatternIx;
    }

    void setPattern(int ix)
    {
        this->setPattern(ix, 0);
    }

    /// Save the params into an array of ints. IF TRANSITIONING show the target params (not the partly faded ones)
    void getParams(int firstParam, int paramCount, uint32_t *storeThemHere)
    {
        for(int ix = 0; ix < paramCount; ix++)
            storeThemHere[ix] = 0;

        if(this->crossfadeParamCount)
        {
            for(int ix = 0; ix < paramCount; ix++)
            {
                int k = ix + firstParam;
                if(k >= 0 && k < this->crossfadeParamCount)
                    storeThemHere[ix] = this->crossfadeParamsTarget[k];
            }
        }
        else if(this->currentPattern)
        {
            this->currentPattern->getParams(firstParam, paramCount, storeThemHere);
        }
    }

    void tickInto(unsigned int ms, STRIPT *ledStrip, PATTERNT *pattern)
    {
        if(pattern)
            pattern->tick(ms, ledStrip);
        else
            ledStrip->clear();
    }
    
    /// specialized migrate for morphing param values
    static int migrate(int currentValue, int targetValue, float portion)
    {
        float distance = fabs((float)targetValue - (float)currentValue);
        float migrateAmountF = distance * portion;
        int migrateAmountI = migrateAmountF;
        float migrateAmountFract = migrateAmountF - migrateAmountI;
        // stochastically handle the fraction.
        if(rr(1.0) < migrateAmountFract)
            migrateAmountI ++;
        // redo the newvalue
        int newValue = migrateI(currentValue, targetValue, migrateAmountI);

        return newValue;
    }

    /// incorporate the delta milliseconds into the frame rate computation
    /// so if the patterns are bypassed, can still share this mechanism.
    void tickStats(unsigned int ms)
    {
        this->msHistoryIx = (this->msHistoryIx + 1) % PM_HISTORY_SIZE;
        this->msHistory[this->msHistoryIx] = ms;
    }

    void tick(unsigned int ms, STRIPT *ledStrip)
    {
        // ms is how far addvanced.
        this->tickStats(ms);

        if(this->crossfadeMs)
        {
//            printf("fading %d of %d\n", this->crossfadeMsSoFar, this->crossfadeMs);
            // we are crossfading! so be it.
            if(this->crossfadeParamCount)
            {
                // newfangled param morphing
                this->crossfadeMsSoFar += ms;
                PATTERNT *cp = this->currentPattern;
                if(this->crossfadeMsSoFar >= this->crossfadeMs)
                {
                    // we're done crossfading. we have arrived.
                    cp->setParams(this->crossfadeFirstParam,
                                                    this->crossfadeParamCount,
                                                    this->crossfadeParamsTarget);
//                    printf("Y done interpolating #%06x, #%06x\n", this->crossfadeParamsTarget[0], this->crossfadeParamsTarget[1]);
                    this->crossfadeParamCount = 0;
                    this->crossfadeMs = 0;
                    this->crossfadeMsSoFar = 0;
                    goto singlePattern;
                }

                // ok yes still crossfade ing
                for(int ix = this->crossfadeFirstParam; ix < this->crossfadeFirstParam + this->crossfadeParamCount; ix++)
                {
                    if(ix < 0 || ix >= cp->params.size())
                        break;

                    EOmLedPatternParamType paramType = cp->getParamType(ix);
                    uint32_t v0 = this->crossfadeParamsStart[ix - this->crossfadeFirstParam];
                    uint32_t v = this->crossfadeParamsTarget[ix - this->crossfadeFirstParam];

                    // if morphing params, 
                    // the amount to migrate, from where we are, is
                    // tick-milliseconds * distance ÷ remaining-milliseconds
                    // precompute the time portion. distance is per.

                    float fractionThere = (float)this->crossfadeMsSoFar / this->crossfadeMs;

                    switch(paramType)
                    {
                        case PPT_INT:
                        {
                            int newValue = mapRange(fractionThere, 0.0, 1.0, v0, v) + 0.5;
                            cp->setParamValueInt(ix, newValue);
                            break;
                        }
                        case PPT_COLOR:
                        {
                            LEDT vCo = LEDT(v);
                            LEDT v0Co = LEDT(v0);
                            LEDT newValue;

                            newValue.r = mapRange(fractionThere, 0.0, 1.0, v0Co.r, vCo.r);
                            newValue.g = mapRange(fractionThere, 0.0, 1.0, v0Co.g, vCo.g);
                            newValue.b = mapRange(fractionThere, 0.0, 1.0, v0Co.b, vCo.b);
                            cp->setParamValueColor(ix, newValue);

//                            printf("X interp #%06x - #%06x (%5.2f): #%06x\n", v0, v, fractionThere, newValue.toHex());
                            break;
                        }
                        default:
                            break;
                    }
                }
                // and before we go... tick the actual pattern, for pixels!
                this->tickInto(ms, ledStrip, this->currentPattern);
            }
            else
            {
                // visual brightness fade
                this->crossfadeMsSoFar += ms;
                if(this->crossfadeMsSoFar >= this->crossfadeMs)
                {
                    // we're done crossfading. we have arrived.
                    this->crossfadeMs = 0;
                    this->crossfadeMsSoFar = 0;
                    goto singlePattern;
                }

                // ok yes crossfade
                if(!this->crossfadeStrip)
                {
                    // could reallocate the strip if the ledCount differs, or ringLed0
                    this->crossfadeStrip = new STRIPT(this->ledCount);
                    this->crossfadeStrip->ringLed0 = ledStrip->ringLed0;
                }

                float t = (float)this->crossfadeMsSoFar / this->crossfadeMs;
                this->tickInto(ms, ledStrip, this->currentPattern);
                this->tickInto(ms, crossfadeStrip, crossfadePattern);

                *ledStrip *= t;
                *crossfadeStrip *= 1.0 - t;
                *ledStrip += crossfadeStrip;
            }
        }
        else
        {
        singlePattern:
            this->tickInto(ms, ledStrip, this->currentPattern);
        }
    }

    void getPerformanceInfo(int &millisecondsPerFrameOut, int &framesPerSecondOut)
    {
        float result = 0;
        for (int ix = 0; ix < PM_HISTORY_SIZE; ix++)
        {
            result += msHistory[ix];
        }
        result = result / PM_HISTORY_SIZE;
        if (result < 1)
            result = 1;
        millisecondsPerFrameOut = result + 0.5;
        framesPerSecondOut = (1000.0 / result) + 0.5;
    }

    // +------------------------------------------
    // | 2024 Midi Actions, for patterns that have it
    void midiKey(uint8_t pitch, uint8_t velocity)
    {
        if(this->currentPattern)
            this->currentPattern->midiKey(pitch, velocity);
    }

    void midiControl(uint8_t control, uint8_t value)
    {
        if(this->currentPattern)
            this->currentPattern->midiControl(control, value);
        return;
    }

    // do all notes off
    void midiPanic()
    {
        if(this->currentPattern)
            this->currentPattern->midiPanic();
        return;
    }


};

typedef OmLedTPatternManager<OmLed8> OmLed8PatternManager;
typedef OmLedTPatternManager<OmLed16> OmLed16PatternManager;

#endif /* OmLedTPatternManager_hpp */
