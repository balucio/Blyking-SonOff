/*
   1MB flash sizee

   sonoff basic header  (1 - accanto switch)
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
// #define BLYNK_WM_DEBUG 1
#define DOUBLERESETDETECTOR_DEBUG     false

//// CONFIG PORTAL
#define CONFIG_PORTAL_IPADDRESS 192,168,4,1

#define   SONOFF_BUTTON          0
#define   SONOFF_INPUT           14
#define   SONOFF_LED             13
#define   SONOFF_RELAY_PIN       12

/*
 * if this is false, led is used to signal startup state, then always on
 * if it s true, it is used to signal startup state, then mirrors relay state
 * S20 Smart Socket works better with it false
 */
#define SONOFF_LED_RELAY_STATE      true

// UsingEEPROM to store data
#define USE_LITTLEFS    true
#define USE_SPIFFS      false
#define CurrentFileFS   "LittleFS"


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

#define   BLYNK_RELAY_VPIN       V5
#define   BLYNK_RESTART_VPIN     V30
#define   BLYNK_RESET_VPIN       V31



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


//for LED status
#include <Ticker.h>
Ticker ticker;


const int CMD_WAIT = 0;
const int CMD_BUTTON_CHANGE = 1;

int cmd = CMD_WAIT;

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
  Serial.print( "\nImposto Rete Wifi e Blynk " );
  Serial.print ( "Blynk.setConfigPortal(" ); 
  Serial.print ( config_portal_ssid ); Serial.print ( "," );  
  Serial.print ( "  IP del portale è: " ); Serial.print ( IPAddress ( CONFIG_PORTAL_IPADDRESS ) );
  Serial.print ( "\n Nome host: "); 
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
    Serial.println ("\nConnesso a Blynk");
    Serial.println ("Nome dispositivo : " + Blynk.getBoardName());
    Serial.print   ( "  Indirizzo IP " ); Serial.println ( WiFi.localIP() ) ;
    Serial.print   ( "  Indirizzo MAC " ); Serial.println ( WiFi.macAddress() );  
    Serial.println ();
#endif  

    ticker.detach();

  } else {
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
    Serial.println ( "NON CONNESSO \n\n" );
#endif
    ticker.attach(0.6, tick);
  }

  checkstatus_timeout = millis() + STATUS_CHECK_INTERVAL;
}

void setState(int state, int pin = SONOFF_RELAY_PIN) {
  //relay
  
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
  Serial.println("Imposto stato Rele " + String(state) + " PIN/VPIN " + String(SONOFF_RELAY_PIN) + "/" + String(BLYNK_RELAY_VPIN) + " e commuto led.");
#endif
  digitalWrite(pin, state);

  //led
  if (SONOFF_LED_RELAY_STATE) {
    digitalWrite(SONOFF_LED, (state + 1) % 2); // led is active low
  }

  //blynk
  Blynk.virtualWrite(BLYNK_RELAY_VPIN, state);
}

void turnOn(int pin = SONOFF_RELAY_PIN) {
  setState(HIGH, pin);
}

void turnOff(int pin = SONOFF_RELAY_PIN) {
  setState(LOW, pin);
}

ICACHE_RAM_ATTR void toggleState() {
  cmd = CMD_BUTTON_CHANGE;
}

void toggle(int pin = SONOFF_RELAY_PIN) {

  int state = digitalRead(pin);
  int relayState = state == HIGH ? LOW : HIGH;
  setState(relayState, pin);
}

void restart() {
  //TODO turn off relays before restarting
  ESP.reset();
  delay(1000);
}

void reset() {

  //reset wifi credentials
  WiFi.disconnect();
  delay(1000);
  Blynk.clearConfigData();
  delay(2000);
  ESP.reset();
  delay(1000);

  
}

/* ON OFF BY PARAM VPIN 5 */
BLYNK_WRITE(BLYNK_RELAY_VPIN) {

  int p = param.asInt() % 3;

  switch (p) {

    case 0: turnOff(); break;
    case 1: turnOn(); break;
    case 2: toggle(); break;
    default:

#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
       Serial.println("Valore non supportato");
#endif
       break;

  }
}

/* ON OFF BY PARAM VPIN 5 */
BLYNK_READ(BLYNK_RELAY_VPIN) {
  int state = digitalRead(SONOFF_RELAY_PIN);
  Blynk.virtualWrite(BLYNK_RELAY_VPIN, state);
}


//restart - button
BLYNK_WRITE(BLYNK_RESTART_VPIN) {
  int a = param.asInt();
  if (a != 0) {
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
    Serial.println("RIAVVIO");
#endif
    restart();
  }
}

//reset - button
BLYNK_WRITE(BLYNK_RESET_VPIN) {
  int a = param.asInt();
  if (a != 0) {
#if defined BLYNK_WM_DEBUG  && BLYNK_WM_DEBUG 
    Serial.println("RESET");
#endif

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
  pinMode(SONOFF_RELAY_PIN, OUTPUT);

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
