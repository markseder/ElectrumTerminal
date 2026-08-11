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

  if (mmhg < 750.0f) return TFT_CYAN;   // низкое давление
  if (mmhg <= 760.0f) return TFT_GREEN; // нормальный диапазон
  return TFT_RED;                       // высокое давление
}

uint16_t weatherTitleColor(float temp) {
  if (isnan(temp)) return TFT_CYAN;

  if (temp < 0.0f) return TFT_CYAN;     // мороз
  if (temp <= 13.0f) return TFT_GREEN;  // прохладно/нормально
  return TFT_RED;                       // тепло/жарко
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

  // Если экран развёрнут на 180 градусов, тач тоже разворачиваем.
  // ts.setRotation() при этом НЕ меняем, иначе будет двойная коррекция координат.
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

  // ВАЖНО: XPT2046 оставляем в базовой ориентации.
  // Экран вращаем через TFT_eSPI, а координаты тача разворачиваем вручную в getTouchXY().
  // Если одновременно делать ts.setRotation(3) и вручную инвертировать X/Y, тач получается двойно перевернутым.
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
