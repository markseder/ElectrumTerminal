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
// WIFI
// =====================================================
const char* DEFAULT_WIFI_SSID = "";
const char* DEFAULT_WIFI_PASS = "";

const char* CONFIG_AP_SSID = "ElectrumTerminal";
const char* CONFIG_AP_PASS = "CHANGE_ME";

// =====================================================
// OPENWEATHERMAP API
// =====================================================
const char* OPENWEATHER_API_KEY = "YOUR_OPENWEATHER_API_KEY";

// =====================================================
// LOCATION — DEFAULT MAGADAN + OPTIONAL GPS
// =====================================================
const float DEFAULT_LAT = 59.56;
const float DEFAULT_LON = 150.80;
const long DEFAULT_GMT_OFFSET_SEC = 11 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// GPS module is optional.
// If GPS is not connected or there is no fix, terminal uses Magadan by default.
// Wiring:
//   GPS TX -> ESP32 GPIO35
//   GPS RX is not used
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
// На ESP32-2432S028R полный экран 320x240 в 16 бит = около 150 KB.
// С Wi-Fi и JSON это может быть тяжело, поэтому используем безопасную
// частичную двойную буферизацию: header, карточки, footer, overlay.
// Визуально это убирает основное мигание, потому что на экран уходит
// уже готовый блок, а не процесс рисования каждой строки.
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

// Если тач зеркалит — меняй местами MIN/MAX или включай TOUCH_SWAP_X/Y ниже
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

