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

// just for IPAddress :-/
#ifdef ARDUINO_ARCH_ESP8266
#include "ESP8266WiFi.h"
#endif
#ifdef ARDUINO_ARCH_ESP32
#include "WiFi.h"
#endif
#if NOT_ARDUINO
#include <string>
#define String std::string
typedef unsigned char IPAddress[4];
#endif


/// A callback you provide for parameter sliders on a web page.
typedef void (* OmWebActionProc)(const char *pageName, const char *parameterName, int value, int ref1, void *ref2);

/// A callback you provide for rendering custom HTML onto a web page.
typedef void (* HtmlProc)(OmXmlWriter &writer, int ref1, void *ref2);

/// A callback you provide for an arbitrary Url Handler
typedef void (* OmUrlHandlerProc)(OmXmlWriter &writer, OmWebRequest &request, int ref1, void *ref2);

/// Internal class of OmWebPages
class Page;
/// Internal class of OmWebPages
class PageItem;
/// Internal class of arbitrary URL handlers
class UrlHandler;

class OmWebPageItem
{
public:
    OmWebPageItem(PageItem *privateItem);
    const char *getName();
    void setName(const char *name);
    void setVisible(bool visible);
    void setVisible(bool visible, const char *name, int value);
    int getValue();
    void setValue(int value);

    PageItem *privateItem;
};

class OmRequestInfo
{
public:
    IPAddress clientIp;
    int clientPort;
    IPAddress serverIp;
    int serverPort;
    const char *bonjourName = "";
    long long uptimeMillis;
    const char *ssid = "";
    const char *ap = "";
};

class OmWebPages
{
public:
    /// No argument constructor
    OmWebPages();

    void setBuildDateAndTime(const char *date, const char *time);
    
    // +----------------------------------
    // | Setting up your pages
    // |
    
    /// Start defining a new page. Subsequent calls like addButton() affect this page.
    void beginPage(const char *pageName);

    /// Add a link on the current page that goes to another page. Also can call an action proc.
    void addPageLink(const char *pageLink, OmWebActionProc proc = 0, int ref1 = 0, void *ref2 = 0);

    /// Add a button on the current page. Calls the action proc with value 1 for press, 0 for release.
    OmWebPageItem *addButton(const char *buttonName, OmWebActionProc proc = 0, int ref1 = 0, void *ref2 = 0);
    
    /// Add a slider control on the current page. The range is 0 to 100, and calls your param proc when changed.
    OmWebPageItem *addSlider(const char *sliderName, OmWebActionProc proc = 0, int value = 0, int ref1 = 0, void *ref2 = 0);

    /// Add a slider control on the current page, with a specific range.
    OmWebPageItem *addSlider(int rangeLow, int rangeHigh, const char *sliderName, OmWebActionProc proc = 0, int value = 0, int ref1 = 0, void *ref2 = 0);

    /// Add a time-input
    OmWebPageItem *addTime(const char *itemName, OmWebActionProc proc = 0, int value = 0, int ref1 = 0, void *ref2 = 0);

    OmWebPageItem *addColor(const char *itemName, OmWebActionProc proc = 0, int value = 0, int ref1 = 0, void *ref2 = 0);

    /// Add a block of custom HTML to the page. Your proc is called each time the page is requested.
    void addHtml(HtmlProc proc, int ref1 = 0, void *ref2 = 0);


    /// By default, any header proc is used on every page. Disable for current page here.
    void allowHeader(bool allowHeader);

    /// By default, any footer proc is used on every page. Disable for current page here.
    void allowFooter(bool allowFooter);

    // +----------------------------------
    // | Setting up arbitrary handler procs
    // |

    /// Add an arbitrary URL handler.
    void addUrlHandler(const char *path, OmUrlHandlerProc proc, int ref1 = 0, void *ref2 = 0);

    // +----------------------------------
    // | Handling Requests
    // |

    /// Process a request. If you used omWebServer.setHandler(omWebPages), then
    /// this is called by the web server on your behalf. In other scenarios
    /// you can call it directly with the output buffer and request. The request
    /// is the part of the URI after the host name, like "/somepage" or "/info?arg=value".
    /// now with streamed variation overloaded on top.
    bool handleRequest(OmIByteStream *consumer, const char *pathAndQuery, OmRequestInfo *requestInfo);

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

    void setBgColor(int bgColor); // like 0xff0000 red, 0xffffff white.

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

    // +----------------------------------
    // | Internal methods
    // | But you could use them in a urlHandler.
    // |
    void renderInfo(OmXmlWriter &w); // builtin "_info" page

    /// Render the http headers
    void renderHttpResponseHeader(const char *contentType, int response);

    /// Render the beginning of the page, leaving <body> element open and ready.
    void renderPageBeginning(OmXmlWriter &w, const char *pageTitle = "", int bgColor = 0xffffff);
    void renderDefaultFooter(OmXmlWriter &w); // builtin footer nav buttons

    void setValue(const char *pageName, const char *itemName, int value);
private:
    bool doAction(const char *pageName, const char *itemName);
    void renderStyle(OmXmlWriter &w, int bgColor = 0xffffff);
    void renderScript(OmXmlWriter &w);
    
    /// Render a simple menu of all the known pages. It's the default page, too.
    void renderTopMenu(OmXmlWriter &w);
    
    Page *findPage(const char *pageName);
    PageItem *findPageItem(const char *pageName, const char *itemName);
    
    Page *homePage = NULL;
    std::vector<Page *> pages;
    std::vector<UrlHandler *>urlHandlers;
    Page *currentPage = 0; // if a page is active.
    
    HtmlProc headerProc = 0;
    HtmlProc footerProc = 0;
    
    OmXmlWriter *wp = 0; // writer pointer during callbacks.

    int bgColor = 0xffffff;

    const char *__date__;
    const char *__time__;
public:
    OmRequestInfo *ri = 0; // request metadate during callbacks
};

#endif /* defined(__OmWebPages__) */
