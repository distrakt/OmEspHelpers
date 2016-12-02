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
 * w.beginElement("<html>");
 * w.beginElement("<head>");
 * w.beginElement("<title>");
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

class OmXmlWriter
{
public:
    static const int kOmXmlMaxDepth = 20;
    char *outputBuffer = 0;
    int outputBufferSize = 0;
    int depth = 0;
    int hasAnything[kOmXmlMaxDepth];
    const char *elementName[kOmXmlMaxDepth];
    int position = 0; // index into output
    bool indenting = false;
    int errorCount = 0;
    
    /// Instantiate an XML writer to write into the specified buffer
    OmXmlWriter(char *outputBuffer, int outputBufferSize);

    /// Begins a new XML element, like <elementName>
    void beginElement(const char *elementName);
    
    /// Begins a new XML element with one attribute already in it, like <elementName attr="value">
    void beginElement(const char *elementName, const char *attribute1, const char *value1);
    
    /// Adds text to an element, like <element>content</element>
    void addContent(const char *content);
    
    /// Adds text to an element, using printf semantics
    void addContentF(const char *fmt,...);
    
    /// Adds an attribute to an element, like <element attr="value">
    void addAttribute(const char *attribute, const char *value);
    
    /// Adds an attribute to an element, using printf semantics
    void addAttributeF(const char *attribute, const char *fmt,...);
    
    /// Adds an attribute to an element, using printf semantics, and %20 escapes.
    void addAttributeUrlF(const char *attribute, const char *fmt,...);
    
    /// Adds an attribute to an element from an integer
    void addAttribute(const char *attribute, int value);
    
    /// Adds an element with no attributes or content (no need for endElement()) like <hr/>
    void addElement(const char *elementName);
    
    /// Adds an element with content (no need for endElement()) like <h1>Content</h1>
    void addElement(const char *elementName, const char *content);
    
    /// Ends the most recent beginElement(). Caps them with either <element/> or </element>.
    void endElement();
    
    /// Ends any remaining elements.
    void endElements();
    
    /// Enables primitive formatting. Uses more bytes of buffer.
    void setIndenting(int onOff);
    
    /// Returns nonzero of any errors occurred, usually buffer overruns leading to missing portions.
    int getErrorCount();

private:
    void indent();
    void cr();
    void addingToElement(bool addContent);
    void putf(const char *fmt,...);
    void put(const char *stuff);
};

#endif /* defined(__OmXmlWriter__) */
