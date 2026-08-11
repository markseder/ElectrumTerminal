void fetchAllData() {
  // V2.5.2: без всплывающей плашки и без лишней графики во время HTTP.
  // Так меньше шанс словить ребут из-за нехватки RAM/фрагментации памяти.
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
  // V2.5.2: кнопка UPDATE обновляет только то, что реально нужно текущему экрану.
  // На FORECAST больше не гоняем USD/RUB и GOLD, поэтому не забиваем память лишними HTTPS+JSON.
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
  // V2.4: визуальную плашку загрузки убрали полностью.
  // dataBusy всё ещё блокирует случайные тачи во время HTTP-запросов,
  // но экран больше не закрывается прямоугольником MARKET... / WEATHER...
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
  // Header рисуем в спрайт 314x47 и отправляем одним куском.
  // Это убирает видимое стирание верхней зоны каждую секунду.
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
