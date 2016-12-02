//
//  OmXmlWriter.cpp
//  OmEspHelpers
//
//  Created by David Van Brink on 11/20/16.
//  Copyright (c) 2016 omino.com. All rights reserved.
//

#include "OmXmlWriter.h"

static const int HAS_ANYTHING = 1;
static const int HAS_SUBELEMENT = 2;
static const int HAS_TEXT = 4;

OmXmlWriter::OmXmlWriter(char *outputBuffer, int outputBufferSize)
{
    this->outputBuffer = outputBuffer;
    this->outputBufferSize = outputBufferSize;
    
    this->outputBuffer[0] = 0;
}

void OmXmlWriter::indent()
{
    if(this->indenting)
        for(int i = 0; i < depth; i++)
            put(" ");
}

void OmXmlWriter::cr()
{
    if(this->indenting)
        put("\n");
}

void OmXmlWriter::addingToElement(bool addContent)
{
    // call this before inserting content or another element, in an elemnet.
    if(depth > 0 && this->hasAnything[depth] == 0)
    {
        put(">");
        if(!addContent) // for embedded text content, we omit line feeds and indents around it. at least a little.
            this->cr();
    }
    
    this->hasAnything[depth] |= HAS_ANYTHING;
    this->hasAnything[depth] |= addContent ? HAS_TEXT : HAS_SUBELEMENT;
}


void OmXmlWriter::beginElement(const char *elementName)
{
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
            return 1;
        
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
    this->put(content);
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
    if(attribute && value)
    {
        char valueEscaped[TINY_XML_ADD_MAX];
        this->errorCount += doEscapes(value, sizeof(valueEscaped), valueEscaped);
        
        this->putf(" %s=\"%s\"",attribute,valueEscaped);
    }
}

void OmXmlWriter::addAttributeF(const char *attribute, const char *fmt,...)
{
    char value[TINY_XML_ADD_MAX];
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(value,sizeof(value),fmt,ap);
    
    char valueEscaped[TINY_XML_ADD_MAX];
    this->errorCount += doEscapes(value, sizeof(valueEscaped), valueEscaped);

    putf(" %s=\"%s\"",attribute,valueEscaped);
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



void OmXmlWriter::addAttribute(const char *attribute, int value)
{
    putf(" %s=\"%d\"",attribute,value);
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


void OmXmlWriter::endElement()
{
    if(this->hasAnything[depth])
    {
        if(!(this->hasAnything[depth] & HAS_TEXT)) // no indent if an element has character content
            this->indent();
        putf("</%s>",this->elementName[depth]);
    }
    else
        put("/>");
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
        
    this->put(putfBuffer);
}

void OmXmlWriter::put(const char *stuff)
{
    long len = strlen(stuff);
    bool error = false;
    if(len + this->position < this->outputBufferSize)
    {
        this->position += sprintf(this->outputBuffer + this->position, "%s", stuff);
    }
    else
    {
        error = true;
    }
    this->outputBuffer[this->position] = 0;
    if(error)
        this->errorCount++;
}

void OmXmlWriter::setIndenting(int onOff)
{
    this->indenting = onOff;
}

int OmXmlWriter::getErrorCount()
{
    return this->errorCount;
}