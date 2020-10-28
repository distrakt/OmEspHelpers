/*
   2019-11-17 If booted in some way (a button down or whatever) then execute THIS setup() and loop() instead.
   It serves up an OTA access page instead.
*/


#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "OmOta.h"

#include "OmUtil.h"
#include "OmEeprom.h"

/*
   Flash image upload page
*/
static const char* kServerIndexTemplate = R"(
  <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>
  <h1>reflash %s %s</h1>
  <form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
  <input type='file' name='update'>
  <input type='submit' value='Update'>
  </form>
  <div id='prg'>progress: 0%%</div>
  <script>
  $('form').submit(function(e) {
    e.preventDefault();
    var form = $('#upload_form')[0];
    var data = new FormData(form);
    $.ajax( {
      url: '/update',
      type: 'POST',
      data: data,
      contentType: false,
      processData:false,
      xhr: function() {
        var xhr = new window.XMLHttpRequest();
        xhr.upload.addEventListener('progress', function(evt) {
          if (evt.lengthComputable) {
            var per = evt.loaded / evt.total;
            $('#prg').html('progress: ' + Math.round(per*100) + '%%');
            if(evt.loaded == evt.total)
            {
              console.log('all sent');
              document.body.style.backgroundColor = "#808080";
              setTimeout(function() { window.location.reload(); }, 10000); // ten seconds to go there.
            }
          }
        }, false);
        return xhr;
      },
      success:function(d, s) {
        console.log('success!')
      },
      error: function (a, b, c) {
      }
    });
  });
  </script>
  <br /><br /><br /><br /><br />
  <a href='/reboot'>Reboot with or without Upload</a>
  )";

static const char *kServerRedirect = R"(
  <html><head><meta http-equiv='refresh' content='0; url=/' /></head>
  <body>/</body></html>)";

static const char *kRebootMessage = R"(
  <html><head><meta http-equiv='refresh' content='5; url=/' /></head>
  <body>rebooting...</body></html>)";

OmOtaClass OmOta;

void OmOtaClass::addEeFields(OmEeprom &ee)
{
  this->ee = &ee;
  this->ee->addInt8("otaMode");
}

bool OmOtaClass::setup(const char *wifiSsid, const char *wifiPassword, const char *wifiBonjour, OtaStatusProc statusProc)
{
  this->statusProc = statusProc;
  if (!this->ee)
  {
    this->ee = new OmEeprom("OmOta_xyzzy");
    this->addEeFields(*this->ee);
    this->ee->begin();
  }
  this->otaMode = this->ee->getInt("otaMode");
  if (!this->otaMode)
    return false; // back to the main program... else continue on down with upload and setup.


  Serial.begin(115200);
  Serial.printf("\n\n\n");
  OMLOG("Welcome to Ota Upload");

  this->ee->set("otaMode", 0); // out of ota mode next time.
  this->ee->commit();

  OMLOG("ota A");

  this->doProc(OSS_BEGIN, 0);
  OMLOG("ota B");

  this->bonjourName[0] = 0;
  if (wifiBonjour)
    strcpy(this->bonjourName, wifiBonjour);

  this->serverPtr = new _ESPWEBSERVER(80);
  OMLOG("ota C");


  // Connect to WiFi network
  WiFi.begin(wifiSsid, wifiPassword);
  //  WiFi.begin(ssid, "bad"); //TEST WITH BAD PASSWORD
  Serial.println("\n");
  OMLOG("ota D");

  // Wait for connection
  int wifiDots = 0;
  do
  {
    this->doProc(OSS_WIFI_CONNECTING, wifiDots++);
    delay(10);
  } while (WiFi.status() != WL_CONNECTED);
  this->doProc(OSS_WIFI_SUCCESS, wifiDots);
  OMLOG("ota connected to wifi %s", wifiSsid);
  OMLOG("ota connected at http://%s/", omIpToString(WiFi.localIP(), true));

  /*also maybe mdns for host name resolution*/
  if (strlen(this->bonjourName))
  {
    MDNS.begin(this->bonjourName);
    MDNS.addService("http", "tcp", 80);
  }

  /*return index page which is stored in serverIndex */
  this->serverPtr->on("/", HTTP_GET, []()
  {
    if (OmOta.rebootOnGet)
    {
      //      displayTextF(true, "reboot");
      OmOta.serverPtr->sendHeader("Connection", "close");
      OmOta.serverPtr->send(200, "text/html", kRebootMessage);
      delay(300);
      ESP.restart();
    }

    //    static int requests = 0;
    //    displayTextF(false, "awaiting ota\n%s\n%d", omIpToString(WiFi.localIP(), true), ++requests);

    static char serverIndex[2000];
    sprintf(serverIndex, kServerIndexTemplate, OmOta.bonjourName, omIpToString(WiFi.localIP(), true));

    OmOta.serverPtr->sendHeader("Connection", "close");
    OmOta.serverPtr->send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  OmOta.serverPtr->on("/update", HTTP_POST, []()
  {
    OmOta.serverPtr->sendHeader("Connection", "close");
    OmOta.serverPtr->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []()
  {
#ifndef UPDATE_SIZE_UNKNOWN
#define UPDATE_SIZE_UNKNOWN 0xffffffff
#endif

    HTTPUpload &upload = OmOta.serverPtr->upload();

    //    displayTextF(true, "^%d", upload.totalSize);
    Serial.printf("upload.currentSize: %d, upload.totalSize: %d\n", upload.currentSize, upload.totalSize);

    switch (upload.status)
    {
      case UPLOAD_FILE_START:
        Serial.printf("Update: %s\n", upload.filename.c_str());
        OmOta.doProc(OSS_UPLOAD_START, 0);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) //start with max available size
          Update.printError(Serial);
        break;

      case UPLOAD_FILE_WRITE:
        /* flashing firmware to ESP*/
        OmOta.doProc(OSS_UPLOADING, upload.totalSize);
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          Update.printError(Serial);
        break;

      case UPLOAD_FILE_END:
        OmOta.doProc(OSS_UPLOADED, upload.totalSize);
        if (Update.end(true))  //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        else
          Update.printError(Serial);
        break;

      case UPLOAD_FILE_ABORTED:
        Serial.printf("upload aborted\n");
        break;
    }
  });


  this->serverPtr->onNotFound( []() {
    OmOta.serverPtr->sendHeader("Connection", "close");
    OmOta.serverPtr->send(200, "text/html", kServerRedirect);
  });

  this->serverPtr->on("/reboot", HTTP_GET, []()
  {
    // set the reboot trigger, and go to '/' to do it.
    OmOta.rebootOnGet = true;
    OmOta.serverPtr->sendHeader("Connection", "close");
    OmOta.serverPtr->send(200, "text/html", kServerRedirect);
  });

  this->serverPtr->begin();
  return true;
}

bool OmOtaClass::loop()
{
  if (!this->otaMode)
    return false;

  // we are doing the setup loop!
#if ARDUINO_ARCH_ESP8266
  // esp32 dont need this i guess
  if (strlen(this->bonjourName))
    MDNS.update();
#endif
  this->doProc(OSS_IDLE, 0);
  this->serverPtr->handleClient();
  delay(1);

  return true;
}

void OmOtaClass::doProc(EOtaSetupState state, int progress)
{
  if(state == OSS_UPLOADING)
  {
    OMLOG("Ota Uploading %d\n", progress);
  }
  else if(state != this->lastState)
  {
    // print it when state changes
    OMLOG("Ota State %d\n", state);
  }
  this->lastState = state;

  if (this->statusProc)
    this->statusProc(state, progress);
}

void OmOtaClass::rebootToOta()
{
  // i guess we are doing it!
  this->ee->set("otaMode", 1);
  this->ee->commit();
  Serial.printf("OmOta: rebooting\n");
  delay(500);
  ESP.restart();
}

static int ir(int low, int high)
{
    return rand() % (high - low) + low;
}

void OmOtaClass::addUpdateControl(OmWebPages &p)
{
    // enter upload mode
    static int otaCode = 0;
    p.addHtml([](OmXmlWriter & w, int ref1, void *ref2)
              {
                  otaCode = ir(1, 99);
                  w.addElement("hr");
                  w.beginElement("pre");
                  w.addContentF("update code: %d", otaCode);
                  w.endElement();
              });
    static OmWebPageItem *otaCodeSlider = p.addSlider("update code");
    p.addButtonWithLink("update", "/", [] (const char *page, const char *item, int value, int ref1, void *ref2)
                        {
                            int v = otaCodeSlider->getValue();
                            otaCodeSlider->setValue(0);
                            
                            if (v == otaCode)
                            {
                                OmOta.rebootToOta();
                            }
                        });
}

const char *OmOtaClass::getIpAddress()
{
    return omIpToString(WiFi.localIP(), true);
}
