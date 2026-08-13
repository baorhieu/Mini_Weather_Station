# Mini Weather Station — Indoor Thermal Comfort Monitor

ESP32-based environmental monitoring station built for the **Applied Microcontrollers (AMC)** course, Summer Semester 2026, Hochschule Rhein-Waal.

The station measures temperature, humidity, atmospheric pressure and ambient light in an indoor space, displays the readings locally, alerts when conditions exceed a comfort threshold, and publishes all values over MQTT for remote logging and visualisation.

---

## Project Intention

Indoor spaces heat up during sunny days through solar gain, often exceeding the temperature considered acceptable for working or studying. This project monitors that effect and warns when the room becomes too warm.

**Monitoring intention:** ambient light intensity entering the room is expected to correlate with a subsequent rise in room temperature. Pressure provides weather context, and humidity determines whether warm conditions are also uncomfortable.

The alert threshold is taken from **ASR A3.5**, the German technical rule for workplace room temperature, which treats 26 °C as the point at which measures should be considered. The rule applies to workplaces; here it is used by analogy for a study space.

---

## Hardware

| Component | Purpose | Interface |
|---|---|---|
| ESP32 DevKit V1 | Controller, Wi-Fi, MQTT client | — |
| DHT22 (AM2302) | Air temperature, relative humidity | 1-Wire digital, GPIO 27 |
| BMP180 | Atmospheric pressure | I²C, address `0x77` |
| Grove Light Sensor (SEN11302P) | Ambient light (relative) | Analog, GPIO 34 (ADC1) |
| 16×2 LCD (HD44780) | Local display, 4-bit parallel mode | GPIO 14, 13, 26, 25, 33, 32 |
| Red / green LEDs | Status indication | GPIO 4 / GPIO 2 |
| KY-012 active buzzer | Audible alert | GPIO 23 |

### Pin Assignment

```
I²C bus        SDA = GPIO 21    SCL = GPIO 22
DHT22          DATA = GPIO 27   (10 kΩ pull-up to 3V3)
Light sensor   SIG  = GPIO 34   (ADC1 — required, ADC2 is unavailable while Wi-Fi is active)

LCD            RS = GPIO 14     E  = GPIO 13
               D4 = GPIO 26     D5 = GPIO 25
               D6 = GPIO 33     D7 = GPIO 32
               VDD = 5V, VSS = GND, RW = GND
               V0  = wiper of 10 kΩ potentiometer (contrast)
               A   = 5V via 220 Ω, K = GND (backlight)

Green LED      GPIO 2  (220 Ω series resistor)
Red LED        GPIO 4  (220 Ω series resistor)
Buzzer (S)     GPIO 23
```

**Pins to avoid:** GPIO 34–39 are input-only and cannot drive the LCD or LEDs. GPIO 12 is a strapping pin and can prevent the board from booting. GPIO 6–11 are connected to the flash memory.

---

## Software

### Dependencies

Install via the Arduino Library Manager, or let PlatformIO resolve them:

- `Adafruit Unified Sensor`
- `Adafruit BMP085 Unified` — this is the correct library for the BMP180
- `DHT sensor library` (Adafruit)
- `LiquidCrystal` (bundled with the Arduino core)
- `PubSubClient` (Nick O'Leary) — MQTT

### PlatformIO

```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200

lib_deps =
    adafruit/Adafruit Unified Sensor
    adafruit/Adafruit BMP085 Unified
    adafruit/DHT sensor library
    knolleary/PubSubClient
```

### Configuration

Before flashing, set the Wi-Fi credentials at the top of the sketch:

```cpp
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
```

> Do not commit real credentials to the repository. Keep them local, or move them to a separate untracked header file.

---

## Behaviour

### Sampling

All four channels are read every 2.5 s. The interval is set by the DHT22, which cannot be polled faster than roughly every 2 s.

### Display

The LCD cycles through three pages, one per reading cycle:

| Page | Line 1 | Line 2 |
|---|---|---|
| 1 | Temperature (°C) | Humidity (%) |
| 2 | Pressure (hPa) | Light (raw ADC) |
| 3 | Status: OK / TOO HOT | MQTT: online / offline |

If a sensor read fails, the previous valid value is retained and `--` is shown rather than a garbage reading.

### Alert Logic

| State | Condition | Green LED | Red LED | Buzzer |
|---|---|---|---|---|
| Normal | T < 26 °C | On | Off | Off |
| Too hot | T ≥ 26 °C | Off | On | Off |
| Too hot & humid | T ≥ 26 °C and RH ≥ 60 % | Off | On | 2 s beep, then 1 min cooldown |

Two design details worth noting:

- **Hysteresis** — the alert turns on at 26 °C but only clears at 25 °C. Without this, a temperature hovering at the threshold would make the LEDs and buzzer flicker.
- **Cooldown** — the buzzer sounds for 2 s and then stays silent for at least one minute, even if conditions remain hot. A continuously sounding alarm during a multi-hour run would be useless and unpleasant.

Pressure and light are logged and displayed but do not drive any actuator. They provide the weather and solar-gain context for interpreting the temperature record.

---

## MQTT

Broker: `broker.hivemq.com:1883` (public test broker, no authentication)

| Topic | Payload |
|---|---|
| `/amc/ss2026/group3/am2302/temperature` | Temperature in °C, 2 decimals |
| `/amc/ss2026/group3/am2302/humidity` | Relative humidity in %, 2 decimals |
| `/amc/ss2026/group3/bmp180/pressure` | Pressure in hPa, 2 decimals |
| `/amc/ss2026/group3/light/raw` | Raw ADC value, 0–4095 |

Subscribe from a terminal to check the stream:

```bash
mosquitto_sub -h broker.hivemq.com -t "/amc/ss2026/group3/#" -v
```

> The broker is public. Anyone who knows the topic path can read the data. Acceptable for a course project; not suitable for anything private.

---

## Getting Started

1. Wire the components according to the pin assignment above.
2. Run an I²C scanner first. The BMP180 should appear at `0x77`. If it does not, stop and fix the wiring before flashing the main sketch.
3. Set the Wi-Fi credentials.
4. Flash the sketch and open the serial monitor at 115200 baud.
5. Adjust the LCD contrast potentiometer until the text is readable — a blank or fully dark screen is usually a contrast problem, not a code problem.

---

## Known Limitations

- The Grove light sensor is uncalibrated. Values are relative ADC counts, not lux, and cannot be compared across different sensor placements.
- Self-heating from the ESP32 regulator biases the DHT22 upward if the sensor is mounted close to the board.
- No automatic MQTT reconnection: if the broker connection drops, publishing fails silently until the board is reset. The LCD status page shows the connection state.
- Wi-Fi credentials are stored in plain text in the source.
- ASR A3.5 is written for workplaces, not residential rooms. The 26 °C threshold is applied here by analogy.

---

## Team

Group 3 — AMC, Summer Semester 2026

| Name | Role |
|---|---|
| Ayasha Siddika | Inputs Lead — sensor interfacing, calibration, conversion |
| Lam Bao Hieu Truong | Outputs Lead — LCD, LEDs, buzzer, alert logic |
| Amena Siddika | MQTT and Communication Lead |
| Prattay Barman | Cloud, Data and Visualisation Lead |

---

## License

Coursework project. Provided as-is for educational purposes.
