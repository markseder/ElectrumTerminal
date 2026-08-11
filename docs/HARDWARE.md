# Hardware and wiring

## Target board

Electrum Terminal currently targets the **ESP32-2432S028R**, commonly known as the Cheap Yellow Display (CYD).

## Display

The integrated 320 × 240 TFT is configured through TFT_eSPI.

| TFT signal | ESP32 GPIO |
|---|---:|
| MISO | 12 |
| MOSI | 13 |
| SCLK | 14 |
| CS | 15 |
| DC | 2 |
| RST | -1 |
| Backlight | 21 |

Working firmware settings:

```cpp
#define DISPLAY_ROTATION 1
tft.setRotation(displayRotation);
tft.invertDisplay(true);
```

The project uses the `ILI9341_2` driver. A wrong TFT_eSPI driver commonly produces a screen filled with coloured noise.

## Touch controller

The XPT2046 touch controller uses a separate VSPI bus:

| Touch signal | ESP32 GPIO |
|---|---:|
| CS | 33 |
| IRQ | 36 |
| MOSI | 32 |
| MISO | 39 |
| CLK | 25 |

Current raw calibration defaults:

```cpp
RAW_X_MIN 200
RAW_X_MAX 3900
RAW_Y_MIN 200
RAW_Y_MAX 3900
```

Calibration may vary between individual CYD boards.

## GPS

Only one UART direction is required:

| GPS signal | ESP32 |
|---|---|
| GPS TX | GPIO 35 |
| GPS RX | Not connected |
| GND | GND |
| VCC | According to the GPS module specification |

Default baud rate: **9600**.

GPIO35 is input-only, which is suitable for receiving NMEA data from the GPS. The firmware uses `HardwareSerial(1)` and TinyGPSPlus.

## BMP580

| BMP580 signal | ESP32 GPIO |
|---|---:|
| SDA | 27 |
| SCL | 22 |
| VCC | 3.3 V |
| GND | GND |

The confirmed I²C address is **0x47**, represented by `BMP5XX_ALTERNATIVE_ADDRESS` in the Adafruit library.

GPIO21 must not be used for I²C on this board because it controls the TFT backlight.

## RGB status LED

| Channel | ESP32 GPIO |
|---|---:|
| Red | 17 |
| Green | 16 |
| Blue | 4 |

The firmware currently treats the LED as active-low.

## Status behaviour

- Green: normal connected state
- Blue/cyan breathing: GPS activity or data update
- Alternating warning colour: degraded/offline state
- Red states are used by UI borders and indicators for unavailable data

## Power notes

The CYD, TFT backlight, Wi-Fi, GPS and sensor can create short current peaks. Use a stable power source and keep sensor wiring short. If random resets occur, inspect the serial reset reason and supply voltage before changing firmware.
