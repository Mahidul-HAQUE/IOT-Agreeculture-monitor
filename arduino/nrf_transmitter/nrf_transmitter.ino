#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <DHT.h>

#define CE_PIN 9
#define CSN_PIN 10
#define DHTPIN 8         // DHT11 connected to D8
#define DHTTYPE DHT11    // Define DHT type
#define SOIL_MOISTURE_PIN A1  // Soil Moisture Sensor at A1
#define WATER_LEVEL_PIN A2    // Water Level Sensor at A2
#define PH_SENSOR_PIN A3      // pH Sensor at A3
#define RAIN_SENSOR_PIN 2     // Rain Sensor at D2
#define PUMP_PIN 3            // Relay controlling the pump at D3

RF24 radio(CE_PIN, CSN_PIN);
DHT dht(DHTPIN, DHTTYPE);
const byte address[6] = "00001";  // Address for communication

// Use attribute((packed)) to prevent struct padding
struct SensorData {
    float temperature;
    float humidity;
    int16_t soilMoisture;
    int16_t waterLevel;
    float pH;
    bool isRaining;
    bool pumpStatus;  // Added pump status
} _attribute_((packed));

void setup() {
    Serial.begin(9600);
    dht.begin();
    pinMode(RAIN_SENSOR_PIN, INPUT);  // Set Rain Sensor as input
    pinMode(PUMP_PIN, OUTPUT);        // Set Pump as output
    digitalWrite(PUMP_PIN, LOW);      // Ensure pump is off initially
    radio.begin();
    radio.openWritingPipe(address);
    radio.setPALevel(RF24_PA_LOW);
    radio.stopListening();
}

void loop() {
    SensorData data;
    data.humidity = dht.readHumidity();
    data.temperature = dht.readTemperature();
    
    data.soilMoisture = analogRead(SOIL_MOISTURE_PIN);  // Read raw value
    data.waterLevel = analogRead(WATER_LEVEL_PIN);      // Read raw value
    
    // Check the soil moisture for dry or wet conditions
    // You can adjust these thresholds based on your specific sensor
    int soilMoisturePercentage = map(data.soilMoisture, 730, 1022, 100, 0);  // Map to 0-100 range
    int waterLevelPercentage = map(data.waterLevel, 0, 588, 0, 100);      // Map to 0-100 range

    // Calibration for pH sensor to get a range between 0-14
    data.pH = analogRead(PH_SENSOR_PIN) * (14.0 / 1023.0);
    data.isRaining = !digitalRead(RAIN_SENSOR_PIN);

    // Pump control logic based on soil moisture and water level
    if (waterLevelPercentage < 30 && soilMoisturePercentage < 30) {
        digitalWrite(PUMP_PIN, HIGH);  // Turn pump ON if low water and soil moisture
        data.pumpStatus = true;
    } else {
        digitalWrite(PUMP_PIN, LOW);   // Turn pump OFF if enough water and soil moisture
        data.pumpStatus = false;
    }

    // Check if DHT readings are valid
    if (isnan(data.humidity) || isnan(data.temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    // Debug print to Serial Monitor
    Serial.print("Sending -> Temp: ");
    Serial.print(data.temperature);
    Serial.print("°C | Humidity: ");
    Serial.print(data.humidity);
    Serial.print("% | Soil Moisture: ");
    Serial.print(soilMoisturePercentage);
    Serial.print("% | Water Level: ");
    Serial.print(waterLevelPercentage);
    Serial.print("% | pH: ");
    Serial.print(data.pH);
    Serial.print(" | Rain: ");
    Serial.print(data.isRaining ? "Yes" : "No");
    Serial.print(" | Pump: ");
    Serial.println(data.pumpStatus ? "ON" : "OFF");

    // Transmit data via NRF24L01
    bool success = radio.write(&data, sizeof(data));
    Serial.println(success ? "Data sent successfully!" : "Data failed to send.");

    delay(2000);  // Send data every 2 seconds
}
