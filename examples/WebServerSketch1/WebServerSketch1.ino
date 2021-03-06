/*
   2016-11-26 dvb
   2019-11-15 updated for Esp32
   2020-05-14 added examples of all the control types, and dynamic url handlers too!
              It's a much better example and intro now. In the time of covid19.
   2020-12-27 added more comments.
*/
#include <OmEspHelpers.h>
#include "jpgImage.h"

// These are the main pieces of OmEspHelpers.
// OmWebServer manages the connection, and OmWebPages manages the controls.
OmWebServer s;
OmWebPages p;

// With this callback, you can know when the wifi is up and running.
// One of the three booleans will be set.
void connectionStatus(const char *ssid, bool trying, bool failure, bool success)
{
  const char *what = "?";
  if (trying) what = "trying";
  else if (failure) what = "failure";
  else if (success) what = "success";

  Serial.printf("%s: connectionStatus for '%s' is now '%s'\n", __func__, ssid, what);
}

// This is the callback suitable for a change on any of the control types.
// Each of your controls can use a different callback, or they can all be shared.
// You should check the valid range of value and ref1, since they can be
// specified in the client-side URL.
// The callback is specified when you add the control to a page in setup() below.
void actionProc(const char *pageName, const char *parameterName, int value, int ref1, void *ref2)
{
  static int actionCount = 0;
  actionCount++;
  Serial.printf("%4d %s.%s value=0x%08x/%d ref1=%d\n", actionCount, pageName, parameterName, value, value, ref1);
}

// This is a callback for a URL handler. You can set a handler for an arbitrary URL
// or a wildcard for URLs that are not in a built page or assigned a handler.
// The OmXmlWriter can write plain text or bytes, or can be used to reliably construct
// compliant xml/xhtml output.
void handyHandler(OmXmlWriter & w, OmWebRequest & r, int ref1, void *ref2)
{
  // you could use ref1, ref2 any way you like...
  // OmWebRequest r.path is the url without query terms,
  // OmWebRequest r.query is a vector of char *s, alternating key & value
  // this handler ignores both
  p.renderHttpResponseHeader("image/jpg", 200);
  for (unsigned int ix = 0; ix < jpgImageSize; ix++)
  {
    // the OmXmlWriter can also emit raw binary bytes, one at a time, like this:
    w.put(jpgImage[ix]);
  }
}

// This callback handles any URL not otherwise handled, see setup().
void wildcardProc(OmXmlWriter &w, OmWebRequest &request, int ref1, void *ref2)
{
  p.renderHttpResponseHeader("text/plain", 200);
  w.putf("---------------------------\n");
  w.putf("no handler matched the request, so here ya go\n");
  w.putf("request path: %s\n", request.path); // putf is limited to 1k intermediate buffer, sorry!
  int k = (int)request.query.size();
  for (int ix = 0; ix < k - 1; ix += 2)
    w.putf("arg %d: %s = %s\n", ix / 2, request.query[ix], request.query[ix + 1]);
  w.putf("---------------------------\n");
  return;
}

OmWebPageItem *xmlRecordCountSlider = NULL; // this global is used by the xml data proc. Find it below.

void setup()
{
  Serial.begin(115200);
  Serial.printf("\n\n\nWelcome to the Sketch\n\n");

  // Add networks to attempt connection with.
  // It will go down the list and repeat until on succeeds.
  // This only happens when tick() is called.
  s.addWifi("connection will fail", "haha");
  s.addWifi("omino warp", "0123456789");
  s.addWifi("caffe pergolesi", "yummylatte");

  // let us know when we are connected or disconnected
  s.setStatusCallback(connectionStatus);

  // Introduce the web pages to the wifi connection.
  s.setHandler(p);

  // Configure the web pages
  // This is just setting them up; they'll be delivered
  // by the web server when queried.

  p.setBuildDateAndTime(__DATE__, __TIME__);

  p.beginPage("Home");

  // This html code is executed on each request, so
  // current values and state can be shown.
  // The first argument is the callback, but here
  // using a C "lambda" which is like a callback proc
  // right in the argument list.

  p.addHtml([] (OmXmlWriter & w, int ref1, void *ref2)
  {
    w.addElement("hr");

    w.beginElement("pre");
    // note the multi-line string literal format.
    // I only just learned about this! // dvb2020.
    w.addContent(R"(
This is a demonstration of
OmEspHelpers. It's a library for
easily adding a web front end to
your ESP8266- and ESP32-based
projects. Links below show the
different control types, and some
dynamic generation of very long
XML data.

Please enjoy! -- dvb)");
    w.endElement();
  });

  p.beginPage("All Control Types");
  /*
     Sliders can be added with automatic range 0 to 100,
     or with a range. The actionProc gets called when you
     change the slider in the web page. That last number 101
     is passed as "ref1" to the actionProc, so if you want
     you can share the same actionProc among multiple controls
     and know who sent it.

     In this example, all the controls use the same actionProc.

     Values are updated as you move the slider, but limited
     to around ten per second. But the latest value when you
     pause somewhere or stop sliding is guaranteed to arrive.
  */
  p.addSlider("slider 1", actionProc, 23, 101);
  p.addSlider(-86, 99, "slider 2", actionProc, 42, 102);

  /*
     It is convenient to embed the callback inline, if it doesn't need
     to be shared.
   */
  p.addSlider("slider 3", [] (const char *pageName, const char *parameterName, int value, int ref1, void *ref2)
  {
      Serial.printf("slider 3 new value: %d\n", value);
  });

  /*
     A button sends a value "1" when pressed (in the web page),
     and zero when released.
  */
  p.addButton("button 1", actionProc, 103);

  /* looks like a button, but is an HMTL link also */
  p.addButtonWithLink("ginger.jpg", "/ginger.jpg", actionProc, 109);


  /*
     A select control shown as a dropdown on desktop browsers,
     and some other multiple-choice control on phones.
     You can add any number of options, and specify
     the value of each.
  */
  p.addSelect("choices", actionProc, 2, 104);
  p.addSelectOption("choice 1", 1);
  p.addSelectOption("choice 2", 2);
  p.addSelectOption("a third choice", 3);
  p.addSelectOption("a hundred!", 100);

  /*
     Checkboxes can be individual, or a group of several.
     The "value" is accumulated as a binary result. If none of the
     boxes are checked, the value is zero. The first checkbox is
     1, the second is 2, the third is 4, the fourth is 8, and so on.
     Changing any of the checkboxes (in the browser) sends a new
     value which adds them all up.
  */
  p.addCheckbox("checkboxes", "box1", actionProc, 1, 105);
  p.addCheckboxX("box2", 0);
  p.addCheckboxX("box three", 1);
  p.addCheckboxX("fourth box", 0);

  /*
     a color uses only the built-in color picker of the browser.
     Some browsers don't have one (safari!). Chrome's is
     pretty good. The value is the hex integer for the color.
     Here the initial value is RGB(255,0,128) or #ff0080.

     Like the slider control, it will do live updates but
     safely throttled to a handful per second.
  */
  p.addColor("color input", actionProc, 0xFF0080, 106);

  /*
     the Time control consists of a time-of-day entry and
     a checkbox. The value is "minute number of the day" or'd
     with 0x8000 if the checkbox is set. This useful for
     an alarm time, and either "set" or "not set".
     Here, the initial value of 182 means 3:02am, alarm not set.
  */
  p.addTime("alarm time", actionProc, 182, 107);

  /*
     Here we see how to embed an html-generating proc
     that gets run on each refresh. See how the XML Writer
     guarantees well-formed XHTML.
  */
  p.addHtml([] (OmXmlWriter & w, int ref1, void *ref2)
  {
    w.addElement("hr");
    w.beginElement("pre");
    w.addContentF("This is a random number: %d\n", random(1000));
    w.endElement(); // pre

    w.beginElement("div");
    w.addAttribute("style", "color:red; font-size:9px");
    w.addContent("this is in a div with a style attribute");
    w.endElement(); // div
  });

  p.beginPage("xml demo");
  xmlRecordCountSlider = p.addSlider(1, 5000, "number of xml records", NULL, 50);
  p.addHtml([] (OmXmlWriter & w, int ref1, void *ref2)
  {
    w.addElement("hr");
    w.beginElement("pre");
    w.addContent("Click ");
    w.beginElement("a", "href", "_sendXml");
    w.addContent("here");
    w.endElement(); // a
    w.addContent(" to get\na bunch of random XML.\n\n");
    w.addContent("It is produced and streamed,\n.");
    w.addContent("and doesn't fit in ESP memory.\n.");
    w.endElement(); // div
  });

  p.beginPage("ginger");
  p.addHtml([] (OmXmlWriter & w, int ref1, void *ref2)
  {
    w.addElement("hr");
    w.beginElement("img");
    w.addAttribute("src", "/ginger.jpg"); // goes to the "ginger" handler.
    w.addAttribute("alt", "");
    w.endElement();
    w.endElement(); // div
  });

  /*
     Here we have a straight-up URL handler, but still emits
     XML-ish output. In this case it's inline as a lambda.
  */
  p.addUrlHandler("_sendXml", [] (OmXmlWriter & w, OmWebRequest & r, int ref1, void *ref2)
  {
    int recordCount = xmlRecordCountSlider->getValue();
    p.renderHttpResponseHeader("text/xml", 200);
    w.beginElement("randomXmlData");
    w.addAttribute("uptime", s.uptimeMillis());
    w.addAttributeF("built", "%s %s", __DATE__, __TIME__);

    w.beginElement("records");
    w.addAttribute("recordCount", recordCount);
    for (int ix = 0; ix < recordCount; ix++)
    {
      w.beginElement("record");
      w.addAttribute("ix", ix);
      w.addAttribute("randomNumber", random(-1000000, 1000000));
      w.endElement(); // record
    }
    w.endElement(); // records
    w.endElement(); // randomxmldata
  });

  p.addUrlHandler("ginger.jpg",  [] (OmXmlWriter & w, OmWebRequest & r, int ref1, void *ref2)
  {
    p.renderHttpResponseHeader("image/jpg", 200);
    for (unsigned int ix = 0; ix < jpgImageSize; ix++)
    {
      // the OmXmlWriter can also emit raw binary bytes, one at a time, like this:
      w.put(jpgImage[ix]);
    }
  });

  /* example of url handler not lambda */
  p.addUrlHandler("ginger2.jpg", handyHandler, 123, NULL); // last two args are int ref1 and void *ref2
  p.addUrlHandler(wildcardProc);
}

int ticks = 0;
void loop()
{
  delay(10);
  /*
     The only Care and Feeding needed after starting up is s.tick(). Call it as often as you like.
     It cycles the wifi-setup state machine, and handles requests once connected.

     Callbacks to your action procs will occur during this s.tick() call.
  */
  s.tick();

  /*
     And here you can do the rest of your business.

     Counting inputs on the pins
     It don't bother me a bit
     Flip a relay switch til dawn
     On a cheapo current sink
     Blinking LEDs and reading PHOto sensors
     Now don't tell me I've nothing to do
  */
}

