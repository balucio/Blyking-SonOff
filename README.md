# Blynking SonOff - (Based on SonoffBoilerplate)

This is a replacement firmware (Arduino IDE with ESP8266 core needed) for the ESP8266 based Sonoff devices mainly used for Blynk Server.
Use it as a starting block for customizing your Sonoff.

It is based on *tzapu/SonoffBoilerplate*. It use BlynkSimpleEsp8266_WM to handle Wifi connection and Blynk server configurations parameters.
It also handle correctly sensor *hostname*

Please note that is tested with ESP8266 Arduino Board version >= 2.7.4

I disabled OTA because for SonOff basic the code seems too big to handle on the air updates.
