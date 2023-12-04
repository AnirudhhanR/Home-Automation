/*#ifdef ENABLE_DEBUG
       #define DEBUG_ESP_PORT Serial
       #define NODEBUG_WEBSOCKETS
       #define NDEBUG
#endif 
*/
#include <Arduino.h>
#include <WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

#include <map>

#define WIFI_SSID         "Bill Wi the Science Fi"    
#define WIFI_PASS         "smilies2020"
#define APP_KEY           "966b9ab6-e08c-40e2-b6be-bdf75bda04d3"      
#define APP_SECRET        "dde96dad-ca9e-42da-ae4b-ed0c143fc81e-7aa15d0c-ea85-4c0c-bf0e-97d532a46a85" 

#define device_ID_1   "62682881753dc5aab4b39d8c"
#define device_ID_2   "626828aa753dc5aab4b39daf"



#define RelayPin1 26  //D22
#define RelayPin2 27  //D5


#define SwitchPin1 22  //D26
#define SwitchPin2 5  //D27

#define wifiLed   2   //D2

#define TACTILE_BUTTON 1

#define BAUD_RATE   9600

#define DEBOUNCE_TIME 250

typedef struct {     
  int relayPIN;
  int flipSwitchPIN;
} deviceConfig_t;


std::map<String, deviceConfig_t> devices = {
    //{deviceId, {relayPIN,  flipSwitchPIN}}
    {device_ID_1, {  RelayPin1, SwitchPin1 }},
    {device_ID_2, {  RelayPin2, SwitchPin2 }}     
};

typedef struct {     
  String deviceId;
  bool lastFlipSwitchState;
  unsigned long lastFlipSwitchChange;
} flipSwitchConfig_t;

std::map<int, flipSwitchConfig_t> flipSwitches;    
void setupRelays() { 
  for (auto &device : devices) {           
    int relayPIN = device.second.relayPIN; 
    pinMode(relayPIN, OUTPUT);             
    digitalWrite(relayPIN, HIGH);
  }
}

void setupFlipSwitches() {
  for (auto &device : devices)  {                     
    flipSwitchConfig_t flipSwitchConfig;              

    flipSwitchConfig.deviceId = device.first;         
    flipSwitchConfig.lastFlipSwitchChange = 0;        
    flipSwitchConfig.lastFlipSwitchState = true;     

    int flipSwitchPIN = device.second.flipSwitchPIN;  

    flipSwitches[flipSwitchPIN] = flipSwitchConfig;   
    pinMode(flipSwitchPIN, INPUT_PULLUP);                   
  }
}

bool onPowerState(String deviceId, bool &state)
{
  Serial.printf("%s: %s\r\n", deviceId.c_str(), state ? "on" : "off");
  int relayPIN = devices[deviceId].relayPIN; // get the relay pin for corresponding device
  digitalWrite(relayPIN, !state);             // set the new relay state
  return true;
}

void handleFlipSwitches() {
  unsigned long actualMillis = millis();                                         
  for (auto &flipSwitch : flipSwitches) {                                         
    unsigned long lastFlipSwitchChange = flipSwitch.second.lastFlipSwitchChange;  

    if (actualMillis - lastFlipSwitchChange > DEBOUNCE_TIME) {                    

      int flipSwitchPIN = flipSwitch.first;                                     
      bool lastFlipSwitchState = flipSwitch.second.lastFlipSwitchState;          
      bool flipSwitchState = digitalRead(flipSwitchPIN);                      
      if (flipSwitchState != lastFlipSwitchState) {                               
#ifdef TACTILE_BUTTON
        if (flipSwitchState) {                                                  
#endif      
          flipSwitch.second.lastFlipSwitchChange = actualMillis;               
          String deviceId = flipSwitch.second.deviceId;                         
          int relayPIN = devices[deviceId].relayPIN;                          
          bool newRelayState = !digitalRead(relayPIN);                            
          digitalWrite(relayPIN, newRelayState);                                

          SinricProSwitch &mySwitch = SinricPro[deviceId];                        
          mySwitch.sendPowerStateEvent(!newRelayState);                           
#ifdef TACTILE_BUTTON
        }
#endif      
        flipSwitch.second.lastFlipSwitchState = flipSwitchState;                
      }
    }
  }
}

void setupWiFi()
{
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    delay(250);
  }
  digitalWrite(wifiLed, HIGH);
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

void setupSinricPro()
{
  for (auto &device : devices)
  {
    const char *deviceId = device.first.c_str();
    SinricProSwitch &mySwitch = SinricPro[deviceId];
    mySwitch.onPowerState(onPowerState);
  }

  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);

  setupRelays();
  setupFlipSwitches();
  setupWiFi();
  setupSinricPro();
}

void loop()
{
  SinricPro.handle();
  handleFlipSwitches();
}
