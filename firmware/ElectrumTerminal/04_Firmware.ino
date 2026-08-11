
  tempC = cur["temperature_2m"].as<float>();
  feelsC = cur["apparent_temperature"].as<float>();
  pressureHpa = cur["pressure_msl"].as<float>();
  windMs = cur["wind_speed_10m"].as<float>();

  int code = cur["weather_code"].as<int>();
  weatherText = weatherCodeToText(code);

  forecastOK = false;
  JsonObject daily = doc["daily"];
  if (!daily.isNull()) {
    JsonArray times = daily["time"].as<JsonArray>();
    JsonArray tmax = daily["temperature_2m_max"].as<JsonArray>();
    JsonArray tmin = daily["temperature_2m_min"].as<JsonArray>();
    JsonArray codes = daily["weather_code"].as<JsonArray>();

    if (times.size() >= FORECAST_DAYS && tmax.size() >= FORECAST_DAYS &&
        tmin.size() >= FORECAST_DAYS && codes.size() >= FORECAST_DAYS) {
      for (int i = 0; i < FORECAST_DAYS; i++) {
        forecastDate[i] = String((const char*)times[i]);
        forecastMax[i] = tmax[i].as<float>();
        forecastMin[i] = tmin[i].as<float>();
        forecastCode[i] = codes[i].as<int>();
      }
      forecastOK = true;
    }
  }

  weatherUpdate = getShortTimeStr();
  weatherOK = true;

  weatherFailCount = 0;
  weatherStatus = forecastOK ? "OM 3D OK" : "OM NOW";

  Serial.println("Open-Meteo OK");
  return true;
}

// =====================================================
// WEATHER PARSE — OPENWEATHER
// =====================================================
bool parseOpenWeatherPayload(const String& body) {
  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    Serial.print("OpenWeather JSON error: ");
    Serial.println(err.c_str());

    weatherFailCount++;
    weatherStatus = "OW JSON";
    lastHttpError = "OW JSON";
    return false;
  }

  int cod = doc["cod"].as<int>();

  if (cod != 200) {
    Serial.print("OpenWeather cod error: ");
    Serial.println(cod);

    weatherFailCount++;
    weatherStatus = "OW " + String(cod);
    lastHttpError = "OW COD " + String(cod);
    return false;
  }

  if (doc["timezone"].is<long>()) {
    long owOffset = doc["timezone"].as<long>();
    applyTimeOffset(owOffset, locationSource == "GPS" ? "OW GPS" : "OW DEFAULT");
  }

  String owCountry = String((const char*)(doc["sys"]["country"] | ""));
  String owName = String((const char*)(doc["name"] | ""));
  applyCountryCurrency(owCountry, owName, true);

  prevTempC = tempC;

  tempC = doc["main"]["temp"].as<float>();
  feelsC = doc["main"]["feels_like"].as<float>();
  pressureHpa = doc["main"]["pressure"].as<float>();
  windMs = doc["wind"]["speed"].as<float>();

  const char* desc = doc["weather"][0]["description"] | "---";
  weatherText = shortenWeather(String(desc));

  forecastOK = false;

  weatherUpdate = getShortTimeStr();
  weatherOK = true;

  weatherFailCount = 0;
  weatherStatus = "OW NOW";

  Serial.println("OpenWeather OK");
  return true;
}

// =====================================================
// FETCH WEATHER — NO WTTR
// =====================================================
bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Weather: WiFi not connected");

    weatherFailCount++;
    weatherStatus = "NO WIFI";
    weatherOK = false;
    lastHttpError = "NO WIFI";
    return false;
  }

  resolveHost("api.open-meteo.com");
  resolveHost("api.openweathermap.org");

  String omUrl = "http://api.open-meteo.com/v1/forecast?";
  omUrl += "latitude=" + String(activeLat, 4);
  omUrl += "&longitude=" + String(activeLon, 4);
  omUrl += "&current=temperature_2m,apparent_temperature,pressure_msl,wind_speed_10m,weather_code";
  omUrl += "&daily=weather_code,temperature_2m_max,temperature_2m_min";
  omUrl += "&forecast_days=3";
  omUrl += "&wind_speed_unit=ms";
  omUrl += "&timezone=auto";

  String query = "lat=" + String(activeLat, 4);
  query += "&lon=" + String(activeLon, 4);
  query += "&appid=" + String(OPENWEATHER_API_KEY);
  query += "&units=metric";
  query += "&lang=en";

  String owHttp  = "http://api.openweathermap.org/data/2.5/weather?" + query;
  String owHttps = "https://api.openweathermap.org/data/2.5/weather?" + query;

  Serial.println();
  Serial.println("Weather source 1: Open-Meteo HTTP");
  weatherStatus = "OM...";

  HttpReply r = httpGETRetry(omUrl, 1, 500, 5000);

  if (r.ok && r.body.length() > 0) {
    if (parseOpenMeteoPayload(r.body)) {
      weatherStatus = forecastOK ? "OM 3D OK" : "OM NOW";

      // Open-Meteo gives weather/timezone, but not country.
      // Ask OpenWeather once for sys.country/name to auto-select main currency.
      fetchLocationMeta();

      return true;
    }
  }

  Serial.println();
  Serial.println("Weather source 2: OpenWeather HTTP");
  weatherStatus = "OW H...";

  r = httpGETRetry(owHttp, 1, 500, 4500);

  if (r.ok && r.body.length() > 0) {
    if (parseOpenWeatherPayload(r.body)) {
      weatherStatus = "OpenWeather HTTP";
      return true;
    }
  }

  Serial.println();
  Serial.println("Weather source 3: OpenWeather HTTPS");
  weatherStatus = "OW S...";

  r = httpGETRetry(owHttps, 1, 500, 5000);

  if (r.ok && r.body.length() > 0) {
    if (parseOpenWeatherPayload(r.body)) {
      weatherStatus = "OpenWeather HTTPS";
      return true;
    }
  }

  Serial.println("All weather sources failed");

  weatherFailCount++;
  weatherStatus = "FAIL " + String(weatherFailCount);
  weatherOK = false;
  forecastOK = false;

  // Старые данные не затираем, чтобы экран не превращался в кладбище прочерков.
  return false;
}

// =====================================================
// FETCH USD/RUB
// =====================================================
bool fetchUsdRub() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("FX: WiFi not connected");
    usdOK = false;
    marketStatus = "NO WIFI";
    lastHttpError = "NO WIFI";
    return false;
  }

  marketStatus = "FX...";
  HttpReply r = httpGETRetry("https://open.er-api.com/v6/latest/USD", 1, 500, 5500);

  if (!r.ok || r.body.length() == 0) {
    Serial.println("FX request failed");
    usdOK = false;
    marketStatus = "FX FAIL";
    return false;
  }

  DynamicJsonDocument doc(12288);
  DeserializationError err = deserializeJson(doc, r.body);

  if (err) {
    Serial.print("FX JSON error: ");
    Serial.println(err.c_str());

    usdOK = false;
    marketStatus = "FX JSON";
    lastHttpError = "FX JSON";
    return false;
  }

  float rub = doc["rates"]["RUB"].as<float>();
  float eur = doc["rates"]["EUR"].as<float>();
  float thb = doc["rates"]["THB"].as<float>();
  float vnd = doc["rates"]["VND"].as<float>();

  float mainRate = 0;
  if (mainCurrency == "USD") {
    mainRate = 1.0f;
  } else {
    mainRate = doc["rates"][mainCurrency].as<float>();
  }

  if (mainRate <= 0) {
    Serial.print("Main currency rate not found: ");
    Serial.println(mainCurrency);
    // Fallback to RUB if API does not contain selected currency.
    mainCurrency = "RUB";
    countryCode = "RU";
    mainRate = rub;
    saveLocationCurrencySettings();
  }

  if (rub <= 0) {
    Serial.println("FX RUB parse failed");
    usdOK = false;
    marketStatus = "FX PARSE";
    lastHttpError = "FX PARSE";
    return false;
  }

  prevUsdRub = usdRub;
  prevMainFxRate = mainFxRate;

  usdRub = rub;
  mainFxRate = mainRate;

  // open.er-api returns all rates against USD.
  // RUB per foreign unit = RUB_per_USD / FOREIGN_per_USD.
  if (eur > 0) eurRub = rub / eur;
  if (thb > 0) thbRub = rub / thb;
  if (vnd > 0) vnd1000Rub = rub / vnd * 1000.0;

  marketUpdate = getShortTimeStr();
  usdOK = true;
  marketStatus = "FX OK";

  Serial.print("USD/RUB OK: ");
  Serial.println(usdRub, 2);
  Serial.print("EUR/RUB OK: ");
  Serial.println(eurRub, 2);
  Serial.print("THB/RUB OK: ");
  Serial.println(thbRub, 4);
  Serial.print("1000 VND/RUB OK: ");
  Serial.println(vnd1000Rub, 4);

  return true;
}

// =====================================================
// FETCH GOLD
// =====================================================
bool fetchGold() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Gold: WiFi not connected");
    goldOK = false;
    marketStatus = "NO WIFI";
    lastHttpError = "NO WIFI";
    return false;
  }

  marketStatus = "GOLD...";
  HttpReply r = httpGETRetry("https://api.gold-api.com/price/XAU", 1, 500, 5500);

  if (!r.ok || r.body.length() == 0) {
    Serial.println("Gold request failed");
    goldOK = false;
    marketStatus = "GOLD FAIL";
    return false;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, r.body);

  if (err) {
    Serial.print("Gold JSON error: ");
    Serial.println(err.c_str());

    goldOK = false;
    marketStatus = "GOLD JSON";
    lastHttpError = "GOLD JSON";
    return false;
  }

  if (doc["price"].is<float>() || doc["price"].is<double>() || doc["price"].is<int>()) {
    goldUsdOz = doc["price"].as<float>();
  } else if (doc["ask"].is<float>() || doc["ask"].is<double>() || doc["ask"].is<int>()) {
    goldUsdOz = doc["ask"].as<float>();
  } else {
    Serial.println("Gold price field not found");
    goldOK = false;
    marketStatus = "GOLD FIELD";
    lastHttpError = "GOLD FIELD";
    return false;
  }

  if (goldUsdOz <= 0) {
    Serial.println("Gold parse failed");
    goldOK = false;
    marketStatus = "GOLD PARSE";
    lastHttpError = "GOLD PARSE";
    return false;
  }

  prevGoldRubGram = goldRubGram;
  prevGoldMainGram = goldMainGram;

  if (!isnan(usdRub) && usdRub > 0) {
    goldRubGram = goldUsdOz * usdRub / 31.1034768;
  }

  if (!isnan(mainFxRate) && mainFxRate > 0) {
    goldMainGram = goldUsdOz * mainFxRate / 31.1034768;
  }

  marketUpdate = getShortTimeStr();
  goldOK = true;
  marketStatus = "OK";

  Serial.print("Gold USD/oz OK: ");
  Serial.println(goldUsdOz, 2);

  Serial.print("Gold RUB/g: ");
  Serial.println(goldRubGram, 2);

  return true;
}



// =====================================================
// FETCH SILVER
// =====================================================
bool fetchSilver() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Silver: WiFi not connected");
    silverOK = false;
    lastHttpError = "NO WIFI";
    return false;
  }

  HttpReply r = httpGETRetry("https://api.gold-api.com/price/XAG", 1, 500, 5500);

  if (!r.ok || r.body.length() == 0) {
    Serial.println("Silver request failed");
    silverOK = false;
    return false;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, r.body);

  if (err) {
    Serial.print("Silver JSON error: ");
    Serial.println(err.c_str());
    silverOK = false;
    lastHttpError = "SILVER JSON";
    return false;
  }

  if (doc["price"].is<float>() || doc["price"].is<double>() || doc["price"].is<int>()) {
    silverUsdOz = doc["price"].as<float>();
  } else if (doc["ask"].is<float>() || doc["ask"].is<double>() || doc["ask"].is<int>()) {
    silverUsdOz = doc["ask"].as<float>();
  } else {
    Serial.println("Silver price field not found");
    silverOK = false;
