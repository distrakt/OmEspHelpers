//
//  OmLedPatterns2.h
//  a second batch of available patterns
//
//  Created by David Van Brink on 2022-08-31
//

#ifndef OmLedPatterns2_h
#define OmLedPatterns2_h

#include "OmLedTGrid.h"

static void migrateCo(OmLed16 &co, const OmLed16 &coDest, int rate)
{
    co.r = migrateI(co.r, coDest.r, rate);
    co.g = migrateI(co.g, coDest.g, rate);
    co.b = migrateI(co.b, coDest.b, rate);
}

class MsTicker
{
public:
    unsigned int msOn;
    unsigned int msOff;
    unsigned int msPeriod;
    unsigned int msPhase = 0;

    void init(unsigned int msOn, unsigned int msOff)
    {
        this->msOn = msOn;
        this->msOff = msOff;
        this->msPeriod = this->msOn + this->msOff;
        this->msPhase = 0;
    }

    int tick(unsigned int ms)
    {
        this->msPhase += ms;
        while(this->msPhase > this->msPeriod)
            this->msPhase -= this->msPeriod;
        int result = this->msPhase <= this->msOn ? 1 : 0;
        return result;
    }
};

class Signals1 : public OmLed16Pattern
{
public:
    /// base class for "signal" elements
    /// signal pattern is made up with collection of Signal elements.
    class Signal
    {
    public:
        float width;
    protected:
        unsigned int phase = 0;

        unsigned int period = 1000;
        OmLed16 signalCo = OmLed16(65535,0,0);
        OmLed16 signalCo2 = OmLed16(0,24000,24000); // background for example
        float alpha = 0.0;
        unsigned int steps = 2;
        bool flip;

    public:

        virtual ~Signal() = default;
        void signalInit(float width)
        {
            this->width = width;
            this->init(); // inner init must be implemented by subclass
        }

        void signalTick(unsigned int ms, OmLed16Strip *strip, float xLow)
        {
            this->phase += ms;
            this->ixF = migrateF(this->ixF, this->ix, this->ixFMigrateAmount);
            while(this->phase >= this->period)
                this->phase -= this->period;
            this->tick(ms, strip, xLow);
        }

        void signalSetPeriod(unsigned int ms, bool resync)
        {
            this->period = ms;
            if(resync)
                this->phase = 0;
            this->setPeriod(ms, resync);
        }

        /// @param steps segments or similar. Negative means to "flip" the phase or similar.
        void signalSetSteps(int steps)
        {
            if(steps < 0)
            {
                this->steps = -steps;
                this->flip = true;
            }
            else
            {
                this->steps = steps;
                this->flip = false;
            }

            if(this->steps < 2)
                this->steps = 2;
            this->setSteps(steps);
        }

        void signalSetColor(OmLed16 co, float alpha)
        {
            this->signalCo = co;
            this->alpha = alpha;
        }

        void signalSetColor2(OmLed16 co)
        {
            this->signalCo2 = co;
        }

        int ix; // its logical index number...
        float ixF; // its current, migrating index number
        float ixFMigrateAmount = .1; // how much to move towards real index per tick

    protected:
        // +-----------------
        // | methods that must be provided by subclass
        virtual void init() = 0;
        virtual void tick(unsigned int ms, OmLed16Strip *strip, float xLow) = 0;

        // +-----------------
        // | These setters can be interpreted by the effect in various ways
        /// set the general speed of the signal
        /// @param ms cycle length of the effect, or of one step, whatever is best
        /// @param resync if true, start the effect at 0 (or nearest beat or whatever)
        virtual void setPeriod(unsigned int ms, bool resync) = 0;

        /// @param steps could mean the number of segments in a blinker for example. Or ignore it.
        virtual void setSteps(unsigned int steps) = 0;

        /// tint the effect to a new color
        /// @param co a color
        /// @param alpha amount of color to influence; 0 means signal's own color
        virtual void setColor(OmLed16 co, float alpha) = 0;

        /* pasteable
        void init()override{}
        void tick(unsigned int ms, OmLed16Strip *strip, float xLow)override{}
        void setPeriod(unsigned int ms, bool resync)override{}
        void setSteps(unsigned int steps)override{}
        void setColor(OmLed16 co, float alpha)override{}
        */
    };

    class Blinker : public Signal
    {
    public:
        OmLed16 co;
        OmLed16 co2;

        void init() override
        {
            return;
        }
        void tick(unsigned int ms, OmLed16Strip *strip, float xLow) override
        {
            migrateCo(this->co, this->signalCo, 2500);
            migrateCo(this->co2, this->signalCo2, 80);

            float pipWidth = this->width / this->steps;
            strip->fillRange(xLow, xLow + this->width, this->co2);
#if 0
            int step = this->phase * this->steps / this->period;
            if(this->flip)
                step = this->steps - 1 - step;
            float x0 = xLow + mapRange(step, 0, this->steps, 0, this->width);
#else
            // try a smoothed transition...
            float step = (float)this->phase * this->steps / this->period;
            float stepWhole = floor(step);
            float stepPart = step - stepWhole;
            float stepThresh = 0.85;
            if(stepPart > stepThresh)
            {
                int nextStep = stepWhole + 1;
                if(nextStep >= this->steps)
                    nextStep = 0;
                step = mapRange(stepPart, stepThresh, 1.0, stepWhole, nextStep);
            }
            else
                step = stepWhole;
            float x0 = mapRange(step, 0, this->steps, 0, this->width);
            if(this->flip)
                x0 = this->width - pipWidth - x0;
            x0 += xLow;
#endif


            float x1 = x0 + pipWidth;
            strip->fillRange(x0, x1, this->co, MODE_REPLACE);
        }
        void setPeriod(unsigned int ms, bool resync)override{}
        void setSteps(unsigned int steps)override{}
        void setColor(OmLed16 co, float alpha)override{}

    };


    class Sprite : public Signal
    {
    public:
        OmLed16 co;

        OmLed16 coEdge; // outer pixels
        OmLed16 coBlinker; // the blinking inner element

        MsTicker tOn; // rate of on-off blinking
        MsTicker tWidth; // rate of narrow/wide
    protected:
        void init() override
        {
            this->co = OmLed16::hsv(ir(0, 65536), 65000, 65000);
            this->coEdge = OmLed16::hsv(ir(0, 65536), 65000, 65000);
            this->coBlinker = OmLed16::hsv(ir(0, 65536), 65000, 65000);

            this->tOn.init(ir(100,600), ir(100, 600));
            this->tWidth.init(ir(100,600), ir(100, 600));

            this->tOn.init(ir(1000,1050), ir(1000,1050));
            this->tWidth.init(ir(1000,1050), ir(1000,1050));

            static int zzz = 1000;
            this->tOn.init(zzz, zzz+1);
            zzz += 3;
            this->tWidth.init(zzz/2, zzz/3);


        }

        // tick and render the sprite, at position xLow.
        void tick(unsigned int ms, OmLed16Strip *strip, float xLow) override
        {
            bool bOn = this->tOn.tick(ms);
            bool bWide = this->tWidth.tick(ms);

            strip->setLed(xLow, this->coEdge);
            strip->setLed(xLow + this->width - 1, this->coEdge);
            strip->setLed(xLow, this->coBlinker * 0.5);
            strip->setLed(xLow + this->width - 1, this->coBlinker * 0.5);
            if(bOn)
            {
                OmLed16 co = this->coBlinker;
                int x0 = xLow + 1;
                int x1 = xLow + this->width - 1;
                if(!bWide)
                {
                    x0 += 3;
                    x1 -= 3;
                }
                else
                {
                    co = co * 0.3;
                }

                for(int ix = x0; ix < x1; ix++)
                    strip->setLed(ix, co);
            }
        }

        void setPeriod(unsigned int ms, bool resync)override{} // noop
        void setSteps(unsigned int steps)override{}
        void setColor(OmLed16 co, float alpha)override{} // noop


    };

    std::vector<Signal *> signals;
    float width = 5.0;
    float signalCount = 6.0; // migrate the count
    
    // randomized versions
    int rWidthParam;
    int rSignalCountParam;
    int bigChangeCount = 0;
    float rSignalCountParamMigrateRate = 0.03;

    void ensureSignalCount()
    {
        int count = ceil(this->signalCount);
        count = count | 0x01;  // for centering, we need an odd number of actual pips
        int oldCount = (int)this->signals.size();
        if(oldCount > count)
        {
            // delete any with ix > count, and move others lower in list
            for(int ix = 0; ix < oldCount; ix++)
            {
                auto aSignal = this->signals[ix];
                if(aSignal->ix >= count)
                {
                    // this one is too high.
                    delete this->signals[ix];
                    this->signals[ix] = NULL;
                    this->signals[ix] = this->signals[oldCount - 1]; // may do nothing
                    oldCount--;
                    ix--; // backtrack to continue
                }
            }
//            for(int ix = count; ix < oldCount; ix++)
//            {
//                delete this->signals[ix];
//                this->signals[ix] = 0;
//            }
            this->signals.resize(count);
        }
        else if(count > oldCount)
        {
            for(int ix = oldCount; ix < count; ix++)
            {
                Signal *b = new Blinker();
                b->ix = ix;

                bool flip = false;
                int ixPrime = ix;
                if(ix >= count / 2)
                {
                    flip = true;
                    ixPrime = count - 1 - ix;
                    b->signalSetColor(OmLed16(0,0,64000), 1.0);
                }
                int steps = 2 + ixPrime /2;
                if(flip)
                    steps = -steps;
                b->signalSetSteps(steps);
                b->signalSetPeriod(1000 + ixPrime * 250, true);

                Signal *s = b;
                s->signalInit(this->width);
                s->ix = ix;
                s->ixF = ix;
                this->signals.push_back(s);
            }
        }
    }
    void innerInit() override
    {
        this->name = "Signals1";
        this->addIntParam("brightness", 80);
        this->addIntParam("count", 9);
        this->addIntParam("width", 12);
//        this->addCheckboxParam("checks", "a,b,c", 0x6);

        this->ensureSignalCount();
    }
    
    void innerChanged() override
    {
        // user param, or program change...
        // go back to the pre-randomized params.
        // pattern does smoothing so it can still look pretty nice.
        this->rWidthParam = this->getParamValueInt(2);
        this->rSignalCountParam = this->getParamValueInt(1);
        if(this->rSignalCountParam < 1)
            this->rSignalCountParam = 1; // no div by zero eh
    }

    int bigChangeCountdown = 200;
    
    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
//        float widthParam = this->getParamValueInt(2);
//        int signalCountParam = this->getParamValueInt(1);
//        if(signalCountParam < 1)
//            signalCountParam = 1; // no div by zero eh
        this->width = migrateF(this->width, this->rWidthParam, 0.09);
        this->signalCount = migrateF(this->signalCount, this->rSignalCountParam, this->rSignalCountParamMigrateRate);

        this->ensureSignalCount();

        // let width be a proportion, ranging up to some overlap.
        float sWidth = mapRange(this->width, 0, 100, 10, this->ledCount / this->signalCount);

        float spaceAllocatedPerSignal = this->ledCount / this->signalCount;
        float zerothSignalCenter = spaceAllocatedPerSignal / 2.0;
        zerothSignalCenter = this->ledCount / 2.0;
        for(int ix = 0; ix < this->signals.size(); ix++)
        {
            Signal *s = this->signals[ix];
            
            s->width = sWidth;
            int sIx = s->ix;
            float sIxF = s->ixF;
            float xPos = (float)this->ledCount * (2 * ix + 1) / (2 * this->signalCount) - s->width / 2;
            xPos = zerothSignalCenter + sIxF * spaceAllocatedPerSignal - s->width / 2;
            if(sIx & 1)
                xPos = zerothSignalCenter + ((sIxF+1) /2.0) * spaceAllocatedPerSignal - s->width / 2.0;
            else
                xPos = zerothSignalCenter - (sIxF /2.0) * spaceAllocatedPerSignal - s->width / 2.0;

            s->signalTick(ms, strip, xPos);
        }

        int irK = ir(0,10000);
        if( irK > 9940)
        {
            // shuffle... any old time
            int k = ir(1,4);
            for(int ix = 0; ix < k; ix++)
            {
                int a = ir(k);
                int b = ir(k);
                swapo(this->signals[a]->ix, this->signals[b]->ix);
            }
        }
        

        if(this->bigChangeCountdown-- <= 0)
        {
            this->bigChangeCount++;
            this->bigChangeCountdown = 200 * ir(1,4);//ir(500, 2000);
            int thing = ir(0,3);
            OmLed16 co2 = OmLed16::hsv(ir(0, 65536), 40000, ir(100, 1000));
            
            if(ir(0,7) > 1)
            {
                // randomize the user params too maybe
                int widthParam = this->getParamValueInt(2);
                int signalCountParam = this->getParamValueInt(1);
                
                widthParam *= rr(0.3, 2.0);
                signalCountParam *= rr(0.25, 2.0);
                this->rSignalCountParamMigrateRate = rr(0.02, 0.13);
                
                widthParam = pinRangeI(widthParam, 10, 120);
                signalCountParam = pinRangeI(signalCountParam, 1, 30) | 1;

                this->rWidthParam = widthParam;
                this->rSignalCountParam = signalCountParam;
            }
            
            switch(thing)
            {
                case 0:
                {
                    // new color, all same rate
                    OmLed16 co = OmLed16::hsv(ir(0,65536), 65535, ir(30000,60000));
                    int p = ir(1200, 5000);
                    int st = ir(2,7);
                    int flipAltMask = 1<<ir(1,6);
                    int ix = ir(0,100);
                    for(Signal *s : this->signals)
                    {
                        s->signalSetPeriod(p, true);
                        s->signalSetColor(co, 1.0);
                        s->signalSetColor2(co2);
                        s->signalSetSteps((ix & flipAltMask) ? -st : st);
                        ix++;
                    }
                    break;
                }

                case 1:
                {
                    // all different
                    for(Signal *s : this->signals)
                    {
                        OmLed16 co = OmLed16::hsv(ir(0,65536), 65535, ir(30000,60000));
                        int p = ir(1200, 5000);
                        int st = ir(-6,6);
                        s->signalSetPeriod(p, true);
                        s->signalSetColor(co, 1.0);
                        s->signalSetColor2(co2);
                        s->signalSetSteps(st);
                    }
                    break;
                }

                case 2:
                {
                    // in sequence
                    int ix = 0;
                    int p = ir(400, 1300);
                    int h0 = ir(65536);
                    for(Signal *s : this->signals)
                    {
                        ix++;
                        int h = ix * 65536 / signalCount;
                        OmLed16 co = OmLed16::hsv(h0 + h, 65535, 40000);
                        co2 = OmLed16::hsv(h, 35000, ir(100, 1000)); // co2 should be rather dark.
                        s->signalSetPeriod(p, true);
                        s->signalSetColor(co, 1.0);
                        s->signalSetColor2(co2);
                    }
                    break;
                }
            }
        }

        float brightness = this->getParamValueInt(0) / 100.0;
        brightness = pow(brightness, 2.5);
        OmLed16 *leds = strip->getLeds();
        for(int ix = 0; ix < strip->getSize(); ix++)
            leds[ix] *= brightness;

        return;
    }
};



class Signals2 : public OmLed16Pattern
{
public:

    /// a fixed mid-bar
    class Indicator1 : public Signals1::Signal
    {
    public:
        int a; // which segment to light
        int b; // out of how many
        void init()override{}
        void tick(unsigned int ms, OmLed16Strip *strip, float xLow)override
        {
            int x0 = this->width * this->a / this->b + xLow;
            int x1 = this->width * (this->a+1) / this->b + xLow;
            strip->fillRange(x0, x1, this->signalCo);
        }
        void setPeriod(unsigned int ms, bool resync)override{}
        void setSteps(unsigned int steps)override{}
        void setColor(OmLed16 co, float alpha)override{}
    };

    /// three bars, center bar pulses width
    class Indicator2 : public Signals1::Signal
    {
    public:
        int barWidth; // fraction of width of side bars and center full
        int centerWidth; // fraction of width center portion that darkens
        unsigned int pulseTime;
        OmLed16 co;

        unsigned int totalMs = 0;

        void init()override
        {
            this->barWidth = 24;
            this->centerWidth = 18;
            this->pulseTime = 1333; // ms
            this->co = OmLed16::hsv(ir(0,0xffff), ir(0x6000,0xf000), ir(0x6000, 0xf000));
        }
        void tick(unsigned int ms, OmLed16Strip *strip, float xLow)override
        {
            strip->fillRange(xLow+0, xLow+this->barWidth, this->co);
            strip->fillRange(xLow+this->width - this->barWidth, xLow+this->width, this->co);
            strip->fillRange(xLow+int(this->width - this->barWidth)/2,xLow+int(this->width + this->barWidth)/2, this->co);

            this->totalMs += ms;
            bool darken = (this->totalMs / this->pulseTime) % 2;
            if(darken)
            {
                strip->fillRange(xLow+int(this->width - this->centerWidth)/2,xLow+int(this->width + this->centerWidth)/2, this->co * 0.1, true);
            }

        }
        void setPeriod(unsigned int ms, bool resync)override{}
        void setSteps(unsigned int steps)override{}
        void setColor(OmLed16 co, float alpha)override{}
    };

    OmLed16 glyphWCo = OmLed16(0x5000, 0x6000, 0x5000); // •
    OmLed16 glyphXCo = OmLed16(0x4000, 0x0000, 0x0000); // -
    OmLed16 glyphYCo = OmLed16(0x0000, 0x2000, 0x8000); // ''
    OmLed16 glyphZCo = OmLed16(0xc000, 0x7000, 0x1000); // '''

    OmLed16 WCo = OmLed16(0xe000, 0xe000, 0xe000); // '''

    int lastPhase4 = -1;
    int lastPhase2 = -1;

    Indicator1 ind1A;
    Indicator2 ind2A;

    void innerInit() override
    {
        this->name = "Signals2";

        ind1A.signalInit(100);
        ind1A.a = 7;
        ind1A.b = 10;
        ind1A.signalSetColor(OmLed16::hsv(ir(0,0xffff), ir(0xc000,0xe000), ir(0x5000,0xa000)), 1.0);

        ind2A.signalInit(100);
    }

    void digitGlyph(OmLed16Strip *strip, int digit, int ledX)
    {
#define GLYPH_BITS(digit,a,b,c,d,e) \
    case digit: \
        strip->setLed(ledX++, this->glyph##a##Co); \
        strip->setLed(ledX++, this->glyph##b##Co); \
        strip->setLed(ledX++, this->glyph##c##Co); \
        strip->setLed(ledX++, this->glyph##d##Co); \
        strip->setLed(ledX++, this->glyph##e##Co); \
    break;

        switch(digit)
        {
                GLYPH_BITS(0, W,X,X,X,W)
                GLYPH_BITS(1, Y,Y,Y,Y,Y)
                GLYPH_BITS(2, W,X,W,Z,W)
                GLYPH_BITS(3, W,Y,W,Y,W)
                GLYPH_BITS(4, W,Y,W,X,W)
                GLYPH_BITS(5, W,X,W,Z,W)
                GLYPH_BITS(6, X,W,X,W,X)
                GLYPH_BITS(7, W,X,X,X,X)
                GLYPH_BITS(8, W,Z,W,Z,W)
                GLYPH_BITS(9, X,W,W,W,X)
        }
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {

        int hour = 0, minute = 30;
        float secondF = 30;
        int second;
        int mwd;
        float swm;
        OmNtp::sGetTimeOfDay(mwd, swm);
        if(mwd >= 0)
        {
            hour = mwd / 60;
            minute = mwd % 60;
            secondF = swm;
        }
        second = secondF;

        int phHhmm = int(secondF / 4) % 4; // phase for hhmm cycle
        int phSs = int(secondF * 1) % 2; // phase for ss digits cycle

        // pixels 0-4 go HHMM loop
        int digit;
        switch(phHhmm)
        {
            case 0: digit = hour/10; break;
            case 1: digit = hour%10; break;
            case 2: digit = minute/10; break;
            default: digit = minute%10; break;
        }
        this->digitGlyph(strip, digit, 0);
        // flash the change pip, 1 frame
        if(phHhmm != this->lastPhase4)
        {
            this->lastPhase4 = phHhmm;
            if(phHhmm == 0)
                strip->setLed(5, this->WCo);
        }

        // pixels 6-10 go SS loop
        switch(phSs)
        {
            case 0: digit = second/10; break;
            case 1: digit = second%10; break;
        }
        this->digitGlyph(strip, digit, 6);
        // flash the change pip, 1 frame
        if(phSs != this->lastPhase2)
        {
            this->lastPhase2 = phSs;
            if(phSs == 0)
                strip->setLed(11, this->WCo);
        }

//        this->ind1A.signalTick(ms, strip, 20);
        this->ind2A.signalTick(ms, strip, 20);
    }
};



class Blops : public OmLed16Pattern
{
public:
    
    struct Blop
    {
        float x0;
        float x1;
        OmLed16 co;
        // decay stage?
    };
    
    static const int maxBlops = 6;
    int k = 3;
    Blop blops[maxBlops];
    int beat1Msec;
    int beat0Msec;
    
    int beat;
    int beatCountdownMs;
    
    void randomize()
    {
        float margin = this->ledCount / 20;
        this->k = ir(3, maxBlops);
        float x = 0;
        for(int ix = 0; ix < this->k; ix++)
        {
            Blop b;
            x += rr(margin);
            b.x0 = x;
            float remaining = this->ledCount - x;
            b.x1 = rr(x, x + remaining * 0.8);
            b.co = OmLed16::hsv(ir(65536), ir(50000,65535), ir(20000,5536));
            this->blops[ix] = b;
            x = b.x1;
        }
        
        // shuffle
        for(int ix = 0; ix <= this->k / 2; ix++)
        {
            int a = ir(0, this->k);
            int b = ir(0, this->k);
            swapo(this->blops[a], this->blops[b]);
        }
        
        this->beat1Msec = ir(200, 800);
        this->beat0Msec = ir(200, 800);
        if(ir(0,3) == 0)
            this->beat0Msec = this->beat1Msec;
        
        this->beat = 0; // which blop is lit
        this->beatCountdownMs = this->beat1Msec + this->beat0Msec; // timing til next.
    }

    void innerInit() override
    {
        this->name = "Blops";
        this->addIntParam("brightness", 80);
        this->addAction("randomize");
        
        this->randomize();
    }
    
    void innerDoAction(unsigned int ix, bool buttonDown) override
    {
        switch (ix)
        {
            case 1: // randomize
                if (buttonDown)
                    this->randomize();
                break;
        }
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        OmLed16 co0 = this->getParamValueColor(0);

        for(int ix = 0; ix < this->k; ix++)
        {
            Blop &b = this->blops[ix];
            strip->fillRange(b.x0, b.x1, b.co * 0.05);
        }
        
        this->beatCountdownMs -= ms;
        if(this->beatCountdownMs < 0)
        {
            this->beat = (this->beat + 1) % this->k;
            this->beatCountdownMs += this->beat1Msec + this->beat0Msec;
        }
        // is a blop on.
        if(this->beatCountdownMs > this->beat0Msec)
        {
            // yes a blop is on.
            Blop &b = this->blops[this->beat];
            strip->fillRange(b.x0, b.x1, b.co);
        }
    }
};

class ThreeBars : public OmLed16Pattern
{
  public:
    void innerInit()
    {
      this->addColorParam("color A", OmLed16(0x1000, 0, 0));
      this->addColorParam("color B", OmLed16(0, 0x1000, 0));
      this->addColorParam("color C", OmLed16(0x1000, 0, 0x2000));

      if (!this->name)
        this->name = "Three Bars";
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip)
    {
      for (int ix = 0; ix < 3; ix++)
      {
        float t = sin(totalMs / (2000.0 + (ix - 1) * 200)) * 8;
        float pL = (ix * 2.0) / 5.0 * this->ledCount + t;
        float pR = (ix * 2.0 + 1.0) / 5.0 * this->ledCount + t;
        OmLed16 aColor = this->getParamValueColor(ix);
        strip->fillRange(pL, pR, aColor);
      }
    }
};

/// esp32 advised, may be heavy?
class Wind1 : public OmLed16Pattern
{
public:

    class Dot
    {
    public:
        enum EDotStage
        {
            EDS_none,
            EDS_start,
            EDS_middle,
            EDS_end,
            EDS_done,
        };

        float x; // position
        float v; // velocity in pixels/second
        float m; // mass, draws bigger and moves slower

        float windMod; // a number like 0.9 or 1.1 that tells this one it can move a little faster or a little slower

        OmLed16 coF; // color font
        OmLed16 coB; // color back


        EDotStage stage = EDS_none;
#define birthDuration 3 // how many seconds to materialize or disappear
        float stageAge = 0; // counts from 0 at start or end
        float lifeTime = 0;


        Dot(float x)
        {
            this->x = x;
            this->v = rrS(2,5);
            this->m = rr(3,8);
            this->windMod = rr(0.85, 1.15);
            this->coF = OmLed16::hsv(ir(0,0xffff), ir(0xc000,0xffff), ir(0x8000, 0xffff));
            this->coB = OmLed16::hsv(ir(0,0xffff), ir(0xc000,0xffff), ir(0x8000, 0xffff));

            this->stage = EDS_start;
            this->stageAge = 0;
            this->lifeTime = rr(5,10);
        }

        /// max is the led strip size, t integration time in seconds (time since last), w wind in pixels/second
        void blow(OmLed16Strip *strip, float t, float w, float brightness)
        {
            w = w * this->windMod;

            // our velocity should become nearer to the wind velocity,
            // and take longer to reach it if this->m mass is higher
            const float deltaV = 100; // pixelsPerSecond per second amount we can change
            this->v = migrateF(this->v, w, deltaV * t / this->m);



//            // reduce v a lot if we're near the ends
//            float limit = 8.5;
//            if(this->v < 0 && this->x < limit)
//                this->v /= 2.0;
//            else if(this->x > strip->ledCount - limit)
//                this->v /= 2.0;

            this->x += t * this->v;

            float wi = this->m; // drawn by size of mass...
            float v = fabs(this->v);
            float vFlip = 10;
            if(v < vFlip)
                wi = wi * v / vFlip;
            wi = wi + 1;
            OmLed16 co = this->coF;
            if(this->v < 0)
                co = this->coB;

            float coDimmer = 1.0;
            switch(this->stage)
            {
                case EDS_start:
                {
                    this->stageAge += t;
                    if(this->stageAge <= birthDuration)
                        coDimmer = this->stageAge / birthDuration;
                    else
                        this->stage = EDS_middle;
                    break;
                }

                case EDS_middle:
                {
                    this->lifeTime -= t;
                    if(this->lifeTime <= 0)
                    {
                        this->stageAge = birthDuration;
                        this->stage = EDS_end;
                    }
                    break;
                }

                case EDS_end:
                {
                    this->stageAge -= t;
                    if(this->stageAge > 0)
                        coDimmer = this->stageAge / birthDuration;
                    else
                    {
                        coDimmer = 0;
                        this->stage = EDS_done;
                    }
                    break;
                }

                case EDS_done:
                {
                    coDimmer = 0;
                    break;
                }
            }

            co = co * coDimmer;
            co = co * brightness;
            strip->fillRange(this->x - wi/2, this->x + wi/2, co, true);
        }
    };

    std::vector<Dot> dots;
    float wind; // randomized, guided by wind param slider


#define WIND1_PARAMS \
int px = 0; \
INT_PARAM(brightness, 60) \
INT_PARAM(k, 4) \
INT_PARAM(wind, 50) \
COLOR_PARAM(co, 0xff0000)


#define INT_PARAM(_name, _defaultValue) this->addIntParam(#_name, _defaultValue);
#define COLOR_PARAM(_name, _defaultValue) this->addColorParam(#_name, _defaultValue);

    void innerInit() override
    {
        WIND1_PARAMS

        this->name = "Wind1";

        this->wind = 100;
    }

#undef INT_PARAM
#undef COLOR_PARAM
#define INT_PARAM(_name, _defaultValue) int _name = this->getParamValueInt(px++);
#define COLOR_PARAM(_name, _defaultValue) OmLed16 _name = this->getParamValueColor(px++);

    void innerTick(uint32_t deltaMs, OmLed16Strip *strip) override
    {
        WIND1_PARAMS

        float brightnessF = brightness * brightness / 10000.0;

        if(k > this->dots.size() && ir(0,20) == 5)
        {
            Dot dot(rr(strip->ledCount * 0.2, strip->ledCount * 0.8));
            dot.coB *= co;
            dot.coF *= co; // new dots tinted by color param
            this->dots.push_back(dot);
        }

        // remove "done" ones
        for(int ix = (int)this->dots.size() - 1; ix >= 0 ; ix--)
        {
            Dot &dot = this->dots[ix];
            if(dot.stage == Dot::EDS_done)
                remove(this->dots, ix);
        }

        if(k > this->dots.size() && ir(0,5) == 2)
        {
            // too many... fine 1 to end-of-life
            for(Dot &dot : this->dots)
            {
                if(dot.stage == Dot::EDS_middle)
                {
                    dot.lifeTime = 0.1;
                    break;
                }
            }
        }

        while(k < this->dots.size())
        {
            remove(this->dots, 0);
        }

        float deltaS = deltaMs / 1000.0;
        int dotsLeft = 0;
        int dotsRight = 0;
        int edgeRange = strip->ledCount / 3;
        for(Dot &dot : this->dots)
        {
            dot.blow(strip, deltaS, this->wind, brightnessF);

            if(dot.x < edgeRange)
                dotsLeft++;
            else if(dot.x > strip->ledCount - edgeRange)
                dotsRight++;
        }

        // time to blow the other way?
        if(this->wind > 0 && dotsRight > k / 2) // more than half on the right?
            this->wind = rr(-wind, -wind/2);
        else if(this->wind < 0 && dotsLeft > k / 2) // more than half on the right?
            this->wind = rr(wind, wind/2);



        return;
    }

};

// much reduced version of OmVec from omino shared libs...
class OmVec2
{
public:
    union
    {
        float v[2];
        struct
        {
            float x;
            float y;
        };
    };

    OmVec2()
    {
        return;
    }

    OmVec2(float x, float y)
    {
        this->x = x;
        this->y = y;
    }

    OmVec2 operator *(float x) const
    {
        OmVec2 result(this->v[0] * x,this->v[1] * x);
        return result;
    }

    OmVec2 operator +=(const OmVec2 &other)
    {
        this->v[0] += other.v[0];
        this->v[1] += other.v[1];
        return *this;
    }

    float length() const
    {
        return sqrtf(this->v[0] * this->v[0] + this->v[1] * this->v[1]);
    }

    OmVec2 operator /(float x) const
    {
        OmVec2 result(this->v[0] / x,this->v[1] / x);
        return result;
    }

    OmVec2 normalize() const
    {
        return *this / this->length();
    }

};

class Orbit1 : public OmLed16Pattern
{
public:

    class Dot
    {
    public:
        enum EDotStage
        {
            EDS_none,
            EDS_start,
            EDS_middle,
            EDS_end,
            EDS_done,
        };

        OmVec2 p; // position
        OmVec2 v; // velocity in pixels/second

        OmLed16 co;


        EDotStage stage = EDS_none;
#define birthDuration 3 // how many seconds to materialize or disappear
        float stageAge = 0; // counts from 0 at start or end
        float lifeTime = 0;


        Dot(float maxVel)
        {
            this->p = OmVec2(rr(-50, 50), rr(-50,50));
            this->v = OmVec2(this->p.y, -this->p.x).normalize() * rr(-maxVel, maxVel);
            this->co = OmLed16::hsv(ir(0,0xffff), ir(0xe800,0xffff), ir(0xe000, 0xffff));

            this->stage = EDS_start;
            this->stageAge = 0;
            this->lifeTime = rr(120,600);
        }

        void tick(OmLed16Strip *strip, float t, float brightness, float gravity, float shadowDepth)
        {
            float coDimmer = 1.0;

            {
                // deltaV towards the origin
                OmVec2 towards0 = this->p.normalize() * -1;
                float distance0 = this->p.length();
                if(distance0 < 0.5) distance0 = 0.5;
                towards0 = towards0 * gravity * 200.0 / (distance0 * distance0);
                this->v += towards0 * t;
            }
            this->p += this->v * t;

            this->stageAge += t;
            switch(this->stage)
            {
                case EDS_start:
                {
                    if(this->stageAge <= birthDuration)
                        coDimmer = this->stageAge / birthDuration;
                    else
                    {
                        this->stageAge = 0;
                        this->stage = EDS_middle;
                    }
                    break;
                }

                case EDS_middle:
                {
                    if(this->stageAge >= this->lifeTime)
                    {
                        this->stageAge = 0;
                        this->stage = EDS_end;
                    }
                    break;
                }

                case EDS_end:
                {
                    if(this->stageAge <= birthDuration)
                        coDimmer = 1.0 - this->stageAge / birthDuration;
                    else
                    {
                        coDimmer = 0;
                        this->stage = EDS_done;
                    }
                    break;
                }

                case EDS_done:
                {
                    coDimmer = 0;
                    break;
                }

                default:break;
            }

            OmLed16 co = this->co * coDimmer;
            co = co * brightness;
            // change width & brightness by the Y position.
            // negative Y values are in front of the sun and closer to the viewer.
            float wi = mapRange(this->p.y, 30, -30, 5, 8);
            brightness *= mapRange(this->p.y, 30, -30, 1.0, 1.2);

            // fraction lit, depends on quadrant
            float fractionLit = 0;
            {
                float xA = abs(this->p.x);
                float yA = abs(this->p.y);
                float xyA = xA + yA;

                fractionLit = xA / xyA * 0.5;
                if(this->p.y > 0)
                    fractionLit = 1.0 - fractionLit;
            }

            float x = this->p.x + strip->ledCount / 2.0;
            fractionLit *= wi;
            x -= wi / 2.0;
            OmLed16 coDark = co * shadowDepth;
//            coDark = OmLed16(0,0,0xffff);
            if(this->p.x < 0)
            {
                // dark side on left
                strip->fillRange(x, x + wi - fractionLit, coDark, true);
                strip->fillRange(x + wi - fractionLit, x + wi, co, true);
            }
            else
            {
                // dark side on right
                strip->fillRange(x, x + fractionLit, co, true);
                strip->fillRange(x + fractionLit, x + wi, coDark, true);
            }
        }
    };

    std::vector<Dot> dots;


#define ORBIT1_PARAMS \
int px = 0; \
INT_PARAM(brightness, 60) \
INT_PARAM(k, 4) \
INT_PARAM(maxVel, 50) \
INT_PARAM(gravity, 50) \
COLOR_PARAM(sunColor, 0x808080) \
INT_PARAM(sunWidth, 30) \
INT_PARAM(shadowDepth, 50) \
COLOR_PARAM(co, 0xff0000)


#undef INT_PARAM
#undef COLOR_PARAM
#define INT_PARAM(_name, _defaultValue) this->addIntParam(#_name, _defaultValue);
#define COLOR_PARAM(_name, _defaultValue) this->addColorParam(#_name, _defaultValue);

    void innerInit() override
    {
        ORBIT1_PARAMS

        this->name = "Orbit1";
    }

#undef INT_PARAM
#undef COLOR_PARAM
#define INT_PARAM(_name, _defaultValue) int _name = this->getParamValueInt(px++);
#define COLOR_PARAM(_name, _defaultValue) OmLed16 _name = this->getParamValueColor(px++);

    void innerTick(uint32_t deltaMs, OmLed16Strip *strip) override
    {
        ORBIT1_PARAMS

        float brightnessF = brightness * brightness / 10000.0;

        if(k > this->dots.size() && ir(0,20) == 5)
        {
            Dot dot(maxVel);
            dot.co *= co;
            this->dots.push_back(dot);
        }

        // remove "done" ones
        for(int ix = (int)this->dots.size() - 1; ix >= 0 ; ix--)
        {
            Dot &dot = this->dots[ix];
            if(dot.stage == Dot::EDS_done)
                remove(this->dots, ix);
        }

        if(k < this->dots.size() && ir(0,5) == 2)
        {
            // too many... fine 1 to end-of-life
            for(Dot &dot : this->dots)
            {
                if(dot.stage == Dot::EDS_middle)
                {
                    dot.lifeTime = 0.1;
                    break;
                }
            }
        }

        while(k < this->dots.size())
        {
            remove(this->dots, 0);
        }

        float deltaS = deltaMs / 1000.0;
//        int dotsLeft = 0;
//        int dotsRight = 0;
//        int edgeRange = strip->ledCount / 3;

        float sd = shadowDepth * shadowDepth / 100000.0;
        for(Dot &dot : this->dots)
            if(dot.p.y > 0)
                dot.tick(strip, deltaS, brightnessF, gravity, sd);

        {
            // draw the sun
            float sunX = strip->ledCount / 2.0;
            float x0 = sunWidth * 0.5;
            float x1 = sunWidth * 0.3;
            strip->fillRange(sunX - x0, sunX - x1, OmLed16(0,0,0), sunColor);
            strip->fillRange(sunX + x0, sunX + x1, OmLed16(0,0,0), sunColor);
            strip->fillRange(sunX - x1, sunX + x1, sunColor, true); // the middle is opaque
        }

        for(Dot &dot : this->dots)
            if(dot.p.y <= 0)
                dot.tick(strip, deltaS, brightnessF, gravity, sd);

        return;
    }

};

class GradientTester1 : public OmLed16Pattern
{
public:

    float movingX;
    uint32_t k = 0;
    uint32_t kMs = 0; // cumulative milliseconds needed before we flash
    uint32_t ms = 0;

    void innerInit() override
    {
        this->addColorParam("colorA", 0x0000ff);
        this->addColorParam("colorB", 0xFF0000);
        this->addIntParam("x", 50);
        this->addIntParam("width", 10);
        this->addIntParam("movement", 0);
        this->addIntParam("ring", 0);
        this->name = "GradientTester1";
    }

    void innerTick(uint32_t deltaMs, OmLed16Strip *strip) override
    {
        OmLed16 coA = this->getParamValueColor(0);
        OmLed16 coB = this->getParamValueColor(1);
        float x = this->getParamValueInt(2) * this->ledCount / 100.0;
        float width = this->getParamValueInt(3);
        float movement = this->getParamValueInt(4) / 10.0;
        bool ring = this->getParamValueInt(5) > 50;

        if(movement > 0)
        {
            this->movingX += deltaMs * movement / 1000.0;
            if(this->movingX >= this->ledCount)
                this->movingX -= this->ledCount;
        }
        else
            this->movingX = x;

        if(ring)
            strip->fillRangeRing(this->movingX - width / 2, this->movingX + width / 2, coA, coB);
        else
            strip->fillRange(this->movingX - width / 2, this->movingX + width / 2, coA, coB);

    }

};

class RingGate : public OmLed16Pattern
{
  public:

    class Blap
    {
      public:
        float x; // where it starts
        float length; // pixels total

        float xDelta; // how its moving
        float lengthDelta;

        float b;

        int age;
        int maxAge;

        int centerBurst;
        int centerBurstMax;
        int centerBurstCount;
        float centerBurstSpread;

        Blap(int pixCount)
        {
          this->x = rr(0, pixCount);
          this->length = rr(5, pixCount / 2);
          this->b = rr(0.07, 0.5);

          this->xDelta = rrS(0.005, 0.1);
          this->lengthDelta = rr(-0.1, 0.1);

          this->maxAge = ir(100, 800); // frames
          this->age = 0;

          this->centerBurst = 0; // stage of center burst sparkles
          this->centerBurstMax = ir(30, 100);
          this->centerBurstCount = ir(2, 8);
          this->centerBurstSpread = rr(0.1, 1.0);
        }

        bool drawAndStep(OmLed16Strip *strip, float brightness, OmLed16 co)
        {
          float timeFold = age * 2.0 / maxAge;
          if (timeFold > 1.0)
            timeFold = 2.0 - timeFold;
          brightness *= timeFold;
          //          brightness *= this->b;

          float x = this->x - this->length / 2.0;
          drawCenteredRing(strip, this->x, this->length, co * brightness);

          this->x += this->xDelta;
          this->length += this->lengthDelta;

          {
            // and the centerBurst if active
            if (this->centerBurst < this->centerBurstMax)
            {
              OmLed16 cbCo = co * (1.0 - (float)this->centerBurst / (float)this->centerBurstMax);
              float spread = this->centerBurstSpread * this->centerBurst; // how far the sparkles have expanded
              for (int ix = 0; ix <= this->centerBurstCount; ix++)
              {
                float x = mapRange(ix, 0, this->centerBurstCount, -spread, +spread) + this->x;
                drawCenteredRing(strip, x, 2, cbCo);
              }
              this->centerBurst++;
            }
          }
          this->age++;
          bool result = this->age < this->maxAge;
          return result;
        }
    };

#define BLAPCOUNT 8

    std::vector<Blap> blaps;

    void innerInit() override
    {
      this->blaps.clear();
        this->addIntParam("brightness", 50);
        this->name = "RingGate";
    }

    void innerTick(uint32_t milliseconds, OmLed16Strip *strip) override
    {
      if (this->blaps.size() < BLAPCOUNT && ir(1, 60) == 3)
      {
        Blap w(this->ledCount);
        this->blaps.push_back(w);
      }

      OmLed16 co = OmLed16(7*257, 0, 20*257);

      for (auto &blap : this->blaps)
      {
        blap.drawAndStep(strip, .5, co);
      }
      for (int ix = 0; ix < (int)this->blaps.size(); ix++)
      {
        auto &blap = this->blaps[ix];
        if (blap.age >= blap.maxAge)
        {
          remove(this->blaps, ix);
          ix--;
        }
      }

      if (ir(1, 100) == 45)
      {
        // do a reburst on them all
        int th = ir(0, 3);
        for (auto &blap : this->blaps)
        {
          if (ir(1, 5) > th)
            blap.centerBurst = 0;
        }
      }

      if (ir(1, 100) == 33)
      {
        int sparkleCount = pow(rr(0, 1), 2.5) * 10 + 1;
        for (int ix = 0; ix < sparkleCount; ix++)
        {
          int x = ir(0, this->ledCount);
          strip->setLed(x, co * ir(0.1, 0.6));
        }
      }

      // lstly, ensure a low-level blue all around the Ring
      for (int ix = 0; ix < this->ledCount; ix++)
      {
        OmLed16 &co = strip->leds[ix];
        if (co.b < 64)
          co.b = 64;
      }
    }

};

class LanternSpots1 : public OmLed16Pattern
{
public:

    double ms = 0;


    void innerInit() override
    {
        this->addIntParam("brightness", 44);
        this->addIntParam("speed", 44);
        this->addColorParam("color", OmLed16(0x8000, 0, 0));
        this->name = "LanternSpots1";
    }

    void innerTick(uint32_t deltaMs, OmLed16Strip *strip) override
    {
        OmLed16 coR(65535,0,0);
        OmLedTGrid<OmLed16, true> grid(strip, 21);

        uint32_t bri = this->getParamValueInt(0);
        uint32_t spd = this->getParamValueInt(1);
        OmLed16 co = this->getParamValueColor(2);

        float msScale = mapRange(spd, 0, 100, -3.5, 3.5);


        this->ms += deltaMs * msScale;
        float x = this->ms / 456.0;
        grid.fill(co, x, 3.5, x + 2.5, 6.3);
    }

};



static void addOmLedPatterns2(OmLed16PatternManager &pm)
{
#define _AP_1_(_pa) pm.addPattern(new _pa())
    _AP_1_(Signals1);
    _AP_1_(Signals2);
    _AP_1_(Blops);
    _AP_1_(ThreeBars);
#undef _AP_1_
}


#endif /* OmLedPatterns2_h */

