/*
 * OmBlinker.cpp
 * 2016-11-26 dvb
 * Implementation of OmBlinker
 */

#include "OmBlinker.h"
#include "Arduino.h"

OmBlinker::OmBlinker(int ledPin)
{
    this->setLedPin(ledPin);
}

void OmBlinker::setLedPin(int ledPin)
{
    if(this->ledPin >= 0)
        digitalWrite(this->ledPin, this->inverted); // old one off.

    this->ledPin = ledPin;
    if(this->ledPin >= 0)
    {
        pinMode(this->ledPin, OUTPUT);
        digitalWrite(this->ledPin, this->inverted); // new one off.
    }
}

void OmBlinker::setTickDivider(int tickDivider)
{
    this->tickDivider = tickDivider;
}

/// Internal helper to advance either pattern (repeating or once). Return "true" if it reaches the end.
static bool advancePattern(std::vector<int> &pattern, int ledPin, bool inverted, int tickDivider, int &countdown, int &step)
{
    if(pattern.size() == 0)
        return true;
    
    bool result = false;
    
    if(tickDivider < 1)
        tickDivider = 1;
    
    if(pattern.size() && ledPin >= 0)
    {
        if(countdown <= 0)
        {
            step = 0;
            goto startStep;
        }
        else
        {
            countdown--;
            if(countdown <= 0)
            {
                step = (step + 1) % pattern.size();
                result = step == 0;
            startStep:
                countdown = pattern[step] * tickDivider;
                digitalWrite(ledPin, step & 1 ^ 1 ^ inverted);
            }
        }
        return result;
    }
}

void OmBlinker::tick()
{
    if(this->ledPin < 0)
        return; // nada.
    
    if(this->oncePattern.size())
    {
        // advance the "interjection" pattern
        bool onceDone = advancePattern(this->oncePattern,
                                       this->ledPin, this->inverted, this->tickDivider / 5, this->patternCountdown, this->patternStep);
        if(onceDone)
        {
            this->oncePattern.clear();
            this->patternCountdown = 0;
            this->patternStep = 0;
        }
    }
    else if(this->repeatingPattern.size())
    {
        // advance the main led pattern
        advancePattern(this->repeatingPattern, this->ledPin, this->inverted, this->tickDivider, this->patternCountdown, this->patternStep);
    }
}

void OmBlinker::clear()
{
    this->repeatingPattern.clear();
}

void OmBlinker::addBlink(int onTime, int offTime)
{
    this->addOnTime(onTime);
    this->addOffTime(offTime);
}

void OmBlinker::addOnTime(int onTime)
{
    int sz = (int)this->repeatingPattern.size();
    if(sz & 1)
        this->repeatingPattern[sz - 1] += onTime;
    else
        this->repeatingPattern.push_back(onTime);
}

void OmBlinker::addOffTime(int offTime)
{
    int sz = (int)this->repeatingPattern.size();
    if(sz & 1)
        this->repeatingPattern.push_back(offTime);
    else if(sz)
        this->repeatingPattern[sz - 1] += offTime;
}


/// add several blinks to represent a digit 0 to 9, like 3 blinks for 3. NOTE: does zero blinks 10 times
void OmBlinker::addDigit(unsigned int digit)
{
    digit %= 10;
    if(digit == 0)
        digit = 10;
    for(unsigned int ix = 0; ix < digit; ix++)
    {
        int p = 1;
        if(ix == 0)
            p = 2; // first blink of each digit is longer. looks nice this way I think.
        int q = 1;
        this->addBlink(p, q);
    }
    this->addOffTime(4); // longer gap after the digit.
}

/// add blinks and pauses to play a number in decimal
void OmBlinker::addNumber(unsigned int number)
{
    std::vector<unsigned int> digits;
    if(number == 0)
        digits.push_back(0);
    else
    {
        while(number)
        {
            unsigned int digit = number % 10;
            number /= 10;
            digits.push_back(digit);
        }
    }
    // poke in the blinks in reverse order.
    int ix = digits.size();
    while(--ix >= 0)
    {
        this->addDigit(digits[ix]);
    }
    this->addOffTime(10); // long pause after last digit
}

/// Play a simple blink "on top of" the repeating pattern
void OmBlinker::addInterjection(int blinkCount, int blinkSize)
{
    this->oncePattern.clear();
    for(int ix = 0; ix < blinkCount; ix++)
    {
        bool last = ix == blinkCount - 1;
        this->oncePattern.push_back(blinkSize); 
        this->oncePattern.push_back(blinkSize + (last ? 100 : 0));
    }
    // and start from the beginning. the oncePattern has precedence.
    this->patternStep = 0;
    this->patternCountdown = 0;
}
