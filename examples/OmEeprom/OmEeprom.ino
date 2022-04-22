#include "OmEspHelpers.h"

void setup()
{
  Serial.begin(115200);
    printf("\n\n\nOmEeprom Simple Example\n\n");


  // define some eeprom fields
  OmEeprom.addInt8("aByte");
  OmEeprom.addInt32("aLong");
  OmEeprom.addString("aString", 12); // max length of string
  OmEeprom.begin(); // you have ta do this, and cant add any more fields afterwards.
}

void loop() 
{
  static int counter = 0; // we use this to advance through our EEPROM demo in a stately fashion.
  // we only do things on the first handful of steps; you can wear out the EEPROM with repeated writes, after all.
  delay(2000);
  if(counter > 10)
    delay(200000);

  printf(". %d\n", counter);
  counter++;
  int k;
  switch(counter)
  {
    case 1:
      OmEeprom.dumpState("state at power on:");
      break;
      
    case 2:
      k = random(100,200);
      OmEeprom.set("aByte", k);
      OMLOG("setting aByte := %d", k);
      OmEeprom.dumpState("after setting aByte");
      break;

    case 3:
      k = random(1000,2000000);
      OmEeprom.set("aLong", k);
      OMLOG("setting aLong := %d", k);
      OmEeprom.dumpState("after setting aLong");
    break;

    case 4:
      OmEeprom.set("aString", "This string won't fit!");
      OmEeprom.dumpState();
    break;

    case 5:
      OMLOG("will be committed to Eeprom on 8.");
      OMLOG("If you reset before then, changes wont be in the Eeprom.");
      break;

    case 8:
      OMLOG("Writing to atual Eeprom.");
      OMLOG("If you reset now, you'll see the old values.");
      OmEeprom.commit();
      break;
  }
}
