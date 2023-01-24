#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <SD.h>
#include <SPI.h>
#include <Esp.h>



const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

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

BH1750 lightMeter(0x23); //0x23
DHT dht(DHT_PIN, DHT_TYPE);


AsyncWebServer server(80);

AsyncEventSource events("/events");


JSONVar readings;

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

uint16_t readSoil()
{
  uint16_t soil = analogRead(SOIL_PIN);
  return map(soil, soil_min, soil_max, 0, 100);
}

// READ Battery
float readBattery()
{
  int vref = 1100;
  uint16_t volt = analogRead(BAT_ADC);
  // Serial.print("Volt direct ");
  // Serial.println(volt);
  float battery_voltage = ((float)volt / 4095.0) * 2.0 * 3.3 * (vref) / 1000;
  Serial.print("Battery Voltage: ");
  Serial.println(battery_voltage);
  battery_voltage = battery_voltage * 100;
  return map(battery_voltage, 416, 290, 100, 0);
}




String getSensorReadings(){
  readings["lux"] = String(lightMeter.readLightLevel());
  readings["soil"] =  String(readSoil());
  readings["temperature"] =  String(dht.readTemperature());
  readings["humidity"] =  String(dht.readHumidity());
  readings["battery"] =  String(readBattery());
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initBME();
  initWiFi();
  initSPIFFS();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
  }

  
}
