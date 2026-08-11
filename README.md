# Electrum Terminal

**Electrum Terminal** is a multifunctional information terminal for the ESP32-2432S028R CYD board. It combines a 2.8-inch touch display, GPS receiver, BMP580 environmental sensor, online weather, exchange rates, commodity prices, earthquake monitoring, calendar, system diagnostics, and a browser-based Wi-Fi setup page.

## Current firmware

The current firmware baseline is **V4.2 UI Status LED**.


## Quick configuration — English

If you use the same ESP32-2432S028R CYD board and wiring described in this repository, edit only the settings below near the beginning of `ElectrumTerminal.ino`.

### 1. Create a password for the terminal setup network

Find:

```cpp
const char* CONFIG_AP_PASS = "ENTER_YOUR_AP_PASSWORD";
```

Replace the placeholder with your own password containing at least 8 characters:

```cpp
const char* CONFIG_AP_PASS = "Electrum2026";
```

This is the password for the temporary `ElectrumTerminal` setup access point. It is not the password for your home router.

### 2. Enter your OpenWeatherMap API key

Find:

```cpp
const char* OPENWEATHER_API_KEY = "ENTER_YOUR_OPENWEATHER_API_KEY";
```

Create a key at [OpenWeatherMap](https://openweathermap.org/api) and paste it between the quotation marks:

```cpp
const char* OPENWEATHER_API_KEY = "your_real_api_key";
```

Keep the quotation marks and the semicolon.

### 3. Set the fallback location

These coordinates are used before GPS obtains a fix:

```cpp
const float DEFAULT_LAT = 59.56;
const float DEFAULT_LON = 150.80;
```

Replace them with the latitude and longitude of your city in decimal degrees. The example values are for Magadan and can be left unchanged by users in Magadan.

### 4. Set the fallback timezone

Find:

```cpp
const long DEFAULT_GMT_OFFSET_SEC = 11 * 3600;
```

Examples:

```cpp
// Moscow, UTC+3
const long DEFAULT_GMT_OFFSET_SEC = 3 * 3600;

// Vladivostok, UTC+10
const long DEFAULT_GMT_OFFSET_SEC = 10 * 3600;

// Magadan, UTC+11
const long DEFAULT_GMT_OFFSET_SEC = 11 * 3600;
```

### 5. Leave home Wi-Fi empty

The recommended public configuration is:

```cpp
const char* DEFAULT_WIFI_SSID = "";
const char* DEFAULT_WIFI_PASS = "";
```

After flashing the firmware:

1. Connect to the `ElectrumTerminal` Wi-Fi network.
2. Enter the password specified in `CONFIG_AP_PASS`.
3. Open [http://192.168.4.1](http://192.168.4.1).
4. Enter the SSID and password of your home Wi-Fi.
5. Save the settings. The terminal stores them and restarts.

For the standard hardware build, do not change display, touch, GPS, BMP580 or RGB LED pins.

**Magadan users only need to replace `CONFIG_AP_PASS` and `OPENWEATHER_API_KEY`.**

---

## Быстрая настройка — Русский

Если используется такая же плата ESP32-2432S028R CYD и подключение из этого репозитория, в начале файла `ElectrumTerminal.ino` нужно изменить только указанные ниже параметры.

### 1. Придумайте пароль настроечной сети терминала

Найдите:

```cpp
const char* CONFIG_AP_PASS = "ENTER_YOUR_AP_PASSWORD";
```

Замените заглушку своим паролем длиной не менее 8 символов:

```cpp
const char* CONFIG_AP_PASS = "Electrum2026";
```

Это пароль временной Wi-Fi-сети `ElectrumTerminal`, а не пароль домашнего роутера.

### 2. Введите API-ключ OpenWeatherMap

Найдите:

```cpp
const char* OPENWEATHER_API_KEY = "ENTER_YOUR_OPENWEATHER_API_KEY";
```

Создайте ключ на сайте [OpenWeatherMap](https://openweathermap.org/api) и вставьте его между кавычками:

```cpp
const char* OPENWEATHER_API_KEY = "ваш_настоящий_API_ключ";
```

Кавычки и точку с запятой в конце строки оставьте.

### 3. Укажите резервное местоположение

Эти координаты используются до получения GPS-фикса:

```cpp
const float DEFAULT_LAT = 59.56;
const float DEFAULT_LON = 150.80;
```

Замените их широтой и долготой своего города в десятичных градусах. В примере указаны координаты Магадана — жителям Магадана их менять не нужно.

### 4. Укажите резервный часовой пояс

Найдите:

```cpp
const long DEFAULT_GMT_OFFSET_SEC = 11 * 3600;
```

Примеры:

```cpp
// Москва, UTC+3
const long DEFAULT_GMT_OFFSET_SEC = 3 * 3600;

// Владивосток, UTC+10
const long DEFAULT_GMT_OFFSET_SEC = 10 * 3600;

// Магадан, UTC+11
const long DEFAULT_GMT_OFFSET_SEC = 11 * 3600;
```

### 5. Оставьте домашний Wi-Fi пустым

Рекомендуемая настройка публичной прошивки:

```cpp
const char* DEFAULT_WIFI_SSID = "";
const char* DEFAULT_WIFI_PASS = "";
```

После загрузки прошивки:

1. Подключитесь к Wi-Fi-сети `ElectrumTerminal`.
2. Введите пароль, заданный в `CONFIG_AP_PASS`.
3. Откройте [http://192.168.4.1](http://192.168.4.1).
4. Введите название и пароль домашней Wi-Fi-сети.
5. Сохраните настройки. Терминал запомнит их и перезагрузится.

Для стандартной аппаратной сборки не меняйте пины дисплея, тачскрина, GPS, BMP580 и RGB-светодиода.

**Пользователю из Магадана достаточно заменить только `CONFIG_AP_PASS` и `OPENWEATHER_API_KEY`.**

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

## Quick start

Follow these steps for the standard ESP32-2432S028R CYD build documented in this repository.

### 1. Install Arduino IDE and ESP32 support

1. Install Arduino IDE 2.x.
2. Open **File → Preferences**.
3. Add the Espressif ESP32 board package URL to **Additional Boards Manager URLs**:

   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

4. Open **Tools → Board → Boards Manager**, search for `esp32`, and install **esp32 by Espressif Systems**.
5. Select **ESP32 Dev Module** as the board for the standard CYD.

### 2. Install the required libraries

Open **Tools → Manage Libraries** and install:

- `TFT_eSPI`
- `XPT2046_Touchscreen`
- `ArduinoJson`
- `TinyGPSPlus`
- `Adafruit Unified Sensor`
- `Adafruit BMP5xx`

WiFi, HTTPClient, WebServer, Preferences, Wire and SPI are included with the ESP32 board package.

### 3. Configure TFT_eSPI

TFT_eSPI must be configured before compilation. In the TFT_eSPI library folder, open `User_Setup.h` or create/select a custom setup through `User_Setup_Select.h`.

Use these values for the standard ESP32-2432S028R CYD:

```cpp
#define ILI9341_2_DRIVER

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SPI_FREQUENCY  55000000
```

If another display driver remains enabled in TFT_eSPI, the screen may show coloured noise or remain blank.

### 4. Open the firmware

Open:

```text
firmware/ElectrumTerminal/ElectrumTerminal.ino
```

All settings a normal user may need are grouped near the beginning under `USER CONFIGURATION`.

### 5. Change the required values

Find this block and replace the placeholders:

```cpp
const char* CONFIG_AP_PASS = "ENTER_YOUR_AP_PASSWORD";
const char* OPENWEATHER_API_KEY = "ENTER_YOUR_OPENWEATHER_API_KEY";

const float DEFAULT_LAT = 59.56;
const float DEFAULT_LON = 150.80;
const long DEFAULT_GMT_OFFSET_SEC = 11 * 3600;
```

Example for a user in UTC+3:

```cpp
const char* CONFIG_AP_PASS = "MyTerminal2026";
const char* OPENWEATHER_API_KEY = "paste_your_real_key_here";

const float DEFAULT_LAT = 55.7558;
const float DEFAULT_LON = 37.6173;
const long DEFAULT_GMT_OFFSET_SEC = 3 * 3600;
```

Do not include extra spaces outside the quotation marks, and keep the terminating semicolon.

### 6. Get an OpenWeatherMap key

1. Create an account at [OpenWeatherMap](https://openweathermap.org/).
2. Open the **API keys** section in your account.
3. Create or copy a key.
4. Paste it between the quotation marks in `OPENWEATHER_API_KEY`.

A newly created key may require some time before it becomes active. Open-Meteo weather works without a key, but OpenWeatherMap is used as a fallback and for location metadata.

### 7. Set coordinates and timezone

Use the coordinates of the location that should be shown before GPS obtains a fix.

- Open a map service.
- Right-click or long-press the desired point.
- Copy latitude and longitude in decimal degrees.
- Latitude comes first, longitude second.

Timezone values:

| Timezone | Code value |
|---|---|
| UTC−8 | `-8 * 3600` |
| UTC−5 | `-5 * 3600` |
| UTC | `0` |
| UTC+3 | `3 * 3600` |
| UTC+8 | `8 * 3600` |
| UTC+11 | `11 * 3600` |

The GPS and online weather metadata can update the active timezone later. The default value is the startup/fallback timezone.

### 8. Choose how Wi-Fi is configured

Recommended public configuration:

```cpp
const char* DEFAULT_WIFI_SSID = "";
const char* DEFAULT_WIFI_PASS = "";
```

After flashing:

1. The terminal attempts to connect to saved/default Wi-Fi.
2. If connection fails, it creates the `ElectrumTerminal` access point.
3. Connect a phone or computer to that access point using `CONFIG_AP_PASS`.
4. Open [http://192.168.4.1](http://192.168.4.1).
5. Enter the home Wi-Fi SSID and password.
6. Save the settings; the terminal restarts and stores them in Preferences.

Alternatively, a private build can contain a default network:

```cpp
const char* DEFAULT_WIFI_SSID = "YourWiFiName";
const char* DEFAULT_WIFI_PASS = "YourWiFiPassword";
```

Never commit real Wi-Fi credentials or API keys to a public repository.

### 9. Compile and upload

1. Connect the CYD to the computer with a data-capable USB cable.
2. Select the correct serial port under **Tools → Port**.
3. Click **Verify**.
4. Fix any missing-library or TFT_eSPI configuration errors.
5. Click **Upload**.
6. Open Serial Monitor at **115200 baud** for connection and sensor diagnostics.

### 10. First startup checklist

Confirm that:

- the display shows the Electrum Terminal interface rather than coloured noise;
- touch buttons respond in the correct orientation;
- the setup portal accepts Wi-Fi credentials;
- SYSTEM shows Wi-Fi as online and displays an IP address;
- GPS changes from WAIT to FIX outdoors;
- BMP580 shows Internal temperature and pressure;
- weather and market status values update without HTTP errors.

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

### Do not change for the standard CYD build

Leave these values unchanged when using the hardware and wiring documented in this repository:

- TFT pins, `DISPLAY_ROTATION` and `TFT_BACKLIGHT_PIN`
- XPT2046 pins and `TOUCH_BASE_ROTATION`
- GPS RX on GPIO35 and baud rate 9600
- BMP580 SDA GPIO27, SCL GPIO22 and address 0x47
- RGB LED pins GPIO17/GPIO16/GPIO4 and `RGB_ACTIVE_LOW`
- weather, market, sensor and redraw intervals

Change hardware pins only when you deliberately use another board or wiring layout. Change `RAW_X_MIN`, `RAW_X_MAX`, `RAW_Y_MIN` and `RAW_Y_MAX` only when touch calibration is inaccurate.

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
├── firmware/ElectrumTerminal/
│   └── ElectrumTerminal.ino
├── docs/
│   ├── HARDWARE.md
│   └── DEVELOPMENT_NOTES.md
├── hardware/
│   ├── enclosure/
│   │   ├── STL/
│   │   ├── STEP/
│   │   └── source/
│   └── wiring/
├── images/
│   ├── device/
│   ├── screens/
│   ├── assembly/
│   └── renders/
├── README.md
└── CHANGELOG.md
```

## Project status

The firmware is functional and under active development. The next recommended technical work is to improve network request scheduling, reduce dynamic-memory fragmentation, move all secrets out of source code, and update weather only when the GPS location changes materially.

## Author

Developed by [Markseder](https://github.com/markseder).
