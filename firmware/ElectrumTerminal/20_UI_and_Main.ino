
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
  drawLabelValue(