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

// xmlwriter has option to pass bytes right to someone else.
class OmIByteStream
{
public:
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

    /// convenience routine, same as put byte-by-byte.
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
    int byteCount = 0;

    const char *attributeName = 0; // current open attribute if any, streaming into it
    
    /// Instantiate an XML writer to write into the specified buffer
    OmXmlWriter(OmIByteStream *consumer);
    
    /// Begins a new XML element, like <elementName>
    void beginElement(const char *elementName);
    
    /// Begins a new XML element with one attribute already in it, like <elementName attr="value">
    void beginElement(const char *elementName, const char *attribute1, const char *value1);
    
    /// Adds text to an element, like <element>content</element>
    void addContent(const char *content);

    /// Adds text to the document, but don't escape things. You can wreck your document with this!
    /// But also needed things like rendering DOCTYPE at the start. Caution.
    void addContentRaw(const char *content);

    /// Adds text to an element, using printf semantics
    void addContentF(const char *fmt,...);
    
    /// Adds an attribute to an element, like <element attr="value">
    void addAttribute(const char *attribute, const char *value);
    
    /// Adds an attribute to an element, using printf semantics
    void addAttributeF(const char *attribute, const char *fmt,...);

    /// Handle oversized attribute. :-/
    void addAttributeFBig(int reserve, const char *attribute, const char *fmt,...);

    /// Adds an attribute to an element, using printf semantics, and %20 escapes.
    void addAttributeUrlF(const char *attribute, const char *fmt,...);
    
    /// Adds an attribute to an element from an integer
    void addAttribute(const char *attribute, long long int value);
    
    /// Adds an element with no attributes or content (no need for endElement()) like <hr/>
    void addElement(const char *elementName);
    
    /// Adds an element with content (no need for endElement()) like <h1>Content</h1>
    void addElement(const char *elementName, const char *content);
    
    /// Adds an element with content (no need for endElement()) like <h1>Content</h1>
    void addElementF(const char *elementName, const char *fmt,...);
    
    /// Ends the most recent beginElement(). Caps them with either <element/> or </element>.
    void endElement();

    /// Ends most recent beginElement, and prints error message if element name does not match.
    void endElement(const char *elementName);
    
    /// Ends any remaining elements.
    void endElements();
    
    /// Enables primitive formatting. Uses more bytes of buffer.
    void setIndenting(int onOff);
    
    /// Returns nonzero of any errors occurred, usually buffer overruns leading to missing portions.
    int getErrorCount();

    /// Bytes written.
    int getByteCount();

    /// Our own put.
    /// OmXmlWriter needs to know what context it's in, to manage escapes correctly.
    /// also relies upon some new concepts like beginAttribute, <stream a big attribute value>, endAttribute.
    bool put(uint8_t ch) override;
    bool done() override;

    void beginAttribute(const char *attributeName);
    void endAttribute();


private:
    void indent();
    void cr();
    void addingToElement(bool addContent);
    void putf(const char *fmt,...);
    void puts(const char *stuff, bool contentEscapes = false);
};

#endif /* defined(__OmXmlWriter__) */
