#include "OmXmlWriter.h"


#if NOT_ARDUINO
// on mac, we must simply pass 'v'. But on esp32
// we need &v reference, otherwise the walk is lost.
#define VA_LIST_ARG va_list v
#else
#define VA_LIST_ARG va_list &v
#endif

/*!
 A limited implementation of printf semantics that streams to a consumer.
 It doesn't handle positional arguments... and may have other subtle omissions.
 But works for basic typical printf. Doesn't depend on an in-memory char target
 big enough for the result.
 */
class OmPrintfStream : public OmIByteStream
{
public:
    OmIByteStream *consumer = NULL;

    bool handlePercent(const char *start, const char *end, VA_LIST_ARG);
    OmPrintfStream(OmIByteStream *consumer);
    bool putF(const char *fmt, ...);
    bool putVF(const char *fmt, VA_LIST_ARG);

    /*! @brief like sprintf, but to an OmIByteStream */
    static bool putF(OmIByteStream *consumer, const char *fmt, ...);
    static bool putVF(OmIByteStream *consumer, const char *fmt, VA_LIST_ARG);
};
