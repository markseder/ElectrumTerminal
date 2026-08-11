#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <XPT2046_Touchscreen.h>
#include <time.h>
#include <math.h>
#include <TinyGPSPlus.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP5xx.h>

// =====================================================
// Electrum TERMINAL V4.2 UI-STATUS-RGB
// Board: ESP32-2432S028R CYD
// Display: TFT_eSPI, ILI9341_2
// Touch: XPT2046
// Data: Open-Meteo + OpenWeather fallback, USD/RUB, Gold, USGS Quakes, Oil
// SD: not used
// wttr.in: removed
// =====================================================

// =====================================================
// USER CONFIGURATION
// =====================================================
// REQUIRED:
// 1. Enter an OpenWeatherMap API key below. Create one at openweathermap.org.
// 2. Change CONFIG_AP_PASS. It must contain at least 8 characters.
// 3. Set DEFAULT_LAT, DEFAULT_LON and DEFAULT_GMT_OFFSET_SEC for the
//    location used before GPS obtains a fix.
//
// WI-FI OPTIONS:
// - Leave DEFAULT_WIFI_SSID and DEFAULT_WIFI_PASS empty to configure Wi-Fi
//   from the device setup portal.
// - Alternatively, enter a default network here for automatic connection.
//
// HARDWARE OPTIONS:
// - Change GPS, BMP580, touch, RGB LED and display pins only when using
//   hardware different from the documented ESP32-2432S028R CYD build.
// - Recalibrate RAW_X/Y values if touch coordinates are inaccurate.
// =====================================================

// =====================================================
// WI-FI — OPTIONAL DEFAULT NETWORK
// =====================================================
const char* DEFAULT_WIFI_SSID = "";
const char* DEFAULT_WIFI_PASS = "";

// Setup access point. Change the SSID if desired; change the password.
const char* CONFIG_AP_SSID = "ElectrumTerminal";
const char* CONFIG_AP_PASS = "ENTER_YOUR_AP_PASSWORD";

// =====================================================
// OPENWEATHERMAP API — REQUIRED FOR FALLBACK WEATHER AND LOCATION METADATA
// =====================================================
const char* OPENWEATHER_API_KEY = "ENTER_YOUR_OPENWEATHER_API_KEY";

// =====================================================
// FALLBACK LOCATION — CHANGE FOR THE USER'S REGION
// =====================================================
const float DEFAULT_LAT = 59.56;                  // Example: Magadan
const float DEFAULT_LON = 150.80;                 // Example: Magadan
const long DEFAULT_GMT_OFFSET_SEC = 11 * 3600;   // Example: UTC+11
const int DAYLIGHT_OFFSET_SEC = 0;

// GPS module is optional.
// If GPS is not connected or there is no fix, terminal uses Magadan by default.
// Wiring:
//   GPS TX -> ESP32 GPIO27
//   GPS RX -> ESP32 GPIO22  optional, can be left disconnected
//   GPS VCC -> 3.3V or 5V according to your GPS module
//   GPS GND -> GND
#define GPS_ENABLED 1
#define GPS_RX_PIN 35   // GPS TX -> ESP32 GPIO35, input only is OK for RX
#define GPS_TX_PIN -1   // GPS RX not used
#define GPS_BAUD   9600

// =====================================================
// BMP580 LOCAL SENSOR — I2C
// =====================================================
// Wiring:
//   Internal SDA -> ESP32 GPIO27
//   Internal SCL -> ESP32 GPIO22
//   Internal VCC -> 3.3V
//   Internal GND -> GND
#define BMP580_ENABLED 1
#define BMP580_SDA_PIN 27
#define BMP580_SCL_PIN 22
#define BMP580_I2C_ADDR BMP5XX_ALTERNATIVE_ADDRESS

Adafruit_BMP5xx bmp580;

bool bmp580OK = false;
String bmp580Status = "OFF";
float bmpTempC = NAN;
float bmpPressureHpa = NAN;
float bmpPressureMmHg = NAN;
float bmpAltitudeM = NAN;
unsigned long lastBmp580Ms = 0;
const unsigned long Internal_INTERVAL = 5000UL;


HardwareSerial GPSSerial(1);
TinyGPSPlus gps;

float activeLat = DEFAULT_LAT;
float activeLon = DEFAULT_LON;
long activeGmtOffsetSec = DEFAULT_GMT_OFFSET_SEC;

bool gpsStarted = false;
bool gpsFix = false;
bool gpsEverFixed = false;
int gpsSatellites = 0;
double gpsAltitudeM = NAN;
unsigned long lastGpsReadMs = 0;
unsigned long lastGpsFixMs = 0;
String locationSource = "MAGADAN";
String timezoneSource = "DEFAULT";

// =====================================================
// DISPLAY
// =====================================================
TFT_eSPI tft = TFT_eSPI();

// =====================================================
// SPRITE BUFFER
// =====================================================
// A full 320x240 16-bit framebuffer uses about 150 KB on the ESP32-2432S028R.
// Wi-Fi and JSON also require substantial memory, so the firmware uses safe
// partial double buffering for the header, cards, footer and overlay.
// Each completed block is pushed to the display at once to reduce flicker.
TFT_eSprite spr = TFT_eSprite(&tft);

bool beginSprite(int w, int h, uint16_t bg) {
  spr.setColorDepth(16);
  void* ptr = spr.createSprite(w, h);
  if (ptr == nullptr) {
    Serial.println("SPRITE create failed");
    return false;
  }
  spr.fillSprite(bg);
  return true;
}

void endSprite(int x, int y) {
  spr.pushSprite(x, y);
  spr.deleteSprite();
}


#define DISPLAY_ROTATION 1
#define TOUCH_BASE_ROTATION 1
#define TFT_BACKLIGHT_PIN 21

int W = 320;
int H = 240;

// =====================================================
// TOUCH XPT2046
// =====================================================
SPIClass touchSPI = SPIClass(VSPI);

#define TOUCH_CS   33
#define TOUCH_IRQ  36
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_CLK  25

XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// If touch is mirrored, swap the MIN/MAX values or enable TOUCH_SWAP_X/Y below.
#define RAW_X_MIN 200
#define RAW_X_MAX 3900
#define RAW_Y_MIN 200
#define RAW_Y_MAX 3900

#define TOUCH_SWAP_XY false
#define TOUCH_INVERT_X false
#define TOUCH_INVERT_Y false

// =====================================================
// COLORS
// =====================================================
#define COLOR_DARKCYAN   0x03EF
#define COLOR_PANEL      0x0841
#define COLOR_PANEL_2    0x1082
#define COLOR_GOLD       0xFEA0
#define COLOR_ORANGE     0xFD20
#define COLOR_DARKGREEN  0x0320
#define COLOR_BAD        0xF800
#define COLOR_GOOD       0x07E0

// =====================================================
// SCREEN
// =====================================================
enum ScreenMode {
  SCREEN_DASHBOARD,
  SCREEN_MENU,         // icon menu
  SCREEN_WEATHER,      // external + internal weather
  SCREEN_QUAKES,       // USGS earthquakes
  SCREEN_SPACE,        // calendar screen
  SCREEN_MARKET,
  SCREEN_OIL,          // FX board
  SCREEN_GOLDCALC,     // commodities: gold/silver/oil
  SCREEN_GPS,          // GPS details
  SCREEN_SYSTEM,
  SCREEN_WIFI_HELP
};

ScreenMode currentScreen = SCREEN_DASHBOARD;

unsigned long lastTouchMs = 0;
const unsigned long TOUCH_DEBOUNCE = 300;

// =====================================================
// BUILT-IN RGB LED
// =====================================================
#define RGB_R_PIN 17
#define RGB_G_PIN 16
#define RGB_B_PIN 4
#define RGB_ACTIVE_LOW 1

unsigned long lastRgbMs = 0;
uint8_t rgbPhase = 0;
const unsigned long RGB_INTERVAL = 20UL;  // smooth software breathing

enum RgbState {
  RGB_STATE_HEALTHY,
  RGB_STATE_GPS_UPDATE,
  RGB_STATE_PROBLEM
};

RgbState rgbState = RGB_STATE_PROBLEM;

// =====================================================
// WEB / PREFS
// =====================================================
WebServer server(80);
Preferences prefs;

String wifiSsid = "";
String wifiPass = "";

bool configMode = false;
bool dataBusy = false;
String busyText = "";

// =====================================================
// DATA
// =====================================================
float tempC = NAN;
float feelsC = NAN;
float pressureHpa = NAN;
float windMs = NAN;

const int FORECAST_DAYS = 3;
float forecastMin[FORECAST_DAYS] = {NAN, NAN, NAN};
float forecastMax[FORECAST_DAYS] = {NAN, NAN, NAN};
int forecastCode[FORECAST_DAYS] = {-1, -1, -1};
String forecastDate[FORECAST_DAYS] = {"--", "--", "--"};
bool forecastOK = false;

const int QUAKE_ITEMS = 4;
float quakeMag[QUAKE_ITEMS] = {NAN, NAN, NAN, NAN};
String quakePlace[QUAKE_ITEMS] = {"---", "---", "---", "---"};
String quakeTime[QUAKE_ITEMS] = {"--:--", "--:--", "--:--", "--:--"};
bool quakesOK = false;
String quakesStatus = "WAIT";
String quakesUpdate = "--:--";

float oilBrentUsd = NAN;
float oilWtiUsd = NAN;
float silverUsdOz = NAN;
float silverRubGram = NAN;
bool silverOK = false;
bool oilOK = false;
String oilStatus = "WAIT";
String oilUpdate = "--:--";

// =====================================================
// CALENDAR — local NTP date
// =====================================================
float spaceKpNow = NAN;
float spaceKpMax = NAN;
float spaceWindKms = NAN;
float spaceDensity = NAN;
float spaceBz = NAN;

bool spaceOK = false;
String spaceStatus = "WAIT";
String spaceUpdate = "--:--";
String spaceKpTime = "--:--";
String spaceWindTime = "--:--";

float usdRub = NAN;
float eurRub = NAN;
float thbRub = NAN;
float vnd1000Rub = NAN;

// Auto country/currency by coordinates/OpenWeather.
// Default remains Russia/RUB/Magadan.
String countryCode = "RU";
String mainCurrency = "RUB";
String locationName = "Magadan";
float mainFxRate = NAN;       // 1 USD = mainFxRate mainCurrency
float goldMainGram = NAN;     // gold in mainCurrency per gram
float prevMainFxRate = NAN;
float prevGoldMainGram = NAN;
bool locationMetaOK = false;
String locationMetaStatus = "DEFAULT";

float goldUsdOz = NAN;
float goldRubGram = NAN;

float prevUsdRub = NAN;
float prevGoldRubGram = NAN;
float prevTempC = NAN;

String weatherText = "---";
String weatherUpdate = "--:--";
String marketUpdate = "--:--";
String bootTimeStr = "--:--";

bool weatherOK = false;
bool usdOK = false;
bool goldOK = false;

int weatherFailCount = 0;
String weatherStatus = "WAIT";
String marketStatus = "WAIT";
String lastHttpHost = "---";
String lastHttpError = "---";
int lastHttpCode = 0;

unsigned long lastClockMs = 0;
unsigned long lastWeatherMs = 0;
unsigned long lastMarketMs = 0;
unsigned long lastFullRedrawMs = 0;
unsigned long lastAnimMs = 0;
unsigned long lastWifiTryMs = 0;
unsigned long lastCalendarMs = 0;
int displayRotation = DISPLAY_ROTATION;

const unsigned long WEATHER_INTERVAL = 15UL * 60UL * 1000UL;
const unsigned long MARKET_INTERVAL  = 15UL * 60UL * 1000UL;
const unsigned long SPACE_INTERVAL   = 10UL * 60UL * 1000UL;
const unsigned long FULL_REDRAW_INTERVAL = 10UL * 60UL * 1000UL;

// =====================================================
// ANIMATION
// =====================================================
int animX = 0;
int animDir = 1;

// =====================================================
// HTTP STRUCT
// =====================================================
struct HttpReply {
  int code = -999;
  String body = "";
  bool ok = false;
};

// =====================================================
// LAYOUT
// =====================================================
const int TOP_BTN_Y = 8;
const int TOP_BTN_H = 30;
const int BACK_BTN_X = 8;
const int BACK_BTN_W = 76;
const int UPDATE_BTN_W = 84;
const int ROT_BTN_X = 184;
const int ROT_BTN_Y = 8;
const int ROT_BTN_W = 38;
const int ROT_BTN_H = 22;
int updateBtnX() { return W - UPDATE_BTN_W - 8; }

// =====================================================
// HELPERS
// =====================================================
String twoDigits(int v) {
  if (v < 10) return "0" + String(v);
  return String(v);
}

String getTimeStr() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 200)) return "--:--:--";

  return twoDigits(timeinfo.tm_hour) + ":" +
         twoDigits(timeinfo.tm_min) + ":" +
         twoDigits(timeinfo.tm_sec);
}

String getShortTimeStr() {
  String s = getTimeStr();
  if (s.length() >= 5) return s.substring(0, 5);
  return "--:--";
}

String getDateStr() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 200)) return "--.--.----";

  return twoDigits(timeinfo.tm_mday) + "." +
         twoDigits(timeinfo.tm_mon + 1) + "." +
         String(timeinfo.tm_year + 1900);
}

String uptimeStr() {
  unsigned long s = millis() / 1000UL;
  unsigned long h = s / 3600UL;
  unsigned long m = (s % 3600UL) / 60UL;
  unsigned long sec = s % 60UL;

  if (h > 99) h = 99;
  return twoDigits(h) + ":" + twoDigits(m) + ":" + twoDigits(sec);
}

String floatText(float v, uint8_t digits) {
  if (isnan(v)) return "---";
  return String((double)v, (unsigned int)digits);
}

String rubCompact(double v) {
  if (isnan(v) || v <= 0) return "---";
  if (v >= 1000000000.0) return String(v / 1000000000.0, 2) + " B RUB";
  if (v >= 1000000.0) return String(v / 1000000.0, 2) + " M RUB";
  if (v >= 1000.0) return String(v / 1000.0, 1) + " K RUB";
  return String(v, 0) + " RUB";
}

String dayName3(int i) {
  if (i == 0) return "TODAY";
  if (i == 1) return "TOMOR";
  return "DAY+2";
}


String pressureMmHgText(float hpa, uint8_t digits = 0) {
  if (isnan(hpa)) return "---";
  float mmhg = hpa * 0.750061683f;
  return String((double)mmhg, (unsigned int)digits);
}

float pressureMmHgValue(float hpa) {
  if (isnan(hpa)) return NAN;
  return hpa * 0.750061683f;
}

uint16_t pressureColorMmHg(float hpa) {
  float mmhg = pressureMmHgValue(hpa);
  if (isnan(mmhg)) return TFT_LIGHTGREY;

  if (mmhg < 750.0f) return TFT_CYAN;   // Low pressure
  if (mmhg <= 760.0f) return TFT_GREEN; // Normal range
  return TFT_RED;                       // High pressure
}

uint16_t weatherTitleColor(float temp) {
  if (isnan(temp)) return TFT_CYAN;

  if (temp < 0.0f) return TFT_CYAN;     // Freezing
  if (temp <= 13.0f) return TFT_GREEN;  // Cool/normal
  return TFT_RED;                       // Warm/hot
}

String shortPayload(String s, int maxLen = 220) {
  s.replace("\n", " ");
  s.replace("\r", " ");
  if (s.length() <= maxLen) return s;
  return s.substring(0, maxLen) + "...";
}

String shortenWeather(String s) {
  s.toLowerCase();

  if (s.indexOf("clear") >= 0) return "Clear";
  if (s.indexOf("cloud") >= 0) return "Clouds";
  if (s.indexOf("rain") >= 0) return "Rain";
  if (s.indexOf("snow") >= 0) return "Snow";
  if (s.indexOf("mist") >= 0) return "Mist";
  if (s.indexOf("fog") >= 0) return "Fog";
  if (s.indexOf("thunder") >= 0) return "Storm";

  if (s.length() > 18) return s.substring(0, 18);
  return s;
}

String weatherCodeToText(int code) {
  if (code == 0) return "Clear";
  if (code == 1 || code == 2 || code == 3) return "Clouds";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Rain";
  if (code >= 85 && code <= 86) return "Snow";
  if (code >= 95) return "Storm";
  return "Weather";
}

uint16_t trendColor(float nowVal, float prevVal) {
  if (isnan(nowVal) || isnan(prevVal)) return TFT_LIGHTGREY;
  if (nowVal > prevVal) return TFT_GREEN;
  if (nowVal < prevVal) return TFT_RED;
  return TFT_LIGHTGREY;
}

String trendText(float nowVal, float prevVal) {
  if (isnan(nowVal) || isnan(prevVal)) return "--";
  float diff = nowVal - prevVal;
  if (fabs(diff) < 0.01) return "FLAT";
  if (diff > 0) return "UP";
  return "DOWN";
}

bool inBox(int x, int y, int bx, int by, int bw, int bh) {
  return x >= bx && x <= bx + bw && y >= by && y <= by + bh;
}

uint16_t okColor(bool ok) {
  return ok ? TFT_GREEN : TFT_RED;
}

String wifiStatusText() {
  if (configMode) return "SETUP";
  if (WiFi.status() == WL_CONNECTED) return "ONLINE";
  return "OFFLINE";
}

String wifiIpText() {
  if (configMode) return WiFi.softAPIP().toString();
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return "---.---.---.---";
}

String getHostFromUrl(String url) {
  url.replace("https://", "");
  url.replace("http://", "");
  int slash = url.indexOf('/');
  if (slash >= 0) url = url.substring(0, slash);
  return url;
}

// =====================================================
// TOUCH
// =====================================================
bool getTouchXY(int &x, int &y) {
  if (!ts.touched()) return false;

  TS_Point p = ts.getPoint();

  int sx = map(p.x, RAW_X_MIN, RAW_X_MAX, 0, W - 1);
  int sy = map(p.y, RAW_Y_MIN, RAW_Y_MAX, 0, H - 1);

  sx = constrain(sx, 0, W - 1);
  sy = constrain(sy, 0, H - 1);

  if (TOUCH_INVERT_X) sx = W - 1 - sx;
  if (TOUCH_INVERT_Y) sy = H - 1 - sy;

  // Rotate touch coordinates when the display is rotated by 180 degrees.
  // Do not change ts.setRotation() here, otherwise coordinates are corrected twice.
  if (displayRotation == 3) {
    sx = W - 1 - sx;
    sy = H - 1 - sy;
  }

  if (TOUCH_SWAP_XY) {
    int tmp = sx;
    sx = sy;
    sy = tmp;
  }

  x = sx;
  y = sy;

  Serial.print("TOUCH RAW x=");
  Serial.print(p.x);
  Serial.print(" y=");
  Serial.print(p.y);
  Serial.print(" z=");
  Serial.println(p.z);

  Serial.print("TOUCH SCREEN x=");
  Serial.print(x);
  Serial.print(" y=");
  Serial.println(y);

  return true;
}

// =====================================================
// WIFI STORAGE
// =====================================================
void loadWiFiSettings() {
  prefs.begin("wifi", true);
  wifiSsid = prefs.getString("ssid", "");
  wifiPass = prefs.getString("pass", "");
  prefs.end();

  if (wifiSsid.length() == 0) {
    wifiSsid = DEFAULT_WIFI_SSID;
    wifiPass = DEFAULT_WIFI_PASS;
  }

  Serial.print("Loaded WiFi SSID: ");
  Serial.println(wifiSsid);
}

void saveWiFiSettings(const String& ssid, const String& pass) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();

  Serial.println("WiFi settings saved");
}

// Forward declaration for rotate helper.
void redrawCurrentScreen();

// =====================================================
// UI STORAGE / ROTATION
// =====================================================
void loadUiSettings() {
  prefs.begin("ui", true);
  displayRotation = prefs.getInt("rot", DISPLAY_ROTATION);
  prefs.end();

  if (displayRotation != 1 && displayRotation != 3) displayRotation = DISPLAY_ROTATION;
}

void saveDisplayRotation() {
  prefs.begin("ui", false);
  prefs.putInt("rot", displayRotation);
  prefs.end();
}

void applyDisplayRotation() {
  tft.setRotation(displayRotation);

  // Keep the XPT2046 in its base orientation.
  // TFT_eSPI rotates the display while getTouchXY() rotates touch coordinates manually.
  // Combining ts.setRotation(3) with manual X/Y inversion would rotate touch twice.
  ts.setRotation(TOUCH_BASE_ROTATION);

  W = tft.width();
  H = tft.height();
}

void toggleDisplayRotation() {
  displayRotation = (displayRotation == 1) ? 3 : 1;
  applyDisplayRotation();
  saveDisplayRotation();
  redrawCurrentScreen();
}

// =====================================================
// DNS
// =====================================================
bool resolveHost(const char* host) {
  IPAddress ip;

  Serial.print("DNS resolve: ");
  Serial.println(host);

  bool ok = WiFi.hostByName(host, ip);

  if (ok) {
    Serial.print("DNS OK: ");
    Serial.println(ip);
    return true;
  }

  Serial.println("DNS FAIL");
  lastHttpError = "DNS FAIL: " + String(host);
  return false;
}

// =====================================================
// DRAW HELPERS - prototypes before HTTP busy overlay use
// =====================================================
void initGPS();
void pollGPS(uint16_t maxMs);
String gpsStatusText();
String activeCoordText();
String currencyByCountry(String cc);
void loadLocationCurrencySettings();
void saveLocationCurrencySettings();
void applyCountryCurrency(const String& cc, const String& name, bool persist = true);
bool fetchLocationMeta();
bool parseOpenWeatherMetaPayload(const String& body);
void scanI2CBus();
void initInternal();
bool readInternal();
String bmp580ShortStatus();
void loadTimezoneSettings();
void saveTimezoneSettings(long offsetSec, const String& src);
void applyTimeOffset(long offsetSec, const String& src, bool persist = true);

void redrawCurrentScreen();
void updateCurrentScreenData();
void drawBusyOverlay(const String& msg);
void drawDashboard();
void drawMenuScreen();
void drawWeatherScreen();
void drawQuakeScreen();
void drawCalendarScreen();
void drawMarketScreen();
void drawOilScreen();
void drawGoldCalcScreen();
void drawGpsScreen();
void drawSystemScreen();
void toggleDisplayRotation();
void drawWiFiHelpScreen();
void drawHeaderDirect();
void drawHeader();
void drawWeatherCardDirect();
void drawWeatherCard();
void drawMarketCardDirect();
void drawMarketCard();
void drawMiniFooterDirect();
void drawMiniFooter();
void initRgbLed();
void animateRgbLed();
void setRgbLed(bool r, bool g, bool b);
String utmZoneText(double lat, double lon);

// =====================================================
// HTTP
// =====================================================
HttpReply httpGET(const String& url, int timeoutMs = 6000) {
  HttpReply result;
  lastHttpHost = getHostFromUrl(url);

  HTTPClient http;
  http.setConnectTimeout(timeoutMs);
  http.setTimeout(timeoutMs);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.useHTTP10(true);

  Serial.println();
  Serial.println("GET:");
  Serial.println(url);

  bool beginOK = false;

  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  if (url.startsWith("https://")) {
    beginOK = http.begin(secureClient, url);
  } else {
    beginOK = http.begin(plainClient, url);
  }

  if (!beginOK) {
    Serial.println("HTTP begin failed");
    result.code = -1000;
    result.ok = false;
    lastHttpCode = result.code;
    lastHttpError = "HTTP BEGIN FAIL";
    return result;
  }

  http.addHeader("User-Agent", "ESP32-Electrum-Terminal/2.3");
  http.addHeader("Connection", "close");
  http.addHeader("Accept", "application/json");

  result.code = http.GET();
  result.body = http.getString();
  result.ok = result.code == HTTP_CODE_OK;

  lastHttpCode = result.code;
  if (result.ok) {
    lastHttpError = "OK";
  } else {
    lastHttpError = "HTTP " + String(result.code);
  }

  Serial.print("HTTP code: ");
  Serial.println(result.code);

  Serial.print("Payload size: ");
  Serial.println(result.body.length());

  Serial.print("Payload: ");
  Serial.println(shortPayload(result.body));

  http.end();
  delay(50);

  return result;
}

HttpReply httpGETRetry(const String& url, int attempts = 1, int pauseMs = 500, int timeoutMs = 5500) {
  HttpReply r;

  for (int i = 1; i <= attempts; i++) {
    Serial.println();
    Serial.print("HTTP attempt ");
    Serial.print(i);
    Serial.print("/");
    Serial.println(attempts);

    r = httpGET(url, timeoutMs);

    if (r.ok && r.body.length() > 0) {
      Serial.println("HTTP OK");
      return r;
    }

    Serial.print("HTTP failed, code: ");
    Serial.println(r.code);

    if (i < attempts) delay(pauseMs);
  }

  Serial.println("HTTP all attempts failed");
  return r;
}

// =====================================================
// WEB PAGE
// =====================================================
String htmlPage() {
  String ipInfo = wifiIpText();

  String page = R"=====(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Electrum TERMINAL</title>
<style>
:root{--bg:#070b14;--card:#101827;--card2:#0b1220;--cyan:#00e5ff;--green:#39ff88;--red:#ff3b6b;--yellow:#ffe14d;--muted:#8aa0b7;--line:#1d3350;}
*{box-sizing:border-box} body{margin:0;font-family:Arial,Helvetica,sans-serif;background:radial-gradient(circle at top,#153052 0,#070b14 42%,#03050a 100%);color:#e8f7ff;padding:18px;}
.wrap{max-width:980px;margin:auto}.top{display:flex;justify-content:space-between;gap:12px;align-items:center;margin-bottom:14px}.brand{font-size:24px;font-weight:800;letter-spacing:1px;color:var(--cyan);text-shadow:0 0 18px #00e5ff80}.pill{border:1px solid var(--line);background:#07101d;border-radius:999px;padding:7px 12px;color:var(--muted);font-size:13px}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}.card{background:linear-gradient(180deg,var(--card),var(--card2));border:1px solid var(--line);border-radius:18px;padding:14px;box-shadow:0 10px 30px #0008, inset 0 0 20px #00e5ff08}.wide{grid-column:span 2}.full{grid-column:span 4}.title{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:1.5px;margin-bottom:8px}.val{font-size:27px;font-weight:800;line-height:1.15}.small{font-size:13px;color:var(--muted);line-height:1.35}.ok{color:var(--green)}.bad{color:var(--red)}.warn{color:var(--yellow)}.cyan{color:var(--cyan)}.red{color:var(--red)}.green{color:var(--green)}.yellow{color:var(--yellow)}.row{display:flex;justify-content:space-between;border-bottom:1px solid #18304a;padding:7px 0;gap:10px}.row:last-child{border-bottom:0}.btn{display:inline-block;width:100%;border:0;border-radius:12px;background:linear-gradient(90deg,#00e5ff,#39ff88);color:#001018;font-weight:800;padding:12px;margin-top:10px}.input{width:100%;padding:12px;margin-top:6px;border-radius:10px;border:1px solid #29425f;background:#050912;color:#fff}.form{display:grid;grid-template-columns:1fr 1fr auto;gap:10px;align-items:end}.dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:7px;background:var(--red);box-shadow:0 0 10px var(--red)}.dot.on{background:var(--green);box-shadow:0 0 10px var(--green)}.quake{display:grid;grid-template-columns:68px 1fr 52px;gap:10px;align-items:center;border-bottom:1px solid #18304a;padding:8px 0}.quake:last-child{border-bottom:0}.mag{font-size:20px;font-weight:800;color:var(--yellow)}.place{font-size:13px;color:#e8f7ff}.qtime{font-size:12px;color:var(--muted);text-align:right}.footer{margin-top:12px;color:var(--muted);font-size:12px;text-align:center}a{color:var(--cyan)}
@media(max-width:760px){.grid{grid-template-columns:1fr}.wide,.full{grid-column:span 1}.form{grid-template-columns:1fr}.top{display:block}.pill{margin-top:8px;display:inline-block}.val{font-size:24px}}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div class="brand">ELECTRUM TERMINAL</div>
    <div class="pill"><span id="dot" class="dot"></span><span id="wifi">loading</span> · <span id="ip">---</span></div>
  </div>

  <div class="grid">
    <div class="card wide">
      <div class="title">Weather</div>
      <div class="val" id="temp">-- °C</div>
      <div class="small"><span id="weatherText">---</span> · feels <span id="feels">--</span> °C</div>
      <div class="small">Pressure: <b id="press">--</b> mmHg · Wind: <b id="wind">--</b> m/s</div>
      <div class="small">Status: <span id="weatherStatus">---</span></div>
      <div class="small">Internal: <span id="bmpStatus">---</span> · <b id="bmpTemp">--</b> °C · <b id="bmpPress">--</b> mmHg</div>
      <div class="small">Location: <b id="locsrc">---</b> · <span id="coords">---</span></div>
      <div class="small">GPS: <span id="gps">---</span> · TZ: <span id="tz">---</span></div>
    </div>

    <div class="card wide">
      <div class="title">Earthquakes</div>
      <div class="small">USGS M4.5+ · Status: <span id="quakesStatus">---</span> · Updated: <span id="quakesUpdate">--:--</span></div>
      <div id="quakeList" class="small" style="margin-top:8px">loading...</div>
    </div>

    <div class="card">
      <div class="title">USD/RUB</div>
      <div class="val yellow" id="usd">--</div>
      <div class="small">Market: <span id="marketStatus">---</span></div>
    </div>

    <div class="card">
      <div class="title">Gold</div>
      <div class="val yellow" id="gold">--</div>
      <div class="small">RUB per gram</div>
    </div>

    <div class="card wide">
      <div class="title">FX Board</div>
      <div class="row"><span>EUR/RUB</span><b id="eur">--</b></div>
      <div class="row"><span>THB/RUB</span><b id="thb">--</b></div>
      <div class="row"><span>1000 VND/RUB</span><b id="vnd">--</b></div>
    </div>

    <div class="card wide">
      <div class="title">Commodities</div>
      <div class="row"><span>Silver XAG</span><b id="silver">--</b></div>
      <div class="row"><span>Brent</span><b id="brent">--</b></div>
      <div class="row"><span>WTI</span><b id="wti">--</b></div>
    </div>

    <div class="card full">
      <div class="title">System</div>
      <div class="row"><span>Heap</span><b id="heap">--</b></div>
      <div class="row"><span>Uptime</span><b id="uptime">--</b></div>
      <div class="row"><span>HTTP</span><b><span id="httpCode">--</span> <span id="httpHost">---</span></b></div>
      <div class="row"><span>Error</span><b id="err">--</b></div>
    </div>

    <div class="card full">
      <div class="title">WiFi setup</div>
      <form class="form" action="/save" method="POST">
        <label><span class="small">SSID</span><input class="input" name="ssid" placeholder="Network name" required></label>
        <label><span class="small">Password</span><input class="input" name="pass" type="password" placeholder="Password"></label>
        <button class="btn" type="submit">SAVE & REBOOT</button>
      </form>
      <div class="small">Current SSID: <b id="ssid">---</b>. Status JSON: <a href="/status">/status</a></div>
    </div>
  </div>
  <div class="footer">Auto refresh every 3 seconds · Electrum TERMINAL CYD</div>
</div>
<script>
function n(v,d=2){return (v===0||v)?Number(v).toFixed(d):'--'}
function cls(el,c){el.className=c}
async function load(){
  try{
    const r=await fetch('/status?ts='+Date.now());
    const s=await r.json();
    document.getElementById('wifi').textContent=s.wifi||'---';
    document.getElementById('ip').textContent=s.ip||'---';
    document.getElementById('ssid').textContent=s.ssid||'---';
    document.getElementById('dot').className=(s.wifi==='ONLINE')?'dot on':'dot';
    document.getElementById('temp').textContent=n(s.temp_c,1)+' °C';
    document.getElementById('temp').className='val '+(s.temp_c<0?'cyan':(s.temp_c<=13?'green':'red'));
    document.getElementById('feels').textContent=n(s.feels_c,1);
    document.getElementById('press').textContent=n(s.pressure_mmhg,0);
    document.getElementById('press').className=(s.pressure_mmhg<750?'cyan':(s.pressure_mmhg<=760?'green':'red'));
    document.getElementById('wind').textContent=n(s.wind_ms,1);
    document.getElementById('weatherText').textContent=s.weather_text||'---';
    document.getElementById('weatherStatus').textContent=s.weather_status||'---';
    document.getElementById('weatherStatus').className=s.weather_ok?'ok':'bad';
    document.getElementById('bmpStatus').textContent=s.bmp580_status||'---';
    document.getElementById('bmpStatus').className=s.bmp580_ok?'ok':'warn';
    document.getElementById('bmpTemp').textContent=n(s.bmp580_temp_c,1);
    document.getElementById('bmpPress').textContent=n(s.bmp580_pressure_mmhg,0);
    document.getElementById('bmpPress').className=(s.bmp580_pressure_mmhg<750?'cyan':(s.bmp580_pressure_mmhg<=760?'green':'red'));
    document.getElementById('locsrc').textContent=(s.location_name||s.location_source||'---')+' '+(s.country||'')+'/'+(s.main_currency||'');
    document.getElementById('coords').textContent=(s.lat&&s.lon)?(Number(s.lat).toFixed(4)+', '+Number(s.lon).toFixed(4)):'---';
    document.getElementById('gps').textContent=(s.gps_fix?'FIX ':'NO FIX ')+(s.gps_sat||0)+' sat';
    document.getElementById('gps').className=s.gps_fix?'ok':'warn';
    document.getElementById('tz').textContent=(s.tz_source||'---')+' '+((s.tz_offset_sec||0)/3600)+'h';

    document.getElementById('quakesStatus').textContent=s.quakes_status||'---';
    document.getElementById('quakesStatus').className=s.quakes_ok?'ok':'warn';
    document.getElementById('quakesUpdate').textContent=s.quakes_update||'--:--';
    const qbox=document.getElementById('quakeList');
    const qs=s.quakes||[];
    qbox.innerHTML='';
    let shown=0;
    qs.forEach(q=>{
      if(!q || !q.mag) return;
      shown++;
      const div=document.createElement('div');
      div.className='quake';
      div.innerHTML='<div class="mag">M '+n(q.mag,1)+'</div><div class="place">'+(q.place||'---')+'</div><div class="qtime">'+(q.time||'--:--')+'</div>';
      qbox.appendChild(div);
    });
    if(!shown) qbox.innerHTML='<div class="small">No M4.5+ events loaded. Open QUAKES on device or tap UPDATE there.</div>';

    document.getElementById('usd').textContent=n(s.usd_rub,2);
    document.getElementById('gold').textContent=n(s.gold_rub_g,0);
    document.getElementById('eur').textContent=n(s.eur_rub,2);
    document.getElementById('thb').textContent=n(s.thb_rub,3);
    document.getElementById('vnd').textContent=n(s.vnd1000_rub,2);
    document.getElementById('silver').textContent=n(s.silver_usd_oz,2)+' USD/oz';
    document.getElementById('brent').textContent=n(s.brent_usd,2)+' USD';
    document.getElementById('wti').textContent=n(s.wti_usd,2)+' USD';
    document.getElementById('marketStatus').textContent=(s.usd_ok&&s.gold_ok)?'OK':'CHECK';
    document.getElementById('marketStatus').className=(s.usd_ok&&s.gold_ok)?'ok':'warn';
    document.getElementById('heap').textContent=Math.round((s.heap||0)/1024)+' KB';
    document.getElementById('uptime').textContent=s.uptime||'--';
    document.getElementById('httpCode').textContent=s.last_http_code||'--';
    document.getElementById('httpHost').textContent=s.last_http_host||'---';
    document.getElementById('err').textContent=s.last_http_error||'--';
    document.getElementById('err').className=(s.last_http_error==='OK')?'ok':'warn';
  }catch(e){document.getElementById('wifi').textContent='WEB ERROR';document.getElementById('dot').className='dot';}
}
load();setInterval(load,3000);
</script>
</body></html>)=====";
  return page;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  ssid.trim();

  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID is empty");
    return;
  }

  saveWiFiSettings(ssid, pass);

  String response = "";
  response += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  response += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  response += "</head><body style='font-family:Arial;background:#111;color:#eee;padding:24px;'>";
  response += "<h2>Saved</h2>";
  response += "<p>WiFi settings saved. Electrum TERMINAL will reboot now.</p>";
  response += "</body></html>";

  server.send(200, "text/html", response);

  delay(1200);
  ESP.restart();
}

void handleApiStatus() {
  DynamicJsonDocument doc(6144);
  doc["wifi"] = wifiStatusText();
  doc["ip"] = wifiIpText();
  doc["ssid"] = wifiSsid;
  doc["weather_ok"] = weatherOK;
  doc["usd_ok"] = usdOK;
  doc["gold_ok"] = goldOK;
  doc["oil_ok"] = oilOK;
  doc["quakes_ok"] = quakesOK;
  doc["calendar_date"] = getDateStr();
  doc["space_kp"] = isnan(spaceKpNow) ? 0 : spaceKpNow;
  doc["calendar_time"] = getTimeStr();
  doc["gps_enabled"] = GPS_ENABLED;
  doc["gps_fix"] = gpsFix;
  doc["gps_sat"] = gpsSatellites;
  doc["gps_alt_m"] = isnan(gpsAltitudeM) ? 0 : gpsAltitudeM;
  doc["location_source"] = locationSource;
  doc["lat"] = activeLat;
  doc["lon"] = activeLon;
  doc["tz_offset_sec"] = activeGmtOffsetSec;
  doc["tz_source"] = timezoneSource;
  doc["country"] = countryCode;
  doc["main_currency"] = mainCurrency;
  doc["location_name"] = locationName;
  doc["location_meta_status"] = locationMetaStatus;
  doc["main_fx_rate"] = isnan(mainFxRate) ? 0 : mainFxRate;
  doc["gold_main_g"] = isnan(goldMainGram) ? 0 : goldMainGram;
  doc["temp_c"] = isnan(tempC) ? 0 : tempC;
  doc["feels_c"] = isnan(feelsC) ? 0 : feelsC;
  doc["pressure_hpa"] = isnan(pressureHpa) ? 0 : pressureHpa;
  doc["pressure_mmhg"] = isnan(pressureHpa) ? 0 : pressureMmHgValue(pressureHpa);
  doc["wind_ms"] = isnan(windMs) ? 0 : windMs;
  doc["weather_text"] = weatherText;
  doc["weather_status"] = weatherStatus;
  doc["bmp580_ok"] = bmp580OK;
  doc["bmp580_status"] = bmp580Status;
  doc["bmp580_temp_c"] = isnan(bmpTempC) ? 0 : bmpTempC;
  doc["bmp580_pressure_hpa"] = isnan(bmpPressureHpa) ? 0 : bmpPressureHpa;
  doc["bmp580_pressure_mmhg"] = isnan(bmpPressureMmHg) ? 0 : bmpPressureMmHg;
  doc["bmp580_alt_m"] = isnan(bmpAltitudeM) ? 0 : bmpAltitudeM;
  doc["usd_rub"] = isnan(usdRub) ? 0 : usdRub;
  doc["gold_rub_g"] = isnan(goldRubGram) ? 0 : goldRubGram;
  doc["silver_usd_oz"] = isnan(silverUsdOz) ? 0 : silverUsdOz;
  doc["brent_usd"] = isnan(oilBrentUsd) ? 0 : oilBrentUsd;
  doc["wti_usd"] = isnan(oilWtiUsd) ? 0 : oilWtiUsd;
  doc["eur_rub"] = isnan(eurRub) ? 0 : eurRub;
  doc["thb_rub"] = isnan(thbRub) ? 0 : thbRub;
  doc["vnd1000_rub"] = isnan(vnd1000Rub) ? 0 : vnd1000Rub;
  doc["rotation"] = displayRotation;
  doc["oil_brent_usd"] = isnan(oilBrentUsd) ? 0 : oilBrentUsd;
  doc["oil_wti_usd"] = isnan(oilWtiUsd) ? 0 : oilWtiUsd;
  doc["last_http_host"] = lastHttpHost;
  doc["last_http_code"] = lastHttpCode;
  doc["last_http_error"] = lastHttpError;
  doc["heap"] = ESP.getFreeHeap();
  doc["uptime"] = uptimeStr();

  doc["quakes_status"] = quakesStatus;
  doc["quakes_update"] = quakesUpdate;
  JsonArray qa = doc.createNestedArray("quakes");
  for (int i = 0; i < QUAKE_ITEMS; i++) {
    JsonObject q = qa.createNestedObject();
    q["mag"] = isnan(quakeMag[i]) ? 0 : quakeMag[i];
    q["place"] = quakePlace[i];
    q["time"] = quakeTime[i];
  }

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/status", HTTP_GET, handleApiStatus);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.begin();

  Serial.println("Web server started");
}

// =====================================================
// WIFI
// =====================================================
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  WiFi.config(
    INADDR_NONE,
    INADDR_NONE,
    INADDR_NONE,
    IPAddress(1, 1, 1, 1),
    IPAddress(8, 8, 8, 8)
  );

  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());

  Serial.print("WiFi connecting to ");
  Serial.println(wifiSsid);

  int tries = 0;

  while (WiFi.status() != WL_CONNECTED && tries < 60) {
    delay(250);
    Serial.print(".");
    tries++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi OK");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("DNS 1: ");
    Serial.println(WiFi.dnsIP(0));
    Serial.print("DNS 2: ");
    Serial.println(WiFi.dnsIP(1));

    configMode = false;
    return true;
  }

  Serial.println("WiFi FAIL");
  return false;
}

void startConfigAP() {
  configMode = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(CONFIG_AP_SSID, CONFIG_AP_PASS);

  Serial.println("Config AP started");
  Serial.print("AP SSID: ");
  Serial.println(CONFIG_AP_SSID);
  Serial.print("AP PASS: ");
  Serial.println(CONFIG_AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void reconnectWiFiIfNeeded() {
  if (configMode) return;
  if (WiFi.status() == WL_CONNECTED) return;

  if (millis() - lastWifiTryMs > 15000) {
    lastWifiTryMs = millis();

    Serial.println("WiFi reconnect...");
    WiFi.disconnect();
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  }
}



// =====================================================
// TIMEZONE STORAGE
// =====================================================
void loadTimezoneSettings() {
  prefs.begin("tz", true);
  bool saved = prefs.getBool("saved", false);
  long savedOffset = prefs.getLong("offset", DEFAULT_GMT_OFFSET_SEC);
  String savedSource = prefs.getString("source", "DEFAULT");
  prefs.end();

  if (saved && savedOffset >= -12L * 3600L && savedOffset <= 14L * 3600L) {
    activeGmtOffsetSec = savedOffset;
    timezoneSource = "SAVED " + savedSource;
  } else {
    activeGmtOffsetSec = DEFAULT_GMT_OFFSET_SEC;
    timezoneSource = "DEFAULT";
  }

  Serial.print("Timezone loaded: ");
  Serial.print(activeGmtOffsetSec);
  Serial.print(" source: ");
  Serial.println(timezoneSource);
}

void saveTimezoneSettings(long offsetSec, const String& src) {
  if (offsetSec < -12L * 3600L || offsetSec > 14L * 3600L) return;

  prefs.begin("tz", false);
  prefs.putBool("saved", true);
  prefs.putLong("offset", offsetSec);
  prefs.putString("source", src);
  prefs.end();

  Serial.print("Timezone saved: ");
  Serial.print(offsetSec);
  Serial.print(" source: ");
  Serial.println(src);
}


// =====================================================
// COUNTRY / CURRENCY HELPERS
// =====================================================
String currencyByCountry(String cc) {
  cc.toUpperCase();

  if (cc == "RU") return "RUB";
  if (cc == "JP") return "JPY";
  if (cc == "TH") return "THB";
  if (cc == "VN") return "VND";
  if (cc == "PH") return "PHP";
  if (cc == "AU") return "AUD";
  if (cc == "US") return "USD";
  if (cc == "CN") return "CNY";
  if (cc == "KR") return "KRW";
  if (cc == "GB") return "GBP";
  if (cc == "CA") return "CAD";
  if (cc == "NZ") return "NZD";
  if (cc == "SG") return "SGD";
  if (cc == "MY") return "MYR";
  if (cc == "ID") return "IDR";
  if (cc == "IN") return "INR";
  if (cc == "TR") return "TRY";
  if (cc == "AE") return "AED";
  if (cc == "KZ") return "KZT";

  // Eurozone countries.
  if (cc == "DE" || cc == "FR" || cc == "IT" || cc == "ES" ||
      cc == "NL" || cc == "FI" || cc == "EE" || cc == "LV" ||
      cc == "LT" || cc == "PT" || cc == "AT" || cc == "BE" ||
      cc == "IE" || cc == "GR" || cc == "SK" || cc == "SI" ||
      cc == "HR" || cc == "CY" || cc == "LU" || cc == "MT") {
    return "EUR";
  }

  return "USD";
}

void loadLocationCurrencySettings() {
  prefs.begin("loc", true);
  countryCode = prefs.getString("country", "RU");
  mainCurrency = prefs.getString("currency", "RUB");
  locationName = prefs.getString("name", "Magadan");
  prefs.end();

  countryCode.toUpperCase();
  mainCurrency.toUpperCase();

  if (countryCode.length() != 2) countryCode = "RU";
  if (mainCurrency.length() < 3) mainCurrency = currencyByCountry(countryCode);
  if (locationName.length() == 0) locationName = "Magadan";

  locationMetaStatus = "SAVED";
}

void saveLocationCurrencySettings() {
  prefs.begin("loc", false);
  prefs.putString("country", countryCode);
  prefs.putString("currency", mainCurrency);
  prefs.putString("name", locationName);
  prefs.end();
}

void applyCountryCurrency(const String& ccRaw, const String& nameRaw, bool persist) {
  String cc = ccRaw;
  cc.trim();
  cc.toUpperCase();

  if (cc.length() != 2) return;

  String cur = currencyByCountry(cc);
  String name = nameRaw;
  name.trim();

  countryCode = cc;
  mainCurrency = cur;
  if (name.length() > 0) locationName = name;

  locationMetaOK = true;
  locationMetaStatus = "OW " + countryCode + "/" + mainCurrency;

  if (persist) saveLocationCurrencySettings();

  Serial.print("Location currency: ");
  Serial.print(countryCode);
  Serial.print(" ");
  Serial.print(mainCurrency);
  Serial.print(" ");
  Serial.println(locationName);
}

bool parseOpenWeatherMetaPayload(const String& body) {
  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    Serial.print("OpenWeather meta JSON error: ");
    Serial.println(err.c_str());
    locationMetaStatus = "META JSON";
    return false;
  }

  int cod = doc["cod"].as<int>();
  if (cod != 200) {
    locationMetaStatus = "META " + String(cod);
    return false;
  }

  if (doc["timezone"].is<long>()) {
    long owOffset = doc["timezone"].as<long>();
    applyTimeOffset(owOffset, locationSource == "GPS" ? "OW GPS" : "OW DEFAULT");
  }

  String owCountry = String((const char*)(doc["sys"]["country"] | ""));
  String owName = String((const char*)(doc["name"] | ""));
  applyCountryCurrency(owCountry, owName, true);

  String cc = String((const char*)(doc["sys"]["country"] | ""));
  String name = String((const char*)(doc["name"] | ""));
  applyCountryCurrency(cc, name, true);

  return locationMetaOK;
}

bool fetchLocationMeta() {
  if (WiFi.status() != WL_CONNECTED) {
    locationMetaStatus = "NO WIFI";
    return false;
  }

  String query = "lat=" + String(activeLat, 4);
  query += "&lon=" + String(activeLon, 4);
  query += "&appid=" + String(OPENWEATHER_API_KEY);
  query += "&units=metric";
  query += "&lang=en";

  String url = "http://api.openweathermap.org/data/2.5/weather?" + query;

  Serial.println();
  Serial.println("Location meta: OpenWeather country/timezone");
  HttpReply r = httpGETRetry(url, 1, 500, 4500);

  if (!r.ok || r.body.length() == 0) {
    locationMetaStatus = "META HTTP";
    return false;
  }

  return parseOpenWeatherMetaPayload(r.body);
}


// =====================================================
// BMP580 HELPERS
// =====================================================
void scanI2CBus() {
#if BMP580_ENABLED
  Serial.println("I2C scan on SDA=GPIO27 SCL=GPIO22");
  byte count = 0;

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      count++;
    }
  }

  if (count == 0) {
    Serial.println("I2C scan: no devices found");
  }
#endif
}

void initInternal() {
#if BMP580_ENABLED
  Wire.begin(BMP580_SDA_PIN, BMP580_SCL_PIN);
  Wire.setClock(100000);   // safe speed for long jumper wires

  bmp580OK = false;
  bmp580Status = "INIT";

  scanI2CBus();

  // Internal breakout boards usually use 0x46 or 0x47.
  // Try both explicitly.
  bool ok = false;

  Serial.println("Trying Internal sensor addr 0x46...");
  if (bmp580.begin(0x46, &Wire)) {
    ok = true;
    bmp580Status = "OK";
  }

  if (!ok) {
    Serial.println("Trying Internal sensor addr 0x47...");
    if (bmp580.begin(0x47, &Wire)) {
      ok = true;
      bmp580Status = "OK";
    }
  }

  if (!ok) {
    bmp580Status = "NO SENSOR";
    Serial.println("Internal sensor not found on 0x46/0x47");
    return;
  }

  bmp580.setTemperatureOversampling(BMP5XX_OVERSAMPLING_2X);
  bmp580.setPressureOversampling(BMP5XX_OVERSAMPLING_16X);
  bmp580.setIIRFilterCoeff(BMP5XX_IIR_FILTER_COEFF_3);
  bmp580.setOutputDataRate(BMP5XX_ODR_10_HZ);
  bmp580.setPowerMode(BMP5XX_POWERMODE_NORMAL);
  bmp580.enablePressure(true);

  bmp580OK = true;
  Serial.print("Internal sensor found: ");
  Serial.println(bmp580Status);
#else
  bmp580Status = "DISABLED";
#endif
}

bool readInternal() {
#if BMP580_ENABLED
  lastBmp580Ms = millis();

  if (!bmp580OK) return false;

  // Do not block reading by dataReady(); some modules/library versions
  // keep dataReady false right after boot while performReading still works.
  if (!bmp580.performReading()) {
    bmp580Status = "READ ERR";
    Serial.println("Internal sensor read error");
    return false;
  }

  bmpTempC = bmp580.temperature;

  // Different BMP5xx library versions/boards may expose pressure as Pa or hPa.
  // Auto-detect:
  //   normal pressure in Pa  ~= 101325
  //   normal pressure in hPa ~= 1013
  float rawPressure = bmp580.pressure;
  if (rawPressure > 20000.0f) {
    bmpPressureHpa = rawPressure / 100.0f;   // Pa -> hPa
  } else {
    bmpPressureHpa = rawPressure;            // already hPa
  }
  bmpPressureMmHg = pressureMmHgValue(bmpPressureHpa);

  bmpAltitudeM = bmp580.readAltitude(1013.25);

  if (bmp580Status.startsWith("OK") == false) bmp580Status = "OK";

  Serial.print("Internal T=");
  Serial.print(bmpTempC, 2);
  Serial.print("C rawP=");
  Serial.print(rawPressure, 2);
  Serial.print(" P=");
  Serial.print(bmpPressureHpa, 2);
  Serial.print("hPa ");
  Serial.print(bmpPressureMmHg, 1);
  Serial.println("mmHg");

  return true;
#else
  return false;
#endif
}

String bmp580ShortStatus() {
#if BMP580_ENABLED
  if (bmp580OK) return "Internal " + bmp580Status;
  return "Internal " + bmp580Status;
#else
  return "Internal OFF";
#endif
}

// =====================================================
// GPS HELPERS
// =====================================================
void initGPS() {
#if GPS_ENABLED
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  gpsStarted = true;
  Serial.println("GPS UART started");
#else
  gpsStarted = false;
#endif
}

void pollGPS(uint16_t maxMs) {
#if GPS_ENABLED
  if (!gpsStarted) return;

  unsigned long start = millis();
  while (GPSSerial.available() && millis() - start < maxMs) {
    gps.encode(GPSSerial.read());
  }

  gpsSatellites = gps.satellites.isValid() ? gps.satellites.value() : 0;

  if (gps.location.isValid() && gps.location.age() < 15000) {
    activeLat = gps.location.lat();
    activeLon = gps.location.lng();
    gpsFix = true;
    gpsEverFixed = true;
    lastGpsFixMs = millis();
    locationSource = "GPS";

    if (gps.altitude.isValid()) gpsAltitudeM = gps.altitude.meters();
  } else {
    gpsFix = false;
  }

  lastGpsReadMs = millis();
#else
  (void)maxMs;
#endif
}

String gpsStatusText() {
#if GPS_ENABLED
  if (!gpsStarted) return "OFF";
  if (gpsFix) return "FIX " + String(gpsSatellites) + " sat";
  if (gpsSatellites > 0) return "NOFIX " + String(gpsSatellites) + " sat";
  return "NO FIX";
#else
  return "DISABLED";
#endif
}

String activeCoordText() {
  return String(activeLat, 4) + ", " + String(activeLon, 4);
}

void applyTimeOffset(long offsetSec, const String& src, bool persist) {
  if (offsetSec < -12L * 3600L || offsetSec > 14L * 3600L) return;

  bool changed = (activeGmtOffsetSec != offsetSec || timezoneSource != src);

  activeGmtOffsetSec = offsetSec;
  timezoneSource = src;

  configTime(activeGmtOffsetSec, DAYLIGHT_OFFSET_SEC,
             "pool.ntp.org",
             "time.google.com",
             "time.nist.gov");

  if (persist) saveTimezoneSettings(offsetSec, src);

  if (changed) {
    Serial.print("Timezone offset updated: ");
    Serial.print(activeGmtOffsetSec);
    Serial.print(" source: ");
    Serial.println(timezoneSource);
  }
}

// =====================================================
// WEATHER PARSE — OPEN-METEO
// =====================================================
bool parseOpenMeteoPayload(const String& body) {
  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    Serial.print("Open-Meteo JSON error: ");
    Serial.println(err.c_str());

    weatherFailCount++;
    weatherStatus = "OM JSON";
    lastHttpError = "OM JSON";
    return false;
  }

  if (doc["utc_offset_seconds"].is<long>()) {
    long omOffset = doc["utc_offset_seconds"].as<long>();
    applyTimeOffset(omOffset, locationSource == "GPS" ? "OM GPS" : "OM DEFAULT");
  }

  JsonObject cur = doc["current"];

  if (cur.isNull()) {
    Serial.println("Open-Meteo current not found");

    weatherFailCount++;
    weatherStatus = "OM ERR";
    lastHttpError = "OM NO CURRENT";
    return false;
  }

  prevTempC = tempC;

  tempC = cur["temperature_2m"].as<float>();
  feelsC = cur["apparent_temperature"].as<float>();
  pressureHpa = cur["pressure_msl"].as<float>();
  windMs = cur["wind_speed_10m"].as<float>();

  int code = cur["weather_code"].as<int>();
  weatherText = weatherCodeToText(code);

  forecastOK = false;
  JsonObject daily = doc["daily"];
  if (!daily.isNull()) {
    JsonArray times = daily["time"].as<JsonArray>();
    JsonArray tmax = daily["temperature_2m_max"].as<JsonArray>();
    JsonArray tmin = daily["temperature_2m_min"].as<JsonArray>();
    JsonArray codes = daily["weather_code"].as<JsonArray>();

    if (times.size() >= FORECAST_DAYS && tmax.size() >= FORECAST_DAYS &&
        tmin.size() >= FORECAST_DAYS && codes.size() >= FORECAST_DAYS) {
      for (int i = 0; i < FORECAST_DAYS; i++) {
        forecastDate[i] = String((const char*)times[i]);
        forecastMax[i] = tmax[i].as<float>();
        forecastMin[i] = tmin[i].as<float>();
        forecastCode[i] = codes[i].as<int>();
      }
      forecastOK = true;
    }
  }

  weatherUpdate = getShortTimeStr();
  weatherOK = true;

  weatherFailCount = 0;
  weatherStatus = forecastOK ? "OM 3D OK" : "OM NOW";

  Serial.println("Open-Meteo OK");
  return true;
}

// =====================================================
// WEATHER PARSE — OPENWEATHER
// =====================================================
bool parseOpenWeatherPayload(const String& body) {
  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    Serial.print("OpenWeather JSON error: ");
    Serial.println(err.c_str());

    weatherFailCount++;
    weatherStatus = "OW JSON";
    lastHttpError = "OW JSON";
    return false;
  }

  int cod = doc["cod"].as<int>();

  if (cod != 200) {
    Serial.print("OpenWeather cod error: ");
    Serial.println(cod);

    weatherFailCount++;
    weatherStatus = "OW " + String(cod);
    lastHttpError = "OW COD " + String(cod);
    return false;
  }

  if (doc["timezone"].is<long>()) {
    long owOffset = doc["timezone"].as<long>();
    applyTimeOffset(owOffset, locationSource == "GPS" ? "OW GPS" : "OW DEFAULT");
  }

  String owCountry = String((const char*)(doc["sys"]["country"] | ""));
  String owName = String((const char*)(doc["name"] | ""));
  applyCountryCurrency(owCountry, owName, true);

  prevTempC = tempC;

  tempC = doc["main"]["temp"].as<float>();
  feelsC = doc["main"]["feels_like"].as<float>();
  pressureHpa = doc["main"]["pressure"].as<float>();
  windMs = doc["wind"]["speed"].as<float>();

  const char* desc = doc["weather"][0]["description"] | "---";
  weatherText = shortenWeather(String(desc));

  forecastOK = false;

  weatherUpdate = getShortTimeStr();
  weatherOK = true;

  weatherFailCount = 0;
  weatherStatus = "OW NOW";

  Serial.println("OpenWeather OK");
  return true;
}

// =====================================================
// FETCH WEATHER — NO WTTR
// =====================================================
bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Weather: WiFi not connected");

    weatherFailCount++;
    weatherStatus = "NO WIFI";
    weatherOK = false;
    lastHttpError = "NO WIFI";
    return false;
  }

  resolveHost("api.open-meteo.com");
  resolveHost("api.openweathermap.org");

  String omUrl = "http://api.open-meteo.com/v1/forecast?";
  omUrl += "latitude=" + String(activeLat, 4);
  omUrl += "&longitude=" + String(activeLon, 4);
  omUrl += "&current=temperature_2m,apparent_temperature,pressure_msl,wind_speed_10m,weather_code";
  omUrl += "&daily=weather_code,temperature_2m_max,temperature_2m_min";
  omUrl += "&forecast_days=3";
  omUrl += "&wind_speed_unit=ms";
  omUrl += "&timezone=auto";

  String query = "lat=" + String(activeLat, 4);
  query += "&lon=" + String(activeLon, 4);
  query += "&appid=" + String(OPENWEATHER_API_KEY);
  query += "&units=metric";
  query += "&lang=en";

  String owHttp  = "http://api.openweathermap.org/data/2.5/weather?" + query;
  String owHttps = "https://api.openweathermap.org/data/2.5/weather?" + query;

  Serial.println();
  Serial.println("Weather source 1: Open-Meteo HTTP");
  weatherStatus = "OM...";

  HttpReply r = httpGETRetry(omUrl, 1, 500, 5000);

  if (r.ok && r.body.length() > 0) {
    if (parseOpenMeteoPayload(r.body)) {
      weatherStatus = forecastOK ? "OM 3D OK" : "OM NOW";

      // Open-Meteo gives weather/timezone, but not country.
      // Ask OpenWeather once for sys.country/name to auto-select main currency.
      fetchLocationMeta();

      return true;
    }
  }

  Serial.println();
  Serial.println("Weather source 2: OpenWeather HTTP");
  weatherStatus = "OW H...";

  r = httpGETRetry(owHttp, 1, 500, 4500);

  if (r.ok && r.body.length() > 0) {
    if (parseOpenWeatherPayload(r.body)) {
      weatherStatus = "OpenWeather HTTP";
      return true;
    }
  }

  Serial.println();
  Serial.println("Weather source 3: OpenWeather HTTPS");
  weatherStatus = "OW S...";

  r = httpGETRetry(owHttps, 1, 500, 5000);

  if (r.ok && r.body.length() > 0) {
    if (parseOpenWeatherPayload(r.body)) {
      weatherStatus = "OpenWeather HTTPS";
      return true;
    }
  }

  Serial.println("All weather sources failed");

  weatherFailCount++;
  weatherStatus = "FAIL " + String(weatherFailCount);
  weatherOK = false;
  forecastOK = false;

  // Keep the previous valid values so the screen does not fill with placeholders.
  return false;
}

// =====================================================
// FETCH USD/RUB
// =====================================================
bool fetchUsdRub() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("FX: WiFi not connected");
    usdOK = false;
    marketStatus = "NO WIFI";
    lastHttpError = "NO WIFI";
    return false;
  }

  marketStatus = "FX...";
  HttpReply r = httpGETRetry("https://open.er-api.com/v6/latest/USD", 1, 500, 5500);

  if (!r.ok || r.body.length() == 0) {
    Serial.println("FX request failed");
    usdOK = false;
    marketStatus = "FX FAIL";
    return false;
  }

  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, r.body);

  if (err) {
    Serial.print("FX JSON error: ");
    Serial.println(err.c_str());

    usdOK = false;
    marketStatus = "FX JSON";
    lastHttpError = "FX JSON";
    return false;
  }

  float rub = doc["rates"]["RUB"].as<float>();
  float eur = doc["rates"]["EUR"].as<float>();
  float thb = doc["rates"]["THB"].as<float>();
  float vnd = doc["rates"]["VND"].as<float>();

  float mainRate = 0;
  if (mainCurrency == "USD") {
    mainRate = 1.0f;
  } else {
    mainRate = doc["rates"][mainCurrency].as<float>();
  }

  if (mainRate <= 0) {
    Serial.print("Main currency rate not found: ");
    Serial.println(mainCurrency);
    // Fallback to RUB if API does not contain selected currency.
    mainCurrency = "RUB";
    countryCode = "RU";
    mainRate = rub;
    saveLocationCurrencySettings();
  }

  if (rub <= 0) {
    Serial.println("FX RUB parse failed");
    usdOK = false;
    marketStatus = "FX PARSE";
    lastHttpError = "FX PARSE";
    return false;
  }

  prevUsdRub = usdRub;
  prevMainFxRate = mainFxRate;

  usdRub = rub;
  mainFxRate = mainRate;

  // open.er-api returns all rates against USD.
  // RUB per foreign unit = RUB_per_USD / FOREIGN_per_USD.
  if (eur > 0) eurRub = rub / eur;
  if (thb > 0) thbRub = rub / thb;
  if (vnd > 0) vnd1000Rub = rub / vnd * 1000.0;

  marketUpdate = getShortTimeStr();
  usdOK = true;
  marketStatus = "FX OK";

  Serial.print("USD/RUB OK: ");
  Serial.println(usdRub, 2);
  Serial.print("EUR/RUB OK: ");
  Serial.println(eurRub, 2);
  Serial.print("THB/RUB OK: ");
  Serial.println(thbRub, 4);
  Serial.print("1000 VND/RUB OK: ");
  Serial.println(vnd1000Rub, 4);

  return true;
}

// =====================================================
// FETCH GOLD
// =====================================================
bool fetchGold() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Gold: WiFi not connected");
    goldOK = false;
    marketStatus = "NO WIFI";
    lastHttpError = "NO WIFI";
    return false;
  }

  marketStatus = "GOLD...";
  HttpReply r = httpGETRetry("https://api.gold-api.com/price/XAU", 1, 500, 5500);

  if (!r.ok || r.body.length() == 0) {
    Serial.println("Gold request failed");
    goldOK = false;
    marketStatus = "GOLD FAIL";
    return false;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, r.body);

  if (err) {
    Serial.print("Gold JSON error: ");
    Serial.println(err.c_str());

    goldOK = false;
    marketStatus = "GOLD JSON";
    lastHttpError = "GOLD JSON";
    return false;
  }

  if (doc["price"].is<float>() || doc["price"].is<double>() || doc["price"].is<int>()) {
    goldUsdOz = doc["price"].as<float>();
  } else if (doc["ask"].is<float>() || doc["ask"].is<double>() || doc["ask"].is<int>()) {
    goldUsdOz = doc["ask"].as<float>();
  } else {
    Serial.println("Gold price field not found");
    goldOK = false;
    marketStatus = "GOLD FIELD";
    lastHttpError = "GOLD FIELD";
    return false;
  }

  if (goldUsdOz <= 0) {
    Serial.println("Gold parse failed");
    goldOK = false;
    marketStatus = "GOLD PARSE";
    lastHttpError = "GOLD PARSE";
    return false;
  }

  prevGoldRubGram = goldRubGram;
  prevGoldMainGram = goldMainGram;

  if (!isnan(usdRub) && usdRub > 0) {
    goldRubGram = goldUsdOz * usdRub / 31.1034768;
  }

  if (!isnan(mainFxRate) && mainFxRate > 0) {
    goldMainGram = goldUsdOz * mainFxRate / 31.1034768;
  }

  marketUpdate = getShortTimeStr();
  goldOK = true;
  marketStatus = "OK";

  Serial.print("Gold USD/oz OK: ");
  Serial.println(goldUsdOz, 2);

  Serial.print("Gold RUB/g: ");
  Serial.println(goldRubGram, 2);

  return true;
}



// =====================================================
// FETCH SILVER
// =====================================================
bool fetchSilver() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Silver: WiFi not connected");
    silverOK = false;
    lastHttpError = "NO WIFI";
    return false;
  }

  HttpReply r = httpGETRetry("https://api.gold-api.com/price/XAG", 1, 500, 5500);

  if (!r.ok || r.body.length() == 0) {
    Serial.println("Silver request failed");
    silverOK = false;
    return false;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, r.body);

  if (err) {
    Serial.print("Silver JSON error: ");
    Serial.println(err.c_str());
    silverOK = false;
    lastHttpError = "SILVER JSON";
    return false;
  }

  if (doc["price"].is<float>() || doc["price"].is<double>() || doc["price"].is<int>()) {
    silverUsdOz = doc["price"].as<float>();
  } else if (doc["ask"].is<float>() || doc["ask"].is<double>() || doc["ask"].is<int>()) {
    silverUsdOz = doc["ask"].as<float>();
  } else {
    Serial.println("Silver price field not found");
    silverOK = false;
    lastHttpError = "SILVER FIELD";
    return false;
  }

  if (silverUsdOz <= 0) {
    Serial.println("Silver parse failed");
    silverOK = false;
    lastHttpError = "SILVER PARSE";
    return false;
  }

  if (!isnan(usdRub) && usdRub > 0) {
    silverRubGram = silverUsdOz * usdRub / 31.1034768;
  }

  silverOK = true;

  Serial.print("Silver USD/oz OK: ");
  Serial.println(silverUsdOz, 2);
  Serial.print("Silver RUB/g: ");
  Serial.println(silverRubGram, 2);

  return true;
}

// =====================================================
// SIMPLE CSV HELPER
// =====================================================
String csvField(const String& line, int index) {
  int start = 0;
  int field = 0;

  for (int i = 0; i <= line.length(); i++) {
    if (i == line.length() || line.charAt(i) == ',') {
      if (field == index) {
        String out = line.substring(start, i);
        out.trim();
        out.replace("\"", "");
        return out;
      }
      field++;
      start = i + 1;
    }
  }

  return "";
}


String cleanCallsign(String s) {
  s.trim();
  if (s.length() == 0 || s == "null") return "---";
  if (s.length() > 8) s = s.substring(0, 8);
  return s;
}

String shortCountry(String s) {
  s.trim();
  if (s.length() == 0 || s == "null") return "---";
  if (s.length() > 12) s = s.substring(0, 12);
  return s;
}

// =====================================================
// FETCH OIL — YAHOO FINANCE CHART JSON, NO KEY
// =====================================================
bool fetchOilSymbol(const String& encodedSymbol, float &outPrice) {
  if (WiFi.status() != WL_CONNECTED) {
    oilStatus = "NO WIFI";
    oilOK = false;
    lastHttpError = "NO WIFI";
    return false;
  }

  // Stooq q/l returned 404 for CL.F on the CYD.
  // Yahoo chart endpoint is lighter here and returns JSON with meta.regularMarketPrice.
  String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + encodedSymbol;
  url += "?range=1d&interval=1d";

  HttpReply r = httpGETRetry(url, 1, 500, 6500);

  if (!r.ok || r.body.length() == 0) {
    oilStatus = "OIL HTTP";
    return false;
  }

  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, r.body);

  if (err) {
    Serial.print("Oil JSON error: ");
    Serial.println(err.c_str());
    oilStatus = "OIL JSON";
    lastHttpError = "OIL JSON";
    return false;
  }

  JsonObject meta = doc["chart"]["result"][0]["meta"];
  if (meta.isNull()) {
    oilStatus = "OIL META";
    lastHttpError = "OIL META";
    return false;
  }

  float v = meta["regularMarketPrice"].as<float>();

  if (v <= 0) {
    v = meta["previousClose"].as<float>();
  }

  if (v <= 0) {
    oilStatus = "OIL PARSE";
    lastHttpError = "OIL PARSE";
    return false;
  }

  outPrice = v;
  return true;
}

bool fetchOil() {
  oilStatus = "OIL...";

  // Yahoo symbols: BZ=F — Brent futures, CL=F — WTI futures.
  // '=' is encoded as %3D because raw '=' inside path may break on some clients.
  bool brentOK = fetchOilSymbol("BZ%3DF", oilBrentUsd);
  yield();
  delay(80);
  bool wtiOK = fetchOilSymbol("CL%3DF", oilWtiUsd);

  oilOK = brentOK || wtiOK;
  oilUpdate = getShortTimeStr();

  if (oilOK) {
    oilStatus = "OIL OK";
    lastHttpError = "OK";
  } else {
    oilStatus = "OIL FAIL";
  }

  Serial.print("Oil Brent: ");
  Serial.println(oilBrentUsd);
  Serial.print("Oil WTI: ");
  Serial.println(oilWtiUsd);

  return oilOK;
}

// =====================================================
// FETCH CALENDAR — NOAA SWPC, NO KEY
// =====================================================
String kpLevelText(float kp) {
  if (isnan(kp)) return "---";
  if (kp < 4.0) return "QUIET";
  if (kp < 5.0) return "ACTIVE";
  if (kp < 6.0) return "G1 STORM";
  if (kp < 7.0) return "G2 STORM";
  if (kp < 8.0) return "G3 STORM";
  if (kp < 9.0) return "G4 STORM";
  return "G5 STORM";
}

String auroraText(float kp) {
  if (isnan(kp)) return "---";
  if (kp < 4.0) return "LOW";
  if (kp < 5.0) return "POSSIBLE";
  if (kp < 6.0) return "GOOD NORTH";
  if (kp < 7.0) return "STRONG";
  return "VERY STRONG";
}

uint16_t kpColor(float kp) {
  if (isnan(kp)) return TFT_LIGHTGREY;
  if (kp < 4.0) return TFT_GREEN;
  if (kp < 5.0) return TFT_YELLOW;
  if (kp < 6.0) return COLOR_ORANGE;
  return TFT_RED;
}

bool parseNoaaKp(const String& body) {
  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print("NOAA Kp JSON error: ");
    Serial.println(err.c_str());
    lastHttpError = "KP JSON";
    return false;
  }

  if (!doc.is<JsonArray>() || doc.size() < 2) {
    lastHttpError = "KP ARRAY";
    return false;
  }

  spaceKpNow = NAN;
  spaceKpMax = NAN;
  spaceKpTime = "--:--";

  // First row is usually a header. Use all numeric rows and take the latest valid Kp.
  for (size_t i = 1; i < doc.size(); i++) {
    JsonArray row = doc[i].as<JsonArray>();
    if (row.isNull() || row.size() < 2) continue;

    String t = row[0].as<String>();
    float kp = row[1].as<float>();
    if (kp < 0) continue;

    spaceKpNow = kp;
    if (isnan(spaceKpMax) || kp > spaceKpMax) spaceKpMax = kp;

    if (t.length() >= 16) {
      // NOAA time is UTC like 2026-06-21 01:23:00. Display just HH:MM UTC.
      spaceKpTime = t.substring(11, 16) + "Z";
    }
  }

  return !isnan(spaceKpNow);
}

bool parseNoaaPlasma(const String& body) {
  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print("NOAA plasma JSON error: ");
    Serial.println(err.c_str());
    lastHttpError = "PLASMA JSON";
    return false;
  }

  if (!doc.is<JsonArray>() || doc.size() < 2) {
    lastHttpError = "PLASMA ARRAY";
    return false;
  }

  spaceWindKms = NAN;
  spaceDensity = NAN;
  spaceWindTime = "--:--";

  for (size_t i = 1; i < doc.size(); i++) {
    JsonArray row = doc[i].as<JsonArray>();
    if (row.isNull() || row.size() < 4) continue;

    String t = row[0].as<String>();
    float density = row[1].as<float>();
    float speed = row[2].as<float>();

    if (speed > 0) spaceWindKms = speed;
    if (density >= 0) spaceDensity = density;

    if (t.length() >= 16) {
      spaceWindTime = t.substring(11, 16) + "Z";
    }
  }

  return !isnan(spaceWindKms) || !isnan(spaceDensity);
}

bool parseNoaaMag(const String& body) {
  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print("NOAA mag JSON error: ");
    Serial.println(err.c_str());
    lastHttpError = "MAG JSON";
    return false;
  }

  if (!doc.is<JsonArray>() || doc.size() < 2) {
    lastHttpError = "MAG ARRAY";
    return false;
  }

  spaceBz = NAN;

  for (size_t i = 1; i < doc.size(); i++) {
    JsonArray row = doc[i].as<JsonArray>();
    if (row.isNull() || row.size() < 5) continue;

    // mag-2-hour.json format is usually:
    // time_tag, bx_gsm, by_gsm, bz_gsm, lon_gsm, lat_gsm, bt
    float bz = row[3].as<float>();
    spaceBz = bz;
  }

  return !isnan(spaceBz);
}

bool fetchCalendarWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    spaceStatus = "NO WIFI";
    spaceOK = false;
    lastHttpError = "NO WIFI";
    return false;
  }

  spaceStatus = "NOAA...";

  bool kpOK = false;
  bool plasmaOK = false;
  bool magOK = false;

  HttpReply r = httpGETRetry("https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json", 1, 500, 7000);
  if (r.ok && r.body.length() > 0) kpOK = parseNoaaKp(r.body);

  yield();
  delay(10);

  r = httpGETRetry("https://services.swpc.noaa.gov/products/solar-wind/plasma-2-hour.json", 1, 500, 7000);
  if (r.ok && r.body.length() > 0) plasmaOK = parseNoaaPlasma(r.body);

  yield();
  delay(10);

  r = httpGETRetry("https://services.swpc.noaa.gov/products/solar-wind/mag-2-hour.json", 1, 500, 7000);
  if (r.ok && r.body.length() > 0) magOK = parseNoaaMag(r.body);

  spaceOK = kpOK || plasmaOK || magOK;
  spaceUpdate = getShortTimeStr();

  if (spaceOK) {
    spaceStatus = "NOAA OK";
    lastHttpError = "OK";
  } else {
    spaceStatus = "NOAA FAIL";
  }

  Serial.print("Calendar Kp: ");
  Serial.println(spaceKpNow);
  Serial.print("Solar wind km/s: ");
  Serial.println(spaceWindKms);
  Serial.print("Bz nT: ");
  Serial.println(spaceBz);

  return spaceOK;
}

// =====================================================
// FETCH EARTHQUAKES — USGS GEOJSON, NO KEY
// =====================================================
bool fetchEarthquakes() {
  if (WiFi.status() != WL_CONNECTED) {
    quakesStatus = "NO WIFI";
    quakesOK = false;
    lastHttpError = "NO WIFI";
    return false;
  }

  quakesStatus = "USGS...";

  // M4.5+ earthquakes during last day. Smaller and safer than all_day feed.
  HttpReply r = httpGETRetry("https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson", 1, 500, 6500);

  if (!r.ok || r.body.length() == 0) {
    quakesStatus = "USGS HTTP";
    quakesOK = false;
    return false;
  }

  DynamicJsonDocument doc(32768);
  DeserializationError err = deserializeJson(doc, r.body);

  if (err) {
    Serial.print("USGS JSON error: ");
    Serial.println(err.c_str());
    quakesStatus = "USGS JSON";
    quakesOK = false;
    lastHttpError = "USGS JSON";
    return false;
  }

  JsonArray features = doc["features"].as<JsonArray>();

  for (int i = 0; i < QUAKE_ITEMS; i++) {
    quakeMag[i] = NAN;
    quakePlace[i] = "---";
    quakeTime[i] = "--:--";
  }

  int count = features.size();
  int n = count < QUAKE_ITEMS ? count : QUAKE_ITEMS;

  for (int i = 0; i < n; i++) {
    JsonObject props = features[i]["properties"];
    quakeMag[i] = props["mag"].as<float>();
    const char* place = props["place"] | "unknown";
    quakePlace[i] = String(place);

    if (quakePlace[i].length() > 31) {
      quakePlace[i] = quakePlace[i].substring(0, 31);
    }

    unsigned long long ms = props["time"].as<unsigned long long>();
    time_t sec = (time_t)(ms / 1000ULL) + activeGmtOffsetSec;
    struct tm *tmx = gmtime(&sec);
    if (tmx != NULL) {
      quakeTime[i] = twoDigits(tmx->tm_hour) + ":" + twoDigits(tmx->tm_min);
    }
  }

  quakesOK = true;
  quakesUpdate = getShortTimeStr();
  quakesStatus = "USGS OK";
  lastHttpError = "OK";

  Serial.print("USGS earthquakes: ");
  Serial.println(count);

  return true;
}

void fetchAllData() {
  // V2.5.2: avoid overlays and extra drawing during HTTP requests.
  // This reduces the risk of resets caused by low or fragmented RAM.
  dataBusy = true;
  busyText = "UPDATING";
  Serial.println("BUSY: UPDATE ALL");

  fetchWeather();
  yield();
  delay(10);

  fetchUsdRub();
  yield();
  delay(10);

  fetchGold();
  yield();
  delay(10);

  fetchSilver();
  yield();
  delay(10);

  fetchOil();
  yield();
  delay(10);

  lastWeatherMs = millis();
  lastMarketMs = millis();
  lastCalendarMs = millis();

  dataBusy = false;

  if (currentScreen == SCREEN_DASHBOARD) {
    drawWeatherCard();
    drawMarketCard();
    drawMiniFooter();
  } else {
    redrawCurrentScreen();
  }
}

void updateCurrentScreenData() {
  // V2.5.2: UPDATE refreshes only the data required by the current screen.
  // Forecast updates no longer load USD/RUB and gold, avoiding unnecessary HTTPS and JSON allocations.
  if (dataBusy) return;

  dataBusy = true;

  if (currentScreen == SCREEN_WEATHER) {
    Serial.println("BUSY: WEATHER ONLY");
    fetchWeather();
    readInternal();
    lastWeatherMs = millis();
    dataBusy = false;
    redrawAfterWeatherUpdate();
    return;
  }

  if (currentScreen == SCREEN_QUAKES) {
    Serial.println("BUSY: QUAKES ONLY");
    fetchEarthquakes();
    dataBusy = false;
    redrawAfterWeatherUpdate();
    return;
  }

  if (currentScreen == SCREEN_SPACE) {
    // CALENDAR is local: just redraw from current NTP date/time.
    Serial.println("CALENDAR REDRAW");
    lastCalendarMs = millis();
    dataBusy = false;
    redrawCurrentScreen();
    return;
  }

  if (currentScreen == SCREEN_MENU) {
    Serial.println("MENU REDRAW");
    dataBusy = false;
    redrawCurrentScreen();
    return;
  }

  if (currentScreen == SCREEN_GPS) {
    Serial.println("GPS REDRAW");
    dataBusy = false;
    redrawCurrentScreen();
    return;
  }

  if (currentScreen == SCREEN_MARKET) {
    Serial.println("BUSY: MARKET ONLY");
    fetchUsdRub();
    yield();
    delay(10);
    fetchGold();
    yield();
    delay(10);
    fetchSilver();
    yield();
    delay(10);
    fetchOil();
    lastMarketMs = millis();
    dataBusy = false;
    redrawAfterMarketUpdate();
    return;
  }

  if (currentScreen == SCREEN_OIL) {
    Serial.println("BUSY: FX ONLY");
    fetchUsdRub();
    lastMarketMs = millis();
    dataBusy = false;
    redrawAfterMarketUpdate();
    return;
  }

  if (currentScreen == SCREEN_GOLDCALC) {
    Serial.println("BUSY: COMMODITIES ONLY");
    fetchUsdRub();
    yield();
    delay(10);
    fetchGold();
    yield();
    delay(10);
    fetchSilver();
    yield();
    delay(10);
    fetchOil();
    lastMarketMs = millis();
    dataBusy = false;
    redrawAfterMarketUpdate();
    return;
  }

  dataBusy = false;
  fetchAllData();
}


// =====================================================
// GPS / RGB UI HELPERS
// =====================================================
String utmZoneText(double lat, double lon) {
  if (isnan(lat) || isnan(lon)) return "---";
  int zone = (int)floor((lon + 180.0) / 6.0) + 1;
  if (zone < 1) zone = 1;
  if (zone > 60) zone = 60;
  String hemi = lat >= 0 ? "N" : "S";
  return String(zone) + hemi;
}

void setRgbLedLevel(uint8_t r, uint8_t g, uint8_t b) {
#if RGB_ACTIVE_LOW
  analogWrite(RGB_R_PIN, 255 - r);
  analogWrite(RGB_G_PIN, 255 - g);
  analogWrite(RGB_B_PIN, 255 - b);
#else
  analogWrite(RGB_R_PIN, r);
  analogWrite(RGB_G_PIN, g);
  analogWrite(RGB_B_PIN, b);
#endif
}

void setRgbLed(bool r, bool g, bool b) {
  setRgbLedLevel(r ? 255 : 0, g ? 255 : 0, b ? 255 : 0);
}

void initRgbLed() {
  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);
  setRgbLedLevel(0, 0, 0);
}

void animateRgbLed() {
  if (millis() - lastRgbMs < RGB_INTERVAL) return;
  lastRgbMs = millis();

  // 0..100..0 triangle wave. Non-blocking and independent from the UI.
  rgbPhase = (rgbPhase + 2) % 200;
  uint8_t wave = (rgbPhase <= 100) ? rgbPhase : (200 - rgbPhase);
  uint8_t glow = map(wave, 0, 100, 10, 210);

  bool liveData = weatherOK && bmp580OK && (usdOK || goldOK || silverOK || oilOK);

  if (dataBusy) {
    rgbState = RGB_STATE_GPS_UPDATE;
  } else if (liveData) {
    rgbState = RGB_STATE_HEALTHY;
  } else if (gpsFix || gpsSatellites > 0) {
    rgbState = RGB_STATE_GPS_UPDATE;
  } else {
    rgbState = RGB_STATE_PROBLEM;
  }

  if (rgbState == RGB_STATE_HEALTHY) {
    // Green breathing pulse: sensor/internet data are alive.
    setRgbLedLevel(0, glow, 0);
  } else if (rgbState == RGB_STATE_GPS_UPDATE) {
    // Blue/cyan breathing pulse: GPS activity or an update is running.
    setRgbLedLevel(0, glow / 2, glow);
  } else {
    // Red/yellow alternating pulse: missing or stale data.
    bool yellowPart = ((millis() / 900UL) % 2UL) != 0;
    setRgbLedLevel(glow, yellowPart ? glow / 2 : 0, 0);
  }
}

// =====================================================
// DRAW HELPERS
// =====================================================
void drawFrame() {
  tft.drawRect(0, 0, W, H, COLOR_DARKCYAN);
  tft.drawRect(1, 1, W - 2, H - 2, COLOR_PANEL_2);
}

void drawCard(int x, int y, int w, int h, uint16_t border, uint16_t fill) {
  tft.fillRoundRect(x, y, w, h, 8, fill);
  tft.drawRoundRect(x, y, w, h, 8, border);
}

void drawStatusDot(int x, int y, bool ok) {
  // V4.2: status dots removed. Green/red panel border is the only status marker.
}


void drawLabelValue(int x, int y, const String& label, const String& value, uint16_t valueColor = TFT_WHITE) {
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(x, y);
  tft.print(label);

  tft.setTextColor(valueColor, TFT_BLACK);
  tft.setCursor(x + 88, y);
  tft.print(value);
}

void drawBusyOverlay(const String& msg) {
  // V2.4: the visual loading overlay was removed.
  // dataBusy still blocks accidental touches during HTTP requests,
  // but the display is no longer covered by a MARKET or WEATHER rectangle.
  Serial.print("BUSY: ");
  Serial.println(msg);
}


// =====================================================
// TOP BUTTONS — NO BOTTOM MENU
// =====================================================
void drawTopButton(int x, int y, int w, int h, const String& title, uint16_t border, uint16_t fill, uint16_t text) {
  tft.fillRoundRect(x, y, w, h, 7, fill);
  tft.drawRoundRect(x, y, w, h, 7, border);

  tft.setTextSize(1);
  tft.setTextColor(text, fill);

  int tx = x + (w / 2) - (title.length() * 3);
  if (tx < x + 4) tx = x + 4;
  tft.setCursor(tx, y + 11);
  tft.print(title);
}

void drawBackButton() {
  drawTopButton(BACK_BTN_X, TOP_BTN_Y, BACK_BTN_W, TOP_BTN_H, "< BACK", TFT_CYAN, COLOR_PANEL, TFT_CYAN);
}

void drawTopUpdateButton() {
  drawTopButton(updateBtnX(), TOP_BTN_Y, UPDATE_BTN_W, TOP_BTN_H, "UPDATE", TFT_GREEN, COLOR_DARKGREEN, TFT_GREEN);
}

void drawPageTitle(const String& title, uint16_t color) {
  tft.setTextSize(2);
  tft.setTextColor(color, TFT_BLACK);
  int tx = (W / 2) - (title.length() * 6);
  if (tx < 90) tx = 90;
  tft.setCursor(tx, 16);
  tft.print(title);
}

// =====================================================
// ICONS
// =====================================================
void drawSunIcon(int cx, int cy, int r) {
  tft.fillCircle(cx, cy, r, TFT_YELLOW);

  for (int i = 0; i < 8; i++) {
    float a = i * 0.785398;
    int x1 = cx + cos(a) * (r + 4);
    int y1 = cy + sin(a) * (r + 4);
    int x2 = cx + cos(a) * (r + 14);
    int y2 = cy + sin(a) * (r + 14);
    tft.drawLine(x1, y1, x2, y2, TFT_YELLOW);
  }
}

void drawCloudIcon(int x, int y, uint16_t color) {
  tft.fillCircle(x + 28, y + 28, 20, color);
  tft.fillCircle(x + 52, y + 20, 24, color);
  tft.fillCircle(x + 78, y + 30, 18, color);
  tft.fillRoundRect(x + 20, y + 30, 78, 28, 10, color);
}

void drawRainIcon(int x, int y) {
  drawCloudIcon(x, y, TFT_LIGHTGREY);

  for (int i = 0; i < 5; i++) {
    int dx = x + 28 + i * 14;
    tft.drawLine(dx, y + 66, dx - 6, y + 82, TFT_CYAN);
  }
}

void drawSnowIcon(int x, int y) {
  drawCloudIcon(x, y, TFT_LIGHTGREY);

  for (int i = 0; i < 4; i++) {
    int sx = x + 30 + i * 18;
    int sy = y + 72;

    tft.drawLine(sx - 5, sy, sx + 5, sy, TFT_WHITE);
    tft.drawLine(sx, sy - 5, sx, sy + 5, TFT_WHITE);
    tft.drawLine(sx - 4, sy - 4, sx + 4, sy + 4, TFT_WHITE);
    tft.drawLine(sx - 4, sy + 4, sx + 4, sy - 4, TFT_WHITE);
  }
}

void drawWeatherIcon(String text, int x, int y) {
  text.toLowerCase();

  if (text.indexOf("clear") >= 0) {
    drawSunIcon(x + 60, y + 48, 26);
  } else if (text.indexOf("rain") >= 0) {
    drawRainIcon(x, y);
  } else if (text.indexOf("snow") >= 0) {
    drawSnowIcon(x, y);
  } else if (text.indexOf("cloud") >= 0) {
    drawCloudIcon(x, y, TFT_LIGHTGREY);
  } else {
    drawCloudIcon(x, y, TFT_LIGHTGREY);
  }
}

void drawDollarIcon(int x, int y) {
  tft.fillCircle(x + 34, y + 34, 32, COLOR_DARKGREEN);
  tft.drawCircle(x + 34, y + 34, 32, TFT_GREEN);

  tft.setTextSize(5);
  tft.setTextColor(TFT_GREEN, COLOR_DARKGREEN);
  tft.setCursor(x + 19, y + 11);
  tft.print("$");
}

void drawGoldIcon(int x, int y) {
  tft.fillRoundRect(x + 10, y + 36, 90, 34, 8, COLOR_GOLD);
  tft.drawRoundRect(x + 10, y + 36, 90, 34, 8, TFT_YELLOW);

  tft.fillRoundRect(x + 28, y + 14, 55, 28, 7, COLOR_GOLD);
  tft.drawRoundRect(x + 28, y + 14, 55, 28, 7, TFT_YELLOW);

  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, COLOR_GOLD);
  tft.setCursor(x + 35, y + 45);
  tft.print("AU");
}


void drawMiniDollarIcon(int cx, int cy, uint16_t color) {
  tft.drawCircle(cx, cy, 13, color);
  tft.setTextSize(2);
  tft.setTextColor(color, COLOR_PANEL);
  tft.setCursor(cx - 6, cy - 9);
  tft.print("$");
}

void drawMiniGoldIcon(int x, int y) {
  tft.fillRoundRect(x, y + 13, 32, 13, 3, COLOR_GOLD);
  tft.drawRoundRect(x, y + 13, 32, 13, 3, TFT_YELLOW);
  tft.fillRoundRect(x + 8, y + 4, 22, 11, 3, COLOR_GOLD);
  tft.drawRoundRect(x + 8, y + 4, 22, 11, 3, TFT_YELLOW);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK, COLOR_GOLD);
  tft.setCursor(x + 9, y + 16);
  tft.print("AU");
}

void drawMiniSatelliteIcon(int x, int y, uint16_t color) {
  // Clear only the icon area using the panel background, then redraw it.
  // This prevents remnants when GPS status changes color.
  tft.fillRect(x, y, 46, 44, COLOR_PANEL);
  tft.drawRect(x + 14, y + 14, 16, 10, color);
  tft.drawLine(x + 22, y + 14, x + 22, y + 9, color);
  tft.drawCircle(x + 22, y + 7, 2, color);

  tft.drawRect(x + 2, y + 12, 10, 14, color);
  tft.drawRect(x + 32, y + 12, 10, 14, color);
  tft.drawLine(x + 12, y + 18, x + 14, y + 18, color);
  tft.drawLine(x + 30, y + 18, x + 32, y + 18, color);

  tft.drawLine(x + 24, y + 25, x + 32, y + 34, color);
  tft.drawCircle(x + 35, y + 37, 5, color);
}


void drawMiniEarthIcon(int cx, int cy, uint16_t color) {
  tft.drawCircle(cx, cy, 15, color);
  tft.drawLine(cx - 14, cy, cx + 14, cy, color);
  tft.drawLine(cx, cy - 14, cx, cy + 14, color);
  tft.drawCircle(cx, cy, 7, color);
}

void drawMiniWeatherIcon(int x, int y, uint16_t color) {
  tft.fillCircle(x + 16, y + 16, 10, TFT_YELLOW);
  tft.fillCircle(x + 31, y + 22, 11, color);
  tft.fillRoundRect(x + 21, y + 23, 26, 10, 5, color);
}

void drawMiniSysIcon(int x, int y, uint16_t color) {
  tft.drawRect(x + 8, y + 7, 28, 21, color);
  tft.drawLine(x + 14, y + 34, x + 30, y + 34, color);
  tft.drawLine(x + 22, y + 28, x + 22, y + 34, color);
  tft.fillCircle(x + 41, y + 10, 3, color);
  tft.fillCircle(x + 41, y + 20, 3, color);
  tft.fillCircle(x + 41, y + 30, 3, color);
}

// =====================================================
// ROTATE ICON HELPERS
// =====================================================
void drawArcPolylineTFT(int cx, int cy, int r, int startDeg, int endDeg, uint16_t color) {
  float prevX = 0, prevY = 0;
  bool first = true;
  for (int a = startDeg; a <= endDeg; a += 10) {
    float rad = a * 0.0174532925f;
    float x = cx + cos(rad) * r;
    float y = cy + sin(rad) * r;
    if (!first) tft.drawLine((int)prevX, (int)prevY, (int)x, (int)y, color);
    prevX = x;
    prevY = y;
    first = false;
  }
}

void drawArcPolylineSprite(int cx, int cy, int r, int startDeg, int endDeg, uint16_t color) {
  float prevX = 0, prevY = 0;
  bool first = true;
  for (int a = startDeg; a <= endDeg; a += 10) {
    float rad = a * 0.0174532925f;
    float x = cx + cos(rad) * r;
    float y = cy + sin(rad) * r;
    if (!first) spr.drawLine((int)prevX, (int)prevY, (int)x, (int)y, color);
    prevX = x;
    prevY = y;
    first = false;
  }
}

void drawArrowHeadTFT(int x, int y, int dir, uint16_t color) {
  // dir: 0=left-up arrow, 1=right-down arrow
  if (dir == 0) {
    tft.drawLine(x, y, x + 5, y + 1, color);
    tft.drawLine(x, y, x + 2, y + 5, color);
  } else {
    tft.drawLine(x, y, x - 5, y - 1, color);
    tft.drawLine(x, y, x - 2, y - 5, color);
  }
}

void drawArrowHeadSprite(int x, int y, int dir, uint16_t color) {
  if (dir == 0) {
    spr.drawLine(x, y, x + 5, y + 1, color);
    spr.drawLine(x, y, x + 2, y + 5, color);
  } else {
    spr.drawLine(x, y, x - 5, y - 1, color);
    spr.drawLine(x, y, x - 2, y - 5, color);
  }
}

void drawRotateIconTFT(int x, int y, int w, int h, uint16_t color) {
  int cx = x + w / 2;
  int cy = y + h / 2 + 1;
  int r = min(w, h) / 2 - 5;
  if (r < 5) r = 5;

  drawArcPolylineTFT(cx, cy, r, 40, 165, color);
  drawArcPolylineTFT(cx, cy, r, 220, 345, color);

  float rad1 = 165 * 0.0174532925f;
  int ax1 = cx + cos(rad1) * r;
  int ay1 = cy + sin(rad1) * r;
  drawArrowHeadTFT(ax1, ay1, 0, color);

  float rad2 = 345 * 0.0174532925f;
  int ax2 = cx + cos(rad2) * r;
  int ay2 = cy + sin(rad2) * r;
  drawArrowHeadTFT(ax2, ay2, 1, color);
}

void drawRotateIconSprite(int x, int y, int w, int h, uint16_t color) {
  int cx = x + w / 2;
  int cy = y + h / 2 + 1;
  int r = min(w, h) / 2 - 5;
  if (r < 5) r = 5;

  drawArcPolylineSprite(cx, cy, r, 40, 165, color);
  drawArcPolylineSprite(cx, cy, r, 220, 345, color);

  float rad1 = 165 * 0.0174532925f;
  int ax1 = cx + cos(rad1) * r;
  int ay1 = cy + sin(rad1) * r;
  drawArrowHeadSprite(ax1, ay1, 0, color);

  float rad2 = 345 * 0.0174532925f;
  int ax2 = cx + cos(rad2) * r;
  int ay2 = cy + sin(rad2) * r;
  drawArrowHeadSprite(ax2, ay2, 1, color);
}

// =====================================================
// HEADER
// =====================================================
void drawHeaderDirect() {
  tft.fillRect(3, 3, W - 6, 45, TFT_BLACK);

  tft.setTextSize(3);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(8, 7);
  tft.print(getTimeStr());

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(8, 35);
  tft.print(getDateStr());

  tft.setTextColor(gpsFix ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
  tft.setCursor(132, 35);
  tft.print(locationSource);

  tft.fillRoundRect(ROT_BTN_X, ROT_BTN_Y, ROT_BTN_W, ROT_BTN_H, 5, COLOR_PANEL);
  tft.drawRoundRect(ROT_BTN_X, ROT_BTN_Y, ROT_BTN_W, ROT_BTN_H, 5, TFT_ORANGE);
  drawRotateIconTFT(ROT_BTN_X + 4, ROT_BTN_Y + 1, ROT_BTN_W - 8, ROT_BTN_H - 2, TFT_ORANGE);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(232, 10);
  tft.print("WiFi");

  drawStatusDot(304, 13, WiFi.status() == WL_CONNECTED || configMode);

  tft.setCursor(232, 27);

  if (configMode) {
    tft.setTextColor(COLOR_ORANGE, TFT_BLACK);
  } else if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
  }

  tft.print(wifiStatusText());
  tft.drawLine(6, 49, W - 7, 49, COLOR_DARKCYAN);
}

void drawHeader() {
  // Draw the 314x47 header in a sprite and push it as one block.
  // This prevents visible clearing of the top area every second.
  const int sx = 3;
  const int sy = 3;
  const int sw = 314;
  const int sh = 47;

  if (!beginSprite(sw, sh, TFT_BLACK)) {
    drawHeaderDirect();
    return;
  }

  spr.setTextSize(3);
  spr.setTextColor(TFT_YELLOW, TFT_BLACK);
  spr.setCursor(5, 4);
  spr.print(getTimeStr());

  spr.setTextSize(1);
  spr.setTextColor(TFT_CYAN, TFT_BLACK);
  spr.setCursor(5, 32);
  spr.print(getDateStr());

  spr.setTextColor(gpsFix ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
  spr.setCursor(129, 32);
  spr.print(locationSource);

  spr.fillRoundRect(ROT_BTN_X - sx, ROT_BTN_Y - sy, ROT_BTN_W, ROT_BTN_H, 5, COLOR_PANEL);
  spr.drawRoundRect(ROT_BTN_X - sx, ROT_BTN_Y - sy, ROT_BTN_W, ROT_BTN_H, 5, TFT_ORANGE);
  drawRotateIconSprite(ROT_BTN_X - sx + 4, ROT_BTN_Y - sy + 1, ROT_BTN_W - 8, ROT_BTN_H - 2, TFT_ORANGE);

  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.setCursor(229, 7);
  spr.print("WiFi");

  // V4.1: no status dot; WiFi status is shown by text color.

  spr.setCursor(229, 24);

  if (configMode) {
    spr.setTextColor(COLOR_ORANGE, TFT_BLACK);
  } else if (WiFi.status() == WL_CONNECTED) {
    spr.setTextColor(TFT_GREEN, TFT_BLACK);
  } else {
    spr.setTextColor(TFT_RED, TFT_BLACK);
  }

  spr.print(wifiStatusText());
  spr.drawLine(3, 46, W - 10, 46, COLOR_DARKCYAN);

  endSprite(sx, sy);
}

// =====================================================
// DASHBOARD
// =====================================================
void drawWeatherCardDirect() {
  const int x = 8, y = 56, w = 304, h = 146;
  tft.fillRoundRect(x, y, w, h, 10, COLOR_PANEL);
  tft.drawRoundRect(x, y, w, h, 10, COLOR_DARKCYAN);

  int halfW = (w - 18) / 2;
  int leftX = x + 7;
  int rightX = x + 11 + halfW;
  int cardY = y + 8;
  int cardH = h - 16;

  // External internet weather — left half
  drawCard(leftX, cardY, halfW, cardH, weatherOK ? TFT_GREEN : TFT_RED, COLOR_PANEL_2);
  tft.setTextSize(2);
  tft.setTextColor(weatherTitleColor(tempC), COLOR_PANEL_2);
  tft.setCursor(leftX + 10, cardY + 10);
  tft.print("External");
  drawStatusDot(leftX + halfW - 12, cardY + 15, weatherOK);

  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, COLOR_PANEL_2);
  tft.setCursor(leftX + 10, cardY + 38);
  tft.print(floatText(tempC, 1));
  tft.print("C");

  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, COLOR_PANEL_2);
  tft.setCursor(leftX + 12, cardY + 75);
  tft.print(weatherText);

  tft.setTextColor(pressureColorMmHg(pressureHpa), COLOR_PANEL_2);
  tft.setCursor(leftX + 12, cardY + 94);
  tft.print("P: ");
  tft.print(pressureMmHgText(pressureHpa, 0));
  tft.print(" mm");

  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL_2);
  tft.setCursor(leftX + 12, cardY + 112);
  tft.print("Wind: ");
  tft.print(floatText(windMs, 1));
  tft.print(" m/s");

  // Internal BMP580 — right half
  drawCard(rightX, cardY, halfW, cardH, bmp580OK ? TFT_GREEN : TFT_RED, COLOR_PANEL_2);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, COLOR_PANEL_2);
  tft.setCursor(rightX + 10, cardY + 10);
  tft.print("Internal");
  drawStatusDot(rightX + halfW - 12, cardY + 15, bmp580OK);

  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, COLOR_PANEL_2);
  tft.setCursor(rightX + 10, cardY + 38);
  tft.print(floatText(bmpTempC, 1));
  tft.print("C");

  tft.setTextSize(1);
  tft.setTextColor(pressureColorMmHg(bmpPressureHpa), COLOR_PANEL_2);
  tft.setCursor(rightX + 12, cardY + 76);
  tft.print("P: ");
  tft.print(pressureMmHgText(bmpPressureHpa, 0));
  tft.print(" mm");

  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL_2);
  tft.setCursor(rightX + 12, cardY + 95);
  tft.print("Alt: ");
  tft.print(floatText(bmpAltitudeM, 0));
  tft.print(" m");

  tft.setTextColor(bmp580OK ? TFT_GREEN : TFT_ORANGE, COLOR_PANEL_2);
  tft.setCursor(rightX + 12, cardY + 113);
  tft.print(bmp580OK ? String("OK") : bmp580Status);
}

void drawWeatherCard() {
  drawWeatherCardDirect();
}

void drawHomeMenuButton() {
  const int x = 8, y = 209, w = 304, h = 25;
  tft.fillRoundRect(x, y, w, h, 8, COLOR_PANEL);
  tft.drawRoundRect(x, y, w, h, 8, TFT_ORANGE);
  tft.setTextSize(2);
  tft.setTextColor(TFT_ORANGE, COLOR_PANEL);
  tft.setCursor(x + 126, y + 6);
  tft.print("MENU");
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(x + 12, y + 9);
  tft.print("open panels");
  tft.setCursor(x + 222, y + 9);
  tft.print("FX/GPS/SYS");
}

void drawMarketCardDirect() {
  drawHomeMenuButton();
}

void drawMarketCard() {
  drawHomeMenuButton();
}

void drawMiniFooterDirect() {
  // Footer hidden on HOME V4: MENU button occupies bottom area.
}

void drawMiniFooter() {
  // Footer hidden on HOME V4: MENU button occupies bottom area.
}

void drawDashboard() {
  currentScreen = SCREEN_DASHBOARD;

  W = tft.width();
  H = tft.height();

  tft.fillScreen(TFT_BLACK);

  drawFrame();
  drawHeader();
  drawWeatherCard();
  drawHomeMenuButton();
}


// =====================================================
// ICON MENU
// =====================================================
void drawMenuButtonCell(int x, int y, int w, int h, const String& title, uint16_t border, bool ok, int iconType) {
  (void)border;  // icon accent is separate; status is shown only by the frame
  uint16_t b = ok ? TFT_GREEN : TFT_RED;
  tft.fillRoundRect(x, y, w, h, 10, COLOR_PANEL);
  tft.drawRoundRect(x, y, w, h, 10, b);

  int ix = x + 10;
  int iy = y + 8;
  if (iconType == 0) drawMiniWeatherIcon(ix, iy, TFT_LIGHTGREY);
  else if (iconType == 1) drawMiniEarthIcon(ix + 20, iy + 20, TFT_CYAN);
  else if (iconType == 2) drawMiniDollarIcon(ix + 20, iy + 20, TFT_GREEN);
  else if (iconType == 3) drawMiniGoldIcon(ix, iy + 2);
  else if (iconType == 4) drawMiniSatelliteIcon(ix, iy, gpsFix ? TFT_GREEN : TFT_ORANGE);
  else if (iconType == 5) drawMiniSysIcon(ix, iy, TFT_CYAN);

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);

  // Shift text left so "Weather" and "Commod" stay inside the button.
  int tx = x + 58;
  int ty = y + 18;
  tft.setCursor(tx, ty);
  tft.print(title);
}


void drawMenuScreen() {
  currentScreen = SCREEN_MENU;
  tft.fillScreen(TFT_BLACK);
  drawFrame();
  drawHeader();

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(12, 52);
  tft.print("Tap panel. Tap header clock area to HOME.");

  const int x1 = 8;
  const int x2 = 162;
  const int y1 = 66;
  const int bw = 150;
  const int bh = 50;
  const int gap = 8;

  drawMenuButtonCell(x1, y1, bw, bh, "Weather", TFT_BLUE, weatherOK && bmp580OK, 0);
  drawMenuButtonCell(x2, y1, bw, bh, "Quake", TFT_CYAN, quakesOK, 1);
  drawMenuButtonCell(x1, y1 + bh + gap, bw, bh, "FX", TFT_GREEN, usdOK, 2);
  drawMenuButtonCell(x2, y1 + bh + gap, bw, bh, "Commod", COLOR_GOLD, goldOK || silverOK || oilOK, 3);
  drawMenuButtonCell(x1, y1 + (bh + gap) * 2, bw, bh, "GPS", TFT_ORANGE, gpsFix, 4);
  drawMenuButtonCell(x2, y1 + (bh + gap) * 2, bw, bh, "SYS", TFT_MAGENTA, WiFi.status() == WL_CONNECTED || configMode, 5);
}

// =====================================================
// BIG WEATHER — CURRENT
// =====================================================
void drawWeatherScreen() {
  currentScreen = SCREEN_WEATHER;

  tft.fillScreen(TFT_BLACK);
  drawFrame();
  drawBackButton();
  drawTopUpdateButton();
  drawPageTitle("WEATHER", weatherTitleColor(tempC));

  drawStatusDot(300, 42, weatherOK && bmp580OK);

  drawCard(12, 54, 296, 76, weatherOK ? TFT_GREEN : TFT_RED, COLOR_PANEL);
  drawMiniWeatherIcon(20, 72, TFT_LIGHTGREY);
  tft.setTextSize(2);
  tft.setTextColor(weatherTitleColor(tempC), COLOR_PANEL);
  tft.setCursor(86, 63);
  tft.print("External");
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(86, 89);
  tft.print(floatText(tempC, 1));
  tft.print("C");
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, COLOR_PANEL);
  tft.setCursor(210, 67);
  tft.print(weatherText);
  tft.setTextColor(pressureColorMmHg(pressureHpa), COLOR_PANEL);
  tft.setCursor(210, 87);
  tft.print(pressureMmHgText(pressureHpa, 0));
  tft.print(" mm");
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(210, 107);
  tft.print("Wind ");
  tft.print(floatText(windMs, 1));
  tft.print(" m/s");

  drawCard(12, 138, 296, 78, bmp580OK ? TFT_GREEN : TFT_RED, COLOR_PANEL);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, COLOR_PANEL);
  tft.setCursor(24, 150);
  tft.print("Internal");
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(24, 176);
  tft.print(floatText(bmpTempC, 1));
  tft.print("C");
  tft.setTextSize(1);
  tft.setTextColor(pressureColorMmHg(bmpPressureHpa), COLOR_PANEL);
  tft.setCursor(166, 154);
  tft.print("Pressure: ");
  tft.print(pressureMmHgText(bmpPressureHpa, 0));
  tft.print(" mm");
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(166, 176);
  tft.print("Altitude: ");
  tft.print(floatText(bmpAltitudeM, 0));
  tft.print(" m");
  tft.setTextColor(bmp580OK ? TFT_GREEN : TFT_ORANGE, COLOR_PANEL);
  tft.setCursor(166, 198);
  tft.print("Status: ");
  tft.print(bmp580OK ? String("OK") : bmp580Status);

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(18, 224);
  tft.print("Update: EXT+INT   Back: MENU");
}

// =====================================================
// EARTHQUAKES SCREEN
// =====================================================
void drawQuakeScreen() {
  currentScreen = SCREEN_QUAKES;

  tft.fillScreen(TFT_BLACK);
  drawFrame();
  drawBackButton();
  drawTopUpdateButton();
  drawPageTitle("QUAKES", TFT_ORANGE);

  drawStatusDot(300, 42, quakesOK);

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(96, 38);
  tft.print(quakesStatus);

  drawCard(10, 52, 300, 168, TFT_ORANGE, COLOR_PANEL);

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, COLOR_PANEL);
  tft.setCursor(20, 62);
  tft.print("USGS M4.5+ last 24h");

  if (!quakesOK) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED, COLOR_PANEL);
    tft.setCursor(24, 96);
    tft.print("NO DATA");

    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
    tft.setCursor(24, 126);
    tft.print("Tap UPDATE to load quakes.");
    return;
  }

  bool any = false;
  int y = 84;

  for (int i = 0; i < QUAKE_ITEMS; i++) {
    if (isnan(quakeMag[i])) continue;
    any = true;

    uint16_t magColor = quakeMag[i] >= 6.0 ? TFT_RED : (quakeMag[i] >= 5.0 ? TFT_ORANGE : TFT_YELLOW);

    tft.setTextSize(2);
    tft.setTextColor(magColor, COLOR_PANEL);
    tft.setCursor(20, y);
    tft.print("M");
    tft.print(floatText(quakeMag[i], 1));

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, COLOR_PANEL);
    tft.setCursor(88, y + 2);
    tft.print(quakePlace[i]);

    tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
    tft.setCursor(88, y + 15);
    tft.print("Magadan time: ");
    tft.print(quakeTime[i]);

    y += 34;
  }

  if (!any) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN, COLOR_PANEL);
    tft.setCursor(24, 100);
    tft.print("CALM");

    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
    tft.setCursor(24, 130);
    tft.print("No M4.5+ quakes in feed.");
  }

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(18, 224);
  tft.print("Updated: ");
  tft.print(quakesUpdate);
}


// =====================================================
// CALENDAR HELPERS
// =====================================================
bool isLeapYear(int year) {
  if (year % 400 == 0) return true;
  if (year % 100 == 0) return false;
  return (year % 4 == 0);
}

int daysInMonth(int year, int month0) {
  static const int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month0 == 1) return isLeapYear(year) ? 29 : 28;
  return days[month0];
}

String monthNameEn(int month0) {
  static const char* names[] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
  };
  if (month0 < 0 || month0 > 11) return "---";
  return String(names[month0]);
}

String weekdayNameEn(int wday) {
  static const char* names[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  if (wday < 0 || wday > 6) return "---";
  return String(names[wday]);
}

// =====================================================
// CALENDAR SCREEN
// =====================================================
void drawCalendarScreen() {
  currentScreen = SCREEN_SPACE;

  tft.fillScreen(TFT_BLACK);
  drawFrame();
  drawBackButton();
  drawTopUpdateButton();
  drawPageTitle("CALENDAR", TFT_CYAN);

  struct tm timeinfo;
  bool timeOK = getLocalTime(&timeinfo, 300);

  drawStatusDot(300, 42, timeOK);

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(96, 38);
  tft.print(timeOK ? "LOCAL NTP" : "NO TIME");

  drawCard(10, 52, 300, 168, TFT_CYAN, COLOR_PANEL);

  if (!timeOK) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED, COLOR_PANEL);
    tft.setCursor(24, 92);
    tft.print("NO NTP TIME");

    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
    tft.setCursor(24, 124);
    tft.print("Connect WiFi and tap UPDATE.");

    tft.setCursor(24, 146);
    tft.print("Calendar uses local time.");
    return;
  }

  int year = timeinfo.tm_year + 1900;
  int month0 = timeinfo.tm_mon;
  int today = timeinfo.tm_mday;
  int wday = timeinfo.tm_wday;

  struct tm firstDay = timeinfo;
  firstDay.tm_mday = 1;
  firstDay.tm_hour = 12;
  firstDay.tm_min = 0;
  firstDay.tm_sec = 0;
  mktime(&firstDay);

  // Monday-first calendar: MON TUE WED THU FRI SAT SUN
  int firstCol = (firstDay.tm_wday + 6) % 7;
  int dim = daysInMonth(year, month0);

  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, COLOR_PANEL);
  tft.setCursor(22, 62);
  tft.print(monthNameEn(month0));
  tft.print(" ");
  tft.print(year);

  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN, COLOR_PANEL);
  tft.setCursor(210, 66);
  tft.print(weekdayNameEn(wday));
  tft.print(" ");
  tft.print(twoDigits(today));

  const char* wd[] = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
  int gridX = 21;
  int gridY = 92;
  int cellW = 40;
  int cellH = 18;

  tft.setTextSize(1);
  for (int c = 0; c < 7; c++) {
    uint16_t col = (c >= 5) ? COLOR_ORANGE : TFT_CYAN;
    tft.setTextColor(col, COLOR_PANEL);
    tft.setCursor(gridX + c * cellW + 8, gridY);
    tft.print(wd[c]);
  }

  int day = 1;
  for (int row = 0; row < 6; row++) {
    for (int col = 0; col < 7; col++) {
      int idx = row * 7 + col;
      if (idx < firstCol || day > dim) continue;

      int x = gridX + col * cellW;
      int y = gridY + 16 + row * cellH;

      bool isToday = (day == today);
      bool weekend = (col >= 5);

      if (isToday) {
        tft.fillRoundRect(x + 2, y - 3, 30, 16, 4, TFT_YELLOW);
        tft.setTextColor(TFT_BLACK, TFT_YELLOW);
      } else {
        tft.setTextColor(weekend ? COLOR_ORANGE : TFT_WHITE, COLOR_PANEL);
      }

      if (day < 10) {
        tft.setCursor(x + 12, y);
      } else {
        tft.setCursor(x + 8, y);
      }
      tft.print(day);

      day++;
    }
  }

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(22, 204);
  tft.print("Time: ");
  tft.print(getTimeStr());
  tft.print("   ");
  tft.print(timezoneSource);
  tft.print(" UTC");
  int tzH = activeGmtOffsetSec / 3600;
  if (tzH >= 0) tft.print("+");
  tft.print(tzH);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(18, 224);
  tft.print("HOME clock -> CALENDAR   UPDATE redraws");
}

// =====================================================
// BIG MARKET
// =====================================================
void drawMarketScreen() {
  currentScreen = SCREEN_MARKET;

  tft.fillScreen(TFT_BLACK);
  drawFrame();
  drawBackButton();
  drawTopUpdateButton();
  drawPageTitle("MARKET", COLOR_GOLD);

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(96, 38);
  tft.print(marketStatus);

  drawStatusDot(300, 42, usdOK && goldOK);

  drawCard(12, 52, 296, 76, TFT_MAGENTA, COLOR_PANEL);
  drawDollarIcon(20, 58);

  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, COLOR_PANEL);
  tft.setCursor(102, 62);
  tft.print("USD/RUB");

  tft.setTextSize(4);
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(102, 86);
  tft.print(floatText(usdRub, 2));

  tft.setTextSize(1);
  tft.setTextColor(trendColor(usdRub, prevUsdRub), COLOR_PANEL);
  tft.setCursor(252, 100);
  tft.print(trendText(usdRub, prevUsdRub));

  drawCard(12, 136, 296, 86, COLOR_GOLD, COLOR_PANEL);
  drawGoldIcon(18, 136);

  tft.setTextSize(2);
  tft.setTextColor(COLOR_GOLD, COLOR_PANEL);
  tft.setCursor(122, 146);
  tft.print("GOLD RUB/g");

  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(122, 170);
  tft.print(floatText(goldRubGram, 0));

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, COLOR_PANEL);
  tft.setCursor(122, 202);
  tft.print("XAU/USD: ");
  tft.print(floatText(goldUsdOz, 2));

  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(230, 202);
  tft.print("Upd:");
  tft.print(marketUpdate);

  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setCursor(18, 224);
  tft.print("Tap USD: FX BOARD   GOLD: COMMOD");
}

// =====================================================
// OIL SCREEN
// =====================================================
void drawOilScreen() {
  currentScreen = SCREEN_OIL;

  tft.fillScreen(TFT_BLACK);
  drawFrame();
  drawBackButton();
  drawTopUpdateButton();
  drawPageTitle("FX BOARD", TFT_CYAN);

  drawStatusDot(300, 42, usdOK);

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(96, 38);
  tft.print("RUB cross rates");

  drawCard(12, 52, 296, 36, TFT_YELLOW, COLOR_PANEL);
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, COLOR_PANEL);
  tft.setCursor(24, 62);
  tft.print("1 USD");
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(132, 62);
  tft.print(floatText(usdRub, 2));
  tft.print(" RUB");

  drawCard(12, 94, 296, 36, TFT_GREEN, COLOR_PANEL);
  tft.setTextColor(TFT_GREEN, COLOR_PANEL);
  tft.setCursor(24, 104);
  tft.print("1 EUR");
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(132, 104);
  tft.print(floatText(eurRub, 2));
  tft.print(" RUB");

  drawCard(12, 136, 296, 36, TFT_CYAN, COLOR_PANEL);
  tft.setTextColor(TFT_CYAN, COLOR_PANEL);
  tft.setCursor(24, 146);
  tft.print("1 THB");
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(132, 146);
  tft.print(floatText(thbRub, 3));
  tft.print(" RUB");

  drawCard(12, 178, 296, 36, TFT_MAGENTA, COLOR_PANEL);
  tft.setTextColor(TFT_MAGENTA, COLOR_PANEL);
  tft.setCursor(24, 188);
  tft.print("1000 VND");
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(132, 188);
  tft.print(floatText(vnd1000Rub, 2));
  tft.print(" RUB");

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(18, 224);
  tft.print("Source: open.er-api  Upd: ");
  tft.print(marketUpdate);
}

// =====================================================
// GOLD CALC SCREEN
// =====================================================
void drawGoldCalcScreen() {
  currentScreen = SCREEN_GOLDCALC;

  tft.fillScreen(TFT_BLACK);
  drawFrame();
  drawBackButton();
  drawTopUpdateButton();
  drawPageTitle("COMMOD", COLOR_GOLD);

  drawStatusDot(300, 42, (goldOK || silverOK || oilOK));

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(96, 38);
  tft.print("metals + oil");

  drawCard(12, 52, 296, 38, COLOR_GOLD, COLOR_PANEL);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_GOLD, COLOR_PANEL);
  tft.setCursor(24, 63);
  tft.print("GOLD");
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(112, 63);
  tft.print(floatText(goldRubGram, 0));
  tft.print(" RUB/g");

  drawCard(12, 96, 296, 38, TFT_LIGHTGREY, COLOR_PANEL);
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(24, 107);
  tft.print("SILVER");
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(112, 107);
  tft.print(floatText(silverRubGram, 1));
  tft.print(" RUB/g");

  drawCard(12, 140, 296, 38, TFT_ORANGE, COLOR_PANEL);
  tft.setTextColor(TFT_ORANGE, COLOR_PANEL);
  tft.setCursor(24, 151);
  tft.print("BRENT");
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(112, 151);
  tft.print(floatText(oilBrentUsd, 2));
  tft.print(" USD/bbl");

  drawCard(12, 184, 296, 38, TFT_CYAN, COLOR_PANEL);
  tft.setTextColor(TFT_CYAN, COLOR_PANEL);
  tft.setCursor(24, 195);
  tft.print("WTI");
  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(112, 195);
  tft.print(floatText(oilWtiUsd, 2));
  tft.print(" USD/bbl");

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(18, 224);
  tft.print("Upd M:");
  tft.print(marketUpdate);
  tft.print(" O:");
  tft.print(oilUpdate);
}


// =====================================================
// GPS SCREEN
// =====================================================
void drawGpsScreen() {
  currentScreen = SCREEN_GPS;

  tft.fillScreen(TFT_BLACK);
  drawFrame();
  drawBackButton();
  drawTopUpdateButton();
  drawPageTitle("GPS", gpsFix ? TFT_GREEN : TFT_ORANGE);

  drawCard(12, 54, 296, 54, gpsFix ? TFT_GREEN : TFT_RED, COLOR_PANEL);
  drawMiniSatelliteIcon(22, 61, gpsFix ? TFT_GREEN : TFT_ORANGE);

  tft.setTextSize(2);
  tft.setTextColor(gpsFix ? TFT_GREEN : TFT_ORANGE, COLOR_PANEL);
  tft.setCursor(82, 63);
  tft.print(gpsFix ? "GPS FIX" : "GPS WAIT");

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  String gpsSourceShort = locationSource;
  if (gpsSourceShort.length() > 13) gpsSourceShort = gpsSourceShort.substring(0, 13);

  tft.setCursor(82, 88);
  tft.print("Sat: ");
  tft.print(gpsSatellites);
  tft.print("  Src: ");
  tft.print(gpsSourceShort);

  drawCard(12, 116, 296, 92, TFT_CYAN, COLOR_PANEL);

  tft.setTextSize(1);

  // Left column — coordinates, shifted left.
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(22, 128); tft.print("Lat");
  tft.setCursor(22, 146); tft.print("Lon");
  tft.setCursor(22, 164); tft.print("Zone");
  tft.setCursor(22, 182); tft.print("Alt");

  tft.setTextColor(TFT_WHITE, COLOR_PANEL);
  tft.setCursor(58, 128); tft.print(String(activeLat, 6));
  tft.setCursor(58, 146); tft.print(String(activeLon, 6));
  tft.setTextColor(TFT_YELLOW, COLOR_PANEL);
  tft.setCursor(58, 164); tft.print(utmZoneText(activeLat, activeLon));
  tft.setTextColor(TFT_CYAN, COLOR_PANEL);
  tft.setCursor(58, 182);
  tft.print(isnan(gpsAltitudeM) ? String("---") : String(gpsAltitudeM, 1) + "m");

  // Right column — location metadata. Short labels keep text inside frame.
  String tzShort = "UTC";
  int tzH = activeGmtOffsetSec / 3600;
  if (tzH >= 0) tzShort += "+";
  tzShort += String(tzH);

  String cityShort = locationName;
  if (cityShort.length() > 12) cityShort = cityShort.substring(0, 12);

  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(174, 128); tft.print("Country");
  tft.setCursor(174, 146); tft.print("City");
  tft.setCursor(174, 164); tft.print("Money");
  tft.setCursor(174, 182); tft.print("TZ");

  tft.setTextColor(TFT_GREEN, COLOR_PANEL);
  tft.setCursor(232, 128); tft.print(countryCode);
  tft.setTextColor(TFT_CYAN, COLOR_PANEL);
  tft.setCursor(232, 146); tft.print(cityShort);
  tft.setTextColor(COLOR_GOLD, COLOR_PANEL);
  tft.setCursor(232, 164); tft.print(mainCurrency);
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(232, 182); tft.print(tzShort);

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(18, 224);
  tft.print("GPS TX->GPIO35   Back: MENU");
}


// =====================================================
// SYSTEM SCREEN
// =====================================================
void drawSystemScreen() {
  currentScreen = SCREEN_SYSTEM;

  tft.fillScreen(TFT_BLACK);
  drawFrame();
  drawBackButton();
  drawTopUpdateButton();
  drawPageTitle("SYSTEM", TFT_CYAN);

  drawStatusDot(300, 42, WiFi.status() == WL_CONNECTED || configMode);

  int y = 52;
  const int step = 14;

  drawLabelValue(14, y, "WiFi", wifiStatusText(), (WiFi.status() == WL_CONNECTED || configMode) ? TFT_GREEN : TFT_RED);
  y += step;
  drawLabelValue(14, y, "SSID", configMode ? String(CONFIG_AP_SSID) : wifiSsid, TFT_CYAN);
  y += step;
  drawLabelValue(14, y, "IP", wifiIpText(), TFT_CYAN);
  y += step;

  String rssi = "---";
  if (WiFi.status() == WL_CONNECTED) rssi = String(WiFi.RSSI()) + " dBm";
  drawLabelValue(14, y, "RSSI", rssi, TFT_WHITE);
  y += step;

  drawLabelValue(14, y, "Heap", String(ESP.getFreeHeap() / 1024) + " KB", TFT_WHITE);
  y += step;
  drawLabelValue(14, y, "Uptime", uptimeStr(), TFT_WHITE);
  y += step;
  drawLabelValue(14, y, "GPS", gpsStatusText(), gpsFix ? TFT_GREEN : TFT_ORANGE);
  y += step;
  drawLabelValue(14, y, "Loc", locationSource + " " + activeCoordText(), gpsFix ? TFT_GREEN : TFT_CYAN);
  y += step;
  drawLabelValue(14, y, "TZ", timezoneSource + " " + String(activeGmtOffsetSec / 3600) + "h", TFT_CYAN);
  y += step;
  drawLabelValue(14, y, "Weather", weatherStatus, weatherOK ? TFT_GREEN : TFT_RED);
  y += step;
  drawLabelValue(14, y, "Market", marketStatus, (usdOK && goldOK) ? TFT_GREEN : TFT_RED);
  y += step;
  drawLabelValue(14, y, "HTTP", String(lastHttpCode) + " " + lastHttpHost, lastHttpError == "OK" ? TFT_GREEN : TFT_ORANGE);
  y += step;
  drawLabelValue(14, y, "Error", lastHttpError, lastHttpError == "OK" ? TFT_GREEN : TFT_RED);

  // The bottom Web/JSON line was removed.
  // The IP address is already shown above, and the long text overlapped system rows.
  tft.fillRect(12, 216, W - 24, 18, TFT_BLACK);
}

// =====================================================
// TOUCH HANDLER
// =====================================================
void goScreen(int m) {
  if (m == SCREEN_DASHBOARD) drawDashboard();
  else if (m == SCREEN_MENU) drawMenuScreen();
  else if (m == SCREEN_WEATHER) drawWeatherScreen();
  else if (m == SCREEN_QUAKES) drawQuakeScreen();
  else if (m == SCREEN_SPACE) drawCalendarScreen();
  else if (m == SCREEN_MARKET) drawMarketScreen();
  else if (m == SCREEN_OIL) drawOilScreen();
  else if (m == SCREEN_GOLDCALC) drawGoldCalcScreen();
  else if (m == SCREEN_GPS) drawGpsScreen();
  else if (m == SCREEN_SYSTEM) drawSystemScreen();
  else if (m == SCREEN_WIFI_HELP) drawWiFiHelpScreen();
}

bool touchBack(int tx, int ty) {
  // Use a larger BACK touch target because the small XPT2046 resistive panel
  // can be difficult to hit precisely, especially after display rotation.
  // The visual button remains unchanged while its active touch area is wider.
  return inBox(tx, ty, 0, 0, 112, 52);
}

bool touchUpdate(int tx, int ty) {
  return inBox(tx, ty, updateBtnX(), TOP_BTN_Y, UPDATE_BTN_W, TOP_BTN_H);
}

void handleTouch() {
  if (dataBusy) return;
  if (millis() - lastTouchMs < TOUCH_DEBOUNCE) return;

  int tx, ty;
  if (!getTouchXY(tx, ty)) return;
  lastTouchMs = millis();

  if (currentScreen == SCREEN_WIFI_HELP) {
    drawWiFiHelpScreen();
    return;
  }

  // ROT button is always active on HOME and MENU because header stays unchanged.
  if ((currentScreen == SCREEN_DASHBOARD || currentScreen == SCREEN_MENU) && inBox(tx, ty, ROT_BTN_X, ROT_BTN_Y, ROT_BTN_W, ROT_BTN_H)) {
    toggleDisplayRotation();
    return;
  }

  if (currentScreen == SCREEN_DASHBOARD) {
    // Clock/header still opens calendar.
    if (inBox(tx, ty, 3, 3, 178, 48)) {
      drawCalendarScreen();
      return;
    }

    if (inBox(tx, ty, 8, 56, 304, 146)) {
      drawWeatherScreen();
      return;
    }

    if (inBox(tx, ty, 8, 205, 304, 35)) {
      drawMenuScreen();
      return;
    }
    return;
  }

  if (currentScreen == SCREEN_MENU) {
    // Header clock area returns HOME on MENU.
    if (inBox(tx, ty, 3, 3, 178, 48)) {
      drawDashboard();
      return;
    }

    const int x1 = 8;
    const int x2 = 162;
    const int y1 = 66;
    const int bw = 150;
    const int bh = 50;
    const int gap = 8;

    if (inBox(tx, ty, x1, y1, bw, bh)) { drawWeatherScreen(); return; }
    if (inBox(tx, ty, x2, y1, bw, bh)) {
      if (!quakesOK) { dataBusy = true; fetchEarthquakes(); dataBusy = false; }
      drawQuakeScreen(); return;
    }
    if (inBox(tx, ty, x1, y1 + bh + gap, bw, bh)) {
      if (!usdOK) { dataBusy = true; fetchUsdRub(); dataBusy = false; }
      drawOilScreen(); return;
    }
    if (inBox(tx, ty, x2, y1 + bh + gap, bw, bh)) {
      if (!silverOK || isnan(oilBrentUsd) || isnan(oilWtiUsd)) {
        dataBusy = true; fetchSilver(); yield(); delay(10); fetchOil(); dataBusy = false;
      }
      drawGoldCalcScreen(); return;
    }
    if (inBox(tx, ty, x1, y1 + (bh + gap) * 2, bw, bh)) { drawGpsScreen(); return; }
    if (inBox(tx, ty, x2, y1 + (bh + gap) * 2, bw, bh)) { drawSystemScreen(); return; }
    return;
  }

  // Inner screens: BACK returns to MENU. Calendar returns to HOME.
  if (touchBack(tx, ty)) {
    if (currentScreen == SCREEN_SPACE) drawDashboard();
    else drawMenuScreen();
    return;
  }

  if (touchUpdate(tx, ty)) {
    updateCurrentScreenData();
    return;
  }
}

// =====================================================
// ANIMATION
// =====================================================
void drawAnimation() {
  // V4: old bottom screen animation is disabled because bottom area is now the MENU button.
  // Built-in RGB LED animation is handled by animateRgbLed().
}


// =====================================================
// WIFI HELP SCREEN
// =====================================================
void drawWiFiHelpScreen() {
  currentScreen = SCREEN_WIFI_HELP;

  tft.fillScreen(TFT_BLACK);
  drawFrame();

  tft.setTextSize(2);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setCursor(18, 16);
  tft.print("WiFi SETUP MODE");

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(18, 42);
  tft.print("Main WiFi not connected");

  drawCard(10, 60, 300, 52, TFT_CYAN, COLOR_PANEL);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(20, 70);
  tft.print("1. Connect phone/PC to network:");
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, COLOR_PANEL);
  tft.setCursor(20, 88);
  tft.print(CONFIG_AP_SSID);

  drawCard(10, 120, 300, 42, TFT_YELLOW, COLOR_PANEL);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(20, 130);
  tft.print("2. Password:");
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, COLOR_PANEL);
  tft.setCursor(125, 134);
  tft.print(CONFIG_AP_PASS);

  drawCard(10, 170, 300, 48, TFT_GREEN, COLOR_PANEL);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, COLOR_PANEL);
  tft.setCursor(20, 180);
  tft.print("3. Open browser:");
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, COLOR_PANEL);
  tft.setCursor(20, 198);
  tft.print("192.168.4.1");

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(12, 226);
  tft.print("Save WiFi settings -> device will reboot");
}

// =====================================================
// STARTUP SCREEN
// =====================================================
void drawStartupScreen(const String& line1, const String& line2 = "") {
  tft.fillScreen(TFT_BLACK);
  drawFrame();

  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(18, 26);
  tft.print("Electrum TERMINAL");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 62);
  tft.print("V4.1 Menu UI");

  tft.setCursor(20, 80);
  tft.print("Weather + Internal + Market");

  tft.setCursor(20, 115);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.print(line1);

  if (line2.length() > 0) {
    tft.setCursor(20, 135);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.print(line2);
  }

  if (configMode) {
    tft.setTextColor(COLOR_ORANGE, TFT_BLACK);
    tft.setCursor(20, 164);
    tft.print("1. Connect to WiFi:");

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(38, 182);
    tft.print(CONFIG_AP_SSID);

    tft.setTextColor(COLOR_ORANGE, TFT_BLACK);
    tft.setCursor(20, 200);
    tft.print("2. Password: ");
    tft.print(CONFIG_AP_PASS);

    tft.setCursor(20, 218);
    tft.print("3. Open: 192.168.4.1");
  }
}


// =====================================================
// PARTIAL REDRAW HELPERS
// =====================================================
void redrawAfterWeatherUpdate() {
  if (currentScreen == SCREEN_DASHBOARD) {
    drawWeatherCard();
    drawHomeMenuButton();
  } else if (currentScreen == SCREEN_MENU) {
    drawMenuScreen();
  } else if (currentScreen == SCREEN_WEATHER) {
    drawWeatherScreen();
  } else if (currentScreen == SCREEN_QUAKES) {
    drawQuakeScreen();
  } else if (currentScreen == SCREEN_SPACE) {
    drawCalendarScreen();
  } else if (currentScreen == SCREEN_GPS) {
    drawGpsScreen();
  } else if (currentScreen == SCREEN_SYSTEM) {
    drawSystemScreen();
  } else if (currentScreen == SCREEN_WIFI_HELP) {
    drawWiFiHelpScreen();
  }
}

void redrawAfterMarketUpdate() {
  if (currentScreen == SCREEN_DASHBOARD) {
    drawHomeMenuButton();
  } else if (currentScreen == SCREEN_MENU) {
    drawMenuScreen();
  } else if (currentScreen == SCREEN_MARKET) {
    drawMarketScreen();
  } else if (currentScreen == SCREEN_OIL) {
    drawOilScreen();
  } else if (currentScreen == SCREEN_GOLDCALC) {
    drawGoldCalcScreen();
  } else if (currentScreen == SCREEN_GPS) {
    drawGpsScreen();
  } else if (currentScreen == SCREEN_SYSTEM) {
    drawSystemScreen();
  }
}

// =====================================================
// REDRAW CURRENT SCREEN
// =====================================================
void redrawCurrentScreen() {
  if (currentScreen == SCREEN_DASHBOARD) {
    drawDashboard();
  } else if (currentScreen == SCREEN_MENU) {
    drawMenuScreen();
  } else if (currentScreen == SCREEN_WEATHER) {
    drawWeatherScreen();
  } else if (currentScreen == SCREEN_QUAKES) {
    drawQuakeScreen();
  } else if (currentScreen == SCREEN_SPACE) {
    drawCalendarScreen();
  } else if (currentScreen == SCREEN_MARKET) {
    drawMarketScreen();
  } else if (currentScreen == SCREEN_OIL) {
    drawOilScreen();
  } else if (currentScreen == SCREEN_GOLDCALC) {
    drawGoldCalcScreen();
  } else if (currentScreen == SCREEN_GPS) {
    drawGpsScreen();
  } else if (currentScreen == SCREEN_SYSTEM) {
    drawSystemScreen();
  } else if (currentScreen == SCREEN_WIFI_HELP) {
    drawWiFiHelpScreen();
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("======================================");
  Serial.println("Electrum TERMINAL V4.1 UI CLEANUP START");
  Serial.println("======================================");

  pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
  digitalWrite(TFT_BACKLIGHT_PIN, HIGH);
  initRgbLed();

  tft.init();
  loadUiSettings();
  loadTimezoneSettings();
  loadLocationCurrencySettings();
  tft.setRotation(displayRotation);
  tft.invertDisplay(true);

  W = tft.width();
  H = tft.height();

  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  ts.begin(touchSPI);
  ts.setRotation(TOUCH_BASE_ROTATION);

  Serial.println("Touch XPT2046 started");

  initGPS();
  initInternal();

  loadWiFiSettings();

  drawStartupScreen("Connecting WiFi...", wifiSsid);

  bool connected = connectWiFi();

  WiFi.setSleep(false);

  if (!connected) {
    startConfigAP();
    startWebServer();

    bootTimeStr = getShortTimeStr();
    drawWiFiHelpScreen();

    return;
  }

  startWebServer();

  drawStartupScreen("WiFi connected", WiFi.localIP().toString());

  configTime(activeGmtOffsetSec, DAYLIGHT_OFFSET_SEC,
             "pool.ntp.org",
             "time.google.com",
             "time.nist.gov");

  delay(1200);
  bootTimeStr = getShortTimeStr();

  // V2.9.1 FAST START:
  // Do not load the slower silver and oil services during startup.
  // Yahoo or Gold API may respond slowly on some networks and delay the market screen.
  // Show the interface first and fetch additional data only when needed.
  drawStartupScreen("Loading basic data...");

  // GPS is optional. Short initial poll only; if there is no module/fix,
  // active location remains Magadan.
  for (int i = 0; i < 80; i++) {
    pollGPS(10);
    delay(10);
    yield();
  }

  fetchUsdRub();
  yield();
  delay(30);

  fetchGold();
  yield();
  delay(30);

  readInternal();

  drawDashboard();

  delay(200);

  fetchWeather();

  lastWeatherMs = millis();
  lastMarketMs = millis();

  drawDashboard();
  lastFullRedrawMs = millis();
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  server.handleClient();

  pollGPS(4);

  if (millis() - lastBmp580Ms > Internal_INTERVAL) {
    readInternal();
  }

  reconnectWiFiIfNeeded();

  handleTouch();
  animateRgbLed();

  if (millis() - lastClockMs > 1000) {
    lastClockMs = millis();

    if (currentScreen == SCREEN_DASHBOARD) {
      drawHeader();
    } else if (currentScreen == SCREEN_MENU) {
      drawHeader();
    } else if (currentScreen == SCREEN_SYSTEM) {
      // Avoid redrawing SYSTEM every second to prevent flicker; use periodic full redraws instead.
    }
  }

  if (millis() - lastAnimMs > 90) {
    lastAnimMs = millis();
    drawAnimation();
  }

  if (!configMode && WiFi.status() == WL_CONNECTED && !dataBusy) {
    // If GPS got a fix, weather will be requested by GPS coordinates on next update.
    if (gpsFix && gpsEverFixed && millis() - lastGpsFixMs < 30000 && millis() - lastWeatherMs > 60000) {
      lastWeatherMs = millis();
      dataBusy = true;
      Serial.println("BUSY: GPS LOCATION WEATHER UPDATE");
      fetchWeather();
      dataBusy = false;
      redrawAfterWeatherUpdate();
    }

    if (millis() - lastWeatherMs > WEATHER_INTERVAL) {
      lastWeatherMs = millis();

      dataBusy = true;
      Serial.println("BUSY: AUTO WEATHER");
      fetchWeather();
      dataBusy = false;
      redrawAfterWeatherUpdate();
    }

    if (millis() - lastMarketMs > MARKET_INTERVAL) {
      lastMarketMs = millis();

      dataBusy = true;
      Serial.println("BUSY: AUTO MARKET BASIC ONLY");
      // Automatic updates load only the lightweight market data used on HOME.
      // Silver and oil are loaded when entering COMMOD or pressing UPDATE there.
      fetchUsdRub();
      yield();
      delay(10);
      fetchGold();
      dataBusy = false;
      redrawAfterMarketUpdate();
    }

    if (currentScreen == SCREEN_SPACE && millis() - lastCalendarMs > SPACE_INTERVAL) {
      lastCalendarMs = millis();
      drawCalendarScreen();
    }
  }

  if (!dataBusy && millis() - lastFullRedrawMs > FULL_REDRAW_INTERVAL) {
    lastFullRedrawMs = millis();

    // Perform an occasional full redraw; normal updates use partial redraws.
    redrawCurrentScreen();
  }

  delay(20);
}
