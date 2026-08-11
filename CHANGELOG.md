# Changelog

## V4.2 — UI Status LED

- Added RGB status indication and animated update/GPS state.
- Preserved the compact top status bar.
- Added GPS information screen with coordinates, UTM zone, satellites and altitude.
- Added automatic country, city, currency and timezone metadata.
- Integrated BMP580 as the Internal sensor.
- Split HOME weather into External API and Internal sensor cards.
- Replaced the old market cards on HOME with a MENU entry point.
- Added menu pages for Weather, Quakes, FX, Commodities, GPS and System.
- Added earthquake data and calendar screen.
- Added currency and commodity boards.
- Added web-based Wi-Fi configuration and JSON status endpoint.
- Added display rotation control and reduced redraw flicker with partial sprites.
- Updated public snapshot to remove personal Wi-Fi credentials and live API keys.
- Corrected GPS wiring documentation to TX → GPIO35.

## Earlier development

Earlier V4.x iterations established the CYD display/touch setup, weather and market cards, GPS support, automatic timezone handling, BMP580 integration, screen navigation and system diagnostics.
