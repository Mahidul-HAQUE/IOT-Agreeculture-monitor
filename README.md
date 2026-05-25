# 🌱 AgriSense IoT — Agriculture Monitoring System

An IoT-based agriculture monitoring system built with an Arduino + ESP32 hardware stack and a Cordova-powered Android mobile app. Sensor data is collected in the field, transmitted wirelessly, and displayed to the farmer on a real-time dashboard.

> **⚠️ Testing Notice:** This system has been tested in a controlled test environment under ideal conditions. It has **not yet been deployed or validated in a real-world agriculture field.** Sensor calibration values (soil moisture thresholds, pH mapping, water level ranges) may need adjustment for actual field conditions.

---

## 📋 Table of Contents

- [System Overview](#system-overview)
- [Features](#features)
- [Hardware Components](#hardware-components)
- [Architecture](#architecture)
- [Repository Structure](#repository-structure)
- [Getting Started](#getting-started)
  - [1. Arduino Transmitter Setup](#1-arduino-transmitter-setup)
  - [2. ESP32 Receiver Setup](#2-esp32-receiver-setup)
  - [3. Mobile App Setup (Cordova APK)](#3-mobile-app-setup-cordova-apk)
- [How It Works](#how-it-works)
- [API Endpoint](#api-endpoint)
- [Dashboard Preview](#dashboard-preview)
- [Known Limitations](#known-limitations)
- [License](#license)

---

## System Overview

```
[Field Sensors]
     │
     ▼
[Arduino Nano]  ──NRF24L01──▶  [ESP32]  ──WiFi──▶  [Farmer's Phone / Browser]
 (Transmitter)                (Receiver +            (Cordova APK / Web Dashboard)
                               Web Server)
```

The Arduino Nano reads all sensors and controls the water pump via a relay. It sends packed sensor data wirelessly every 2 seconds using the NRF24L01 2.4 GHz radio module. The ESP32 receives this data, stores it in memory, and hosts a small web server over WiFi. The farmer's Android phone (running the Cordova APK) connects to the ESP32's local IP and displays live readings on an animated gauge dashboard.

---

## Features

- 🌡️ **Temperature & Humidity** — DHT11 sensor, displayed as live gauges
- 💧 **Soil Moisture** — Analog sensor, mapped to 0–100% and displayed as a gauge
- 🪣 **Water Level** — Analog sensor, shown as a visual water tank fill meter
- 🧪 **pH Level** — Analog sensor, calibrated to 0–14 pH range
- 🌧️ **Rain Detection** — Digital rain sensor with status indicator
- ⚙️ **Pump Auto-Control** — Relay-controlled water pump activates automatically when soil moisture and water level both fall below 30%
- 📊 **Live Chart** — Real-time Chart.js plot of all readings over time
- 📱 **Mobile App** — Apache Cordova Android APK wrapping the dashboard
- 🔐 **Auth Flow** — Register/Login screens with password validation and "Remember Me"

---

## Hardware Components

| Component | Role |
|---|---|
| Arduino Nano | Sensor data collection, pump relay control, NRF24L01 transmitter |
| ESP32 (38-pin) | NRF24L01 receiver, WiFi web server, SPIFFS file system |
| ESP32-CAM | *(present in circuit, reserved for future camera feed feature)* |
| NRF24L01+ (×2) | 2.4 GHz wireless link between Arduino and ESP32 |
| DHT11 | Temperature and humidity |
| Soil Moisture Sensor | Capacitive/resistive soil moisture at pin A1 |
| Water Level Sensor | Analog water depth sensor at pin A2 |
| pH Sensor Module | Analog pH probe at pin A3 |
| Rain Sensor | Digital rain detection at pin D2 |
| 5V Relay Module | Controls the water pump at pin D3 |
| DC Water Pump | Irrigation pump |
| DC Fan | Cooling / ventilation |
| Solar Panel | Primary power source |
| LM2596 Buck Converter | Regulates solar/battery voltage |
| AAA Battery Pack (×3) | Backup power supply |
| MOSFET (TO-220) | Fan / load switching |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     ARDUINO NANO (Transmitter)               │
│                                                             │
│  DHT11 ──► D8      Soil Moisture ──► A1                    │
│  Rain ──► D2       Water Level ──► A2                      │
│  Pump Relay ◄── D3  pH Sensor ──► A3                       │
│                                                             │
│  Reads sensors → packs into SensorData struct               │
│  → transmits every 2s via NRF24L01 (CE=D9, CSN=D10)        │
└────────────────────────┬────────────────────────────────────┘
                         │ NRF24L01 2.4GHz Radio
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                     ESP32 (Receiver + Web Server)            │
│                                                             │
│  NRF24L01 (CE=GPIO4, CSN=GPIO5, SPI: 18/19/23)             │
│  Listens for SensorData packets → stores in volatile struct  │
│                                                             │
│  WiFi (STA mode) → joins local network                      │
│  WebServer on port 80:                                      │
│    /           → register.html  (SPIFFS)                    │
│    /login      → login.html     (SPIFFS)                    │
│    /dashboard  → index.html     (SPIFFS)                    │
│    /data       → JSON sensor payload                        │
│    /ESP-image  → ESP.png        (SPIFFS)                    │
└────────────────────────┬────────────────────────────────────┘
                         │ HTTP over WiFi (local network)
                         ▼
┌─────────────────────────────────────────────────────────────┐
│              Android App (Cordova APK) / Browser             │
│                                                             │
│  index.html   → Register screen                             │
│  login.html   → Login screen (localStorage auth)            │
│  ESP.html     → Live sensor dashboard                       │
│                 Gauges: Temp, Humidity, Soil, pH            │
│                 Water tank animation                         │
│                 Rain & Pump status indicators               │
│                 Chart.js live time-series plot              │
│                 Polls GET /data every 2 seconds             │
└─────────────────────────────────────────────────────────────┘
```

---

## Repository Structure

```
agri-iot/
│
├── arduino/
│   └── nrf_transmitter/
│       └── nrf_transmitter.ino     # Arduino Nano firmware
│
├── esp32/
│   └── nrf_receiver/
│       ├── nrf_receiver.ino        # ESP32 firmware (receiver + web server)
│       └── data/                   # Files uploaded to ESP32 SPIFFS
│           ├── login.html
│           ├── ESP.png
│           ├── iconlog.png
│           └── iconreg.png
│
├── mobile-app/
│   ├── config.xml                  # Apache Cordova project configuration
│   └── www/                        # Web app source (also served by ESP32)
│       ├── index.html              # Register screen
│       ├── login.html              # Login screen
│       ├── ESP.html                # Main sensor dashboard
│       ├── css/
│       │   └── index.css
│       ├── js/
│       │   └── index.js            # Cordova bootstrap
│       └── img/
│           └── logo.png
│
├── docs/
│   └── Circuit_diagram.png         # Full wiring diagram
│
├── .gitignore
└── README.md
```

---

## Getting Started

### Prerequisites

- [Arduino IDE](https://www.arduino.cc/en/software) (1.8.x or 2.x)
- [ESP32 board support](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) installed in Arduino IDE
- [Node.js](https://nodejs.org/) + [Apache Cordova CLI](https://cordova.apache.org/docs/en/latest/guide/cli/) for building the APK

#### Required Arduino Libraries

Install these via **Sketch → Include Library → Manage Libraries**:

| Library | Purpose |
|---|---|
| `RF24` by TMRh20 | NRF24L01 radio driver |
| `DHT sensor library` by Adafruit | DHT11 temperature/humidity |
| `SPI` | Built-in, required by RF24 |

---

### 1. Arduino Transmitter Setup

1. Open `arduino/nrf_transmitter/nrf_transmitter.ino` in Arduino IDE.
2. Wire up your components as shown in `docs/Circuit_diagram.png`.
3. Select **Board: Arduino Nano** and the correct COM port.
4. Upload the sketch.

**Pin mapping (Arduino Nano):**

| Pin | Component |
|---|---|
| D8 | DHT11 data |
| D9 | NRF24L01 CE |
| D10 | NRF24L01 CSN |
| D2 | Rain sensor (digital) |
| D3 | Relay (pump control) |
| A1 | Soil moisture sensor |
| A2 | Water level sensor |
| A3 | pH sensor |

> **Calibration note:** Soil moisture is mapped from raw ADC range `730–1022` → `100–0%`. Water level from `0–588` → `0–100%`. These values were calibrated for the test environment — you may need to adjust `map()` ranges for your specific sensors and soil conditions.

---

### 2. ESP32 Receiver Setup

1. Open `esp32/nrf_receiver/nrf_receiver.ino` in Arduino IDE.
2. **Update WiFi credentials** on lines 5–6:
   ```cpp
   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
3. Select **Board: ESP32 Dev Module** and the correct COM port.
4. **Upload the SPIFFS data folder** before uploading the sketch:
   - Copy the HTML/image files from `esp32/nrf_receiver/data/` into the `data/` folder inside your Arduino sketch directory.
   - Use **Tools → ESP32 Sketch Data Upload** (requires the [arduino-esp32fs-plugin](https://github.com/me-no-dev/arduino-esp32fs-plugin)).
5. Upload the sketch.
6. Open Serial Monitor at **115200 baud** to find the ESP32's IP address.

**Pin mapping (ESP32):**

| Pin | Component |
|---|---|
| GPIO4 | NRF24L01 CE |
| GPIO5 | NRF24L01 CSN |
| GPIO18 | SPI SCK |
| GPIO19 | SPI MISO |
| GPIO23 | SPI MOSI |

---

### 3. Mobile App Setup (Cordova APK)

The app is a standard Apache Cordova project. The `www/` folder is the web source and the `config.xml` defines the Android build.

```bash
# Install Cordova globally if you haven't already
npm install -g cordova

# Create a new Cordova project and replace www/ and config.xml
cordova create AgriSense com.example.esp32 ControlESP
cd AgriSense

# Replace config.xml and www/ with the ones from this repo
cp -r path/to/repo/mobile-app/config.xml .
cp -r path/to/repo/mobile-app/www ./www

# Add Android platform
cordova platform add android

# Before building: update the ESP32 IP address in ESP.html
# Find the fetch('/data') call and replace with http://<ESP32_IP>/data

# Build the APK
cordova build android

# The APK will be at:
# platforms/android/app/build/outputs/apk/debug/app-debug.apk
```

Install `app-debug.apk` on your Android phone. The phone must be on the **same WiFi network** as the ESP32.

> **Minimum Android SDK:** 22 (Android 5.1 Lollipop)  
> **Target SDK:** 34 (Android 14)

---

## How It Works

### Data Flow

1. **Arduino** reads all sensors in a 2-second loop and packs the values into a `SensorData` struct (temperature, humidity, soilMoisture, waterLevel, pH, isRaining, pumpStatus).
2. The struct is transmitted over the air via **NRF24L01** on address `"00001"`.
3. **ESP32** is in listening mode. When a packet arrives it copies the struct into a `volatile SensorData` variable and immediately updates the web server's `/data` response.
4. The **dashboard (ESP.html)** polls `GET /data` every 2 seconds and receives a JSON payload like:
   ```json
   {
     "temperature": 28.5,
     "humidity": 72.0,
     "soilMoisture": 45,
     "waterLevel": 60,
     "pH": 6.8,
     "isRaining": false,
     "pumpStatus": false
   }
   ```
5. Gauges, the water tank animation, status badges, and the Chart.js time-series plot all update live from this JSON.

### Pump Auto-Control Logic

```cpp
if (waterLevelPercentage < 30 && soilMoisturePercentage < 30) {
    digitalWrite(PUMP_PIN, HIGH);  // Pump ON
} else {
    digitalWrite(PUMP_PIN, LOW);   // Pump OFF
}
```

Both conditions must be met simultaneously to activate the pump, preventing unnecessary irrigation when the soil is already moist.

### Authentication

Login credentials are stored in the browser's `localStorage` — this is a lightweight local auth suitable for a single-farmer personal device. There is no backend server or database. Credentials persist across app restarts when "Remember Me" is checked.

---

## Connecting the App to the ESP32

This was the main practical issue with the original system: the app had the ESP32's IP address hardcoded. Whenever the ESP32 connected to a new network, or the router assigned it a different IP via DHCP, the app stopped working with no way to fix it short of editing the source code and rebuilding.

The current version solves this in two places.

### ESP32 side — mDNS hostname

The ESP32 now advertises itself on the local network as `agrisense.local` using the `ESPmDNS` library. This hostname is stable regardless of what IP DHCP assigns. On most Android phones and all iOS devices, the app can reach the ESP32 at `http://agrisense.local/data` without ever knowing the IP.

```cpp
// In nrf_receiver.ino — runs automatically after WiFi connects
if (MDNS.begin("agrisense")) {
    MDNS.addService("http", "tcp", 80);
}
```

The ESP32 also now serves a lightweight `/ping` endpoint that returns `{"device":"agrisense"}`. The app uses this to confirm it has found the right device before locking on to an address.

### App side — 4-stage auto-discovery

If `agrisense.local` does not resolve (some older Android versions have incomplete mDNS support), the app runs a discovery sequence automatically on startup and whenever the connection is lost:

```
Stage 1 → Try agrisense.local                       (~600 ms)
Stage 2 → Try last saved IP from localStorage        (~600 ms)
Stage 3 → Scan the same /24 subnet as the saved IP  (~2–4 s)
           e.g. if saved IP was 192.168.1.45,
           scans 192.168.1.1–254
Stage 4 → Scan common subnets: 192.168.0.x,         (~8–12 s)
          192.168.1.x, 192.168.4.x (AP mode),
          10.0.0.x, 10.0.1.x
```

Probes in each stage run in parallel batches of 40 so even a full /24 scan finishes in a few seconds. The discovered address is saved to `localStorage` so Stage 2 succeeds instantly on the next launch.

### Settings modal — always accessible

A **⚙ Settings** button sits permanently in the top status bar. Tapping it opens the connection modal where the farmer can:

- See which host the app is currently using
- Type in a known IP or hostname manually and tap **Connect**
- Tap **🔍 Auto-Discover** to run the full 4-stage scan from scratch
- Watch a live discovery log showing exactly what the app is trying

If all discovery stages fail (e.g. the phone is on a different WiFi network than the ESP32), the modal opens automatically and stays open until a working address is found or entered.

### Static IP fallback (most reliable option)

For deployments where the network configuration is under your control, the most robust approach is to give the ESP32 a fixed IP that is outside the router's DHCP range. Four lines in the firmware are already prepared — just uncomment them:

```cpp
// In nrf_receiver.ino, before WiFi.begin():
IPAddress staticIP(192, 168, 1, 200);
IPAddress gateway (192, 168, 1,   1);
IPAddress subnet  (255, 255, 255, 0);
IPAddress dns     (8,   8,   8,   8);
WiFi.config(staticIP, gateway, subnet, dns);
```

Then enter `192.168.1.200` once in the app's Settings modal. It will be saved to localStorage and Stage 2 of discovery will always succeed immediately.

---

## API Endpoint

The ESP32 exposes two endpoints:

```
GET http://agrisense.local/ping   (or http://<IP>/ping)
GET http://agrisense.local/data   (or http://<IP>/data)
```

**`/ping` — identity probe (used by auto-discovery):**

```json
{ "device": "agrisense", "version": "1.0", "ip": "192.168.1.45" }
```

**`/data` — live sensor readings:**

**Response (application/json):**

```json
{
  "temperature": 28.5,
  "humidity": 72.0,
  "soilMoisture": 45,
  "waterLevel": 60,
  "pH": 6.8,
  "isRaining": false,
  "pumpStatus": true
}
```

CORS headers are set to `*` so any origin can fetch the data.

---

## Dashboard Preview

See `docs/Circuit_diagram.png` for the full hardware wiring diagram.

The dashboard (`ESP.html`) includes:
- **Temperature gauge** — JustGage radial meter, 0–50 °C
- **Humidity gauge** — JustGage radial meter, 0–100%
- **Soil Moisture gauge** — JustGage radial meter, 0–100%
- **Water Level tank** — Animated CSS water fill visualization
- **pH gauge** — JustGage radial meter, 0–14
- **Rain status** — Colour-coded badge (Raining / Not Raining)
- **Pump status** — Colour-coded badge (ON / OFF)
- **Live plot** — Chart.js line chart streaming all numeric readings

---

## Known Limitations

| Limitation | Detail |
|---|---|
| **Lab-tested only** | Sensor calibration was done in a controlled test environment. Real-field deployment may require re-tuning the `map()` ranges for soil moisture and water level. |
| **Single WiFi network** | The phone and ESP32 must share the same local WiFi. There is no internet/cloud relay. |
| **No persistent data storage** | Sensor history exists only in the Chart.js memory for the current session. There is no SD card or cloud logging. |
| **localStorage auth** | Credentials are stored unencrypted in browser localStorage — suitable only for personal-device use. |
| **NRF24L01 range** | Typical range is 50–100 m (line of sight) at `RF24_PA_LOW`. Larger fields may require PA_HIGH or a directional antenna. |
| **Single sensor node** | Only one Arduino transmitter node is supported per ESP32 receiver (address `"00001"`). Multi-node support would require code changes. |
| **ESP32-CAM** | Appears in the circuit diagram but is not yet integrated into the firmware or dashboard. Reserved for a future live camera feed feature. |

---

## License

This project is open source. Feel free to fork, adapt, and improve it.  
If you use it in a real agriculture deployment, consider contributing your field calibration data back via a pull request!
