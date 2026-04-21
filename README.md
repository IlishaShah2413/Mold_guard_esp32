#  Mold Guard ESP32 — Context-Aware Mold Risk Detector

A context-aware mold risk monitoring system built on the **ESP32-DevKit**, using real-time temperature and humidity readings to estimate mold growth probability across four room profiles. Sensor data is displayed on an LCD and uploaded to **ThingSpeak** for remote monitoring.

---

##  Features

- **4 Room Profiles** — Hospital, Food Storage, Archive/Library, Residential
- **7-level mold risk scale** — from SAFE to critical (~2hrs risk)
- **Real-time LCD display** — 16×2 LCD showing mode, T/H readings, and risk message
- **LED indicators** — Green (safe), Yellow (caution), Red (danger)
- **Non-blocking buzzer alerts** — 200ms ON / 300ms OFF cycle on high risk
- **ThingSpeak IoT upload** — logs temperature, humidity, risk level, and room mode every 30 seconds
- **Button-controlled mode switching** — cycle through room profiles with a single button press
- **Wokwi simulation support** — ready to simulate without physical hardware

---

##  Hardware

| Component         | Pin       |
|------------------|-----------|
| Temperature POT  | GPIO 34   |
| Humidity POT     | GPIO 35   |
| Green LED        | GPIO 19   |
| Yellow LED       | GPIO 21   |
| Red LED          | GPIO 22   |
| Buzzer           | GPIO 23   |
| Mode Button      | GPIO 27   |
| LCD RS           | GPIO 5    |
| LCD EN           | GPIO 17   |
| LCD D4           | GPIO 16   |
| LCD D5           | GPIO 4    |
| LCD D6           | GPIO 2    |
| LCD D7           | GPIO 15   |

>  In simulation (Wokwi), potentiometers are used in place of real sensors. For physical deployment, replace ADC reads with a DHT22 or similar sensor.

---

##  Room Profile Thresholds

| Profile        | Safe Humidity | Standard Basis         |
|---------------|--------------|------------------------|
| Hospital       | < 50% RH     | ASHRAE 170 / NFPA-99   |
| Food Storage   | < 55% RH     | USDA Dry Storage       |
| Archive/Library| < 50% RH     | NISO / European        |
| Residential    | < 60% RH     | EPA / WHO              |

Risk levels increase with both humidity and temperature. Higher temperature accelerates mold germination.

---

##  Risk Level Scale

| Level | Condition          | Output              |
|-------|--------------------|---------------------|
| 0     | SAFE               | Green LED           |
| 1–2   | Low Risk           | Green + Yellow LED  |
| 3–4   | Medium Risk        | Yellow LED          |
| 5–6   | High Risk          | Red LED + Buzzer    |

---

##  ThingSpeak Integration

Data is uploaded every **30 seconds** to ThingSpeak with these fields:

| Field   | Data             |
|---------|-----------------|
| Field 1 | Temperature (°C) |
| Field 2 | Humidity (%)     |
| Field 3 | Risk Level (0–6) |
| Field 4 | Room Mode (0–3)  |

To use your own ThingSpeak channel, update these lines in `sketch.ino`:

```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey   = "YOUR_THINGSPEAK_API_KEY";
```

---

## 📁 Project Structure

```
mold-guard-esp32/
├── sketch/
│   └── sketch.ino          # Main Arduino sketch
├── .vscode/
│   └── c_cpp_properties.json  # IntelliSense config
├── build/                  # Compiled binaries (auto-generated)
├── partitions.csv          # Custom partition table
├── wokwi.toml             # Wokwi simulation config
├── arduino-cli.json        # Arduino CLI build config
└── README.md
```

---

##  Getting Started

### Option A — Wokwi Simulation (No Hardware Needed)

1. Go to [wokwi.com](https://wokwi.com) and create a new ESP32 project
2. Copy `sketch.ino` into the editor
3. Add an LCD1602, 2 potentiometers, 3 LEDs, a buzzer, and a button per the pin table above
4. Press **Run**

### Option B — Flash to Real ESP32

**Prerequisites:**
- Arduino IDE 2.x or Arduino CLI
- ESP32 board package installed
- `LiquidCrystal` library (v1.0.7+)

**Steps:**

1. Clone the repo:
   ```bash
   git clone https://github.com/IlishaShah2413/mold-guard-esp32.git
   cd mold-guard-esp32
   ```

2. Open `sketch/sketch.ino` in Arduino IDE

3. Update your WiFi credentials and ThingSpeak API key

4. Select **ESP32 Dev Module** as your board

5. Upload!

**Or via Arduino CLI:**
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 sketch/
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 sketch/
```

**Manual Flash (esptool):**
```bash
esptool.py --chip esp32 --baud 460800 write_flash \
  --flash-mode dio --flash-freq 80m --flash-size 4MB \
  0x1000 sketch.ino.bootloader.bin \
  0x8000 sketch.ino.partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 sketch.ino.bin
```

---

##  Dependencies

- [LiquidCrystal](https://www.arduino.cc/reference/en/libraries/liquidcrystal/) — for LCD control
- [WiFi](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html) — built into ESP32 Arduino core
- [HTTPClient](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/httpclient.html) — built into ESP32 Arduino core

---

##  How It Works

1. On boot, the device connects to WiFi and shows a startup screen
2. Temperature and humidity are read from analog pins (potentiometers in simulation)
3. The `getMoldRisk()` function selects the appropriate room profile and computes the mold risk category based on published standards
4. The LCD, LEDs, and buzzer are updated to reflect current risk
5. Every 30 seconds, data is pushed to ThingSpeak
6. Pressing the button (GPIO 27) cycles through the 4 room profiles

---

##  License

MIT License — feel free to use, modify, and share.

---

##  Author
Made by Ilisha Shah
Made using ESP32 + Arduino
```