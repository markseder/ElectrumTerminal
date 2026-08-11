    Serial.println(r.code);

    if (i < attempts) delay(pauseMs);
  }

  Serial.println("HTTP all attempts failed");
  return r;
}

// =====================================================
// WEB PAGE
// =====================================================
String htmlPage() {
  String ipInfo = wifiIpText();

  String page = R"=====(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Electrum TERMINAL</title>
<style>
:root{--bg:#070b14;--card:#101827;--card2:#0b1220;--cyan:#00e5ff;--green:#39ff88;--red:#ff3b6b;--yellow:#ffe14d;--muted:#8aa0b7;--line:#1d3350;}
*{box-sizing:border-box} body{margin:0;font-family:Arial,Helvetica,sans-serif;background:radial-gradient(circle at top,#153052 0,#070b14 42%,#03050a 100%);color:#e8f7ff;padding:18px;}
.wrap{max-width:980px;margin:auto}.top{display:flex;justify-content:space-between;gap:12px;align-items:center;margin-bottom:14px}.brand{font-size:24px;font-weight:800;letter-spacing:1px;color:var(--cyan);text-shadow:0 0 18px #00e5ff80}.pill{border:1px solid var(--line);background:#07101d;border-radius:999px;padding:7px 12px;color:var(--muted);font-size:13px}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}.card{background:linear-gradient(180deg,var(--card),var(--card2));border:1px solid var(--line);border-radius:18px;padding:14px;box-shadow:0 10px 30px #0008, inset 0 0 20px #00e5ff08}.wide{grid-column:span 2}.full{grid-column:span 4}.title{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:1.5px;margin-bottom:8px}.val{font-size:27px;font-weight:800;line-height:1.15}.small{font-size:13px;color:var(--muted);line-height:1.35}.ok{color:var(--green)}.bad{color:var(--red)}.warn{color:var(--yellow)}.cyan{color:var(--cyan)}.red{color:var(--red)}.green{color:var(--green)}.yellow{color:var(--yellow)}.row{display:flex;justify-content:space-between;border-bottom:1px solid #18304a;padding:7px 0;gap:10px}.row:last-child{border-bottom:0}.btn{display:inline-block;width:100%;border:0;border-radius:12px;background:linear-gradient(90deg,#00e5ff,#39ff88);color:#001018;font-weight:800;padding:12px;margin-top:10px}.input{width:100%;padding:12px;margin-top:6px;border-radius:10px;border:1px solid #29425f;background:#050912;color:#fff}.form{display:grid;grid-template-columns:1fr 1fr auto;gap:10px;align-items:end}.dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:7px;background:var(--red);box-shadow:0 0 10px var(--red)}.dot.on{background:var(--green);box-shadow:0 0 10px var(--green)}.quake{display:grid;grid-template-columns:68px 1fr 52px;gap:10px;align-items:center;border-bottom:1px solid #18304a;padding:8px 0}.quake:last-child{border-bottom:0}.mag{font-size:20px;font-weight:800;color:var(--yellow)}.place{font-size:13px;color:#e8f7ff}.qtime{font-size:12px;color:var(--muted);text-align:right}.footer{margin-top:12px;color:var(--muted);font-size:12px;text-align:center}a{color:var(--cyan)}
@media(max-width:760px){.grid{grid-template-columns:1fr}.wide,.full{grid-column:span 1}.form{grid-template-columns:1fr}.top{display:block}.pill{margin-top:8px;display:inline-block}.val{font-size:24px}}
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <div class="brand">ELECTRUM TERMINAL</div>
    <div class="pill"><span id="dot" class="dot"></span><span id="wifi">loading</span> · <span id="ip">---</span></div>
  </div>

  <div class="grid">
    <div class="card wide">
      <div class="title">Weather</div>
      <div class="val" id="temp">-- °C</div>
      <div class="small"><span id="weatherText">---</span> · feels <span id="feels">--</span> °C</div>
      <div class="small">Pressure: <b id="press">--</b> mmHg · Wind: <b id="wind">--</b> m/s</div>
      <div class="small">Status: <span id="weatherStatus">---</span></div>
      <div class="small">Internal: <span id="bmpStatus">---</span> · <b id="bmpTemp">--</b> °C · <b id="bmpPress">--</b> mmHg</div>
      <div class="small">Location: <b id="locsrc">---</b> · <span id="coords">---</span></div>
      <div class="small">GPS: <span id="gps">---</span> · TZ: <span id="tz">---</span></div>
    </div>

    <div class="card wide">
      <div class="title">Earthquakes</div>
      <div class="small">USGS M4.5+ · Status: <span id="quakesStatus">---</span> · Updated: <span id="quakesUpdate">--:--</span></div>
      <div id="quakeList" class="small" style="margin-top:8px">loading...</div>
    </div>

    <div class="card">
      <div class="title">USD/RUB</div>
      <div class="val yellow" id="usd">--</div>
      <div class="small">Market: <span id="marketStatus">---</span></div>
    </div>

    <div class="card">
      <div class="title">Gold</div>
      <div class="val yellow" id="gold">--</div>
      <div class="small">RUB per gram</div>
    </div>

    <div class="card wide">
      <div class="title">FX Board</div>
      <div class="row"><span>EUR/RUB</span><b id="eur">--</b></div>
      <div class="row"><span>THB/RUB</span><b id="thb">--</b></div>
      <div class="row"><span>1000 VND/RUB</span><b id="vnd">--</b></div>
    </div>

    <div class="card wide">
      <div class="title">Commodities</div>
      <div class="row"><span>Silver XAG</span><b id="silver">--</b></div>
      <div class="row"><span>Brent</span><b id="brent">--</b></div>
      <div class="row"><span>WTI</span><b id="wti">--</b></div>
    </div>

    <div class="card full">
      <div class="title">System</div>
      <div class="row"><span>Heap</span><b id="heap">--</b></div>
      <div class="row"><span>Uptime</span><b id="uptime">--</b></div>
      <div class="row"><span>HTTP</span><b><span id="httpCode">--</span> <span id="httpHost">---</span></b></div>
      <div class="row"><span>Error</span><b id="err">--</b></div>
    </div>

    <div class="card full">
      <div class="title">WiFi setup</div>
      <form class="form" action="/save" method="POST">
        <label><span class="small">SSID</span><input class="input" name="ssid" placeholder="Network name" required></label>
        <label><span class="small">Password</span><input class="input" name="pass" type="password" placeholder="Password"></label>
        <button class="btn" type="submit">SAVE & REBOOT</button>
      </form>
      <div class="small">Current SSID: <b id="ssid">---</b>. Status JSON: <a href="/status">/status</a></div>
    </div>
  </div>
  <div class="footer">Auto refresh every 3 seconds · Electrum TERMINAL CYD</div>
</div>
<script>
function n(v,d=2){return (v===0||v)?Number(v).toFixed(d):'--'}
function cls(el,c){el.className=c}
async function load(){
  try{
    const r=await fetch('/status?ts='+Date.now());
    const s=await r.json();
    document.getElementById('wifi').textContent=s.wifi||'---';
    document.getElementById('ip').textContent=s.ip||'---';
    document.getElementById('ssid').textContent=s.ssid||'---';
    document.getElementById('dot').className=(s.wifi==='ONLINE')?'dot on':'dot';
    document.getElementById('temp').textContent=n(s.temp_c,1)+' °C';
    document.getElementById('temp').className='val '+(s.temp_c<0?'cyan':(s.temp_c<=13?'green':'red'));
    document.getElementById('feels').textContent=n(s.feels_c,1);
    document.getElementById('press').textContent=n(s.pressure_mmhg,0);
    document.getElementById('press').className=(s.pressure_mmhg<750?'cyan':(s.pressure_mmhg<=760?'green':'red'));
    document.getElementById('wind').textContent=n(s.wind_ms,1);
    document.getElementById('weatherText').textContent=s.weather_text||'---';
    document.getElementById('weatherStatus').textContent=s.weather_status||'---';
    document.getElementById('weatherStatus').className=s.weather_ok?'ok':'bad';
    document.getElementById('bmpStatus').textContent=s.bmp580_status||'---';
    document.getElementById('bmpStatus').className=s.bmp580_ok?'ok':'warn';
    document.getElementById('bmpTemp').textContent=n(s.bmp580_temp_c,1);
    document.getElementById('bmpPress').textContent=n(s.bmp580_pressure_mmhg,0);
    document.getElementById('bmpPress').className=(s.bmp580_pressure_mmhg<750?'cyan':(s.bmp580_pressure_mmhg<=760?'green':'red'));
    document.getElementById('locsrc').textContent=(s.location_name||s.location_source||'---')+' '+(s.country||'')+'/'+(s.main_currency||'');
    document.getElementById('coords').textContent=(s.lat&&s.lon)?(Number(s.lat).toFixed(4)+', '+Number(s.lon).toFixed(4)):'---';
    document.getElementById('gps').textContent=(s.gps_fix?'FIX ':'NO FIX ')+(s.gps_sat||0)+' sat';
    document.getElementById('gps').className=s.gps_fix?'ok':'warn';
    document.getElementById('tz').textContent=(s.tz_source||'---')+' '+((s.tz_offset_sec||0)/3600)+'h';

    document.getElementById('quakesStatus').textContent=s.quakes_status||'---';
    document.getElementById('quakesStatus').className=s.quakes_ok?'ok':'warn';
    document.getElementById('quakesUpdate').textContent=s.quakes_update||'--:--';
    const qbox=document.getElementById('quakeList');
    const qs=s.quakes||[];
    qbox.innerHTML='';
    let shown=0;
    qs.forEach(q=>{
      if(!q || !q.mag) return;
      shown++;
      const div=document.createElement('div');
      div.className='quake';
      div.innerHTML='<div class="mag">M '+n(q.mag,1)+'</div><div class="place">'+(q.place||'---')+'</div><div class="qtime">'+(q.time||'--:--')+'</div>';
      qbox.appendChild(div);
    });
    if(!shown) qbox.innerHTML='<div class="small">No M4.5+ events loaded. Open QUAKES on device or tap UPDATE there.</div>';

    document.getElementById('usd').textContent=n(s.usd_rub,2);
    document.getElementById('gold').textContent=n(s.gold_rub_g,0);
    document.getElementById('eur').textContent=n(s.eur_rub,2);
    document.getElementById('thb').textContent=n(s.thb_rub,3);
    document.getElementById('vnd').textContent=n(s.vnd1000_rub,2);
    document.getElementById('silver').textContent=n(s.silver_usd_oz,2)+' USD/oz';
    document.getElementById('brent').textContent=n(s.brent_usd,2)+' USD';
    document.getElementById('wti').textContent=n(s.wti_usd,2)+' USD';
    document.getElementById('marketStatus').textContent=(s.usd_ok&&s.gold_ok)?'OK':'CHECK';
    document.getElementById('marketStatus').className=(s.usd_ok&&s.gold_ok)?'ok':'warn';
    document.getElementById('heap').textContent=Math.round((s.heap||0)/1024)+' KB';
    document.getElementById('uptime').textContent=s.uptime||'--';
    document.getElementById('httpCode').textContent=s.last_http_code||'--';
    document.getElementById('httpHost').textContent=s.last_http_host||'---';
    document.getElementById('err').textContent=s.last_http_error||'--';
    document.getElementById('err').className=(s.last_http_error==='OK')?'ok':'warn';
  }catch(e){document.getElementById('wifi').textContent='WEB ERROR';document.getElementById('dot').className='dot';}
}
load();setInterval(load,3000);
</script>
</body></html>)=====";
  return page;
}

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  ssid.trim();

  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID is empty");
    return;
  }

  saveWiFiSettings(ssid, pass);

  String response = "";
  response += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  response += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  response += "</head><body style='font-family:Arial;background:#111;color:#eee;padding:24px;'>";
  response += "<h2>Saved</h2>";
  response += "<p>WiFi settings saved. Electrum TERMINAL will reboot now.</p>";
  response += "</body></html>";

  server.send(200, "text/html", response);

  delay(1200);
  ESP.restart();
}

void handleApiStatus() {
  DynamicJsonDocument doc(6144);
  doc["wifi"] = wifiStatusText();
  doc["ip"] = wifiIpText();
  doc["ssid"] = wifiSsid;
  doc["weather_ok"] = weatherOK;
  doc["usd_ok"] = usdOK;
  doc["gold_ok"] = goldOK;
  doc["oil_ok"] = oilOK;
  doc["quakes_ok"] = quakesOK;
  doc["calendar_date"] = getDateStr();
  doc["space_kp"] = isnan(spaceKpNow) ? 0 : spaceKpNow;
  doc["calendar_time"] = getTimeStr();
  doc["gps_enabled"] = GPS_ENABLED;
  doc["gps_fix"] = gpsFix;
  doc["gps_sat"] = gpsSatellites;
  doc["gps_alt_m"] = isnan(gpsAltitudeM) ? 0 : gpsAltitudeM;
  doc["location_source"] = locationSource;
  doc["lat"] = activeLat;
  doc["lon"] = activeLon;
  doc["tz_offset_sec"] = activeGmtOffsetSec;
  doc["tz_source"] = timezoneSource;
  doc["country"] = countryCode;
  doc["main_currency"] = mainCurrency;
  doc["location_name"] = locationName;
  doc["location_meta_status"] = locationMetaStatus;
  doc["main_fx_rate"] = isnan(mainFxRate) ? 0 : mainFxRate;
  doc["gold_main_g"] = isnan(goldMainGram) ? 0 : goldMainGram;
  doc["temp_c"] = isnan(tempC) ? 0 : tempC;
  doc["feels_c"] = isnan(feelsC) ? 0 : feelsC;
  doc["pressure_hpa"] = isnan(pressureHpa) ? 0 : pressureHpa;
  doc["pressure_mmhg"] = isnan(pressureHpa) ? 0 : pressureMmHgValue(pressureHpa);
  doc["wind_ms"] = isnan(windMs) ? 0 : windMs;
  doc["weather_text"] = weatherText;
  doc["weather_status"] = weatherStatus;
  doc["bmp580_ok"] = bmp580OK;
  doc["bmp580_status"] = bmp580Status;
  doc["bmp580_temp_c"] = isnan(bmpTempC) ? 0 : bmpTempC;
  doc["bmp580_pressure_hpa"] = isnan(bmpPressureHpa) ? 0 : bmpPressureHpa;
  doc["bmp580_pressure_mmhg"] = isnan(bmpPressureMmHg) ? 0 : bmpPressureMmHg;
  doc["bmp580_alt_m"] = isnan(bmpAltitudeM) ? 0 : bmpAltitudeM;
  doc["usd_rub"] = isnan(usdRub) ? 0 : usdRub;
  doc["gold_rub_g"] = isnan(goldRubGram) ? 0 : goldRubGram;
  doc["silver_usd_oz"] = isnan(silverUsdOz) ? 0 : silverUsdOz;
  doc["brent_usd"] = isnan(oilBrentUsd) ? 0 : oilBrentUsd;
  doc["wti_usd"] = isnan(oilWtiUsd) ? 0 : oilWtiUsd;
  doc["eur_rub"] = isnan(eurRub) ? 0 : eurRub;
  doc["thb_rub"] = isnan(thbRub) ? 0 : thbRub;
  doc["vnd1000_rub"] = isnan(vnd1000Rub) ? 0 : vnd1000Rub;
  doc["rotation"] = displayRotation;
  doc["oil_brent_usd"] = isnan(oilBrentUsd) ? 0 : oilBrentUsd;
  doc["oil_wti_usd"] = isnan(oilWtiUsd) ? 0 : oilWtiUsd;
  doc["last_http_host"] = lastHttpHost;
  doc["last_http_code"] = lastHttpCode;
  doc["last_http_error"] = lastHttpError;
  doc["heap"] = ESP.getFreeHeap();
  doc["uptime"] = uptimeStr();

  doc["quakes_status"] = quakesStatus;
  doc["quakes_update"] = quakesUpdate;
  JsonArray qa = doc.createNestedArray("quakes");
  for (int i = 0; i < QUAKE_ITEMS; i++) {
    JsonObject q = qa.createNestedObject();
    q["mag"] = isnan(quakeMag[i]) ? 0 : quakeMag[i];
    q["place"] = quakePlace[i];
    q["time"] = quakeTime[i];
  }

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/status", HTTP_GET, handleApiStatus);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.begin();

  Serial.println("Web server started");
}

// =====================================================
// WIFI
// =====================================================
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  WiFi.config(
    INADDR_NONE,
    INADDR_NONE,
    INADDR_NONE,
    IPAddress(1, 1, 1, 1),
    IPAddress(8, 8, 8, 8)
  );

  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());

  Serial.print("WiFi connecting to ");
  Serial.println(wifiSsid);

  int tries = 0;

  while (WiFi.status() != WL_CONNECTED && tries < 60) {
    delay(250);
    Serial.print(".");
    tries++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi OK");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("DNS 1: ");
    Serial.println(WiFi.dnsIP(0));
    Serial.print("DNS 2: ");
    Serial.println(WiFi.dnsIP(1));

    configMode = false;
    return true;
  }

  Serial.println("WiFi FAIL");
  return false;
}

void startConfigAP() {
  configMode = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(CONFIG_AP_SSID, CONFIG_AP_PASS);

  Serial.println("Config AP started");
  Serial.print("AP SSID: ");
  Serial.println(CONFIG_AP_SSID);
  Serial.print("AP PASS: ");
  Serial.println(CONFIG_AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void reconnectWiFiIfNeeded() {
  if (configMode) return;
  if (WiFi.status() == WL_CONNECTED) return;

  if (millis() - lastWifiTryMs > 15000) {
    lastWifiTryMs = millis();

    Serial.println("WiFi reconnect...");
    WiFi.disconnect();
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  }
}



// =====================================================
// TIMEZONE STORAGE
// =====================================================
void loadTimezoneSettings() {
  prefs.begin("tz", true);
  bool saved = prefs.getBool("saved", false);
  long savedOffset = prefs.getLong("offset", DEFAULT_GMT_OFFSET_SEC);
  String savedSource = prefs.getString("source", "DEFAULT");
  prefs.end();

  if (saved && savedOffset >= -12L * 3600L && savedOffset <= 14L * 3600L) {
    activeGmtOffsetSec = savedOffset;
    timezoneSource = "SAVED " + savedSource;
  } else {
    activeGmtOffsetSec = DEFAULT_GMT_OFFSET_SEC;
    timezoneSource = "DEFAULT";
  }

  Serial.print("Timezone loaded: ");
  Serial.print(activeGmtOffsetSec);
  Serial.print(" source: ");
  Serial.println(timezoneSource);
}

void saveTimezoneSettings(long offsetSec, const String& src) {
  if (offsetSec < -12L * 3600L || offsetSec > 14L * 3600L) return;

  prefs.begin("tz", false);
  prefs.putBool("saved", true);
  prefs.putLong("offset", offsetSec);
  prefs.putString("source", src);
  prefs.end();

  Serial.print("Timezone saved: ");
  Serial.print(offsetSec);
  Serial.print(" source: ");
  Serial.println(src);
}
