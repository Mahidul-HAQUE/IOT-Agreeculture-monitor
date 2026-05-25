#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>          // ← NEW: mDNS so the device is reachable as agrisense.local
#include "FS.h"
#include "SPIFFS.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ─────────────────────────────────────────────────────────────
// WiFi credentials — update these for your network
// ─────────────────────────────────────────────────────────────
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ─────────────────────────────────────────────────────────────
// mDNS hostname — device will be reachable as agrisense.local
// regardless of which IP DHCP assigns
// ─────────────────────────────────────────────────────────────
const char* MDNS_HOSTNAME = "agrisense";

// ─────────────────────────────────────────────────────────────
// OPTIONAL: static IP — uncomment all four lines below to fix
// the ESP32 to a permanent IP instead of relying on DHCP.
// Make sure the IP is outside your router's DHCP range.
// ─────────────────────────────────────────────────────────────
// IPAddress staticIP(192, 168, 1, 200);
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnet(255, 255, 255, 0);
// IPAddress dns(8, 8, 8, 8);

// NRF24L01+ pins
#define CE_PIN  4
#define CSN_PIN 5
RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

// Sensor data struct (must match the transmitter exactly)
struct SensorData {
    float   temperature;
    float   humidity;
    int16_t soilMoisture;
    int16_t waterLevel;
    float   pH;
    bool    isRaining;
    bool    pumpStatus;
} __attribute__((packed));

volatile SensorData sensorData = {0.0, 0.0, 0, 0, 0.0, false, false};

WebServer server(80);

// ─────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────
void addCORSHeaders() {
    server.sendHeader("Access-Control-Allow-Origin",  "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void serveFile(const char* path, const char* contentType) {
    File file = SPIFFS.open(path, "r");
    if (!file) {
        server.send(404, "text/plain", "File Not Found");
        return;
    }
    server.streamFile(file, contentType);
    file.close();
}

void printSensorData() {
    Serial.print("Temp: ");      Serial.print(sensorData.temperature);
    Serial.print("°C  Hum: ");   Serial.print(sensorData.humidity);
    Serial.print("%  Soil: ");   Serial.print(map(sensorData.soilMoisture, 730, 1022, 100, 0));
    Serial.print("%  Water: ");  Serial.print(map(sensorData.waterLevel, 0, 588, 0, 100));
    Serial.print("%  pH: ");     Serial.print(sensorData.pH);
    Serial.print("  Rain: ");    Serial.print(sensorData.isRaining ? "Yes" : "No");
    Serial.print("  Pump: ");    Serial.println(sensorData.pumpStatus ? "ON" : "OFF");
}

// ─────────────────────────────────────────────────────────────
// setup
// ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println("\nStarting AgriSense ESP32...");

    // ── Static IP (optional) ───────────────────────────────
    // Uncomment the next line if you enabled the static IP block above:
    // WiFi.config(staticIP, gateway, subnet, dns);

    // ── Connect to WiFi ────────────────────────────────────
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address : "); Serial.println(WiFi.localIP());

    // ── Start mDNS ─────────────────────────────────────────
    // Device will be reachable at http://agrisense.local/
    // on any phone on the same network — no IP address needed.
    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("mDNS started → http://agrisense.local/");
    } else {
        Serial.println("mDNS failed to start (IP-based access still works)");
    }

    // ── SPIFFS ─────────────────────────────────────────────
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed!");
        return;
    }

    // ── NRF24L01 ───────────────────────────────────────────
    SPI.begin(18, 19, 23);   // SCK=18, MISO=19, MOSI=23
    if (!radio.begin()) {
        Serial.println("NRF24L01 not detected! Check wiring.");
        while (1);
    }
    radio.openReadingPipe(0, address);
    radio.setPALevel(RF24_PA_LOW);
    radio.startListening();
    Serial.println("NRF24L01 listening...");

    // ── Routes ─────────────────────────────────────────────

    // Identity probe endpoint — used by the app auto-discovery to
    // confirm it has found the right device before committing to an IP.
    server.on("/ping", HTTP_GET, []() {
        addCORSHeaders();
        server.send(200, "application/json",
            "{\"device\":\"agrisense\",\"version\":\"1.0\","
            "\"ip\":\"" + WiFi.localIP().toString() + "\"}");
    });

    // Live sensor data
    server.on("/data", HTTP_GET, []() {
        addCORSHeaders();
        String json =
            "{\"temperature\":"  + String(sensorData.temperature)                          +
            ",\"humidity\":"     + String(sensorData.humidity)                             +
            ",\"soilMoisture\":" + String(map(sensorData.soilMoisture, 730, 1022, 100, 0)) +
            ",\"waterLevel\":"   + String(map(sensorData.waterLevel,   0,   588, 0,   100))+
            ",\"pH\":"           + String(sensorData.pH)                                   +
            ",\"isRaining\":"    + String(sensorData.isRaining  ? "true" : "false")        +
            ",\"pumpStatus\":"   + String(sensorData.pumpStatus ? "true" : "false")        +
            "}";
        server.send(200, "application/json", json);
    });

    // Handle CORS preflight
    server.on("/data",  HTTP_OPTIONS, []() { addCORSHeaders(); server.send(204); });
    server.on("/ping",  HTTP_OPTIONS, []() { addCORSHeaders(); server.send(204); });

    // HTML pages
    server.on("/",          []() { serveFile("/register.html", "text/html"); });
    server.on("/login",     []() { serveFile("/login.html",    "text/html"); });
    server.on("/dashboard", []() { serveFile("/index.html",    "text/html"); });

    // Static assets
    server.on("/register-image",  HTTP_GET, []() { serveFile("/wlc.png",     "image/png"); });
    server.on("/iconreg-image",   HTTP_GET, []() { serveFile("/iconreg.png", "image/png"); });
    server.on("/iconlog-image",   HTTP_GET, []() { serveFile("/iconlog.png", "image/png"); });
    server.on("/ESP-image",       HTTP_GET, []() { serveFile("/ESP.png",     "image/png"); });

    server.begin();
    Serial.println("Web server started!");
    Serial.println("─────────────────────────────────────────");
    Serial.println("Access dashboard at:");
    Serial.println("  http://agrisense.local/  (hostname — preferred)");
    Serial.print  ("  http://"); Serial.print(WiFi.localIP()); Serial.println("/  (IP address)");
    Serial.println("─────────────────────────────────────────");
}

// ─────────────────────────────────────────────────────────────
// loop
// ─────────────────────────────────────────────────────────────
void loop() {
    server.handleClient();
    MDNS.update();   // keep mDNS alive

    if (radio.available()) {
        SensorData received;
        radio.read(&received, sizeof(received));

        sensorData.temperature  = received.temperature;
        sensorData.humidity     = received.humidity;
        sensorData.soilMoisture = received.soilMoisture;
        sensorData.waterLevel   = received.waterLevel;
        sensorData.pH           = received.pH;
        sensorData.isRaining    = received.isRaining;
        sensorData.pumpStatus   = received.pumpStatus;

        Serial.print("Data received → ");
        printSensorData();
        delay(100);
    }
}
