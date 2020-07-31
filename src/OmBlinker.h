/*
 * OmBlinker.h
 * 2016-11-26 dvb
 * This class manages a simple sequence of on-off blinks on
 * an Arduino digital output. Its intended purpose is for
 * indicating conditions numerically, like "three blinks
 * means a relay failure."
 *
 * My particular need today is to show an IP address after
 * connecting, blinking out "one one five (pause)" for example.
 *
 * EXAMPLE
 *
 * OmBlinker b(BUILTIN_LED);
 *
 * in setup():   b.addNumber(1234);
 * in loop():    b.tick();
 */

#include <vector>

/*! @abstract class to help send out timed repeating blinks, can be used to show IP address of the current connection.
 kind of silly, really, but I found it handy to blink out the last octet of 192.168.1.&lt;what ip was i assigned?>.
 */

class OmBlinker
{
    /*! This pattern just keeps repeating */
    std::vector<int> repeatingPattern; // entries are intervals, On then Off. p[0] is an On-duration.
    
    /*! This pattern would fire once, and then the repeating pattern resumes. */
    std::vector<int> oncePattern;
    
    int ledPin = -1;
    int tickDivider = 12; // assuming 50-100 ticks per second, we make time=1 be 12 ticks.
    int patternStep = 0;
    int patternCountdown = 0;
    bool doingOncePattern = false;
    bool inverted = true; // if inverted, digitalWrite(0) means ON.
    
public:
    /*! By default, OmBlinker won't blink, set to pin -1 to disable. */
    OmBlinker(int ledPin = -1);
    void setLedPin(int ledPin);
    void setTickDivider(int tickDivider);
    
    // call this every 10 to 20 milliseconds from loop()
    void tick();
    
    void clear();
    
    /*! add an On blink and an Off blink */
    void addBlink(int onTime, int offTime);
    
    /*! add an On blink (or extend the last one) */
    void addOnTime(int onTime);
    /*! add an Off blink (or extend the last pause) */
    void addOffTime(int offTime);
    
    
    /*! add several blinks to represent a digit 0 to 9, like 3 blinks for 3. NOTE: does zero blinks 10 times */
    void addDigit(unsigned int digit);
    
    /*! add blinks and pauses to play a number in decimal */
    void addNumber(unsigned int number);
    
    /*! Play a simple blink "on top of" the repeating pattern */
    void addInterjection(int blinkCount, int blinkSize);
};
