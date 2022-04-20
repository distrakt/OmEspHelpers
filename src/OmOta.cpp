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

// with flexible procs and interfaces, parameters are often optionally used.
#pragma GCC diagnostic ignored "-Wunused-parameter"

#define OTA_MODE_TIMEOUT_SECONDS 600

/*
 Flash image upload page
 */
static const char* kServerIndexTemplate = R"(
<h1>reflash %s /%s/</h1>
<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
<input type='file' name='update'>
<input type='submit' value='Update'>
</form>
<div id='prg'>progress: 0%%</div>

<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>
<script>
var form = document.getElementById('upload_form');
var prg = document.getElementById('prg');

$('form').submit(function(e) {
    document.body.style.backgroundColor = "#c0e0e0";
    e.preventDefault();
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
                prg.innerHTML = 'progress: ' + Math.round(per*100) + '%%';
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

void OmOtaClass::addEeFields()
{
    OmEeprom.addInt8("otaMode", OME_GROUP_OTA, NULL);
    OmEeprom.addString("otaBonjourName", OOC_STRING_LEN, OME_GROUP_WIFI_SETUP | OME_FLAG_BONJOUR, "Bonjour name");
    OmEeprom.addString("otaWifiSsid", OOC_STRING_LEN, OME_GROUP_WIFI_SETUP, "Wifi");
    OmEeprom.addString("otaWifiPassword", OOC_STRING_LEN, OME_GROUP_WIFI_SETUP, "Password");
}

// private to attempt a wifi connection, eventually timeout & go again.
bool OmOtaClass::doAWiFiTry(String ssid, String password, int &wifiDots)
{
    long long t0 = millis();
    long long t = 0;
#define WIFI_TIMEOUT 8500
    int wifiStatus0 = -1;
    int wifiStatus;

    WiFi.begin(ssid.c_str(), password.c_str());

    OMLOG("doAWiFiTry %s[%d] %s[%d]", ssid.c_str(), (int)ssid.length(), password.c_str(), (int)password.length());
    do
    {
        this->doProc(OSS_WIFI_CONNECTING, wifiDots++);
        delay(30);
        wifiStatus = WiFi.status();
        if(wifiStatus != wifiStatus0)
        {
            OMLOG("ota wifi status: %d %s\n", wifiStatus, OmWebServer::statusString(wifiStatus));
            wifiStatus0 = wifiStatus;
        }
        if(wifiStatus == WL_CONNECTED)
        {
            this->ssidActuallyConnected = ssid;
            return true;
        }
        t = millis() - t0;
    } while (t < WIFI_TIMEOUT);
    OMLOG("%s t = %d, not connected", ssid.c_str(), (int)t);
    return false; // long enough. go home.
}


bool OmOtaClass::setup(OtaStatusProc statusProc)
{
    bool result = this->setup(NULL, NULL, NULL, statusProc);
    return result;
}

bool OmOtaClass::setup(const char *wifiSsid, const char *wifiPassword, const char *wifiBonjour, OtaStatusProc statusProc)
{
    this->statusProc = statusProc;
    bool weOwnTheEeprom = OmEeprom.getFieldCount() == 0;
    if (weOwnTheEeprom)
    {
        this->addEeFields();
        OmEeprom.begin(this->defaultSignature);
        OmEeprom.dumpState("omota 3");
    }

    this->bonjourName[0] = 0;
    if (wifiBonjour)
        strcpy(this->bonjourName, wifiBonjour);

    this->retrieveWifiConfig();
    if(OmWebServer::s)
    {
        OMLOG("setup wifis");
        OmWebServer::s->addWifi(OmOta.otaWifiSsid, OmOta.otaWifiPassword);
        OmWebServer::s->setBonjourName(OmOta.otaBonjourName);
        char apSsid[32];
        sprintf(apSsid,"om_%03d", omGetChipId() % 1000);
        OmWebServer::s->setAccessPoint(apSsid, "", 90);
    }

    this->otaMode = OmEeprom.getInt("otaMode");
    if (!this->otaMode)
        return false; // back to the main program... else continue on down with upload and setup.


    Serial.begin(115200);
    OMLOG("Welcome to Ota Upload");

    OmEeprom.set("otaMode", 0); // out of ota mode next time.
    OmEeprom.commit();
    OMLOG("dom 01");

    this->doProc(OSS_BEGIN, 0);
    OMLOG("dom 02");

    OMLOG("dom 03");

    this->serverPtr = new _ESPWEBSERVER(80);
    OMLOG("dom 04");

    // Connect to WiFi network
    WiFi.persistent(false);
//    WiFi.begin(wifiSsid, wifiPassword);
    //  WiFi.begin(ssid, "bad"); //TEST WITH BAD PASSWORD
    //    Serial.println("\n");
    OMLOG("dom 05");

    // Wait for connection
    int wifiDots = 0;

    int loopyTries = 10; // if we dont connect after this many, reboot so AP mode will fire up.
    while(loopyTries-- > 0)
    {
        delay(100);
        if(this->otaWifiSsid[0])
        {
            OMLOG("dom 04a ssid:%p, pwd:%p", this->otaWifiSsid, this->otaWifiPassword);
            OMLOG("dom 04b ssid:%s, pwd:%s", this->otaWifiSsid, this->otaWifiPassword);
            bool did = this->doAWiFiTry(this->otaWifiSsid, this->otaWifiPassword, wifiDots);
            if(did)
                goto gotWifi;
        }
        delay(100);
        if(wifiSsid && wifiSsid[0])
        {
            OMLOG("dom 04c ssid:%p, pwd:%p", wifiSsid, wifiPassword);
            OMLOG("dom 04d ssid:%s, pwd:%s", wifiSsid, wifiPassword);
            bool did = this->doAWiFiTry(wifiSsid, wifiPassword, wifiDots);
            if(did)
                goto gotWifi;
        }
    }

    OMLOG("couldn't connect to wifi for update... go back to main mode");
    delay(300);
    ESP.restart();

gotWifi:
    OmOta.otaStarted = millis();
    this->doProc(OSS_WIFI_SUCCESS, wifiDots);
    OMLOG("ota connected to wifi %s", this->ssidActuallyConnected.c_str());
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
                            OmOta.otaStarted = millis();
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
                            OmOta.otaStarted = millis();
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

    {
        // have we been idling in otaMode for too long?
        long long t = millis();
        int otaModeIdlingSeconds = (t - this->otaStarted) / 1000;
        if(otaModeIdlingSeconds > OTA_MODE_TIMEOUT_SECONDS)
        {
            OMLOG("idling in OTA mode for %d seconds, rebooting", otaModeIdlingSeconds);
            ESP.restart();
        }
    }

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
    OmEeprom.set("otaMode", 1);
    OmEeprom.commit();
    Serial.printf("OmOta: rebooting\n");
    delay(500);
    ESP.restart();
}

static int ir(int low, int high)
{
    return rand() % (high - low) + low;
}

void OmOtaClass::addUpdateControl()
{
    // enter upload mode
    static int otaCode = 0;
    OmWebPages::p->addHtml([](OmXmlWriter & w, int ref1, void *ref2)
              {
                  otaCode = (millis() + ir(0, 1000)) % 99 + 1;
                  //                  otaCode = ir(1, 99);
                  w.addElement("hr");
                  w.beginElement("pre");
                  w.addContentF("update code: %d", otaCode);
                  w.endElement();
              });
    static OmWebPageItem *otaCodeSlider = NULL;
    otaCodeSlider = OmWebPages::p->addSlider("update code");
    // (the leading percent sign '%' lets omwebpages preface the url better, optionally)
    OmWebPages::p->addButtonWithLink("update", "%/", [] (const char *page, const char *item, int value, int ref1, void *ref2)
                        {
                            int v = otaCodeSlider->getValue();
                            otaCodeSlider->setValue(0);

                            if (v == otaCode)
                            {
                                OmOta.rebootToOta();
                            }
                        });
}

// helper method for the Form
static void addTextInput(OmXmlWriter &w, const char *label, const char *name, const char *value)
{
    w.addContentF("%s: ", label);
    w.beginElement("input");
    w.addAttribute("type", "text");
    w.addAttribute("name", name);
    w.addAttributeF("value", value);
    w.endElement();
    w.addElement("br");
}

static void webRequestToEepromWrite(OmWebRequest &r, const char *fieldName, int makeBonjourLegal = 0)
{
    // depends on the html form id being same as the eeprom field name...
    char *newBn = r.getValue(fieldName);
    if (newBn)
    {
        OmEeprom.put(fieldName, newBn);
    }
}

void OmOtaClass::retrieveWifiConfig()
{
    OmEeprom.get("otaBonjourName", this->otaBonjourName);
    OmEeprom.get("otaWifiSsid", this->otaWifiSsid);
    OmEeprom.get("otaWifiPassword", this->otaWifiPassword);

    OMLOG("1 ota bonjour name %s", this->otaBonjourName);
    OMLOG("1 bonjour name %s", this->bonjourName);
    OMLOG("1 otaWifiSsid %s", this->otaWifiSsid);
    if(this->otaWifiSsid[0] && this->otaBonjourName[0])
    {
        strcpy(this->bonjourName, this->otaBonjourName);
        OMLOG("2 ota bonjour name %s", this->otaBonjourName);
        OMLOG("2 bonjour name %s", this->bonjourName);
    }
}

void OmOtaClass::addWifiConfigForm()
{
    // handler for the text edit entries
    OmWebPages::p->addEepromConfigForm([] (OmXmlWriter & w, int ref1, void *ref2)
                                       {
                                           OmOta.retrieveWifiConfig();
                                       }, OME_GROUP_WIFI_SETUP);
}

const char *OmOtaClass::getIpAddress()
{
    return omIpToString(WiFi.localIP(), true);
}


