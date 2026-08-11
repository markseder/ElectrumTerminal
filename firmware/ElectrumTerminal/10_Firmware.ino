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
  Serial.println("Electrum TERMINAL V4.2 UI STATUS LED START");
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
  // Не грузим на старте тяжёлые API серебра/нефти.
  // На некоторых сетях Yahoo/gold-api может долго отвечать, и экран висит на Loading market.
  // Сначала показываем интерфейс, а данные подтягиваются выборочно.
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
      // Чтобы SYSTEM не мигал каждую секунду, обновляем только периодически полной перерисовкой.
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
      // Автообновление — только лёгкий основной рынок для HOME.
      // Серебро и нефть грузим только при входе на COMMOD или по UPDATE на COMMOD.
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

    // Редкий санитарный полный redraw. Основная работа теперь частичная.
    redrawCurrentScreen();
  }

  delay(20);
}
