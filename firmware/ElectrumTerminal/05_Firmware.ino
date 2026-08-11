    lastHttpError = "SILVER FIELD";
    return false;
  }

  if (silverUsdOz <= 0) {
    Serial.println("Silver parse failed");
    silverOK = false;
    lastHttpError = "SILVER PARSE";
    return false;
  }

  if (!isnan(usdRub) && usdRub > 0) {
    silverRubGram = silverUsdOz * usdRub / 31.1034768;
  }

  silverOK = true;

  Serial.print("Silver USD/oz OK: ");
  Serial.println(silverUsdOz, 2);
  Serial.print("Silver RUB/g: ");
  Serial.println(silverRubGram, 2);

  return true;
}

// =====================================================
// SIMPLE CSV HELPER
// =====================================================
String csvField(const String& line, int index) {
  int start = 0;
  int field = 0;

  for (int i = 0; i <= line.length(); i++) {
    if (i == line.length() || line.charAt(i) == ',') {
      if (field == index) {
        String out = line.substring(start, i);
        out.trim();
        out.replace("\"", "");
        return out;
      }
      field++;
      start = i + 1;
    }
  }

  return "";
}


String cleanCallsign(String s) {
  s.trim();
  if (s.length() == 0 || s == "null") return "---";
  if (s.length() > 8) s = s.substring(0, 8);
  return s;
}

String shortCountry(String s) {
  s.trim();
  if (s.length() == 0 || s == "null") return "---";
  if (s.length() > 12) s = s.substring(0, 12);
  return s;
}

// =====================================================
// FETCH OIL — YAHOO FINANCE CHART JSON, NO KEY
// =====================================================
bool fetchOilSymbol(const String& encodedSymbol, float &outPrice) {
  if (WiFi.status() != WL_CONNECTED) {
    oilStatus = "NO WIFI";
    oilOK = false;
    lastHttpError = "NO WIFI";
    return false;
  }

  // Stooq q/l returned 404 for CL.F on the CYD.
  // Yahoo chart endpoint is lighter here and returns JSON with meta.regularMarketPrice.
  String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + encodedSymbol;
  url += "?range=1d&interval=1d";

  HttpReply r = httpGETRetry(url, 1, 500, 6500);

  if (!r.ok || r.body.length() == 0) {
    oilStatus = "OIL HTTP";
    return false;
  }

  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, r.body);

  if (err) {
    Serial.print("Oil JSON error: ");
    Serial.println(err.c_str());
    oilStatus = "OIL JSON";
    lastHttpError = "OIL JSON";
    return false;
  }

  JsonObject meta = doc["chart"]["result"][0]["meta"];
  if (meta.isNull()) {
    oilStatus = "OIL META";
    lastHttpError = "OIL META";
    return false;
  }

  float v = meta["regularMarketPrice"].as<float>();

  if (v <= 0) {
    v = meta["previousClose"].as<float>();
  }

  if (v <= 0) {
    oilStatus = "OIL PARSE";
    lastHttpError = "OIL PARSE";
    return false;
  }

  outPrice = v;
  return true;
}

bool fetchOil() {
  oilStatus = "OIL...";

  // Yahoo symbols: BZ=F — Brent futures, CL=F — WTI futures.
  // '=' is encoded as %3D because raw '=' inside path may break on some clients.
  bool brentOK = fetchOilSymbol("BZ%3DF", oilBrentUsd);
  yield();
  delay(80);
  bool wtiOK = fetchOilSymbol("CL%3DF", oilWtiUsd);

  oilOK = brentOK || wtiOK;
  oilUpdate = getShortTimeStr();

  if (oilOK) {
    oilStatus = "OIL OK";
    lastHttpError = "OK";
  } else {
    oilStatus = "OIL FAIL";
  }

  Serial.print("Oil Brent: ");
  Serial.println(oilBrentUsd);
  Serial.print("Oil WTI: ");
  Serial.println(oilWtiUsd);

  return oilOK;
}

// =====================================================
// FETCH CALENDAR — NOAA SWPC, NO KEY
// =====================================================
String kpLevelText(float kp) {
  if (isnan(kp)) return "---";
  if (kp < 4.0) return "QUIET";
  if (kp < 5.0) return "ACTIVE";
  if (kp < 6.0) return "G1 STORM";
  if (kp < 7.0) return "G2 STORM";
  if (kp < 8.0) return "G3 STORM";
  if (kp < 9.0) return "G4 STORM";
  return "G5 STORM";
}

String auroraText(float kp) {
  if (isnan(kp)) return "---";
  if (kp < 4.0) return "LOW";
  if (kp < 5.0) return "POSSIBLE";
  if (kp < 6.0) return "GOOD NORTH";
  if (kp < 7.0) return "STRONG";
  return "VERY STRONG";
}

uint16_t kpColor(float kp) {
  if (isnan(kp)) return TFT_LIGHTGREY;
  if (kp < 4.0) return TFT_GREEN;
  if (kp < 5.0) return TFT_YELLOW;
  if (kp < 6.0) return COLOR_ORANGE;
  return TFT_RED;
}

bool parseNoaaKp(const String& body) {
  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print("NOAA Kp JSON error: ");
    Serial.println(err.c_str());
    lastHttpError = "KP JSON";
    return false;
  }

  if (!doc.is<JsonArray>() || doc.size() < 2) {
    lastHttpError = "KP ARRAY";
    return false;
  }

  spaceKpNow = NAN;
  spaceKpMax = NAN;
  spaceKpTime = "--:--";

  // First row is usually a header. Use all numeric rows and take the latest valid Kp.
  for (size_t i = 1; i < doc.size(); i++) {
    JsonArray row = doc[i].as<JsonArray>();
    if (row.isNull() || row.size() < 2) continue;

    String t = row[0].as<String>();
    float kp = row[1].as<float>();
    if (kp < 0) continue;

    spaceKpNow = kp;
    if (isnan(spaceKpMax) || kp > spaceKpMax) spaceKpMax = kp;

    if (t.length() >= 16) {
      // NOAA time is UTC like 2026-06-21 01:23:00. Display just HH:MM UTC.
      spaceKpTime = t.substring(11, 16) + "Z";
    }
  }

  return !isnan(spaceKpNow);
}

bool parseNoaaPlasma(const String& body) {
  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print("NOAA plasma JSON error: ");
    Serial.println(err.c_str());
    lastHttpError = "PLASMA JSON";
    return false;
  }

  if (!doc.is<JsonArray>() || doc.size() < 2) {
    lastHttpError = "PLASMA ARRAY";
    return false;
  }

  spaceWindKms = NAN;
  spaceDensity = NAN;
  spaceWindTime = "--:--";

  for (size_t i = 1; i < doc.size(); i++) {
    JsonArray row = doc[i].as<JsonArray>();
    if (row.isNull() || row.size() < 4) continue;

    String t = row[0].as<String>();
    float density = row[1].as<float>();
    float speed = row[2].as<float>();

    if (speed > 0) spaceWindKms = speed;
    if (density >= 0) spaceDensity = density;

    if (t.length() >= 16) {
      spaceWindTime = t.substring(11, 16) + "Z";
    }
  }

  return !isnan(spaceWindKms) || !isnan(spaceDensity);
}

bool parseNoaaMag(const String& body) {
  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print("NOAA mag JSON error: ");
    Serial.println(err.c_str());
    lastHttpError = "MAG JSON";
    return false;
  }

  if (!doc.is<JsonArray>() || doc.size() < 2) {
    lastHttpError = "MAG ARRAY";
    return false;
  }

  spaceBz = NAN;

  for (size_t i = 1; i < doc.size(); i++) {
    JsonArray row = doc[i].as<JsonArray>();
    if (row.isNull() || row.size() < 5) continue;

    // mag-2-hour.json format is usually:
    // time_tag, bx_gsm, by_gsm, bz_gsm, lon_gsm, lat_gsm, bt
    float bz = row[3].as<float>();
    spaceBz = bz;
  }

  return !isnan(spaceBz);
}

bool fetchCalendarWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    spaceStatus = "NO WIFI";
    spaceOK = false;
    lastHttpError = "NO WIFI";
    return false;
  }

  spaceStatus = "NOAA...";

  bool kpOK = false;
  bool plasmaOK = false;
  bool magOK = false;

  HttpReply r = httpGETRetry("https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json", 1, 500, 7000);
  if (r.ok && r.body.length() > 0) kpOK = parseNoaaKp(r.body);

  yield();
  delay(10);

  r = httpGETRetry("https://services.swpc.noaa.gov/products/solar-wind/plasma-2-hour.json", 1, 500, 7000);
  if (r.ok && r.body.length() > 0) plasmaOK = parseNoaaPlasma(r.body);

  yield();
  delay(10);

  r = httpGETRetry("https://services.swpc.noaa.gov/products/solar-wind/mag-2-hour.json", 1, 500, 7000);
  if (r.ok && r.body.length() > 0) magOK = parseNoaaMag(r.body);

  spaceOK = kpOK || plasmaOK || magOK;
  spaceUpdate = getShortTimeStr();

  if (spaceOK) {
    spaceStatus = "NOAA OK";
    lastHttpError = "OK";
  } else {
    spaceStatus = "NOAA FAIL";
  }

  Serial.print("Calendar Kp: ");
  Serial.println(spaceKpNow);
  Serial.print("Solar wind km/s: ");
  Serial.println(spaceWindKms);
  Serial.print("Bz nT: ");
  Serial.println(spaceBz);

  return spaceOK;
}

// =====================================================
// FETCH EARTHQUAKES — USGS GEOJSON, NO KEY
// =====================================================
bool fetchEarthquakes() {
  if (WiFi.status() != WL_CONNECTED) {
    quakesStatus = "NO WIFI";
    quakesOK = false;
    lastHttpError = "NO WIFI";
    return false;
  }

  quakesStatus = "USGS...";

  // M4.5+ earthquakes during last day. Smaller and safer than all_day feed.
  HttpReply r = httpGETRetry("https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson", 1, 500, 6500);

  if (!r.ok || r.body.length() == 0) {
    quakesStatus = "USGS HTTP";
    quakesOK = false;
    return false;
  }

  DynamicJsonDocument doc(32768);
  DeserializationError err = deserializeJson(doc, r.body);

  if (err) {
    Serial.print("USGS JSON error: ");
    Serial.println(err.c_str());
    quakesStatus = "USGS JSON";
    quakesOK = false;
    lastHttpError = "USGS JSON";
    return false;
  }

  JsonArray features = doc["features"].as<JsonArray>();

  for (int i = 0; i < QUAKE_ITEMS; i++) {
    quakeMag[i] = NAN;
    quakePlace[i] = "---";
    quakeTime[i] = "--:--";
  }

  int count = features.size();
  int n = count < QUAKE_ITEMS ? count : QUAKE_ITEMS;

  for (int i = 0; i < n; i++) {
    JsonObject props = features[i]["properties"];
    quakeMag[i] = props["mag"].as<float>();
    const char* place = props["place"] | "unknown";
    quakePlace[i] = String(place);

    if (quakePlace[i].length() > 31) {
      quakePlace[i] = quakePlace[i].substring(0, 31);
    }

    unsigned long long ms = props["time"].as<unsigned long long>();
    time_t sec = (time_t)(ms / 1000ULL) + activeGmtOffsetSec;
    struct tm *tmx = gmtime(&sec);
    if (tmx != NULL) {
      quakeTime[i] = twoDigits(tmx->tm_hour) + ":" + twoDigits(tmx->tm_min);
    }
  }

  quakesOK = true;
