#include "net.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <mbedtls/sha256.h>

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
    float drift = ((int)(esp_random() % 2001) - 1000) / 1000.0f * 0.0002f;
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


// 5 Tage werden abgefragt; behalten werden nur die letzten "keep" Werte (Ringpuffer), damit der Verlauf
// kurz nach Boersenstart nicht nur aus wenigen Punkten besteht
static bool parseChart(Stream* s, Market& m, int keep) {
  static const char* KEYS[3] = {"\"regularMarketPrice\":", "\"previousClose\":", "\"close\":["};
  static float ring[SERIES_MAX];
  if (keep > SERIES_MAX) keep = SERIES_MAX;
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
          int total = 0;
          do {
            bool ok = readNumber(r, v, term);
            if (ok) ring[total++ % keep] = v;
          } while (term == ',');
          const int cnt = total < keep ? total : keep, start = total - cnt;
          for (int i = 0; i < cnt; i++) m.series[i] = ring[(start + i) % keep];
          m.n = cnt;
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
                   "?range=5d&interval=" + interval;
      if (http.begin(client, url)) {
        int code = http.GET();
        if (code == 200) good = parseChart(http.getStreamPtr(), markets[i], i == M_OIL ? 96 : 78);
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

// ================================================================ Updates
// Stammzertifikate fuer api.github.com/github.com (Sectigo -> USERTrust ECC) und die Download-Server (Let's Encrypt -> ISRG)
static const char GITHUB_ROOTS[] PROGMEM =
"-----BEGIN CERTIFICATE-----\n"
"MIICjzCCAhWgAwIBAgIQXIuZxVqUxdJxVt7NiYDMJjAKBggqhkjOPQQDAzCBiDEL\n"
"MAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNl\n"
"eSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMT\n"
"JVVTRVJUcnVzdCBFQ0MgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAwMjAx\n"
"MDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNVBAgT\n"
"Ck5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVUaGUg\n"
"VVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBFQ0MgQ2VydGlm\n"
"aWNhdGlvbiBBdXRob3JpdHkwdjAQBgcqhkjOPQIBBgUrgQQAIgNiAAQarFRaqflo\n"
"I+d61SRvU8Za2EurxtW20eZzca7dnNYMYf3boIkDuAUU7FfO7l0/4iGzzvfUinng\n"
"o4N+LZfQYcTxmdwlkWOrfzCjtHDix6EznPO/LlxTsV+zfTJ/ijTjeXmjQjBAMB0G\n"
"A1UdDgQWBBQ64QmG1M8ZwpZ2dEl23OA1xmNjmjAOBgNVHQ8BAf8EBAMCAQYwDwYD\n"
"VR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAwNoADBlAjA2Z6EWCNzklwBBHU6+4WMB\n"
"zzuqQhFkoJ2UOQIReVx7Hfpkue4WQrO/isIJxOzksU0CMQDpKmFHjFJKS04YcPbW\n"
"RNZu9YO6bVi9JNlWSOrvxKJGgYhqOkbRqZtNyWHa0V1Xahg=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
"h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
"0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
"A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n"
"T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
"B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
"B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
"KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
"OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
"jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
"qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
"rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
"HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
"hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
"ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
"3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
"NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
"ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
"TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
"jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
"oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
"4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
"mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
"emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
"-----END CERTIFICATE-----\n"
"-----BEGIN CERTIFICATE-----\n"
"MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw\n"
"CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg\n"
"R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00\n"
"MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT\n"
"ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw\n"
"EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW\n"
"+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9\n"
"ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T\n"
"AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI\n"
"zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW\n"
"tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1\n"
"/q4AaOeMSQ+2b1tbFfLn\n"
"-----END CERTIFICATE-----\n";
static const char* const UPDATE_API = "https://api.github.com/repos/wolfgangrabenstein/margincall/releases/latest";
static const char* const UPDATE_PREFIX = "https://github.com/wolfgangrabenstein/margincall/releases/download/";

static bool versionNewer(const char* a, const char* b) {
  int x[3] = {0, 0, 0}, y[3] = {0, 0, 0};
  sscanf(a, "%d.%d.%d", &x[0], &x[1], &x[2]); sscanf(b, "%d.%d.%d", &y[0], &y[1], &y[2]);
  for (int i = 0; i < 3; i++) if (x[i] != y[i]) return x[i] > y[i];
  return false;
}
// Text eines JSON-Feldes ab Position from (einfache Zeichenketten ohne Escapes)
static bool jsonStr(const String& s, int from, const char* key, char* out, size_t n, int* at = nullptr) {
  int k = s.indexOf(key, from); if (k < 0) return false;
  k += strlen(key); int e = s.indexOf('"', k); if (e < 0 || e - k >= (int)n) return false;
  s.substring(k, e).toCharArray(out, n); if (at) *at = e; return true;
}

bool updateCheck(const char* current, FwUpdate& u) {
  if (WiFi.status() != WL_CONNECTED) return false;
  streamPause(true);
  String body;
  {
    WiFiClientSecure client;
    client.setCACert(GITHUB_ROOTS);
    client.setTimeout(10);
    HTTPClient http;
    http.setTimeout(10000);
    http.setUserAgent(String("MarginCall-Cardputer/") + current);
    if (http.begin(client, UPDATE_API)) {
      http.addHeader("Accept", "application/vnd.github+json");
      if (http.GET() == 200 && http.getSize() < 64 * 1024) body = http.getString();
      http.end();
    }
  }
  streamPause(false);
  if (!body.length()) return false;
  memset(&u, 0, sizeof(u));
  char tag[20];
  if (!jsonStr(body, 0, "\"tag_name\":\"", tag, sizeof(tag))) return false;
  strncpy(u.version, tag[0] == 'v' ? tag + 1 : tag, sizeof(u.version) - 1);
  if (!versionNewer(u.version, current)) return false;
  snprintf(u.name, sizeof(u.name), "MarginCall-%s-Cardputer.bin", u.version);
  int at = body.indexOf(String("\"name\":\"") + u.name + "\"");
  if (at < 0) return false;   // Firmware dieser Version (noch) nicht da
  int sz = body.indexOf("\"size\":", at);
  if (sz < 0) return false;
  u.size = (uint32_t)body.substring(sz + 7, sz + 20).toInt();
  if (!jsonStr(body, at, "\"digest\":\"sha256:", u.sha256, sizeof(u.sha256))) return false;
  if (!jsonStr(body, at, "\"browser_download_url\":\"", u.url, sizeof(u.url))) return false;
  if (strlen(u.sha256) != 64 || u.size < 100000 || u.size > 6 * 1024 * 1024 || strncmp(u.url, UPDATE_PREFIX, strlen(UPDATE_PREFIX)) != 0) return false;
  return true;
}

int updateDownload(const FwUpdate& u, void (*progress)(uint32_t, uint32_t)) {
  if (WiFi.status() != WL_CONNECTED) return 1;
  static SPIClass sdSpi(HSPI);
  sdSpi.begin(40, 39, 14, 12);   // SD-Karte des Cardputer: SCK, MISO, MOSI, CS
  if (!SD.begin(12, sdSpi, 25000000)) return 2;
  String path = String("/") + u.name, tmp = path + ".part";
  SD.remove(tmp);
  File f = SD.open(tmp, FILE_WRITE);
  if (!f) { SD.end(); return 3; }
  streamPause(true);
  int err = 4;
  uint32_t got = 0;
  uint8_t hash[32];
  {
    WiFiClientSecure client;
    client.setCACert(GITHUB_ROOTS);
    client.setTimeout(15);
    HTTPClient http;
    http.setTimeout(15000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);   // GitHub leitet zum Download-Server weiter
    http.setUserAgent("MarginCall-Cardputer");
    if (http.begin(client, u.url) && http.GET() == 200) {
      mbedtls_sha256_context sha; mbedtls_sha256_init(&sha); mbedtls_sha256_starts(&sha, 0);
      WiFiClient* st = http.getStreamPtr();
      static uint8_t buf[2048];
      uint32_t last = millis();
      while (got < u.size && millis() - last < 15000) {
        int a = st->available();
        if (a <= 0) { if (!http.connected()) break; delay(2); continue; }
        int n = st->readBytes(buf, min((uint32_t)sizeof(buf), min((uint32_t)a, u.size - got)));
        if (n <= 0) continue;
        if (f.write(buf, n) != (size_t)n) { err = 5; break; }
        mbedtls_sha256_update(&sha, buf, n);
        got += n; last = millis();
        if (progress) progress(got, u.size);
      }
      mbedtls_sha256_finish(&sha, hash); mbedtls_sha256_free(&sha);
      if (got == u.size && err == 4) err = 0;
    }
    http.end();
  }
  streamPause(false);
  f.close();
  if (!err) {   // Pruefsumme vergleichen
    char hex[65]; for (int i = 0; i < 32; i++) sprintf(hex + i * 2, "%02x", hash[i]);
    if (strcmp(hex, u.sha256) != 0) err = 6;
  }
  if (!err) { SD.remove(path); if (!SD.rename(tmp, path)) err = 7; }
  if (err) SD.remove(tmp);
  SD.end();
  return err;
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
