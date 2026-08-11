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
