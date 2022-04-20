/*
 * OmWebPages.cpp
 * 2016-11-20
 *
 * Implementation of OmWebPages
 */

#include "OmWebPages.h"
#include "OmLog.h"
#include "OmEeprom.h"

#ifdef NOT_ARDUINO
#include "EepromTesting.h"
#endif

#ifndef NOT_ARDUINO
#include "Arduino.h"
#include "OmNtp.h"

#ifdef ARDUINO_ARCH_ESP8266
#include "user_interface.h" // system_free_space and others.
#include "ESP8266WiFi.h"
#endif

#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal.h"
#include "esp_system.h"
#include "WiFi.h"
#endif

#endif

// with flexible procs and interfaces, parameters are often optionally used.
#pragma GCC diagnostic ignored "-Wunused-parameter"

OmWebPages OmWebPagesSingleton;

class PageItem
{
public:
    const char *name = "";
    const char *url = "_";
    char id[6]; // name is the user-facing name. id is a unique label like i0 or i22.
    const char *pageLink = "";
    int ref1 = 0;
    void *ref2 = 0;
    int value = 0;
    bool visible = true;
    
    virtual void render(OmXmlWriter &w, Page *inPage, bool inBox = true) = 0;
    virtual bool doAction(Page *fromPage) = 0;
    void setValue(int value) { this->value = value; }

    virtual void renderStatusey(OmXmlWriter &w)=0;// {};

    OmWebPageItem item;
    PageItem() : item(this)
    {
        return;
    }

    virtual ~PageItem()
    {
        return;
    }

protected:
    void maybeBox1Start(OmXmlWriter &w, bool inBox)
    {
        if(inBox)
        {
            w.beginElement("div", "class", "box1");
            w.addContent(this->name);
        }
        else
            w.beginElement("div");
    }
    void maybeBox1End(OmXmlWriter &w, bool inBox)
    {
//        if(inBox)
        {
            w.endElement("div");
        }
    }

    void rejiggerUrl(const char * &urlInOut, const char * &maybeUrlBaseOut)
    {
        // Some page items support going to a new URL on action.
        // this shared code helps set it up, in two parts, base & url
        if(urlInOut == NULL) // we allow '' to mean reload same page
            urlInOut = "_";
        else
        {
            // handle % to mean ip address up front
            if(urlInOut[0] == '%')
            {
                urlInOut += 1;
                maybeUrlBaseOut = OmWebPages::httpBase;
            }
        }
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
    ~Page()
    {
        this->clearPage();
    }

    void clearPage()
    {
        for(PageItem *item : this->items)
        {
            if(item)
                delete item;
        }
        this->items.clear();
    }

    const char *name = "";
    bool listed;
    char id[6]; // simple mechanical id like p0 or p23.
    /// this proc gets called before renderind. It could rebuild the whole page "just in case" for example.
    OmWebActionProc arrivalAction = NULL;
    int arrivalActionRef1 = 0;
    void *arrivalActionRef2 = 0;

    std::vector<PageItem *> items;
    
    // set to false to inhibit this pages's default header or footer.
    // replace with different via Html items if you like.
    bool allowHeader = true;
    bool allowFooter = true;
    void addItem(PageItem *item)
    {
        sprintf(item->id, "i%d", (int)this->items.size());
        this->items.push_back(item);
    }
};

class PageLink : public PageItem
{
public:
    OmWebActionProc proc = 0;
    bool mini = false;
    
    void render(OmXmlWriter &w, Page *inPage, bool inBox) override
    {
        w.beginElement("a");
        w.addAttributeUrlF("href", "/%s?page=%s&item=%s", this->pageLink, inPage->id, this->id);
        
        w.beginElement("div");
        w.addAttribute("class", this->mini ? "box2" : "box1");
        w.addContentF("%s", this->name);
        w.endElement();
        
        w.endElement();
    }
    
    bool doAction(Page *fromPage) override
    {
        if(this->proc)
        {
            this->proc(fromPage->name, this->id, 0, this->ref1, this->ref2);
            return true;
        }
        else
        {
            return false;
        }
    }

    void renderStatusey(OmXmlWriter &w) override
    {
        w.addAttribute("kind", "pageLink");
    }
};

/// Show a visual slider, which call back on changes
class PageSlider : public PageItem
{
public:
    OmWebActionProc proc = 0;
    int min = 0;
    int max = 100;
    
    void render(OmXmlWriter &w, Page *inPage, bool inBox) override
    {
        /*
         <form>
         <span id="id1" style="display:inline-block;width:100px">100</span>
         <input id="id2" type="range" onchange="aChange(this, 'id1')"
         oninput="aSlide(this, 'id1')" />
         
         <input id="id3" type="range" onchange="aChange(this, this.value)" />
         </form>
         */
        this->maybeBox1Start(w, inBox);
        w.beginElement("form");
        w.beginElement("span", "class", "sliderValue");
        w.addAttribute("style", "margin-bottom:15px");
        w.addAttributeF("id", "%s_%s_value", inPage->id, this->id);
        w.addContentF("%d", this->value);
        w.endElement(); // span
        w.beginElement("input", "type", "range");
        w.addAttributeF("value", "%d", this->value);
        w.addAttribute("style", "width: 330px");
        w.addAttributeF("id", "%s_%s", inPage->id, this->id);
        w.addAttributeF("onchange", "sliderInput(this,'%s', '%s')", inPage->id, this->id);
        w.addAttributeF("oninput", "sliderInput(this,'%s','%s')", inPage->id, this->id);
        if(this->min != 0)
            w.addAttribute("min", this->min);
        if(this->max != 100)
            w.addAttribute("max", this->max);
        w.endElement(); // input
        w.endElement(); // form
        this->maybeBox1End(w, inBox);
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

    void renderStatusey(OmXmlWriter &w) override
    {
        w.addAttribute("kind", "slider");
        w.addAttribute("min", this->min);
        w.addAttribute("max", this->max);
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

    void render(OmXmlWriter &w, Page *inPage, bool inBox) override
    {
        /*
         <form>
         <span id="id1" style="display:inline-block;width:100px">100</span>
         <input id="id2" type="range" onchange="aChange(this, 'id1')"
         oninput="aSlide(this, 'id1')" />

         <input id="id3" type="range" onchange="aChange(this, this.value)" />
         </form>
         */
        this->maybeBox1Start(w, inBox);
        w.beginElement("form");
//        w.beginElement("span", "class", "sliderValue");
//        w.addAttributeF("id", "%s_%s_value", inPage->name, this->name);
////        w.addContentF("%s", minutesToTimeString(this->value));
//        w.endElement(); // span
        w.beginElement("input", "type", "time");
        w.addAttributeF("value", "%s", minutesToTimeString(this->value));
        w.addAttributeF("id", "%s_%s", inPage->id, this->id);
        w.addAttributeF("onchange", "timeChange(this,'%s', '%s')", inPage->id, this->id);
        w.endElement(); // input
        w.beginElement("input", "type", "checkbox");
        w.addAttributeF("id", "%s_%s_checkbox", inPage->id, this->id);
        if(this->value & 0x8000)
            w.addAttribute("checked", "checked");
        w.addAttributeF("onchange", "timeChange(this,'%s', '%s')", inPage->id, this->id);
        w.endElement();
        w.endElement(); // form
        this->maybeBox1End(w, inBox);
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

    void renderStatusey(OmXmlWriter &w) override
    {
        w.addAttribute("kind", "time");
    }
};

class PageSelect : public PageItem
{
public:
    OmWebActionProc proc = 0;
    std::vector<const char*> optionNames;
    std::vector<int> optionNumbers;
    const char *url = "_";

    void addOption(const char *optionName, int optionNumber)
    {
        this->optionNames.push_back(optionName);
        this->optionNumbers.push_back(optionNumber);
    }

    void render(OmXmlWriter &w, Page *inPage, bool inBox) override
    {
        /*
         <select id="" onchange="">
         <option value="23">dogs</option>
         <option value="42">cats</option>
         </select>

         */
        const char *maybeUrlBase = "";
        const char *url = this->url;
        this->rejiggerUrl(url, maybeUrlBase);

        this->maybeBox1Start(w, inBox);
        w.beginElement("select");
        w.addAttributeF("id", "%s_%s", inPage->id, this->id);
        w.addAttributeF("onchange", "selectChange(this,'%s', '%s', '%s%s')", inPage->id, this->id, maybeUrlBase, url);

        bool foundSelectedOption = false;
        for(unsigned int ix = 0; ix < this->optionNumbers.size(); ix++)
        {
            w.beginElement("option");
            int optionNumber = this->optionNumbers[ix]; // the integer assigned to this menu choice
            w.addAttributeF("value", "%d", optionNumber);
            if(this->value == optionNumber)
            {
                w.addAttribute("selected", "selected");
                foundSelectedOption = true;
            }
            w.addContent(this->optionNames[ix]);
            w.endElement();
        }

        // if somehow the current value isn't one of the choices, add it here to show ya.
        if(!foundSelectedOption)
        {
            w.beginElement("option");
            w.addAttribute("selected", "selected");
            w.addAttribute("disabled", "disabled");
            w.addContentF("(%d)", this->value);
            w.endElement();
        }

        w.endElement(); // select
        w.beginElement("span", "class", "selectValue");
        w.addAttributeF("id", "%s_%s_value", inPage->id, this->id);
        w.addContentF(" %d", this->value);
        w.endElement(); // span
        this->maybeBox1End(w, inBox);
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

    void renderStatusey(OmXmlWriter &w) override
    {
        w.addAttribute("kind", "pageSelect");
    }
};

static const char *intToString(int x)
{
    static char s[20];
    sprintf(s, "%d", x);
    return s;
}

class PageCheckboxes : public PageItem
{
public:
    OmWebActionProc proc = 0;
    std::vector<String> checkboxNames; // checkbox values start at bit 0 and work up.

    void addCheckbox(String checkboxName, int value)
    {
        // add the boolean value for this checkbox. later ones are higher bits.
        value = value ? 1 : 0;
        value <<= this->checkboxNames.size();
        this->value |= value;

        this->checkboxNames.push_back(checkboxName);
    }

    void render(OmXmlWriter &w, Page *inPage, bool inBox) override
    {
        /*
         <select id="" onchange="">
         <option value="23">dogs</option>
         <option value="42">cats</option>
         </select>

         */
        this->maybeBox1Start(w, inBox);

        // list of all the checkboxes in this group, like "controls_components_checkbox_1,controls_components_checkbox_2,controls_components_checkbox_3"
        String checkboxesAll;
        for(unsigned int ix = 0; ix < this->checkboxNames.size(); ix++)
        {
            if(ix)
                checkboxesAll += ",";
            checkboxesAll += inPage->id;
            checkboxesAll += "_";
            checkboxesAll += this->id;
            checkboxesAll += "_checkbox_";
            checkboxesAll += intToString(ix);
        }
        uint32_t bit = 1;
        for(unsigned int ix = 0; ix < this->checkboxNames.size(); ix++)
        {
            w.addElement("br");
            w.beginElement("input");
            w.addAttributeF("id", "%s_%s_checkbox_%d", inPage->id, this->id, ix);
            w.addAttribute("type", "checkbox");
            w.addAttributeF("value", "%d", bit);
            if(bit & this->value)
                w.addAttribute("checked", "checked");
            w.addAttributeF("onchange", "checkboxChange('%s', '%s', '%s')", inPage->id, this->id, checkboxesAll.c_str());
            w.endElement("input");
            w.beginElement("span", "class", "checkboxLabel");
            w.addContent(this->checkboxNames[ix].c_str());
            w.endElement();
            bit = bit + bit;
        }
        w.beginElement("span", "class", "selectValue");
        w.addAttributeF("id", "%s_%s_value", inPage->id, this->id);
        w.addContentF(" %d", this->value);
        w.endElement(); // span
        this->maybeBox1End(w, inBox);
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

    void renderStatusey(OmXmlWriter &w) override
    {
        w.addAttribute("kind", "checkboxes");
        String s = "";
        for(auto cn : this->checkboxNames)
        {
            if(s.length()) s += ",";
            s += cn;
        }
        w.addAttribute("checkboxNames", s.c_str());
    }
};

class PageColor : public PageItem
{
public:
    OmWebActionProc proc = 0;

    void render(OmXmlWriter &w, Page *inPage, bool inBox) override
    {
        /*
         <form>
         <span id="id1" style="display:inline-block;width:100px">100</span>
         <input id="id2" type="range" onchange="aChange(this, 'id1')"
         oninput="aSlide(this, 'id1')" />

         <input id="id3" type="range" onchange="aChange(this, this.value)" />
         </form>
         */
        this->maybeBox1Start(w, inBox);
        w.beginElement("form");
        w.beginElement("input", "type", "color");
        w.addAttributeF("value", "#%06x", this->value);
        w.addAttributeF("id", "%s_%s", inPage->id, this->id);
        // change just like a slider -- it's a numeric value... ish.
        w.addAttributeF("onchange", "colorInput(this,'%s', '%s')", inPage->id, this->id);
        w.addAttributeF("oninput", "colorInput(this,'%s', '%s')", inPage->id, this->id);
        w.endElement(); // input

        w.beginElement("span", "class", "colorValue");
        w.addAttributeF("id", "%s_%s_value", inPage->id, this->id);
        w.addContentF(" #%06x", this->value);
        w.endElement(); // span

        w.endElement(); // form
        this->maybeBox1End(w, inBox);
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

    void renderStatusey(OmXmlWriter &w) override
    {
        w.addAttribute("kind", "color");
    }
};


class PageButton : public PageItem
{
public:
    OmWebActionProc proc = 0;
    const char *url = "_";

    void render(OmXmlWriter &w, Page *inPage, bool inBox) override
    {
        const char *maybeUrlBase = "";
        const char *url = this->url;
        this->rejiggerUrl(url, maybeUrlBase);

        w.beginElement("button");
        w.addAttribute("class", "button");
        // We send both mouse and touch events; the event handler may then suppress mouse events
        w.addAttributeF("onmousedown", "button(this,'%s', '%s', 1, '%s%s')", inPage->id, this->id, maybeUrlBase, url);
        w.addAttributeF("onmouseup", "button(this,'%s', '%s', 0, '%s%s')", inPage->id, this->id, maybeUrlBase, url);
        // w3c validator says having mousedown AND touchstart is an error. But it makes it all work. So. :-/
        w.addAttributeF("ontouchstart", "button(this,'%s', '%s', 3, '%s%s')", inPage->id, this->id, maybeUrlBase, url);
        w.addAttributeF("ontouchend", "button(this,'%s', '%s', 2, '%s%s')", inPage->id, this->id, maybeUrlBase, url);
        w.addContentF("%s", this->name);
        w.endElement();
    }

    void renderStatusey(OmXmlWriter &w) override
    {
        w.addAttribute("kind", "button");
        if(!omStringEqual(this->url, "_"))
            w.addAttribute("url", this->url);
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
    OmHtmlProc proc;
    
    void render(OmXmlWriter &w, Page *inPage, bool inBox) override
    {
        UNUSED(inPage);
        if(this->proc)
            this->proc(w, this->ref1, this->ref2);
    }
    bool doAction(Page *fromPage) override
    {
        return true;
    }

    void renderStatusey(OmXmlWriter &w) override
    {
        w.addAttribute("kind", "html");
    }
};

class StaticHtmlItem : public PageItem
{
public:
    String staticHtml;

    void render(OmXmlWriter &w, Page *inPage, bool inBox) override
    {
        UNUSED(inPage);
        w.addRawContent(this->staticHtml.c_str());
    }
    void renderStatusey(OmXmlWriter &w) override
    {
        w.addAttribute("kind", "htmlStatic");
    }
    bool doAction(Page *fromPage) override
    {
        return true;
    }
};

void infoHtmlProc(OmXmlWriter &w, int ref1, void *ref2)
{
    UNUSED(ref1);
    OmWebPages *owp = (OmWebPages *)ref2;
    owp->renderInfo(w);
}

void statusXmlProc(OmXmlWriter &w, OmWebRequest &request, int ref1, void *ref2)
{
    OmWebPages *owp = (OmWebPages *)ref2;
    owp->renderStatusXml(w);
}

void defaultFooterHtmlProc(OmXmlWriter &w, int ref1, void *ref2)
{
    OmWebPages *owp = (OmWebPages *)ref2;
    owp->renderDefaultFooter(w);
}

OmWebPages::OmWebPages()
{
    OmWebPages::p = this; // reference to last one made. really, the only one.
    this->__date__ = __DATE__;
    this->__time__ = __TIME__;
    this->__file__ = NULL;
    // create the built-in pages
    this->beginPage("_info");
    this->addHtml(infoHtmlProc, 0, this);
    
    // default navigation buttons
    this->setFooterProc(defaultFooterHtmlProc);
    
    // Make sure none of the built-in pages become the user's home page.
    this->homePage = NULL;

    // Add the poll-able xml status page, to allow local discover.
    this->addUrlHandler("_status", statusXmlProc, 0, this);
}

OmWebPages::~OmWebPages()
{
    for(Page *page : this->pages)
    {
        if(page)
            delete page;
    }
    this->pages.clear();
    
    for(UrlHandler *handler : this->urlHandlers)
    {
        delete handler;
    }
    this->urlHandlers.clear();
}

void OmWebPages::setBuildDateAndTime(const char *date, const char *time, const char *file)
{
    this->__date__ = date;
    this->__time__ = time;
    this->__file__ = file;

    if(this->__file__)
    {
        // point past last slash
        const char *w = this->__file__;
        while(*w != 0)
        {
            if(*w == '/')
                this->__file__ = w + 1;
            w++;
        }
    }
}

void *OmWebPages::beginPage(const char *pageName, bool listed)
{
    // if a page with this name already exists, clear out its contents so
    // you can replace them with new elements.
    void *oldPage = this->currentPage;
    for(Page *aPage : this->pages)
    {
        if(omStringEqual(pageName, aPage->name))
        {
            aPage->clearPage();
            this->currentPage = aPage;
            aPage->listed = listed;
            goto goHome;
        }
    }

    // It is indeed a brand new page. Let's create it.
    {
        Page *page = new Page();
        page->name = pageName;
        page->listed = listed;
        sprintf(page->id, "p%d", (int)this->pages.size());
        this->pages.push_back(page);
        this->currentPage = page;
        if(!this->homePage)
            this->homePage = page;
    }

goHome:
    return oldPage;
}

void OmWebPages::setPageArrivalAction(OmWebActionProc arrivalAction, int ref1, void *ref2)
{
    if(this->currentPage)
    {
        this->currentPage->arrivalAction = arrivalAction;
        this->currentPage->arrivalActionRef1 = ref1;
        this->currentPage->arrivalActionRef2 = ref2;
    }
}

void OmWebPages::resumePage(void *previousPage)
{
    this->currentPage = (Page *)previousPage;
}


void OmWebPages::allowHeader(bool allowHeader)
{
    this->currentPage->allowHeader = allowHeader;
}

void OmWebPages::allowFooter(bool allowFooter)
{
    this->currentPage->allowFooter = allowFooter;
}

void OmWebPages::setHeaderProc(OmHtmlProc headerProc)
{
    this->headerProc = headerProc;
}

void OmWebPages::setFooterProc(OmHtmlProc footerProc)
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
    this->currentPage->addItem(b);
}

void OmWebPages::addPageLinkMini(const char *pageLink, const char *label)
{
    PageLink *b = new PageLink();
    b->name = label;
    b->pageLink = pageLink;
    b->mini = true;
    this->currentPage->addItem(b);
}

OmWebPageItem *OmWebPages::addButton(const char *buttonName, OmWebActionProc proc, int ref1, void *ref2)
{
    PageButton *b = new PageButton();
    b->name = buttonName;
    b->proc = proc;
    b->ref1 = ref1;
    b->ref2 = ref2;
    this->currentPage->addItem(b);
    return &b->item;
}

OmWebPageItem *OmWebPages::addButtonWithLink(const char *buttonName, const char *url, OmWebActionProc proc, int ref1, void *ref2)
{
    OmWebPageItem *item = this->addButton(buttonName, proc, ref1, ref2);
    PageButton *b = (PageButton *)item->privateItem;
    b->url = url;
    return item;
}

OmWebPageItem *OmWebPages::addSlider(const char *itemName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    return this->addSlider(0, 100, itemName, proc, value, ref1, ref2);
}

OmWebPageItem *OmWebPages::addSlider(int rangeLow, int rangeHigh, const char *itemName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    PageSlider *p = new PageSlider();
    p->name = itemName;
    p->value = value;
    p->ref1 = ref1;
    p->ref2 = ref2;
    p->proc = proc;
    p->min = rangeLow;
    p->max = rangeHigh;
    this->currentPage->addItem(p);
    return &p->item;
}

OmWebPageItem *OmWebPages::addTime(const char *itemName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    PageTime *p = new PageTime();
    p->name = itemName;
    p->value = value;
    p->ref1 = ref1;
    p->ref2 = ref2;
    p->proc = proc;
    this->currentPage->addItem(p);
    return &p->item;
}

OmWebPageItem *OmWebPages::addColor(const char *itemName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    PageColor *p = new PageColor();
    p->name = itemName;
    p->value = value;
    p->ref1 = ref1;
    p->ref2 = ref2;
    p->proc = proc;
    this->currentPage->addItem(p);
    return &p->item;
}

OmWebPageItem *OmWebPages::addSelect(const char *itemName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    return this->addSelectWithLink(itemName, NULL, proc, value, ref1, ref2);
}

OmWebPageItem *OmWebPages::addSelectWithLink(const char *itemName, const char *url, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    PageSelect *p = new PageSelect();
    p->name = itemName;
    p->url = url;
    p->value = value;
    p->ref1 = ref1;
    p->ref2 = ref2;
    p->proc = proc;
    this->currentPage->addItem(p);
    this->currentSelect = p;
    return &p->item;
}

void OmWebPages::addSelectOption(const char *optionName, int optionValue)
{
    if(this->currentSelect)
    {
        PageSelect *select = (PageSelect *)this->currentSelect;
        select->addOption(optionName, optionValue);
    }
}

OmWebPageItem *OmWebPages::addCheckbox(const char *itemName, const char *checkboxName, OmWebActionProc proc, int value, int ref1, void *ref2)
{
    PageCheckboxes *p = new PageCheckboxes();
    p->name = itemName;
    p->value = 0;
    p->ref1 = ref1;
    p->ref2 = ref2;
    p->proc = proc;
    this->currentPage->addItem(p);
    this->currentCheckboxes = p;

    if(checkboxName)
        p->addCheckbox(checkboxName, value);
    return &p->item;
}
void OmWebPages::addCheckboxX(const char *checkboxName, int value)
{
    if(this->currentCheckboxes && checkboxName)
    {
        PageCheckboxes *p = (PageCheckboxes *)this->currentCheckboxes;
        p->addCheckbox(checkboxName, value);
    }
}

void OmWebPages::addHtml(OmHtmlProc proc, int ref1, void *ref2)
{
    HtmlItem *h = new HtmlItem();
    h->name = "_";
    h->ref1 = ref1;
    h->ref2 = ref2;
    h->proc = proc;
    this->currentPage->addItem(h);
}

void OmWebPages::addStaticHtml(String staticHtml)
{
    StaticHtmlItem *h = new StaticHtmlItem();
    h->staticHtml = staticHtml;
    this->currentPage->addItem(h);
}

bool OmWebPages::doAction(const char *pageName, const char *itemId)
{
    Page *page = this->findPage(pageName, true);
    PageItem *item = this->findPageItem(pageName, itemId, true);
    if(item)
    {
        return item->doAction(page);
    }
    return false;
}

void OmWebPages::renderStatusXml(OmXmlWriter &w)
{
    this->renderHttpResponseHeader("text/xml", 200);

    long long now;
#ifndef NOT_ARDUINO
    now= millis();
#else
    now = 101;
#endif

    w.beginElement("xml");
    w.addAttribute("poweredBy", "OmEspHelpers");
    w.beginElement("status");
    w.addAttribute("uptime", omTime(now));
    w.addAttribute("millis", now);

    w.addAttributeF("requests","%d", this->requestsAll);
    w.addAttributeF("maxHtml", "%d", this->greatestRenderLength);

#ifndef NOT_ARDUINO
    if(this->ri)
    {
        w.addAttributeF("clientIp", "%s:%d", omIpToString(ri->clientIp, true), ri->clientPort);
        w.addAttributeF("serverIp", "%s:%d",
                      omIpToString(ri->serverIp), ri->serverPort);
        String bonjourName = ri->bonjourName;
        if(ri->bonjourName && ri->bonjourName[0])
            w.addAttributeF("bonjour", "%s", ri->bonjourName, ri->bonjourName);
        if(ri->ap && ri->ap[0])
            w.addAttributeF(    "accessPoint", "%s", ri->ap);
        else
            w.addAttributeF(    "wifiNetwork", "%s", ri->ssid);
    }
#endif
    w.addAttributeF("built", "%s %s", this->__date__, this->__time__);
    if(this->__file__)
        w.addAttributeF("file", "%s", this->__file__);

    w.endElement("status");

    if(OmEepromClass::active)
    {
        w.beginElement("eeprom");
        w.addAttribute("signature", OmEeprom.getSignature());
        w.addAttribute("dataSize", OmEeprom.getDataSize());
        int k = OmEeprom.getFieldCount();
        w.addAttribute("fieldCount", k);
        for(int ix = 0; ix < k; ix++)
        {
            w.beginElement("field");
            w.addAttribute("ix", ix);
            const char *fieldName = OmEeprom.getFieldName(ix);
            w.addAttribute("name", fieldName);
            w.addAttribute("length", OmEeprom.getFieldLength(ix));
            int fieldType = OmEeprom.getFieldType(ix);
            w.addAttribute("type", fieldType);
            // TODO could have flags for dont show, like wifi password i guess 2020
            if(fieldType == OME_TYPE_STRING)
                w.addAttribute("value", OmEeprom.getString(fieldName).c_str());
            else if(fieldType == OME_TYPE_INT)
                w.addAttribute("value", OmEeprom.getInt(fieldName));
            w.endElement();
        }
        w.endElement();
    }

    for(auto page : this->pages)
    {
        w.beginElement("page");
        w.addAttribute("name", page->name);
        w.addAttribute("id", page->id);
        w.addAttribute("k", page->items.size());
        w.endElement("page");
    }

    for (auto urlHandler : this->urlHandlers)
    {
        w.beginElement("urlHandler");
        w.addAttribute("url", urlHandler->url);
        w.addAttribute("ref1", urlHandler->ref1);
        w.addAttributeF("ref2", "0x%08x", urlHandler->ref2);
        w.addAttributeF("proc", "0x%08x", urlHandler->handlerProc);
        w.endElement("urlHandler");
    }

    for(auto page : this->pages)
        for(auto pageItem : page->items)
        {
            w.beginElement("item");
            w.addAttributeF("ctl", "http://%s:%d/_control?page=%s&item=%s&value=xxx",
                            omIpToString(ri->serverIp),
                            ri->serverPort,
                            page->id,
                            pageItem->id);
            w.addAttribute("name", pageItem->name);
            w.addAttribute("id", pageItem->id);
            w.addAttribute("pageId", page->id);
            w.addAttribute("value", pageItem->value);
            w.addAttribute("ref1", pageItem->ref1);
            w.addAttributeF("ref2", "0x%08x", pageItem->ref2);
            pageItem->renderStatusey(w);
            w.endElement("item");
        }

    w.endElement("xml");
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
#endif
#ifdef ARDUINO_ARCH_ESP8266
    w.addContentF("freeBytes:   %d\n", system_get_free_heap_size());
    w.addContentF("chipId:      %08x '8266 @%d\n", omGetChipId(), F_CPU / 1000000);
    w.addContentF("systemSdk:   %s/%d\n", system_get_sdk_version(), system_get_boot_version());
#endif
#ifdef ARDUINO_ARCH_ESP32
    // todo: try uint64_t chipid = ESP.getEfuseMac(); as per https://arduino.stackexchange.com/questions/58677/get-esp32-chip-id-into-a-string-variable-arduino-c-newbie-here
    w.addContentF("freeBytes:   %d\n", esp_get_free_heap_size());
    w.addContentF("chipId:      %08x '32 @%d\n", omGetChipId(), F_CPU / 1000000);
#endif
#ifdef NOT_ARDUINO
    w.addContentF("what:        Not Arduino\n");
    w.addContentF("chipId:      %08x 'test\n", omGetChipId());
#endif
#ifndef NOT_ARDUINO
    if(this->ri)
    {
        w.addContentF("clientIp:    %s:%d\n", omIpToString(ri->clientIp, true), ri->clientPort);

        w.addContentF("serverIp:    ");
        w.beginElement("a");
        w.addAttributeF("href", "http://%s:%d/",omIpToString(ri->serverIp), ri->serverPort);
        w.addContentF("%s:%d",omIpToString(ri->serverIp), ri->serverPort);
        w.endElement("a");
        w.addContent("\n");

        if(ri->bonjourName && ri->bonjourName[0])
        {
            w.addContentF("bonjour:     ");
            w.beginElement("a");
            w.addAttributeF("href", "http://%s.local/",ri->bonjourName);
            w.addContentF("%s:local",ri->bonjourName);
            w.endElement("a");
            w.addContent("\n");
        }

        if(ri->ap && ri->ap[0])
            w.addContentF(    "accessPoint: %s\n", ri->ap);
        else
            w.addContentF(    "wifiNetwork: %s\n", ri->ssid);
    }
    if(OmNtp::ntp())
    {
//        unsigned int ntpRequestsSent = 0;
//        unsigned int ntpRequestsAnswered = 0;
//        long ntpRequestMostRecentMilli = -1;
//        unsigned int timeUrlRequestsSent = 0;
//        unsigned int timeUrlRequestsAnswered = 0;
//        long timeUrlRequestMostRecentMillis = 0;

        OmNtp::OmNtpStats stats = OmNtp::ntp()->stats;

        long now = millis();
        float ntpAgo = (now - stats.ntpRequestMostRecentMillis) / 1000.0;
        float timeUrlAgo = (now - stats.timeUrlRequestMostRecentMillis) / 1000.0;

        w.addContentF(    "ntpRequests: %d/%d %.1fs ago\n", stats.ntpRequestsAnswered, stats.ntpRequestsSent, ntpAgo);
        w.addContentF(    "timeUrlReqs: %d/%d %.1fs ago\n", stats.timeUrlRequestsAnswered, stats.timeUrlRequestsSent, timeUrlAgo);
        if(stats.ntpServerIp)
            w.addContentF(    "ntpServer:   %s\n", omIpToString(stats.ntpServerIp,1));
        w.addContentF(    "time:        %s\n", OmNtp::ntp()->getTimeString());
    }
#endif
    if(OmEepromClass::active)
    {
        w.addContentF("eeprom:      %s/%db\n", OmEeprom.getSignature(), OmEeprom.getDataSize());
    }
    w.addContentF("built:       %s %s\n", this->__date__, this->__time__);
    if(this->__file__)
        w.addContentF("file:%s\n", this->__file__);

    w.endElement();
}

void OmWebPages::renderDefaultFooter(OmXmlWriter &w)
{
    w.addElement("hr");
    for(auto page : this->pages)
    {
        if(page->listed)
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
    // https://stackoverflow.com/questions/4137255/checkboxes-in-web-pages-how-to-make-them-bigger for the big checkboxes
    w.addContent(R"JS(
                 pre, pre a, .t {font-size:23px;font-family:Courier, monospace; word-wrap:break-word; overflow:auto; color:black}
                 form {margin-bottom:0px}
                 a:link { text-decoration: none;}
                 pre a:link {text-decoration: underline;}
                 .sliderValue { display:inline-block; font-size:16px; width:75px}
                 .colorValue { font-size:16px; margin-left:20px}
                 .selectValue { font-size:16px; float:right}
                 .checkboxLabel { font-size:16px; margin-left:9px}
                 h1,h2,h3{text-align:center}
                 select{font-size:24px; margin-left:20px}
                 input[type='checkbox'] {
                     -webkit-appearance:none;
                 width:20px;
                 height:20px;
                 background:white;
                     border-radius:5px;
                 border:2px solid #555;
                     margin-bottom:-5px;
                 }
                 input[type='checkbox']:checked {
                 background: #abd;
                 }
                 )JS"
    );


//    calc(var(--width) / 10)

//    calc(var(--parentWidth) - 90px)


    // dvb2021-08-09 pardon these experiments with trying to size the button predictable smaller than its container, for nested groups.
//        w.addContentF(".box1,.box2,.button{font-size:30px; width: 90%% ; margin:10px; "
    w.addContentF(".box1,.box2,.button{font-size:30px; width:420px ; margin:10px; "
//    w.addContentF(".box1,.box2,.button{font-size:30px; width:calc(100%%-150px) ; margin:10px; "
                 "padding:10px ; background:%s;"
                 "border-top-left-radius:15px;"
                 "border-bottom-right-radius:15px;"
                 "-webkit-user-select:none;"  // disable the touch-and-hold selection, it interferes with button pressing.
                 "}\n"
                 , colorItem
                 );
    // Not sure why I need to add more width to the button, to make it match
//    w.addContent(".button{border:2px solid black;width:440px;display:block}\n");
    w.addContent(".button{border:2px solid black;width:calc(100% - 30px);display:block}\n");
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

    function selectChange(select, page, selectName, url)
    {
        var textId = select.id + '_value';
        var text = document.getElementById(textId);
        text.innerHTML = select.value; // show in the UI the int value.

        var oReq = new XMLHttpRequest();
        oReq.addEventListener('load', reqListener);
        oReq.open('GET', '/_control?page=' + page + '&item=' + selectName + '&value=' + select.value);
        oReq.send();

        if(url == '')
        {
            document.body.style.backgroundColor = "#f0f0f0";
            setTimeout(function(){ window.location.reload(); }, 500);
        }
        else if(url != '_')
        {
            document.body.style.backgroundColor = "#C0C0C0";
            setTimeout(function() { window.location.assign(url); }, 700);
        }

    }

    function checkboxChange(page, checkboxGroupName, allCheckBoxnames)
    {
        // add up the whole group.
        var checkboxNameArray = allCheckBoxnames.split(',');
        var value = 0;
        for(ix in checkboxNameArray)
        {
            var aCheckboxName = checkboxNameArray[ix];
            var aCheckbox = document.getElementById(aCheckboxName);
            if(aCheckbox.checked)
            value += parseInt(aCheckbox.value);
        }
        var textId = page + '_' + checkboxGroupName + '_value';
        var text = document.getElementById(textId);
        text.innerHTML = value; // show in the UI the int value.

        var oReq = new XMLHttpRequest();
        oReq.addEventListener('load', reqListener);
        oReq.open('GET', '/_control?page=' + page + '&item=' + checkboxGroupName + '&value=' + value);
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
  R"JS(
  var quellMouse = 0;
  function button(button, page, buttonName, v, url)
  {
      if(v>=2)quellMouse=1;  // ignore mouses after first touch event
      if(quellMouse&&v<2)return; // ignore the mouse.
      v&=1; // mousedown/up is 1,0 and touchstart/end is 3,2
      button.style.backgroundColor=v?'%s':'%s';
      var oReq = new XMLHttpRequest();
      oReq.addEventListener('load', reqListener);
      oReq.open('GET', '/_control?page=' + page + '&item=' + buttonName + '&value=' + v);
      oReq.send();
      if(url == '')
          setTimeout(function(){ window.location.reload(); }, 500);
      else if(url != '_')
      {
          document.body.style.backgroundColor = "#C0C0C0";
          setTimeout(function() { window.location.assign(url); }, 700);
      }
  }
  )JS",
    colorButtonPress, colorItem
                 );
    w.endElement();
}

/// Render the beginning of the page, leaving <body> element open and ready.
void OmWebPages::renderPageBeginning(OmXmlWriter &w, const char *pageTitle, int bgColor, OmWebPages *p)
{
    OmWebPages::renderPageBeginningWithRedirect(w, NULL, 0, pageTitle, bgColor, p);
}

void OmWebPages::renderPageBeginningWithRedirect(OmXmlWriter &w, const char *redirectUrl, int redirectSeconds, const char *pageTitle, int bgColor, OmWebPages *p)
{
    w.addContentRaw("<!DOCTYPE html>\n");
    w.beginElement("html");
    w.addAttribute("lang", "en");
    w.beginElement("head");

    // discourage favicon requests with <link rel="icon" href="data:,">
    // as per https://stackoverflow.com/questions/1321878/how-to-prevent-favicon-ico-requests
    w.beginElement("link");
    w.addAttribute("rel", "icon");
    w.addAttribute("href", "data:,");
    w.endElement();

    // maybe a redirect
    if(redirectUrl)
    {
        w.beginElement("meta");
        w.addAttribute("http-equiv", "refresh");
        w.addAttributeF("content", "%d;%s", redirectSeconds, redirectUrl);
        w.endElement();
    }

    w.beginElement("meta", "charset", "UTF-8");
    w.endElement();
    w.beginElement("meta");
    w.addAttribute("name", "viewport");
    w.addAttribute("content", "width=480");
    w.endElement();

    // fact is, I'd really rather see the name of the device (or ip id)
    // than particular page title.
    if(p && p->ri && p->ri->bonjourName)
    {
        const char *bonjourName = p->ri->bonjourName;
        if(bonjourName[0] && (bonjourName[1] == '-'))
           bonjourName += 2; // if name is x-something, just something
        w.addElementF("title", "%d %s", p->ri->serverIp[0], bonjourName);
    }
    else if(p && p->ri)
        w.addElementF("title", "%d", p->ri->serverIp[0]);
    else
        w.addElement("title", pageTitle);

    OmWebPages::renderStyle(w, bgColor);
    OmWebPages::renderScript(w);
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
    const char *defaultHttpBase = "/";
    const char *httpBase = defaultHttpBase;
#ifndef NOT_ARDUINO
//    httpBase = OmWebPages::httpBase;
#endif
    w.addAttributeUrlF("href", "%s%s", httpBase, pageName);
    w.beginElement("span", "class", "box2");
    w.addContent(pageName);
    w.endElement();
    w.endElement();
}

Page *OmWebPages::findPage(const char *pageName, bool byId)
{
    Page *p = 0;
    for(Page *page : this->pages)
    {
        const char *lookingFor = byId ? page->id : page->name;
        if(omStringEqual(pageName, lookingFor))
        {
            p = page;
            break;
        }
    }
    return p;
}

PageItem *OmWebPages::findPageItem(const char *pageName, const char *itemName, bool byId)
{
    Page *page = this->findPage(pageName, byId);
    if(page)
    {
        for(PageItem *b : page->items)
        {
            const char *lookingFor = byId ? b->id : b->name;
            if(omStringEqual(itemName, lookingFor))
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

    // make our httpBase up to date
    sprintf(OmWebPages::httpBase, "http://%s:%d/",omIpToString(requestInfo->serverIp), requestInfo->serverPort);

    this->wp = &w;
    this->ri = requestInfo;

    this->requestsAll++;
    static OmWebRequest request;
    request.init(pathAndQuery);
    const char *requestPath = request.path;
    this->rq = &request; // here during the url handling callback

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
        const char *itemId = request.getValue("item");
        const char *valueS = request.getValue("value");
        
        bool setParam = false;
        if(valueS)
        {
            // if it's a time string, do minutes of day.
            int value = stringToIntMaybeTimeString(valueS);
            PageItem *item = this->findPageItem(pageName, itemId, true);
            if(item)
            {
                item->setValue(value);
                setParam = true;
            }
        }
        
        this->doAction(pageName, itemId);
        
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

    if(!page && this->urlHandler.handlerProc)
    {
        // oh we have a global url handler, ok.
        this->urlHandler.handlerProc(w, request, urlHandler.ref1, urlHandler.ref2);
        goto goHome;
    }

    if(!page)
        page = this->homePage;

    // +-----------------------------------
    // | Page Rendering Happens Now.

    if(page->arrivalAction)
        page->arrivalAction(page->name, NULL, 0, page->arrivalActionRef1, page->arrivalActionRef2);

    // Do a page, starting with the response headers.
    this->renderHttpResponseHeader("text/html", 200);
    this->renderPageBeginning(w, page->name, this->bgColor, this);
    w.addContent("\n");
    
    if(this->headerProc && page->allowHeader)
        this->headerProc(w, 0, 0);
    
    {
        w.beginElement("h2");
        w.beginElement("a");
        w.addAttributeF("href", "/%s", page->name);
        w.addContent(page->name);
        w.endElement();
        w.endElement();
        w.addContent("\n");
    }

#define tempTestGroupEmAll  false

    if(tempTestGroupEmAll)
    {
        w.beginElement("div", "class", "box1");
        w.addContent("grouped");
    }
    for(PageItem *b : page->items)
    {
        if(b->visible)
        {
            b->render(w, page, !tempTestGroupEmAll);
            w.addContent("\n");
        }
    }
    if(tempTestGroupEmAll)
        w.endElement("div");

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

void OmWebPages::addUrlHandler(OmUrlHandlerProc proc, int ref1, void *ref2)
{
    this->urlHandler.handlerProc = proc;
    this->urlHandler.ref1 = ref1;
    this->urlHandler.ref2 = ref2;
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

int OmWebPages::getValue(const char *pageName, const char *itemName)
{
    int result = -1;
    PageItem *pi = this->findPageItem(pageName, itemName);
    if(pi)
        result = pi->value;
    return result;
}

// helper method for the Form
static void addTextInput(OmXmlWriter &w, const char *label, const char *name, String &value)
{
    if(label == NULL)
        label = name;
    w.beginElement("tr");
    w.beginElement("td");
    w.addContentF("%s: ", label);
    w.endElement("td");
    w.beginElement("td");
    w.beginElement("input");
    w.addAttribute("type", "text");
    w.addAttribute("name", name);
    w.addAttributeF("value", value.c_str());
    w.endElement();
    w.endElement("td");
    w.endElement("tr");
//    w.addElement("br");
}



void OmWebPages::addEepromConfigForm(OmHtmlProc eepromConfigCallbackProc, int eepromFieldGroup)
{
    // handler for the text edit entries
#define EE_CONFIG_PAGE "_configSubmit"

    this->eepromConfigCallbackProc = eepromConfigCallbackProc;

    // THIS is the unlisted page that handles form submissions, including the restart button.
    void *oldPage = OmWebPages::p->beginPage(EE_CONFIG_PAGE, false); // unlisted page
    OmWebPages::p->addHtml([] (OmXmlWriter & w, int ref1, void *ref2)
                           {
                               OmWebRequest &r = *OmWebPages::p->rq;


                               // render the web page result...
                               w.beginElement("pre");
                               int k = r.getQueryCount();
                               for (int ix = 0; ix < k; ix++)
                               {
                                   auto queryKey = r.getQueryKey(ix);
                                   auto queryValue = r.getQueryValue(ix);
                                   OmEeprom.setString(queryKey, queryValue);
                                   w.addContentF("%3d. %s : %s / %s\n", ix + 1, queryKey, queryValue, OmEeprom.getString(queryKey).c_str());
                               }
                               int eepromCommitResult = OmEeprom.commit();
                               w.addContentF("%d bytes changed\n", eepromCommitResult);
                               String bonjourName = OmEeprom.getString("otaBonjourName");
                               if(bonjourName.length())
                                   w.addContentF("→http://%s.local\n", bonjourName.c_str());
                               w.endElement();

                               if(OmWebPages::p->eepromConfigCallbackProc)
                                   OmWebPages::p->eepromConfigCallbackProc(w, ref1, ref2);
                           });
    OmWebPages::p->addButtonWithLink("restart with new config", "%/", [] (const char *page, const char *item, int value, int ref1, void *ref2)
                                     {
                                         ESP.restart();
                                     });
    OmWebPages::p->addButtonWithLink("don't restart", "%/");

    // THIS is where we now add the visible form entry page content.
    OmWebPages::p->resumePage(oldPage);

    OmWebPages::p->addHtml([](OmXmlWriter & w, int ref1, void *ref2)
                           {
                               // ref1 is which group to display

                               w.addElement("hr");
                               w.beginElement("form");
                               w.addAttribute("action", "/" EE_CONFIG_PAGE);
                               int k = OmEeprom.getFieldCount();
                               w.beginElement("table");
                               for(int ix = 0; ix < k; ix++)
                               {
                                   OmEepromField *f = OmEeprom.findField(ix);
                                   int fieldGroup = f->omeFlags & 0x00ff;
                                   if(fieldGroup == ref1)
                                   {
                                       String value = OmEeprom.getString(f->name);
                                       addTextInput(w, f->label, f->name, value);
                                   }
                               }
                               w.endElement("table");
                               w.addElement("br");
                               w.beginElement("input");
                               w.addAttribute("type", "submit");
                               w.addAttribute("value", "Submit");
                               w.endElement();
                               w.endElement();
                           }, eepromFieldGroup);

}

char OmWebPages::httpBase[32]; // string of form "http://10.0.1.1/" that can prefix all internal links, esp since OTA update doesnt play well with bonjour addresses

OmWebPages *OmWebPages::p; // most recently instanteated OmWebPages (usually the one and only)



uint32_t omGetChipId()
{
    uint32_t result;
#ifdef NOT_ARDUINO
    result = 0x655321;
#endif
#ifdef ARDUINO_ARCH_ESP8266
    result = system_get_chip_id();
#endif
#ifdef ARDUINO_ARCH_ESP32
    result = ESP.getEfuseMac();
#endif
    return result;
}
