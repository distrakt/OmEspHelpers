#include "OmEspHelpers.h"

#ifndef __OmEta__
#define __OmEta__

#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <Updater.h>
#include "OmWebPages.h"

#define _ESPWEBSERVER ESP8266WebServer
#endif

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Update.h>

#define _ESPWEBSERVER WebServer
#endif

typedef enum
{
  OSS_BEGIN = 0,       // once at beginning
  OSS_WIFI_CONNECTING, // while trying to connect
  OSS_WIFI_FAIL,       // if connection did fail. will keep calling this.
  OSS_WIFI_SUCCESS,    // once when connection established
  OSS_IDLE,            // called repeatedly while serving the Update web page
  OSS_UPLOAD_START,    // called once to begin upload, progress = bytes total
  OSS_UPLOADING,       // called repeatedly during upload, progress = bytes so far
  OSS_UPLOADED,        // called once when the upload is complete, progress = bytes got, reboot imminent
} EOtaSetupState;

typedef void (* OtaStatusProc)(EOtaSetupState, unsigned int progress);

void otaSetup(const char *ssid, const char *password, const char *bonjourName);
void otaSetStatusProc(OtaStatusProc statusProc);
void otaLoop(void) ;


class OmOtaClass
{
  private:
    _ESPWEBSERVER *serverPtr = NULL;
    OtaStatusProc statusProc = NULL;
    bool otaMode = false;
    char bonjourName[64];
    bool rebootOnGet = false;

    void doProc(EOtaSetupState state, int progress);
    bool doAWiFiTry(String ssid, String password, int &wifiDots);
    EOtaSetupState lastState = OSS_BEGIN;

    String ssidActuallyConnected;
    long long otaStarted = 0;

  public:
    char *defaultSignature = "OmOta_xyzzy"; // manually reassign before use...

#define OOC_STRING_LEN 20
    // you can just read these.
    char otaBonjourName[OOC_STRING_LEN];
    char otaWifiSsid[OOC_STRING_LEN];
    char otaWifiPassword[OOC_STRING_LEN];

    void addEeFields();

    /** If returns true, skip the rest of your own setup. Uses the whole eeprom, unless you set up eeprom
     * yourself, and call OmOta.addEeFields().
     */
    bool setup(const char *wifiSsid, const char *wifiPassword, const char *wifiBonjour = NULL, OtaStatusProc statusProc = NULL);

    /** Setup using eeprom. optional callback setup.
     */
    bool setup(OtaStatusProc statusProc = NULL);


    /*! Call at the beginning of your loop() function. If it returns true, skip the rest of your loop
       (Any work needed to update displays during setup and upload and such must be handled in your statusProc)
    */
    bool loop();

    /*! Call this to reboot, right now, into update mode.
       (The next boot after this won't be in ota mode.)
    */
    void rebootToOta();
    
    /*! Add an update control to the current page in progress
     */
    void addUpdateControl();

    /*! will return a printable IP Address String only after OSS_WIFI_CONNECT has happened
     */
    const char *getIpAddress();
    
    /** Add form for submitting new Wifi credentials and bonjour name
     */
    void addWifiConfigForm();

    /** after eeprom is fully init'd, call to update the public fields of the wifi config. */
    void retrieveWifiConfig();
};

// magics into existence if you use it.
extern OmOtaClass OmOta;

#endif // __OmEta__
