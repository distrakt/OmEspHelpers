/*
 * Some technical notes about why I did it this way.
 * Each rotary encoder gets its own plain-old-C zero-arguments interrupt
 * service routine (isr). This happens because each unique template
 * signature of the class gets its own set of class-global (static)
 * variables and functions, including the isr. A c++ static method
 * is a plain old c function.
 *
 * So, each set of unique pins... gets it own isr.
 *
 * There's no reason to declare two RotaryEncoder instances with
 * the same pins. But it'd probably work, returning same value for both.
 */
template <int pinD0, int pinD1, int pinB>
class OmRotaryEncoder
{
public:
    static int oldD0;
    static int oldD1;
    static int oldB;
    static int irqs;
    static int value4;
    static int value;

    OmRotaryEncoder()
    {
        pinMode(pinD0, INPUT_PULLUP);
        pinMode(pinD1, INPUT_PULLUP);
        pinMode(pinB, INPUT_PULLUP);

        attachInterrupt(pinD0, OmRotaryEncoder::isrRe, CHANGE);
        attachInterrupt(pinD1, OmRotaryEncoder::isrRe, CHANGE);
        attachInterrupt(pinB, OmRotaryEncoder::isrRe, CHANGE);

        OmRotaryEncoder::oldD0 = 1;
        OmRotaryEncoder::oldD1 = 1;
        OmRotaryEncoder::oldB = 1;
        OmRotaryEncoder::irqs = 0;
        OmRotaryEncoder::value4 = 0;
        OmRotaryEncoder::value = 0;
    }

    int getValue()
    {
        return OmRotaryEncoder::value;
    }

    static void IRAM_ATTR isrRe() // i read on the internet that IRAM_ATTR is needed, on dual core esp32... ok
    {

        OmRotaryEncoder::irqs++;

        int d0 = digitalRead(pinD0);
        int d1 = digitalRead(pinD1);

        if(OmRotaryEncoder::oldD0 == d0 && OmRotaryEncoder::oldD1 == d1) return; // not for us.

        int dir = 0;

        if(d0 != OmRotaryEncoder::oldD0)
            dir = d0 == d1 ? +1 : -1;
        else if(d1 != OmRotaryEncoder::oldD1)
            dir = d0 != d1 ? +1 : -1;

        OmRotaryEncoder::value4 += dir;

        /*
         * The rotary encoder I'm using has 4 steps per click.
         * So for debouncing and hysteresis, we lock in the new
         * value when it achieves a multiple of 4, giving room
         * for three steps of slip and bounce without fluttering
         * the value. Really we could do multiples of 2 safely as well.
         * Or 3, but it wouldn't line up to the clicks.
         * Perhaps this precision should be a variable to the instantiation.
         * dvb2019
         */
        if(OmRotaryEncoder::value4 % 4 == 0)
            OmRotaryEncoder::value = OmRotaryEncoder::value4 / 4;

        OmRotaryEncoder::oldD0 = d0;
        OmRotaryEncoder::oldD1 = d1;
    }

};

// TODO this belongs in .cpp file. It can only be stated once, so not belong in .h. dvb2019.
template <int pinD0, int pinD1, int pinB> int OmRotaryEncoder<pinD0, pinD1, pinB>::oldD0;
template <int pinD0, int pinD1, int pinB> int OmRotaryEncoder<pinD0, pinD1, pinB>::oldD1;
template <int pinD0, int pinD1, int pinB> int OmRotaryEncoder<pinD0, pinD1, pinB>::oldB;
template <int pinD0, int pinD1, int pinB> int OmRotaryEncoder<pinD0, pinD1, pinB>::irqs;
template <int pinD0, int pinD1, int pinB> int OmRotaryEncoder<pinD0, pinD1, pinB>::value4;
template <int pinD0, int pinD1, int pinB> int OmRotaryEncoder<pinD0, pinD1, pinB>::value;

