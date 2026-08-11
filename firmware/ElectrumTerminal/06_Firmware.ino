  quakesUpdate = getShortTimeStr();
  quakesStatus = "USGS OK";
  lastHttpError = "OK";

  Serial.print("USGS earthquakes: ");
  Serial.println(count);

  return true;
}

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
