//
//  Patterns.h
//  PatternsTester
//
//  Created by David Van Brink on 3/23/21.
//  Copyright © 2021 David Van Brink. All rights reserved.
//

#ifndef Patterns_h
#define Patterns_h
#include <string>

class Bouncings : public OmLed16Pattern
{
public:
#define BALLS 30
    class Ball
    {
    public:
        float x;
        float v;
        OmLed16 co;
        int age;
    };
    std::vector<Ball> balls;

    void innerInit() override
    {
        this->addIntParam("brightness", 80);
        this->addIntParam("odds", 50);
        this->addIntParam("balls", 80);

        if (!this->name)
            this->name = "Bouncings";
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        int odds = this->getParamValueInt(1);
        float bri = this->getParamValueInt(0) / 100.0;
        int ballK = this->getParamValueInt(2);

        if (this->balls.size() < ballK)
        {
            if (ir(0, 1000) < odds)
            {
                Ball b;
                b.x = 0;
                b.v = rr(0.1, 3);
                b.co = OmLed16::hsv(ir(0, 65536), ir(50000, 65536), 32000);
                b.age = ir(200, 600);
                this->balls.push_back(b);
            }
        }

        for (int ix = 0; ix < this->balls.size(); ix++)
        {
            Ball &b = this->balls[ix];
            float oldX = b.x;
            b.x += b.v;
            b.v -= 0.02; // gravity
            if (b.x < 0 && b.v < 0)
            {
                b.x = -b.x;
                b.v = -b.v * 0.9; // elasticity
            }
            float newX = b.x;

            // exaggerate the oldX now...
            float xDel = oldX - newX;
            oldX = newX + xDel * 4;

            if (oldX > newX)
                swapo(oldX, newX);
            
            const int fadeout = 20;
            float bbri = bri; // ball brightness
            if(b.age < fadeout)
                bbri *= (float)b.age / fadeout;

            strip->fillRange(oldX, newX + 1, b.co * bbri);

            b.age--;
            if (b.age <= 0)
            {
                remove(this->balls, ix);
                ix --;
            }
        }
    }
};

#ifdef PI
#undef PI
#endif
#define PI 3.1415926535897932384626433832795028841971

#define TAU (PI + PI)
class RainbowWorm : public OmLed16Pattern
{
public:
    float kX;
    float wiggleTheta;

    void innerInit() override
    {
        this->name = "RainbowWorm";
        this->kX = 0;
        this->addIntParam("brightness", 30);
        this->addIntParam("speed", 40);
        this->addIntParam("wiggle", 50);
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        OmLed16 co16;

        // animation...
        //    float rate = 41; // pixels/second
        float brightness = this->getParamValueInt(0) / 100.0;
        float rate = this->getParamValueInt(1);
        float wigglesPerSecond = getParamValueInt(2) / 40.0;

        brightness = pow(brightness, 3.5);

        this->kX += rate * ms / 1000.0;
        if (this->kX >= this->ledCount)
            this->kX -= this->ledCount;

        int w = 37;
        this->wiggleTheta += ms / 1000.0 * TAU * wigglesPerSecond;
        if (this->wiggleTheta > TAU)
            this->wiggleTheta -= TAU;
        w += sin(this->wiggleTheta) * 6;


        for (int ix = 0; ix < w; ix++)
        {
            co16.setHsv(ix * 0xffff / w, 0xffff, brightness * 0xffff);
            int x = kX + ix;
            strip->leds[x % this->ledCount] = co16;
        }
    }
};

class ChasingWorms1 : public OmLed16Pattern
{
    class Worm
    {
    public:
        int hue;
        float x; // position, current
        float width;
        float dir; // +/- 1
        int age;
        int lifeTime;
        std::string toString()
        {
            char k[200];
            sprintf(k, "(h%d, x%f, d%f a%d L%d)", this->hue, this->x, this->dir, this->age, this->lifeTime);
            std::string s = k;
            return s;
        }
    };

    std::vector<Worm> worms;
public:
    // set this to a lower number to make the works move slower and live longer.
    float masterSpeedMultiplier = 1.0;

    void innerInit() override
    {
        worms.clear();
        this->addColorParam("color", OmLed16(0xE000, 0xE000, 0xE000));
        this->addIntParam("wormCount", 24);
        this->addIntParam("chance", 10);
        this->addIntParam("speed", 50);

        if (!this->name)
            this->name = "Worms";
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
#define _ptip 65536
#define _ptip2 (_ptip + _ptip - 1)

        unsigned int mostWorms = getParamValueInt(1);
        int chance = getParamValueInt(2);
        float speed = getParamValueInt(3) / 100.0 * this->masterSpeedMultiplier;
        if (ir(0, 100) < chance && worms.size() < mostWorms)
        {
            Worm spark;
            spark.hue = ir(_ptip);
            spark.dir = (ir(40, 5000) / 1000.0) * (ir(2) ? +1 : -1);
            spark.width = ir(5, 200) / 10.0;
            spark.x = ir(0, this->ledCount);
            spark.age = 0;
            spark.lifeTime = ir(20, 280) / this->masterSpeedMultiplier;
            worms.push_back(spark);
            //    printf("adding %s\n", spark.toString().c_str());
        }

        for (unsigned int ix = 0; ix < worms.size(); ix++)
        {
            Worm &spark = worms[ix];
            int brightness = _ptip2 * spark.age / spark.lifeTime;
            if (brightness >= _ptip)
                brightness = _ptip2 - brightness;

            OmLed16 co = OmLed16::hsv(spark.hue, _ptip - 1, brightness);

            co = co * this->getParamValueColor(0);


            strip->fillRangeRing(spark.x, spark.x + spark.width, co);
            spark.x += spark.dir * speed;

            spark.age++;
            if (spark.age > spark.lifeTime)
            {
                remove(worms, ix);
                ix--;
            }
        }
    }
};


/// Something with a lot of light, but also some interesting shading.
class PunchyRoomLight : public OmLed16Pattern
{
public:

    float oo = 0;
    float rate = 0.051278;
    void innerInit() override
    {
        this->addIntParam("brightness", 30);
        this->addIntParam("speed", 24);
        this->addColorParam("color", 0xffffff);
        this->name = "Punchy Room Light";
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        float m = 1.0;
        float v = getParamValueInt(1) / 100.0; // 0.0 to 1.0
        v = v * v * v; // a nice curve
        m = mapRange(v, 0.0, 1.0, 0.03, 15); // from very slow indeed to quite quick!

        float brightness = getParamValueInt(0) / 100.0;
        OmLed16 co = this->getParamValueColor(2);
        brightness = pow(brightness, 2.5);
        brightness *= 255.0;

        this->oo += this->rate * m;
        if (this->oo > 1000000) // crazy reset in the far framed future
            this->oo = 0;

#define _PRL(_sp, _mu, _le, _co) \
for(int ix = 0; ix < this->ledCount; ix += _sp) \
{ \
float left = this->oo * _mu + ix; \
strip->fillRangeRing(left, left + _le, _co); \
}

        _PRL(25, 1.0, 10.3, OmLed16(250, 0, 0) * brightness * co);
        _PRL(30, -1.7, 17.3, OmLed16(0, 0, 250) * brightness * co);
        _PRL(20, 2.112, 9.1, OmLed16(0, 250, 0) * brightness * co);

        _PRL(27, -4.2, 2, OmLed16(100, 0, 100) * brightness * co);

        _PRL(60, -0.712, 5.8, OmLed16(200, 200, 200) * brightness * co);
    }
};

class RisingPairs1 : public OmLed16Pattern
{
public:

    //    float oo = 0;
    //    float rate = 0.051278;

    float pipWidth = 5;
    float pipGap0 = 7.5;
    float pipGap1 = 18.5;

    float phase = 0;

    void innerInit() override
    {
        this->addIntParam("brightness", 10);
        this->addColorParam("bg", OmLed16(8000, 0, 0));
        this->addColorParam("fg", OmLed16(8000, 8000, 0));
        this->addIntParam("speed", 51);
        if (!this->name)
            this->name = "RisingPairs";
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        float brightness = this->getParamValueInt(0) / 100.0;
        brightness = brightness * brightness;
        float speed = (this->getParamValueInt(3) - 50) / 30.0;

        OmLed16 coB = this->getParamValueColor(1) * brightness;
        OmLed16 coF = this->getParamValueColor(2) * brightness;

        float tot = pipWidth + pipWidth + pipGap0 + pipGap1;

        int k = strip->ledCount;

        strip->fillRange(0, k, coB, 1);
        float x = this->phase - tot;
        while (x < k)
        {
            strip->fillRange(x, x + pipWidth, coF, 1);
            x += pipWidth + pipGap0;
            strip->fillRange(x, x + pipWidth, coF, 1);
            x += pipWidth + pipGap1;
        }

        phase += speed;
        if (speed > 0)
        {
            if (phase >= tot)
                phase -= tot;
        }
        else
        {
            if (phase < 0)
                phase += tot;
        }
    }
};

class TBSpark
{
public:
    int ledCount;
    float x;
    float w; // width
    OmLed16 color;
    OmLed16 tipColor;

    int waitMs;

    float wDest;
    float xDest;
    float xRate; // pixels per second

    typedef enum ESparkMode
    {
        ESM_NONE = 0,
        ESM_BORN,  // and materializing
        ESM_WAITING,
        ESM_MOVING,
        ESM_SPEWING,
        ESM_DYING
    } ESparkMode;

    ESparkMode st = ESM_NONE;

    TBSpark(int ledCount)
    {
        this->ledCount = ledCount;
        this->w = 0;
        this->wDest = rr(8, 16);
        this->x = rr(this->wDest/2, this->ledCount-this->wDest/2);
        this->xRate = rr(6,11);
        this->st = ESM_BORN;
        uint16_t h = ir(0,65536);
        this->color = OmLed16::hsv(h, 65535, 40000);
        this->tipColor = OmLed16::hsv(h + 22000, 65535, 40000);
        this->tipColor = OmLed16::hsv(h, 65535, 64000);
    }

    void wait()
    {
        // enter wait state
        this->st = ESM_WAITING;
        this->waitMs = ir(2000, 4000);
    }

    void waitDone()
    {
        if(ir(0,12) == 7)
        {
            this->st = ESM_DYING;
            this->xRate = rr(15,30);
        }
        else
        {
            this->st = ESM_MOVING;
            this->xDest = rr(this->wDest/2, this->ledCount-this->wDest/2);
            this->xRate = rr(10,50);
            if(ir(0,5) == 2)
                this->xRate *= rr(3,7);
        }
    }

    bool tick(unsigned int ms, OmLed16Strip *strip, float br)
    {
        bool result = true; // return false if we wish to die.
        float rate = this->xRate * ms / 1000.0;
        switch(this->st)
        {
            case ESM_NONE:
                break;
            case ESM_BORN:
                this->w = migrateF(this->w, this->wDest, rate);
                if(this->w == this->wDest)
                    this->wait();
                break;
            case ESM_WAITING:
                this->waitMs -= ms;
                if(this->waitMs < 0)
                    this->waitDone();
                break;
            case ESM_MOVING:
                this->x = migrateF(this->x, this->xDest, rate);
                if(this->x == this->xDest)
                    this->wait();
                break;
            case ESM_DYING:
                this->w = migrateF(this->w, 0, rate);
                if(this->w == 0)
                    result = false;
                break;
            default:
                break;
        }

        float left = this->x - this->w/2;
        float right = left + this->w;
        float tipL = this->wDest / 3;
        if(this->w > tipL * 2)
        {
            strip->fillRange(left, left+tipL, this->tipColor * br, 1);
            strip->fillRange(left+tipL, right-tipL, this->color * br, 1);
            strip->fillRange(right-tipL, right, this->tipColor * br, 1);
        }
        else
        {
            strip->fillRange(left, right, this->tipColor * br, 1);
        }
        return result;
    }
};

class ThinkingBlocks1 : public OmLed16Pattern
{
public:
    std::vector<TBSpark *> sparks;

    void innerInit() override
    {
        this->name = "Thinkin Blox";
        this->addIntParam("brightness", 50);
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        float br = this->getParamValueInt(0) / 100.0;
        if(this->sparks.size() < 7 && ir(0,98) == 23)
        {
            TBSpark *spark = new TBSpark(this->ledCount);
            this->sparks.push_back(spark);
        }

        for(int ix = 0; ix < this->sparks.size(); ix++)
        {
            TBSpark *spark = this->sparks[ix];
            bool alive = spark->tick(ms, strip, br);
            if(!alive)
            {
                remove(this->sparks, ix);
                ix--;
            }
        }
    }
};

class Solid : public OmLed16Pattern
{
public:
    void innerInit() override
    {
        this->addColorParam("color", 30);
        if (!this->name)
            this->name = "Solid";
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        OmLed16 co0 = this->getParamValueColor(0);

        for (int ix = 0; ix < strip->ledCount; ix++)
            strip->leds[ix] = co0;
    }
};

static bool nonBlack(OmLed16 &co)
{
    return (co.r | co.g | co.b) > 0;
}
class Dashes : public OmLed16Pattern
{
public:
    float x = 0;
    int coIx = 0;
    void innerInit()
    {
        this->addColorParam("color 1", 0xff0000);
        this->addColorParam("color 2", 0);
        this->addColorParam("color 3", 0);
        this->addIntParam("interval", 10);
        this->addIntParam("intervalCents", 0);
        this->addIntParam("width", 20);
        this->addIntParam("motion", 51);

        if (!this->name)
            this->name = "Dashes";
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip)
    {
        OmLed16 co[3];
        co[0] = this->getParamValueColor(0);
        co[1] = this->getParamValueColor(1);
        co[2] = this->getParamValueColor(2);
        int interval = this->getParamValueInt(3);
        int intervalCents = this->getParamValueInt(4);
        int width = this->getParamValueInt(5);
        int motion = this->getParamValueInt(6);

        int colorCount = 1;
        if(nonBlack(co[2]))
            colorCount = 3;
        else if(nonBlack(co[1]))
            colorCount = 2;

        float intervalF = interval + intervalCents / 100.0;
        if(intervalF < 1.0)
            intervalF = 1.0;
        float widthF = width * intervalF / 100.0;
        float motionF = mapRange(motion, 0, 100, -20, 20); // pixels/sec

        this->x += motionF * ms / 1000.0;
        while(this->x > 0)
        {
            this->x -= intervalF;
            this->coIx += colorCount - 1;
        }
        while(this->x < (-3 * intervalF))
        {
            this->x += intervalF;
            this->coIx++;
        }
        this->coIx %= colorCount;

        float x = this->x;
        int ix = this->coIx;


        while(x <= this->ledCount)
        {
            strip->fillRange(x, x + widthF, co[ix]);
            x += intervalF;
            ix = (ix + 1) % colorCount;
        }
    }
};

class Wheels : public OmLed16Pattern
{
public:

    class Wheel
    {
    public:
        OmLed16 color;
        int ptCount; // points on the wheel
        float ptRadians;
        float radiansPerSecond;
        float theta = 0;
        float x;
        float y;
        float radius;

        Wheel()
        {
            this->color = OmLed16::hsv(ir(0, 65536), ir(40000, 65535), 0xffff);
            this->ptCount = ir(3, 20);
            this->ptRadians = TAU / this->ptCount;
            this->radiansPerSecond = rrS(0.05, 2);
            this->x = rr(-2, 2);
            this->y = rr(2, 6);
            this->radius = rr(0.2, 1.6);

            //      this->color = OmLed16::hsv(ir(0,65536), ir(40000,65535), ir(200, 50000));
            //      this->ptCount = ir(1, 20);
            //      this->ptRadians = TAU / this->ptCount;
            //      this->radiansPerSecond = rrS(0.05, 2);
            //      this->x = 0;
            //      this->y = 3;
            //      this->radius = 1;



//            printf("wheel ptCount:%d x:%f y:%f radius:%f\n", this->ptCount, this->x, this->y, this->radius);
        }
    };

    std::vector<Wheel> wheels;

    void addWheel()
    {
        Wheel w;
        this->wheels.push_back(w);
    }

    void innerInit() override
    {
        if (!this->name)
            this->name = "Wheels";

        this->addAction("clear wheels");
        this->addAction("add wheel");
        this->addAction("momentary wheel");

        int wheelCount = 3;
        //    wheelCount = 1;
        for (int ix = 0; ix < wheelCount; ix++)
        {
            this->addWheel();
        }
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        float ed = 1.0; // eye distance
        float blipRadius = 6;
        for (Wheel &w : this->wheels)
        {
            float th = w.theta;
            for (int ix = 0; ix < w.ptCount; ix++)
            {
                float x = cos(th) * w.radius;
                float y = sin(th) * w.radius;
                x += w.x;
                y += w.y;
                float shrink = ed / (ed + y); // generic distance reducer
                float xPrime = x * shrink;
                float xPos = mapRangeNoPin(xPrime, -1, 1, 0, this->ledCount);
                float blipRadiusPrime = blipRadius * shrink;
                strip->fillRange(xPos - blipRadiusPrime, xPos + blipRadiusPrime, w.color * shrink);

                th += w.ptRadians;
            }
            w.theta += w.radiansPerSecond * ms / 1000.0;
            if (w.theta > TAU)
                w.theta -= TAU;
            else if (w.theta < 0)
                w.theta += TAU;
        }
    }

    void innerDoAction(unsigned int ix, bool buttonDown) override
    {
        switch (ix)
        {
            case 0: // clear
                if (buttonDown)
                    this->wheels.clear();
                break;

            case 1: // add wheel
                if (buttonDown)
                    this->addWheel();
                break;

            case 2: // temp wheel
            {
                if (buttonDown)
                    this->addWheel();
                else
                    this->wheels.pop_back();
                break;
            }
        }
    }
};

class Fire2 : public OmLed16Pattern
{
    class Flame
    {
    public:
        float phase;
        OmLed16 co;
        uint16_t h, s, v;
        float huePhase;
    };

    std::vector<Flame> flames;

    void innerInit() override
    {
        flames.clear();
        this->name = "Fire2";

        this->addColorParam("color", 0x404000);
        this->addIntParam("flames", 6);
        this->addIntParam("hueSpread", 50);
        this->addIntParam("speed", 50);
    }

    void innerChanged() override
    {
        OmLed16 co = this->getParamValueColor(0);
        int flames = this->getParamValueInt(1);

        int oldK = (int)this->flames.size();
        this->flames.resize(flames);
        for(int ix = oldK; ix < flames; ix++)
        {
            this->flames[ix].phase = ir(0, TAU);
            this->flames[ix].huePhase = ir(0, TAU);
        }

        for(auto &f : this->flames)
        {
            f.co = co;
            co.toHsv(f.h, f.s, f.v);
        }
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        int flames = this->getParamValueInt(1);
//        OmLed16 co = this->getParamValueColor(0);
        int hueSpread = this->getParamValueInt(2);
        int speed = this->getParamValueInt(3);

        hueSpread = mapRange(hueSpread, 0, 100, 0, 32768);
        float speedF = mapRange(speed, 0, 100, 0.01, 0.3);
        int ix = 0;
        for(auto &f : this->flames)
        {
            f.phase += rr(0, speedF);
            if(f.phase > TAU)
                f.phase -= TAU;
            f.huePhase += rr(0, speedF);
            if(f.huePhase > TAU)
                f.huePhase -= TAU;
            OmLed16 fCo;
            fCo.setHsv(mapRange(sin(f.huePhase), -1, +1, f.h - hueSpread, f.h + hueSpread), f.s, mapRange(sin(f.phase), -1, +1, 0, f.v));
            float x0 = (float)this->ledCount * ix / flames;
            float x1 = (float)this->ledCount * (ix + 1) / flames;
            strip->fillRange(x0, x1, fCo);
            ix++;
        }
    }
};

class Fire1 : public OmLed16Pattern
{
    class Flame
    {
    public:
        float x;
        float width;
        OmLed16 co;
        float age = 0;
        int lifeTime = 20;
        float breakPoint = 0.5; // expand vs contract over lifetime
    };

    std::vector<Flame> flames;

    void innerInit() override
    {
        flames.clear();
        this->name = "Fire1";

        this->addIntParam("hue", 0);
        this->addIntParam("flames", 20);
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        unsigned int maxFlames = getParamValueInt(1);
        if (this->flames.size() < maxFlames && ir(0, 20) < 6)
        {
            Flame f;
            f.x = rr(0, this->ledCount);
            f.width = rr(3, 40);
            int hue = getParamValueInt(0) * 65536 / 100;
            f.co = OmLed16::hsv(ir(hue, hue + 6000), ir(0xf400, 0xffff), ir(0x3000, 0x4800));
            f.age = 0;
            f.lifeTime = ir(5, 80);
            f.breakPoint = rr(0, 0.6);
            this->flames.push_back(f);
        }

        for (unsigned int ix = 0; ix < this->flames.size(); ix++)
        {
            Flame &f = this->flames[ix];
            f.age += ms / 30.0;
            if (f.age >= f.lifeTime)
            {
                remove(flames, ix);
                ix--;
                continue;
            }
            float t = (float)f.age / (float)f.lifeTime;
            float s;
            if (t < f.breakPoint)
                s = t / f.breakPoint;
            else
                s = 1.0 - (t - f.breakPoint) / (1.0 - f.breakPoint);

            float w = s * f.width;
            w *= rr(0.5, 1.5);
            strip->fillRange(f.x - w / 2.0, f.x + w / 2.0, f.co);
        }
    }
};

class Automata : public OmLed16Pattern
{
public:
    unsigned char *bits0 = NULL;
    unsigned char *bits1 = NULL;
    int bitsSize = 0;
//    OmLed16 co; // current color
//    OmLed16 coTarget; // color migrating too

    uint16_t coHsv[3]; // current hsv
    uint16_t coHsvTarget[3]; // migrating to this.

    bool migrating = false; // set when we have begin

    bool readBit(int bit)
    {
        if(bit < 0 || bit >= this->ledCount)
            return 0;
        unsigned char byte = this->bits0[bit / 8];
        bool result = (byte & (1 << (bit % 8))) != 0;
        return result;
    }

    void setBit(int bit, bool v)
    {
        if(bit < 0 || bit >= this->ledCount)
            return;
        int bix = bit / 8;
        unsigned char mask = 1 << (bit % 8);

        if(v)
            this->bits1[bix] |= mask;
        else
            this->bits1[bix] &= ~mask;
    }

    void flip()
    {
        for(int ix = 0; ix < this->bitsSize; ix++)
            this->bits0[ix] = this->bits1[ix];
    }

    void innerInit() override
    {
        this->name = "Automata";

        this->addAction("reset");
        this->addColorParam("color", OmLed16(0xE000, 0xE000, 0xE000));
        this->addIntParam("colorRange", 0);

        this->migrating = false;

        if(this->bits0)
            free(this->bits0);
        if(this->bits1)
            free(this->bits1);
        this->bitsSize = (this->ledCount + 7) / 8;
        this->bits0 = (unsigned char *)calloc(this->bitsSize, 1);
        this->bits1 = (unsigned char *)calloc(this->bitsSize, 1);

        for(int ix = 0; ix < 10; ix++)
            this->setBit(ir(0, this->ledCount), true);
    }

    std::vector<unsigned char>rules{30, 54, 60, 62, 90, 94,102, 110, 122, 126, 150, 158,182, 188, 190,255};
    unsigned char rule = 30;

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        OmLed16 colorParam = this->getParamValueColor(1);
        OmLed16 co = colorParam;

        if(this->migrating)
        {
#define KM 923
            this->coHsv[0] = migrateI(this->coHsv[0], this->coHsvTarget[0], KM);
            this->coHsv[1] = migrateI(this->coHsv[1], this->coHsvTarget[1], KM);
            this->coHsv[2] = migrateI(this->coHsv[2], this->coHsvTarget[2], KM);
            //            this->co.r = migrateI(this->co.r, this->coTarget.r, KM);
            //            this->co.g = migrateI(this->co.g, this->coTarget.g, KM);
            //            this->co.b = migrateI(this->co.b, this->coTarget.b, KM);
            co = OmLed16::hsv(this->coHsv[0], this->coHsv[1], this->coHsv[2]);
        }
        else
        {
            co.toHsv(this->coHsv[0], this->coHsv[1], this->coHsv[2]);
        }

        bool doNewColor = false;
        if(ir(0,333) == 27) // occasional reset
        {
            this->innerDoAction(0,true); // occasional reset
            doNewColor = true;
        }

        if(ir(0,78) == 27) // occasional new rule
        {
            this->rule = this->rules[ir(0, (int)this->rules.size())];
            doNewColor = true;
        }

        if(doNewColor)
        {
//            uint16_t h,s,v;
            colorParam.toHsv(this->coHsvTarget[0], this->coHsvTarget[1], this->coHsvTarget[2]);
            int colorRange = this->getParamValueInt(2); // how far to allow hue deviation
            int hueRange = 32768 * colorRange / 100;
            this->coHsvTarget[0] += + ir(-hueRange, hueRange);
            this->migrating = true;
        }

        int setCount = 0;
        for(int ix = 0; ix < this->ledCount; ix++)
        {
            int k = (this->readBit(ix - 1) ? 0 : 4)
            + (this->readBit(ix) ? 0 : 2)
            + (this->readBit(ix + 1) ? 0 : 1);
            bool v = (this->rule & (128 >> k)) != 0;
            this->setBit(ix, v);
            if(v)
            {
                strip->leds[ix] = co;
                setCount++;
            }
        }
        if(setCount == 0)
        {
            for(int ix = 0; ix < 10; ix++)
                this->setBit(ir(0, this->ledCount), true);
        }
        this->flip();
    }

    void innerDoAction(unsigned int ix, bool buttonDown) override
    {
        for(int ix = 0; ix < this->ledCount; ix++)
            this->setBit(ix, false);
        int k = ir(1,3);
        for(int ix = 0; ix < k; ix++)
            this->setBit(ir(0, this->ledCount), true);
        this->flip();
    }

};

extern const uint8_t font8x8[];
// TODO scan the font, determine actual width of each char so we can print narrower where possible.
// 0x20 choose a width for blank o course

uint8_t font8x8Spans[96]; // low 4 bits is the lowest bit used (rightmost), high 4 bits is the highest bit used (left column)

class Text : public OmLed16Pattern
{
public:
    const char *s = "TESTING 1 2 3 4  "
    "This is the voice of COLOSSUS. This is the voice of WORLD CONTROL."
    ;
    float curPos = 0.0;
    void innerInit() override
    {
        this->addColorParam("color", 0x8000e0);
        this->addIntParam("width", 21);
        this->addIntParam("speed", 1);
        if (!this->name)
            this->name = "Text";

        // scan the 96 characters for low & high bits
        for(int cx = 0; cx < 96; cx++)
        {
            int offset = cx * 8;
            uint8_t bits = 0;
            for(int ix = 0; ix < 8; ix++)
                bits |= font8x8[offset + ix];

            int lowBit = 7;
            int highBit = 0;

            if(bits == 0)
            {
                lowBit = 3;
                highBit = 3;
            }
            else
            {
                for(int ix = 0; ix < 8; ix++)
                    if(bits & (1 << ix))
                    {
                        lowBit = ix;
                        break;
                    }
                for(int ix = 7; ix >= 0; ix--)
                    if(bits & (1 << ix))
                    {
                        highBit = ix;
                        break;
                    }
            }
            int x = lowBit + (highBit << 4);
            font8x8Spans[cx] = x;
        }
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        OmLed16 co = this->getParamValueColor(0);
        int width = this->getParamValueInt(1);
        int speed = this->getParamValueInt(2);

        for(int ix = 0; ix < this->ledCount; ix += width)
            strip->leds[ix] = OmLed16(0,0,15000);


        int k = (int)strlen(this->s);
        int x0 = 0;
        for(int ic = 0; ic < k; ic++)
        {
            // FOR EACH CHARACTER

            OmLed16 co0;
            co0.setHsv(ic * 11000, -1, 32000);
            co0 *= co;
            char ch = this->s[ic];
            ch -= 32;
            int bitLeft = 7 - ((font8x8Spans[(int)ch] >> 4) & 0x0f);
            int bitRight = 7 - (font8x8Spans[(int)ch] & 0x0f);
            int bitWidth = 0;

            if(ch < 0 || ch >= 96)
                goto nextChar;
            // we look at only, really, the top 7 lines of the font.
            // which, by the way, are stored top to bottom.
            // our pixels go bottom to top.

            bitWidth = bitRight - bitLeft + 1;
            for(int iy = 0; iy < 7; iy++)
                for(int ix = 0; ix < width; ix++)
                {
                    // FOR EACH PIXEL GOING ACROSS
                    float x1 = x0 + ix - this->curPos;
                    if(x1 < 0)
                        continue;
                    if(x1 >= width)
                        continue;
                    // x0 is in range 0 to width-1
                    // iy is in range 0 to 7

                    float lx = iy * width + (width - 1 - x1);

                    if(lx >= this->ledCount || lx < 0)
                        continue;

                    int bit = font8x8[ch * 8 + 6-iy] & (1 << (7-ix-bitLeft));
                    if(bit)
                        strip->fillRange(lx, lx + 1.2, co0);
//                        strip->leds[lx] = co0;
                }
        nextChar:
            x0 += bitWidth + 1;
//            if(x0 >= width)
//                break;
        }
        this->curPos += (float)ms * (float)speed / 1000.0;
        if(this->curPos > x0)
            this->curPos = -width;
    }
};


class LoadTester : public OmLed16Pattern
{
public:
    float kCur = 0;
    int msCur = 0; // counts up to 2000 and repeats. 2 second cycle
    void innerInit() override
    {
        this->name = "LoadTester";
        this->addColorParam("color", OmLed16(0x8000, 0, 0));
        this->addIntParam("k0001", 0);
        this->addIntParam("k0100", 0);

        this->addIntParam("rangeLo_u", 0);
        this->addIntParam("rangeLo_m", 0);
        this->addIntParam("rangeLo_l", 0);
        this->addIntParam("rangeHi_u", 0);
        this->addIntParam("rangeHi_m", 0);
        this->addIntParam("rangeHi_l", 0);
    }

    int readSixDigits(int paramIx)
    {
        int k = 10000 * pinRangeI(this->getParamValueInt(paramIx), 0, 7)
        + 100 * this->getParamValueInt(paramIx+1)
        + this->getParamValueInt(paramIx+2);
        k = pinRangeI(k, 0, 65536);
        return k;
    }

    // unbound read, 0..9999 inclusive
    int readFourDigits(int paramIx)
    {
        int k = 1 * this->getParamValueInt(paramIx+0)
        + 100 * this->getParamValueInt(paramIx+1);
        return k;
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        OmLed16 co = this->getParamValueColor(0);

        int rangeLo = readSixDigits(3);
        int rangeHi = readSixDigits(6);
        if(rangeLo != rangeHi)
        {
            this->msCur += ms;
            while(this->msCur >= 2000)
                this->msCur -= 2000;
            float phase = this->msCur / 1000.0;
            if(phase > 1.0)
                phase = 2.0 - phase;
            int g = mapRange(phase, 0, 1, rangeLo, rangeHi);
            co *= OmLed16(g, g, g);
        }

        int k = readFourDigits(1) + 1;// this->getParamValueInt(1) + this->getParamValueInt(2);
        // draw white pip until it migrates & arrives
        if(this->kCur != k)
            strip->fillRange(k-1, k, OmLed16(0xffff,0xffff,0xffff));
        float kMig = 30.0 * ms / 1000.0;
        this->kCur = migrateF(this->kCur, k, kMig);
        strip->fillRange(0, this->kCur, co);
    }
};

class Metropolis : public OmLed16Pattern
{
public:

    class Ring
    {
    public:
        float y; // current y spot
        float v; // current velocity
    };

    std::vector<Ring> rings;

    const float maxV = 4.0;

    float kCur = 0;
    void innerInit() override
    {
        this->name = "Metropolis";
        this->addColorParam("color", OmLed16(0, 0, 0x3000));
        this->addIntParam("interval", 21);
        this->addIntParam("intervalCents", 0);
        this->addIntParam("rings", 4);
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        OmLed16 co = this->getParamValueColor(0);
        int interval = this->getParamValueInt(1);
        int intervalCents = this->getParamValueInt(2);
        int rings = this->getParamValueInt(3);

        int intervalF = interval + intervalCents / 100.0;
        if(intervalF < 3)
            intervalF = 3;

        float yMax = this->ledCount / intervalF;

        while(this->rings.size() < rings)
        {
            {
                Ring r;
                r.y = 0;
                r.v = rr(0.1, maxV);
                this->rings.push_back(r);
            }
        }

        for(int rx = 0; rx < rings; rx++)
        {
            Ring &r = this->rings[rx];
            r.y += r.v * ms / 1000.0;
            if(r.y < 0)
            {
                r.y = 0;
                r.v = rr(0.1, maxV);
            }
            else if(r.y >= yMax - 1)
            {
                r.y = yMax - 1;
                r.v = -rr(0.1, maxV);
            }
            // draw antialiased ring...
            float yFloor = floor(r.y);
            float b1 = r.y - yFloor;
            float b0 = 1.0 - b1;
            strip->fillRange(yFloor * intervalF, (yFloor+1) * intervalF, co * b0);
            strip->fillRange((yFloor+1) * intervalF, (yFloor+2) * intervalF, co * b1);
        }
    }
};


class FourOnFloor : public OmLed16Pattern
{
public:
    float kCur = 0;
    uint32_t msCur = 0;
    float decay;
    int lastBeatNumber;

    class Beat
    {
    public:
        float decay = 0;
        float decayE = 0.9;
        float x;
        float w;
        float xVel;
        void init(float x, float w, float xVel, float decayE)
        {
            this->x = x;
            this->w = w;
            this->xVel = xVel;
            this->decayE = decayE;
        }
    };

    Beat beats[3];

    void innerInit() override
    {
        beats[0].init(20, 30, rrS(10,20), 0.84);
        beats[1].init(40, 40, rrS(10,20), 0.87);
        beats[2].init(60, 50, rrS(10,20), 0.91);

        this->name = "FourOnFloor";
        this->addColorParam("color1", 0x808080);
        this->addColorParam("color2", 0x808080);
        this->addColorParam("color4", 0x808080);
        this->addIntParam("bpm", 120);
        this->addAction("sync");
    }

    void innerDoAction(unsigned int ix, bool buttonDown) override
    {
        if(buttonDown)
        {
            for(int ix = 0; ix < 3; ix++)
            {
                this->beats[ix].decay = 1.0;
                this->beats[ix].xVel = rrS(1,10);
                this->beats[ix].w = rr(20,60);
                this->beats[ix].decayE = 0.68 + ix * 0.05 + rr(-.07, 0.07);
            }
            this->lastBeatNumber = 0xffffffff;
            this->msCur = 0;
        }
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        int bpm = this->getParamValueInt(3);

        this->msCur += ms;

        int beatNumber = this->msCur * bpm * 4 / 60 / 1000;
        int beatRising = (this->lastBeatNumber & (~beatNumber)) & 0x0007; // lowest three beat bumpings.
        this->lastBeatNumber = beatNumber;

        for(int ix = 0; ix < 3; ix++)
        {
            OmLed16 co = this->getParamValueColor(ix);
            Beat &beat = this->beats[ix];
            if(beatRising & (1<<ix))
                beat.decay = 1.0;
            beat.x += beat.xVel * ms / 1000.0;
            if(beat.xVel > 0)
            {
                if(beat.x >= strip->ledCount)
                    beat.x -= strip->ledCount;
            }
            else
            {
                if(beat.x < 0)
                    beat.x += strip->ledCount;
            }
            strip->fillRangeRing(beat.x, beat.x + beat.w, co * beat.decay);
            beat.decay *= 0.9;
        }
    }
};




void drawCenteredRing(OmLed16Strip *strip, float x, float w, OmLed16 color)
{
    strip->fillRangeRing(x - w / 2, x + w / 2, color);
}

#ifdef __OmNtp__
// clock patterns depend on OmNtp, but can't require it.

class Clock1 : public OmLed16Pattern
{
public:

    float oo = 0;
    float rate = 0.051278;
    void innerInit() override
    {
        this->addIntParam("brightness", 10);
        if (!this->name)
            this->name = "Clock";
    }

    void innerTick(uint32_t ms, OmLed16Strip *strip) override
    {
        int hour = 0, minute = 30;
        float second = 30;
        int mwd;
        float swm;
        OmNtp::sGetTimeOfDay(mwd, swm);
        if(mwd >= 0)
        {
            hour = mwd / 60;
            minute = mwd % 60;
            second = swm;
        }

        float secondFrac = second - floor(second);

        int sz = this->ledCount;
        hour = (hour * sz / 12) % sz;
        minute = (minute * sz / 60) % sz;
        second = fmodf(second * sz / 60, sz);
        secondFrac = secondFrac * sz;


        float b = getParamValueInt(0) / 100.0;
        b = pow(b, 2.5); // curve
        //      b = mapRange(b, 0, 1, 0.05, 1);

        drawCenteredRing(strip, hour, 6.7, OmLed16::hsv(0, 0, 0xc800) * b);
        drawCenteredRing(strip, minute, 4.3, OmLed16::hsv(0, 0xffff, 0xa000) * b);
        drawCenteredRing(strip, second, 2.5, OmLed16::hsv(0xb800, 0xffff, 0x5000) * b);
        drawCenteredRing(strip, secondFrac, 5.5, OmLed16::hsv(0xAAAA, 0xffff, 0x3000) * b);

        OmLed16 tickCo(0, 0x0300, 0);
        for (int ix = 0; ix < 12; ix ++)
        {
            float x = ix * sz / 12.0;
            float wi = ix % 3 ? 2.2 : 4.2;
            OmLed16 tickCo = ix % 3 ? OmLed16(0, 0x0C00, 0) : OmLed16(0x0800, 0x1000, 0x0400);
            tickCo = tickCo * b;

            drawCenteredRing(strip, x, wi, tickCo);
        }
    }
};

#endif

void addOmLedPatterns1(OmLed16PatternManager &pm)
{
#define _AP_1_(_pa) pm.addPattern(new _pa())
    _AP_1_(PunchyRoomLight);
#ifdef __OmNtp__
    _AP_1_(Clock1);
#endif
    _AP_1_(Metropolis);
    _AP_1_(Bouncings);
    _AP_1_(Dashes);
    _AP_1_(Text);
    _AP_1_(RainbowWorm);
    _AP_1_(ChasingWorms1);
    _AP_1_(ThinkingBlocks1);
    _AP_1_(Solid);
    _AP_1_(Wheels);
    _AP_1_(Fire1);
    _AP_1_(Fire2);
    _AP_1_(FourOnFloor);
    _AP_1_(Automata);
    _AP_1_(LoadTester);

#undef _AP_1_
}


#endif /* Patterns_h */

