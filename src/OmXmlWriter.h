/*
 * OmXmlWriter.h
 * 2016-11-20
 *
 * This class implements an XML writer that produces
 * more-or-less well-formed XML.
 *
 * This code is platform agnostic.
 *
 * It has some weakness with regard to escaping special
 * characters. If you tell it produce an element named named "h1<hahaha>"
 * it really won't be quoted right. But in basic use it's quite helpful.
 *
 * It writes into a character buffer that you provide.
 *
 * EXAMPLE
 *
 * char myBuffer[1000];
 * OmXmlWriter w(myBuffer, 1000); // tell it the space limit
 *
 * w.beginElement("html");
 * w.beginElement("head");
 * w.beginElement("title");
 * w.addContent("A Web Page");
 * w.endElement(); // ends title
 * w.endElement(); // ends head
 * w.beginElement("body");
 * w.beginElement("img");
 * w.addAttribute("src", "http://www.omino.com/img/2016-02-24.jpg");
 * w.endElement(); // ends img
 * w.addContent("This is a nice picture");
 * w.endElement(); // ends body
 * w.endElement(); // ends html.
 *
 * someNetworkThingie.send(w);
 *
 * To produce <html><head><title>A Web Page</title></head><body>
 *            <img src="http://www.omino.com/img/2016-02-24.jpg" />
 *            This is a nice picture
 *            </body></html>
 */

#ifndef __OmXmlWriter__
#define __OmXmlWriter__

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*! Interface for streaming byte output. Can be chained together, and used as a destination by OmXmlWriter */
class OmIByteStream
{
public:
    /*!
     @abstract emit a single byte, overridden by any implementation
     */
    virtual bool put(uint8_t ch)
    {
        if(this->isDone)
            return false;
        printf("%c", ch); // simplest implementation
        return true; // yup, did.
    }
    
    virtual bool done()
    {
        this->isDone = true;
        return true;
    }

    /*! @abstract convenience routine, same as put byte-by-byte. */
    virtual bool putS(const char *s)
    {
        bool result = true;
        while(uint8_t ch = (uint8_t)(*s++))
        {
            result &= this->put(ch);
        }
        return result;
    }
    
    bool isDone = false;
};

/*!
 @class OmXmlWriter
 Writes formatted XML to an OmIByteStream, streaming as it goes (mostly)
 */
class OmXmlWriter : public OmIByteStream
{
public:
    static const int kOmXmlMaxDepth = 20;
    OmIByteStream *consumer = 0;
    int depth = 0;
    int hasAnything[kOmXmlMaxDepth];
    const char *elementName[kOmXmlMaxDepth];
    int position = 0; // index into output
    bool indenting = false;
    int errorCount = 0;
    unsigned int byteCount = 0;

    const char *attributeName = 0; // current open attribute if any, streaming into it
    
    /*! @abstract Instantiate an XML writer to write into the specified buffer */
    OmXmlWriter(OmIByteStream *consumer);
    
    /*! @abstract Begins a new XML element, like &lt;elementName> */
    void beginElement(const char *elementName);
    
    /*! @abstract Begins a new XML element with one attribute already in it, like &lt;elementName attr="value"> */
    void beginElement(const char *elementName, const char *attribute1, const char *value1);
    
    /*! @abstract Adds text to an element, like &lt;element>content&lt;/element> */
    void addContent(const char *content);

    /*! @abstract Adds text to the document ignoring XML rules
     But also needed things like rendering DOCTYPE at the start. Caution.
     */
    void addContentRaw(const char *content);

    /*! @abstract Adds text to an element, using printf semantics */
    void addContentF(const char *fmt,...);
    
    /*! @abstract Adds an attribute to an element, like &lt;element attr="value"> */
    void addAttribute(const char *attribute, const char *value);
    
    /*! @abstract Adds an attribute to an element, using printf semantics */
    void addAttributeF(const char *attribute, const char *fmt,...);

    /*! @abstract Handle oversized attribute. :-/ */
    void addAttributeFBig(int reserve, const char *attribute, const char *fmt,...);

    /*! @abstract Adds an attribute to an element, using printf semantics, and %20 escapes. */
    void addAttributeUrlF(const char *attribute, const char *fmt,...);
    
    /*! @abstract Adds an attribute to an element from an integer */
    void addAttribute(const char *attribute, long long int value);
    
    /*! @abstract Adds an element with no attributes or content (no need for endElement()) like &lt;hr/> */
    void addElement(const char *elementName);
    
    /*! @abstract Adds an element with content (no need for endElement()) like &lt;h1>Content&lt;/h1> */
    void addElement(const char *elementName, const char *content);
    
    /*! @abstract Adds an element with content (no need for endElement()) like &lt;h1>Content&lt;/h1> */
    void addElementF(const char *elementName, const char *fmt,...);
    
    /*! @abstract Ends the most recent beginElement(). Caps them with either &lt;element/> or &lt;/element>. */
    void endElement();

    /*! @abstract Ends most recent beginElement, and prints error message if element name does not match. */
    void endElement(const char *elementName);
    
    /*! @abstract Ends any remaining elements. */
    void endElements();

    /*! @abstract Add content directly to the output stream, no escaping or element balancing.
        You can definitely break your XML output with this! Use wisely.
     */
    void addRawContent(const char *rawContent);
    
    /*! @abstract Enables primitive formatting. Uses more bytes of buffer. */
    void setIndenting(int onOff);
    
    /*! @abstract Returns nonzero of any errors occurred, usually buffer overruns leading to missing portions. */
    int getErrorCount();

    /*! @abstract Bytes written. */
    unsigned int getByteCount();

    /*! Our own put.
    OmXmlWriter needs to know what context it's in, to manage escapes correctly.
    also relies upon some new concepts like beginAttribute, &lt;stream a big attribute value>, endAttribute. */
    bool put(uint8_t ch) override;
    bool done() override;

    void beginAttribute(const char *attributeName);
    void endAttribute();

    void putf(const char *fmt,...);

private:
    void indent();
    void cr();
    void addingToElement(bool addContent);
    bool puts(const char *stuff, bool contentEscapes = false);
    bool inElementContentWithEscapes = false; // triggered by addContent, halted by beginElement and endElement. But not inside <script> for example.
};

#endif /* defined(__OmXmlWriter__) */
