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

All user-editable values are grouped near the top of the sketch under `USER CONFIGURATION`.

### Required settings

| Setting | What to enter |
|---|---|
| `OPENWEATHER_API_KEY` | Your key from [OpenWeatherMap](https://openweathermap.org/api). Use `ENTER_YOUR_OPENWEATHER_API_KEY` only as a placeholder. |
| `CONFIG_AP_PASS` | A new password of at least 8 characters for the device setup access point. |
| `DEFAULT_LAT` | Fallback latitude used before GPS obtains a fix. |
| `DEFAULT_LON` | Fallback longitude used before GPS obtains a fix. |
| `DEFAULT_GMT_OFFSET_SEC` | Fallback UTC offset in seconds, for example `3 * 3600` for UTC+3. |

### Wi-Fi setup

The public firmware contains no personal network credentials.

- Recommended: leave `DEFAULT_WIFI_SSID` and `DEFAULT_WIFI_PASS` empty and configure Wi-Fi from the device web portal.
- Optional: enter a default SSID and password in the sketch for automatic connection.
- The setup access point name is controlled by `CONFIG_AP_SSID`.

When the saved/default network is unavailable, connect to the Electrum Terminal setup access point and open `http://192.168.4.1`.

### Hardware settings

The included values match the documented ESP32-2432S028R CYD build. Change them only for different hardware:

- `GPS_ENABLED`, `GPS_RX_PIN`, `GPS_TX_PIN`, `GPS_BAUD`
- `BMP580_ENABLED`, SDA/SCL pins and I²C address
- display rotation and backlight pin
- XPT2046 touch pins and `RAW_X/Y` calibration values
- RGB LED pins and `RGB_ACTIVE_LOW`
- automatic refresh intervals

### External services

| Service | Purpose | User API key |
|---|---|---|
| Open-Meteo | Primary weather | No |
| OpenWeatherMap | Fallback weather, country, city and timezone | **Yes** |
| open.er-api.com | Currency rates | No |
| Gold API | Gold and silver prices | No key in the current endpoint |
| Yahoo Finance | Brent and WTI prices | No |
| USGS | Earthquakes | No |
| NOAA SWPC | Space-weather data retained in firmware | No |
| NTP servers | Time synchronization | No |

Third-party services can change their endpoints, policies or rate limits.

## Opening the firmware

Open `firmware/ElectrumTerminal/ElectrumTerminal.ino` in Arduino IDE.

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
│       └── ElectrumTerminal.ino
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
