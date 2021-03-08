# Blynking SonOff - (Based on SonoffBoilerplate)

This is a replacement firmware for the ESP8266 based Sonoff devices mainly used for Blynk Server.
Use it as a starting block for customizing your Sonoff.

It is based on *tzapu/SonoffBoilerplate*, but i changed some things and also it use BlynkSimpleEsp8266_WM to handle Wifi connection and Blynk server configurations parameters.
It also handle correctly sensor *hostname*

Please note that is tested with ESP8266 Arduino Board version >= 2.7.4

I disabled OTA because SonOff basic has 1Mb Flash, too small to handle on the air updates.
