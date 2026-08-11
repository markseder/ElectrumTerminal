# Development notes

## Current baseline

- Firmware snapshot: V4.2 UI Status LED
- Board: ESP32-2432S028R CYD
- Display: ILI9341_2, rotation 1, inverted
- Touch: XPT2046 on separate VSPI pins
- GPS: TX to GPIO35, receiver only
- Internal sensor: BMP580 at 0x47 on GPIO27/22

## Architecture

The firmware is a single Arduino sketch organized into these areas:

1. hardware and service configuration
2. shared state and timers
3. HTTP and JSON helpers
4. Wi-Fi configuration server
5. GPS, timezone and location metadata
6. BMP580 sensor handling
7. external data providers
8. RGB status logic
9. TFT drawing functions
10. touch routing
11. setup and main loop

Partial TFT sprites are used for frequently redrawn regions. A full-screen 16-bit sprite would require roughly 150 KB and would compete with TLS and ArduinoJson allocations.

## Known technical debt

### Secrets

Never commit real Wi-Fi credentials or API keys. The public snapshot contains placeholders. A future version should move secrets into a local ignored header or web configuration storage.

### Blocking network calls

`HTTPClient::GET()` is synchronous. A slow provider can temporarily stop touch, GPS parsing and web-server servicing. A future version should use a request queue or a dedicated FreeRTOS task.

### GPS-triggered weather refresh

The current condition can refresh weather too frequently while a valid GPS fix continuously updates `lastGpsFixMs`. Replace it with a location-change flag and a distance threshold.

### Dynamic memory

Large HTTP response strings, TLS buffers, `DynamicJsonDocument`, and temporary TFT sprites coexist. Recommended improvements:

- ArduinoJson filters
- parsing only required fields
- tracking `ESP.getMinFreeHeap()`
- tracking `ESP.getMaxAllocHeap()`
- reducing temporary `String` construction

### Transport security

HTTPS currently uses `setInsecure()`, and OpenWeather has an HTTP fallback. Production firmware should validate CA certificates and remove plaintext API requests.

### Naming cleanup

`SCREEN_SPACE` and `SPACE_INTERVAL` now refer to the calendar and should eventually become `SCREEN_CALENDAR` and `CALENDAR_REDRAW_INTERVAL`.

## Recommended roadmap

1. Separate credentials from firmware.
2. Fix GPS location-change detection.
3. Introduce staged background data updates.
4. Add minimum-heap and reset-reason diagnostics.
5. Filter large JSON payloads.
6. Cache the last valid values in Preferences.
7. Add stale-data indicators based on last successful update.
8. Split the monolithic sketch into modules after behaviour is stable.
