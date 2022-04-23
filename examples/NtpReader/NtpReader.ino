
/*
 * 2019-08-05 dvb
 * simplest example reading NTP to get time of day.
 */

#include <OmEspHelpers.h>

// These includes arent used in this file, but tells Arduino IDE that this project uses them.
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

OmWebServer s; // not serving any pages, put this brings up the wifi.
OmNtp ntp;

void setup()
{
  Serial.begin(115200);
  Serial.printf("\n\n\nWelcome to the Sketch\n\n");
  
  s.addWifi("omino warp", "0123456789");
  s.setNtp(&ntp);

  ntp.setTimeZone(-8); // CA, when in Pacific Standard Time, winter
                       // (not Pacific Daylight Time, -7, summer)
/*
 *
You can get the correct time zone if you run a short PHP script on
a web server you control, in your time zone. Here is the script:

    <?
    print date("Y-m-d H:i:s");
    print " + ";
    print date("e");
    ?>

And then you add this line here in setup():

    ntp.setTimeUrl("http://myserver.com/time.php");

Because ESP8266 web client library is blocking, we still prefer
the NTP protocol running on asynch UDP for the periodic synchronization,
with apologies and thanks to the time servers.
*
*/
}

int ticks = 0;
void loop()
{
  delay(10);
  s.tick();

  ticks++;
  if(ticks % 877 == 0)
    Serial.printf("tick %4d, time: %s\n", ticks, ntp.getTimeString());
}
