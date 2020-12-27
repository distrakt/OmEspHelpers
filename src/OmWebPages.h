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

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

/*! @abstract A callback you provide for value changes of most controls on the web page. */
typedef void (* OmWebActionProc)(const char *pageName, const char *parameterName, int value, int ref1, void *ref2);

/*! @abstract A callback you provide for rendering custom HTML onto a web page. */
typedef void (* HtmlProc)(OmXmlWriter &writer, int ref1, void *ref2);

/*! @abstract A callback you provide for an arbitrary Url Handler */
typedef void (* OmUrlHandlerProc)(OmXmlWriter &writer, OmWebRequest &request, int ref1, void *ref2);

/*! Internal class of OmWebPages */
class Page;
/*! Internal class of OmWebPages */
class PageItem;
/*! Internal class of arbitrary URL handlers */
class UrlHandler;

/*! @class Reference to a single control */
class OmWebPageItem
{
public:
    OmWebPageItem(PageItem *privateItem);
    /*! @abstract current name */
    const char *getName();
    /*! @abstract change name */
    void setName(const char *name);
    /*! @abstract show or hide */
    void setVisible(bool visible);
    /*! @abstract handy method to update several aspects at once */
    void setVisible(bool visible, const char *name, int value);
    /*! @abstract current value */
    int getValue();
    /*! @abstract changes the value, but only visible on next browser load or refresh */
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

/*! @abstract A class that routes and serves web pages, and manages control values, typically works with OmWebServer for the network interface */
class OmWebPages
{
public:
    /*! No argument constructor */
    OmWebPages();
    ~OmWebPages();

    /*!
     @abstract say p.setBuildDateAndTime(__DATE__, __TIME__) so the info web page can display it.
     */
    void setBuildDateAndTime(const char *date, const char *time, const char *file = NULL);
    
    // +----------------------------------
    // | Setting up your pages
    // |
    
    /*! @abstract Start defining a new page. Subsequent calls like addButton() affect this page. */
    void beginPage(const char *pageName);

    /*! @abstract Add a link on the current page that goes to another page. Also can call an action proc. */
    void addPageLink(const char *pageLink, OmWebActionProc proc = NULL, int ref1 = 0, void *ref2 = 0);

    /*! @abstract Add a button on the current page. Calls the action proc with value 1 for press, 0 for release. */
    OmWebPageItem *addButton(const char *buttonName, OmWebActionProc proc = NULL, int ref1 = 0, void *ref2 = 0);

    /*! @abstract Add a button on the current page, which fires a page redirect after the button-up.
     The OmWebActionProc would often be NULL here, if the button's main purpose is to go to another web page.
     */
    OmWebPageItem *addButtonWithLink(const char *buttonName, const char *url, OmWebActionProc proc = NULL, int ref1 = 0, void *ref2 = 0);
    
    /*! @abstract Add a slider control on the current page. The range is 0 to 100, and calls your param proc when changed. */
    OmWebPageItem *addSlider(const char *sliderName, OmWebActionProc proc = NULL, int value = 0, int ref1 = 0, void *ref2 = 0);

    /*! @abstract Add a slider control on the current page, with a specific range. */
    OmWebPageItem *addSlider(int rangeLow, int rangeHigh, const char *sliderName, OmWebActionProc proc = NULL, int value = 0, int ref1 = 0, void *ref2 = 0);

    /*! @abstract Add a time-input */
    OmWebPageItem *addTime(const char *itemName, OmWebActionProc proc = NULL, int value = 0, int ref1 = 0, void *ref2 = 0);

    /*! @abstract Add a color-input, int value is 0xRRGGBB */
    OmWebPageItem *addColor(const char *itemName, OmWebActionProc proc = NULL, int value = 0, int ref1 = 0, void *ref2 = 0);

    /*! @abstract Begin a menu select control. Choices are added with addSelectOption() */
    OmWebPageItem *addSelect(const char *itemName, OmWebActionProc proc = NULL, int value = 0, int ref1 = 0, void *ref2 = 0);
    /*! @abstract Add one selectable item, and its integer value if selected */
    void addSelectOption(const char *optionName, int optionValue);

    /*! @abstract Add a single checkbox.
     If checkboxName is NULL, then the first checkbox will be added by addCheckboxX
     The int value of the control is binary, where the nth checkbox adds 2^(n-1) to the value
     */
    OmWebPageItem *addCheckbox(const char *itemName, const char *checkboxName, OmWebActionProc proc = NULL, int value = 0, int ref1 = 0, void *ref2 = 0);
    /*! @abstract Add additional checkboxes */
    void addCheckboxX(const char *checkboxName, int value = 0); // additional checkboxes.


    /*! @abstract Add a block of custom dynamic HTML to the page. Your proc is called each time the page is requested. */
    void addHtml(HtmlProc proc, int ref1 = 0, void *ref2 = 0);

    /*! @abstract Add a string of static prebuilt HTML. Included in web page unchecked, you're on your own! */
    void addStaticHtml(String staticHtml);

    /*! @abstract By default, any header proc is used on every page. Disable for current page here. */
    void allowHeader(bool allowHeader);

    /*! @abstract By default, any footer proc is used on every page. Disable for current page here. */
    void allowFooter(bool allowFooter);

    // +----------------------------------
    // | Setting up arbitrary handler procs
    // |

    /*! @abstract Add an arbitrary URL handler. */
    void addUrlHandler(const char *path, OmUrlHandlerProc proc, int ref1 = 0, void *ref2 = 0);

    /*! @abstract Add a wildcard url handler, if no page or specific handler gets it. */
    void addUrlHandler(OmUrlHandlerProc proc, int ref1 = 0, void *ref2 = 0);


    // +----------------------------------
    // | Handling Requests
    // |

    /*! @abstract Explicitly process a request. (Not typically used.)
     If you used omWebServer.setHandler(omWebPages), then
    this is called by the web server on your behalf. In other scenarios
    you can call it directly with the output buffer and request. The request
    is the part of the URI after the host name, like "/somepage" or "/info?arg=value".
    now with streamed variation overloaded on top. */
    bool handleRequest(OmIByteStream *consumer, const char *pathAndQuery, OmRequestInfo *requestInfo);

    // +----------------------------------
    // | HtmlProc helpers
    // | Call these from within your HtmlProc to use the builtin styling and formatting.
    // |

    /*! @abstract Within an HtmlProc: Adds a floating link-button to another page. */
    void renderPageButton(OmXmlWriter &w, const char *pageName);

    // +----------------------------------
    // | Global Settings
    // | Setup calls that affect all pages
    // |

    /*! @abstract Override the default header html */
    void setHeaderProc(HtmlProc headerProc);
    /*! @abstract Override the default footer html */
    void setFooterProc(HtmlProc footerProc);

    /*! @abstract set the background color for next web request, 0xRRGGBB */
    void setBgColor(int bgColor); // like 0xff0000 red, 0xffffff white.

    // +----------------------------------
    // | Statistics
    // | Publicly readable variables giving insight into the server behavior.
    // |
    
    /*! Total number of requests served */
    unsigned int requestsAll = 0;
    
    /*! Total number of parameter-change requests served */
    unsigned int requestsParam = 0;
    
    /*! Maximum size of pages served */
    unsigned int greatestRenderLength = 0;

    // +----------------------------------
    // | Internal methods
    // | But you could use them in a urlHandler.
    // |
    void renderInfo(OmXmlWriter &w); // builtin "_info" page
    void renderStatusXml(OmXmlWriter &w); // builtin "_status" url

    /*! @abstract in a OmUrlHandlerProc, set the mimetype (like "text/plain") and response code (200 is OK) */
    void renderHttpResponseHeader(const char *contentType, int response);

    /*! @abstract in a OmUrlHandlerProc Render the beginning of the page, leaving <body> element open and ready. */
    static void renderPageBeginning(OmXmlWriter &w, const char *pageTitle = "", int bgColor = 0xffffff);
    void renderDefaultFooter(OmXmlWriter &w); // builtin footer nav buttons

    static void renderPageBeginningWithRedirect(OmXmlWriter &w, const char *redirectUrl, int redirectSeconds, const char *pageTitle = "", int bgColor = 0xffffff);

    void setValue(const char *pageName, const char *itemName, int value);
    int getValue(const char *pageName, const char *itemName);

    char httpBase[32]; // string of form "http://10.0.1.1/" that can prefix all internal links, esp since OTA update doesnt play well with bonjour addresses
private:

    /*! inner private class */
    class UrlHandler
    {
    public:
        const char *url = "";
        OmUrlHandlerProc handlerProc = NULL;
        int ref1 = 0;
        void *ref2 = 0;
    };

    bool doAction(const char *pageName, const char *itemName);
    static void renderStyle(OmXmlWriter &w, int bgColor = 0xffffff);
    static void renderScript(OmXmlWriter &w);
    
    /*! Render a simple menu of all the known pages. It's the default page, too. */
    void renderTopMenu(OmXmlWriter &w);
    
    Page *findPage(const char *pageName, bool byId = false);
    PageItem *findPageItem(const char *pageName, const char *itemName, bool byId = false);
    
    Page *homePage = NULL;
    std::vector<Page *> pages;
    std::vector<UrlHandler *>urlHandlers;
    Page *currentPage = 0; // if a page is active.

    UrlHandler urlHandler; // generic handler if no specific urls match. Over to you!

    PageItem *currentSelect = 0; // addSelectOption applies to the most recently begun select.
    PageItem *currentCheckboxes = 0; // addCheckboxX adds another checkbox here
    
    HtmlProc headerProc = NULL;
    HtmlProc footerProc = NULL;
    
    OmXmlWriter *wp = 0; // writer pointer during callbacks.

    int bgColor = 0xffffff;

    const char *__date__;
    const char *__time__;
    const char *__file__;
public:
    OmRequestInfo *ri = 0; // request metadate during callbacks
};

extern OmWebPages OmWebPagesSingleton;

#endif /* defined(__OmWebPages__) */
