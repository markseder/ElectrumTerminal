

// =====================================================
// COUNTRY / CURRENCY HELPERS
// =====================================================
String currencyByCountry(String cc) {
  cc.toUpperCase();

  if (cc == "RU") return "RUB";
  if (cc == "JP") return "JPY";
  if (cc == "TH") return "THB";
  if (cc == "VN") return "VND";
  if (cc == "PH") return "PHP";
  if (cc == "AU") return "AUD";
  if (cc == "US") return "USD";
  if (cc == "CN") return "CNY";
  if (cc == "KR") return "KRW";
  if (cc == "GB") return "GBP";
  if (cc == "CA") return "CAD";
  if (cc == "NZ") return "NZD";
  if (cc == "SG") return "SGD";
  if (cc == "MY") return "MYR";
  if (cc == "ID") return "IDR";
  if (cc == "IN") return "INR";
  if (cc == "TR") return "TRY";
  if (cc == "AE") return "AED";
  if (cc == "KZ") return "KZT";

  // Eurozone countries.
  if (cc == "DE" || cc == "FR" || cc == "IT" || cc == "ES" ||
      cc == "NL" || cc == "FI" || cc == "EE" || cc == "LV" ||
      cc == "LT" || cc == "PT" || cc == "AT" || cc == "BE" ||
      cc == "IE" || cc == "GR" || cc == "SK" || cc == "SI" ||
      cc == "HR" || cc == "CY" || cc == "LU" || cc == "MT") {
    return "EUR";
  }

  return "USD";
}

void loadLocationCurrencySettings() {
  prefs.begin("loc", true);
  countryCode = prefs.getString("country", "RU");
  mainCurrency = prefs.getString("currency", "RUB");
  locationName = prefs.getString("name", "Magadan");
  prefs.end();

  countryCode.toUpperCase();
  mainCurrency.toUpperCase();

  if (countryCode.length() != 2) countryCode = "RU";
  if (mainCurrency.length() < 3) mainCurrency = currencyByCountry(countryCode);
  if (locationName.length() == 0) locationName = "Magadan";

  locationMetaStatus = "SAVED";
}

void saveLocationCurrencySettings() {
  prefs.begin("loc", false);
  prefs.putString("country", countryCode);
  prefs.putString("currency", mainCurrency);
  prefs.putString("name", locationName);
  prefs.end();
}

void applyCountryCurrency(const String& ccRaw, const String& nameRaw, bool persist) {
  String cc = ccRaw;
  cc.trim();
  cc.toUpperCase();

  if (cc.length() != 2) return;

  String cur = currencyByCountry(cc);
  String name = nameRaw;
  name.trim();

  countryCode = cc;
  mainCurrency = cur;
  if (name.length() > 0) locationName = name;

  locationMetaOK = true;
  locationMetaStatus = "OW " + countryCode + "/" + mainCurrency;

  if (persist) saveLocationCurrencySettings();

  Serial.print("Location currency: ");
  Serial.print(countryCode);
  Serial.print(" ");
  Serial.print(mainCurrency);
  Serial.print(" ");
  Serial.println(locationName);
}

bool parseOpenWeatherMetaPayload(const String& body) {
  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    Serial.print("OpenWeather meta JSON error: ");
    Serial.println(err.c_str());
    locationMetaStatus = "META JSON";
    return false;
  }

  int cod = doc["cod"].as<int>();
  if (cod != 200) {
    locationMetaStatus = "META " + String(cod);
    return false;
  }

  if (doc["timezone"].is<long>()) {
    long owOffset = doc["timezone"].as<long>();
    applyTimeOffset(owOffset, locationSource == "GPS" ? "OW GPS" : "OW DEFAULT");
  }

  String owCountry = String((const char*)(doc["sys"]["country"] | ""));
  String owName = String((const char*)(doc["name"] | ""));
  applyCountryCurrency(owCountry, owName, true);

  String cc = String((const char*)(doc["sys"]["country"] | ""));
  String name = String((const char*)(doc["name"] | ""));
  applyCountryCurrency(cc, name, true);

  return locationMetaOK;
}

bool fetchLocationMeta() {
  if (WiFi.status() != WL_CONNECTED) {
    locationMetaStatus = "NO WIFI";
    return false;
  }

  String query = "lat=" + String(activeLat, 4);
  query += "&lon=" + String(activeLon, 4);
  query += "&appid=" + String(OPENWEATHER_API_KEY);
  query += "&units=metric";
  query += "&lang=en";

  String url = "http://api.openweathermap.org/data/2.5/weather?" + query;

  Serial.println();
  Serial.println("Location meta: OpenWeather country/timezone");
  HttpReply r = httpGETRetry(url, 1, 500, 4500);

  if (!r.ok || r.body.length() == 0) {
    locationMetaStatus = "META HTTP";
    return false;
  }

  return parseOpenWeatherMetaPayload(r.body);
}


// =====================================================
// BMP580 HELPERS
// =====================================================
void scanI2CBus() {
#if BMP580_ENABLED
  Serial.println("I2C scan on SDA=GPIO27 SCL=GPIO22");
  byte count = 0;

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      count++;
    }
  }

  if (count == 0) {
    Serial.println("I2C scan: no devices found");
  }
#endif
}

void initInternal() {
#if BMP580_ENABLED
  Wire.begin(BMP580_SDA_PIN, BMP580_SCL_PIN);
  Wire.setClock(100000);   // safe speed for long jumper wires

  bmp580OK = false;
  bmp580Status = "INIT";

  scanI2CBus();

  // Internal breakout boards usually use 0x46 or 0x47.
  // Try both explicitly.
  bool ok = false;

  Serial.println("Trying Internal sensor addr 0x46...");
  if (bmp580.begin(0x46, &Wire)) {
    ok = true;
    bmp580Status = "OK";
  }

  if (!ok) {
    Serial.println("Trying Internal sensor addr 0x47...");
    if (bmp580.begin(0x47, &Wire)) {
      ok = true;
      bmp580Status = "OK";
    }
  }

  if (!ok) {
    bmp580Status = "NO SENSOR";
    Serial.println("Internal sensor not found on 0x46/0x47");
    return;
  }

  bmp580.setTemperatureOversampling(BMP5XX_OVERSAMPLING_2X);
  bmp580.setPressureOversampling(BMP5XX_OVERSAMPLING_16X);
  bmp580.setIIRFilterCoeff(BMP5XX_IIR_FILTER_COEFF_3);
  bmp580.setOutputDataRate(BMP5XX_ODR_10_HZ);
  bmp580.setPowerMode(BMP5XX_POWERMODE_NORMAL);
  bmp580.enablePressure(true);

  bmp580OK = true;
  Serial.print("Internal sensor found: ");
  Serial.println(bmp580Status);
#else
  bmp580Status = "DISABLED";
#endif
}

bool readInternal() {
#if BMP580_ENABLED
  lastBmp580Ms = millis();

  if (!bmp580OK) return false;

  // Do not block reading by dataReady(); some modules/library versions
  // keep dataReady false right after boot while performReading still works.
  if (!bmp580.performReading()) {
    bmp580Status = "READ ERR";
    Serial.println("Internal sensor read error");
    return false;
  }

  bmpTempC = bmp580.temperature;

  // Different BMP5xx library versions/boards may expose pressure as Pa or hPa.
  // Auto-detect:
  //   normal pressure in Pa  ~= 101325
  //   normal pressure in hPa ~= 1013
  float rawPressure = bmp580.pressure;
  if (rawPressure > 20000.0f) {
    bmpPressureHpa = rawPressure / 100.0f;   // Pa -> hPa
  } else {
    bmpPressureHpa = rawPressure;            // already hPa
  }
  bmpPressureMmHg = pressureMmHgValue(bmpPressureHpa);

  bmpAltitudeM = bmp580.readAltitude(1013.25);

  if (bmp580Status.startsWith("OK") == false) bmp580Status = "OK";

  Serial.print("Internal T=");
  Serial.print(bmpTempC, 2);
  Serial.print("C rawP=");
  Serial.print(rawPressure, 2);
  Serial.print(" P=");
  Serial.print(bmpPressureHpa, 2);
  Serial.print("hPa ");
  Serial.print(bmpPressureMmHg, 1);
  Serial.println("mmHg");

  return true;
#else
  return false;
#endif
}

String bmp580ShortStatus() {
#if BMP580_ENABLED
  if (bmp580OK) return "Internal " + bmp580Status;
  return "Internal " + bmp580Status;
#else
  return "Internal OFF";
#endif
}

// =====================================================
// GPS HELPERS
// =====================================================
void initGPS() {
#if GPS_ENABLED
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  gpsStarted = true;
  Serial.println("GPS UART started");
#else
  gpsStarted = false;
#endif
}

void pollGPS(uint16_t maxMs) {
#if GPS_ENABLED
  if (!gpsStarted) return;

  unsigned long start = millis();
  while (GPSSerial.available() && millis() - start < maxMs) {
    gps.encode(GPSSerial.read());
  }

  gpsSatellites = gps.satellites.isValid() ? gps.satellites.value() : 0;

  if (gps.location.isValid() && gps.location.age() < 15000) {
    activeLat = gps.location.lat();
    activeLon = gps.location.lng();
    gpsFix = true;
    gpsEverFixed = true;
    lastGpsFixMs = millis();
    locationSource = "GPS";

    if (gps.altitude.isValid()) gpsAltitudeM = gps.altitude.meters();
  } else {
    gpsFix = false;
  }

  lastGpsReadMs = millis();
#else
  (void)maxMs;
#endif
}

String gpsStatusText() {
#if GPS_ENABLED
  if (!gpsStarted) return "OFF";
  if (gpsFix) return "FIX " + String(gpsSatellites) + " sat";
  if (gpsSatellites > 0) return "NOFIX " + String(gpsSatellites) + " sat";
  return "NO FIX";
#else
  return "DISABLED";
#endif
}

String activeCoordText() {
  return String(activeLat, 4) + ", " + String(activeLon, 4);
}

void applyTimeOffset(long offsetSec, const String& src, bool persist) {
  if (offsetSec < -12L * 3600L || offsetSec > 14L * 3600L) return;

  bool changed = (activeGmtOffsetSec != offsetSec || timezoneSource != src);

  activeGmtOffsetSec = offsetSec;
  timezoneSource = src;

  configTime(activeGmtOffsetSec, DAYLIGHT_OFFSET_SEC,
             "pool.ntp.org",
             "time.google.com",
             "time.nist.gov");

  if (persist) saveTimezoneSettings(offsetSec, src);

  if (changed) {
    Serial.print("Timezone offset updated: ");
    Serial.print(activeGmtOffsetSec);
    Serial.print(" source: ");
    Serial.println(timezoneSource);
  }
}

// =====================================================
// WEATHER PARSE — OPEN-METEO
// =====================================================
bool parseOpenMeteoPayload(const String& body) {
  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    Serial.print("Open-Meteo JSON error: ");
    Serial.println(err.c_str());

    weatherFailCount++;
    weatherStatus = "OM JSON";
    lastHttpError = "OM JSON";
    return false;
  }

  if (doc["utc_offset_seconds"].is<long>()) {
    long omOffset = doc["utc_offset_seconds"].as<long>();
    applyTimeOffset(omOffset, locationSource == "GPS" ? "OM GPS" : "OM DEFAULT");
  }

  JsonObject cur = doc["current"];

  if (cur.isNull()) {
    Serial.println("Open-Meteo current not found");

    weatherFailCount++;
    weatherStatus = "OM ERR";
    lastHttpError = "OM NO CURRENT";
    return false;
  }

  prevTempC = tempC;
