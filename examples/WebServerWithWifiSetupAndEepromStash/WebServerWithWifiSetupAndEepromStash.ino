#include "OmEspHelpers.h"

OmWebServer s;
OmWebPages p;

/// Callback proc during Over-the-air update
/// On my lamp projects, I show distinctive patterns so you can tell it's getting updated.
void otaStatus(EOtaSetupState state, unsigned int progress)
{
  static EOtaSetupState lastState = (EOtaSetupState)-1;

  if(state != lastState)
  {
    OMLOG("OmOta new status: %d\n", state);
    lastState = state;
  }
}


void setup() 
{
  delay(400);
  Serial.begin(115200);
  delay(400);
  OMLOG("%s", __FILE__);
 
  // to support "Over The Air" updates -- install a new sketch by wifi -- this adds some necessary eeprom config.
  // this includes the wifi and password
  OmOta.addEeFields();

  // Now we'll add some fields, in two groups.
  #define GRP_A 21 // number doesnt matter, just under 200
  #define GRP_B 49 // a different number
  OmEeprom.addInt8("otaMode", OME_GROUP_OTA, NULL);
  OmEeprom.addString("otaBonjourName", OOC_STRING_LEN, OME_GROUP_WIFI_SETUP | OME_FLAG_BONJOUR, "Bonjour name");
  OmEeprom.addString("otaWifiSsid", OOC_STRING_LEN, OME_GROUP_WIFI_SETUP, "Wifi");
  OmEeprom.addString("otaWifiPassword", OOC_STRING_LEN, OME_GROUP_WIFI_SETUP, "Password");

 
  OmEeprom.addInt8("param1", GRP_A, "Param 1 (small number)"); // last is the "pretty name" for display
  OmEeprom.addString("param2", 12, GRP_A, "Param 2 (string)"); // 12 chars max

  OmEeprom.addInt16("param3", GRP_B, "Param 3 (medium number)");
  OmEeprom.addInt16("param4", GRP_B, "Param 4 (big number)");
  OmEeprom.addString("param5", 22, GRP_B, "Param 5 (string)");

  // after defining all the eprom fields, begin() it with a signature.
  // if the signature matches what's in eeprom already, it will read out any existing
  // fields to OmEeprom's in-memory space.
  // Using different signatures on different sketches can help prevent old settings from messing
  // with the new sketch. Or you could use the same signature on all your sketches, and make sure
  // the field names aren't problematic.
  
  OmEeprom.begin("WSExample");

  // Ritual incantation
  if (OmOta.setup(NULL, NULL, NULL, otaStatus)) return;


  p.setBuildDateAndTime(__DATE__, __TIME__, __FILE__);

  p.beginPage("main");
  p.addEepromConfigForm(NULL, /*OmHtmlProc eepromConfigCallbackProc*/ GRP_A);
  p.addEepromConfigForm(NULL, /*OmHtmlProc eepromConfigCallbackProc*/ GRP_B);
  

  p.beginPage("config");
  OmOta.addUpdateControl();
  OmOta.addWifiConfigForm();

  s.setHandler(p);

}

void loop() 
{
  if(OmOta.loop()) return;

  delay(50);
  s.tick();
}
