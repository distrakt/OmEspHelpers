/*
 * OmWebPages.h
 * 2016-11-20
 *
 * This class implements a description for simple,
 * structured web pages. It offers simplicity at the
 * expense of flexibility. It's intended for simple
 * IoT user interfaces, and very basic text & information
 * display. Also, designed for small screens like phones.
 *
 * This code is platform agnostic.
 *
 * EXAMPLE
 *
 * your callback proc:
 *             void buttonProc(const char *page, const char *item, int value, int ref1, void *ref2)
 *             {
 *                 digitalWrite(LED_BUILTIN, ref1);
 *             }
 *
 *             void buttonMomentaryProc(const char *page, const char *item, int value, int ref1, void *ref2)
 *             {
 *                 digitalWrite(LED_BUILTIN, value);
 *             }
 *
 * in setup():
 *             OmWebPages p
 *             p.beginPage("Home");
 *             p.addButton("ledOn", buttonProc, 0);
 *             p.addButton("ledOff", buttonProc, 1);
 *             p.addButton("ledMomentary", buttonMomentaryProc);
 *
 *             OmWebServer s;
 *             s.addWifi("My Network", "password1234");
 *             s.setHandler(p);
 *             s.setStatusLedPin(-1);
 *
 * in loop():
 *             s.tick(); // in turn calls OmWebPages
 *
 *
 * OmWebPages p;
 */

#ifndef __OmWebPages__
#define __OmWebPages__

#include <vector>
#include "OmXmlWriter.h"
#include "OmUtil.h"
#include "OmWebRequest.h"

/// A callback you provide for parameter sliders on a web page.
typedef void (* OmWebActionProc)(const char *pageName, const char *parameterName, int value, int ref1, void *ref2);

/// A callback you provide for rendering custom HTML onto a web page.
typedef void (* HtmlProc)(OmXmlWriter &writer, int ref1, void *ref2);

/// Internal class of OmWebPages
class Page;
/// Internal class of OmWebPages
class PageItem;

/// External class reference to OmWebServer, for a particular back-connect use
class OmWebServer;

class OmWebPages
{
public:
    // +----------------------------------
    // | Setting up your pages
    // |
    
    /// Start defining a new page. Subsequent calls like addButton() affect this page.
    void beginPage(const char *pageName, const char *pageDisplayName = 0);

    /// Add a link on the current page that goes to another page. Also can call an action proc.
    void addPageLink(const char *pageLink, OmWebActionProc proc = 0, int ref1 = 0, void *ref2 = 0);

    /// Add a button on the current page. Calls the action proc with value 1 for press, 0 for release.
    void addButton(const char *buttonName, OmWebActionProc proc = 0, int ref1 = 0, void *ref2 = 0);
    
    /// Add a slider control on the current page. The range is always 0 to 100, and calls your param proc when changed.
    void addSlider(const char *sliderName, OmWebActionProc proc = 0, int value = 0, int ref1 = 0, void *ref2 = 0);

    /// Add a block of custom HTML to the page. Your proc is called each time the page is requested.
    void addHtml(HtmlProc proc, int ref1 = 0, void *ref2 = 0);


    /// By default, any header proc is used on every page. Disable for current page here.
    void allowHeader(bool allowHeader);

    /// By default, any footer proc is used on every page. Disable for current page here.
    void allowFooter(bool allowFooter);

    // +----------------------------------
    // | Handling Requests
    // |
    
    /// Process a request. If you used omWebServer.setHandler(omWebPages), then
    /// this is called by the web server on your behalf. In other scenarios
    /// you can call it directly with the output buffer and request. The request
    /// is the part of the URI after the host name, like "/somepage" or "/info?arg=value".
    bool handleRequest(char *output, int outputSize, const char *request);
    
    // +----------------------------------
    // | HtmlProc helpers
    // | Call these from within your HtmlProc to use the builtin styling and formatting.
    // |

    /// Adds a floating link-button to another page.
    void renderPageButton(OmXmlWriter &w, const char *pageName);

    // +----------------------------------
    // | Global Settings
    // | Setup calls that affect all pages
    // |
    void setHeaderProc(HtmlProc headerProc);
    void setFooterProc(HtmlProc footerProc);

    // +----------------------------------
    // | Statistics
    // | Publicly readable variables giving insight into the server behavior.
    // |
    
    /// Total number of requests served
    unsigned int requestsAll = 0;
    
    /// Total number of parameter-change requests served
    unsigned int requestsParam = 0;
    
    /// Maximum size of pages served
    unsigned int greatestRenderLength = 0;
    
    /// +----------------------------------
    /// | For _info page only
    void setOmWebServer(OmWebServer *omWebServer);

private:
    // +----------------------------------
    // | Internal methods
    // |
    bool doAction(const char *pageName, const char *itemName);
    void renderTree(OmXmlWriter &w); // builtin "tree" page
    void renderInfo(OmXmlWriter &w); // builtin "_info" page
    void renderStyle(OmXmlWriter &w);
    void renderScript(OmXmlWriter &w);
    
    /// Render the beginning of the page, leaving <body> element open and ready.
    void renderPageBeginning(OmXmlWriter &w, const char *pageTitle);
    /// Render a simple menu of all the known pages. It's the default page, too.
    void renderTopMenu(OmXmlWriter &w);
    
    Page *findPage(const char *pageName);
    PageItem *findPageItem(const char *pageName, const char *itemName);
    
    OmWebServer *omWebServer = NULL; // if directly connected, can show some helpful _info.
    
    std::vector<Page *> pages;
    Page *currentPage = 0; // if a page is active.
    
    HtmlProc headerProc = 0;
    HtmlProc footerProc = 0;
    
    OmXmlWriter *wp = 0; // writer pointer during callbacks.
};

#endif /* defined(__OmWebPages__) */
