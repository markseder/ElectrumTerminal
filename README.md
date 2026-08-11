# Electrum Terminal

**Electrum Terminal** is a multifunctional information terminal for the ESP32-2432S028R CYD board. It combines a 2.8-inch touch display, GPS receiver, BMP580 environmental sensor, online weather, exchange rates, commodity prices, earthquake monitoring, calendar, system diagnostics, and a browser-based Wi-Fi setup page.

## Current firmware

The current firmware baseline is **V4.2 UI Status LED**.

### Main screens

- **HOME** — clock, location, External weather and Internal BMP580 readings
- **MENU** — Weather, Quakes, FX, Commodities, GPS and System
- **WEATHER** — online weather plus local temperature, pressure and calculated altitude
- **QUAKES** — recent M4.5+ earthquakes from USGS
- **FX** — USD/RUB, EUR/RUB, THB/RUB and VND/RUB
- **COMMODITIES** — gold, silver, Brent and WTI
- **GPS** — latitude, longitude, UTM zone, satellites, altitude, country, city, currency and timezone
- **SYSTEM** — Wi-Fi, IP, RSSI, heap, uptime, GPS, timezone and HTTP diagnostics
- **CALENDAR** — local calendar synchronized through NTP

### Additional features

- Touch control through XPT2046
- Display rotation button
- RGB status LED with animated states
- Open-Meteo as the primary weather provider
- OpenWeatherMap fallback and location metadata
- Automatic timezone selection
- Web-based Wi-Fi configuration
- Preferences storage for UI, Wi-Fi, location and timezone settings
- Partial sprite buffering to reduce display flicker
- Automatic Wi-Fi reconnection and periodic data refresh

## Hardware

| Component | Connection |
|---|---|
| Board | ESP32-2432S028R CYD |
| Display | ILI9341_2 via TFT_eSPI |
| Touch CS | GPIO 33 |
| Touch IRQ | GPIO 36 |
| Touch MOSI | GPIO 32 |
| Touch MISO | GPIO 39 |
| Touch CLK | GPIO 25 |
| GPS TX → ESP32 RX | GPIO 35 |
| BMP580 SDA | GPIO 27 |
| BMP580 SCL | GPIO 22 |
| BMP580 address | 0x47 |
| TFT backlight | GPIO 21 |
| RGB LED R/G/B | GPIO 17 / 16 / 4 |

See [docs/HARDWARE.md](docs/HARDWARE.md) for full wiring and setup notes.

## Required Arduino libraries

- ESP32 board package
- TFT_eSPI
- XPT2046_Touchscreen
- ArduinoJson
- TinyGPSPlus
- Adafruit Unified Sensor
- Adafruit BMP5xx
- Built-in ESP32 libraries: WiFi, HTTPClient, WebServer, Preferences, Wire and SPI

## TFT_eSPI configuration

The working display configuration uses:

- Driver: `ILI9341_2`
- MISO: GPIO 12
- MOSI: GPIO 13
- SCLK: GPIO 14
- CS: GPIO 15
- DC: GPIO 2
- RST: `-1`
- Backlight: GPIO 21
- Display rotation: `1`
- `invertDisplay(true)`

Configure these values in your TFT_eSPI user setup before compiling.

## Configuration before upload

The public firmware does not contain personal Wi-Fi credentials or a live OpenWeatherMap key. In the sketch, configure:

```cpp
const char* DEFAULT_WIFI_SSID = "";
const char* DEFAULT_WIFI_PASS = "";
const char* OPENWEATHER_API_KEY = "YOUR_OPENWEATHER_API_KEY";
```

If Wi-Fi credentials are empty or the connection fails, the device starts its configuration access point:

- SSID: `ElectrumTerminal`
- Setup page: `http://192.168.4.1`

Change the default configuration access-point password before using the device outside a trusted environment.

## Opening the firmware

Open `firmware/ElectrumTerminal/ElectrumTerminal.ino` in Arduino IDE. The numbered `.ino` tabs are consecutive parts of the same 4340-line sketch and are concatenated automatically by the Arduino build system. Keep all files together in the `ElectrumTerminal` folder.

## Data sources

- Open-Meteo — primary weather
- OpenWeatherMap — fallback weather and location metadata
- ExchangeRate-API — foreign exchange data
- Gold API — gold and silver
- Yahoo Finance — oil prices
- USGS — earthquakes
- NOAA SWPC — space-weather data retained in the firmware
- NTP — time synchronization

External services may change their formats, limits or availability.

## Repository structure

```text
ElectrumTerminal/
├── firmware/
│   └── ElectrumTerminal/
│       ├── ElectrumTerminal.ino
│       └── 01_Firmware.ino … 10_Firmware.ino
├── README.md
├── CHANGELOG.md
└── docs/
    ├── HARDWARE.md
    └── DEVELOPMENT_NOTES.md
```

## Project status

The firmware is functional and under active development. The next recommended technical work is to improve network request scheduling, reduce dynamic-memory fragmentation, move all secrets out of source code, and update weather only when the GPS location changes materially.

## Author

Developed by [Markseder](https://github.com/markseder).
