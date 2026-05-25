#include <WiFi.h>
#include <WebServer.h>
#include "FS.h"
#include "SPIFFS.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

const char* ssid = "MIST";
const char* password = "Il0veMIST";

// NRF24L01+ Pins
#define CE_PIN  4
#define CSN_PIN 5
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

// Struct to Hold Sensor Data
struct SensorData {
    float temperature;
    float humidity;
    int16_t soilMoisture;
    int16_t waterLevel;
    float pH;
    bool isRaining;
    bool pumpStatus;
} __attribute__((packed));

volatile SensorData sensorData = {0.0, 0.0, 0, 0, 0.0, false, false};

// Create Web Server
WebServer server(80);

// Function to Print Sensor Data to Serial Monitor
void printSensorData() {
    Serial.print("Temperature: ");
    Serial.print(sensorData.temperature);
    Serial.print(" °C | Humidity: ");
    Serial.print(sensorData.humidity);
    Serial.print("% | Soil Moisture: ");
    Serial.print(map(sensorData.soilMoisture, 730, 1022, 100, 0)); // ✅ Convert to %
    Serial.print("% | Water Level: ");
    Serial.print(map(sensorData.waterLevel, 0, 588, 0, 100));   // ✅ Convert to %
    Serial.print("% | pH: ");
    Serial.print(sensorData.pH);
    Serial.print(" | Rain: ");
    Serial.print(sensorData.isRaining ? "Yes" : "No");
    Serial.print(" | Pump: ");
    Serial.println(sensorData.pumpStatus ? "ON" : "OFF");
}

// Serve Static Files
void serveFile(WebServer &server, const char *path, const char *contentType) {
    File file = SPIFFS.open(path, "r");
    if (!file) {
        server.send(404, "text/plain", "File Not Found");
        return;
    }
    server.streamFile(file, contentType);
    file.close();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ESP32...");

    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed!");
        return;
    }

    // Initialize NRF24L01
    SPI.begin(18, 19, 23);   // SCK = GPIO18, MISO = GPIO19, MOSI = GPIO23
    if (!radio.begin()) {
        Serial.println("NRF24L01 not detected!");
        while (1);
    }
    radio.openReadingPipe(0, address);
    radio.setPALevel(RF24_PA_LOW);
    radio.startListening();
    Serial.println("NRF24L01 Listening for Data...");

    // Serve HTML pages
    server.on("/", []() { serveFile(server, "/register.html", "text/html"); });
    server.on("/login", []() { serveFile(server, "/login.html", "text/html"); });
    server.on("/dashboard", []() { serveFile(server, "/index.html", "text/html"); });

    // Serve sensor data as JSON
    server.on("/data", []() {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      String json = "{\"temperature\": " + String(sensorData.temperature) + 
                  ", \"humidity\": " + String(sensorData.humidity) + 
                  ", \"soilMoisture\": " + String(map(sensorData.soilMoisture, 0, 1023, 100, 0)) +  // ✅ Convert to %
                  ", \"waterLevel\": " + String(map(sensorData.waterLevel, 0, 1023, 0, 100)) +      // ✅ Convert to %
                  ", \"pH\": " + String(sensorData.pH) + 
                  ", \"isRaining\": " + String(sensorData.isRaining ? "true" : "false") + 
                  ", \"pumpStatus\": " + String(sensorData.pumpStatus ? "true" : "false") + "}";
    
      server.send(200, "application/json", json);
    });

    // Serve images for register and login pages
    server.on("/register-image", HTTP_GET, []() { serveFile(server, "/wlc.png", "image/png"); });
    server.on("/login-image", HTTP_GET, []() { serveFile(server, "/ha.png", "image/png"); });
    server.on("/iconreg-image", HTTP_GET, []() { serveFile(server, "/iconreg.png", "image/png"); });
    server.on("/iconlog-image", HTTP_GET, []() { serveFile(server, "/iconlog.png", "image/png"); });
    server.on("/ESP-image", HTTP_GET, []() { serveFile(server, "/ESP.png", "image/png"); });
    server.begin();
    Serial.println("Web Server Started!");
}

void loop() {
    server.handleClient();

    if (radio.available()) {
        SensorData receivedData;
        radio.read(&receivedData, sizeof(receivedData));

        sensorData.temperature = receivedData.temperature;
        sensorData.humidity = receivedData.humidity;
        sensorData.soilMoisture = receivedData.soilMoisture;
        sensorData.waterLevel = receivedData.waterLevel;
        sensorData.pH = receivedData.pH;
        sensorData.isRaining = receivedData.isRaining;
        sensorData.pumpStatus = receivedData.pumpStatus;

        Serial.print("Received Data -> ");
        printSensorData();  // Print sensor data to Serial Monitor
        delay(100);
    }
}
