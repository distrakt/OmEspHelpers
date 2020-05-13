/*
 * OmWebPages.cpp
 * 2016-11-20
 *
 * Implementation of OmWebPages
 */

#include "OmWebPages.h"
#include "OmLog.h"
#ifndef NOT_ARDUINO
#include "Arduino.h"
//extern "C" {
#ifdef ARDUINO_ARCH_ESP8266
#include "user_interface.h" // system_free_space and others.
#include "ESP8266WiFi.h"
#endif
#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal.h"
#include "esp_system.h"
#include "WiFi.h"
#endif
//}
#endif



class PageItem
{
public:
    const char *name = "";
    const char *pageLink = "";
    int ref1 = 0;
    void *ref2 = 0;
    int value = 0;
    bool visible = true;
    
    virtual void render(OmXmlWriter &w, Page *inPage) = 0;
    virtual bool doAction(Page *fromPage) = 0;
    void setValue(int value) { this->value = value; }

    OmWebPageItem item;
    PageItem() : item(this)
    {
        return;
    }
};

OmWebPageItem::OmWebPageItem(PageItem *privateItem)
{
    this->privateItem = privateItem;
}

const char *OmWebPageItem::getName()
{
    return this->privateItem->name;
}

void OmWebPageItem::setName(const char *name)
{
    this->privateItem->name = name;
}

void OmWebPageItem::setVisible(bool visible)
{
    this->privateItem->visible = visible;
}

void OmWebPageItem::setVisible(bool visible, const char *name, int value)
{
    this->privateItem->visible = visible;
    this->privateItem->name = name;
    this->privateItem->value = value;
}


int OmWebPageItem::getValue()
{
    return this->privateItem->value;
}
void OmWebPageItem::setValue(int value)
{
    this->privateItem->value = value;
}

class Page
{
public:
    const char *name = "";
    std::vector<PageItem *> items;
    
    // set to false to inhibit this pages's default header or footer.
    // replace with different via Html items if you like.
    bool allowHeader = true;
    bool allowFooter = true;
};

class UrlHandler
{
public:
    const char *url = "";
    OmUrlHandlerProc handlerProc = NULL;
    int ref1 = 0;
    void *ref2 = 0;
};



class PageLink : public PageItem
{
public:
    OmWebActionProc proc = 0;
    
    void render(OmXmlWriter &w, Page *inPage) override
    {
        w.beginElement("a");
        w.addAttributeUrlF("href", "/%s?page=%s&item=%s", this->pageLink, inPage->name, this->name);
        
        w.beginElement("div");
        w.addAttribute("class", "box1");
        w.addContentF("%s", this->name);
        w.endElement();
        
        w.endElement();
    }
    
    bool doAction(Page *fromPage) override
    {
        if(this->proc)
        {
            this->proc(fromPage->name, this->name, 0, this->ref1, this->ref2);
            return true;
        }
        else
        {
            return false;
        }
    }
};

/// Show a visual slider, which call back on changes
class PageSlider : public PageItem
{
public:
    OmWebActionProc proc = 0;
    int min = 0;
    int max = 100;
    
    void render(OmXmlWriter &w, Page *inPage) override
    {
        /*
         <form>
         <span id="id1" style="display:inline-block;width:100px">100</span>
         <input id="id2" type="range" onchange="aChange(this, 'id1')"
         oninput="aSlide(this, 'id1')" />
         
         <input id="id3" type="range" onchange="aChange(this, this.value)" />
         </form>
         */
        w.beginElement("div", "class", "box1");
        w.addContent(this->name);
        w.beginElement("form");
        w.beginElement("span", "class", "sliderValue");
        w.addAttribute("style", "margin-bottom:15px");
        w.addAttributeF("id", "%s_%s_value", inPage->name, this->name);
        w.addContentF("%d", this->value);
        w.endElement(); // span
        w.beginElement("input", "type", "range");
        w.addAttributeF("value", "%d", this->value);
        w.addAttribute("style", "width: 330px");
        w.addAttributeF("id", "%s_%s", inPage->name, this->name);
        w.addAttributeF("onchange", "sliderInput(this,'%s', '%s')", inPage->name, this->name);
        w.addAttributeF("oninput", "sliderInput(this,'%s','%s')", inPage->name, this->name);
        if(this->min != 0)
            w.addAttribute("min", this->min);
        if(this->max != 100)
            w.addAttribute("max", this->max);
        w.endElement(); // input
        w.endElement(); // form
        w.endElement(); // div
    }
    
    bool doAction(Page *fromPage) override
    {
        if(this->proc)
        {
            this->proc(fromPage->name, this->name, this->value, this->ref1, this->ref2);
            return true;
        }
        else
        {
            return false;
        }
    }
};

/// Show a visual time input, which call back on changes
class PageTime : public PageItem
{
public:
    OmWebActionProc proc = 0;

    static char *minutesToTimeString(int minutes)
    {
        // this control type uses minutes within the 24 hour day to express a time of day.
        static char t[10];
        minutes &= 2047; // 1440 is tops but ok.
        sprintf(t, "%02d:%02d", minutes/60, minutes % 60);
        return t;
    }

    void render(OmXmlWriter &w, Page *inPage) override
    {
        /*
         <form>
         <span id="id1" style="display:inline-block;width:100px">100</span>
         <input id="id2" type="range" onchange="aChange(this, 'id1')"
         oninput="aSlide(this, 'id1')" />

         <input id="id3" type="range" onchange="aChange(this, this.value)" />
         </form>
         */
        w.beginElement("div", "class", "box1");
        w.addContent(this->name);
        w.beginElement("form");
        w.beginElement("span", "class", "sliderValue");
        w.addAttributeF("id", "%s_%s_value", inPage->name, this->name);
//        w.addContentF("%s", minutesToTimeString(this->value));
        w.endElement(); // span
        w.beginElement("input", "type", "time");
        w.addAttributeF("value", "%s", minutesToTimeString(this->value));
        w.addAttributeF("id", "%s_%s", inPage->name, this->name);
        w.addAttributeF("onchange", "timeChange(this,'%s', '%s')", inPage->name, this->name);
        w.endElement(); // input
        w.beginElement("input", "type", "checkbox");
        w.addAttributeF("id", "%s_%s_checkbox", inPage->name, this->name);
        if(this->value & 0x8000)
            w.addAttribute("checked", "checked");
        w.addAttributeF("onchange", "timeChange(this,'%s', '%s')", inPage->name, this->name);
        w.endElement();
        w.endElement(); // form
        w.endElement(); // div
    }

    bool doAction(Page *fromPage) override
    {
        if(this->proc)
        {
            this->proc(fromPage->name, this->name, this->value, this->ref1, this->ref2);
            return true;
        }
        else
        {
            return false;
        }
    }
};

class PageColor : public PageItem
{
public:
    OmWebActionProc proc = 0;

    void render(OmXmlWriter &w, Page *inPage) override
    {
        /*
         <form>
         <span id="id1" style="display:inline-block;width:100px">100</span>
         <input id="id2" type="range" onchange="aChange(this, 'id1')"
         oninput="aSlide(this, 'id1')" />

         <input id="id3" type="range" onchange="aChange(this, this.value)" />
         </form>
         */
        w.beginElement("div", "class", "box1");
        w.addContent(this->name);
        w.beginElement("form");
        w.beginElement("input", "type", "color");
        w.addAttributeF("value", "#%06x", this->value);
        w.addAttributeF("id", "%s_%s", inPage->name, this->name);
        // change just like a slider -- it's a numeric value... ish.
        w.addAttributeF("onchange", "colorInput(this,'%s', '%s')", inPage->name, this->name);
        w.addAttributeF("oninput", "colorInput(this,'%s', '%s')", inPage->name, this->name);
        w.endElement(); // input

        w.beginElement("span", "class", "colorValue");
        w.addAttributeF("id", "%s_%s_value", inPage->name, this->name);
        w.addContentF(" #%06x", this->value);
        w.endElement(); // span

        w.endElement(); // form
        w.endElement(); // div
    }

    bool doAction(Page *fromPage) override
    {
        if(this->proc)
        {
            this->proc(fromPage->name, this->name, this->value, this->ref1, this->ref2);
            return true;
        }
        else
        {
            return false;
        }
    }
};


class PageButton : public PageItem
{
public:
    OmWebActionProc proc = 0;

    void render(OmXmlWriter &w, Page *inPage) override
    {
        w.beginElement("button");
        w.addAttribute("class", "button");
        // We send both mouse and touch events; the event handler may then suppress mouse events
        w.addAttributeF("onmousedown", "button(this,'%s', '%s', 1)", inPage->name, this->name);
        w.addAttributeF("onmouseup", "button(this,'%s', '%s', 0)", inPage->name, this->name);
        w.addAttributeF("ontouchstart", "button(this,'%s', '%s', 3)", inPage->name, this->name);
        w.addAttributeF("ontouchend", "button(this,'%s', '%s', 2)", inPage->name, this->name);
        w.addContentF("%s", this->name);
        w.endElement();
    }
    
    bool doAction(Page *fromPage) override
    {
        if(this->proc)
        {
            this->proc(fromPage->name, this->name, this->value, this->ref1, this->ref2);
            return true;
        }
        else
        {
            return false;
        }
    }
};

/// Call the HtmlProc to render balanced HTML.
class HtmlItem : public PageItem
{
public:
    HtmlProc proc;
    
    void render(OmXmlWriter &w, Page *inPage) override
    {
        if(this->proc)
            this->proc(w, this->ref1, this->ref2);
    }
    bool doAction(Page *fromPage) override
    {
        return true;
    }
};

void infoHtmlProc(OmXmlWriter &w, int ref1, void *ref2)
{
    OmWebPages *owp = (OmWebPages *)ref2;
    owp->renderInfo(w);
}

void defaultFooterHtmlProc(OmXmlWriter &w, int ref1, void *ref2)
{
    OmWebPages *owp = (OmWebPages *)ref2;
    owp->renderDefaultFooter(w);
}

OmWebPages::OmWebPages()
{
    this->__date__ = __DATE__;
    this->__time__ = __TIME__;
    // create the built-in pages
    this->beginPage("_info");
    this->addHtml(infoHtmlProc, 0, this);
    
    // default navigation buttons
    this->setFooterProc(defaultFooterHtmlProc);
    
    // Make sure none of the built-in pages become the user's home page.
    this->homePage = NULL;
}

void OmWebPages::setBuildDateAndTime(const char *date, const char *time)
{
    this->__date__ = date;
    this->__time__ = time;
}

void OmWebPages::beginPage(const char *pageName)
{
    Page *page = new Page();
    page->name = pageName;
    this->pages.push_back(page);
    this->currentPage = page;
    if(!this->homePage)
        this->homePage = page;
}

void OmWebPages::allowHeader(bool allowHeader)
{
    this->currentPage->allowHeader = allowHeader;
}

void OmWebPages::allowFooter(bool allowFooter)
{
    this->currentPage->allowFooter = allowFooter;
}

void OmWebPages::setHeaderProc(HtmlProc headerProc)
{
    this->headerProc = headerProc;
}

void OmWebPages::setFooterProc(HtmlProc footerProc)
{
    this->footerProc = footerProc;
}

void OmWebPages::addPageLink(const char *pageLink, OmWebActionProc proc, int ref1, void *ref2)
{
    PageLink *b = new PageLink();
    b->name = pageLink;
    b->pageLink = pageLink;
    b->proc = proc;
    b->ref1 = ref1;
    b->ref2 = ref2;
    this->currentPage->items.push_back(b);
}

OmWebPageItem *OmWebPages::addButton(const char *buttonName, OmWebActionProc proc, int ref1, void *ref2)
{
    PageButton *b = new PageButton();
    b->name = buttonName;
    b->proc = proc;
    b->ref1 = ref1;
    b->ref2 = ref2;
    this->currentPage->items.push_back(b);
    return &b->item;
}

OmWebPageItem *OmWebPages::addSlider(const char *sliderName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    return this->addSlider(0, 100, sliderName, proc, value, ref1, ref2);
}

OmWebPageItem *OmWebPages::addSlider(int rangeLow, int rangeHigh, const char *sliderName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    PageSlider *p = new PageSlider();
    p->name = sliderName;
    p->value = value;
    p->ref1 = ref1;
    p->ref2 = ref2;
    p->proc = proc;
    p->min = rangeLow;
    p->max = rangeHigh;
    this->currentPage->items.push_back(p);
    return &p->item;
}

OmWebPageItem *OmWebPages::addTime(const char *sliderName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    PageTime *p = new PageTime();
    p->name = sliderName;
    p->value = value;
    p->ref1 = ref1;
    p->ref2 = ref2;
    p->proc = proc;
    this->currentPage->items.push_back(p);
    return &p->item;
}

OmWebPageItem *OmWebPages::addColor(const char *sliderName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    PageColor *p = new PageColor();
    p->name = sliderName;
    p->value = value;
    p->ref1 = ref1;
    p->ref2 = ref2;
    p->proc = proc;
    this->currentPage->items.push_back(p);
    return &p->item;
}

void OmWebPages::addHtml(HtmlProc proc, int ref1, void *ref2)
{
    HtmlItem *h = new HtmlItem();
    h->name = "_";
    h->ref1 = ref1;
    h->ref2 = ref2;
    h->proc = proc;
    this->currentPage->items.push_back(h);
}

bool OmWebPages::doAction(const char *pageName, const char *itemName)
{
    Page *page = this->findPage(pageName);
    PageItem *item = this->findPageItem(pageName, itemName);
    if(item)
    {
        return item->doAction(page);
    }
    return false;
}

static const char *fileName(const char *filePath)
{
    if(!filePath)
        return "";

    int len = (int)strlen(filePath);
    for(int ix = len; ix >= 0; ix--)
    {
        if(filePath[ix] == '/')
            return filePath + ix;
    }
    return filePath;
}

void OmWebPages::renderInfo(OmXmlWriter &w)
{
    w.addElement("hr");
    w.beginElement("pre");

    long long now;
    if(this->ri)
        now = this->ri->uptimeMillis;
    else
    {
#ifndef NOT_ARDUINO
        now= millis();
#else
        now = 101;
#endif
    }
    w.addContentF("requests:    %d\n", this->requestsAll);
    w.addContentF("maxHtml:     %d\n", this->greatestRenderLength);

#ifndef NOT_ARDUINO
    w.addContentF("uptime:      %s\n", omTime(now));
    w.addContentF("freeBytes:   %d\n", system_get_free_heap_size());
#endif
#ifdef ARDUINO_ARCH_ESP8266
    w.addContentF("chipId:      0x%08x '8266\n", system_get_chip_id());
    w.addContentF("systemSdk:   %s/%d\n", system_get_sdk_version(), system_get_boot_version());
#endif
#ifdef ARDUINO_ARCH_ESP32
    w.addContentF("chipId:      '32\n");
    w.addContentF("systemSdk:   %s\n", system_get_sdk_version());
#endif
#ifdef NOT_ARDUINO
    w.addContentF("what:        Not Arduino\n");
#endif
#ifndef NOT_ARDUINO
    if(this->ri)
    {
        w.addContentF("clientIp:    %s:%d\n", omIpToString(ri->clientIp, true), ri->clientPort);
        w.addContentF("serverIp:    <a href='http://%s:%d/'>%s:%d</a>\n",
                      omIpToString(ri->serverIp), ri->serverPort,
                      omIpToString(ri->serverIp), ri->serverPort);
        String bonjourName = ri->bonjourName;
        if(ri->bonjourName && ri->bonjourName[0])
            w.addContentF("bonjour:     <a href='http://%s.local/'>%s.local</a>\n", ri->bonjourName, ri->bonjourName);
        if(ri->ap && ri->ap[0])
            w.addContentF(    "accessPoint: %s\n", ri->ap);
        else
            w.addContentF(    "wifiNetwork: %s\n", ri->ssid);
    }
#endif
    w.addContentF("built:     %s %s\n", this->__date__, this->__time__);

    w.endElement();
}

void OmWebPages::renderDefaultFooter(OmXmlWriter &w)
{
    w.addElement("hr");
    for(auto page : this->pages)
    {
        this->renderPageButton(w, page->name);
    }
}

const char *colorItem = "#e0e0e0";
const char *colorHover = "#e0ffe0";
const char *colorButtonPress = "#707070";

void OmWebPages::renderStyle(OmXmlWriter &w, int bgColor)
{
    w.beginElement("style");
    w.addContent("*{font-family:arial}\n");
    // http://jkorpela.fi/forms/extraspace.html suggests margin:0 for forms to remove strange extra verticals.
    w.addContent(R"JS(
                 pre, pre a, .t {font-size:23px;font-family:Courier, monospace; word-wrap:break-word; overflow:auto; color:black}
                 form {margin-bottom:0px}
                 a:link { text-decoration: none;}
                 pre a:link {text-decoration: underline;}
                 .sliderValue { display:inline-block; font-size:16px; width:75px}
                 .colorValue { font-size:16px; margin-left:20px}
                 h1,h2,h3{text-align:center}
                 )JS"
    );

    w.addContentF(".box1,.box2,.button{font-size:30px; width:420px ; margin:10px; "
                 "padding:10px ; background:%s;"
                 "border-top-left-radius:15px;"
                 "border-bottom-right-radius:15px;"
                 "-webkit-user-select:none;"  // disable the touch-and-hold selection, it interferes with button pressing.
                 "}\n"
                 , colorItem
                 );
    // Not sure why I need to add more width to the button, to make it match
    w.addContent(".button{border:2px solid black;width:440px;display:block}\n");
    w.addContent(".box2{display:inline-block;"
                 "font-size:22px; padding:7px;"
                 "width:auto;overflow:hidden ; "
                 "margin-right:0px;margin-top:0px}\n"); // no right margin, so they space nicely in a row
    w.addContentF(".box1:hover,.box2:hover{background:%s;}\n", colorHover);
    w.addContent("body{width:470px;padding:5px;margin:0px;margin-top:5px}\n");
    
    // slider styling, from http://brennaobrien.com/blog/2014/05/style-input-type-range-in-every-browser.html
    
    w.addContent(
                 "input[type=range] { -webkit-appearance: none; border: 0px; } \n"
                 "input[type=range]::-webkit-slider-runnable-track { height: 5px; background: #663; border: none; border-radius: 3px; } \n"
                 "input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; border: none; height: 50px; width: 50px; border-radius: 10%; background: goldenrod; margin-top: -22px; } \n"
                 );

    w.addContentF("body { background-color: #%06x }", bgColor);
    w.endElement();
}

void OmWebPages::renderScript(OmXmlWriter &w)
{
    w.beginElement("script");

    /*
     * So, about live updates. Sliders and Color input types are defined to send input/oninput() events
     * as you drag the mouse, and change/onchange() when you release/commit. Experimentally, however,
     * I see on chrome that color picker sends oninput and onchange on every movement AND mouseup. And
     * on firefox, the color picker sends exclusively oninput, and never onchange.
     *
     * So, I treat change and input the same, and the timeout performs a commit on the last received
     * event of either type.
     */
    
    w.addContent(
R"JS(

    function reqListener ()
    {
        console.log(this.responseText)
    }

    var itemEmbargoes = [];
    function doOrEmbargo(itemName, commitProc)
    {
        // the commitProc should include the itemName, and commit the current (latest) value, and send the http update.
        if(!Boolean(itemEmbargoes[itemName]))
        {
            commitProc(); // nothing in-waiting, so do it now... and set a timeout to do it again with the latest value.
            itemEmbargoes[itemName] = setTimeout(function()
                                                 {
                                                     itemEmbargoes[itemName] = null; // clear the embargo
                                                     commitProc();
                                                 }, 100);
        }
    }
                 function clearEmbargo(itemName)
                 {
                     clearTimeout(itemEmbargoes[itemName]);
                     itemEmbargoes[itemName] = null;
                 }

    function colorInput(item, page, itemName)
    {
//        var textId = item.id + '_value';
//        var text = document.getElementById(textId);
//        text.style.color = '#a0a0a0'; // update in grey until actually committed
//        text.innerHTML = item.value;
        doOrEmbargo(itemName, function(){colorCommit(item, page, itemName);});
    }
    function sliderInput(slider, page, sliderName)
    {
        var textId = slider.id + '_value';
        var text = document.getElementById(textId);
        text.style.color = '#a0a0a0'; // update in grey until actually committed
        text.innerHTML = slider.value;
        doOrEmbargo(sliderName, function(){sliderCommit(slider, page, sliderName);});
    }

    function colorCommit(slider, page, sliderName)
    {
        clearEmbargo(sliderName);
        var textId = slider.id + '_value';
        var text = document.getElementById(textId);

        text.style.color = '#000000';
        text.innerHTML = slider.value;
        var oReq = new XMLHttpRequest();
        oReq.addEventListener('load', reqListener);

        var cv = '0x' + slider.value.substring(1);
        var uu = '/_control?page=' + page + '&item=' + sliderName + '&value=' + cv;
        oReq.open('GET', uu);
        oReq.send();
    }

    function sliderCommit(slider, page, sliderName)
    {
        clearEmbargo(sliderName);
        var textId = slider.id + '_value';
        var text = document.getElementById(textId);

        text.style.color = '#000000';
        text.innerHTML = slider.value;

        var oReq = new XMLHttpRequest();
        oReq.addEventListener('load', reqListener);

        oReq.open('GET', '/_control?page=' + page + '&item=' + sliderName + '&value=' + slider.value);
        oReq.send();
    }
    function timeChange(timeInput_nope, page, timeInputName)
    {
    //                  var textId = timeInput.id + '_value';
    //                  var text = document.getElementById(textId);

    //                  text.style.color = '#000000';
    //                  text.innerHTML = timeInput.value;

        var s = page + '_' + timeInputName;
        var timeInput = document.getElementById(s);
        var timeCheckbox = document.getElementById(s + '_checkbox');
        var oReq = new XMLHttpRequest();
        oReq.addEventListener('load', reqListener);

        var value = timeInput.value + '/' + (timeCheckbox.checked ? '1' : '0');
        oReq.open('GET', '/_control?page=' + page + '&item=' + timeInputName + '&value=' + value);
        oReq.send();
    }

    )JS"
    );


                 /*
                  * Getting the momentary button to work was tricky.
                  * On desktop, mousedown/up are fine. But on mobile
                  * Safari, they are both issued on finger-up from the button.
                  * So we use touchstart/end also, and disable mousedown/up
                  * if we see touch events. (So we dont get both pairs.)
                  */
    w.addContentF(
                 "var quellMouse = 0;\n"
                 "function button(button, page, buttonName, v) \n"
                 "{\n"
                 "if(v>=2)quellMouse=1;\n" // ignore mouses after first touch event
                 "if(quellMouse&&v<2)return;\n" // ignore the mouse.
                 "v&=1;\n" // mousedown/up is 1,0 and touchstart/end is 3,2
                 "button.style.backgroundColor=v?'%s':'%s';"
                 "var oReq = new XMLHttpRequest(); \n"
                 "oReq.addEventListener('load', reqListener); \n"
                 "oReq.open('GET', '/_control?page=' + page + '&item=' + buttonName + '&value=' + v); \n"
                 "oReq.send(); \n"
                 "}\n"
                 , colorButtonPress, colorItem
                 );
    w.endElement();
}

/// Render the beginning of the page, leaving <body> element open and ready.
void OmWebPages::renderPageBeginning(OmXmlWriter &w, const char *pageTitle, int bgColor)
{
    w.addContent("<!DOCTYPE html>\n");
    w.beginElement("html");
    w.addAttribute("lang", "en");
    w.beginElement("head");

    // discourage favicon requests with <link rel="icon" href="data:,">
    // as per https://stackoverflow.com/questions/1321878/how-to-prevent-favicon-ico-requests
    w.beginElement("link");
    w.addAttribute("rel", "icon");
    w.addAttribute("href", "data:,");
    w.endElement();
    
    w.beginElement("meta", "charset", "UTF-8");
    w.endElement();
    w.beginElement("meta");
    w.addAttribute("name", "viewport");
    w.addAttribute("content", "width=480");
    w.endElement();
    
    w.addElement("title", pageTitle);

    this->renderStyle(w, bgColor);
    this->renderScript(w);
    w.endElement();
    w.beginElement("body");
}

static void renderLink(OmXmlWriter &w, const char *pageName)
{
    w.beginElement("a");
    w.addAttributeUrlF("href", "/%s", pageName);
    
    w.beginElement("div");
    w.addAttribute("class", "box1");
    w.addContentF("%s", pageName);
    w.endElement();
    
    w.endElement();
}

void OmWebPages::renderTopMenu(OmXmlWriter &w)
{
    this->wp = &w;
    
    this->renderPageBeginning(w, "top");
    
    if(this->headerProc)
        this->headerProc(w, 0, 0);
    
    for(Page *page : this->pages)
    {
        if(page->name[0] != '_')
            renderLink(w, page->name);
    }
    w.addElement("hr");
    for(Page *page : this->pages)
    {
        if(page->name[0] == '_')
            renderLink(w, page->name);
    }
    
    if(this->footerProc)
        this->footerProc(w, 0, this);
    this->wp = 0;
}

void OmWebPages::renderPageButton(OmXmlWriter &w, const char *pageName)
{
    w.beginElement("a");
    w.addAttributeUrlF("href", "/%s", pageName);
    w.beginElement("span", "class", "box2");
    w.addContent(pageName);
    w.endElement();
    w.endElement();
}

Page *OmWebPages::findPage(const char *pageName)
{
    Page *p = 0;
    for(Page *page : this->pages)
    {
        if(omStringEqual(pageName, page->name))
        {
            p = page;
            break;
        }
    }
    return p;
}

PageItem *OmWebPages::findPageItem(const char *pageName, const char *itemName)
{
    Page *page = this->findPage(pageName);
    if(page)
    {
        for(PageItem *b : page->items)
        {
            if(omStringEqual(itemName, b->name))
                return b;
        }
    }
    return NULL;
}

static int stringToIntMaybeTimeString(const char *valueS)
{
    int value;
    // if it's a time string, do minutes of day.
    if(strlen(valueS) == 7 && valueS[2] == ':' && valueS[5] == '/')
    {
        int hour = (valueS[0] & 0xf) * 10 + (valueS[1] & 0xf);
        int minute = (valueS[3] & 0xf) * 10 + (valueS[4] & 0xf);
        int checked = (valueS[6] & 1); // ends with /0 or /1 for is the alarm set.
        value = hour * 60 + minute;
        if(checked)
            value |= 0x8000;
    }
    else
        value = omStringToInt(valueS);
    return value;
}

bool OmWebPages::handleRequest(OmIByteStream *consumer, const char *pathAndQuery, OmRequestInfo *requestInfo)
{
    bool result = false;
    OmXmlWriter w = OmXmlWriter(consumer);
    Page *page = 0;

    this->wp = &w;
    this->ri = requestInfo;

    this->requestsAll++;
    static OmWebRequest request;
    request.init(pathAndQuery);
    const char *requestPath = request.path;

    if(this->pages.size() == 0)
    {
        this->renderHttpResponseHeader("text/html", 200);
        w.addElement("hr");
        w.beginElement("pre");
        w.addContent("OmWebPages\n");
        w.addContent("No pages, try omWebPages.beginPage(\"pageName\")\n");
        w.addContent("              omWebPages.addButton(\"buttonName\")\n");
        w.addContent("\n");
        w.endElement();
        w.addElement("hr");
        result = false;
        goto goHome;
    }

    {
        // Do the previous action, if any.
        const char *pageName = request.getValue("page");
        const char *itemName = request.getValue("item");
        const char *valueS = request.getValue("value");
        
        bool setParam = false;
        if(valueS)
        {
            // if it's a time string, do minutes of day.
            int value = stringToIntMaybeTimeString(valueS);
            PageItem *item = this->findPageItem(pageName, itemName);
            if(item)
            {
                item->setValue(value);
                setParam = true;
            }
        }
        
        this->doAction(pageName, itemName);
        
        if(omStringEqual("/_control", requestPath))
        {
            this->requestsParam++;
            this->renderHttpResponseHeader("text/html", 200);
            w.addContentF("%s", setParam ? "ok" : "notok");
            result = true;
            goto goHome;
        }
    }
    
    // the URLs are just the page name with a leading slash. so.
    requestPath++;
    page = this->findPage(requestPath);
    
    // Empty path is "the home page"
    if(!page && omStringEqual(requestPath, ""))
        page = this->homePage;

    // Take a pocket-universe detour to maybe do a plain old URL handler.
    if(!page)
    {
        for(UrlHandler *uh : this->urlHandlers)
        {
            if(omStringEqual(requestPath, uh->url))
            {
                // found a handler.
                // Handler must do their own http response header.
                uh->handlerProc(w, request, uh->ref1, uh->ref2);
                result = true;
                goto goHome;
            }
        }
    }

    if(!page)
    {
        OMLOG("no page found, just do top menu");
        this->renderTopMenu(w);
        result = false;
        goto goHome;
    }

    // Do a page, starting with the response headers.
    this->renderHttpResponseHeader("text/html", 200);
    this->renderPageBeginning(w, page->name, this->bgColor);
    w.addContent("\n");
    
    if(this->headerProc && page->allowHeader)
        this->headerProc(w, 0, 0);
    
    {
        w.beginElement("h2");
        w.addContent(page->name);
        w.endElement();
        w.addContent("\n");
    }
    
    for(PageItem *b : page->items)
    {
        if(b->visible)
        {
            b->render(w, page);
            w.addContent("\n");
        }
    }
    
    if(this->footerProc && page->allowFooter)
        this->footerProc(w, 0, this);
    
    w.addContent("\n");
    w.endElements();
    result = true;
    

goHome:
    if(w.getByteCount() > this->greatestRenderLength)
        this->greatestRenderLength = w.getByteCount();
    if(w.getErrorCount())
    {
        // TODO: consider showing the partial page anyways? dvb2019
        w.addContent("\n\nerror generating page\n\n");
    }
    this->ri = 0;
    this->wp = 0;
    return result;
}

void OmWebPages::setBgColor(int bgColor)
{
    this->bgColor = bgColor;
}

void OmWebPages::addUrlHandler(const char *path, OmUrlHandlerProc proc, int ref1, void *ref2)
{
    UrlHandler *uh = new UrlHandler();
    // skip leading slash, it's implied throughout in our code here.
    if(path && path[0] == '/')
        path++;
    uh->url = path;
    uh->handlerProc = proc;
    uh->ref1 = ref1;
    uh->ref2 = ref2;
    this->urlHandlers.push_back(uh);
}

void OmWebPages::renderHttpResponseHeader(const char *contentType, int response)
{
    this->wp->addContentF("HTTP/1.1 %d OK\n"
                   "Content-type:%s\n"
                   "Connection: close\n"
                   "\n", response, contentType);
}

void OmWebPages::setValue(const char *pageName, const char *itemName, int value)
{
    PageItem *pi = this->findPageItem(pageName, itemName);
    if(pi)
        pi->setValue(value);
}
