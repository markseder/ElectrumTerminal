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