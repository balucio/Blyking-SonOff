/*
   1MB flash sizee

   sonoff header
   1 - vcc 3v3
   2 - rx
   3 - tx
   4 - gnd
   5 - gpio 14

   esp8266 connections
   gpio  0 - button
   gpio 12 - relay
   gpio 13 - green led - active low
   gpio 14 - pin 5 on header

*/


// DEBUG

// #define BLYNK_PRINT Serial    
#define BLYNK_WM_DEBUG 0
#define DOUBLERESETDETECTOR_DEBUG     false

//// CONFIG PORTAL
#define CONFIG_PORTAL_IPADDRESS 192,168,4,1

#define   SONOFF_BUTTON             0
#define   SONOFF_INPUT              14
#define   SONOFF_LED                13
#define   SONOFF_AVAILABLE_CHANNELS 1
const int SONOFF_RELAY_PINS[4] =    {12, 12, 12, 12};

/*
 * if this is false, led is used to signal startup state, then always on
 * if it s true, it is used to signal startup state, then mirrors relay state
 * S20 Smart Socket works better with it false
 */
#define SONOFF_LED_RELAY_STATE      true

// UsingEEPROM to store data
#define USE_LITTLEFS    true
#define USE_SPIFFS      false
#define CurrentFileFS     "LittleFS"


// Force some params in Blynk, only valid for library version 1.0.1 and later
#define TIMEOUT_RECONNECT_WIFI                    10000L
#define STATUS_CHECK_INTERVAL                     15000L
#define RESET_IF_CONFIG_TIMEOUT                   true
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET    3


/* needed to set hostname */
extern "C" {
#include "user_interface.h"
}

#include <BlynkSimpleEsp8266_WM.h>

// Comment this out to disable prints and save space

/*
#include <BlynkSimpleEsp8266.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/zhouhan0126/WIFIMANAGER-ESP32

#include <EEPROM.h>
*/

// 
bool LOAD_DEFAULT_CONFIG_DATA = false;

Blynk_WM_Configuration defaultConfig =
{
  "NonSSL",            // SSL
  "sapacasa",          // ESSID1
  "",                  // ESSID1 PASS
  "",                  // EESID2
  "",                  // EEDID2 PASS    
  "blynk.sapacasa.it", // URL Blynk Server 1
  "token",             // Blynk Token 1
  "",                  // URL Blynk Server 2
  "",                  // Blynk Token 2
  8080,                // Blynk Port (9443 SSL)
  "sonoff_switch",     // Device name
  0                    // Checksum non usato
};

char* HOSTNAME = defaultConfig.board_name;

// 
#define BOOTSTATE_LEN                3
char    BOOTSTATE[BOOTSTATE_LEN+1] = "off";

#define USE_DYNAMIC_PARAMETERS  true

#define BOOT_STATE_LEN      4
char BOOT_STATE[BOOT_STATE_LEN + 1]   = "off";


MenuItem myMenuItems [] = {
  { "bste", "Stato iniziale", BOOT_STATE, BOOT_STATE_LEN }
};

uint16_t NUM_MENU_ITEMS = sizeof(myMenuItems) / sizeof(MenuItem);  //MenuItemSize;


// #include <ArduinoOTA.h>

//for LED status
#include <Ticker.h>
Ticker ticker;


const int CMD_WAIT = 0;
const int CMD_BUTTON_CHANGE = 1;

int cmd = CMD_WAIT;
//int relayState = HIGH;

//inverted button state
int buttonState = HIGH;

static long startPress = 0;

void tick() {
  //toggle state
  int state = digitalRead(SONOFF_LED);  // get the current state of GPIO1 pin
  digitalWrite(SONOFF_LED, !state);     // set pin to the opposite state
}


void setupPortal() {

  // Setup WLAN and Blynk
  byte mac[6];
  char macAddr[6];
  WiFi.macAddress(mac);

  
  String hostname = Blynk.getBoardName();
  char blynk_hostname[hostname.length() + 1];
  hostname.toCharArray(blynk_hostname, sizeof(blynk_hostname));

  WiFi.hostname(hostname);

  sprintf(macAddr, "%2X%2X%2X", mac[3], mac[4], mac[5]);
  String config_portal_pass = String(HOSTNAME);
  String config_portal_ssid = String(HOSTNAME) + String(macAddr);
   
  Blynk.setConfigPortal ( config_portal_ssid, config_portal_pass);
  Blynk.setConfigPortalIP ( IPAddress ( CONFIG_PORTAL_IPADDRESS ) );
  Blynk.setConfigPortalChannel(0);
  
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
  Serial.print( "\nSetting up WLAN and Blynk " );
  Serial.print ( "Blynk.setConfigPortal(" ); 
  Serial.print ( config_portal_ssid ); Serial.print ( "," );  
  Serial.print ( "  Config Portal will be found at IP: " ); Serial.print ( IPAddress ( CONFIG_PORTAL_IPADDRESS ) );
  Serial.print ( "\n Hostname: "); 
  Serial.print ( blynk_hostname ); 
  Serial.println ( "\n" );
#endif
  
  Blynk.begin(blynk_hostname); // DHCP (router) device name
  
}

void check_status()
{
  static unsigned long checkstatus_timeout = 0;

  if ( (millis() <= checkstatus_timeout) && (checkstatus_timeout !=  0) )
    return;

  if ( Blynk.connected() ) {
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
    Serial.println ("\nBlynk connected");
    Serial.println ("Board Name : " + Blynk.getBoardName());
    Serial.println ( "Blynk connected just fine" ); 
    Serial.print   ( "  IP address  " ); Serial.println ( WiFi.localIP() ) ;
    Serial.print   ( "  MAC address " ); Serial.println ( WiFi.macAddress() );  
    Serial.println ();
  #endif  
    ticker.detach();

  } else {
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
    Serial.println ( "Blynk NOT CONNECTED \n\n" );
#endif
    ticker.attach(0.6, tick);
  }

  checkstatus_timeout = millis() + STATUS_CHECK_INTERVAL;
}

void updateBlynk(int channel) {
  int state = digitalRead(SONOFF_RELAY_PINS[channel]);
  Blynk.virtualWrite(channel * 5 + 4, state * 255);
}

void setState(int state, int channel) {
  //relay
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
  Serial.println("Set state " + String(state) + " channel " + String(channel));
#endif  
  digitalWrite(SONOFF_RELAY_PINS[channel], state);

  //led
  if (SONOFF_LED_RELAY_STATE) {
    digitalWrite(SONOFF_LED, (state + 1) % 2); // led is active low
  }

  //blynk
  updateBlynk(channel);
}

void turnOn(int channel = 0) {
  int relayState = HIGH;
  setState(relayState, channel);
}

void turnOff(int channel = 0) {
  int relayState = LOW;
  setState(relayState, channel);
}

ICACHE_RAM_ATTR void toggleState() {
  cmd = CMD_BUTTON_CHANGE;
}

void toggle(int channel = 0) {
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
  Serial.println("toggle state");
  Serial.println(digitalRead(SONOFF_RELAY_PINS[channel]));
#endif
  int relayState = digitalRead(SONOFF_RELAY_PINS[channel]) == HIGH ? LOW : HIGH;
  setState(relayState, channel);
}

void restart() {
  //TODO turn off relays before restarting
  ESP.reset();
  delay(1000);
}

void reset() {
  //reset settings to defaults
  //TODO turn off relays before restarting
  /*
    WMSettings defaults;
    settings = defaults;
    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();
  */
  //reset wifi credentials
  WiFi.disconnect();
  delay(1000);
  Blynk.clearConfigData();
  delay(2000);
  ESP.reset();
  delay(1000);

  
}

/* ON OFF BY PARAM VPIN 5*/
BLYNK_WRITE(5) {
  int a = param.asInt();
  if (a) {
    turnOn();
  } else {
    turnOff();
  }
}

/* ON OFF BY PARAM VPIN 5*/
BLYNK_READ(5) {
  Blynk.virtualWrite(5, digitalRead(0));
}

/**********
 * VPIN % 5
 * 0 off
 * 1 on
 * 2 toggle
 * 3 value
 * 4 led
 ***********/

BLYNK_WRITE_DEFAULT() {
  int pin = request.pin;
  int channel = pin / 5;
  int action = pin % 5;
  int a = param.asInt();
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
  Serial.println("WRITE PIN "  + String(pin));
  Serial.println("WRITE CHANNEL "  + String(channel));
  Serial.println("WRITE ACTION "  + String(action));
  Serial.println("WRITE PARAM A "  + String(a));
#endif
  if (a != 0) {
    
    switch(action) {
      case 0:
        turnOff(channel);
        break;
      case 1:
        turnOn(channel);
        break;
      case 2:
        toggle(channel);
        break;
      default:
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
        Serial.print("unknown action");
        Serial.print(action);
        Serial.print(channel);
#endif
        break;
    }
  }
}

BLYNK_READ_DEFAULT() {
  // Generate random response
  int pin = request.pin;
  int channel = pin / 5;
  int action = pin % 5;
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
  Serial.println("READ PIN "  + String(pin));
  Serial.println("READ CHANNEL "  + String(channel));
  Serial.println("READ ACTION "  + String(action));
  Serial.println("READ RELAY PIN "  + String(SONOFF_RELAY_PINS[channel]));
#endif
  Blynk.virtualWrite(pin, digitalRead(SONOFF_RELAY_PINS[channel]));

}

//restart - button
BLYNK_WRITE(30) {
  int a = param.asInt();
  if (a != 0) {
    restart();
  }
}

//reset - button
BLYNK_WRITE(31) {
  int a = param.asInt();
  if (a != 0) {
    reset();
  }
}


void setup()
{
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
  Serial.begin(115200);
  while (!Serial);
#endif

  //set led pin as output
  pinMode(SONOFF_LED, OUTPUT);
  setupPortal();

  char *bootState = myMenuItems[0].pdata;

  //setup button
  pinMode(SONOFF_BUTTON, INPUT);
  attachInterrupt(SONOFF_BUTTON, toggleState, CHANGE);

  //setup relay
  //TODO multiple relays
  pinMode(SONOFF_RELAY_PINS[0], OUTPUT);

   //TODO this should move to last state maybe
   //TODO multi channel support

  if (strcmp(bootState, "on") == 0) {
    turnOn();
  } else {
    turnOff();
  }

  //setup led
  if (!SONOFF_LED_RELAY_STATE) {
    digitalWrite(SONOFF_LED, LOW);
  }
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
  Serial.println("done setup");
#endif
}

void loop() {

  Blynk.run();
  check_status();


  //delay(200);
  //Serial.println(digitalRead(SONOFF_BUTTON));
  switch (cmd) {
    case CMD_WAIT:
      break;
    case CMD_BUTTON_CHANGE:
      int currentState = digitalRead(SONOFF_BUTTON);
      if (currentState != buttonState) {
        if (buttonState == LOW && currentState == HIGH) {
          long duration = millis() - startPress;
          if (duration < 1000) {
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
            Serial.println("short press - toggle relay");
#endif
            toggle();
          } else if (duration < 5000) {
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
            Serial.println("medium press - reset");
#endif
            restart();
          } else if (duration < 60000) {
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
            Serial.println("long press - reset settings");
#endif           
            reset();
          }
        } else if (buttonState == HIGH && currentState == LOW) {
          startPress = millis();
        }
        buttonState = currentState;
      }
      break;
  }
}
