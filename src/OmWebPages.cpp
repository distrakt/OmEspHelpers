/*
 * OmWebPages.cpp
 * 2016-11-20
 *
 * Implementation of OmWebPages
 */

#include "OmWebPages.h"

#ifndef NOT_ARDUINO
#include "OmWebServer.h"
extern "C" {
#include "user_interface.h" // system_free_space and others.
}
#endif

class PageItem
{
public:
    const char *name = "";
    const char *displayName = "";
    const char *pageLink = "";
    int ref1 = 0;
    void *ref2 = 0;
    int value = 0;
    
    virtual void render(OmXmlWriter &w, Page *inPage) = 0;
    virtual bool doAction(Page *fromPage) = 0;
    void setValue(int value) { this->value = value; }
};


class Page
{
public:
    const char *name = "";
    const char *displayName = "";
    std::vector<PageItem *> items;
    
    // set to false to inhibit this pages's default header or footer.
    // replace with different via Html items if you like.
    bool allowHeader = true;
    bool allowFooter = true;
    
    void renderTree(OmXmlWriter &w)
    {
        for(PageItem *button : this->items)
        {
            w.addContent("    ");
            
            w.beginElement("a");
            w.addAttributeUrlF("href", "/%s?page=%s&item=%s", button->pageLink, this->name, button->name);
            w.addContentF("%s", button->name);
            w.endElement();
            
            w.addContent("\n");
        }
    }
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
        w.addContentF("%d", this->value);
        w.endElement(); // span
        w.beginElement("input", "type", "range");
        w.addAttributeF("value", "%d", this->value);
        w.addAttribute("style", "width: 300px");
        w.addAttributeF("id", "%s_%s", inPage->name, this->name);
        w.addAttributeF("onchange", "sliderChange(this,'%s', '%s')", inPage->name, this->name);
        w.addAttributeF("oninput", "sliderSlide(this,'%s','%s')", inPage->name, this->name);
        w.endElement(); // input
        w.endElement(); // form
        w.addElement("br");
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

void OmWebPages::beginPage(const char *pageName, const char *pageDisplayName)
{
    Page *page = new Page();
    page->name = pageName;
    page->displayName = pageDisplayName;
    this->pages.push_back(page);
    this->currentPage = page;
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
    b->displayName = pageLink;
    b->pageLink = pageLink;
    b->proc = proc;
    b->ref1 = ref1;
    b->ref2 = ref2;
    this->currentPage->items.push_back(b);
}

void OmWebPages::addButton(const char *buttonName, OmWebActionProc proc, int ref1, void *ref2)
{
    PageButton *b = new PageButton();
    b->name = buttonName;
    b->displayName = buttonName;
    b->proc = proc;
    b->ref1 = ref1;
    b->ref2 = ref2;
    this->currentPage->items.push_back(b);
}

void OmWebPages::addSlider(const char *sliderName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    PageSlider *p = new PageSlider();
    p->name = sliderName;
    p->displayName = sliderName;
    p->value = value;
    p->ref1 = ref1;
    p->ref2 = ref2;
    p->proc = proc;
    this->currentPage->items.push_back(p);
}

void OmWebPages::addHtml(HtmlProc proc, int ref1, void *ref2)
{
    HtmlItem *h = new HtmlItem();
    h->name = "_";
    h->displayName = h->name;
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

void OmWebPages::renderTree(OmXmlWriter &w)
{
    this->renderPageBeginning(w, "(tree)");
    w.beginElement("pre");
    for(Page *page : this->pages)
    {
        w.beginElement("b");
        w.beginElement("a");
        w.addAttributeUrlF("href", "/%s", page->name);
        w.addContentF("%s", page->name);
        w.endElement();
        w.endElement();
        w.addContent("\n");
        
        page->renderTree(w);
    }
    w.endElements();
}


void OmWebPages::renderInfo(OmXmlWriter &w)
{
    this->renderPageBeginning(w, "(tree)");
    w.addElement("h1", "_info");
    
    w.addElement("hr");
    w.beginElement("pre");

    w.addContentF("requests:    %d\n", this->requestsAll);
    w.addContentF("paramReqs:   %d\n", this->requestsParam);
    w.addContentF("longestHtml: %d\n", this->greatestRenderLength);

#ifndef NOT_ARDUINO
    w.addContentF("uptime:      %s\n", omTime(millis()));
    w.addContentF("freeBytes:   %d\n", system_get_free_heap_size());
    w.addContentF("chipId:      0x%08x\n", system_get_chip_id());
    w.addContentF("systemSdk:   %s[%d]\n", system_get_sdk_version(), system_get_boot_version());
    
    if(this->omWebServer)
    {
        w.addContentF("ticks:       %d\n", this->omWebServer->getTicks());
        w.addContentF("serverIp:    %s:%d\n", omIpToString(this->omWebServer->getIp()), this->omWebServer->getPort());
        w.addContentF("clientIp:    %s:%d\n", omIpToString(this->omWebServer->getClientIp()), this->omWebServer->getClientPort());
    }
#endif
    
    w.endElement();
    w.addElement("hr");
    
    w.endElements();
}


const char *colorItem = "#e0e0e0";
const char *colorHover = "#e0ffe0";
const char *colorButtonPress = "#707070";

void OmWebPages::renderStyle(OmXmlWriter &w)
{
    w.beginElement("style");
    w.addAttribute("type", "text/css");
    w.addContent("*{font-family:arial}\n");
    w.addContent("pre, pre a{font-size:24px;font-family:Courier, monospace; word-wrap:break-word; overflow:auto; color:black}\n"
                 "a:link { text-decoration: none;}\n"
                 "pre a:link {text-decoration: underline;}\n"
                 ".sliderValue { display:inline-block; width:100px}\n"
                 );
    w.addContent("h1,h2,h3{text-align:center}\n");
    w.addContentF(".box1,.box2,.button{font-size:30px; width:420px ; margin:10px; "
                 "padding:10px ; background:%s;"
                 "border-top-left-radius:15px;"
                 "border-bottom-right-radius:15px;"
                 "-webkit-user-select:none;"  // disable the touch-and-hold selection, it interferes with button pressing.
                 "}\n"
                 , colorItem
                 );
    // Not sure why I need to add more width to the button, to make it match
    w.addContent(".button{borderz:2px solid black;width:440px;display:block}\n");
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
    w.endElement();
}

void OmWebPages::renderScript(OmXmlWriter &w)
{
    w.beginElement("script");
    
    
    w.addContentF(
                 "function reqListener () \n"
                 "{ console.log(this.responseText) } \n;"
                 
                 "function sliderSlide(slider, page, sliderName) \n"
                 "{ \n"
                 "var textId = slider.id + '_value'; \n"
                 "var text = document.getElementById(textId); \n"
                 "text.style.color = '#a0a0a0'; \n"
                 "text.innerHTML = slider.value; \n"
#if 0
                  // sending every slider nudge ends up queueing a lot of
                  // messages and adding delay. TODO: do some sort of rate limiting, but
                  // always including the lastly paused spot.
                  "var oReq = new XMLHttpRequest(); \n"
                  "oReq.addEventListener('load', reqListener); \n"
                  "oReq.open('GET', '/_control?page=' + page + '&item=' + sliderName + '&value=' + slider.value); \n"
                  "oReq.send(); \n"
#endif
                 "} \n"
                  );
    w.addContentF(
                 
                 "function sliderChange(slider, page, sliderName) \n"
                 "{ \n"
                 "var textId = slider.id + '_value'; \n"
                 "var text = document.getElementById(textId); \n"
                 
                 "text.style.color = '#000000'; \n"
                 "text.innerHTML = slider.value; \n"
                 
                 "var oReq = new XMLHttpRequest(); \n"
                 "oReq.addEventListener('load', reqListener); \n"

                 "oReq.open('GET', '/_control?page=' + page + '&item=' + sliderName + '&value=' + slider.value); \n"
                 "oReq.send(); \n"
                 "} \n"
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
void OmWebPages::renderPageBeginning(OmXmlWriter &w, const char *pageTitle)
{
    w.beginElement("html");
    w.beginElement("head");
    
    w.beginElement("meta", "charset", "UTF-8");
    w.endElement();
    w.beginElement("meta");
    w.addAttribute("name", "viewport");
    w.addAttribute("content", "width=480");
    w.endElement();
    
    w.addElement("title", pageTitle);

    this->renderStyle(w);
    this->renderScript(w);
    w.endElement();
    w.beginElement("body");
}

void OmWebPages::renderTopMenu(OmXmlWriter &w)
{
    this->wp = &w;
    
    this->renderPageBeginning(w, "top");
    
    if(this->headerProc)
        this->headerProc(w, 0, 0);
    
    for(Page *page : this->pages)
    {
        w.beginElement("a");
        w.addAttributeUrlF("href", "/%s", page->name);
        
        w.beginElement("div");
        w.addAttribute("class", "box1");
        w.addContentF("%s", page->name);
        w.endElement();
        
        w.endElement();
    }
    if(this->footerProc)
        this->footerProc(w, 0, 0);
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

bool OmWebPages::handleRequest(char *output, int outputSize, const char *pathAndQuery)
{
    OmXmlWriter w(output, outputSize);
    output[0] = 0; // safety.

    w.addContent("<!DOCTYPE html>\n");
    if(this->pages.size() == 0)
    {
        w.addElement("hr");
        w.beginElement("pre");
        w.addContent("OmWebPages\n");
        w.addContent("No pages, try omWebPages.beginPage(\"pageName\")\n");
        w.addContent("              omWebPages.addButton(\"buttonName\")\n");
        w.addContent("\n");
        w.endElement();
        w.addElement("hr");
        return false;
    }

    Page *page = 0;
    
    
    bool result = false;
    this->requestsAll++;
    static OmWebRequest request;
    request.init(pathAndQuery);
    const char *requestPath = request.path;
    
    {
        // Do the previous action, if any.
        const char *pageName = request.getValue("page");
        const char *itemName = request.getValue("item");
        const char *valueS = request.getValue("value");
        
        bool setParam = false;
        if(valueS)
        {
            int value = omStringToInt(valueS);
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
            sprintf(output, "%s", setParam ? "ok" : "notok");
            result = true;
            goto goHome;
        }
    }
    
    // the URLs are just the page name with a leading slash. so.
    requestPath++;
    page = this->findPage(requestPath);
    
    if(!page && omStringEqual(requestPath, "_tree"))
    {
        this->renderTree(w);
        result = true;
        goto goHome;
    }
    else if(!page && omStringEqual(requestPath, "_info"))
    {
        this->renderInfo(w);
        result = true;
        goto goHome;
    }
    
    if(!page)
    {
        this->renderTopMenu(w);
        result = false;
        goto goHome;
    }
    
    this->renderPageBeginning(w, page->name);
    w.addContent("\n");
    
    this->wp = &w;
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
        b->render(w, page);
        w.addContent("\n");
    }
    
    if(this->footerProc && page->allowFooter)
        this->footerProc(w, 0, 0);
    
    w.addContent("\n");
    w.endElements();
    result = true;
    
    this->wp = 0;
    
goHome:
    if(w.position > greatestRenderLength)
        this->greatestRenderLength = w.position;
    if(w.getErrorCount())
    {
        sprintf(output, "error generating page");
    }
    return result;
}

void OmWebPages::setOmWebServer(OmWebServer *omWebServer)
{
    this->omWebServer = omWebServer;
}
