//
//  OmXmlWriter.cpp
//  OmEspHelpers
//
//  Created by David Van Brink on 11/20/16.
//  Copyright (c) 2016 omino.com. All rights reserved.
//

#include "OmXmlWriter.h"
#include "OmUtil.h"
#include <stdlib.h>

static const int HAS_ANYTHING = 1;
static const int HAS_SUBELEMENT = 2;
static const int HAS_TEXT = 4;

class XmlContentEscaper : public OmIByteStream
{
public:
    OmIByteStream *consumer = NULL;
    XmlContentEscaper()
    {
        return;
    }

    bool putX(const char *s)
    {
        bool ok = true;
        while(char ch = *s++)
        {
            ok &= this->consumer->put(ch);
        }
        return ok;
    }

    virtual bool put(uint8_t ch)
    {
        if(this->isDone || !this->consumer)
            return false;

        bool ok = true;
        switch(ch)
        {
            case '"': ok = this->putX("&quot;"); break;
            case '<': ok = this->putX("&lt;"); break;
            case '&': ok = this->putX("&amp;"); break;
            default:
                ok = this->consumer->put(ch);
                break;
        }
        return ok; // yup, did.
    }
};

XmlContentEscaper xmlContentEscaper;

/// Instantiate an XML writer to write into the specified buffer
OmXmlWriter::OmXmlWriter(OmIByteStream *consumer)
{
    this->consumer = consumer;
}

void OmXmlWriter::indent()
{
    if(this->indenting)
        for(int i = 0; i < depth; i++)
            puts(" ");
}

void OmXmlWriter::cr()
{
    if(this->indenting)
        puts("\n");
}

void OmXmlWriter::addingToElement(bool addContent)
{
    // call this before inserting content or another element, in an elemnet.
    if(depth > 0 && this->hasAnything[depth] == 0)
    {
        puts(">");
        if(!addContent) // for embedded text content, we omit line feeds and indents around it. at least a little.
            this->cr();
    }
    
    this->hasAnything[depth] |= HAS_ANYTHING;
    this->hasAnything[depth] |= addContent ? HAS_TEXT : HAS_SUBELEMENT;
}


void OmXmlWriter::beginElement(const char *elementName)
{
    this->endAttribute(); // close any such open element.

    this->addingToElement(false);
    this->depth++;
    this->hasAnything[depth] = 0;
    this->elementName[depth] = elementName;
    this->indent();
    putf("<%s",elementName);
}

void OmXmlWriter::beginElement(const char *elementName, const char *attribute1, const char *value1)
{
    this->beginElement(elementName);
    this->addAttribute(attribute1, value1);
}


static int putSome(char *writeHere, const char *s)
{
    int k = 0;
    while(char c = *s++)
    {
        *writeHere++ = c;
        k++;
    }
    return k;
}

// either for element or for attribute. attributes need &#10; and &#13; for line breaks.
// return 0 for ok, 1 for errors.
// TODO: this could be a class method, this->putWithEscapes(), in every instance, to avoid intermediate buffer. dvb2019.
static int doEscapes(const char *sIn, int outSize, char *sOut,
                     bool forElement = false,
                     bool isUrl = false)
{
    int position = 0;
    
    if(forElement)
    {
        // if there's any '<' or some others do cdata.
        bool hasLt = false;
        char c;
        const char *sInW = sIn;
        while((c = *sInW++) && !hasLt)
        {
            switch(c)
            {
                case '<':
                case '"':
                    hasLt = true;
                    break;
            }
        }
        
        if(hasLt)
        {
            sInW = sIn;
            position = putSome(position + sOut, "<![CDATA[");
            while(char c = *sIn++)
            {
                sOut[position++] = c;
                if(position >= outSize - 3)
                {
                    sOut[position] = 0;
                    return 1;
                }
            }
            position += putSome(position + sOut, "]]>");
            sOut[position++] = 0;
            return 0;
        }
    }
    
    while(char c = *sIn++)
    {
        if(outSize - position < 10)
        {
            return 1;
        }
        
        switch(c)
        {
            case '"':
                position += putSome(position + sOut, "&quot;");
                break;
            case '<':
                position += putSome(position + sOut, "&lt;");
                break;
            case 10:
                position += putSome(position + sOut, "&#10;");
                break;
            case ' ':
                if(isUrl)
                    position += putSome(position + sOut, "%20");
                else
                    sOut[position++] = c;
                break;
                
            default:
                sOut[position++] = c;
                break;
        }
    }
    sOut[position] = 0;
    return 0;
}

#define TINY_XML_ADD_MAX 1000
void OmXmlWriter::addContent(const char *content)
{
    this->addingToElement(true);
    bool doEscapes = true;
    if(this->depth)
    {
        const char *currentElement = this->elementName[this->depth];
        if(omStringEqual("script", currentElement))
            doEscapes = false;
    }
    this->puts(content, doEscapes);
}

void OmXmlWriter::addContentRaw(const char *content)
{
    this->puts(content); // no escapes. Just add text.
}


void OmXmlWriter::addContentF(const char *fmt,...)
{
    char content[TINY_XML_ADD_MAX];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(content, sizeof(content), fmt, ap);
    if(len >= sizeof(content))
        this->errorCount++;
    this->addContent(content);
}


void OmXmlWriter::addAttribute(const char *attribute, const char *value)
{
    this->endAttribute(); // just in case
    if(attribute && value)
    {
        char valueEscaped[TINY_XML_ADD_MAX];
        this->errorCount += doEscapes(value, sizeof(valueEscaped), valueEscaped);
        
        this->putf(" %s=\"%s\"",attribute,valueEscaped);
    }
}

void OmXmlWriter::addAttributeF(const char *attribute, const char *fmt,...)
{
    this->endAttribute(); // just in case
    char value[TINY_XML_ADD_MAX];
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(value,sizeof(value),fmt,ap);
    
    char valueEscaped[TINY_XML_ADD_MAX];
    this->errorCount += doEscapes(value, sizeof(valueEscaped), valueEscaped);

    putf(" %s=\"%s\"",attribute,valueEscaped);
}

void OmXmlWriter::addAttributeFBig(int reserve, const char *attribute, const char *fmt,...)
{
    this->endAttribute(); // just in case

    char *value = NULL;
    char *valueEscaped = NULL;
    value = (char *)calloc(1, reserve);
    int wrote;
    if(!value)
    {
        this->errorCount++;
        goto goHome;
    }
    valueEscaped = (char *)calloc(1, reserve);
    if(!valueEscaped)
    {
        this->errorCount++;
        goto goHome;
    }

    va_list ap;
    va_start(ap,fmt);
    wrote = vsnprintf(value,reserve - 1,fmt,ap);
    if(wrote < 0 || wrote >= reserve - 1)
        this->errorCount++;

    this->errorCount += doEscapes(value, reserve, valueEscaped);

    puts(" ");
    puts(attribute);
    puts("=\"");
    puts(valueEscaped);
    puts("\"");

goHome:
    if(value)
        free(value);
    if(valueEscaped)
        free(valueEscaped);
}


void OmXmlWriter::addAttributeUrlF(const char *attribute, const char *fmt,...)
{
    char value[TINY_XML_ADD_MAX];
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(value,sizeof(value),fmt,ap);
    
    char valueEscaped[TINY_XML_ADD_MAX];
    this->errorCount += doEscapes(value, sizeof(valueEscaped), valueEscaped, false, true);
    
    putf(" %s=\"%s\"",attribute,valueEscaped);
}



void OmXmlWriter::addAttribute(const char *attribute, long long int value)
{
    this->endAttribute(); // just in case
    putf(" %s=\"%lld\"",attribute,value);
}

void OmXmlWriter::addElement(const char *elementName)
{
    this->beginElement(elementName);
    this->endElement();
}

void OmXmlWriter::addElement(const char *elementName, const char *content)
{
    this->beginElement(elementName);
    this->addContent(content);
    this->endElement();
}

void OmXmlWriter::addElementF(const char *elementName, const char *fmt,...)
{
    char value[TINY_XML_ADD_MAX];
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(value,sizeof(value),fmt,ap);

    this->addElement(elementName, value);
}

void OmXmlWriter::endElement()
{
    this->endElement(NULL);
}

void OmXmlWriter::endElement(const char *elementName)
{
    if(elementName)
    {
        if(strcmp(elementName, this->elementName[this->depth]))
            this->errorCount++;
    }

    if(this->hasAnything[depth])
    {
        if(!(this->hasAnything[depth] & HAS_TEXT)) // no indent if an element has character content
            this->indent();
        putf("</%s>",this->elementName[depth]);
    }
    else
        puts("/>");
    this->cr();
    this->depth--;
}

void OmXmlWriter::endElements()
{
    while(this->depth)
        this->endElement();
}

void OmXmlWriter::putf(const char *fmt,...)
{
    static char putfBuffer[TINY_XML_ADD_MAX];
    long len;
    va_list ap;
    va_start(ap,fmt);
    
    // first to our local buffer, for the length check.
    len = vsnprintf(putfBuffer, sizeof(putfBuffer), fmt, ap);
    if(len >= sizeof(putfBuffer))
        this->errorCount++;
        
    this->puts(putfBuffer);
}

void OmXmlWriter::puts(const char *stuff, bool contentEscapes)
{
    long len = strlen(stuff);
    bool error = false;

    // TODO here is where we could consider checking
    // running flags like addingToElement and addingToAttribute
    // to see how best to escape them.
    // TODO: right now CDATA never happens, we never do escapes
    // on content blocks.
    // sometimes this is good, to let you manually add
    // in XML/HTML directives but you have to be rigorous.
    // Maybe we need addContent and addContentRaw.
    if(this->consumer)
    {
        OmIByteStream *dest = this->consumer;
        if(contentEscapes)
        {
            dest = &xmlContentEscaper;
            xmlContentEscaper.consumer = this->consumer;
        }
        bool result = true; // success
        for(int ix = 0; ix < len; ix++)
        {
            this->byteCount++;
            result &= dest->put(stuff[ix]);
        }
        if(!result)
            error = true;
    }
}

void OmXmlWriter::setIndenting(int onOff)
{
    this->indenting = onOff;
}

int OmXmlWriter::getErrorCount()
{
    return this->errorCount;
}

int OmXmlWriter::getByteCount()
{
    return this->byteCount;
}

bool OmXmlWriter::put(uint8_t ch)
{
    /// currently only supported for long attributes. Could easily support element content too.
    bool result = true;
    if(this->attributeName)
    {
        this->byteCount++;
        result &= this->consumer->put(ch);
    }
    return result;
}

bool OmXmlWriter::done()
{
    return this->consumer->done(); // should probably really be "flush"
}

void OmXmlWriter::beginAttribute(const char *attributeName)
{
    this->endAttribute(); // really, almost every method should call this. Just in case it's sitting open.
    this->attributeName = attributeName;
    this->putf(" %s=\"", attributeName);
}
void OmXmlWriter::endAttribute()
{
    if(this->attributeName)
    {
        // an attribute is open, so close it.
        this->put('"');
        this->attributeName = 0;
    }
}
