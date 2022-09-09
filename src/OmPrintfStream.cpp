#include "OmPrintfStream.h"

static bool endsPercent(char ch)
{
    const char *enders = "dfgsxc";
    const char *w = enders;
    while(*w)
        if(ch == *w++)
            return true;
    return false;
}

static bool endsWith(const char *s, const char *ending)
{
    int sLen = (int)strlen(s);
    int eLen = (int)strlen(ending);
    if(sLen < eLen)
        return false;

    s += sLen - eLen;
    bool match = strcmp(s, ending) == 0;
    return match;
}

bool OmPrintfStream::handlePercent(const char *start, const char *end, VA_LIST_ARG)
{
    bool result = true;
    const int littleBufferSize = 64;
    char format[littleBufferSize + 1];
    char output[littleBufferSize + 1];
    if(end - start > littleBufferSize)
        result = false;
    else
    {
        char *w = format;
        while(start < end)
            *w++ = *start++;
        *w++ = 0;
        // now what do we do with the format? depends what its type is.

        // first: a special case for plain old %s.
        if(strcmp("%s", format) == 0)
        {
            // just a %s? we can put all the chars ourself.
            // no possible buffer overflow that way.
            const char *chP = va_arg(v, const char *);
            while(*chP)
            {
                result &= this->consumer->put(*chP++);
            }
            goto goHome;
        }

#define FMT_CASE(_f, _t) \
else if(endsWith(format, #_f)) \
{ \
_t x = va_arg(v, _t); \
wrote = snprintf(output, littleBufferSize, format, x); \
}
        int wrote = 0;
        if(0) {}
        FMT_CASE(lld, long long int)
        FMT_CASE(ld, long int)
        FMT_CASE(d, int)
        FMT_CASE(llx, long long int)
        FMT_CASE(lx, long int)
        FMT_CASE(x, int)
        FMT_CASE(f, double)
        FMT_CASE(g, double)
        FMT_CASE(c, int)
        FMT_CASE(s, char *)
        else
        {
            result = false;
        }
        if(result)
        {
            // if wrote is equal to buffer size or greater, some were truncated.
            if(wrote >= littleBufferSize)
            {
                // some were truncated... call it an error.
                result = false;
            }
            // but emit it anyway.
            w = output;
            while(*w)
                this->consumer->put(*w++);
        }
    }
goHome:
    return result;
}

OmPrintfStream::OmPrintfStream(OmIByteStream *consumer)
{
    this->consumer = consumer;
}

bool OmPrintfStream::putF(const char *fmt, ...)
{
    va_list v;
    va_start(v,fmt);
    bool result = putVF(fmt, v);
    return result;
}

bool OmPrintfStream::putVF(const char *fmt, VA_LIST_ARG)
{
    bool result = true;

    bool going = true;
    const char *w = fmt;

    bool inPercent = false;
    const char *percentBegin = 0;
    while(going)
    {
        char ch = *w++;
        if(ch == 0)
        {
            going = false;
            break;
        }

        if(inPercent)
        {
            if(ch == '%' && w == (percentBegin + 2))
            {
                // just an escaped % as %%. Ok.
                result &= this->consumer->put(ch);
                inPercent = false;
            }
            else if(endsPercent(ch))
            {
                result &= this->handlePercent(percentBegin, w, v);
                inPercent = false;
            }
        }
        else if(ch == '%')
        {
            inPercent = true;
            percentBegin = w - 1;
        }
        else
        {
            result &= this->consumer->put(ch);
        }
    }

    return result;
}

bool OmPrintfStream::putVF(OmIByteStream *consumer, const char *fmt, VA_LIST_ARG)
{
    OmPrintfStream ops(consumer);
    bool result = ops.putVF(fmt, v);
    return result;
}

bool OmPrintfStream::putF(OmIByteStream *consumer, const char *fmt, ...)
{
    OmPrintfStream ops(consumer);
    va_list v;
    va_start(v,fmt);

    bool result = ops.putVF(fmt, v);
    return result;
}
