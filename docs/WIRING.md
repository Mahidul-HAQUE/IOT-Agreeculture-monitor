# Hardware Wiring Notes

Refer to `Circuit_diagram.png` for the full visual schematic.

## Arduino Nano — Transmitter

| Arduino Pin | Connected To |
|---|---|
| D2 | Rain Sensor (digital output) |
| D3 | Relay Module IN (controls water pump) |
| D8 | DHT11 DATA |
| D9 | NRF24L01 CE |
| D10 | NRF24L01 CSN |
| D11 (MOSI) | NRF24L01 MOSI |
| D12 (MISO) | NRF24L01 MISO |
| D13 (SCK) | NRF24L01 SCK |
| A1 | Soil Moisture Sensor (analog) |
| A2 | Water Level Sensor (analog) |
| A3 | pH Sensor Module (analog) |
| 3.3V | NRF24L01 VCC |
| GND | Common ground |
| 5V | DHT11 VCC, Relay VCC, Sensor VCC |

## ESP32 — Receiver + Web Server

| ESP32 Pin | Connected To |
|---|---|
| GPIO4 | NRF24L01 CE |
| GPIO5 | NRF24L01 CSN |
| GPIO18 | NRF24L01 SCK |
| GPIO19 | NRF24L01 MISO |
| GPIO23 | NRF24L01 MOSI |
| 3.3V | NRF24L01 VCC |
| GND | Common ground |

## Power System

| Component | Detail |
|---|---|
| Solar Panel | Primary input to LM2596 buck converter |
| LM2596 Buck Converter | Steps down solar voltage to stable 5V/3.3V rails |
| AAA Battery Pack (×3, 4.5V) | Backup power, in parallel via switch |
| MOSFET (TO-220) | Switches the DC fan load |
| Relay Module | Switches the 5V DC water pump |

## NRF24L01 Tips

- **Always use a 10–100 µF capacitor** across the NRF24L01 VCC and GND pins — the module is sensitive to power supply noise and will fail to communicate without filtering.
- Run at `RF24_PA_LOW` for bench testing. Switch to `RF24_PA_HIGH` outdoors if range is insufficient (increase current draw accordingly).
- Keep the NRF24L01 antenna away from the ESP32's WiFi antenna to minimize interference.

## Sensor Calibration (Test Environment Values)

| Sensor | Raw ADC Range | Mapped Output |
|---|---|---|
| Soil Moisture | 730 (wet) – 1022 (dry) | 100% – 0% |
| Water Level | 0 (empty) – 588 (full) | 0% – 100% |
| pH | 0 – 1023 | 0.0 – 14.0 (linear approximation) |

> These ranges were measured with specific sensors in a lab test setup. Real agricultural sensors will likely have different characteristics. Re-calibrate by reading raw ADC values at known conditions (dry/wet soil, empty/full tank) and update the `map()` calls in `nrf_transmitter.ino`.
