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

  // Нижнюю строку Web/JSON убрали полностью:
  // IP уже показан выше, а длинный текст накладывался на системные строки.
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
  // Увеличенная зона BACK: на маленьком резистивном таче XPT2046
  // попадать точно в 76x30 иногда неудобно, особенно после поворота экрана.
  // Визуальная кнопка остается прежней, но зона нажатия шире.
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
