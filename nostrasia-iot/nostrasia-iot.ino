#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

fs::SPIFFSFS &FlashFS = SPIFFS;
#define FORMAT_ON_FAIL true
#define PARAM_FILE "/elements.json"

struct KeyValue {
    String key;
    String value;
};

#include "settings.h"
#include "common.h"
#include "nostr.h"


String config_nsec = "null";
String config_ssid = "null";
String config_password= "null";

// Serial config
int portalPin = 4;

StaticJsonDocument<2500> doc;

void setupConfig() {
    readConfig(doc);
    if(config_ssid == "null"){
        config_ssid = getJsonValue(doc, "config_ssid");
        Serial.println("SSID (memory): " + config_ssid);
    }
    else{
        Serial.println("SSID (hardcoded): " + config_ssid);
    }
    if(config_password == "null"){
        config_password = getJsonValue(doc, "config_password");
        Serial.println("SSID password (memory): " + config_password);
    }
    else{
        Serial.println("SSID password (hardcoded): " + config_password);
    }
    if(config_nsec == "null"){
        config_nsec = getJsonValue(doc, "config_nsec");
        Serial.println("Nostr nsec (memory): " + config_nsec);
    }
    else{
        Serial.println("Nostr nsec (hardcoded): " + config_nsec);
    }
}

String lastPayload = "";
int ledPin = 15;

void setup() {
  Serial.begin(115200);
  Serial.println("boot");

  initTft();
  initLamp();
  writeTextToTft("Booting");

  bool triggerConfig = false;
  pinMode (2, OUTPUT); // To blink on board LED
  FlashFS.begin(FORMAT_ON_FAIL);
  int timer = 0;
  while (timer < 2000) {
      digitalWrite(2, HIGH);
      Serial.println("Detecting portalPin: " + String(touchRead(portalPin)));
      if (touchRead(portalPin) < 60) {
          triggerConfig = true;
          timer = 5000;
      }
      timer = timer + 100;
      delay(150);
      digitalWrite(2, LOW);
      delay(150);
  }
  setupConfig();
  if(triggerConfig == true || config_ssid == "" || config_ssid == "null"){
      Serial.println("Launch serial config");
      configOverSerialPort();
  }
  else{
      WiFi.begin(config_ssid.c_str(), config_password.c_str());
      Serial.print("Connecting to WiFi");
      writeTextToTft("Connecting to WiFi..");
      while (WiFi.status() != WL_CONNECTED) {
          Serial.print(".");
          delay(500);
          digitalWrite(2, HIGH);
          Serial.print(".");
          delay(500);
          digitalWrite(2, LOW);
      }
      Serial.println("WiFi connection etablished!");
      writeTextToTft("Connected to WiFi");

      createIntentReq();
      connectToNostrRelays();
  }
}

void loop() {
  while(WiFi.status() != WL_CONNECTED){
      Serial.println("WiFi disconnected!");
      writeTextToTft("WiFi disconnected!");
      delay(500);
  }

  nostrLoop();

  // reboot every hour. Helps with memory leaks and sometimes, relays disconnect but dont trigger the disconnect event
  if (millis() > 3600000) {
    Serial.println("Rebooting");
    ESP.restart();
  }

  if(lastPayload != "") {
    handlePayload(lastPayload);
    lastPayload = "";
  }

}
