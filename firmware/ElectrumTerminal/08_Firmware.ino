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
