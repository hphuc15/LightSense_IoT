#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <WiFi.h>
#include <WiFiManager.h>

// ----------------- CONFIG ----------------
#define SDA_PIN 21
#define SCL_PIN 22
#define WIFI_BUTTON_PIN 4


void wifiConfig()
{
  WiFiManager wm;
  wm.setConfigPortalTimeout(300); // 5 minutes

  if(!wm.startConfigPortal("ESP32-AP"))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    Serial.println("Reconnecting...");
    ESP.restart();
  }

  Serial.println("connected to wifi!");
}




BH1750 lightMeter;

void setup() {
  Serial.begin(115200);
  
  /* ------------------------------------- WiFi ------------------------------------- */
  pinMode(WIFI_BUTTON_PIN, INPUT_PULLUP);
  if(digitalRead(WIFI_BUTTON_PIN) == LOW)
  {
    Serial.println("WiFi Config Mode");
    wifiConfig();
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  /* -------------------------------------------------------------------------------- */



  Wire.begin(SDA_PIN, SCL_PIN);

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)) {
    Serial.println("BH1750 initialized successfully");
  } else {
    Serial.println("Error: BH1750 not found!");
  }
}

void loop() {
  if (lightMeter.measurementReady()) {
    float lux = lightMeter.readLightLevel();
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
  } else {
    Serial.println("Waiting for measurement...");
  }

  delay(1000);
}
 