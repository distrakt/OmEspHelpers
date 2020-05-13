#include "OmXmlWriter.h"


class OmPrintfStream : public OmIByteStream
{
public:
    OmIByteStream *consumer = NULL;

#if NOT_ARDUINO
    // on mac, we must simply pass 'v'. But on esp32
    // we need &v reference, otherwise the walk is lost.
    bool handlePercent(const char *start, const char *end, va_list v);
#else
    bool handlePercent(const char *start, const char *end, va_list &v);
#endif
    OmPrintfStream(OmIByteStream *consumer);
    bool putF(const char *fmt, ...);
    bool putVF(const char *fmt, va_list v);

    /// one-shot printer.
    static bool putF(OmIByteStream *consumer, const char *fmt, ...);
};
