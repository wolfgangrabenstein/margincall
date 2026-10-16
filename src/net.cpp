#include "net.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>

// ================================================================ Maerkte
Market markets[M_COUNT] = {
  {"%5EDJI",  "DOW"},
  {"%5EGSPC", "S&P"},
  {"%5ENDX",  "NDX"},
  {"CL%3DF",  "OEL"},
};
Stock stocks[STOCK_COUNT] = {
  {"GS", "GS", M_DOW},     {"CAT", "CAT", M_DOW},     {"HD", "HD", M_DOW},       {"JPM", "JPM", M_DOW},
  {"BA", "BA", M_DOW},     {"KO", "KO", M_DOW},       {"AAPL", "AAPL", M_SPX},   {"AMZN", "AMZN", M_SPX},
  {"GOOGL", "GOOGL", M_SPX}, {"META", "META", M_SPX}, {"LLY", "LLY", M_SPX},     {"BRK-B", "BRK.B", M_SPX},
  {"NVDA", "NVDA", M_NDX}, {"MSFT", "MSFT", M_NDX},   {"AVGO", "AVGO", M_NDX},   {"TSLA", "TSLA", M_NDX},
  {"NFLX", "NFLX", M_NDX}, {"AMD", "AMD", M_NDX},     {"XOM", "XOM", M_OIL},     {"CVX", "CVX", M_OIL},
};
static uint32_t lastFetchMs = 0;
static bool fetchedOnce = false;

uint32_t marketsAgeMs() { return fetchedOnce ? millis() - lastFetchMs : 0xFFFFFFFFu; }

void marketsSimulate() {
  static const float base[M_COUNT] = {51000, 7700, 30000, 90};
  for (int m = 0; m < M_COUNT; m++) {
    Market& k = markets[m];
    if (k.live) continue;   // echte Daten nicht ueberschreiben
    float v = base[m] * (0.97f + 0.06f * (esp_random() % 1000) / 1000.0f);
    k.prev = v;
    k.n = 78;
    float drift = ((int)(esp_random() % 2001) - 1000) / 1000.0f * 0.0004f;
    for (int i = 0; i < k.n; i++) {
      v *= 1.0f + drift + ((int)(esp_random() % 2001) - 1000) / 1000.0f * 0.0015f;
      k.series[i] = v;
    }
    k.price = v;
    k.chg = (k.price - k.prev) / k.prev * 100.0f;
  }
  // simulierte Einzelwerte folgen grob ihrem Index (ohne Preis, nur Veraenderung)
  for (auto& s : stocks) {
    if (s.live) continue;
    s.price = 0;
    s.chg = markets[s.market].chg + ((int)(esp_random() % 2001) - 1000) / 1000.0f * 1.5f;
  }
}

// Alle Einzelwerte mit einer Anfrage (Yahoo "spark", ca. 5 KB): {"GS":{..."chartPreviousClose":805.1,"close":[812.3]},...}
static float jsonNum(const String& s, int from, const char* key) {
  int k = s.indexOf(key, from);
  if (k < 0) return NAN;
  k += strlen(key);
  while (k < (int)s.length() && (s[k] == ' ' || s[k] == '[')) k++;
  return s.substring(k, k + 16).toFloat();
}
static int fetchStocks() {
  String syms;
  for (auto& s : stocks) { if (syms.length()) syms += ','; syms += s.symbol; }
  WiFiClientSecure client;
  client.setInsecure();   // oeffentliche Kursdaten, nur lesend
  client.setTimeout(8);
  HTTPClient http;
  http.setTimeout(8000);
  http.setUserAgent("Mozilla/5.0 (MarginCall Cardputer)");
  String url = "https://query1.finance.yahoo.com/v8/finance/spark?symbols=" + syms + "&range=1d&interval=1d";
  if (!http.begin(client, url)) return 0;
  int ok = 0;
  if (http.GET() == 200) {
    String body = http.getString();
    for (auto& s : stocks) {
      String key = String("\"") + s.symbol + "\":{";
      int at = body.indexOf(key);
      if (at < 0) continue;
      int end = body.indexOf("}", at);   // Objekt des Werts ist flach
      float prev = jsonNum(body, at, "\"chartPreviousClose\":");
      int c = body.indexOf("\"close\":[", at);
      if (c < 0 || (end > 0 && c > end) || isnan(prev) || prev <= 0) continue;
      // letzter Wert im close-Array
      int ce = body.indexOf("]", c);
      int last = body.lastIndexOf(",", ce);
      if (last < c) last = c + 8;   // nur ein Wert: direkt hinter '['
      float p = body.substring(last + 1, ce).toFloat();
      if (p <= 0) continue;
      s.price = p; s.chg = (p - prev) / prev * 100.0f; s.live = true; ok++;
    }
  }
  http.end();
  return ok;
}

// --- kleiner Stream-Parser fuer die Yahoo-Chart-JSON (kein Komplett-Puffer noetig)
struct Reader {
  Stream* s; uint32_t deadline;
  uint8_t buf[256]; int len = 0, pos = 0;
  int next() {
    while (pos >= len) {
      if (millis() > deadline) return -1;
      int a = s->available();
      if (a <= 0) {
        if (!static_cast<WiFiClient*>(s)->connected() && s->available() <= 0) return -1;
        delay(2); continue;
      }
      len = s->readBytes(buf, min(a, (int)sizeof(buf))); pos = 0;
    }
    return buf[pos++];
  }
};

static bool readNumber(Reader& r, float& out, int& term) {
  char b[24]; int n = 0; int c;
  while ((c = r.next()) == ' ') {}
  if (c == 'n') { r.next(); r.next(); r.next(); term = r.next(); return false; }   // null
  while (c >= 0 && (isdigit(c) || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E')) {
    if (n < 23) b[n++] = (char)c;
    c = r.next();
  }
  b[n] = 0; term = c;
  if (!n) return false;
  out = strtof(b, nullptr);
  return true;
}

static void seriesPush(Market& m, float v, int& stride, int& cnt, float& acc) {
  acc += v; cnt++;
  if (cnt < stride) return;
  if (m.n >= SERIES_MAX) {   // halbieren: je zwei Werte mitteln
    for (int i = 0; i < SERIES_MAX / 2; i++) m.series[i] = (m.series[2 * i] + m.series[2 * i + 1]) * 0.5f;
    m.n = SERIES_MAX / 2; stride *= 2;
  }
  m.series[m.n++] = acc / cnt; acc = 0; cnt = 0;
}

static bool parseChart(Stream* s, Market& m) {
  static const char* KEYS[3] = {"\"regularMarketPrice\":", "\"chartPreviousClose\":", "\"close\":["};
  int match[3] = {0, 0, 0};
  Reader r; r.s = s; r.deadline = millis() + 10000;
  float price = NAN, prev = NAN;
  int n0 = m.n; m.n = 0;
  bool gotSeries = false;
  int c;
  while ((c = r.next()) >= 0) {
    for (int k = 0; k < 3; k++) {
      if (c == KEYS[k][match[k]]) match[k]++;
      else match[k] = (c == KEYS[k][0]) ? 1 : 0;
      if (KEYS[k][match[k]] == 0) {
        match[k] = 0;
        float v; int term;
        if (k == 0) { if (readNumber(r, v, term)) price = v; }
        else if (k == 1) { if (readNumber(r, v, term)) prev = v; }
        else if (!gotSeries) {
          int stride = 1, cnt = 0; float acc = 0;
          do {
            bool ok = readNumber(r, v, term);
            if (ok) seriesPush(m, v, stride, cnt, acc);
          } while (term == ',');
          gotSeries = true;
        }
      }
    }
    if (gotSeries && !isnan(price) && !isnan(prev)) break;
  }
  if (isnan(price) || isnan(prev) || prev <= 0) { m.n = n0; return false; }
  m.price = price; m.prev = prev;
  m.chg = (price - prev) / prev * 100.0f;
  if (m.n < 2) { m.series[0] = prev; m.series[1] = price; m.n = 2; }
  m.live = true;
  return true;
}

int marketsFetch(void (*progress)(int, bool)) {
  if (WiFi.status() != WL_CONNECTED) { marketsSimulate(); return 0; }
  streamPause(true);
  int ok = 0;
  for (int i = 0; i < M_COUNT; i++) {
    bool good = false;
    {
      WiFiClientSecure client;
      client.setInsecure();   // oeffentliche Kursdaten, nur lesend
      client.setTimeout(8);
      HTTPClient http;
      http.useHTTP10(true);   // kein chunked encoding -> einfacher Stream
      http.setTimeout(8000);
      http.setUserAgent("Mozilla/5.0 (MarginCall Cardputer)");
      String interval = (i == M_OIL) ? "15m" : "5m";
      String url = String("https://query1.finance.yahoo.com/v8/finance/chart/") + markets[i].symbol +
                   "?range=1d&interval=" + interval;
      if (http.begin(client, url)) {
        int code = http.GET();
        if (code == 200) good = parseChart(http.getStreamPtr(), markets[i]);
        http.end();
      }
    }
    if (good) ok++;
    if (progress) progress(i, good);
  }
  if (ok) { lastFetchMs = millis(); fetchedOnce = true; fetchStocks(); }
  marketsSimulate();   // fehlende Maerkte simulieren
  streamPause(false);
  return ok;
}

// ================================================================ WLAN
static Preferences prefs;
static char ssid[33] = "", pass[65] = "";
static char ipStr[16] = "";
static bool serverStarted = false;
static void streamStart();

void netInit() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  if (prefs.begin("margincall", true)) {
    prefs.getString("ssid", ssid, sizeof(ssid));
    prefs.getString("pass", pass, sizeof(pass));
    prefs.end();
  }
  if (ssid[0]) WiFi.begin(ssid, pass);
}

bool netConnected() {
  bool c = WiFi.status() == WL_CONNECTED;
  if (c && !serverStarted) streamStart();
  return c;
}
bool netHasCreds() { return ssid[0] != 0; }
const char* netSSID() { return ssid; }
const char* netIP() {
  if (WiFi.status() != WL_CONNECTED) return "";
  IPAddress ip = WiFi.localIP();
  snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return ipStr;
}

int netScan(char names[][33], int8_t* rssi, bool* secured, int maxCount) {
  WiFi.disconnect();
  int n = WiFi.scanNetworks();
  int out = 0;
  for (int i = 0; i < n && out < maxCount; i++) {
    String s = WiFi.SSID(i);
    if (!s.length()) continue;
    bool dup = false;
    for (int j = 0; j < out; j++) if (s == names[j]) dup = true;
    if (dup) continue;
    strncpy(names[out], s.c_str(), 32); names[out][32] = 0;
    rssi[out] = WiFi.RSSI(i);
    secured[out] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    out++;
  }
  WiFi.scanDelete();
  if (ssid[0]) WiFi.begin(ssid, pass);   // alte Verbindung wieder aufnehmen
  return out;
}

bool netConnect(const char* s, const char* p, uint32_t timeoutMs) {
  WiFi.disconnect();
  delay(100);
  WiFi.begin(s, p);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) delay(100);
  if (WiFi.status() != WL_CONNECTED) {
    if (ssid[0]) WiFi.begin(ssid, pass);
    return false;
  }
  strncpy(ssid, s, sizeof(ssid) - 1); strncpy(pass, p, sizeof(pass) - 1);
  if (prefs.begin("margincall", false)) {
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();
  }
  return true;
}

void netForget() {
  ssid[0] = pass[0] = 0;
  if (prefs.begin("margincall", false)) { prefs.clear(); prefs.end(); }
  WiFi.disconnect();
}

// ================================================================ Stream-Server
static constexpr int FW = 240, FH = 135;
static WebServer server(80);
static uint8_t* snap = nullptr;             // RGB332-Schnappschuss
static volatile bool wantFrame = false, frameReady = false, paused = false;
static volatile uint32_t lastViewerMs[4];
static volatile int viewerSlot = 0;

static const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>MARGIN CALL Live</title>
<style>
body{margin:0;background:#05070d;color:#d8e0f0;font:14px/1.4 system-ui,sans-serif;display:flex;flex-direction:column;align-items:center;padding:16px}
h1{font:700 22px monospace;color:#ffd23a;margin:4px 0 12px;letter-spacing:2px}
canvas{width:min(960px,100%);aspect-ratio:240/135;image-rendering:pixelated;border:2px solid #1e7a46;border-radius:6px;background:#000}
#t{font:13px monospace;margin-top:10px;white-space:nowrap;overflow:hidden;width:min(960px,100%)}
.u{color:#3fe07a}.d{color:#ff4f4f}small{color:#6b7896}
</style></head><body>
<h1>MARGIN CALL &middot; LIVE</h1>
<canvas id="c" width="240" height="135"></canvas>
<div id="t">Lade Kurse...</div>
<small id="s">verbinde...</small>
<script>
const W=240,H=135,cv=document.getElementById('c'),cx=cv.getContext('2d'),img=cx.createImageData(W,H);
const R=new Uint8Array(256),G=new Uint8Array(256),B=new Uint8Array(256);
for(let i=0;i<256;i++){R[i]=((i>>5)&7)*255/7;G[i]=((i>>2)&7)*255/7;B[i]=(i&3)*255/3;}
let frames=0,t0=performance.now();
async function loop(){
  try{
    const r=await fetch('/frame',{cache:'no-store'});
    if(r.ok){const b=new Uint8Array(await r.arrayBuffer());
      if(b.length===W*H){const d=img.data;for(let i=0,j=0;i<b.length;i++,j+=4){const c=b[i];d[j]=R[c];d[j+1]=G[c];d[j+2]=B[c];d[j+3]=255;}
        cx.putImageData(img,0,0);frames++;}}
    else await new Promise(s=>setTimeout(s,300));
  }catch(e){document.getElementById('s').textContent='Verbindung unterbrochen...';await new Promise(s=>setTimeout(s,1000));}
  const now=performance.now();
  if(now-t0>1000){document.getElementById('s').textContent=(frames*1000/(now-t0)).toFixed(1)+' FPS';frames=0;t0=now;}
  requestAnimationFrame(loop);
}
async function market(){
  try{const m=await (await fetch('/market',{cache:'no-store'})).json();
    document.getElementById('t').innerHTML=m.map(x=>`<b>${x.l}</b> ${x.p.toLocaleString('de-DE',{maximumFractionDigits:2})} <span class="${x.c>=0?'u':'d'}">${x.c>=0?'+':''}${x.c.toFixed(2)}%</span> ${x.live?'':'(sim)'}`).join(' &nbsp;|&nbsp; ');
  }catch(e){}
  setTimeout(market,15000);
}
loop();market();
</script></body></html>)HTML";

static void touchViewer() {
  uint32_t now = millis();
  lastViewerMs[viewerSlot] = now;
  viewerSlot = (viewerSlot + 1) & 3;
}

static void handleFrame() {
  if (paused) { server.send(503, "text/plain", "busy"); return; }
  if (!snap) snap = (uint8_t*)malloc(FW * FH);
  if (!snap) { server.send(503, "text/plain", "nomem"); return; }
  touchViewer();
  frameReady = false; wantFrame = true;
  uint32_t t0 = millis();
  while (!frameReady && millis() - t0 < 300) vTaskDelay(2);
  wantFrame = false;
  if (!frameReady) { server.send(503, "text/plain", "noframe"); return; }
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(FW * FH);
  server.send(200, "application/octet-stream", "");
  server.sendContent((const char*)snap, FW * FH);
}

static void handleMarket() {
  String j = "[";
  for (int i = 0; i < M_COUNT; i++) {
    char b[96];
    snprintf(b, sizeof(b), "%s{\"l\":\"%s\",\"p\":%.2f,\"c\":%.3f,\"live\":%s}", i ? "," : "",
             markets[i].label, markets[i].price, markets[i].chg, markets[i].live ? "true" : "false");
    j += b;
  }
  j += "]";
  server.send(200, "application/json", j);
}

static void serverTask(void*) {
  for (;;) { server.handleClient(); vTaskDelay(2); }
}

static void streamStart() {
  serverStarted = true;
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/frame", handleFrame);
  server.on("/market", handleMarket);
  server.begin();
  MDNS.begin("margincall");
  MDNS.addService("http", "tcp", 80);
  xTaskCreatePinnedToCore(serverTask, "stream", 6144, nullptr, 1, nullptr, 0);
}

void streamPause(bool p) {
  paused = p;
  if (p) {
    uint32_t t0 = millis();
    while (wantFrame && millis() - t0 < 400) delay(5);
    if (snap) { free(snap); snap = nullptr; }
  }
}

bool streamWantsFrame() { return wantFrame && !frameReady && snap; }

void streamSubmitFrame(const uint16_t* fb, int w, int h) {
  static const uint8_t bayer[4] = {0, 4, 6, 2};   // 2x2 Dither (0..7)
  uint8_t* o = snap;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      uint16_t c = *fb++; c = (c >> 8) | (c << 8);
      int d = bayer[((y & 1) << 1) | (x & 1)];
      int r = ((c >> 11) << 3) + d * 4, g = (((c >> 5) & 63) << 2) + d * 4, b = ((c & 31) << 3) + d * 8;
      if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;
      *o++ = (uint8_t)((r & 0xE0) | ((g >> 3) & 0x1C) | (b >> 6));
    }
  }
  frameReady = true;
}

int streamViewers() {
  uint32_t now = millis(); int n = 0;
  for (int i = 0; i < 4; i++) if (lastViewerMs[i] && now - lastViewerMs[i] < 3000) n++;
  return n > 0 ? 1 : 0;   // grobe Anzeige: 0 = keiner, 1 = mind. einer schaut zu
}
