#include <WiFiManager.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <SD.h>
#include <SPI.h>
#include <Esp.h>




#define I2C_SDA 25
#define I2C_SCL 26
#define DHT_PIN 16
#define BAT_ADC 33
#define SALT_PIN 34
#define SOIL_PIN 32
#define BOOT_PIN 0
#define POWER_CTRL 4
#define USER_BUTTON 35

#define soil_max 1638
#define soil_min 3285

#define DHT_TYPE DHT11


float luxRead;
float advice;
float soil;

BH1750 lightMeter(0x23);
DHT dht(DHT_PIN, DHT_TYPE);





AsyncWebServer server(8080);

String readTemp() {

  float t = dht.readTemperature();

  if (isnan(t)) {
    Serial.println("Failed to read from  sensor!");
    return "";
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

String readHum() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from  sensor!");
    return "";
  }
  else {
    Serial.println(h);
    return String(h);
  }
}

String readLight() {
  float l = lightMeter.readLightLevel();
  if (isnan(l)) {
    Serial.println("Failed to read from  sensor!");
    return "";
  }
  else {
    Serial.println(l);
    return String(l);
  }
}

String readSoil()
{
  uint16_t soil = analogRead(SOIL_PIN);

  if (map(soil, soil_min, soil_max, 0, 100) <= 0 ) {
    return String(0);
  } else {
    return String(map(soil, soil_min, soil_max, 0, 100));
  }


}


float readBattery()
{
  int vref = 1100;
  uint16_t volt = analogRead(BAT_ADC);


  float battery_voltage = ((float)volt / 4095.0) * 2.0 * 3.3 * (vref) / 1000;

  battery_voltage = battery_voltage * 100;
  if (map(battery_voltage, 416, 290, 100, 0) >= 100) {
    return 100;
  } else {
    return map(battery_voltage, 416, 290, 100, 0);
  }

}


String readSalt()
{
  uint8_t samples = 120;
  uint32_t humi = 0;
  uint16_t array[120];

  for (int i = 0; i < samples; i++)
  {
    array[i] = analogRead(SALT_PIN);
    delay(2);
  }
  std::sort(array, array + samples);
  for (int i = 0; i < samples; i++)
  {
    if (i == 0 || i == samples - 1)
      continue;
    humi += array[i];
  }
  humi /= samples - 2;
  return String(humi);
}

void setup() {


  Serial.begin(115200);

  dht.begin();
  lightMeter.begin();

  pinMode(POWER_CTRL, OUTPUT);
  digitalWrite(POWER_CTRL, 1);
  delay(1000);





  bool wireOk = Wire.begin(I2C_SDA, I2C_SCL);
  if (wireOk)
  {
    Serial.println(F("Wire ok"));
  }
  else
  {
    Serial.println(F("Wire NOK"));
  }
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
  {
    Serial.println(F("BH1750 Advanced begin"));
  }
  else
  {
    Serial.println(F("Error initialising BH1750"));
  }


  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }


  WiFiManager wm;

  bool res;

  res = wm.autoConnect("Flowers At Home", "password"); // password protected ap

  wm.resetSettings();


  if (!res) {

    Serial.println("Failed to connect");

    // ESP.restart();

  }

  // Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html");
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", readTemp().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", readHum().c_str());
  });
  server.on("/light", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", readLight().c_str());
  });

  server.on("/soil", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", readSoil().c_str());
  });

  server.on("/salt", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", readSalt().c_str());
  });

  server.on("/battery", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(readBattery()).c_str());
  });

  server.begin();
}

void loop() {

}
