// MARGIN CALL - ein Boersen-Raycaster fuer den M5Stack Cardputer ADV
//
// Steuerung:  E / S = vor / zurueck     A / D = drehen
//             W / R = seitwaerts        Leertaste / Enter = schiessen
//             P     = Pause             M = Musik an/aus
//
// Bullen und Baeren sind allgegenwaertig, bekaempfen dich UND sich gegenseitig.
// Ihr Kraefteverhaeltnis kommt aus den echten Tageskursen (Dow, S&P 500, Nasdaq-100),
// der Oelpreis beeinflusst Tempo und Feuerrate, und die KI ALGO-9000 manipuliert den Markt.

#include <M5Cardputer.h>
#include <math.h>
#include "audio.h"
#include "net.h"
#include <algorithm>
#include <Preferences.h>

// ---------------------------------------------------------------- Konstanten
static constexpr int SW = 240, SH = 135;
static constexpr int VH = 108;          // Hoehe der 3D-Ansicht, darunter HUD
static constexpr int TEX = 32;
static constexpr int MAP_W = 20, MAP_H = 20;
static constexpr int NUM_LEVELS = 5;
static constexpr float PLANE = 0.66f;
static constexpr float PROJ = SW / (2.0f * PLANE);   // quadratische Pixel

// Sprache: Englisch ist Standard, Deutsch per [L] im Titelbild (wird gespeichert). L(en, de) waehlt den Text.
static bool langDe = false;
static inline const char* L(const char* en, const char* de) { return langDe ? de : en; }

static constexpr uint16_t c565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
static constexpr uint16_t swp(uint16_t c) { return (uint16_t)((c >> 8) | (c << 8)); }

M5Canvas canvas(&M5Cardputer.Display);
static uint16_t* fb = nullptr;   // Framebuffer (RGB565, byte-swapped)

// ---------------------------------------------------------------- Texturen
enum { T_MARBLE, T_TICKER, T_SERVER, T_GOLD, T_VAULT, T_CHART, T_PIPES, T_BARRELS, T_AICORE, NUM_WALL };
static uint16_t wallTex[NUM_WALL][TEX * TEX];       // RGB565 (nicht geswappt), Schattierung beim Rendern

enum { S_BEAR0, S_BEAR1, S_SELL0, S_SELL1, S_BUB0, S_BUB1, S_SHARK0, S_SHARK1,
       S_BULL0, S_BULL1, S_BARON0, S_BARON1, S_SLICK0, S_SLICK1, S_BOT0, S_BOT1, S_ALGO0, S_ALGO1,
       S_DEAD, S_PROJ, S_PROJ_OIL, S_LASER, S_CASH, S_DIV, S_PIR0, S_PIR1, S_PIR2, S_PIR3, S_PIR4, S_PIR5, S_NOTE, S_KEY, S_REFI, S_LIQ, NUM_SPR };
static uint16_t sprTex[NUM_SPR][TEX * TEX];          // geswappt, 0 = transparent

static uint32_t tmp[TEX * TEX];                      // RGB888-Arbeitspuffer (0 = transparent)
#define FW_VERSION "3.35.2"   // Spielversion, wird mit jedem Release angehoben
static uint32_t rngState = 0x1234567;
static uint32_t rnd() { rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5; return rngState; }
static float frand() { return (rnd() & 0xFFFF) / 65535.0f; }

static inline uint32_t RGB(int r, int g, int b) {
  r = constrain(r, 0, 255); g = constrain(g, 0, 255); b = constrain(b, 0, 255);
  return 0xFF000000u | (r << 16) | (g << 8) | b;
}
static void tclear(uint32_t c) { for (int i = 0; i < TEX * TEX; i++) tmp[i] = c; }
static void tpx(int x, int y, uint32_t c) { if (x >= 0 && y >= 0 && x < TEX && y < TEX) tmp[y * TEX + x] = c; }
static void trect(int x, int y, int w, int h, uint32_t c) { for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) tpx(i, j, c); }
static void tellipse(float cx, float cy, float rx, float ry, uint32_t c) {
  for (int y = 0; y < TEX; y++) for (int x = 0; x < TEX; x++) {
    float dx = (x + 0.5f - cx) / rx, dy = (y + 0.5f - cy) / ry;
    if (dx * dx + dy * dy <= 1.0f) tpx(x, y, c);
  }
}
static void tcirc(float cx, float cy, float r, uint32_t c) { tellipse(cx, cy, r, r, c); }
static void ttri(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c) {
  for (int y = 0; y < TEX; y++) for (int x = 0; x < TEX; x++) {
    float px = x + 0.5f, py = y + 0.5f;
    float d0 = (x1 - x0) * (py - y0) - (y1 - y0) * (px - x0);
    float d1 = (x2 - x1) * (py - y1) - (y2 - y1) * (px - x1);
    float d2 = (x0 - x2) * (py - y2) - (y0 - y2) * (px - x2);
    bool neg = d0 < 0 || d1 < 0 || d2 < 0, pos = d0 > 0 || d1 > 0 || d2 > 0;
    if (!(neg && pos)) tpx(x, y, c);
  }
}
static void tline(int x0, int y0, int x1, int y1, uint32_t c, int th = 1) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, err = dx + dy;
  for (;;) {
    trect(x0, y0, th, th, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

static void storeWall(int idx) {
  for (int i = 0; i < TEX * TEX; i++) {
    uint32_t c = tmp[i];
    wallTex[idx][i] = c565((c >> 16) & 255, (c >> 8) & 255, c & 255);
  }
}
static void storeSprite(int idx);
// Keycard des Levels: jede Tresortuer braucht ihre eigene Farbe
static const uint8_t KEY_COL[5][3] = {{255, 210, 40}, {60, 140, 255}, {235, 50, 50}, {50, 205, 90}, {175, 80, 235}};
static const char* const KEY_EN[5] = {"YELLOW", "BLUE", "RED", "GREEN", "PURPLE"};
static const char* const KEY_DE[5] = {"GELBE", "BLAUE", "ROTE", "GRUENE", "VIOLETTE"};
static void makeKeySprite(int lv);
static void storeSprite(int idx) {
  for (int i = 0; i < TEX * TEX; i++) {
    uint32_t c = tmp[i];
    if (!(c >> 24)) { sprTex[idx][i] = 0; continue; }
    uint16_t v = swp(c565((c >> 16) & 255, (c >> 8) & 255, c & 255));
    sprTex[idx][i] = v ? v : swp(0x0841);  // echtes Schwarz != transparent
  }
}
static void makeKeySprite(int lv) {
  const uint8_t* k = KEY_COL[lv % 5];
  uint32_t dark = RGB(k[0] / 2, k[1] / 2, k[2] / 2);
  tclear(0);
  trect(5, 14, 22, 16, dark); trect(6, 15, 20, 14, RGB(k[0], k[1], k[2]));
  trect(6, 18, 20, 3, RGB(240, 240, 245)); trect(9, 23, 6, 4, RGB(230, 185, 60)); trect(10, 24, 4, 2, RGB(170, 130, 30));
  trect(18, 24, 6, 1, dark); trect(18, 26, 4, 1, dark);
  tline(16, 14, 16, 8, RGB(40, 40, 50), 1); tcirc(16, 7, 2, RGB(40, 40, 50)); tcirc(16, 7, 1, 0);
  storeSprite(S_KEY);
}

// ALGO-Manipulation, mit der die Wand-Charts zuletzt gezeichnet wurden (Prozentpunkte)
static float manipDrawn = 0;
// Von ALGO-9000 verbogener Verlauf: ab 60 % des Tages schrittweise auf den manipulierten Kurs
static float manipSeries(const Market& m, int i) {
  int a = m.n * 6 / 10;
  return m.series[i] * (1.0f + (i > a ? manipDrawn / 100.0f * (i - a) / max(1, m.n - 1 - a) : 0.0f));
}
static void seriesRange(const Market& m, float& lo, float& hi) {
  lo = 1e30f; hi = -1e30f;
  for (int i = 0; i < m.n; i++) { lo = min(lo, m.series[i]); hi = max(hi, m.series[i]); }
  if (manipDrawn != 0 && m.n > 0) { float v = manipSeries(m, m.n - 1); lo = min(lo, v); hi = max(hi, v); }
  if (hi - lo < 1e-6f) hi = lo + 1;
}
static const uint32_t MANIP_COL = RGB(255, 60, 200);

// Ticker-Wand: LED-Laufband + echter Intraday-Chart des Level-Markts
static void genTickerTex(const Market& m) {
  tclear(RGB(8, 12, 22));
  bool up = m.chg >= 0;
  for (int x = 0; x < TEX; x++)
    for (int y = 2; y < 9; y++) if ((x % 8) < 6 && (rnd() % 3)) tpx(x, y, ((x / 8) % 3 == 2) != up ? RGB(40, 230, 90) : RGB(240, 50, 50));
  float lo, hi; seriesRange(m, lo, hi);
  for (int x = 0; x < TEX; x++) {
    int i = m.n > 1 ? x * (m.n - 1) / (TEX - 1) : 0;
    int y = 29 - (int)((m.series[i] - lo) / (hi - lo) * 15);
    uint32_t line = up ? RGB(80, 255, 130) : RGB(255, 80, 80), fill = up ? RGB(10, 70, 35) : RGB(80, 15, 15);
    for (int yy = y + 1; yy < 31; yy++) tpx(x, yy, fill);
    tpx(x, y, line);
  }
  int py = 29 - (int)((m.prev - lo) / (hi - lo) * 15);   // Vortages-Schluss gestrichelt
  if (py >= 13 && py <= 30) for (int x = 0; x < TEX; x += 3) tpx(x, py, RGB(200, 200, 90));
  if (manipDrawn != 0)
    for (int x = TEX * 6 / 10; x < TEX; x++) {
      int i = m.n > 1 ? x * (m.n - 1) / (TEX - 1) : 0;
      tpx(x, constrain(29 - (int)((manipSeries(m, i) - lo) / (hi - lo) * 15), 12, 30), MANIP_COL);
    }
  for (int i = 0; i < TEX; i++) { tpx(i, 0, RGB(90, 90, 100)); tpx(i, 11, RGB(40, 40, 60)); tpx(i, 31, RGB(90, 90, 100)); }
  storeWall(T_TICKER);
}

// Candlestick-Tafel aus echten Kursen (7 Kerzen)
static void genChartTex(const Market& m) {
  tclear(RGB(225, 228, 235));
  for (int y = 4; y < 30; y += 6) trect(1, y, 30, 1, RGB(200, 205, 215));
  float lo, hi; seriesRange(m, lo, hi);
  auto Y = [&](float v) { return 27 - (int)((v - lo) / (hi - lo) * 22); };
  const int K = 7;
  for (int k = 0; k < K; k++) {
    int a = k * m.n / K, b = max(a + 1, (k + 1) * m.n / K);
    float o = m.series[a], c = m.series[b - 1], h = o, l = o;
    for (int i = a; i < b; i++) { h = max(h, m.series[i]); l = min(l, m.series[i]); }
    int x = 2 + k * 4;
    uint32_t col = c >= o ? RGB(20, 170, 60) : RGB(220, 40, 40);
    trect(x + 1, Y(h), 1, Y(l) - Y(h) + 1, RGB(60, 60, 60));
    int t = Y(max(o, c)), bt = Y(min(o, c)); if (bt - t < 1) bt = t + 1;
    trect(x, t, 3, bt - t, col);
  }
  if (manipDrawn != 0)   // manipulierter Verlauf, doppelt so dick
    for (int x = 17; x < 31; x++) {
      int i = min(m.n - 1, (x - 1) * m.n / 30), y = constrain(Y(manipSeries(m, i)), 1, 29);
      tpx(x, y, MANIP_COL); tpx(x, y + 1, MANIP_COL);
    }
  for (int i = 0; i < TEX; i++) { tpx(i, 0, RGB(50, 50, 60)); tpx(i, 31, RGB(50, 50, 60)); tpx(0, i, RGB(50, 50, 60)); tpx(31, i, RGB(50, 50, 60)); }
  storeWall(T_CHART);
}

static void genWalls() {
  // Marmor der Schalterhalle
  for (int y = 0; y < TEX; y++) for (int x = 0; x < TEX; x++) {
    int n = (int)(frand() * 24) - 12;
    float v = sinf(x * 0.22f + y * 0.13f + 2.5f * sinf(y * 0.27f + x * 0.05f));
    int base = 196 + n;
    uint32_t c = RGB(base, base - 8, base - 26);
    if (fabsf(v) < 0.10f) c = RGB(120 + n, 115 + n, 110 + n);
    if (y % 16 == 0 || ((x + (y / 16) * 16) % 32) == 0) c = RGB(140, 130, 112);
    tmp[y * TEX + x] = c;
  }
  storeWall(T_MARBLE);

  // Serverschrank (Hochfrequenzhandel)
  tclear(RGB(42, 44, 50));
  for (int y = 0; y < TEX; y += 4) trect(1, y, 30, 1, RGB(20, 20, 24));
  for (int y = 1; y < TEX; y += 4) {
    for (int x = 3; x < 20; x += 2) tpx(x, y + 1, RGB(28, 28, 32));
    static const uint32_t led[3] = {RGB(40, 255, 80), RGB(60, 140, 255), RGB(255, 180, 30)};
    for (int x = 23; x < 30; x += 2) if (rnd() % 2) tpx(x, y + 1, led[rnd() % 3]);
  }
  trect(0, 0, 1, 32, RGB(80, 80, 90)); trect(31, 0, 1, 32, RGB(15, 15, 18));
  storeWall(T_SERVER);

  // Goldbarren
  for (int y = 0; y < TEX; y++) for (int x = 0; x < TEX; x++) {
    int row = y / 8, ly = y % 8, lx = (x + (row % 2) * 8) % 16;
    uint32_t c;
    if (ly == 7 || lx == 15) c = RGB(80, 50, 10);
    else { int b = 255 - ly * 14 - (int)(frand() * 10); c = RGB(b, b * 3 / 4, 30); if (ly == 0) c = RGB(255, 240, 150); }
    tmp[y * TEX + x] = c;
  }
  storeWall(T_GOLD);

  // Tresortuer = Ausgang
  for (int y = 0; y < TEX; y++) for (int x = 0; x < TEX; x++) { int n = rnd() % 12; tmp[y * TEX + x] = RGB(110 + n, 115 + n, 122 + n); }
  tcirc(16, 17, 13, RGB(70, 74, 80));
  tcirc(16, 17, 11, RGB(150, 155, 162));
  tcirc(16, 17, 4, RGB(60, 62, 68));
  tline(16, 17, 16, 8, RGB(220, 200, 80), 2); tline(16, 17, 8, 22, RGB(220, 200, 80), 2); tline(16, 17, 24, 22, RGB(220, 200, 80), 2);
  for (int a = 0; a < 8; a++) tpx(16 + (int)(12 * cosf(a * 0.785f)), 17 + (int)(12 * sinf(a * 0.785f)), RGB(40, 40, 40));
  trect(13, 1, 6, 2, RGB(40, 255, 90));
  storeWall(T_VAULT);

  // Rohrleitungen der Raffinerie
  for (int y = 0; y < TEX; y++) for (int x = 0; x < TEX; x++) { int n = rnd() % 14; tmp[y * TEX + x] = RGB(62 + n, 48 + n / 2, 36); }
  for (int p = 0; p < 2; p++) {
    int y0 = p ? 18 : 4;
    for (int y = 0; y < 9; y++) {
      float s = sinf((y + 0.5f) / 9 * 3.1416f);
      int g = 50 + (int)(130 * s);
      trect(0, y0 + y, 32, 1, p ? RGB(g, g * 2 / 3, g / 3) : RGB(g * 3 / 4, g * 3 / 4, g * 4 / 5));
    }
    for (int fx : {7, 23}) { trect(fx, y0 - 1, 3, 11, RGB(40, 40, 44)); tpx(fx + 1, y0 + 1, RGB(200, 200, 200)); tpx(fx + 1, y0 + 7, RGB(200, 200, 200)); }
  }
  for (int i = 0; i < 6; i++) { int x = rnd() % 32; trect(x, 13 + rnd() % 3, 1, 2 + rnd() % 4, RGB(10, 10, 12)); }   // Oeltropfen
  storeWall(T_PIPES);

  // Gestapelte Oelfaesser
  tclear(RGB(20, 18, 16));
  for (int bx = 0; bx < 2; bx++) for (int by = 0; by < 2; by++) {
    int x0 = bx * 16 + 1, y0 = by * 16 + 1;
    for (int x = 0; x < 14; x++) {
      float s = sinf((x + 0.5f) / 14 * 3.1416f);
      uint32_t c = (bx + by) % 2 ? RGB(30 + 60 * s, 40 + 70 * s, 110 + 90 * s) : RGB(120 + 110 * s, 25 + 30 * s, 20 + 20 * s);
      trect(x0 + x, y0, 1, 14, c);
    }
    trect(x0, y0 + 3, 14, 1, RGB(15, 15, 15)); trect(x0, y0 + 10, 14, 1, RGB(15, 15, 15));
    tcirc(x0 + 7, y0 + 6.5f, 2, RGB(250, 210, 40)); tpx(x0 + 7, y0 + 6, RGB(10, 10, 10));
  }
  storeWall(T_BARRELS);

  // KI-Kern: Matrix-Code + rotes Auge
  tclear(RGB(4, 8, 14));
  for (int x = 1; x < TEX; x += 2) {
    int start = rnd() % 32, len = 6 + rnd() % 14;
    for (int i = 0; i < len; i++) {
      int y = (start + i) % 32;
      if (rnd() % 3) tpx(x, y, i == len - 1 ? RGB(200, 255, 220) : RGB(20, 120 + i * 6, 70 + i * 4));
    }
  }
  tcirc(16, 16, 7, RGB(40, 40, 48));
  tcirc(16, 16, 5.5f, RGB(120, 0, 0));
  tcirc(16, 16, 3.5f, RGB(255, 30, 20));
  tcirc(16, 16, 1.5f, RGB(255, 230, 120));
  storeWall(T_AICORE);

  genTickerTex(markets[M_DOW]);
  genChartTex(markets[M_DOW]);
}

static void genSprites() {
  const uint32_t T = 0;
  // --- Baer (Bear Market)
  for (int f = 0; f < 2; f++) {
    tclear(T);
    uint32_t fur = RGB(110, 70, 38), dark = RGB(70, 42, 20);
    tellipse(16, 21, 10, 9, fur);
    tcirc(10, 4, 3, fur); tcirc(22, 4, 3, fur); tcirc(10, 4, 1.5f, RGB(200, 120, 120)); tcirc(22, 4, 1.5f, RGB(200, 120, 120));
    tcirc(16, 10, 7, fur);
    tellipse(16, 13, 3.5f, 2.5f, RGB(175, 125, 85));
    trect(15, 11, 3, 2, RGB(0, 0, 0));
    trect(12, 8, 2, 2, RGB(255, 30, 30)); trect(19, 8, 2, 2, RGB(255, 30, 30));
    trect(11, 7, 4, 1, dark); trect(18, 7, 4, 1, dark);
    trect(15, 17, 3, 5, RGB(230, 30, 30)); ttri(12, 22, 21, 22, 16.5f, 27, RGB(230, 30, 30));
    trect(9, 28, 5, 4, dark); trect(18, 28, 5, 4, dark);
    if (f == 0) { trect(4, 17, 4, 9, fur); trect(24, 17, 4, 9, fur); }
    else {
      trect(3, 9, 4, 10, fur); trect(25, 9, 4, 10, fur);
      for (int i = 0; i < 3; i++) { tpx(3 + i, 8, RGB(240, 240, 240)); tpx(26 + i, 8, RGB(240, 240, 240)); }
    }
    storeSprite(S_BEAR0 + f);
  }
  // --- Bulle (Bull Market)
  for (int f = 0; f < 2; f++) {
    tclear(T);
    uint32_t hide = RGB(55, 38, 26), dark = RGB(30, 20, 14), gold = RGB(245, 200, 60);
    int hy = f ? 3 : 0;   // Kopf gesenkt beim Angriff
    tellipse(16, 21, 12, 8, hide);
    trect(6, 26, 4, 6, dark); trect(12, 27, 3, 5, dark); trect(18, 27, 3, 5, dark); trect(23, 26, 4, 6, dark);
    trect(15, 21, 3, 5, RGB(30, 210, 70)); ttri(12, 21, 21, 21, 16.5f, 16.5f, RGB(30, 210, 70));   // Aufwaerts-Pfeil auf dem Koerper, der Kopf liegt davor
    ttri(10, 8 + hy, 2, 1 + hy, 7, 11 + hy, gold); ttri(22, 8 + hy, 30, 1 + hy, 25, 11 + hy, gold);
    tcirc(16, 10 + hy, 6.5f, hide);
    tellipse(16, 15 + hy, 4.5f, 3, RGB(190, 130, 110));
    tpx(14, 15 + hy, RGB(0, 0, 0)); tpx(18, 15 + hy, RGB(0, 0, 0));
    static const int8_t RING[7][2] = {{14, 17}, {18, 17}, {14, 18}, {18, 18}, {15, 19}, {16, 19}, {17, 19}};   // Nasenring: goldener Bogen unter dem Maul, oben steckt er in der Nase
    for (auto& r : RING) tpx(r[0], r[1] + hy, gold);
    tpx(15, 19 + hy, RGB(170, 120, 20)); tpx(17, 19 + hy, RGB(170, 120, 20)); tpx(14, 17 + hy, RGB(255, 240, 170));   // Schatten und Glanzpunkt
    trect(12, 8 + hy, 2, 2, RGB(255, 255, 255)); trect(19, 8 + hy, 2, 2, RGB(255, 255, 255));
    tpx(13, 9 + hy, RGB(200, 0, 0)); tpx(19, 9 + hy, RGB(200, 0, 0));
    if (f) { tcirc(9, 17 + hy, 2, RGB(230, 230, 230)); tcirc(23, 17 + hy, 2, RGB(230, 230, 230)); tcirc(6, 18 + hy, 1.5f, RGB(200, 200, 200)); }
    storeSprite(S_BULL0 + f);
  }
  // --- Short Seller im Anzug
  for (int f = 0; f < 2; f++) {
    tclear(T);
    uint32_t suit = RGB(30, 32, 55), skin = RGB(230, 188, 150);
    trect(9, 14, 14, 14, suit);
    ttri(13, 14, 19, 14, 16, 21, RGB(240, 240, 240));
    trect(15, 15, 2, 7, RGB(220, 20, 30));
    trect(10, 28, 5, 4, RGB(20, 20, 30)); trect(17, 28, 5, 4, RGB(20, 20, 30));
    tcirc(16, 8, 5, skin);
    trect(11, 2, 10, 3, RGB(25, 20, 15));
    trect(12, 7, 9, 2, RGB(0, 0, 0));
    trect(14, 11, 4, 1, RGB(150, 60, 60));
    trect(6, 15, 3, 11, suit);
    trect(3, 23, 7, 5, RGB(110, 70, 30)); trect(5, 22, 3, 1, RGB(60, 40, 20));
    if (f == 0) trect(23, 15, 3, 11, suit);
    else {
      trect(23, 15, 7, 3, suit); trect(28, 14, 2, 2, skin);
      tcirc(29, 12, 3, RGB(255, 140, 0)); tcirc(29, 12, 1.5f, RGB(255, 255, 160));
    }
    storeSprite(S_SELL0 + f);
  }
  // --- Krypto-Blase: Bitcoin-Muenze in einer stacheligen Blase, gruen und rot fluoreszierend (Gefahr, kein Bonus)
  for (int f = 0; f < 2; f++) {
    tclear(T);
    const float r = f ? 11.5f : 10.5f;
    const uint32_t bc = RGB(150, 90, 0), spike = f ? RGB(150, 20, 35) : RGB(30, 140, 75);   // Stacheln gruen, im Angriff rot
    for (int k = 0; k < 10; k++) {   // Stacheln rundherum
      const float an = k * 3.14159265f / 5 + (f ? 0.31f : 0), c = cosf(an), sn = sinf(an);
      ttri(16 + c * (r - 1) - sn * 2.2f, 16 + sn * (r - 1) + c * 2.2f, 16 + c * (r - 1) + sn * 2.2f, 16 + sn * (r - 1) - c * 2.2f, 16 + c * (r + 4), 16 + sn * (r + 4), spike);
    }
    tcirc(16, 16, r, f ? RGB(255, 90, 70) : RGB(60, 210, 130));   // gruen, pulsiert rot
    tcirc(16, 16, r - 1.5f, f ? RGB(150, 25, 40) : RGB(25, 110, 70));
    tcirc(16, 16, 6.5f, RGB(245, 180, 30));
    tcirc(16, 16, 5, RGB(255, 210, 70));
    trect(14, 12, 2, 9, bc); trect(14, 12, 4, 1, bc); trect(14, 16, 4, 1, bc); trect(14, 20, 4, 1, bc);
    trect(18, 13, 1, 3, bc); trect(18, 17, 1, 3, bc); tpx(15, 11, bc); tpx(15, 21, bc);
    tcirc(10, 10, 1.6f, f ? RGB(255, 200, 200) : RGB(255, 255, 255));
    storeSprite(S_BUB0 + f);
  }
  // --- Hedgefonds-Hai (Boss Level 3)
  for (int f = 0; f < 2; f++) {
    tclear(T);
    uint32_t grey = RGB(105, 118, 140);
    ttri(16, 1, 10, 12, 22, 12, grey);
    tellipse(16, 18, 14, 9, grey);
    tellipse(16, 22, 10, 4.5f, RGB(230, 235, 240));
    tcirc(9, 15, 2, RGB(0, 0, 0)); tpx(9, 14, RGB(255, 40, 40));
    tcirc(23, 15, 2, RGB(0, 0, 0)); tpx(23, 14, RGB(255, 40, 40));
    int mh = f ? 6 : 3;
    trect(8, 20, 16, mh, RGB(60, 0, 10));
    for (int x = 8; x < 24; x += 3) { ttri(x, 20, x + 3, 20, x + 1.5f, 22, RGB(255, 255, 255)); ttri(x, 20 + mh, x + 3, 20 + mh, x + 1.5f, 18 + mh, RGB(255, 255, 255)); }
    trect(12, 27, 8, 3, RGB(25, 25, 40));
    trect(15, 27, 2, 5, RGB(220, 20, 30));
    trect(5, 25, 4, 5, RGB(255, 210, 50));
    storeSprite(S_SHARK0 + f);
  }
  // --- Oelbaron (Zylinder, Monokel, Zigarre)
  for (int f = 0; f < 2; f++) {
    tclear(T);
    uint32_t black = RGB(18, 18, 22), skin = RGB(235, 195, 160), gold = RGB(240, 200, 60);
    trect(11, 0, 10, 6, black); trect(11, 4, 10, 1, RGB(140, 20, 20)); trect(8, 6, 16, 2, black);
    tcirc(16, 11, 5, skin);
    tcirc(19, 10, 1.8f, gold); tpx(19, 10, RGB(200, 230, 255));
    tpx(13, 10, RGB(0, 0, 0));
    trect(12, 13, 8, 1, RGB(200, 200, 200));
    trect(20, 14, 6, 1, RGB(130, 80, 40)); tpx(26, 14, RGB(255, 80, 0));
    tellipse(16, 23, 9, 8, black);
    trect(13, 17, 6, 10, gold);
    trect(15, 17, 2, 4, RGB(255, 255, 255));
    tline(13, 22, 18, 25, RGB(255, 240, 150));
    trect(11, 29, 4, 3, black); trect(17, 29, 4, 3, black);
    if (f) { trect(24, 15, 7, 9, RGB(20, 20, 20)); trect(24, 17, 7, 1, RGB(250, 210, 40)); trect(24, 21, 7, 1, RGB(250, 210, 40)); }
    else trect(5, 20, 3, 8, black);
    storeSprite(S_BARON0 + f);
  }
  // --- Oelschlick-Monster
  for (int f = 0; f < 2; f++) {
    tclear(T);
    tellipse(16, 25, 14, 7, RGB(14, 14, 18));
    tellipse(16, 20, 9, 7, RGB(20, 20, 26));
    for (int i = 0; i < 18; i++) { int x = 6 + rnd() % 20, y = 18 + rnd() % 10; tpx(x, y, (i % 3) ? RGB(60, 30, 90) : RGB(20, 90, 100)); }
    tcirc(13, 19, 1.8f, RGB(255, 220, 40)); tcirc(19, 19, 1.8f, RGB(255, 220, 40));
    tpx(13, 19, RGB(0, 0, 0)); tpx(19, 19, RGB(0, 0, 0));
    trect(12, 23, 8, 1, RGB(120, 20, 20));
    if (f) { tcirc(10, 13, 2.5f, RGB(20, 20, 26)); tcirc(21, 11, 2, RGB(20, 20, 26)); tcirc(16, 10, 1.5f, RGB(30, 30, 38)); }
    storeSprite(S_SLICK0 + f);
  }
  // --- Trading-Bot (Drohne)
  for (int f = 0; f < 2; f++) {
    tclear(T);
    uint32_t body = RGB(150, 160, 178);
    trect(15, 1, 2, 5, RGB(90, 90, 100)); tpx(15, 0, RGB(255, 40, 40));
    trect(f ? 1 : 3, 7, 9, 2, RGB(120, 230, 255)); trect(f ? 22 : 20, 7, 9, 2, RGB(120, 230, 255));
    trect(9, 8, 3, 3, RGB(80, 80, 90)); trect(20, 8, 3, 3, RGB(80, 80, 90));
    tcirc(16, 15, 8, body);
    trect(10, 11, 12, 8, RGB(10, 14, 20));
    tcirc(16, 15, f ? 3 : 2, RGB(255, 40, 30));
    tpx(16, 15, RGB(255, 230, 200));
    trect(14, 23, 4, 4, RGB(90, 95, 110));
    if (f) tcirc(16, 28, 2.5f, RGB(120, 255, 255));
    storeSprite(S_BOT0 + f);
  }
  // --- ALGO-9000 (KI-Monolith, Endboss)
  for (int f = 0; f < 2; f++) {
    tclear(T);
    trect(6, 0, 20, 32, RGB(16, 16, 24));
    trect(6, 0, 1, 32, RGB(90, 90, 110)); trect(25, 0, 1, 32, RGB(90, 90, 110));
    for (int y = 22; y < 31; y += 2) for (int x = 9; x < 23; x += 2) if (rnd() % 2) tpx(x, y, f ? RGB(80, 255, 255) : RGB(40, 200, 90));
    tcirc(16, 11, 7, RGB(60, 60, 70));
    tcirc(16, 11, 5.8f, RGB(90, 0, 0));
    tcirc(16, 11, f ? 5 : 3.8f, RGB(255, 30, 20));
    tcirc(16, 11, f ? 2.5f : 1.6f, RGB(255, 220, 120));
    tpx(14, 9, RGB(255, 255, 255));
    trect(9, 19, 14, 1, RGB(200, 200, 210));
    storeSprite(S_ALGO0 + f);
  }
  // --- Toter Gegner: abgestuerzter Chart
  tclear(T);
  trect(3, 27, 26, 5, RGB(70, 70, 78));
  tline(2, 14, 9, 18, RGB(240, 40, 40), 2); tline(9, 18, 14, 16, RGB(240, 40, 40), 2); tline(14, 16, 22, 25, RGB(240, 40, 40), 2); tline(22, 25, 28, 27, RGB(240, 40, 40), 2);
  ttri(24, 22, 30, 28, 23, 29, RGB(240, 40, 40));
  storeSprite(S_DEAD);
  // --- Projektile
  tclear(T);
  tcirc(16, 16, 10, RGB(120, 0, 0));
  trect(13, 7, 6, 10, RGB(255, 50, 50)); ttri(8, 16, 24, 16, 16, 26, RGB(255, 50, 50));
  trect(15, 8, 2, 8, RGB(255, 200, 200));
  storeSprite(S_PROJ);
  tclear(T);
  trect(8, 6, 16, 20, RGB(25, 25, 28)); trect(8, 10, 16, 2, RGB(250, 210, 40)); trect(8, 20, 16, 2, RGB(250, 210, 40));
  trect(10, 6, 3, 20, RGB(70, 70, 80));
  storeSprite(S_PROJ_OIL);
  tclear(T);
  tellipse(16, 16, 12, 6, RGB(0, 90, 140)); tellipse(16, 16, 9, 3.5f, RGB(80, 230, 255)); tellipse(16, 16, 5, 1.5f, RGB(230, 255, 255));
  storeSprite(S_LASER);
  // --- Cash-Buendel
  tclear(T);
  for (int i = 0; i < 3; i++) { trect(5, 16 + i * 4, 22, 9, RGB(30, 110, 45)); trect(6, 17 + i * 4, 20, 7, RGB(90, 180, 95)); }
  tcirc(16, 28, 3, RGB(200, 240, 200));
  trect(15, 25, 2, 7, RGB(20, 90, 30));
  trect(12, 21, 8, 2, RGB(250, 210, 60));
  storeSprite(S_CASH);
  // --- Dividende
  tclear(T);   // Muenzstapel mit grossem gruenen Kurspfeil nach oben
  for (int i = 0; i < 3; i++) { tellipse(16, 28 - i * 4, 9, 3.2f, RGB(150, 95, 10)); tellipse(16, 27 - i * 4, 8, 2.4f, RGB(255, 200, 50)); }
  tellipse(13, 19, 2, 0.8f, RGB(255, 250, 210));
  ttri(5, 14, 27, 14, 16, 2, RGB(10, 80, 30)); trect(11, 13, 10, 9, RGB(10, 80, 30));
  ttri(8, 13, 24, 13, 16, 4.5f, RGB(60, 230, 90)); trect(13, 12, 6, 9, RGB(60, 230, 90)); trect(14, 8, 2, 11, RGB(190, 255, 200));
  storeSprite(S_DIV);
  // --- Refinanzierungspaket: Paket mit gruenem Band und goldenem Siegel mit Plus
  tclear(T);
  trect(6, 13, 20, 16, RGB(120, 80, 35)); trect(7, 14, 18, 14, RGB(205, 150, 75));
  trect(14, 13, 4, 16, RGB(30, 170, 70)); trect(6, 19, 20, 4, RGB(30, 170, 70));
  tellipse(12, 11, 4, 2.5f, RGB(30, 170, 70)); tellipse(20, 11, 4, 2.5f, RGB(30, 170, 70)); tcirc(16, 12, 1.5f, RGB(20, 120, 50));
  tcirc(16, 21, 3.5f, RGB(250, 205, 50)); trect(15, 19, 2, 4, RGB(255, 255, 255)); trect(14, 20, 4, 2, RGB(255, 255, 255));
  storeSprite(S_REFI);
  // --- Liquiditaetsschub: schwarzer Aktenkoffer, aus dem Geldscheine quellen
  tclear(T);
  { uint32_t C = RGB(45, 45, 58), CL = RGB(100, 100, 120), GOLD = RGB(250, 205, 50), GOLDD = RGB(170, 120, 20), BILL = RGB(120, 205, 120), BILLD = RGB(25, 95, 40), BAND = RGB(250, 210, 60);
  trect(12, 7, 8, 2, CL); trect(12, 7, 1, 4, CL); trect(19, 7, 1, 4, CL);   // Griff
  static const int8_t BUND[3][3] = {{6, 10, 9}, {15, 9, 10}, {10, 11, 11}};   // Geldscheine quellen heraus
  for (auto& b : BUND) { trect(b[0], b[1], b[2], 3, BILLD); trect(b[0], b[1], b[2] - 1, 2, BILL); trect(b[0] + b[2] / 2 - 1, b[1], 2, 3, BAND); }
  trect(3, 13, 26, 15, C); trect(3, 13, 26, 1, CL); trect(3, 13, 1, 15, CL); trect(3, 19, 26, 1, CL);
  trect(8, 18, 3, 3, GOLD); trect(21, 18, 3, 3, GOLD); tpx(9, 19, GOLDD); tpx(22, 19, GOLDD); }
  storeSprite(S_LIQ);
  // --- fliegender Geldschein (Projektil der Waffe)
  tclear(T);
  trect(5, 11, 22, 11, RGB(25, 95, 40)); trect(6, 12, 20, 9, RGB(120, 205, 120)); tcirc(16, 16.5f, 3, RGB(25, 95, 40)); tcirc(16, 16.5f, 1.6f, RGB(170, 235, 170));
  trect(7, 13, 3, 2, RGB(25, 95, 40)); trect(22, 18, 3, 2, RGB(25, 95, 40));
  storeSprite(S_NOTE);
  makeKeySprite(0);
  // Piraten der Steueroase: 3 Typen (Kapitaen, Matrose, Freibeuter), je 2 Bilder (Laufen, Saebel erhoben)
  for (int k = 0; k < 3; k++) for (int f = 0; f < 2; f++) {
    tclear(0);
    uint32_t skin = k == 2 ? RGB(165, 110, 70) : RGB(235, 190, 150), boot = RGB(40, 28, 20), black = RGB(15, 15, 18), steel = RGB(205, 210, 220), gold = RGB(255, 200, 40);
    uint32_t shirt = k == 0 ? RGB(170, 30, 35) : k == 1 ? RGB(235, 235, 235) : RGB(230, 220, 200);
    uint32_t pants = k == 0 ? RGB(40, 35, 60) : k == 1 ? RGB(30, 30, 35) : RGB(110, 80, 50);
    trect(11, 21, 4, 8, pants); trect(17, 21, 4, 8, pants);
    trect(10, 28, 5, 3, boot); trect(17, 28, 5, 3, boot);
    trect(10, 12, 12, 10, shirt);
    if (k == 0) { trect(9, 12, 4, 12, shirt); trect(19, 12, 4, 12, shirt); trect(13, 12, 6, 10, RGB(240, 230, 210)); trect(9, 20, 14, 2, black); trect(15, 20, 2, 2, gold); }
    if (k == 1) for (int y = 13; y < 22; y += 3) trect(10, y, 12, 1, RGB(200, 30, 40));
    if (k == 2) { trect(10, 12, 3, 10, RGB(25, 45, 100)); trect(19, 12, 3, 10, RGB(25, 45, 100)); }
    tcirc(16, 8, 4.5, skin);
    trect(13, 7, 2, 2, black);
    if (k == 1) { trect(17, 6, 3, 3, black); tline(11, 5, 21, 7, black); } else trect(18, 7, 2, 2, black);
    if (k == 0) tellipse(16, 11.5, 4, 2.2, RGB(90, 55, 30)); else trect(15, 11, 3, 1, RGB(120, 40, 40));
    if (k == 0) { ttri(7, 5, 25, 5, 16, -2, black); trect(9, 2, 14, 4, RGB(25, 25, 30)); tcirc(16, 3, 1.3, RGB(240, 240, 240)); }
    if (k == 1) { tellipse(16, 4.5, 5, 2.5, RGB(200, 30, 40)); ttri(20, 4, 25, 3, 23, 8, RGB(200, 30, 40)); }
    if (k == 2) { tellipse(16, 4.5, 5, 2.5, RGB(40, 140, 60)); tcirc(11.5, 9.5, 1, gold); }
    uint32_t arm = k == 1 ? RGB(235, 235, 235) : k == 2 ? skin : shirt;
    trect(6, 13, 4, 8, arm); tcirc(8, 21.5, 1.6, skin);
    if (!f) {
      trect(22, 13, 4, 8, arm);
      if (k == 2) { tline(24, 21, 26, 24, steel); tline(26, 24, 24, 26, steel); } else tcirc(24, 21.5, 1.6, skin);
      tline(25, 22, 30, 29, steel, 2);
    } else {
      trect(22, 5, 4, 9, arm); tcirc(24, 4.5, 1.6, skin);
      tline(24, 4, 30, -2, steel, 2); trect(22, 3, 5, 1, gold);
    }
    storeSprite(S_PIR0 + k * 2 + f);
  }
}

// ---------------------------------------------------------------- Level
// Waende: # Marmor  T Ticker  R Server  G Gold  X Tresor(Ausgang)  C Chart  O Rohre  B Faesser  I KI-Kern
// Figuren: P Start  A Bulle/Baer (nach Live-Markt)  u Bulle  b Baer  s Short Seller  c Krypto-Blase
//          H Hedgefonds-Hai  o Oelbaron  l Oelschlick  t Trading-Bot  K ALGO-9000   Items: $ Cash  + Dividende
static const char* const LEVELS[NUM_LEVELS][MAP_H] = {
  { // 1: Dow Jones - Schalterhalle
    "####################",
    "#P...#......#......#",
    "#....#..A...C..$...#",
    "#..+.........#.....#",
    "#....#......#...s..#",
    "##.###..##..##.#####",
    "#.......##.........#",
    "#..A...............#",
    "#......GG...C##C...#",
    "#.$....GG...#..#.A.#",
    "##.##.......#..#...#",
    "#....#..+.......$..#",
    "#.A..#.....u.......#",
    "#....#######..######",
    "#..................#",
    "#..c....#..#...A...#",
    "#.......#..#.......X",
    "#..$....#..#..b..+.#",
    "#.......#..#.......#",
    "####################",
  },
  { // 2: S&P 500 - Handelsparkett
    "TTTTTTTTTTTTTTTTTTTT",
    "T..s....T......A...T",
    "T.P.....T..........T",
    "T.......T...CC...$.T",
    "T..TT.......CC.....T",
    "T..TT..A...........T",
    "T.......TTTT...s...T",
    "T.$.....T..T.......T",
    "TTTT.TTTT..T..TTTTTT",
    "T..........T.......T",
    "T..c..A....+...A...T",
    "T..........T.......T",
    "TTTTTT.TTTTT.TTT.TTT",
    "T....T.......T.....T",
    "T.A..T..s..c.T..$..T",
    "T....T.......T.....T",
    "T..+...............T",
    "T....TTT..A..TTT.s.T",
    "T.$..T.........T...T",
    "TTTTTTTTTXTTTTTTTTTT",
  },
  { // 3: Nasdaq-100 - Tresor der Hedgefonds
    "RRRRRRRRRRRRRRRRRRRR",
    "RP.....R.....R.....R",
    "R......R..s..R..c..R",
    "R..$...R.....R.....R",
    "R......RRR.RRR..$..R",
    "RRR.RRR.........RR.R",
    "R.....R..A..s...R..R",
    "R..c..R.........R..R",
    "R..+..RRRRR.RRRRR..R",
    "R.........R.R......R",
    "R..s......R.R...A..R",
    "RRRRRR.RRRR.RRRR.RRR",
    "G..................G",
    "G..$....G....G..+..G",
    "G..A..s.....H......G",
    "G.......G....G.....G",
    "G..+...........$...G",
    "G....A.......A.....G",
    "G........s.........G",
    "GGGGGGGGGXGGGGGGGGGG",
  },
  { // 4: Oel - Raffinerie
    "OOOOOOOOOOOOOOOOOOOO",
    "OP....O.....l....$.O",
    "O.....O..o.........O",
    "O..BB.O.....BB..A..O",
    "O..BB...........l..O",
    "O.......OOOO.......O",
    "O.l..A..O..O..o....O",
    "OOOO..OOO..OOOO..OOO",
    "O.........+........O",
    "O..o..BB......BB...O",
    "O.....BB..A...BB.l.O",
    "O..$...........o...O",
    "OOOOOO..OOOOOO..OOOO",
    "O....O..O....O.....O",
    "O.A..O..O.l..O..+..O",
    "O....O.......O.....O",
    "O..l.....o.........O",
    "O.......BB...BB..A.O",
    "O.$.....BB...BB....O",
    "OOOOOOOOOOOOOOOOOOXO",
  },
  { // 5: ALGO-9000 - Rechenzentrum
    "RRRRRRRRRRRRRRRRRRRR",
    "RP....R......R..t..R",
    "R.....R..t...R.....R",
    "R..$..R......R..A..R",
    "R.....RRR..RRR.....R",
    "RRR..........t...RRR",
    "R...A..IIII..IIII..R",
    "R......I........I..R",
    "R..+...I...K....I.$R",
    "R......I........I..R",
    "R..t...II..II..II..R",
    "R..................R",
    "RRRR.RRRR..RRRR.RRRR",
    "R....R........R....R",
    "R.A..R..t..A..R..u.R",
    "R....R........R....R",
    "R..+.....$.........R",
    "R.....RR....RR..t..R",
    "R..c..RR....RR.....R",
    "RRRRRRRRRXRRRRRRRRRR",
  },
};
static const char* const LEVEL_NAMES[NUM_LEVELS] = {
  "DOW JONES: ZENTRALE", "S&P 500: HANDELSPARKETT", "NASDAQ-100: HEDGEFONDS-TRESOR",
  "OEL: RAFFINERIE", "ALGO-9000: RECHENZENTRUM"};
static const char* const LEVEL_NAMES_EN[NUM_LEVELS] = {
  "DOW JONES: BANKING HALL", "S&P 500: TRADING FLOOR", "NASDAQ-100: HEDGE FUND VAULT",
  "OIL: REFINERY", "ALGO-9000: DATA CENTER"};
static const int LEVEL_MARKET[NUM_LEVELS] = {M_DOW, M_SPX, M_NDX, M_OIL, M_NDX};

static char world[MAP_H][MAP_W];

static inline bool solidCell(int x, int y) {
  if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return true;
  return world[y][x] != '.';
}
static inline int texOf(char c) {
  switch (c) {
    case 'T': return T_TICKER; case 'R': return T_SERVER; case 'G': return T_GOLD;
    case 'X': return T_VAULT;  case 'C': return T_CHART;  case 'O': return T_PIPES;
    case 'B': return T_BARRELS; case 'I': return T_AICORE; default: return T_MARBLE;
  }
}

// ---------------------------------------------------------------- Spielobjekte
enum { E_BEAR, E_SELLER, E_BUBBLE, E_SHARK, E_BULL, E_BARON, E_SLICK, E_BOT, E_ALGO, NUM_TYPES };
enum { F_BULL, F_BEAR, F_NEUTRAL, F_AI };

struct TypeInfo { uint8_t faction; int16_t hp; float speed, scale; uint8_t spr; int pts; bool boss; const char* name; };
static const TypeInfo TYPES[NUM_TYPES] = {
  {F_BEAR,    6, 1.5f, 1.00f, S_BEAR0,   120, false, "BAER"},
  {F_BEAR,    3, 1.1f, 1.00f, S_SELL0,   150, false, "SHORT SELLER"},
  {F_BULL,    1, 2.2f, 0.75f, S_BUB0,     60, false, "KRYPTO-BLASE"},
  {F_NEUTRAL,40, 1.2f, 1.30f, S_SHARK0, 1500, true,  "HEDGEFONDS-HAI"},
  {F_BULL,    6, 1.4f, 1.05f, S_BULL0,   120, false, "BULLE"},
  {F_NEUTRAL, 6, 1.0f, 1.00f, S_BARON0,  200, false, "OELBARON"},
  {F_NEUTRAL, 4, 0.9f, 0.85f, S_SLICK0,   80, false, "OELSCHLICK"},
  {F_AI,      3, 2.0f, 0.70f, S_BOT0,    100, false, "TRADING-BOT"},
  {F_AI,     80, 0.0f, 1.50f, S_ALGO0,  5000, true,  "ALGO-9000"},
};
static const char* const TYPE_NAME_EN[NUM_TYPES] = {"BEAR", "SHORT SELLER", "CRYPTO BUBBLE", "HEDGE FUND SHARK", "BULL", "OIL BARON", "OIL SLICK", "TRADING BOT", "ALGO-9000"};

struct Enemy {
  float x, y; uint8_t type, faction; int16_t hp; bool alive, awake, gone, halfDone;
  float cd, anim, hurt, attackAnim, think, deadT, str, chargeT, chargeCd, mdx, mdy, spawnCd;
  int8_t pir;       // Piratentyp in der Steueroase (-1 = keiner)
  int16_t target;   // -1 = Spieler, -2 = keins, sonst Index
  uint8_t role; bool alert; float gx, gy, delay, alertT;   // Piraten: 0 Jaeger, 2 Waechter (bewacht den Schatz bei gx/gy)
  bool stampede, summoned; int8_t orb;   // Stampede-Bulle, vom Boss gerufener Bot, Kreisrichtung des Hais in Phase 2
  float aggroT, coverT, coverCd, covX, covY;   // beschossen: verfolgt den Spieler; Deckung bei covX/covY
  float lurkT, lurkCd, rushT; bool lurking;
  float sideT, sideCd, sdx, sdy;   // Hardcore: Seitschritt aus dem Fadenkreuz
  bool pack;   // gehoert zum Rudel der Baerenfalle
  bool atk;    // Kleinanleger: darf gerade angreifen
  float hopT, hopCd, hopV;   // Oelschlick: Sprung (Dauer, Pause, Tempo)   // Hinterhalt: lauert hinter einer Ecke, bricht dann hervor
  float trapT;   // Baerenfalle: benommen (role 3 = Crash-Waechter bei gx/gy, role 4 = Crash-Jaeger)
};
struct Item { float x, y; uint8_t type; bool active, stray; float fx, fy, tx, ty, slide, flash; bool crash; };   // crash: Notverkauf-Bargeld (150 $)   // slide > 0: schlittert gerade (Baerenfalle)   // 0 = Cash, 1 = Dividende; stray = verstreutes Bargeld
struct Proj { float x, y, dx, dy; bool active; uint8_t dmg, faction, spr; };
static constexpr uint8_t F_SHOT = 9;   // Geldschein aus der Waffe des Spielers

static constexpr int MAX_E = 48, MAX_I = 48, MAX_P = 40;
static Enemy enemies[MAX_E]; static int nEnemies;
static Item items[MAX_I];    static int nItems;
static Proj projs[MAX_P];

static float px, py, dirX, dirY, planeX, planeY;
static int hp, ammo, level, kills, marketKills;
static long pnl;
static long winBonus;   // Bullenmarkt-Bonus fuers Durchspielen
static int deaths;      // Tode im aktuellen Lauf (weisse Weste)
static float fireCd, flashT, hurtT, pickT, msgT, bobT, gameTime, slowT;
static float dodgeT, dodgeCd, dodgeX, dodgeY;   // Ausweichen per Doppeltipp
static char msg[48];
static float zbuf[SW];
static uint8_t hist[48]; static int histN; static float histT;

// Markt- und KI-Zustand
static float levelChg, oilChg, pBullBase, momentum, replayT;
static int replayIdx;
static float spawnT, cashT, aiT, aiFaceT, fakeT, crashT, glitchT, invertT;
// Marktphasen: 1 = Bullrun, -1 = Baerenmarkt. sentiment steigt mit erlegten Baeren, faellt mit erlegten Bullen; echter Kurs = Grundstimmung
static float stampHitT;   // Schutz nach einem Stampede-Treffer
static float pvx, pvy, lastPx, lastPy;   // Laufrichtung des Spielers (geglaettet), damit Hardcore-Schuetzen vorhalten koennen
static float levelTime;   // Zeit im Level (Schnellhandel-Bonus am Tresor: 2000 - 5 je Sekunde)
static int phase; static float phaseT, phaseCd, sentiment, quakeT, stampedeAt, bearHintAt; static bool trapSnapped;
// Boersencrash (hoechstens einmal pro Lauf): crashStage 1 Handelsstopp, 2 Crash mit Margin Call, 3 Erholungsrallye.
// Ausloeser: ueberhitzter Markt (2 Bullruns im Level oder 3 im Lauf) beim naechsten Abschwung, oder echter Kurs mit Live-Daten <= -3 %
static int crashStage, levelBullruns, runBullruns, coolKills; static float crashStT, crashChg; static long marginDue, marginBase, marginPaid; static bool crashDone, rallySaid;
static bool canCrash(); static bool overheated();   // unten definiert
static bool refiDropped, liqDropped;   // Refinanzierungspaket und Liquiditaetsschub: hoechstens je einer pro Level
static int16_t navDist[MAP_W * MAP_H]; static float navT; static bool navOk;   // Wegkarte: Schritte bis zum Spieler je Feld
static constexpr float PHASE_LEN = 20;
static inline bool bullRun() { return phase == 1 && phaseT > 0; }
static inline bool bearMkt() { return phase == -1 && phaseT > 0; }
// ALGO-Manipulation: Aufschlag auf den echten Kurs (Prozentpunkte), laeuft nach manipT Sekunden aus
static float realChg, manipChg, manipT, dumpAt;
static bool hardcore, easy;   // Einfach: unbegrenzt Leben; Normal: 5 Leben; Hardcore: 1 Leben
static constexpr int SOLO_LIVES = 5;
// Bonus-Level Steueroase nach dem Sieg (Code weiter unten)
static bool island, islandDone, isleAll;
static void endIsland(const char* msg, bool all);
// Ticker als farbige Segmente: Index, danach seine Einzelwerte
struct TickSeg { char t[28]; uint16_t col; };
static TickSeg tickSegs[M_COUNT * 2 + STOCK_COUNT];
static int nTickSegs, tickChars;
static int bullsAlive, bearsAlive;

enum State { ST_TITLE, ST_PLAY, ST_PAUSE, ST_LEVELDONE, ST_DEAD, ST_WIN, ST_WIFI_LIST, ST_WIFI_PASS, ST_CREDITS, ST_OUTRO };
static State state = ST_TITLE;

static void say(const char* s, float t = 2.0f) { strncpy(msg, s, sizeof(msg) - 1); msg[sizeof(msg) - 1] = 0; msgT = t; }
static float dist2Player(float x, float y) { float dx = x - px, dy = y - py; return dx * dx + dy * dy; }
// Geraeuschanzeige: Gegner-Geraeusche in Hoerweite (Ort und Restzeit)
struct SoundCue { float x, y, t; };
static SoundCue soundCues[6];
static void addSoundCue(float x, float y) {
  int slot = 0;
  for (int i = 0; i < 6; i++) { float dx = soundCues[i].x - x, dy = soundCues[i].y - y; if (soundCues[i].t > 0 && dx * dx + dy * dy < 2) { slot = i; break; } if (soundCues[i].t < soundCues[slot].t) slot = i; }
  soundCues[slot] = {x, y, 1.5f};
}
static void sfxNear(SfxId id, float x, float y, float range = 7.0f) {
  if (dist2Player(x, y) >= range * range) return;
  sfxPlay(id);
  if (id == SFX_ID_ROAR || id == SFX_ID_BULL || id == SFX_ID_ZAP || id == SFX_ID_LASER || id == SFX_ID_OIL || id == SFX_ID_POP) addSoundCue(x, y);
}

static bool isBoss(const Enemy& e) { return TYPES[e.type].boss; }
static bool hostileFactions(uint8_t a, uint8_t b) { return (a == F_BULL && b == F_BEAR) || (a == F_BEAR && b == F_BULL); }

// ---------------------------------------------------------------- Live-Markt
static const Market& levelMarket() { return markets[LEVEL_MARKET[level]]; }

static void applyLang() { markets[M_OIL].label = L("OIL", "OEL"); }
static void buildTicker() {
  const uint16_t magenta = c565(255, 60, 200), yellow = c565(255, 200, 60);
  nTickSegs = 0; tickChars = 0;
  auto add = [](const char* t, uint16_t col) {
    TickSeg& s = tickSegs[nTickSegs++];
    strncpy(s.t, t, sizeof(s.t) - 1); s.t[sizeof(s.t) - 1] = 0; s.col = col;
    tickChars += strlen(s.t);
  };
  char b[28];
  for (int i = 0; i < M_COUNT; i++) {
    const Market& m = markets[i];
    float price = m.price, chg = m.chg;
    uint16_t col = yellow;
    if (fakeT > 0) { chg = ((int)(rnd() % 4001) - 2000) / 100.0f; price *= 1 + chg / 100; col = magenta; }   // ALGO-9000 Fake News
    else if (manipDrawn != 0 && &m == &levelMarket()) { chg += manipDrawn; price *= 1 + manipDrawn / 100; col = magenta; }
    snprintf(b, sizeof(b), "%s %.*f %s%.2f%%%s  ", m.label, price < 1000 ? 2 : 0, price, chg >= 0 ? "+" : "", chg, m.live ? "" : "*");
    add(b, col);
    // Einzelwerte: nur Anzeige, gruen steigend / rot fallend (^ bzw. v)
    for (const auto& s : stocks) {
      if (s.market != i) continue;
      float sc = fakeT > 0 ? ((int)(rnd() % 1601) - 800) / 100.0f : s.chg;
      if (s.price > 0) snprintf(b, sizeof(b), "%s %.2f %c%.2f%%%s  ", s.label, s.price, sc >= 0 ? '^' : 'v', fabsf(sc), s.live ? "" : "*");
      else snprintf(b, sizeof(b), "%s %c%.2f%%%s  ", s.label, sc >= 0 ? '^' : 'v', fabsf(sc), s.live ? "" : "*");
      add(b, fakeT > 0 ? magenta : sc >= 0 ? c565(80, 255, 130) : c565(255, 80, 80));
    }
    add("    ", yellow);
  }
}

// Manipulation blendet in den letzten 4 Sekunden aus
static float manipEff() { return manipT > 0 ? manipChg * min(1.0f, manipT / 4) : 0.0f; }
static void marketEffect() {   // Spielkurs = echter Kurs + Manipulation
  levelChg = realChg + manipEff() + crashChg;
  pBullBase = constrain(0.5f + levelChg * 0.2f, 0.15f, 0.85f);
}

static void applyMarket() {
  if (level == NUM_LEVELS - 1) realChg = (markets[M_DOW].chg + markets[M_SPX].chg + markets[M_NDX].chg) / 3;
  else realChg = levelMarket().chg;
  oilChg = markets[M_OIL].chg;
  manipChg = manipT = dumpAt = 0; manipDrawn = 0;
  marketEffect();
  genTickerTex(levelMarket());
  genChartTex(levelMarket());
  buildTicker();
  replayIdx = 0; replayT = 0; momentum = 0;
}

// Wand-Charts und Ticker nachzeichnen, wenn sich die Manipulation spuerbar aendert
static void marketVisuals() {
  float eff = manipEff();
  if (fabsf(eff - manipDrawn) > 0.2f || (eff == 0 && manipDrawn != 0)) {
    manipDrawn = eff; genTickerTex(levelMarket()); genChartTex(levelMarket()); buildTicker();
  }
}

static float animalStrength(bool bull) {
  float s = 1.0f + max(0.0f, (bull ? 1 : -1) * levelChg) * 0.25f;
  return min(s, 1.6f);
}
static float fireRateMul() { return oilChg > 0 ? 1.0f + min(oilChg, 4.0f) * 0.1f : 1.0f; }
static float playerSpeedMul() {
  float m = (oilChg < 0 ? 1.0f + min(-oilChg, 4.0f) * 0.05f : 1.0f) * (phase == 1 && phaseT > 0 ? 1.15f : 1.0f);   // Bullrun: schneller
  return slowT > 0 ? m * 0.5f : m;
}

// ---------------------------------------------------------------- Spawning
static int allocEnemy() {
  for (int i = 0; i < nEnemies; i++) if (enemies[i].gone) return i;
  if (nEnemies < MAX_E) return nEnemies++;
  for (int i = 0; i < nEnemies; i++) if (!enemies[i].alive) return i;
  return -1;
}

static int spawnEnemy(uint8_t type, float x, float y) {
  int i = allocEnemy();
  if (i < 0) return -1;
  Enemy& e = enemies[i];
  e = {};
  e.x = x; e.y = y; e.type = type; e.faction = TYPES[type].faction;
  e.alive = true; e.target = -2; e.pir = -1;
  e.str = type == E_BULL ? animalStrength(true) : (type == E_BEAR ? animalStrength(false) : 1.0f);
  e.hp = (int16_t)roundf(TYPES[type].hp * e.str * (hardcore ? 1.4f : easy ? 0.7f : 1.0f));
  e.cd = 1.0f + frand(); e.anim = frand() * 6; e.think = frand() * 0.4f;
  e.spawnCd = 6;
  float a = frand() * 6.283f; e.mdx = cosf(a); e.mdy = sinf(a);
  return i;
}

static bool lineOfSight(float x0, float y0, float x1, float y1);
// Schussposition fuer Fernkaempfer: Mitte oder Rand des Gegners (so breit wie seine Trefferzone), von der aus der
// Spieler frei sichtbar ist. Sonst trifft man breite Bosse halb hinter einer Kante, waehrend deren Schuesse dort haengen bleiben.
static bool fireOrigin(const Enemy& e, float tx, float ty, float& ox, float& oy, float& oang) {
  float dx = tx - e.x, dy = ty - e.y, d = sqrtf(dx * dx + dy * dy) + 1e-4f, qx = -dy / d, qy = dx / d, r = 0.32f * TYPES[e.type].scale;
  static const float ks[5] = {0, -0.5f, 0.5f, -1, 1};
  for (float k : ks) {
    ox = e.x + qx * r * k; oy = e.y + qy * r * k;
    if (!solidCell((int)floorf(ox), (int)floorf(oy)) && lineOfSight(e.x, e.y, ox, oy) && lineOfSight(ox, oy, tx, ty)) { oang = atan2f(ty - oy, tx - ox); return true; }
  }
  return false;
}
// Rasterlauf ueber alle Zellen, die die Strecke beruehrt; genau durch eine Ecke zaehlt als blockiert
static bool segBlocked(float x0, float y0, float x1, float y1) {
  int cx = (int)floorf(x0), cy = (int)floorf(y0);
  const int ex = (int)floorf(x1), ey = (int)floorf(y1);
  const float dx = x1 - x0, dy = y1 - y0;
  const int sx = dx > 0 ? 1 : -1, sy = dy > 0 ? 1 : -1;
  const float ddx = dx != 0 ? fabsf(1 / dx) : 1e30f, ddy = dy != 0 ? fabsf(1 / dy) : 1e30f;
  float tx = dx != 0 ? (dx > 0 ? cx + 1 - x0 : x0 - cx) * ddx : 1e30f, ty = dy != 0 ? (dy > 0 ? cy + 1 - y0 : y0 - cy) * ddy : 1e30f;
  for (int n = 0; n < 256 && (cx != ex || cy != ey); n++) {
    if (fabsf(tx - ty) < 1e-6f) {
      if (tx > 1) break;
      if (solidCell(cx + sx, cy) || solidCell(cx, cy + sy)) return true;
      cx += sx; cy += sy; tx += ddx; ty += ddy;
    } else if (tx < ty) { if (tx > 1) break; cx += sx; tx += ddx; }
    else { if (ty > 1) break; cy += sy; ty += ddy; }
    if (solidCell(cx, cy)) return true;
  }
  return false;
}
static bool lineOfSight(float x0, float y0, float x1, float y1) { return !segBlocked(x0, y0, x1, y1); }

// freie Zelle, weit weg und ausser Sicht des Spielers
static bool findHiddenCell(float& ox, float& oy, float minDist) {
  for (int tries = 0; tries < 40; tries++) {
    int x = 1 + rnd() % (MAP_W - 2), y = 1 + rnd() % (MAP_H - 2);
    if (solidCell(x, y)) continue;
    float cx = x + 0.5f, cy = y + 0.5f;
    if (dist2Player(cx, cy) < minDist * minDist) continue;
    if (lineOfSight(cx, cy, px, py)) continue;
    ox = cx; oy = cy; return true;
  }
  return false;
}

static void spawnAnimal(bool bull) {
  float x, y;
  if (findHiddenCell(x, y, 5)) spawnEnemy(bull ? E_BULL : E_BEAR, x, y);
}
static bool lineOfSight(float x0, float y0, float x1, float y1);
static void spawnBots(int count, int bossIdx = -1) {   // Trading-Bots mit Obergrenze; vom Boss gerufene erscheinen bei ihm und zaehlen halb
  int alive = 0; for (int i = 0; i < nEnemies; i++) if (enemies[i].alive && enemies[i].type == E_BOT) alive++;
  int n = min(count, (easy ? 4 : 6) - alive);
  for (int k = 0; k < n; k++) {
    float x = 0, y = 0; bool ok = false;
    if (bossIdx >= 0) {
      const Enemy& b = enemies[bossIdx];
      for (int t = 0; t < 40 && !ok; t++) {
        int cx = (int)b.x + (int)(rnd() % 13) - 6, cy = (int)b.y + (int)(rnd() % 13) - 6; float fx = cx + 0.5f, fy = cy + 0.5f;
        float d = sqrtf((fx - b.x) * (fx - b.x) + (fy - b.y) * (fy - b.y));
        if (d < 2.5f || d > 6 || solidCell(cx, cy) || lineOfSight(fx, fy, px, py)) continue;
        x = fx; y = fy; ok = true;
      }
    }
    if (!ok) ok = findHiddenCell(x, y, 3.5f);
    if (!ok) continue;
    int i = spawnEnemy(E_BOT, x, y); if (i < 0) continue;
    if (bossIdx >= 0) { const Enemy& b = enemies[bossIdx]; enemies[i].summoned = true; enemies[i].awake = (b.x - px) * (b.x - px) + (b.y - py) * (b.y - py) < 100; }
  }
}
static void spawnNear(uint8_t type, int count) {
  for (int k = 0; k < count; k++) { float x, y; if (findHiddenCell(x, y, 3.5f)) spawnEnemy(type, x, y); }
}

// Keycard: Bosse lassen sie fallen; in Leveln ohne Boss faellt sie nach 8-16 erledigten Gegnern (jedes Mal zufaellig, ohne Hinweis)
static bool hasKey, keyDropped, levelHasBoss; static int keyKills, keyNeed = 12;
static uint8_t reach[MAP_H][MAP_W];
static void computeReach() {   // alle Felder, die man vom Start aus zu Fuss erreicht
  memset(reach, 0, sizeof(reach));
  static uint16_t q[MAP_W * MAP_H]; int n = 0;
  int sx = (int)px, sy = (int)py; reach[sy][sx] = 1; q[n++] = sy * MAP_W + sx;
  while (n) {
    int c = q[--n], x = c % MAP_W, y = c / MAP_W;
    static const int8_t D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto& d : D) { int nx = x + d[0], ny = y + d[1]; if (!solidCell(nx, ny) && !reach[ny][nx]) { reach[ny][nx] = 1; q[n++] = ny * MAP_W + nx; } }
  }
}
static void placeVault(char wall) {   // Tresortuer an einer zufaelligen Aussenwand, erreichbar und moeglichst weit weg vom Start
  static uint8_t cand[4 * MAP_W][2]; int nc = 0, nfar = 0;
  for (int pass = 0; pass < 2 && !nfar; pass++) {
    nc = 0;
    for (int i = 1; i < MAP_W - 1; i++) {
      const int C[4][4] = {{i, 0, i, 1}, {i, MAP_H - 1, i, MAP_H - 2}, {0, i, 1, i}, {MAP_W - 1, i, MAP_W - 2, i}};
      for (auto& c : C) {
        if (!reach[c[3]][c[2]]) continue;
        float dx = c[2] + 0.5f - px, dy = c[3] + 0.5f - py;
        if (pass == 0 && dx * dx + dy * dy < 81) continue;
        cand[nc][0] = c[0]; cand[nc][1] = c[1]; nc++;
      }
    }
    nfar = nc;
  }
  for (int y = 0; y < MAP_H; y++) for (int x = 0; x < MAP_W; x++) if (world[y][x] == 'X') world[y][x] = wall;
  if (nc) { int k = rnd() % nc; world[cand[k][1]][cand[k][0]] = 'X'; }
}
static bool randomEnemyCell(float minD, float& ox, float& oy) {   // zufaelliges erreichbares Feld, nicht zu nah am Start und an anderen Gegnern
  for (int t = 0; t < 160; t++) {
    int x = 1 + rnd() % (MAP_W - 2), y = 1 + rnd() % (MAP_H - 2); float cx = x + 0.5f, cy = y + 0.5f, md = t < 120 ? minD : 3;
    if (!reach[y][x] || dist2Player(cx, cy) < md * md) continue;
    bool bad = false;
    for (int i = 0; i < nEnemies; i++) if (enemies[i].alive && (enemies[i].x - cx) * (enemies[i].x - cx) + (enemies[i].y - cy) * (enemies[i].y - cy) < 2) bad = true;
    for (int i = 0; i < nItems; i++) if (items[i].active && (int)items[i].x == x && (int)items[i].y == y) bad = true;
    if (bad) continue;
    ox = cx; oy = cy; return true;
  }
  return false;
}
static void dropKey(float x, float y) {
  keyDropped = true;
  int i = 0; while (i < nItems && items[i].active) i++;
  if (i == nItems) { if (nItems < MAX_I) nItems++; else i = 0; }
  items[i] = {x, y, 3, true, false};
  say(L("A KEYCARD DROPPED!", "EINE KEYCARD IST GEFALLEN!"), 2.5f);
}
static void loadLevel(int lv) {
  nEnemies = nItems = 0; kills = marketKills = 0;
  uint8_t pend[64]; int npend = 0;   // Gegner der Karte, werden danach zufaellig verteilt
  for (auto& p : projs) p.active = false;
  applyMarket();
  for (int y = 0; y < MAP_H; y++)
    for (int x = 0; x < MAP_W; x++) {
      char c = LEVELS[lv][y][x];
      float cx = x + 0.5f, cy = y + 0.5f;
      world[y][x] = '.';
      switch (c) {
        case 'P': px = cx; py = cy; break;
        case 'A': if (npend < 64) pend[npend++] = frand() < pBullBase ? E_BULL : E_BEAR; break;
        case 'u': if (npend < 64) pend[npend++] = E_BULL; break;
        case 'b': if (npend < 64) pend[npend++] = E_BEAR; break;
        case 's': if (npend < 64) pend[npend++] = E_SELLER; break;
        case 'c': if (npend < 64) pend[npend++] = E_BUBBLE; break;
        case 'H': if (npend < 64) pend[npend++] = E_SHARK; break;
        case 'o': if (npend < 64) pend[npend++] = E_BARON; break;
        case 'l': if (npend < 64) pend[npend++] = E_SLICK; break;
        case 't': if (npend < 64) pend[npend++] = E_BOT; break;
        case 'K': if (npend < 64) pend[npend++] = E_ALGO; break;
        case '$': case '+':
          if (nItems < MAX_I) items[nItems++] = {cx, cy, (uint8_t)(c == '+'), true, false};
          break;
        default: world[y][x] = c;
      }
    }
  computeReach();
  placeVault(LEVELS[lv][0][0]);
  levelHasBoss = false;
  for (int k = 0; k < npend; k++) {
    if (TYPES[pend[k]].boss) levelHasBoss = true;
    float x, y; if (randomEnemyCell(TYPES[pend[k]].boss ? 9 : 5, x, y)) spawnEnemy(pend[k], x, y);
  }
  hasKey = keyDropped = refiDropped = liqDropped = false; keyKills = 0; keyNeed = lv == 0 ? 3 + rnd() % 3 + (easy ? 0 : 1 + rnd() % 2) : 8 + rnd() % 9 - (easy && lv == 1 ? 1 + rnd() % 3 : 0);   // im ersten Level gibt es wenige Gegner
  makeKeySprite(lv);
  static const float dirs[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
  int best = 0, bestLen = -1;
  for (int d = 0; d < 4; d++) {
    int len = 0; float tx = px, ty = py;
    while (!solidCell((int)(tx += dirs[d][0]), (int)(ty += dirs[d][1]))) len++;
    if (len > bestLen) { bestLen = len; best = d; }
  }
  dirX = dirs[best][0]; dirY = dirs[best][1];
  planeX = -dirY * PLANE; planeY = dirX * PLANE;
  histN = 0; histT = 0;
  spawnT = 6; cashT = 40 + frand() * 30; aiT = level == 0 ? 60 + frand() * 25 : level == NUM_LEVELS - 1 ? 22 + frand() * 10 : 40 + frand() * 25;
  aiFaceT = fakeT = crashT = glitchT = invertT = slowT = 0;
  phase = 0; phaseT = 0; phaseCd = 15; sentiment = 0; quakeT = 0; stampedeAt = 0; bearHintAt = 0; navOk = false; levelTime = 0;
  crashStage = 0; crashChg = 0; levelBullruns = 0; coolKills = 0;
  char b[48]; snprintf(b, sizeof(b), "LEVEL %d: %s", lv + 1, L(LEVEL_NAMES_EN[lv], LEVEL_NAMES[lv]));
  say(b, 3.0f);
  sfxPlay(SFX_ID_BELL);   // Eroeffnungsglocke
  musicStart(1 + lv);
}

// Hardcore (Variable oben): staerkere, schnellere Gegner, weniger Nachschub, 1,5-fache Punkte – ein Tod beendet den Lauf
static uint8_t bossPaid;   // Bit je Level: Boss-Punkte schon kassiert
static void newGame() { marketsSimulate(); bossPaid = 0; rngState = esp_random() | 1; hp = 100; ammo = hardcore ? 20 : 30; pnl = 0; deaths = 0; winBonus = 0; level = 0; crashDone = false; runBullruns = 0; island = islandDone = false; loadLevel(0); }

static bool freeAt(float x, float y, float r) {
  return !solidCell((int)(x - r), (int)(y - r)) && !solidCell((int)(x + r), (int)(y - r)) &&
         !solidCell((int)(x - r), (int)(y + r)) && !solidCell((int)(x + r), (int)(y + r));
}
static bool moveEntity(float& x, float& y, float mx, float my, float r) {
  bool ok = true;
  if (freeAt(x + mx, y, r)) x += mx; else ok = false;
  if (freeAt(x, y + my, r)) y += my; else ok = false;
  return ok;
}

// ---- Wegsuche: Schritte bis zum Spieler fuer jedes Feld (Breitensuche), alle 0,25 s neu
static void updateNav(float dt) {
  if ((navT -= dt) > 0 && navOk) return;
  navT = 0.25f; navOk = true;
  static uint16_t q[MAP_W * MAP_H];
  for (int k = 0; k < MAP_W * MAP_H; k++) navDist[k] = -1;
  int head = 0, tail = 0, pi = (int)py * MAP_W + (int)px;
  if (pi < 0 || pi >= MAP_W * MAP_H) return;
  navDist[pi] = 0; q[tail++] = pi;
  static const int8_t D4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  while (head < tail) {
    int c = q[head++], x = c % MAP_W, y = c / MAP_W;
    for (auto& d : D4) {
      int nx = x + d[0], ny = y + d[1]; if (solidCell(nx, ny)) continue;
      int j = ny * MAP_W + nx; if (navDist[j] >= 0) continue;
      navDist[j] = navDist[c] + 1; q[tail++] = j;
    }
  }
}
static bool segBlocked(float x0, float y0, float x1, float y1);
// freie Bahn fuer einen Koerper mit Radius r (Mitte und beide Seiten)
static bool clearPath(float x0, float y0, float x1, float y1, float r) {
  float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy) + 1e-6f, ox = -dy / d * r, oy = dx / d * r;
  return !segBlocked(x0, y0, x1, y1) && !segBlocked(x0 + ox, y0 + oy, x1 + ox, y1 + oy) && !segBlocked(x0 - ox, y0 - oy, x1 - ox, y1 - oy);
}
// auf ein Ziel zugehen: bei freier Bahn direkt, sonst entlang der Wegkarte von Feldmitte zu Feldmitte
static void chase(float& ex, float& ey, float tx, float ty, bool toPlayer, float sp, float dt, float r = 0.25f) {
  float gx = tx, gy = ty;
  if (toPlayer && navOk && !clearPath(ex, ey, tx, ty, r)) {
    int cx = (int)ex, cy = (int)ey, here = navDist[cy * MAP_W + cx], best = here >= 0 ? here : 30000, bx = -1, by = -1;
    static const int8_t D4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto& d : D4) {
      int nx = cx + d[0], ny = cy + d[1]; if (solidCell(nx, ny)) continue;
      int v = navDist[ny * MAP_W + nx]; if (v >= 0 && v < best) { best = v; bx = nx; by = ny; }
    }
    if (bx >= 0) { gx = bx + 0.5f; gy = by + 0.5f; } else { gx = cx + 0.5f; gy = cy + 0.5f; }
  }
  float dx = gx - ex, dy = gy - ey, d = sqrtf(dx * dx + dy * dy) + 1e-6f, step = min(d, sp * dt);
  if (!moveEntity(ex, ey, dx / d * step, dy / d * step, r)) {   // an einer Kante: Richtung Feldmitte ruecken
    float mx = (int)ex + 0.5f - ex, my = (int)ey + 0.5f - ey, lim = sp * dt;
    moveEntity(ex, ey, mx > 0 ? min(mx, lim) : max(mx, -lim), my > 0 ? min(my, lim) : max(my, -lim), r);
  }
}
// steckt ein Gegner in einer Wand, ins naechste freie Feld setzen
static void unstick(float& ex, float& ey, float r) {
  if (freeAt(ex, ey, r * 0.8f)) return;
  int cx = (int)ex, cy = (int)ey; float bd = 1e9f, bxp = ex, byp = ey;
  for (int oy = -1; oy <= 1; oy++) for (int ox = -1; ox <= 1; ox++) {
    if (solidCell(cx + ox, cy + oy)) continue;
    float x = cx + ox + 0.5f, y = cy + oy + 0.5f, dd = (x - ex) * (x - ex) + (y - ey) * (y - ey);
    if (dd < bd) { bd = dd; bxp = x; byp = y; }
  }
  ex = bxp; ey = byp;
}

static int hurtAmt;   // Staerke des letzten Treffers (fuer roten Blitz und Schadenszahl)
// Trefferrichtung: Herkunft des letzten Treffers (Weltkoordinaten); hitSrcOk = Quelle fuer den naechsten Treffer gesetzt
static float hitSrcX, hitSrcY, hitDirX, hitDirY, hitDirT; static bool hitSrcOk;
static void damagePlayer(int d, const char* why) {
  if (hitSrcOk && state == ST_PLAY) { hitDirX = hitSrcX; hitDirY = hitSrcY; hitDirT = 1.0f; }
  if (state != ST_PLAY) return;
  if (hardcore) d = (int)roundf(d * 1.65f); else if (easy) d = (int)roundf(d * 0.7f);
  hp -= d; hurtT = 0.35f; hurtAmt = d; sfxPlay(SFX_ID_PHONE);
  if (why) say(why, 1.2f);
  if (hp <= 0) {
    hp = 0;
    if (island) { endIsland(L("CAUGHT BY THE PIRATES", "VON PIRATEN ERWISCHT"), false); return; }   // beendet nur das Bonus-Level
    deaths++; state = ST_DEAD; musicStop(); sfxPlay(SFX_ID_CRASH);
  }
}

static void spawnProj(float x, float y, float ang, float spd, uint8_t dmg, uint8_t faction, uint8_t spr) {
  if (hardcore) spd *= 1.3f; else if (easy) spd *= 0.85f;
  int used = 0; for (auto& p : projs) if (p.active) used++;
  if (used >= MAX_P - 6) return;   // die letzten Plaetze bleiben fuer die Schuesse des Spielers
  for (auto& p : projs) if (!p.active) { p = {x, y, cosf(ang) * spd, sinf(ang) * spd, true, dmg, faction, spr}; return; }
}

static void dropItem(float x, float y, uint8_t type, bool stray = false) {
  int i = 0;   // aufgesammelte Plaetze wiederverwenden
  while (i < nItems && items[i].active) i++;
  if (i == nItems) { if (nItems >= MAX_I) return; nItems++; }
  items[i] = {x, y, type, true, stray};
}

// Stimmung nach einem Abschuss: Baer/Short Seller -> Richtung Bullrun, Bulle/Krypto-Blase -> Richtung Baerenfalle (Level 1: groessere Schritte)
static void moveSentiment(const Enemy& e, float k) {
  float st = (level == 0 ? 0.34f : 0.2f) * k;
  if (e.faction == F_BEAR) sentiment = min(1.2f, sentiment + st); else if (e.faction == F_BULL) sentiment = max(-1.2f, sentiment - st);
}
static void killEnemy(Enemy& e, bool byPlayer) {
  e.alive = false; e.deadT = 0;
  const TypeInfo& ti = TYPES[e.type];
  if (e.type == E_BUBBLE || e.type == E_BOT) e.gone = true;
  if (byPlayer) {
    kills++;
    if (!island && phaseT <= 0) moveSentiment(e, 1.0f);
    const bool shortGain = bearMkt() && e.faction == F_BEAR;
    int pts = (int)(ti.pts * (bullRun() ? 1.5f : 1.0f) * (shortGain ? 2 : 1) * (e.summoned ? 0.5f : 1.0f) * (e.trapT > 0 ? 1.5f : 1.0f) * (crashStage == 3 ? 1.25f : 1.0f));
    if (e.faction == F_BULL && canCrash() && overheated() && ++coolKills >= 6) { coolKills = 0; levelBullruns = 0; runBullruns = min(runBullruns, 1); say(L("MARKET COOLED DOWN", "MARKT ABGEKUEHLT"), 2); }
    if (ti.boss) { if (bossPaid & (1 << level)) pts /= 10; else bossPaid |= 1 << level; }   // nach einem Tod gibt es den Boss nur noch mit 10 %
    pnl += hardcore ? pts * 3 / 2 : pts;
    char b[48];
    if (shortGain && !ti.boss) { snprintf(b, sizeof(b), L("SHORT GAIN +%d", "SHORT-GEWINN +%d"), pts); say(b); }
    else switch (e.type) {
      case E_BEAR:   say(island ? L("PIRATE DOWN!", "PIRAT ERLEDIGT!") : L("BEAR DOWN!", "BAER ERLEGT!")); break;
      case E_BULL:   say(L("BULL DOWN!", "BULLE ERLEGT!")); break;
      case E_SELLER: say(L("SHORT SELLER LIQUIDATED", "SHORT SELLER LIQUIDIERT")); break;
      case E_BUBBLE: say(L("BUBBLE BURST", "BLASE GEPLATZT")); break;
      case E_SHARK:  say(L("HEDGE FUND SMASHED! GET THE KEYCARD!", "HEDGEFONDS ZERSCHLAGEN! KEYCARD HOLEN!"), 3); break;
      case E_ALGO:   say(L("ALGO-9000 SHUT DOWN! GET THE KEYCARD!", "ALGO-9000 AUS! KEYCARD HOLEN!"), 3); break;
      default: snprintf(b, sizeof(b), "%s %s", L(TYPE_NAME_EN[e.type], ti.name), L("DOWN", "ERLEDIGT")); say(b);
    }
  } else {
    marketKills++;
    if (!island && phaseT <= 0) moveSentiment(e, 0.5f);   // Tiere untereinander bewegen den Markt mit halber Kraft
    if (dist2Player(e.x, e.y) < 64) say(e.type == E_BEAR || e.type == E_SELLER ? L("BULLS TRAMPLE THE BEARS", "BULLEN TRAMPELN BAEREN NIEDER") : L("BEARS MAUL THE BULLS", "BAEREN ZERFLEISCHEN BULLEN"), 1.5f);
  }
  sfxNear(e.type == E_BUBBLE ? SFX_ID_POP : e.type == E_SLICK ? SFX_ID_OIL : SFX_ID_STAMP, e.x, e.y, 10);
  if (!island && !keyDropped) {
    if (ti.boss) { bool other = false; for (int i = 0; i < nEnemies; i++) if (enemies[i].alive && isBoss(enemies[i])) other = true; if (!other) dropKey(e.x, e.y); }
    else if (!levelHasBoss && byPlayer && ++keyKills >= keyNeed) dropKey(e.x, e.y);
  }
  float dropChance = byPlayer ? (e.type == E_SELLER || e.type == E_BARON ? 1.0f : e.summoned ? 0.25f : 0.38f) : 0.22f;
  if (ti.boss) dropItem(e.x, e.y, 1);
  else if (frand() < dropChance * (hardcore ? 0.6f : 1.0f) * (bullRun() ? 2.0f : bearMkt() ? 0.75f : 1.0f)) dropItem(e.x, e.y, frand() < 0.15f ? 1 : 0);
  if (!ti.boss && byPlayer && !e.summoned && !island && !refiDropped && frand() < (hardcore ? 0.005f : easy ? 0.015f : 0.01f)) { refiDropped = true; dropItem(e.x, e.y, 4); }   // ganz selten
  if (!ti.boss && byPlayer && !e.summoned && !island && !liqDropped && frand() < (hardcore ? 0.005f : easy ? 0.015f : 0.01f)) { liqDropped = true; dropItem(e.x, e.y, 5); }   // genauso selten
}

// ---- Marktphasen
// Groesse von Baerenrudel und Bullen-Herde je Level (Index 0–4) und Schwierigkeitsgrad (Kleinanleger, Spekulant, Day-Trader)
// Kleinanleger: gleichzeitige Angreifer Level 1: 3, Level 2–3: 4, Level 4–5: 5
static const uint8_t PACK[3][5] = {{2, 2, 3, 3, 3}, {2, 3, 3, 3, 4}, {3, 3, 4, 4, 5}}, HERD[3][5] = {{3, 3, 4, 4, 4}, {3, 4, 4, 4, 5}, {4, 4, 5, 5, 6}};
static int diffIdx() { return easy ? 0 : hardcore ? 2 : 1; }
static int packSize() { return PACK[diffIdx()][min(4, level)]; }
static int herdSize() { return HERD[diffIdx()][min(4, level)]; }
static int attackSlots() { return 3 + (level + 1) / 2; }
// kein langer gerader Gang in der Naehe: die Herde kommt aus zwei Verstecken und jagt den Spieler
static void stampedeHerd() {
  int n = 0;
  for (int g = 0; g < 2; g++) {
    float x, y; int cnt = 0;   // Versteck, das auf dem Laufweg 5 bis 12 Felder entfernt ist (die Herde soll zuegig ankommen)
    if (navOk) for (int cy = 1; cy < MAP_H - 1; cy++) for (int cx = 1; cx < MAP_W - 1; cx++) {
      int v = navDist[cy * MAP_W + cx];
      if (v >= 5 && v <= 12 && !lineOfSight(cx + 0.5f, cy + 0.5f, px, py) && rnd() % ++cnt == 0) { x = cx + 0.5f; y = cy + 0.5f; }
    }
    if (!cnt && !findHiddenCell(x, y, 5)) continue;
    for (int j = 0; j < 3 && n < herdSize(); j++) { static const float OX[3] = {-0.3f, 0.3f, 0}, OY[3] = {0, 0, 0.3f}; int i = spawnEnemy(E_BULL, x + OX[j], y + OY[j]); if (i < 0) continue; Enemy& e = enemies[i]; e.awake = true; e.target = -1; e.stampede = true; e.chargeCd = 1 + g + j * 0.5f; e.rushT = 6; n++; if (dist2Player(e.x, e.y) < 196) addSoundCue(e.x, e.y); }   // stuermt mit erhoehtem Tempo los
  }
  if (n) { say(L("WATCH OUT, BULLRUN!", "VORSICHT, BULLRUN!"), 2); sfxPlay(SFX_ID_BULL); }
}
static bool stampede() {   // Bullen-Herde stuermt durch einen langen Gang auf den Spieler zu; false = gerade kein Gang frei
  int best = 0, bdx = 0, bdy = 0, cx = 0, cy = 0;
  static const int8_t OFF[5][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}}, DIR[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  for (auto& o : OFF) {
    int bx = (int)px + o[0], by = (int)py + o[1]; if (solidCell(bx, by)) continue;
    for (auto& d : DIR) {
      int n = 0; while (n < 14 && !solidCell(bx + d[0] * (n + 1), by + d[1] * (n + 1))) n++;
      if (n >= 6 && n > best) { best = n; bdx = d[0]; bdy = d[1]; cx = bx; cy = by; }
    }
  }
  if (!best) return false;
  float sx = cx + bdx * best + 0.5f, sy = cy + bdy * best + 0.5f;
  for (int k = 0; k < herdSize(); k++) {
    int i = spawnEnemy(E_BULL, sx - bdx * k * 0.7f + bdy * (k % 2 ? 0.2f : -0.2f), sy - bdy * k * 0.7f + bdx * (k % 2 ? 0.2f : -0.2f));   // gestaffelt, damit man alle vier sieht
    if (i < 0) continue;
    Enemy& e = enemies[i]; if (k == 0 && dist2Player(e.x, e.y) < 196) addSoundCue(e.x, e.y); e.awake = true; e.target = -1; e.chargeT = 4 + k * 0.15f; e.chargeCd = 6; e.mdx = -bdx; e.mdy = -bdy; e.attackAnim = 0.7f; e.stampede = true;
  }
  say(L("WATCH OUT, BULLRUN!", "VORSICHT, BULLRUN!"), 2); sfxPlay(SFX_ID_BULL);
  return true;
}
static void bearTrap() {   // Baerenfalle: ein Baerenrudel greift an (Gegenstueck zur Stampede)
  sfxPlay(SFX_ID_ROAR);
  for (int k = 0; k < packSize(); k++) {   // Rudel aus verschiedenen Verstecken, greift gestaffelt an
    float x, y; if (!findHiddenCell(x, y, 5)) continue;
    int i = spawnEnemy(E_BEAR, x, y); if (i >= 0) { enemies[i].awake = true; enemies[i].cd = 0.5f + k * 0.45f; enemies[i].pack = true; if (dist2Player(x, y) < 196) addSoundCue(x, y); }
  }
}
// letzte 4 Sekunden der Baerenfalle: der Markt dreht, die Baeren sitzen in der Falle (3 s benommen, Fallen-Bonus)
static void snapTrap() {
  trapSnapped = true; sfxPlay(SFX_ID_BELL);
  say(L("TRAP SNAPS SHUT! BEARS STUNNED", "FALLE ZU! BAEREN BENOMMEN"), 3);
  for (int i = 0; i < nEnemies; i++) { Enemy& e = enemies[i]; if (e.alive && e.faction == F_BEAR && e.pir < 0) { e.trapT = 3; e.lurkT = 0; e.lurking = false; e.coverT = 0; } }
}
static bool canCrash() { return !crashDone && !island && !easy && level >= 1; }
static bool overheated() { return levelBullruns >= 2 || runBullruns >= 3; }
static void startCrash() {
  crashDone = true; crashStage = 1; crashStT = 3; rallySaid = false;
  phase = 0; phaseT = 0; stampedeAt = 0; bearHintAt = 0;
  sfxPlay(SFX_ID_ALARM); musicStop();   // Alarm, danach Stille im Handelsstopp
  say(L("TRADING HALT - CRASH!", "HANDELSSTOPP - BOERSENCRASH!"), 3);
  for (int i = 0; i < nEnemies; i++) { enemies[i].lurkT = 0; enemies[i].lurking = false; enemies[i].coverT = 0; enemies[i].chargeT = 0; }
}
static void crashCash(float x, float y) {
  int i = 0; while (i < nItems && items[i].active) i++;
  if (i == nItems) { if (nItems >= MAX_I) return; nItems++; }
  items[i] = {x, y, 0, true, false}; items[i].crash = true;
}
static void beginCrash() {   // nach dem Handelsstopp: Margin Call, Notverkaeufe mit Waechtern, Jagdrudel
  crashStage = 2; crashStT = 20; sfxPlay(SFX_ID_ROAR); musicStart(1 + level);
  marginDue = (long)((500 + 250 * level) * (hardcore ? 1.5f : 1.0f) / 50) * 50;
  marginBase = pnl; marginPaid = 0;
  char b[48]; snprintf(b, sizeof(b), L("MARGIN CALL! RAISE $%ld", "MARGIN CALL! BRING $%ld AUF"), marginDue); say(b, 3.5f);
  for (int k = 0; k < 3; k++) {   // Bargeld-Haufen, jeweils von Baeren bewacht
    float x, y; if (!findHiddenCell(x, y, 4)) continue;
    for (int j = 0; j < 4; j++) crashCash(x + (j % 2 - 0.5f) * 0.5f, y + ((j >> 1) - 0.5f) * 0.5f);
    for (int j = 0; j < (hardcore ? 2 : 1); j++) { int i = spawnEnemy(E_BEAR, x + (j ? 0.6f : -0.6f), y); if (i >= 0) { enemies[i].role = 3; enemies[i].gx = x; enemies[i].gy = y; enemies[i].awake = true; } }
  }
  float hx, hy;
  if (findHiddenCell(hx, hy, 5)) for (int k = 0; k < 3; k++) { int i = spawnEnemy(E_BEAR, hx + (k - 1) * 0.35f, hy); if (i >= 0) { enemies[i].role = 4; enemies[i].awake = true; enemies[i].cd = 0.5f + k * 0.45f; } }
}
static void endCrash(bool ok) {
  crashStage = 3; crashStT = 12; quakeT = 0;
  for (int i = 0; i < nEnemies; i++) if (enemies[i].pir < 0) enemies[i].role = 0;
  if (ok) {
    long bonus = (long)(marginDue * 0.25f / 50) * 50; pnl += hardcore ? bonus * 3 / 2 : bonus; sfxPlay(SFX_ID_KACHING);
    char b[48]; snprintf(b, sizeof(b), L("MARGIN CALL MET! +$%ld", "MARGIN CALL ERFUELLT! +$%ld"), bonus); say(b, 3.5f);
  } else {
    say(L("MARGIN CALL MISSED! 1 LIFE LOST", "MARGIN CALL VERFEHLT! 1 LEBEN WEG"), 3.5f);
    if (hardcore || deaths >= SOLO_LIVES - 1) damagePlayer(9999, nullptr);   // letztes Leben: Insolvenz
    else { deaths++; sfxPlay(SFX_ID_CRASH); }
  }
}
static void updateCrash(float dt) {
  crashStT -= dt; aiT = max(aiT, 5.0f);   // keine ALGO-Ereignisse mitten im Crash
  if (crashStage == 1) { if (crashStT <= 0) beginCrash(); return; }
  if (crashStage == 2) {
    quakeT = max(quakeT, 0.5f); crashChg = max(-10.0f, crashChg - dt * 0.5f); marketEffect();   // Kurs faellt live
    marginPaid = max(marginPaid, pnl - marginBase);
    if (marginPaid >= marginDue) endCrash(true); else if (crashStT <= 0) endCrash(false);
    return;
  }
  crashChg += (-4 - crashChg) * min(1.0f, dt * 0.5f); marketEffect();   // Erholung, aber nicht ganz
  if (!rallySaid && crashStT < 8.5f) { rallySaid = true; say(L("RECOVERY RALLY: 1.25x POINTS", "ERHOLUNGSRALLYE: 1,25-FACHE PUNKTE"), 3); }
  if (crashStT <= 0) { crashStage = 0; phaseCd = 60; say(L("THE MARKET CALMS DOWN", "DER MARKT BERUHIGT SICH"), 1.8f); }
}
static void startPhase(int dir, bool noCrash = false) {
  if (!noCrash && canCrash() && ((levelMarket().live && realChg <= -3) || (dir < 0 && overheated()))) { startCrash(); return; }
  phase = dir; phaseT = PHASE_LEN; sentiment = 0; sfxPlay(dir > 0 ? SFX_ID_BULLRUN : SFX_ID_BEARTRAP); trapSnapped = false;
  if (dir > 0) { levelBullruns++; runBullruns++; }
  if (dir > 0) {
    say(L("BULLRUN! 1.5x POINTS", "BULLRUN! 1,5-FACHE PUNKTE"), 3.5f);
    for (int k = 0; k < 2; k++) { float x, y; if (findHiddenCell(x, y, 4)) dropItem(x, y, 0, true); }
    stampedeAt = 4;
  } else {
    say(L("BEAR TRAP!", "BAERENFALLE!"), 3);
    bearTrap(); bearHintAt = 3.5f;
  }
}
static void updatePhase(float dt) {
  for (int i = 0; i < nItems; i++) {   // Bargeld schlittert sichtbar zur neuen Stelle
    Item& it = items[i];
    if (it.flash > 0) it.flash -= dt;
    if (!(it.slide > 0)) continue;
    it.slide += dt / 1.2f; float k = min(1.0f, it.slide), e = 1 - (1 - k) * (1 - k);
    it.x = it.fx + (it.tx - it.fx) * e; it.y = it.fy + (it.ty - it.fy) * e;
    if (k >= 1) { it.slide = 0; it.flash = 1.5f; }
  }
  if (bearHintAt > 0 && (bearHintAt -= dt) <= 0) say(L("SHORT GAINS: BEARS COUNT 2x", "SHORT-GEWINNE: BAEREN ZAEHLEN 2x"), 2.5f);
  if (crashStage) { updateCrash(dt); return; }
  if (phaseT > 0) {
    phaseT -= dt;
    if (phase < 0 && !trapSnapped && phaseT <= 4) snapTrap();
    if (stampedeAt > 0 && (stampedeAt -= dt) <= 0 && !stampede()) { stampedeHerd(); }   // kein langer Gang frei: Ersatz-Herde aus Verstecken
    if (phaseT <= 0) {
      phase = 0; phaseCd = 35;
      if (canCrash() && overheated()) say(L("MARKET OVERHEATED! KILL BULLS", "MARKT UEBERHITZT! BULLEN ERLEGEN"), 3);
      else say(L("THE MARKET CALMS DOWN", "DER MARKT BERUHIGT SICH"), 1.8f);
    }
    return;
  }
  float base = constrain(levelChg * 0.08f, -0.2f, 0.2f);
  sentiment += (base - sentiment) * dt * 0.01f;
  if ((phaseCd -= dt) > 0) return;
  if (fabsf(sentiment) >= 0.97f) startPhase(sentiment > 0 ? 1 : -1);
  else if (phaseCd < -45) startPhase(sentiment + base > 0 || (sentiment + base == 0 && levelChg >= 0) ? 1 : -1);
}

// naechstes freies Feld (hoechstens 3 Felder weg), von dem aus der Spieler nicht zu sehen ist
static bool findCover(const Enemy& e, float& ox, float& oy) {
  int ex = (int)e.x, ey = (int)e.y; float bd = 1e9f; bool found = false;
  for (int y = ey - 3; y <= ey + 3; y++) for (int x = ex - 3; x <= ex + 3; x++) {
    if (solidCell(x, y)) continue;
    float cx = x + 0.5f, cy = y + 0.5f, d = sqrtf((cx - e.x) * (cx - e.x) + (cy - e.y) * (cy - e.y));
    if (d < bd && !lineOfSight(cx, cy, px, py) && clearPath(e.x, e.y, cx, cy, 0.25f)) { bd = d; ox = cx; oy = cy; found = true; }
  }
  return found;
}
// beschossen: den Spieler eine Weile verfolgen, aus der Ferne getroffen manchmal erst kurz in Deckung gehen
// Hardcore: Schuetzen halten vor, zielen also dorthin, wo der Spieler gleich sein wird
static float leadAng(float fx, float fy, float fa, float spd) {
  if (!hardcore) return fa;
  float k = sqrtf((px - fx) * (px - fx) + (py - fy) * (py - fy)) / (spd * 1.3f) * 0.8f;
  return atan2f(py + pvy * k - fy, px + pvx * k - fx);
}
// Hardcore: wer ins Fadenkreuz geraet, springt ab und zu zur Seite
static bool dodgeAim(Enemy& e, float d, float dt) {
  if (e.sideT > 0) { e.sideT -= dt; if (!moveEntity(e.x, e.y, e.sdx * 3 * dt, e.sdy * 3 * dt, 0.25f)) e.sideT = 0; return true; }
  if ((e.sideCd -= dt) > 0 || d > 12 || d < 1.5f) return false;
  if (((e.x - px) * dirX + (e.y - py) * dirY) / d < 0.985f || !lineOfSight(e.x, e.y, px, py)) return false;
  e.sideCd = 1.8f + frand() * 1.2f; if (frand() < 0.4f) return false;
  float sd = frand() < 0.5f ? 1.0f : -1.0f; e.sdx = -dirY * sd; e.sdy = dirX * sd; e.sideT = 0.3f; return true;
}
static void provoke(Enemy& e) {
  e.aggroT = easy ? 8 : 20; e.think = 0;
  if (hardcore) for (int i = 0; i < nEnemies; i++) {   // Hardcore: Rudel-Alarm, Artgenossen in der Naehe verfolgen den Spieler mit
    Enemy& o = enemies[i]; float ox = o.x - e.x, oy = o.y - e.y;
    if (&o != &e && o.alive && o.faction == e.faction && o.pir < 0 && o.aggroT <= 0 && ox * ox + oy * oy < 36) { o.aggroT = 10; o.think = 0; o.awake = true; }
  }
  if (e.coverCd > 0 || e.stampede || dist2Player(e.x, e.y) < 36 || frand() < 0.5f) return;
  e.coverCd = 6;
  if (findCover(e, e.covX, e.covY)) e.coverT = 1.2f + frand() * 0.8f;
}
// Schuetzen: in Deckung nachladen, zum Schiessen kurz herauslehnen (nicht im leichten Modus)
static bool shooterCover(Enemy& e, bool los, float d, float sp, float dt) {
  if (easy) return false;
  if (e.coverT <= 0 && e.coverCd <= 0 && e.cd > 0.8f && los && d < 12) {
    e.coverCd = hardcore ? 1.2f : 2.0f;
    if (findCover(e, e.covX, e.covY)) { e.coverT = min(2.5f, e.cd - 0.3f); e.aggroT = max(e.aggroT, 6.0f); }   // hinter der Deckung den Spieler nicht vergessen
  }
  if (e.coverT <= 0) return false;
  float cx = e.covX - e.x, cy = e.covY - e.y;
  if (cx * cx + cy * cy > 0.01f) chase(e.x, e.y, e.covX, e.covY, false, sp, dt);
  return true;
}
// Bullen und Baeren: vom Spieler gesehen und noch nicht nah -> hinter einer Ecke lauern, bis er nah ist oder wegschaut
static bool ambush(Enemy& e, float d, float sp, float dt) {
  if (easy || e.target != -1 || e.stampede || (bullRun() && e.type == E_BULL) || (bearMkt() && e.type == E_BEAR)) { e.lurkT = 0; return false; }
  bool seen = lineOfSight(e.x, e.y, px, py), watched = seen && ((e.x - px) * dirX + (e.y - py) * dirY) / d > 0.7f;
  if (e.lurkT > 0 && (d < 3 || (seen && !watched))) e.lurkT = 0;   // Spieler nah oder schaut weg: zuschlagen
  if (e.lurkT > 0) {
    float cx = e.covX - e.x, cy = e.covY - e.y;
    if (cx * cx + cy * cy > 0.01f) chase(e.x, e.y, e.covX, e.covY, false, sp, dt);
    return true;
  }
  if (e.lurking) { e.lurking = false; e.rushT = 2.5f; e.lurkCd = 8; return false; }   // aus dem Versteck hervorbrechen
  if (e.lurkCd > 0 || d < 4 || d > (hardcore ? 12 : 10) || !watched) return false;
  e.lurkCd = 3; if (!findCover(e, e.covX, e.covY)) return false;
  e.lurkT = 4 + frand() * 2; e.lurking = true; e.aggroT = max(e.aggroT, 9.0f); return true;
}
static void hurtEnemy(Enemy& t, int dmg, bool byPlayer) {
  if (!t.alive) return;
  t.hp -= dmg; t.hurt = 0.15f; t.awake = true;
  if (byPlayer && t.hp > 0 && (t.faction == F_BULL || t.faction == F_BEAR) && t.pir < 0) provoke(t);
  if (t.hp <= 0) killEnemy(t, byPlayer);
  else if (byPlayer) sfxPlay(SFX_ID_HIT);
}

// ---------------------------------------------------------------- KI ALGO-9000
// Meldung passt in msg[48]: "ALGO: FLASH CRASH! KURS -5.00%"
static void manipulate(float chg, float t, const char* what) {
  manipChg = chg; manipT = t; dumpAt = 0; marketEffect();
  char b[48]; snprintf(b, sizeof(b), "ALGO: %s %s %+.2f%%", what, L("PRICE", "KURS"), chg);
  say(b, 2.5f);
}
static void aiEvent(int forced = -1) {
  static const uint8_t EASY_EV[3] = {0, 2, 3};
  int ev = forced >= 0 ? forced : level == 0 ? EASY_EV[rnd() % 3] : rnd() % 6;   // Level 1: nur die harmloseren Ereignisse
  aiFaceT = 2.2f; glitchT = max(glitchT, 0.6f);
  sfxPlay(SFX_ID_GLITCH);
  switch (ev) {
    case 0: fakeT = 10; spawnNear(frand() < 0.5f ? E_BULL : E_BEAR, 2); manipulate((frand() < 0.5f ? -1 : 1) * (2 + frand() * 2), 20, "FAKE NEWS!"); break;
    case 1: crashT = 8; manipulate(-5, 15, "FLASH CRASH!"); break;
    case 2: spawnBots(2 + rnd() % 2); manipulate(-1.5f - frand(), 20, L("BOTS ACTIVE", "BOTS AKTIV")); break;
    case 3: spawnNear(E_BUBBLE, 3); manipulate(4, 24, "PUMP & DUMP!"); dumpAt = 12; break;
    case 4: glitchT = 4; invertT = 3; manipulate((frand() < 0.5f ? -1 : 1) * 4, 15, L("MANIPULATED!", "MANIPULIERT!")); break;
    default:
      for (int i = 0; i < nEnemies; i++) if (enemies[i].alive && (enemies[i].type == E_BULL || enemies[i].type == E_BEAR)) { enemies[i].str = fminf(enemies[i].str * 1.2f, 1.92f); enemies[i].awake = true; }
      spawnNear(E_BULL, 1); spawnNear(E_BEAR, 1);
      manipulate(realChg < 0 ? -3 : 3, 20, L("ENRAGED!", "AUFGEHETZT!"));
      break;
  }
  buildTicker();
}

// ---------------------------------------------------------------- Spieler
static void tryMove(float mx, float my) {
  const float r = 0.22f;
  float nx = px + mx, ny = py + my;
  int cx = (int)(nx + (mx > 0 ? r : -r)), cy = (int)(ny + (my > 0 ? r : -r));
  if ((cx >= 0 && cx < MAP_W && world[(int)py][cx] == 'X') || (cy >= 0 && cy < MAP_H && world[cy][(int)px] == 'X')) {
    if (!hasKey) {
      if (msgT <= 0) { char b[48]; snprintf(b, sizeof(b), L("VAULT LOCKED - NEED THE %s KEYCARD", "TRESOR ZU - DU BRAUCHST DIE %s KEYCARD"), L(KEY_EN[level % 5], KEY_DE[level % 5])); say(b, 1.5f); sfxPlay(SFX_ID_DECLINED); }
    }
    else {
      state = level >= NUM_LEVELS - 1 ? ST_WIN : ST_LEVELDONE; pnl += hardcore ? (500 + hp * 5) * 3 / 2 : 500 + hp * 5;
      { int quick = max(0, 2000 - 5 * (int)levelTime); if (quick) { pnl += hardcore ? quick * 3 / 2 : quick; char qb[48]; snprintf(qb, sizeof(qb), L("BONUS +$%d", "BONUS +$%d"), quick); say(qb, 3.0f); } }
      if (state == ST_WIN) {   // Bullenmarkt-Bonus: 10.000 + 25 % des P&L + 5.000 ohne Tod, Hardcore x1,5
        winBonus = 10000 + pnl / 4 + (deaths ? 0 : 5000);
        if (hardcore) winBonus = winBonus * 3 / 2;
        pnl += winBonus;
      }
      musicStop(); sfxPlay(SFX_ID_BELL); if (state == ST_WIN) sfxPlay(SFX_ID_KACHING);
      return;
    }
  }
  moveEntity(px, py, mx, my, r);
}

static void shoot() {
  if (fireCd > 0) return;
  if (ammo <= 0) { say(L("NO LIQUIDITY! GRAB SOME CASH", "KEINE LIQUIDITAET! HOL DIR ETWAS BARGELD"), 1.2f); fireCd = 0.4f; sfxPlay(SFX_ID_DECLINED); return; }
  Proj* slot = nullptr; for (auto& p : projs) if (!p.active) { slot = &p; break; }
  if (!slot) { fireCd = 0.1f; return; }   // kein freier Platz: kein Schuss und auch kein Cash weg
  ammo--; fireCd = 0.28f; flashT = 0.12f; sfxPlay(SFX_ID_SHOT);
  // die Kanone feuert einen Geldschein ab; Schaden gibt es erst, wenn er trifft
  *slot = {px + dirX * 0.3f, py + dirY * 0.3f, dirX * 13, dirY * 13, true, 2, F_SHOT, S_NOTE};
}
static void stepShot(Proj& p, float dt) {   // Geldschein bewegen, in zwei Teilschritten, damit er duenne Gegner nicht ueberspringt
  for (int k = 0; k < 2 && p.active; k++) {
    const float ox = p.x, oy = p.y; p.x += p.dx * dt * 0.5f; p.y += p.dy * dt * 0.5f;
    if (segBlocked(ox, oy, p.x, p.y)) { p.active = false; break; }
    for (int i = 0; i < nEnemies; i++) {
      Enemy& e = enemies[i];
      if (!e.alive) continue;
      float r = 0.32f * TYPES[e.type].scale + 0.05f, dx = e.x - p.x, dy = e.y - p.y;
      if (dx * dx + dy * dy < r * r && !segBlocked(p.x, p.y, e.x, e.y)) { hurtEnemy(e, p.dmg, true); p.active = false; break; }
    }
  }
}

// ---------------------------------------------------------------- Gegner-KI
static void chooseTarget(Enemy& e, int self) {
  if ((bullRun() && e.type == E_BULL) || (bearMkt() && e.type == E_BEAR)) { e.target = -1; e.awake = true; return; }   // Marktphase: jagt den Spieler, egal wo
  if (e.role == 4 && crashStage == 2) { e.target = -1; e.awake = true; return; }   // Crash-Jagdrudel
  if (e.aggroT > 0) { e.target = -1; return; }   // wer schiesst, wird verfolgt – auch aus der Ferne
  float best = 1e9f; int bt = -2;
  float dp = sqrtf(dist2Player(e.x, e.y));
  if (dp < 11 && lineOfSight(e.x, e.y, px, py)) { best = dp; bt = -1; }
  if (crashT <= 0) {
    for (int j = 0; j < nEnemies; j++) {
      if (j == self) continue;
      const Enemy& o = enemies[j];
      if (!o.alive || !hostileFactions(e.faction, o.faction) || (o.pack && bearMkt()) || (o.stampede && bullRun())) continue;   // Rudel der Baerenfalle und Stampede laesst man in Ruhe
      float dx = o.x - e.x, dy = o.y - e.y, d = sqrtf(dx * dx + dy * dy);
      if (d > 8 || d * 0.6f > best) continue;
      if (!lineOfSight(e.x, e.y, o.x, o.y)) continue;
      best = d * 0.6f; bt = j;   // Bullen und Baeren hassen sich mehr als dich
    }
  }
  e.target = bt;
}

static void meleeHit(Enemy& e, int tgt, int dmgPlayer, int dmgEnemy, const char* why) {
  if (tgt == -1 ? segBlocked(e.x, e.y, px, py) : segBlocked(e.x, e.y, enemies[tgt].x, enemies[tgt].y)) return;   // nicht um Ecken herum
  if (tgt == -1) damagePlayer((int)(dmgPlayer * e.str), why);
  else { hurtEnemy(enemies[tgt], (int)ceilf(dmgEnemy * e.str), false); sfxNear(e.faction == F_BULL ? SFX_ID_BULL : SFX_ID_ROAR, e.x, e.y, 6); }
}

static void pirateAI(Enemy& e, float dt);
// Kleinanleger: wer gerade keinen Angriffsplatz hat, haelt Abstand und kreist, statt anzugreifen
static void holdBack(Enemy& e, float d, float nx, float ny, float dt) {
  e.chargeT = 0;
  float o = e.orb < 0 ? -1.0f : 1.0f;
  if (d < 5) moveEntity(e.x, e.y, -nx * 0.9f * dt, -ny * 0.9f * dt, 0.25f);
  else if (d > 8) chase(e.x, e.y, px, py, true, 0.9f, dt);
  else if (!moveEntity(e.x, e.y, -ny * o * 0.6f * dt, nx * o * 0.6f * dt, 0.25f)) e.orb = o > 0 ? -1 : 1;
}
static void updateEnemy(Enemy& e, int idx, float dt) {
  const float bearRage = bearMkt() && e.faction == F_BEAR ? 1.3f : 1.0f;   // Baerenmarkt: Baeren schneller und angriffslustiger
  e.anim += dt; e.hurt -= dt; e.cd -= dt * min(2.0f, fireRateMul() * (hardcore ? 1.45f : easy ? 0.75f : 1.0f) * (bearRage > 1 ? 1.15f : 1.0f)); e.attackAnim -= dt; e.chargeCd -= dt; e.think -= dt; e.aggroT -= dt; e.coverT -= dt; e.coverCd -= dt; e.lurkT -= dt; e.lurkCd -= dt; e.rushT -= dt; e.trapT -= dt;   // Gesamttakt hoechstens x2
  if (crashStage == 1) return;   // Handelsstopp: alles erstarrt
  if (e.pir >= 0) { pirateAI(e, dt); return; }
  unstick(e.x, e.y, e.type == E_SHARK ? 0.35f : 0.25f);
  if (!TYPES[e.type].boss) for (int j = 0; j < nEnemies; j++) {   // Abstand halten: Gegner laufen nicht deckungsgleich uebereinander
    Enemy& o = enemies[j];
    if (&o == &e || !o.alive || o.pir >= 0 || TYPES[o.type].boss) continue;
    float ox = e.x - o.x, oy = e.y - o.y, q = ox * ox + oy * oy;
    if (q >= 0.36f) continue;
    if (q < 1e-6f) { moveEntity(e.x, e.y, (frand() - 0.5f) * 0.1f, (frand() - 0.5f) * 0.1f, 0.25f); continue; }
    float l = sqrtf(q), push = (0.6f - l) * 4 * dt; moveEntity(e.x, e.y, ox / l * push, oy / l * push, 0.25f);
  }
  const float spdMul = min(1.6f, (crashT > 0 ? 1.4f : 1.0f) * bearRage);
  const bool animal = e.faction == F_BULL || e.faction == F_BEAR;
  if (animal && e.trapT > 0) {   // in der Falle: benommen
    if (frand() < dt * 2) { float a = frand() * 6.283f; e.mdx = cosf(a); e.mdy = sinf(a); }
    moveEntity(e.x, e.y, e.mdx * 0.4f * dt, e.mdy * 0.4f * dt, 0.25f); return;
  }
  if (crashStage == 2 && e.faction == F_BULL) {   // Crash: Panik, die Bullen fliehen
    float fx = e.x - px, fy = e.y - py, l = sqrtf(fx * fx + fy * fy) + 1e-4f;
    if (l < 12 && !moveEntity(e.x, e.y, fx / l * 2 * dt, fy / l * 2 * dt, 0.25f)) moveEntity(e.x, e.y, -fy / l * 2 * dt, fx / l * 2 * dt, 0.25f);
    return;
  }
  if (crashStage == 2 && e.role == 3) {   // Crash-Waechter: bleibt am Bargeld-Haufen, greift nur in der Naehe an
    float gdx = px - e.gx, gdy = py - e.gy, dg = sqrtf(gdx * gdx + gdy * gdy), d = sqrtf(dist2Player(e.x, e.y));
    e.alertT -= dt;
    if (e.hurt > 0 || (dg < 4 && lineOfSight(e.x, e.y, px, py))) e.alertT = 4;
    if (e.alertT > 0 && dg < (hardcore ? 7 : 6)) {
      if (d > 0.75f) chase(e.x, e.y, px, py, true, 1.6f * spdMul, dt);
      else if (e.cd <= 0) { e.cd = 1; e.attackAnim = 0.3f; meleeHit(e, -1, 12, 2, L("BEAR CLAW! PRICE DROP", "BAERENTATZE! KURSVERLUST")); }
      return;
    }
    float gx = e.gx + cosf(e.anim * 0.8f) * 0.7f, gy = e.gy + sinf(e.anim * 0.8f) * 0.7f, ddx = gx - e.x, ddy = gy - e.y;
    chase(e.x, e.y, gx, gy, false, ddx * ddx + ddy * ddy > 2.25f ? 1.6f * spdMul : 0.8f, dt);
    return;
  }

  if (animal && e.think <= 0) { e.think = 0.4f; chooseTarget(e, idx); if (e.target != -2) e.awake = true; }

  // Zielposition
  float tx = px, ty = py; bool hasTarget = true;
  if (animal) {
    if (e.target >= 0) {
      const Enemy& t = enemies[e.target];
      if (!t.alive) { e.target = -2; hasTarget = false; } else { tx = t.x; ty = t.y; }
    } else if (e.target == -2) hasTarget = false;
  }
  float dx = tx - e.x, dy = ty - e.y, d = sqrtf(dx * dx + dy * dy) + 1e-4f;
  float nx = dx / d, ny = dy / d, ang = atan2f(dy, dx);
  if (hardcore && hasTarget && e.awake && (!animal || e.target == -1) && !TYPES[e.type].boss && e.type != E_BUBBLE && !(e.chargeT > 0) && dodgeAim(e, d, dt)) return;
  if (easy && hasTarget && e.awake && (!animal || e.target == -1) && !TYPES[e.type].boss && !e.atk) { holdBack(e, d, nx, ny, dt); return; }   // wartet auf einen freien Angriffsplatz
  const bool toPl = !animal || e.target == -1;   // nur zum Spieler gibt es eine Wegkarte

  if (!animal) {
    float fx = e.x, fy = e.y, fa = ang;
    bool los = d < (TYPES[e.type].boss ? 30 : 12) && fireOrigin(e, px, py, fx, fy, fa);   // Bosse schiessen ueber die ganze Karte zurueck
    if (!e.awake) { if (los && d < 9) e.awake = true; else return; }
    switch (e.type) {
      case E_SELLER:
        if (d > 4.0f || !los) chase(e.x, e.y, tx, ty, toPl, 1.1f * spdMul, dt);
        else if (d < 2.2f) moveEntity(e.x, e.y, -nx * 0.9f * dt, -ny * 0.9f * dt, 0.25f);
        break;
      case E_SHARK:
        if (!e.halfDone && e.hp * 2 < (int)roundf(TYPES[E_SHARK].hp * (hardcore ? 1.4f : easy ? 0.7f : 1.0f))) { e.halfDone = true; e.orb = 1; say("MARGIN CALL!", 2); sfxPlay(SFX_ID_ROAR); }   // Phase 2
        if (e.halfDone && los && d < 7 && d > 1.5f) {   // Phase 2: umkreist den Spieler auf 3-5 Feldern
          float want = d < 3 ? -1.0f : d > 5 ? 1.0f : 0.0f, sp = (easy ? 1.4f : 1.8f) * dt;
          if (!moveEntity(e.x, e.y, (nx * want - ny * 0.8f * e.orb) * sp, (ny * want + nx * 0.8f * e.orb) * sp, 0.35f)) e.orb = -e.orb;
        } else if (d > 3.0f || !los || d < 1.5f) chase(e.x, e.y, tx, ty, toPl, 1.4f, dt, 0.35f);   // aus der Naehe ran zum Biss
        if (d < 0.9f && e.cd <= 0 && !segBlocked(e.x, e.y, tx, ty)) { e.cd = 1.0f; damagePlayer(20, L("HOSTILE TAKEOVER!", "FEINDLICHE UEBERNAHME!")); }
        if (los && e.cd <= 0 && d >= 1.5f) {   // unter 1,5 Feldern nur Biss, keine Salve
          e.attackAnim = 0.4f; sfxNear(SFX_ID_ZAP, e.x, e.y, 12);
          if (e.halfDone) { e.cd = 1.5f; for (int k = -2; k <= 2; k++) spawnProj(fx, fy, fa + k * 0.14f, 4.0f, 10, F_NEUTRAL, S_PROJ); }
          else { e.cd = 1.3f; for (int k = -1; k <= 1; k++) spawnProj(fx, fy, fa + k * 0.18f, 4.0f, 10, F_NEUTRAL, S_PROJ); }
        }
        return;
      case E_BARON:
        if (shooterCover(e, los, d, 1.2f, dt)) return;
        if (d > 4.5f || !los) chase(e.x, e.y, tx, ty, toPl, 1.0f, dt);
        else if (d < 2.5f) moveEntity(e.x, e.y, -nx * 0.8f * dt, -ny * 0.8f * dt, 0.25f);
        if (los && e.cd <= 0 && d < 10) {
          e.cd = 2.0f + frand(); e.attackAnim = 0.4f;
          spawnProj(fx, fy, leadAng(fx, fy, fa, 3.4f), 3.4f, 10, F_NEUTRAL, S_PROJ_OIL);
          sfxNear(SFX_ID_OIL, e.x, e.y, 10);
        }
        return;
      case E_SLICK:   // kriecht; springt nur zum Angriff (aus Sprungweite) oder um auszuweichen
        e.hopCd -= dt;
        if (e.hopT > 0) {   // in der Luft: schneller, landet mit einem Platsch
          e.hopT -= dt; moveEntity(e.x, e.y, e.mdx * e.hopV * dt, e.mdy * e.hopV * dt, 0.25f);   // Satz nach vorne, bis gut 2 Felder weit
          if (e.hopT <= 0) { sfxNear(SFX_ID_OIL, e.x, e.y, 7); if (d < 0.9f && e.cd <= 0 && !segBlocked(e.x, e.y, tx, ty)) { e.cd = 1.2f; slowT = 3; damagePlayer(8, L("OIL SPILL! YOU'RE STUCK", "OELPEST! DU KLEBST FEST")); } }
          return;
        }
        if (e.hopCd <= 0 && d > 1.2f && d < 3.1f && los) {   // Angriffssprung: aus Sprungweite auf den Spieler
          e.hopCd = (easy ? 2.4f : hardcore ? 1.1f : 1.6f) + frand() * 1.2f; e.hopT = 0.55f; e.hopV = min(4.5f * spdMul, (d - 0.6f) / 0.55f);
          float sw = (frand() - 0.5f) * 0.3f; e.mdx = nx * cosf(sw) - ny * sinf(sw); e.mdy = ny * cosf(sw) + nx * sinf(sw); return;
        }
        // Ausweichsprung wie bei den anderen Gegnern je Schwierigkeitsgrad: Kleinanleger nie, Spekulant nur wenn getroffen (50 %),
        // Day-Trader auch wenn du auf ihn zielst (60 %); nicht ganz aus der Naehe
        if (!easy && e.hopCd <= 0 && d > 1.2f && d < 9 && los && (e.hurt > 0 || (hardcore && ((e.x - px) * dirX + (e.y - py) * dirY) / d > 0.985f))) {
          e.hopCd = hardcore ? 1.8f + frand() * 1.2f : 3.0f + frand();
          if (frand() < (hardcore ? 0.6f : 0.5f)) { e.hopT = 0.55f; e.hopV = 3.2f * spdMul; float sd = frand() < 0.5f ? 1.0f : -1.0f; e.mdx = -ny * sd; e.mdy = nx * sd; return; }
        }
        if (d > 0.7f) chase(e.x, e.y, tx, ty, toPl, 0.9f * spdMul, dt);
        else if (e.cd <= 0 && !segBlocked(e.x, e.y, tx, ty)) { e.cd = 1.2f; e.attackAnim = 0.3f; slowT = 3; sfxPlay(SFX_ID_OIL); damagePlayer(8, L("OIL SPILL! YOU'RE STUCK", "OELPEST! DU KLEBST FEST")); }
        return;
      case E_BOT: {
        if (shooterCover(e, los, d, 2.0f, dt)) return;
        float w = sinf(e.anim * 3) * 0.6f;
        if (!los) chase(e.x, e.y, tx, ty, toPl, 2.0f, dt, 0.2f);
        else if (d > 3.2f) moveEntity(e.x, e.y, (nx - ny * w) * 2.0f * dt, (ny + nx * w) * 2.0f * dt, 0.2f);
        else moveEntity(e.x, e.y, -ny * w * 1.5f * dt, nx * w * 1.5f * dt, 0.2f);
        if (los && e.cd <= 0 && d < 10) {
          e.cd = 1.3f + frand() * 0.5f; e.attackAnim = 0.25f;
          spawnProj(fx, fy, leadAng(fx, fy, fa, 6.0f) + (frand() - 0.5f) * 0.1f, 6.0f, 6, F_AI, S_LASER);
          sfxNear(SFX_ID_LASER, e.x, e.y, 10);
        }
        return;
      }
      case E_ALGO:
        e.spawnCd -= dt;
        if (los && e.cd <= 0) {
          e.cd = 1.2f; e.attackAnim = 0.5f;
          for (int k = -2; k <= 2; k++) spawnProj(fx, fy, fa + k * 0.14f, 5.0f, 8, F_AI, S_LASER);
          sfxPlay(SFX_ID_LASER);
        }
        if (e.spawnCd <= 0) { e.spawnCd = 8; spawnBots(2, idx); }
        if (!e.halfDone && e.hp < TYPES[E_ALGO].hp * (hardcore ? 0.7f : easy ? 0.35f : 0.5f)) {   // halbe HP (Modusfaktor 1,4 / 0,7 / 1)
          e.halfDone = true;
          if (crashT > 0) crashT = min(crashT + 3.0f, 10.0f); else aiEvent(1);
          aiT = max(aiT, crashT + 6);   // kein Zufallsereignis direkt hinterher
        }
        return;
      default: break;
    }
    // Short Seller schiesst
    if (e.type == E_SELLER && los && e.cd <= 0 && d < 10) {
      e.cd = 1.6f + frand(); e.attackAnim = 0.3f;
      spawnProj(fx, fy, leadAng(fx, fy, fa, 4.5f) + (frand() - 0.5f) * 0.12f, 4.5f, 8, F_BEAR, S_PROJ);
      sfxNear(SFX_ID_ZAP, e.x, e.y, 10);
    }
    return;
  }

  // ---- Bullen-Fraktion & Baeren-Fraktion
  if (!hasTarget) {   // umherstreifen
    if (!moveEntity(e.x, e.y, e.mdx * 0.6f * dt, e.mdy * 0.6f * dt, 0.25f) || frand() < dt * 0.3f) {
      float a = frand() * 6.283f; e.mdx = cosf(a); e.mdy = sinf(a);
    }
    return;
  }
  if (e.coverT > 0) {   // kurz in Deckung, dann Angriff
    float cx = e.covX - e.x, cy = e.covY - e.y;
    if (cx * cx + cy * cy > 0.01f) chase(e.x, e.y, e.covX, e.covY, false, 2.0f * spdMul, dt);
    return;
  }
  const char* hitMsg = e.faction == F_BULL ? L("BULL RUSH! PRICE JUMP", "BULLE RAMMT DICH! KURSSPRUNG") : e.pir >= 0 ? L("CUTLASS SLASH!", "SAEBELHIEB!") : L("BEAR CLAW! PRICE DROP", "BAERENTATZE! KURSVERLUST");
  switch (e.type) {
    case E_BEAR:
      // Gebruell auf der Jagd (spawnCd dient beim Baer als Gebruell-Timer)
      e.spawnCd -= dt;
      if (e.target == -1 && e.spawnCd <= 0 && d < 8) { e.spawnCd = 5 + frand() * 4; sfxNear(SFX_ID_ROAR, e.x, e.y, 9); }
      if (ambush(e, d, 1.5f * spdMul, dt)) break;
      if (d > 0.75f) { float ps = e.pir >= 0 ? 1.15f : 1.0f; chase(e.x, e.y, tx, ty, toPl, 1.5f * spdMul * ps * (e.rushT > 0 ? (hardcore ? 1.9f : 1.7f) : 1.0f), dt); }
      else if (e.cd <= 0) {
        e.cd = 1.0f; e.attackAnim = 0.3f;
        if (e.target == -1 && e.spawnCd < 3) { e.spawnCd = 3; sfxNear(SFX_ID_ROAR, e.x, e.y, 9); }
        meleeHit(e, e.target, 12, 2, hitMsg);
      }
      break;
    case E_BULL:
      if (e.chargeT > 0) {   // Sturmangriff
        e.chargeT -= dt;
        if (!moveEntity(e.x, e.y, e.mdx * 5.0f * dt, e.mdy * 5.0f * dt, 0.25f)) e.chargeT = 0;
        if (d < 0.75f && !(e.stampede && e.target == -1 && stampHitT > 0)) {   // Stampede: nach einem Treffer trampeln die anderen durch
          e.chargeT = 0; e.cd = 1.0f; meleeHit(e, e.target, 15, 3, hitMsg);
          if (e.stampede && e.target == -1) stampHitT = 0.7f;
        }
      } else if (d > 0.75f) {
        if (ambush(e, d, 1.3f * spdMul, dt)) break;
        chase(e.x, e.y, tx, ty, toPl, 1.3f * spdMul * (e.rushT > 0 ? (hardcore ? 1.9f : 1.7f) : 1.0f), dt);
        if (e.chargeCd <= 0 && d > 1.5f && d < 6 && lineOfSight(e.x, e.y, tx, ty)) {
          e.chargeT = 0.7f; e.chargeCd = 3.5f + frand() * 2; e.mdx = nx; e.mdy = ny; e.attackAnim = 0.7f;
          sfxNear(SFX_ID_BULL, e.x, e.y, 8);
        }
      } else if (e.cd <= 0) { e.cd = 1.1f; e.attackAnim = 0.3f; meleeHit(e, e.target, 10, 2, hitMsg); }
      break;
    case E_SELLER:
      if (e.target == -1 && shooterCover(e, lineOfSight(e.x, e.y, px, py), d, 1.4f, dt)) break;
      if (d > 4.0f || !lineOfSight(e.x, e.y, tx, ty)) chase(e.x, e.y, tx, ty, toPl, 1.1f, dt);
      else if (d < 2.2f) moveEntity(e.x, e.y, -nx * 0.9f * dt, -ny * 0.9f * dt, 0.25f);
      float fx, fy, fa;
      if (e.cd <= 0 && d < 10 && fireOrigin(e, tx, ty, fx, fy, fa)) {
        e.cd = 1.6f + frand(); e.attackAnim = 0.3f;
        spawnProj(fx, fy, (e.target == -1 ? leadAng(fx, fy, fa, 4.5f) : fa) + (frand() - 0.5f) * 0.12f, 4.5f, 8, F_BEAR, S_PROJ);
        sfxNear(SFX_ID_ZAP, e.x, e.y, 10);
      }
      break;
    case E_BUBBLE: {
      float w = sinf(e.anim * 5) * 0.8f;
      if (toPl && !clearPath(e.x, e.y, tx, ty, 0.25f)) chase(e.x, e.y, tx, ty, true, 2.2f * spdMul, dt);   // ohne freie Bahn der Wegkarte folgen
      else moveEntity(e.x, e.y, (nx - ny * w) * 2.2f * dt * spdMul, (ny + nx * w) * 2.2f * dt * spdMul, 0.25f);
      if (d < 0.65f && !segBlocked(e.x, e.y, tx, ty)) {
        e.alive = false; e.gone = true; sfxNear(SFX_ID_POP, e.x, e.y, 10);
        if (e.target == -1) damagePlayer(20, L("CRYPTO CRASH! -20%", "KRYPTO-CRASH! -20%"));
        else hurtEnemy(enemies[e.target], 4, false);
      }
      break;
    }
  }
}

static void drawChartBackground(uint16_t up, uint16_t down);
static void centerText(const char* s, int y, int size, uint16_t col);
// ================================================================ BONUS-LEVEL: STEUEROASE
// Nach dem Sieg ueber ALGO-9000: eine Tropeninsel im Freien. Statt Raycasting zeichnet ein kleiner Low-Poly-Renderer
// flach schattierte Dreiecke, nach Entfernung sortiert (Maleralgorithmus, kein Tiefenpuffer -> wenig RAM).
// Die Insel liegt auf dem normalen 20x20-Raster: Wasser ('~') ist fuer Bewegung und Baeren eine Wand.
static constexpr int ISLE_TIME = 45, ISLE_COINS = 42, ISLE_MIN = -6, ISLE_N = 33, ISLE_R = 10;
static constexpr long ISLE_COIN_PTS = 500, ISLE_ALL_BONUS = 10000;
static float isleT, isleSpawnT, isleCashT, creditsT;
static int isleCoins, isleCoinsPlaced = ISLE_COINS;   // ausgelegte Muenzen (falls weniger Platz ist)
static const char* isleResult = "";
static float isleV[ISLE_N * ISLE_N];
struct IsleTri { float v[9]; uint8_t r, g, b, deco; };   // deco: Aufkleber, wird immer nach der Flaeche dahinter gezeichnet
static constexpr int MAX_OBJ_TRI = 300;
static IsleTri isleObj[MAX_OBJ_TRI]; static int nIsleObj;

static float isleShape(float x, float y) {   // flacher Huegel mit kleinem Gipfel, Rand unter dem Meeresspiegel
  float r = sqrtf((x - 10) * (x - 10) + (y - 10) * (y - 10) * 1.1025f);
  float n = sinf(x * 1.3f) * cosf(y * 1.1f) * 0.15f + sinf(x * 0.7f + y * 0.9f) * 0.2f;
  float pk = 1 - sqrtf((x - 12) * (x - 12) + (y - 8) * (y - 8)) / 2.4f;
  return (1 - r / 9.2f) * 0.75f + n * 0.6f - 0.06f + max(0.0f, pk) * 0.8f;
}
static float isleVert(int i, int j) {
  i = constrain(i - ISLE_MIN, 0, ISLE_N - 1); j = constrain(j - ISLE_MIN, 0, ISLE_N - 1);
  return isleV[j * ISLE_N + i];
}
static float isleGround(float x, float y) {   // begehbare Hoehe (Wasser = 0)
  int i = (int)floorf(x), j = (int)floorf(y); float tx = x - i, ty = y - j;
  float h = (isleVert(i, j) * (1 - tx) + isleVert(i + 1, j) * tx) * (1 - ty) + (isleVert(i, j + 1) * (1 - tx) + isleVert(i + 1, j + 1) * tx) * ty;
  return max(0.0f, h);
}
static float isleTaken[64][2]; static int nIsleTaken;
static bool isleFreeCell(float minDist, bool avoid, float& ox, float& oy) {
  for (int t = 0; t < 200; t++) {
    int x = 1 + rnd() % (MAP_W - 2), y = 1 + rnd() % (MAP_H - 2); float cx = x + 0.5f, cy = y + 0.5f;
    if (world[y][x] != '.' || sqrtf(dist2Player(cx, cy)) < minDist) continue;
    bool bad = false;
    if (avoid) for (int k = 0; k < nIsleTaken; k++) { float ex = isleTaken[k][0] - cx, ey = isleTaken[k][1] - cy; if (ex * ex + ey * ey < 1.0f) bad = true; }
    if (bad) continue;
    ox = cx; oy = cy; return true;
  }
  return false;
}
static void isleTake(float x, float y) { if (nIsleTaken < 64) { isleTaken[nIsleTaken][0] = x; isleTaken[nIsleTaken][1] = y; nIsleTaken++; } }
static void isleAddObj(const float* a, const float* b, const float* c, uint8_t r, uint8_t g, uint8_t bl) {
  if (nIsleObj >= MAX_OBJ_TRI) return;
  IsleTri& t = isleObj[nIsleObj++];
  for (int k = 0; k < 3; k++) { t.v[k] = a[k]; t.v[3 + k] = b[k]; t.v[6 + k] = c[k]; }
  t.r = r; t.g = g; t.b = bl;
}
static void isleQuad(const float* a, const float* b, const float* c, const float* d, uint8_t r, uint8_t g, uint8_t bl) {
  isleAddObj(a, b, c, r, g, bl); isleAddObj(a, c, d, r, g, bl);
}
static void isleBuildObjects() {
  nIsleObj = 0; nIsleTaken = 0;
  // Palmen nahe am Strand: dreikantiger, geneigter Stamm + 5 haengende Wedel
  for (int k = 0, tries = 0; k < 12 && tries < 400; tries++) {
    float x = 1 + rnd() % (MAP_W - 2) + 0.5f, y = 1 + rnd() % (MAP_H - 2) + 0.5f, h = isleGround(x, y);
    if (world[(int)y][(int)x] != '.' || h < 0.08f || h > 0.45f || dist2Player(x, y) < 4) continue;
    bool near = false; for (int q = 0; q < nIsleTaken; q++) if (fabsf(isleTaken[q][0] - x) + fabsf(isleTaken[q][1] - y) < 2.5f) near = true;
    if (near) continue;
    isleTake(x, y); k++;
    float lean = atan2f(y - 10, x - 10), lx = cosf(lean) * 0.35f, ly = sinf(lean) * 0.35f, H = 1.7f + frand() * 0.5f;
    float ring[3][3][3];   // [Hoehe][Ecke][xyz]
    const float ts[3] = {0, 0.5f, 1}, ws[3] = {0.09f, 0.07f, 0.05f};
    for (int s = 0; s < 3; s++) for (int i = 0; i < 3; i++) {
      float a = i * 2.094f, t = ts[s];
      ring[s][i][0] = x + lx * t * t + cosf(a) * ws[s]; ring[s][i][1] = y + ly * t * t + sinf(a) * ws[s]; ring[s][i][2] = h + H * t;
    }
    for (int s = 0; s < 2; s++) for (int i = 0; i < 3; i++) {
      int j = (i + 1) % 3; uint8_t br = i ? 140 : 120, bg = i ? 100 : 85, bb = i ? 60 : 50;
      isleQuad(ring[s][i], ring[s][j], ring[s + 1][j], ring[s + 1][i], br, bg, bb);
    }
    float top[3] = {x + lx, y + ly, h + H};
    for (int i = 0; i < 5; i++) {
      float a = i * 1.257f + frand() * 0.3f, dx = cosf(a), dy = sinf(a);
      float l[3] = {top[0] + dx * 1.1f - dy * 0.22f, top[1] + dy * 1.1f + dx * 0.22f, top[2] - 0.35f};
      float r[3] = {top[0] + dx * 1.1f + dy * 0.22f, top[1] + dy * 1.1f - dx * 0.22f, top[2] - 0.35f};
      float tip[3] = {top[0] + dx * 1.7f, top[1] + dy * 1.7f, top[2] - 0.85f};
      isleAddObj(top, l, r, i % 2 ? 40 : 60, i % 2 ? 150 : 175, i % 2 ? 60 : 70);
      isleAddObj(l, r, tip, 35, 130, 55);
    }
  }
  // Strandbar: Holzbox mit Strohdach
  { float x = 9.5f, y = 14.5f; isleFreeCell(3, true, x, y); isleTake(x, y);
    float bh = isleGround(x, y), s = 0.45f, H = 0.55f, c[4][2] = {{x - s, y - s}, {x + s, y - s}, {x + s, y + s}, {x - s, y + s}};
    for (int i = 0; i < 4; i++) { int j = (i + 1) % 4;
      float a0[3] = {c[i][0], c[i][1], bh}, b0[3] = {c[j][0], c[j][1], bh}, b1[3] = {c[j][0], c[j][1], bh + H}, a1[3] = {c[i][0], c[i][1], bh + H};
      isleQuad(a0, b0, b1, a1, 175, 120, 70); }
    float o = s + 0.2f, R[4][2] = {{x - o, y - o}, {x + o, y - o}, {x + o, y + o}, {x - o, y + o}}, tip[3] = {x, y, bh + H + 0.55f};
    for (int i = 0; i < 4; i++) { int j = (i + 1) % 4; float a[3] = {R[i][0], R[i][1], bh + H}, b[3] = {R[j][0], R[j][1], bh + H};
      isleAddObj(a, b, tip, i % 2 ? 230 : 210, i % 2 ? 200 : 180, i % 2 ? 110 : 95); } }
  // Luxusyacht vor der Kueste
  { float yx = 16.5f, yy = 19;
    for (int t = 0; t < 40; t++) { float a = frand() * 6.283f; yx = 10 + cosf(a) * 12; yy = 10 + sinf(a) * 11.5f; if (isleShape(yx, yy) < -0.3f) break; }
    float ang = atan2f(yy - 10, yx - 10) + 1.5708f, ca = cosf(ang), sa = sinf(ang);
    auto P = [&](float u, float v, float z, float* o) { o[0] = yx + ca * u - sa * v; o[1] = yy + sa * u + ca * v; o[2] = z; };
    const float L = 1.6f, B = 0.45f;
    float bow[3], d[4][3], k[4][3];
    P(L + 0.5f, 0, 0.35f, bow);
    P(L, -B, 0.35f, d[0]); P(-L, -B, 0.35f, d[1]); P(-L, B, 0.35f, d[2]); P(L, B, 0.35f, d[3]);
    P(L, -B * 0.6f, 0, k[0]); P(-L, -B * 0.6f, 0, k[1]); P(-L, B * 0.6f, 0, k[2]); P(L, B * 0.6f, 0, k[3]);
    isleQuad(k[0], k[1], d[1], d[0], 40, 70, 140); isleQuad(k[2], k[3], d[3], d[2], 40, 70, 140); isleQuad(k[1], k[2], d[2], d[1], 40, 70, 140);
    isleAddObj(k[0], d[0], bow, 40, 70, 140); isleAddObj(k[3], bow, d[3], 40, 70, 140); isleAddObj(d[0], d[3], bow, 240, 240, 245);
    isleQuad(d[0], d[1], d[2], d[3], 220, 200, 160);
    float c0[4][3], c1[4][3];
    P(0.6f, -0.3f, 0.35f, c0[0]); P(-0.9f, -0.3f, 0.35f, c0[1]); P(-0.9f, 0.3f, 0.35f, c0[2]); P(0.6f, 0.3f, 0.35f, c0[3]);
    for (int i = 0; i < 4; i++) { c1[i][0] = c0[i][0]; c1[i][1] = c0[i][1]; c1[i][2] = 0.8f; }
    for (int i = 0; i < 4; i++) { int j = (i + 1) % 4; if (i == 0) isleQuad(c0[i], c0[j], c1[j], c1[i], 90, 160, 220); else isleQuad(c0[i], c0[j], c1[j], c1[i], 240, 240, 245); }
    isleQuad(c1[0], c1[1], c1[2], c1[3], 240, 240, 245); }
  // ein paar Felsen
  for (int k = 0; k < 4; k++) {
    float x, y; if (!isleFreeCell(2.5f, true, x, y)) break; isleTake(x, y);
    float h = isleGround(x, y), s = 0.25f + frand() * 0.2f, top[3] = {x, y, h + s * 1.3f}, b[3][3];
    for (int i = 0; i < 3; i++) { b[i][0] = x + cosf(i * 2.1f) * s; b[i][1] = y + sinf(i * 2.1f) * s; b[i][2] = h; }
    for (int i = 0; i < 3; i++) isleAddObj(b[i], b[(i + 1) % 3], top, 135, 128, 118);
  }
}
// Piraten: 60 % mehr Energie, etwas schneller als der Spieler. Rollen: Waechter lauern an einem Muenzhaufen und
// greifen erst an, wenn man nahe kommt; Abfaenger laufen dorthin, wo der Spieler gleich sein wird; Jaeger verfolgen direkt.
static void makePirate(Enemy& e) {
  e.pir = rnd() % 3; e.hp = (int16_t)roundf(e.hp * 1.6f); e.alert = false; e.alertT = 0; e.delay = 1.5f + frand() * 2;
  int coins[MAX_I], nc = 0;
  for (int i = 0; i < nItems; i++) if (items[i].active && items[i].type == 2 && (items[i].x - px) * (items[i].x - px) + (items[i].y - py) * (items[i].y - py) > 64) coins[nc++] = i;
  e.role = nc && rnd() % 100 < 85 ? 2 : 0;   // die meisten bewachen den Schatz, wenige jagen
  if (e.role == 2) { const Item& c = items[coins[rnd() % nc]]; e.gx = c.x; e.gy = c.y; e.x = c.x + 0.8f; e.y = c.y; if (solidCell((int)e.x, (int)e.y)) { e.x = c.x; e.y = c.y; } }
}
static float pirHitAt;   // gemeinsamer Angriffstakt: auch umzingelt trifft nur alle 0,5 s ein Saebel
// Waechter (die meisten) bleiben beim Schatz. Wer angegriffen wird oder wem man zu nahe kommt, schlaegt zurueck, und die
// Waechter in der Naehe helfen mit. Sie verfolgen aber nur innerhalb ihres Reviers: Laeuft der Spieler weiter weg, kehren
// sie im Zickzack zum Schatz zurueck, statt sich einzeln weglocken und abschiessen zu lassen. Nur wenige Jaeger verfolgen direkt.
static void pirateAI(Enemy& e, float dt) {
  if (e.delay > 0 && e.hurt <= 0) { e.delay -= dt; return; }
  float dx = px - e.x, dy = py - e.y, d = sqrtf(dx * dx + dy * dy) + 1e-4f;
  auto go = [&](float tx, float ty, float v) {   // bewegen, an Wasserkanten seitlich ausweichen
    float ex = tx - e.x, ey = ty - e.y, l = sqrtf(ex * ex + ey * ey) + 1e-4f, mx = ex / l * v * dt, my = ey / l * v * dt;
    if (!moveEntity(e.x, e.y, mx, my, 0.25f)) moveEntity(e.x, e.y, -my, mx, 0.25f);
  };
  float v = (hardcore ? 3.2f : 3.0f) * (crashT > 0 ? 1.2f : 1.0f);   // Spieler laeuft 2,8
  if (e.role == 2) {
    bool there = false; int best = -1; float bd = 1e9f;
    for (int i = 0; i < nItems; i++) {
      if (!items[i].active || items[i].type != 2) continue;
      if (fabsf(items[i].x - e.gx) < 0.3f && fabsf(items[i].y - e.gy) < 0.3f) there = true;
      float q = (items[i].x - e.gx) * (items[i].x - e.gx) + (items[i].y - e.gy) * (items[i].y - e.gy);
      if (q < bd) { bd = q; best = i; }
    }
    if (!there && best >= 0) { e.gx = items[best].x; e.gy = items[best].y; }   // Schatz weg: naechsten bewachen
    if (e.hurt > 0) {   // angegriffen: zurueckschlagen und Kameraden in der Naehe alarmieren
      if (!e.alert) {
        sfxNear(SFX_ID_ROAR, e.x, e.y, 8);
        for (int i = 0; i < nEnemies; i++) { Enemy& o = enemies[i]; if (&o != &e && o.alive && o.pir >= 0 && o.role == 2 && (o.x - e.x) * (o.x - e.x) + (o.y - e.y) * (o.y - e.y) < 25) { o.alert = true; o.alertT = 6; o.delay = 0; } }
      }
      e.alert = true; e.alertT = 6;
    }
    if (!e.alert && d < 3.8f && lineOfSight(e.x, e.y, px, py)) { e.alert = true; e.alertT = 6; e.attackAnim = 0.5f; sfxNear(SFX_ID_ROAR, e.x, e.y, 8); }
    if (!e.alert) { go(e.gx + cosf(e.anim * 0.8f) * 0.7f, e.gy + sinf(e.anim * 0.8f) * 0.7f, 0.8f); return; }   // bewacht den Schatz
    e.alertT -= dt;
    float gdx = px - e.gx, gdy = py - e.gy;
    if (gdx * gdx + gdy * gdy > (hardcore ? 49.0f : 36.0f)) {   // nicht weglocken lassen: im Zickzack zurueck zum Schatz
      float hx = e.gx - e.x, hy = e.gy - e.y, hl = sqrtf(hx * hx + hy * hy) + 1e-4f, side = sinf(e.anim * 4) * 0.9f;
      if (hl > 1.2f) go(e.gx - hy / hl * side, e.gy + hx / hl * side, v * 0.9f);
      else go(e.gx + cosf(e.anim * 3) * 1.1f, e.gy + sinf(e.anim * 3) * 1.1f, v * 0.6f);   // am Schatz unruhig hin und her
      if (e.alertT <= 0) e.alert = false;
      return;
    }
  }
  if (d > 0.75f) go(px, py, v);
  else if (e.cd <= 0 && fabsf(gameTime - pirHitAt) > (hardcore ? 0.35f : 0.5f) && !segBlocked(e.x, e.y, px, py)) { pirHitAt = gameTime; e.cd = 1.2f; e.attackAnim = 0.3f; damagePlayer((int)(11 * e.str), L("CUTLASS SLASH!", "SAEBELHIEB!")); }
}
static void startIsland() {
  island = true; islandDone = false; isleAll = false; isleT = ISLE_TIME; isleCoins = 0; isleSpawnT = 5; isleCashT = 10;
  for (int j = 0; j < ISLE_N; j++) for (int i = 0; i < ISLE_N; i++) isleV[j * ISLE_N + i] = isleShape(i + ISLE_MIN, j + ISLE_MIN);
  for (int y = 0; y < MAP_H; y++) for (int x = 0; x < MAP_W; x++) world[y][x] = isleGround(x + 0.5f, y + 0.5f) > 0.06f ? '.' : '~';
  nEnemies = nItems = 0; marketKills = 0;
  for (auto& p : projs) p.active = false;
  manipChg = manipT = dumpAt = 0; manipDrawn = 0; marketEffect();
  aiFaceT = fakeT = crashT = glitchT = invertT = slowT = 0;
  phase = 0; phaseT = 0; sentiment = 0; quakeT = 0; crashStage = 0; crashChg = 0;   // keine Marktphasen in der Steueroase
  int sy = MAP_H - 2; while (sy > 2 && world[sy][10] != '.') sy--;   // Start am Suedstrand, Blick zur Inselmitte
  px = 10.5f; py = sy - 0.5f; dirX = 0; dirY = -1; planeX = -dirY * PLANE; planeY = dirX * PLANE;
  hp = 100; ammo = min(99, ammo + 20); histN = 0;
  isleBuildObjects();
  isleCoinsPlaced = 0;
  for (int k = 0; k < ISLE_COINS; k++) {
    float x, y; if (!isleFreeCell(2.5f, true, x, y) || nItems >= MAX_I) break;
    isleTake(x, y); items[nItems++] = {x, y, 2, true, false}; isleCoinsPlaced++;
  }
  for (int k = 0; k < (hardcore ? 2 : 3); k++) { float x, y; if (isleFreeCell(4, true, x, y)) { isleTake(x, y); dropItem(x, y, 0); } }   // vereinzelt Munition
  for (int k = 0; k < (hardcore ? 10 : 8); k++) { float x, y; if (isleFreeCell(6, false, x, y)) { int i = spawnEnemy(E_BEAR, x, y); if (i >= 0) { makePirate(enemies[i]); enemies[i].awake = true; } } }
  state = ST_PLAY; fireCd = 0.4f; musicStart(6);
  { char b[48]; snprintf(b, sizeof(b), L("TAX HAVEN! %d S - GRAB THE GOLD", "STEUEROASE! %d S - SAMMLE DAS GOLD"), ISLE_TIME); say(b, 3.0f); }
}
// ---- Schlussszene nach der Steueroase: der Spieler im Liegestuhl auf seiner Yacht, mit Sonnenbrille und Cocktail.
// Echtzeit mit dem Low-Poly-Renderer, Kamerafahrt im Bogen auf die Yacht zu; ENTER ueberspringt.
static constexpr float OUTRO_LEN = 13, OUTRO_X = 10, OUTRO_Y = 23.2f;
static float outroT, isleEyeZ = -1;   // isleEyeZ >= 0: feste Augenhoehe statt Gelaende
static constexpr int MAX_EXTRA_TRI = 330;
static IsleTri isleExtra[MAX_EXTRA_TRI]; static int nIsleExtra;
static void oTri(const float* a, const float* b, const float* c, uint8_t r, uint8_t g, uint8_t bl) {
  if (nIsleExtra >= MAX_EXTRA_TRI) return;
  IsleTri& t = isleExtra[nIsleExtra++];
  for (int k = 0; k < 3; k++) { t.v[k] = a[k]; t.v[3 + k] = b[k]; t.v[6 + k] = c[k]; }
  t.r = r; t.g = g; t.b = bl; t.deco = 0;
}
static void oQuad(const float* a, const float* b, const float* c, const float* d, uint8_t r, uint8_t g, uint8_t bl) { oTri(a, b, c, r, g, bl); oTri(a, c, d, r, g, bl); }
static void oBox(float x0, float y0, float z0, float x1, float y1, float z1, uint8_t r, uint8_t g, uint8_t bl) {
  float a[3] = {x0, y0, z1}, b[3] = {x1, y0, z1}, c[3] = {x1, y1, z1}, d[3] = {x0, y1, z1};
  float e[3] = {x0, y0, z0}, f[3] = {x1, y0, z0}, gg[3] = {x1, y1, z0}, h[3] = {x0, y1, z0};
  oQuad(a, b, c, d, r, g, bl); oQuad(e, f, b, a, r, g, bl); oQuad(h, gg, c, d, r, g, bl); oQuad(e, h, d, a, r, g, bl); oQuad(f, gg, c, b, r, g, bl);
}
static void oRib(const float* a, const float* b, float w, uint8_t r, uint8_t g, uint8_t bl) {   // duenner Stab von a nach b
  float a0[3] = {a[0] - w, a[1], a[2]}, a1[3] = {a[0] + w, a[1], a[2]}, b0[3] = {b[0] - w, b[1], b[2]}, b1[3] = {b[0] + w, b[1], b[2]};
  oQuad(a0, a1, b1, b0, r, g, bl);
  float c0[3] = {a[0], a[1], a[2] - w}, c1[3] = {a[0], a[1], a[2] + w}, d0[3] = {b[0], b[1], b[2] - w}, d1[3] = {b[0], b[1], b[2] + w};
  oQuad(c0, c1, d1, d0, r, g, bl);
}
static void outroScene(float t) {
  nIsleExtra = 0;
  const float X = OUTRO_X, Y = OUTRO_Y, D = 0.55f, Lh = 2.8f, B = 1;
  // Rumpf (Bug nach Osten)
  float d[4][3] = {{X + Lh, Y - B, D}, {X - Lh, Y - B, D}, {X - Lh, Y + B, D}, {X + Lh, Y + B, D}};
  float k[4][3] = {{X + Lh, Y - B * 0.6f, 0}, {X - Lh, Y - B * 0.6f, 0}, {X - Lh, Y + B * 0.6f, 0}, {X + Lh, Y + B * 0.6f, 0}};
  float bow[3] = {X + Lh + 0.9f, Y, D};
  oQuad(k[0], k[1], d[1], d[0], 240, 240, 245); oQuad(k[2], k[3], d[3], d[2], 240, 240, 245); oQuad(k[1], k[2], d[2], d[1], 30, 50, 110);
  oTri(k[0], d[0], bow, 240, 240, 245); oTri(k[3], bow, d[3], 240, 240, 245); oTri(d[0], d[3], bow, 180, 130, 80);
  oQuad(d[0], d[1], d[2], d[3], 180, 130, 80);
  // Kajuete
  oBox(X - Lh + 0.2f, Y - 0.75f, D, X - 0.5f, Y + 0.75f, D + 0.65f, 240, 240, 245);
  { float a[3] = {X - Lh + 0.5f, Y + 0.76f, D + 0.3f}, b[3] = {X - 0.8f, Y + 0.76f, D + 0.3f}, c[3] = {X - 0.8f, Y + 0.76f, D + 0.52f}, e[3] = {X - Lh + 0.5f, Y + 0.76f, D + 0.52f};
    oQuad(a, b, c, e, 40, 60, 90); }
  // Sonnenschirm
  const float sx = X + 1.75f, sy = Y - 0.95f;
  oBox(sx - 0.03f, sy - 0.03f, D, sx + 0.03f, sy + 0.03f, D + 1.45f, 220, 220, 220);
  for (int i = 0; i < 8; i++) {
    float a0 = i * 0.785f, a1 = (i + 1) * 0.785f;
    float tp[3] = {sx, sy, D + 1.6f}, p0[3] = {sx + cosf(a0) * 0.95f, sy + sinf(a0) * 0.95f, D + 1.25f}, p1[3] = {sx + cosf(a1) * 0.95f, sy + sinf(a1) * 0.95f, D + 1.25f};
    if (i % 2) oTri(tp, p0, p1, 240, 240, 240); else oTri(tp, p0, p1, 220, 50, 60);
  }
  // Liegestuhl
  const float cx = X + 0.9f;
  for (int i = 0; i < 4; i++) {
    float y0 = Y - 0.05f + i * 0.2f, y1 = y0 + 0.2f;
    float a[3] = {cx - 0.32f, y0, D + 0.26f}, b[3] = {cx + 0.32f, y0, D + 0.26f}, c[3] = {cx + 0.32f, y1, D + 0.26f}, e[3] = {cx - 0.32f, y1, D + 0.26f};
    if (i % 2) oQuad(a, b, c, e, 245, 245, 245); else oQuad(a, b, c, e, 250, 175, 40);
  }
  for (int i = 0; i < 3; i++) {
    float u0 = i / 3.0f, u1 = (i + 1) / 3.0f, ya = Y - 0.05f - u0 * 0.5f, yb = Y - 0.05f - u1 * 0.5f, za = D + 0.26f + u0 * 0.62f, zb = D + 0.26f + u1 * 0.62f;
    float a[3] = {cx - 0.32f, ya, za}, b[3] = {cx + 0.32f, ya, za}, c[3] = {cx + 0.32f, yb, zb}, e[3] = {cx - 0.32f, yb, zb};
    if (i % 2) oQuad(a, b, c, e, 245, 245, 245); else oQuad(a, b, c, e, 250, 175, 40);
  }
  oBox(cx - 0.3f, Y + 0.68f, D, cx - 0.25f, Y + 0.73f, D + 0.25f, 180, 130, 80); oBox(cx + 0.25f, Y + 0.68f, D, cx + 0.3f, Y + 0.73f, D + 0.25f, 180, 130, 80);
  // Spieler: derselbe Trader wie im HUD - blauer Anzug, weisser Kragen, rote Krawatte, barfuss
  const float hz = D + 0.3f;
  oBox(cx - 0.17f, Y + 0.3f, hz, cx - 0.03f, Y + 0.7f, hz + 0.1f, 30, 60, 140); oBox(cx + 0.03f, Y + 0.3f, hz, cx + 0.17f, Y + 0.7f, hz + 0.1f, 30, 60, 140);
  oBox(cx - 0.16f, Y + 0.7f, hz, cx - 0.04f, Y + 0.8f, hz + 0.12f, 235, 190, 150); oBox(cx + 0.04f, Y + 0.7f, hz, cx + 0.16f, Y + 0.8f, hz + 0.12f, 235, 190, 150);
  oBox(cx - 0.19f, Y + 0.05f, hz, cx + 0.19f, Y + 0.32f, hz + 0.14f, 30, 60, 140);
  { float a[3] = {cx - 0.2f, Y + 0.06f, hz + 0.15f}, b[3] = {cx + 0.2f, Y + 0.06f, hz + 0.15f}, c[3] = {cx + 0.2f, Y - 0.4f, D + 0.78f}, e[3] = {cx - 0.2f, Y - 0.4f, D + 0.78f};
    oQuad(a, b, c, e, 30, 60, 140); }
  {
    auto tp = [&](float x, float y, float* o) { o[0] = cx + x; o[1] = Y + y; o[2] = D + 0.78f - (y + 0.4f) / 0.46f * 0.33f + 0.012f; };
    auto dtri = [&](float x0, float y0, float x1, float y1, float x2, float y2, uint8_t r, uint8_t g, uint8_t bl) {
      float a[3], b[3], c[3]; tp(x0, y0, a); tp(x1, y1, b); tp(x2, y2, c);
      int n0 = nIsleExtra; oTri(a, b, c, r, g, bl); for (int k = n0; k < nIsleExtra; k++) isleExtra[k].deco = 1;
    };
    dtri(-0.1f, -0.4f, -0.03f, -0.4f, -0.02f, -0.2f, 240, 240, 240); dtri(0.03f, -0.4f, 0.1f, -0.4f, 0.02f, -0.2f, 240, 240, 240);   // Kragen
    dtri(-0.03f, -0.4f, 0.03f, -0.4f, 0, -0.08f, 210, 40, 40);   // Krawatte
  }
  // Kopf: Ringe aus einem Querschnitt mit breiter flacher Front, Kinn verjuengt, Hinterkopf abgeschraegt
  static const float SEC[8][2] = {{-0.198f, 0.171f}, {0.198f, 0.171f}, {0.216f, 0.036f}, {0.171f, -0.144f}, {0.072f, -0.189f}, {-0.072f, -0.189f}, {-0.171f, -0.144f}, {-0.216f, 0.036f}};
  static const float RINGS[4][2] = {{0.0f, 0.62f}, {0.126f, 0.9f}, {0.27f, 1.0f}, {0.405f, 0.97f}};
  const float hx = cx, hy = Y - 0.48f, hb = D + 0.8f;
  auto rp = [&](int r, int i, float* o) { o[0] = hx + SEC[i][0] * RINGS[r][1]; o[1] = hy + SEC[i][1] * RINGS[r][1]; o[2] = hb + RINGS[r][0]; };
  for (int r = 0; r < 3; r++) for (int i = 0; i < 8; i++) {
    int j = (i + 1) % 8; float a[3], b[3], c[3], e[3]; rp(r, i, a); rp(r, j, b); rp(r + 1, j, c); rp(r + 1, i, e);
    if (r == 2 && i >= 3 && i <= 5) oQuad(a, b, c, e, 60, 38, 20); else oQuad(a, b, c, e, 235, 190, 150);
  }
  { float tp[3] = {hx, hy - 0.02f, hb + 0.48f}; for (int i = 0; i < 8; i++) { float a[3], b[3]; rp(3, i, a); rp(3, (i + 1) % 8, b); oTri(a, b, tp, 60, 38, 20); } }
  oBox(hx - 0.07f, hy - 0.06f, D + 0.7f, hx + 0.07f, hy + 0.06f, hb + 0.05f, 235, 190, 150);
  oBox(hx - 0.26f, hy - 0.02f, hb + 0.18f, hx - 0.21f, hy + 0.06f, hb + 0.29f, 235, 190, 150); oBox(hx + 0.21f, hy - 0.02f, hb + 0.18f, hx + 0.26f, hy + 0.06f, hb + 0.29f, 235, 190, 150);
  const float fy = hy + 0.171f;
  { float n0[3] = {hx - 0.03f, fy, hb + 0.24f}, n1[3] = {hx + 0.03f, fy, hb + 0.24f}, nt[3] = {hx, fy + 0.06f, hb + 0.18f}, nb[3] = {hx, fy, hb + 0.17f};
    oTri(n0, n1, nt, 225, 170, 130); oTri(n0, nt, nb, 215, 160, 120); oTri(n1, nb, nt, 215, 160, 120); }
  auto deco = [&](float x0, float x1, float za0, float za1, float zb0, float zb1, float y, uint8_t r, uint8_t g, uint8_t bl) {   // Aufkleber: immer obenauf
    float a[3] = {x0, y, za0}, b[3] = {x1, y, zb0}, c[3] = {x1, y, zb1}, e[3] = {x0, y, za1};
    int n0 = nIsleExtra; oQuad(a, b, c, e, r, g, bl); for (int k = n0; k < nIsleExtra; k++) isleExtra[k].deco = 1;
  };
  deco(hx - 0.19f, hx + 0.19f, hb + 0.37f, hb + 0.405f, hb + 0.37f, hb + 0.405f, fy + 0.006f, 60, 38, 20);   // Pony
  deco(hx - 0.1f, hx + 0.1f, hb + 0.1f, hb + 0.122f, hb + 0.117f, hb + 0.14f, fy + 0.006f, 120, 30, 30);   // verschmitztes Grinsen
  deco(hx + 0.07f, hx + 0.11f, hb + 0.117f, hb + 0.14f, hb + 0.126f, hb + 0.153f, fy + 0.006f, 120, 30, 30);
  // Meme-Sonnenbrille ("Deal with it"), dauerhaft auf: Pixelbloecke mit weissen Glanzpixeln, so breit wie das Gesicht
  {
    static const int8_t RUNS[13][4] = {{0, 0, 21, 0}, {1, 0, 9, 0}, {1, 12, 21, 0}, {2, 1, 8, 0}, {2, 13, 20, 0}, {3, 2, 7, 0}, {3, 14, 19, 0}, {1, 2, 3, 1}, {1, 14, 15, 1}, {2, 3, 4, 1}, {2, 15, 16, 1}, {3, 4, 4, 1}, {3, 16, 16, 1}};
    const float gp = 0.44f / 22, gx = hx - 11 * gp, gz = hb + 0.36f;
    for (auto& rr : RUNS) {
      float y = fy + 0.009f + rr[3] * 0.004f, z0 = gz - (rr[0] + 1) * gp, z1 = gz - rr[0] * gp, xa = gx + rr[1] * gp, xb = gx + (rr[2] + 1) * gp;
      if (rr[3]) deco(xa, xb, z0, z1, z0, z1, y, 255, 255, 255); else deco(xa, xb, z0, z1, z0, z1, y, 8, 8, 10);
    }
  }
  // Arm mit Cocktail: alle 4 s ein Schluck, das Glas wird langsam leer
  float ph = fmodf(t, 4) / 4, lift = ph < 0.3f ? sinf(ph / 0.3f * 3.1416f) : 0;
  float sh[3] = {cx + 0.2f, Y - 0.25f, D + 0.72f}, rest[3] = {cx + 0.38f, Y + 0.1f, D + 0.5f}, mouth[3] = {hx + 0.03f, hy + 0.33f, hb - 0.07f}, hand[3];   // Hand vor dem Kinn, Glasrand auf Mundhoehe
  for (int i = 0; i < 3; i++) hand[i] = rest[i] + (mouth[i] - rest[i]) * lift;
  oRib(sh, hand, 0.04f, 30, 60, 140); oBox(hand[0] - 0.035f, hand[1] - 0.035f, hand[2] - 0.03f, hand[0] + 0.035f, hand[1] + 0.035f, hand[2] + 0.03f, 235, 190, 150);
  float g0[3] = {hand[0], hand[1], hand[2] + 0.02f}; const float gr = 0.07f, gh = 0.18f; float full = max(0.15f, 1 - t / OUTRO_LEN * 0.85f), lz = g0[2] + gh * full;
  for (int i = 0; i < 6; i++) {
    float a0 = i * 1.047f, a1 = (i + 1) * 1.047f;
    float p0[3] = {g0[0] + cosf(a0) * gr, g0[1] + sinf(a0) * gr, g0[2] + gh}, p1[3] = {g0[0] + cosf(a1) * gr, g0[1] + sinf(a1) * gr, g0[2] + gh};
    oTri(g0, p0, p1, 200, 230, 240);
    float lc[3] = {g0[0], g0[1], lz}, l0[3] = {g0[0] + cosf(a0) * gr * full, g0[1] + sinf(a0) * gr * full, lz}, l1[3] = {g0[0] + cosf(a1) * gr * full, g0[1] + sinf(a1) * gr * full, lz};
    oTri(lc, l0, l1, 255, 140, 40);
  }
  { float s0[3] = {g0[0] + 0.02f, g0[1], g0[2] + 0.1f}, s1[3] = {g0[0] + 0.06f, g0[1] - 0.02f, g0[2] + 0.3f}, lips[3] = {hx + 0.03f, hy + 0.185f, hb + 0.12f};
    for (int i = 0; i < 3; i++) s1[i] += (lips[i] - s1[i]) * lift;   // Strohhalm neigt sich beim Trinken zum Mund
    oRib(s0, s1, 0.01f, 255, 60, 160);
    float u0[3] = {g0[0] - 0.02f, g0[1], g0[2] + 0.3f}, u1[3] = {g0[0] - 0.12f, g0[1] + 0.03f, g0[2] + 0.22f}, u2[3] = {g0[0] + 0.07f, g0[1] - 0.03f, g0[2] + 0.22f}; oTri(u0, u1, u2, 255, 80, 180); }
}
static void startOutro() {
  outroT = 0; state = ST_OUTRO; nEnemies = nItems = 0;
  int n = 0;   // Insel-Yacht nicht doppelt zeichnen
  for (int i = 0; i < nIsleObj; i++) {
    const IsleTri& o = isleObj[i]; float mx = (o.v[0] + o.v[3] + o.v[6]) / 3 - OUTRO_X, my = (o.v[1] + o.v[4] + o.v[7]) / 3 - OUTRO_Y;
    if (mx * mx + my * my > 16) isleObj[n++] = o;
  }
  nIsleObj = n;
}
static void endIsland(const char* msg, bool all) {
  if (all) pnl += hardcore ? ISLE_ALL_BONUS * 3 / 2 : ISLE_ALL_BONUS;
  island = false; islandDone = true; isleAll = all; isleResult = msg;
  musicStop(); sfxPlay(all ? SFX_ID_KACHING : SFX_ID_BELL);
  startOutro();
}
static void islandCoin() {
  isleCoins++; pnl += hardcore ? ISLE_COIN_PTS * 3 / 2 : ISLE_COIN_PTS;
  char b[48]; snprintf(b, sizeof(b), "GOLD %d/%d - %s", isleCoins, isleCoinsPlaced, L("TAX-FREE!", "STEUERFREI!")); say(b, 1.2f);
  pickT = 0.25f; sfxPlay(SFX_ID_COINS);
}
static void updateIsland(float dt) {
  isleT -= dt;
  bullsAlive = bearsAlive = 0;
  for (int i = 0; i < nEnemies; i++) {
    Enemy& e = enemies[i];
    if (e.gone) continue;
    if (!e.alive) { e.deadT += dt; if (e.deadT > 7) e.gone = true; continue; }
    bearsAlive++; e.awake = true; hitSrcX = e.x; hitSrcY = e.y; hitSrcOk = true; updateEnemy(e, i, dt); hitSrcOk = false;
    if (state != ST_PLAY) return;
  }
  isleSpawnT -= dt;
  if (isleSpawnT <= 0) {   // Steuerfahnder-Baeren ruecken nach
    isleSpawnT = hardcore ? 4.5f : 6;
    float x, y; if (bearsAlive < (hardcore ? 14 : 12) && isleFreeCell(8, false, x, y)) { int i = spawnEnemy(E_BEAR, x, y); if (i >= 0) makePirate(enemies[i]); }
  }
  for (auto& p : projs) if (p.active && p.faction == F_SHOT) stepShot(p, dt);
  isleCashT -= dt;
  if (isleCashT <= 0) {   // ab und zu neues Bargeld, hoechstens 2 Buendel liegen gleichzeitig herum
    isleCashT = (hardcore ? 14 : 10) + frand() * 4;
    int cash = 0; for (int i = 0; i < nItems; i++) if (items[i].active && items[i].type == 0) cash++;
    float x, y; if (cash < 2 && isleFreeCell(5, false, x, y)) dropItem(x, y, 0);
  }
  if (isleCoins >= isleCoinsPlaced) endIsland(L("ALL COINS - TAX-FREE!", "ALLE MUENZEN - STEUERFREI!"), true);
  else if (isleT <= 0) endIsland(L("TIME'S UP - TAX INSPECTORS!", "ZEIT UM - DIE STEUERFAHNDUNG KOMMT"), false);
}

// ---- Low-Poly-Renderer
static const float ISLE_SUN[3] = {-0.62f / 1.0035f, 0.28f / 1.0035f, 0.73f / 1.0035f};   // tiefe Abendsonne im Westen
static const uint8_t ISLE_FOG[3] = {255, 160, 110};
static uint16_t isleSky[VH]; static bool isleSkyReady = false;
struct IsleItem { float d; float x[4], y[4]; uint16_t col; uint8_t n; int8_t spr; bool hurt; };
static constexpr int MAX_ISLE_ITEMS = 720;
static IsleItem isleItems[MAX_ISLE_ITEMS]; static uint16_t isleOrder[MAX_ISLE_ITEMS]; static int nIsleItems;

static void isleFill(float ax, float ay, float bx, float by, float cx, float cy, uint16_t col) {
  if (ay > by) { std::swap(ax, bx); std::swap(ay, by); }
  if (ay > cy) { std::swap(ax, cx); std::swap(ay, cy); }
  if (by > cy) { std::swap(bx, cx); std::swap(by, cy); }
  if (cy < 0 || ay >= VH || cy - ay < 0.01f) return;
  int y0 = max(0, (int)ceilf(ay)), y1 = min(VH - 1, (int)floorf(cy - 1e-4f));
  for (int y = y0; y <= y1; y++) {
    float xl = ax + (cx - ax) * (y - ay) / (cy - ay);
    float xr = y < by ? ax + (bx - ax) * (y - ay) / max(1e-4f, by - ay) : bx + (cx - bx) * (y - by) / max(1e-4f, cy - by);
    int x0 = (int)ceilf(min(xl, xr)), x1 = (int)floorf(max(xl, xr) - 1e-4f);
    if (x0 < 0) x0 = 0; if (x1 >= SW) x1 = SW - 1;
    uint16_t* row = fb + y * SW;
    for (int x = x0; x <= x1; x++) row[x] = col;
  }
}
static float isleEye;
static void isleCam(float x, float y, float z, float* o) {   // Welt -> Kamera (x rechts, y oben, z vorne)
  float dx = x - px, dy = y - py, rx = planeX / PLANE, ry = planeY / PLANE;
  o[0] = dx * rx + dy * ry; o[1] = z - isleEye; o[2] = dx * dirX + dy * dirY;
}
static void isleAddTri(const float* a, const float* b, const float* c, uint8_t r, uint8_t g, uint8_t bl, float bias = 0, bool bright = false) {
  if (nIsleItems >= MAX_ISLE_ITEMS) return;
  const float NEAR = 0.12f, lim = 1.3f * PLANE;
  float P[3][3]; isleCam(a[0], a[1], a[2], P[0]); isleCam(b[0], b[1], b[2], P[1]); isleCam(c[0], c[1], c[2], P[2]);
  if (P[0][2] < NEAR && P[1][2] < NEAR && P[2][2] < NEAR) return;
  bool allR = true, allL = true;
  for (int k = 0; k < 3; k++) { if (!(P[k][0] > P[k][2] * lim + 0.5f)) allR = false; if (!(-P[k][0] > P[k][2] * lim + 0.5f)) allL = false; }
  if (allR || allL) return;
  // flache Schattierung nach Sonnenstand, Dunst nach Entfernung
  float ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2], vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
  float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx, nl = sqrtf(nx * nx + ny * ny + nz * nz);
  if (nl < 1e-6f) nl = 1; nx /= nl; ny /= nl; nz /= nl; if (nz < 0) { nx = -nx; ny = -ny; nz = -nz; }
  float lum = (bright ? 0.82f : 0.55f) + (bright ? 0.28f : 0.5f) * max(0.0f, nx * ISLE_SUN[0] + ny * ISLE_SUN[1] + nz * ISLE_SUN[2]);
  float d = (P[0][2] + P[1][2] + P[2][2]) / 3 - bias, f = constrain((d - 4) / (ISLE_R - 4), 0.0f, 1.0f);
  int cr = r * lum + (ISLE_FOG[0] - r * lum) * f, cg = g * lum + (ISLE_FOG[1] - g * lum) * f, cb = bl * lum + (ISLE_FOG[2] - bl * lum) * f;
  // an der nahen Ebene abschneiden, dann projizieren
  float Q[4][3]; int n = 0;
  for (int k = 0; k < 3; k++) {
    const float* A = P[k]; const float* B = P[(k + 1) % 3];
    bool ain = A[2] >= NEAR, bin = B[2] >= NEAR;
    if (ain && n < 4) { Q[n][0] = A[0]; Q[n][1] = A[1]; Q[n][2] = A[2]; n++; }
    if (ain != bin && n < 4) { float t = (NEAR - A[2]) / (B[2] - A[2]); Q[n][0] = A[0] + (B[0] - A[0]) * t; Q[n][1] = A[1] + (B[1] - A[1]) * t; Q[n][2] = NEAR; n++; }
  }
  if (n < 3) return;
  IsleItem& it = isleItems[nIsleItems++];
  it.d = d; it.n = n; it.spr = -1; it.hurt = false;
  it.col = swp(c565(constrain(cr, 0, 255), constrain(cg, 0, 255), constrain(cb, 0, 255)));
  for (int k = 0; k < n; k++) { it.x[k] = SW / 2 + Q[k][0] / Q[k][2] * PROJ; it.y[k] = VH / 2 - Q[k][1] / Q[k][2] * PROJ; }
}
static void isleAddSprite(float x, float y, int spr, float sc, bool hurt, float zoff = 0) {
  if (nIsleItems >= MAX_ISLE_ITEMS) return;
  float c[3]; isleCam(x, y, isleGround(x, y) + zoff, c); if (c[2] < 0.2f) return;
  float size = PROJ / c[2] * sc; if (size < 1) return;
  IsleItem& it = isleItems[nIsleItems++];
  it.d = c[2]; it.spr = spr; it.hurt = hurt; it.n = 0;
  float bottom = VH / 2 - c[1] / c[2] * PROJ;
  it.x[0] = SW / 2 + c[0] / c[2] * PROJ - size / 2; it.y[0] = bottom - size; it.x[1] = size;
}
static void renderIsland() {
  for (int x = 0; x < SW; x++) zbuf[x] = 1e9f;   // Treffertest beim Schiessen: keine Waende
  const int hor = VH / 2;
  if (!isleSkyReady) {   // Sonnenuntergang oben, Meer unten
    for (int y = 0; y < VH; y++) {
      float c[3];
      if (y < hor) { float t = (float)y / hor;
        const float a[3] = {45, 30, 95}, m[3] = {200, 85, 125};
        for (int k = 0; k < 3; k++) c[k] = t < 0.55f ? a[k] + (m[k] - a[k]) * t / 0.55f : m[k] + (ISLE_FOG[k] - m[k]) * (t - 0.55f) / 0.45f;
      } else { float t = min(1.0f, (float)(y - hor) / (VH - hor) * 2.2f); const float s[3] = {25, 120, 150};
        for (int k = 0; k < 3; k++) c[k] = ISLE_FOG[k] + (s[k] - ISLE_FOG[k]) * t; }
      isleSky[y] = swp(c565(c[0], c[1], c[2]));
    }
    isleSkyReady = true;
  }
  for (int y = 0; y < VH; y++) { uint16_t* row = fb + y * SW; for (int x = 0; x < SW; x++) row[x] = isleSky[y]; }
  isleEye = isleEyeZ >= 0 ? isleEyeZ : isleGround(px, py) + 0.5f;
  { float c[3]; isleCam(px + ISLE_SUN[0] * 100, py + ISLE_SUN[1] * 100, isleEye, c);   // Sonne knapp ueber dem Horizont
    if (c[2] > 1) { int cx = SW / 2 + c[0] / c[2] * PROJ, r = 8; uint16_t sc = swp(c565(255, 225, 140));
      for (int y = -r; y <= r; y++) for (int x = -r; x <= r; x++) if (x * x + y * y <= r * r) { int X = cx + x, Y = hor - 10 + y; if (X >= 0 && X < SW && Y >= 0 && Y < hor) fb[Y * SW + X] = sc; } } }
  { uint16_t gl = swp(c565(255, 225, 175));   // Glitzern auf dem Meer
    for (int k = 0; k < 60; k++) { int y = hor + 1 + rnd() % (VH - hor - 1), x = rnd() % SW; if (fb[y * SW + x] == isleSky[y]) fb[y * SW + x] = gl; } }
  nIsleItems = 0;
  // Gelaende (reine Wasserfelder liefert der Hintergrund)
  int ci = (int)floorf(px), cj = (int)floorf(py);
  for (int j = max(ISLE_MIN, cj - ISLE_R); j < min(ISLE_MIN + ISLE_N - 1, cj + ISLE_R); j++)
    for (int i = max(ISLE_MIN, ci - ISLE_R); i < min(ISLE_MIN + ISLE_N - 1, ci + ISLE_R); i++) {
      float h00 = isleVert(i, j), h10 = isleVert(i + 1, j), h01 = isleVert(i, j + 1), h11 = isleVert(i + 1, j + 1);
      if (h00 <= 0 && h10 <= 0 && h01 <= 0 && h11 <= 0) continue;
      if ((i + 0.5f - px) * dirX + (j + 0.5f - py) * dirY < -1.5f) continue;   // hinter der Kamera
      float v00[3] = {(float)i, (float)j, max(0.0f, h00)}, v10[3] = {(float)i + 1, (float)j, max(0.0f, h10)};
      float v01[3] = {(float)i, (float)j + 1, max(0.0f, h01)}, v11[3] = {(float)i + 1, (float)j + 1, max(0.0f, h11)};
      int nn = ((i * 7 + j * 13) % 5 + 5) % 5 * 6;
      const float* T[2][3] = {{v00, v10, v11}, {v00, v11, v01}};
      for (int t = 0; t < 2; t++) {
        float ah = (T[t][0][2] + T[t][1][2] + T[t][2][2]) / 3;
        if (ah < 0.04f) isleAddTri(T[t][0], T[t][1], T[t][2], 70, 205, 200);
        else if (ah < 0.2f) isleAddTri(T[t][0], T[t][1], T[t][2], 235, 210, 150);
        else if (ah < 0.95f) isleAddTri(T[t][0], T[t][1], T[t][2], 80 + nn, 170 + nn, 75);
        else isleAddTri(T[t][0], T[t][1], T[t][2], 150, 140, 125);
      }
    }
  // Figuren und Muenzen zuerst einplanen, damit sie bei vollem Puffer nicht fehlen
  for (int i = 0; i < nEnemies; i++) { const Enemy& e = enemies[i]; if (e.gone) continue;
    isleAddSprite(e.x, e.y, e.alive ? (e.pir >= 0 ? S_PIR0 + e.pir * 2 : TYPES[e.type].spr) + ((e.attackAnim > 0 || fmodf(e.anim, 0.6f) < 0.3f) ? 1 : 0) : S_DEAD, e.alive ? TYPES[e.type].scale : 0.6f, e.alive && e.hurt > 0); }
  for (int i = 0; i < nItems; i++) {
    const Item& it = items[i]; if (!it.active || dist2Player(it.x, it.y) > ISLE_R * ISLE_R) continue;
    if (it.type < 2) { isleAddSprite(it.x, it.y, it.type ? S_DIV : S_CASH, 0.5f, false); continue; }
    float z = isleGround(it.x, it.y) + 0.35f + sinf(gameTime * 3 + it.x) * 0.06f, a = gameTime * 2.5f + it.x * 3, ca = cosf(a) * 0.22f, sa = sinf(a) * 0.22f;
    float ctr[3] = {it.x, it.y, z};
    for (int k = 0; k < 8; k++) {   // drehende Goldmuenze als Achteck
      float t0 = k * 0.785f, t1 = (k + 1) * 0.785f;
      float p0[3] = {it.x + ca * cosf(t0), it.y + sa * cosf(t0), z + 0.22f * sinf(t0)}, p1[3] = {it.x + ca * cosf(t1), it.y + sa * cosf(t1), z + 0.22f * sinf(t1)};
      isleAddTri(ctr, p0, p1, 255, k % 2 ? 205 : 180, k % 2 ? 50 : 30);
    }
  }
  for (auto& p : projs) if (p.active && p.faction == F_SHOT) isleAddSprite(p.x, p.y, S_NOTE, 0.3f, false, 0.3f);
  for (int k = 0; k < nIsleExtra; k++) { const IsleTri& o = isleExtra[k]; isleAddTri(&o.v[0], &o.v[3], &o.v[6], o.r, o.g, o.b, o.deco ? 0.4f : 0, true); }
  for (int k = 0; k < nIsleObj; k++) { const IsleTri& o = isleObj[k];
    float mx = (o.v[0] + o.v[3] + o.v[6]) / 3 - px, my = (o.v[1] + o.v[4] + o.v[7]) / 3 - py;
    if (mx * mx + my * my < ISLE_R * ISLE_R) isleAddTri(&o.v[0], &o.v[3], &o.v[6], o.r, o.g, o.b); }
  for (int i = 0; i < nIsleItems; i++) isleOrder[i] = i;
  std::sort(isleOrder, isleOrder + nIsleItems, [](uint16_t a, uint16_t b) { return isleItems[a].d > isleItems[b].d; });
  static const uint16_t white = swp(c565(255, 255, 255));
  for (int o = 0; o < nIsleItems; o++) {
    const IsleItem& it = isleItems[isleOrder[o]];
    if (it.spr >= 0) {
      int size = (int)it.x[1], left = (int)it.x[0], top = (int)it.y[0]; if (size < 1) continue;
      const uint16_t* tex = sprTex[it.spr]; float step = (float)TEX / size;
      for (int x = max(0, left); x < min(SW, left + size); x++) { int tx = (int)((x - left) * step);
        for (int y = max(0, top); y < min(VH, top + size); y++) { uint16_t c = tex[(int)((y - top) * step) * TEX + tx]; if (c) fb[y * SW + x] = it.hurt ? white : c; } }
      continue;
    }
    for (int k = 1; k + 1 < it.n; k++) isleFill(it.x[0], it.y[0], it.x[k], it.y[k], it.x[k + 1], it.y[k + 1], it.col);
  }
}

// ---- Abspann nach dem Endboss (kurz, mit ENTER ueberspringbar)
static constexpr float CREDITS_LEN = 15;
static void renderCredits() {
  drawChartBackground(c565(20, 90, 45), c565(40, 40, 40));
  canvas.fillRect(0, 0, SW, SH, c565(0, 0, 0));
  static const struct { const char* en; const char* de; uint8_t size; uint16_t col; } CL[] = {
    {"MARGIN CALL", "MARGIN CALL", 2, c565(255, 210, 50)}, {"", "", 1, 0}, {"A STOCK MARKET SHOOTER BY", "EIN BOERSEN-SHOOTER VON", 1, c565(160, 160, 180)}, {"WOLFGANG RABENSTEIN", "WOLFGANG RABENSTEIN", 1, c565(255, 255, 255)}, {"", "", 1, 0},
    {"LIVE QUOTES: YAHOO FINANCE", "LIVE-KURSE: YAHOO FINANCE", 1, c565(200, 200, 215)}, {"HARDWARE: M5STACK CARDPUTER ADV", "HARDWARE: M5STACK CARDPUTER ADV", 1, c565(200, 200, 215)}, {"", "", 1, 0},
    {"NO BEARS WERE SERIOUSLY HARMED.", "KEIN BAER WURDE ERNSTHAFT VERLETZT.", 1, c565(160, 160, 180)}, {"NOT INVESTMENT ADVICE.", "KEINE ANLAGEBERATUNG.", 1, c565(160, 160, 180)}, {"", "", 1, 0},
    {"THE MARKET IS FREE AGAIN.", "DER MARKT IST WIEDER FREI.", 1, c565(80, 255, 130)}, {"BUT WHERE TO PUT ALL THAT", "ABER WOHIN JETZT MIT", 1, c565(80, 255, 130)}, {"MONEY NOW...?", "DEM GANZEN GELD...?", 1, c565(80, 255, 130)}, {"", "", 1, 0},
    {"OFF TO", "AB INS", 2, c565(255, 210, 50)}, {"TAX HAVEN!", "STEUERPARADIES!", 2, c565(255, 210, 50)},
  };
  int total = 0; for (auto& l : CL) total += l.size * 8 + 4;
  float t = min(1.0f, creditsT / (CREDITS_LEN - 2));
  int y = SH - (int)(t * (SH / 2 + total - 16));   // laeuft von unten nach oben, die letzte Zeile bleibt in der Mitte stehen
  for (auto& l : CL) { const char* s = L(l.en, l.de); if (s[0] && y > -20 && y < SH) centerText(s, y, l.size, l.col); y += l.size * 8 + 4; }
}

static void updateGame(float dt) {
  gameTime += dt;
  musicSetPanic(hp > 0 && hp < 30);
  fireCd -= dt; flashT -= dt; hurtT -= dt; pickT -= dt; msgT -= dt; slowT -= dt;
  aiFaceT -= dt; crashT -= dt; glitchT -= dt; invertT -= dt; quakeT -= dt; stampHitT -= dt; hitDirT -= dt;
  for (auto& c : soundCues) c.t -= dt;
  { float vx = (px - lastPx) / max(dt, 0.001f), vy = (py - lastPy) / max(dt, 0.001f);
    if (fabsf(vx) > 6 || fabsf(vy) > 6) vx = vy = 0;   // Sprung (Levelstart)
    pvx = pvx * 0.7f + vx * 0.3f; pvy = pvy * 0.7f + vy * 0.3f; lastPx = px; lastPy = py; }
  musicSetMood(bullRun() ? 112 : bearMkt() ? 90 : crashStage == 2 ? 82 : 100);   // Bullrun schnellere, Baerenmarkt langsamere Musik
  if (fakeT > 0) { fakeT -= dt; if (fakeT <= 0 || ((int)(gameTime * 4) % 2)) buildTicker(); }
  if (manipT > 0) {
    manipT -= dt;
    if (dumpAt > 0 && manipT <= dumpAt) {
      dumpAt = 0; manipChg = -manipChg;
      char b[48]; snprintf(b, sizeof(b), "ALGO: DUMP! %s %+.2f%%", L("PRICE", "KURS"), manipChg); say(b, 2.5f); sfxPlay(SFX_ID_GLITCH);
      aiFaceT = 2.2f; glitchT = max(glitchT, 0.6f);
    }
    marketEffect();
  }
  marketVisuals();

  auto& kb = M5Cardputer.Keyboard;
  auto ks = kb.keysState();
  float mv = 2.8f * playerSpeedMul() * dt, rot = 2.4f * dt;
  float fwd = 0, side = 0, turn = 0;
  if (kb.isKeyPressed('e')) fwd += 1;
  if (kb.isKeyPressed('s')) fwd -= 1;
  if (kb.isKeyPressed('a')) turn -= 1;
  if (kb.isKeyPressed('d')) turn += 1;
  if (kb.isKeyPressed('w')) side -= 1;
  if (kb.isKeyPressed('r')) side += 1;
  if (invertT > 0) { turn = -turn; side = -side; }
  if (ks.space || ks.enter || M5Cardputer.BtnA.isPressed()) shoot();

  if (turn != 0) {
    float a = turn * rot, c = cosf(a), s = sinf(a);
    float odx = dirX; dirX = dirX * c - dirY * s; dirY = odx * s + dirY * c;
    float opx = planeX; planeX = planeX * c - planeY * s; planeY = opx * s + planeY * c;
  }
  if (fwd != 0 || side != 0) {
    float sx = -dirY, sy = dirX;
    float mx = (dirX * fwd + sx * side), my = (dirY * fwd + sy * side);
    float l = sqrtf(mx * mx + my * my); mx = mx / l * mv; my = my / l * mv;
    tryMove(mx, my);
    bobT += dt * 10;
  }

  // Ausweichen: dieselbe Richtung zweimal schnell antippen
  static const char DODGE_KEYS[4] = {'e', 's', 'w', 'r'};
  static const float DODGE_F[4] = {1, -1, 0, 0}, DODGE_S[4] = {0, 0, -1, 1};
  static bool keyWasDown[4];
  static uint32_t lastTapMs[4];
  uint32_t nowMs = millis();
  for (int i = 0; i < 4; i++) {
    bool isDown = kb.isKeyPressed(DODGE_KEYS[i]);
    if (isDown && !keyWasDown[i]) {   // neu gedrueckt
      if (lastTapMs[i] && nowMs - lastTapMs[i] < 250 && dodgeCd <= 0 && dodgeT <= 0) {
        float f = DODGE_F[i], s = invertT > 0 ? -DODGE_S[i] : DODGE_S[i];
        dodgeX = dirX * f - dirY * s; dodgeY = dirY * f + dirX * s;
        dodgeT = 0.25f; dodgeCd = 0.85f; lastTapMs[i] = 0;
        sfxPlay(SFX_ID_DODGE);
      } else lastTapMs[i] = nowMs;
    }
    keyWasDown[i] = isDown;
  }
  dodgeCd -= dt;
  if (dodgeT > 0) {
    float v = min(dodgeT, dt) * 7.0f * (slowT > 0 ? 0.5f : 1.0f);
    tryMove(dodgeX * v, dodgeY * v);
    bobT += dt * 20;
    dodgeT -= dt;
  }
  if (state != ST_PLAY) return;

  // Items
  for (int i = 0; i < nItems; i++) {
    Item& it = items[i];
    if (!it.active || it.slide > 0) continue;
    if (dist2Player(it.x, it.y) > 0.3f) continue;
    if (it.type == 2) { it.active = false; islandCoin(); continue; }   // Goldmuenze in der Steueroase
    if (it.type == 3) {
      it.active = false; hasKey = true; pickT = 0.25f; sfxPlay(SFX_ID_KEYCARD);
      char b[48]; snprintf(b, sizeof(b), L("%s KEYCARD! THE VAULT IS OPEN", "%s KEYCARD! DER TRESOR IST OFFEN"), L(KEY_EN[level % 5], KEY_DE[level % 5])); say(b, 2.5f);
      continue;
    }
    if (it.crash) { it.active = false; pickT = 0.25f; pnl += hardcore ? 225 : 150; ammo = min(99, ammo + 5); say(L("FIRE SALE +$150", "NOTVERKAUF +$150"), 1); sfxPlay(SFX_ID_KACHING); continue; }
    if (it.type == 5) {   // Liquiditaetsschub: viel Cash, etwas Depot
      if (ammo >= 99) continue;
      it.active = false; pickT = 0.25f; ammo = min(99, ammo + (hardcore ? 45 : 60)); hp = min(100, hp + (hardcore ? 5 : 10)); pnl += hardcore ? 150 : 100; sfxPlay(SFX_ID_LIQ);
      say(hardcore ? L("LIQUIDITY! CASH +45 PORTFOLIO +5%", "LIQUIDITAETSSCHUB! CASH+45 DEPOT+5%") : L("LIQUIDITY! CASH +60 PORTFOLIO +10%", "LIQUIDITAETSSCHUB! CASH+60 DEPOT+10%"), 2.5f);
      continue;
    }
    if (it.type == 4) {   // Refinanzierungspaket: Depot voll, dazu Cash
      if (hp >= 100) continue;
      it.active = false; pickT = 0.25f; hp = 100; ammo = min(99, ammo + (hardcore ? 15 : 20)); pnl += hardcore ? 150 : 100; sfxPlay(SFX_ID_REFI);
      say(hardcore ? L("REFINANCED! PORTFOLIO 100% CASH +15", "REFINANZIERT! DEPOT 100% CASH +15") : L("REFINANCED! PORTFOLIO 100% CASH +20", "REFINANZIERT! DEPOT 100% CASH +20"), 2.5f);
      continue;
    }
    if (it.type == 0) { if (ammo >= 99) continue; ammo = min(99, ammo + (hardcore ? 7 : 10)); say(hardcore ? "CASHFLOW +7" : "CASHFLOW +10", 1.2f); }
    else { if (hp >= 100) continue; hp = min(100, hp + (hardcore ? 15 : 20)); say(hardcore ? L("DIVIDEND PAID +15%", "DIVIDENDE AUSGESCHUETTET +15%") : L("DIVIDEND PAID +20%", "DIVIDENDE AUSGESCHUETTET +20%"), 1.2f); }
    it.active = false; pickT = 0.25f; pnl += hardcore ? 37 : 25; sfxPlay(it.type == 0 ? SFX_ID_KACHING : SFX_ID_COINS);
  }

  if (island) { updateIsland(dt); return; }

  // Intraday-Replay: Momentum des echten Kursverlaufs steuert den Nachschub
  replayT -= dt;
  const Market& m = levelMarket();
  if (replayT <= 0 && m.n > 3) {
    replayT = 3;
    replayIdx = (replayIdx + 1) % m.n;
    int i0 = max(0, replayIdx - 3);
    momentum = (m.series[replayIdx] - m.series[i0]) / m.series[i0] * 100.0f;
  }

  updatePhase(dt); updateNav(dt); levelTime += dt;
  // Gegner
  if (easy) {   // Kleinanleger: nur wenige Gegner greifen gleichzeitig an (je nach Level), die naechsten zuerst
    const int slots = attackSlots(); int best[6] = {-1, -1, -1, -1, -1, -1}; float bd[6] = {1e9f, 1e9f, 1e9f, 1e9f, 1e9f, 1e9f};
    for (int i = 0; i < nEnemies; i++) {
      Enemy& e = enemies[i]; e.atk = false;
      if (e.gone || !e.alive || !e.awake || e.pir >= 0 || TYPES[e.type].boss) continue;
      if ((e.faction == F_BULL || e.faction == F_BEAR) && e.target != -1) continue;
      float q = dist2Player(e.x, e.y);
      for (int k = 0; k < slots; k++) if (q < bd[k]) { for (int m = slots - 1; m > k; m--) { bd[m] = bd[m - 1]; best[m] = best[m - 1]; } bd[k] = q; best[k] = i; break; }
    }
    for (int k = 0; k < slots; k++) if (best[k] >= 0) enemies[best[k]].atk = true;
  }
  bullsAlive = bearsAlive = 0;
  for (int i = 0; i < nEnemies; i++) {
    Enemy& e = enemies[i];
    if (e.gone) continue;
    if (!e.alive) { e.deadT += dt; if (e.deadT > 7) e.gone = true; continue; }
    if (e.faction == F_BULL) bullsAlive++; else if (e.faction == F_BEAR) bearsAlive++;
    hitSrcX = e.x; hitSrcY = e.y; hitSrcOk = true; updateEnemy(e, i, dt); hitSrcOk = false;
    if (state != ST_PLAY) return;
  }

  // Allgegenwaertige Bullen und Baeren: Nachschub nach Marktlage
  spawnT -= dt;
  if (spawnT <= 0) {
    spawnT = 7 + frand() * 5;
    if (bullsAlive + bearsAlive < 9) {
      float p = constrain(pBullBase + momentum * 1.5f + (phaseT > 0 ? phase * 0.25f : 0.0f), 0.1f, 0.9f);
      spawnAnimal(frand() < p);
    }
  }

  // vereinzeltes Bargeld abseits des Spielers, damit man nie ganz ohne Munition dasteht;
  // wer fast leer ist, findet schneller etwas, mehr als 2 solcher Scheine liegen nie herum
  cashT -= dt;
  if (cashT <= 0) {
    cashT = (ammo <= 3 ? 15 + frand() * 10 : 45 + frand() * 30) * (hardcore ? 1.6f : 1.0f) * (bullRun() ? 0.5f : 1.0f);
    int strays = 0;
    for (int i = 0; i < nItems; i++) if (items[i].active && items[i].stray) strays++;
    float x, y;
    if (strays < 2 && findHiddenCell(x, y, 6)) dropItem(x, y, 0, true);
  }

  // KI-Ereignisse
  aiT -= dt;
  if (aiT <= 0) {
    if (crashT > 0) aiT = crashT + 1;   // waehrend eines Flash Crash kein weiteres Ereignis
    else { aiT = (level == NUM_LEVELS - 1 ? 22 + frand() * 12 : 40 + frand() * 30) * (hardcore ? 0.7f : 1.0f); aiEvent(); }
  }

  // Projektile
  for (auto& p : projs) {
    if (!p.active) continue;
    if (p.faction == F_SHOT) { stepShot(p, dt); continue; }
    for (int k = 0; k < 2 && p.active; k++) {   // in Teilschritten; Wand auf der Teilstrecke stoppt den Schuss
      const float ox = p.x, oy = p.y; p.x += p.dx * dt * 0.5f; p.y += p.dy * dt * 0.5f;
      if (segBlocked(ox, oy, p.x, p.y)) p.active = false;
      else if (dist2Player(p.x, p.y) < 0.12f) break;
    }
    if (!p.active) continue;
    if (dist2Player(p.x, p.y) < 0.12f && !segBlocked(p.x, p.y, px, py)) {
      p.active = false; hitSrcX = px - p.dx; hitSrcY = py - p.dy; hitSrcOk = true;   // Schuss: Richtung, aus der er kam
      damagePlayer(p.dmg, p.spr == S_LASER ? L("ALGO ORDER EXECUTED!", "ALGO-ORDER AUSGEFUEHRT!") : p.spr == S_PROJ_OIL ? L("OIL BARREL! ENERGY CRISIS", "OELFASS! ENERGIEKRISE") : "MARGIN CALL!");
      if (p.spr == S_PROJ_OIL) slowT = 2;
      continue;
    }
    if (p.faction == F_BULL || p.faction == F_BEAR)
      for (int i = 0; i < nEnemies; i++) {
        Enemy& e = enemies[i];
        if (!e.alive || !hostileFactions(p.faction, e.faction)) continue;
        float dx = e.x - p.x, dy = e.y - p.y;
        if (dx * dx + dy * dy < 0.15f) { hurtEnemy(e, 2, false); p.active = false; break; }
      }
  }

  // Depotwert-Verlauf
  histT -= dt;
  if (histT <= 0) {
    histT = 0.4f;
    if (histN < (int)sizeof(hist)) hist[histN++] = hp;
    else { memmove(hist, hist + 1, sizeof(hist) - 1); hist[sizeof(hist) - 1] = hp; }
  }
}

// ---------------------------------------------------------------- Rendering
static uint16_t rowColor[VH];

static void initRowColors() {
  for (int y = 0; y < VH; y++) {
    if (y < VH / 2) {
      float t = (float)y / (VH / 2);
      rowColor[y] = swp(c565(40 - 30 * t, 36 - 28 * t, 44 - 32 * t));
    } else {
      float t = (float)(y - VH / 2) / (VH / 2);
      rowColor[y] = swp(c565(18 + 100 * t, 6 + 18 * t, 8 + 20 * t));
    }
  }
}

#define WALL_LOOP(EXPR) for (int y = ys; y < ye; y++, out += SW, pos += step) { uint16_t c = tex[(((pos >> 16) & (TEX - 1)) << 5) + tx]; c = (EXPR); *out = (uint16_t)((c >> 8) | (c << 8)); }

static void renderWalls() {
  for (int y = 0; y < VH; y++) { uint16_t c = rowColor[y]; uint16_t* row = fb + y * SW; for (int x = 0; x < SW; x++) row[x] = c; }
  int scroll = (int)(gameTime * 10) & (TEX - 1);
  for (int x = 0; x < SW; x++) {
    float cam = 2.0f * x / SW - 1.0f;
    float rdx = dirX + planeX * cam, rdy = dirY + planeY * cam;
    int mx = (int)px, my = (int)py;
    float ddx = rdx == 0 ? 1e30f : fabsf(1 / rdx), ddy = rdy == 0 ? 1e30f : fabsf(1 / rdy);
    int stepX, stepY; float sdx, sdy;
    if (rdx < 0) { stepX = -1; sdx = (px - mx) * ddx; } else { stepX = 1; sdx = (mx + 1.0f - px) * ddx; }
    if (rdy < 0) { stepY = -1; sdy = (py - my) * ddy; } else { stepY = 1; sdy = (my + 1.0f - py) * ddy; }
    int sideHit = 0; char cell = '#';
    for (int i = 0; i < 64; i++) {
      if (sdx < sdy) { sdx += ddx; mx += stepX; sideHit = 0; } else { sdy += ddy; my += stepY; sideHit = 1; }
      if (mx < 0 || my < 0 || mx >= MAP_W || my >= MAP_H) break;
      if (world[my][mx] != '.') { cell = world[my][mx]; break; }
    }
    float perp = sideHit == 0 ? sdx - ddx : sdy - ddy;
    if (perp < 0.05f) perp = 0.05f;
    zbuf[x] = perp;
    int lineH = (int)(PROJ / perp);
    int y0 = VH / 2 - lineH / 2, y1 = VH / 2 + lineH / 2;
    float wx = sideHit == 0 ? py + perp * rdy : px + perp * rdx;
    wx -= floorf(wx);
    int tx = (int)(wx * TEX) & (TEX - 1);
    if ((sideHit == 0 && rdx > 0) || (sideHit == 1 && rdy < 0)) tx = TEX - 1 - tx;
    int t = texOf(cell);
    if (t == T_TICKER) tx = (tx + scroll) & (TEX - 1);
    int shade = perp < 3 ? 0 : perp < 6 ? 1 : perp < 9 ? 2 : 3;
    if (sideHit && shade < 3) shade++;
    const uint16_t* tex = wallTex[t];
    int32_t step = (TEX << 16) / (lineH ? lineH : 1);
    int ys = max(0, y0), ye = min(VH, y1);
    int32_t pos = (ys - y0) * step;
    uint16_t* out = fb + ys * SW + x;
    switch (shade) {
      case 0: WALL_LOOP(c); break;
      case 1: WALL_LOOP(c - ((c >> 2) & 0x39E7)); break;
      case 2: WALL_LOOP((c >> 1) & 0x7BEF); break;
      default: WALL_LOOP((c >> 2) & 0x39E7); break;
    }
  }
}

struct Spr { float x, y; int16_t tex; float scale; bool hurt; float dist; float z; };   // z: Sprunghoehe (Oelschlick)
static Spr sprs[MAX_E + MAX_I + MAX_P];

static void renderSprites() {
  int n = 0;
  for (int i = 0; i < nEnemies; i++) {
    Enemy& e = enemies[i];
    if (e.gone) continue;
    const TypeInfo& ti = TYPES[e.type];
    int t; float sc = ti.scale;
    if (!e.alive) { t = S_DEAD; sc = 0.6f; }
    else {
      bool alt = e.attackAnim > 0 || e.chargeT > 0;
      if (!alt && (e.type == E_BEAR || e.type == E_BUBBLE || e.type == E_SLICK || e.type == E_BOT || e.type == E_ALGO))
        alt = fmodf(e.anim, 0.6f) < 0.3f && (e.awake || e.type == E_BOT || e.type == E_ALGO);
      t = ti.spr + (alt ? 1 : 0);
    }
    sprs[n++] = {e.x, e.y, (int16_t)t, sc, e.alive && e.hurt > 0, 0, e.alive && e.hopT > 0 ? sinf(3.1416f * (1 - e.hopT / 0.55f)) * 0.42f : 0.0f};
  }
  for (int i = 0; i < nItems; i++) if (items[i].active) sprs[n++] = {items[i].x, items[i].y, (int16_t)(items[i].type == 5 ? S_LIQ : items[i].type == 4 ? S_REFI : items[i].type == 3 ? S_KEY : items[i].type ? S_DIV : S_CASH), items[i].flash > 0 ? 0.5f + 0.15f * fabsf(sinf(gameTime * 10)) : 0.5f, false, 0};
  for (auto& p : projs) if (p.active) sprs[n++] = {p.x, p.y, (int16_t)p.spr, 0.35f, false, 0};

  for (int i = 0; i < n; i++) { float dx = sprs[i].x - px, dy = sprs[i].y - py; sprs[i].dist = dx * dx + dy * dy; }
  for (int i = 1; i < n; i++) { Spr k = sprs[i]; int j = i - 1; while (j >= 0 && sprs[j].dist < k.dist) { sprs[j + 1] = sprs[j]; j--; } sprs[j + 1] = k; }

  float invDet = 1.0f / (planeX * dirY - dirX * planeY);
  static const uint16_t hurtCol = swp(c565(255, 255, 255));
  for (int i = 0; i < n; i++) {
    Spr& s = sprs[i];
    float sx = s.x - px, sy = s.y - py;
    float tX = invDet * (dirY * sx - dirX * sy), tY = invDet * (-planeY * sx + planeX * sy);
    if (tY < 0.15f) continue;
    int scrX = (int)(SW / 2 * (1 + tX / tY));
    int full = (int)(PROJ / tY);
    int size = (int)(full * s.scale); if (size < 1) continue;
    bool floating = s.tex == S_PROJ || s.tex == S_PROJ_OIL || s.tex == S_LASER || s.tex == S_BOT0 || s.tex == S_BOT1 || s.tex == S_NOTE;
    int vMove = (floating ? 0 : (full - size) / 2) - (int)(full * s.z);
    int top = VH / 2 - size / 2 + vMove, left = scrX - size / 2;
    int32_t step = (TEX << 16) / size;
    int xs = max(0, left), xe = min(SW, left + size);
    int ys = max(0, top), ye = min(VH, top + size);
    const uint16_t* tex = sprTex[s.tex];
    for (int x = xs; x < xe; x++) {
      if (tY >= zbuf[x]) continue;
      int tx = ((x - left) * step) >> 16;
      int32_t pos = (ys - top) * step;
      uint16_t* out = fb + ys * SW + x;
      for (int y = ys; y < ye; y++, out += SW, pos += step) {
        uint16_t c = tex[((pos >> 16) << 5) + tx];
        if (c) *out = s.hurt ? hurtCol : c;
      }
    }
  }
}

static void renderGlitch() {
  if (glitchT <= 0) return;
  int rows = glitchT > 1 ? 14 : 6;
  for (int k = 0; k < rows; k++) {
    int y = rnd() % VH, h = 1 + rnd() % 4, off = (int)(rnd() % 30) - 15;
    for (int yy = y; yy < min(VH, y + h); yy++) {
      uint16_t* row = fb + yy * SW;
      if (off > 0) memmove(row + off, row, (SW - off) * 2); else memmove(row, row - off, (SW + off) * 2);
      if (rnd() % 3 == 0) for (int x = 0; x < SW; x += 2) row[x] ^= 0xE007;
    }
  }
}

// Boersencrash: ganzer Bildschirm blinkt rot, bleibt aber durchsichtig (wie am PC)
static void renderCrashTint() {
  if (crashStage != 1 && crashStage != 2) return;
  float sn = sinf(gameTime * 4), k = crashStage == 1 ? 0.2f : 0.1f + 0.3f * sn * sn;
  for (int i = 0; i < SW * VH; i++) {
    uint16_t c = swp(fb[i]); int r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
    r += (int)((28 - r) * k); g = (int)(g * (1 - k)); b = (int)(b * (1 - k));
    fb[i] = swp((max(r, 0) << 11) | (g << 5) | b);
  }
}
// Bildschirm-Richtung zu einem Punkt (Einheitsvektor): vorne = oben, rechts = rechts
static void screenDir(float x, float y, float& ox, float& oy) {
  float sx = x - px, sy = y - py, l = sqrtf(sx * sx + sy * sy) + 1e-4f, pl = sqrtf(planeX * planeX + planeY * planeY) + 1e-4f;
  ox = (sx * planeX + sy * planeY) / (l * pl); oy = -(sx * dirX + sy * dirY) / l;
}
// Trefferrichtung: roter Balken am Bildrand in Richtung des Angreifers (vorne = oben, hinten = unten, schraeg = Ecke)
static void renderHitDir() {
  if (hitDirT <= 0) return;
  float x, y; screenDir(hitDirX, hitDirY, x, y);
  uint16_t c = hitDirT > 0.5f || ((int)(gameTime * 8) % 2) ? c565(255, 30, 30) : c565(150, 0, 0);
  for (int k = 0; k < 3; k++) {
    if (y < -0.38f) canvas.drawFastHLine(0, k, SW, c);
    if (y > 0.38f) canvas.drawFastHLine(0, VH - 1 - k, SW, c);
    if (x < -0.38f) canvas.drawFastVLine(k, 0, VH, c);
    if (x > 0.38f) canvas.drawFastVLine(SW - 1 - k, 0, VH, c);
  }
}
// Geraeuschanzeige: gelbes Randstueck in Richtung des Geraeuschs; nah kurz und hell, fern laenger und blasser
static void renderSounds() {
  if (island || state != ST_PLAY) return;
  for (auto& c : soundCues) {
    if (c.t <= 0) continue;
    float x, y; screenDir(c.x, c.y, x, y);
    float d = sqrtf(dist2Player(c.x, c.y)), a = atan2f(y, x), co = cosf(a), si = sinf(a), cx = SW / 2.0f, cy = VH / 2.0f;
    float t = min((cx - 2) / max(0.01f, fabsf(co)), (cy - 2) / max(0.01f, fabsf(si)));
    int ex = (int)(cx + co * t), ey = (int)(cy + si * t), len = 10 + (int)(min(d, 14.0f) * 2.5f);
    uint16_t col = d < 3 ? c565(255, 240, 160) : d < 8 ? c565(255, 200, 50) : c565(170, 130, 30);
    bool horiz = fabsf(si) * cx > fabsf(co) * cy;   // trifft oben/unten -> waagrechtes Stueck, sonst senkrechtes
    for (int k = 0; k < 2; k++) {
      if (horiz) canvas.drawFastHLine(constrain(ex - len / 2, 0, SW - len), ey < cy ? k : VH - 1 - k, len, col);
      else canvas.drawFastVLine(ex < cx ? k : SW - 1 - k, constrain(ey - len / 2, 0, VH - len), len, col);
    }
  }
}
static void renderHurt() {   // roter Bildblitz
  if (hurtT <= 0) return;
  float k = min(1.0f, hurtT / 0.35f) * min(1.0f, 0.45f + hurtAmt / 40.0f) * 0.65f;
  for (int i = 0; i < SW * VH; i++) {
    uint16_t c = swp(fb[i]); int r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
    r += (int)((31 - r) * k); g = (int)(g * (1 - k)); b = (int)(b * (1 - k));
    fb[i] = swp((r << 11) | (g << 5) | b);
  }
}
static void renderFlicker() {   // ALGO-9000 schlaegt zu: Licht flackert in den ersten ~0,7 s des Angriffs
  if (aiFaceT < 1.5f || ((int)(aiFaceT * 14) % 3)) return;
  for (int i = 0; i < SW * VH; i++) fb[i] = swp((swp(fb[i]) >> 1) & 0x7BEF);
}

static void renderWeapon() {
  int bob = (int)(fabsf(sinf(bobT)) * 4);
  int by = VH - 42 + bob + (flashT > 0 ? 3 : 0);
  if (flashT > 0) {
    canvas.fillCircle(120, by - 4, 11, c565(255, 170, 0));
    canvas.fillCircle(120, by - 4, 6, c565(255, 255, 180));
    canvas.setTextColor(c565(40, 160, 40)); canvas.setTextSize(1);
    canvas.drawString("$", 104, by - 14); canvas.drawString("$", 132, by - 12);
  }
  // Geldkoffer-Kanone: Aktenkoffer mit Chromlauf, Banknotenbuendel als Magazin und kleinem Kursdisplay, in Trader-Haenden
  const float s = 0.72f; const int cx = SW / 2, B = VH + bob;
  auto X = [&](float x) { return (int)(cx + x * s); }; auto Y = [&](float y) { return (int)(B + y * s); };
  auto R = [&](float x, float y, float w, float h, uint16_t c) { canvas.fillRect(X(x), Y(y), max(1, (int)(w * s + 0.5f)), max(1, (int)(h * s + 0.5f)), c); };
  const uint16_t gold = c565(225, 180, 55), dark = c565(25, 20, 18);
  for (int i = 0; i < 3; i++) { R(-29 + i * 3, -38 + i * 3, 14, 18, c565(30, 110, 45)); R(-28 + i * 3, -37 + i * 3, 12, 16, c565(90, 180, 95)); }
  R(-24, -29, 14, 3, c565(250, 210, 60));
  R(12, -31, 14, 3, dark); R(12, -31, 3, 8, dark); R(23, -31, 3, 8, dark);
  canvas.fillRoundRect(X(-34), Y(-24), (int)(68 * s), (int)(28 * s), 4, c565(42, 31, 26));
  R(-32, -22, 64, 2, c565(90, 66, 50));
  R(-27, -21, 6, 5, gold); R(21, -21, 6, 5, gold);
  // Kanone: verchromte Halterung im Kofferdeckel, konischer Lauf mit Messingringen, Zufuehrung aus dem Geldscheinmagazin
  canvas.fillTriangle(X(-17), Y(-22), X(17), Y(-22), X(12), Y(-32), c565(95, 98, 112)); canvas.fillTriangle(X(-17), Y(-22), X(12), Y(-32), X(-12), Y(-32), c565(95, 98, 112));
  canvas.fillTriangle(X(-17), Y(-22), X(-12), Y(-32), X(-9), Y(-32), c565(165, 168, 182));
  for (int k = 0; k < 3; k++) { canvas.drawLine(X(-19) + k - 1, Y(-28), X(-19) + k - 1, Y(-38), c565(200, 160, 40)); canvas.drawLine(X(-19), Y(-41) + k - 1, X(-8), Y(-42) + k - 1, c565(200, 160, 40)); }
  { int b0 = Y(-32), w0 = (int)(10 * s), w1 = (int)(7 * s);
    canvas.fillTriangle(cx - w0, b0, cx + w0, b0, cx + w1, by, c565(58, 60, 70)); canvas.fillTriangle(cx - w0, b0, cx + w1, by, cx - w1, by, c565(58, 60, 70));
    canvas.fillTriangle(cx - w0 + 1, b0, cx - 2, b0, cx - w1 + 1, by, c565(160, 162, 175));
    for (int f = 1; f <= 2; f++) { int yy = b0 + (by - b0) * f / 3; canvas.fillRect(cx - w0 + f, yy, 2 * (w0 - f), 2, c565(200, 160, 40)); }
    canvas.fillRect(cx - w1 - 1, by - 2, 2 * w1 + 3, 4, c565(220, 175, 50)); canvas.fillRect(cx - 3, by - 1, 7, 1, c565(15, 15, 18));
    if (flashT <= 0) canvas.fillRect(cx - 3, by - 3, 7, 2, c565(110, 200, 110)); }   // naechster Geldschein schaut heraus
  R(-11, -11, 22, 9, c565(5, 18, 10));
  { static const int8_t PT[6][2] = {{-9, -4}, {-5, -6}, {-2, -5}, {2, -8}, {5, -7}, {9, -10}};
    for (int i = 0; i < 5; i++) canvas.drawLine(X(PT[i][0]), Y(PT[i][1]), X(PT[i + 1][0]), Y(PT[i + 1][1]), c565(80, 255, 120)); }
  for (int m = -1; m <= 1; m += 2) {   // Haende mit Anzugaermel und Manschette
    uint16_t suit = c565(30, 45, 100);
    canvas.fillTriangle(X(m * 66), Y(6), X(m * 46), Y(6), X(m * 36), Y(-8), suit);
    canvas.fillTriangle(X(m * 66), Y(6), X(m * 36), Y(-8), X(m * 48), Y(-16), suit);
    canvas.fillEllipse(X(m * 40), Y(-10), 3, 4, c565(240, 240, 240));
    canvas.fillEllipse(X(m * 34), Y(-13), 5, 4, c565(235, 190, 150));
  }
  uint16_t ch = c565(80, 255, 120);
  canvas.drawFastHLine(SW / 2 - 4, VH / 2, 3, ch); canvas.drawFastHLine(SW / 2 + 2, VH / 2, 3, ch);
  canvas.drawFastVLine(SW / 2, VH / 2 - 4, 3, ch); canvas.drawFastVLine(SW / 2, VH / 2 + 2, 3, ch);
}

static void drawTickerBar(int y) {
  canvas.fillRect(0, y, SW, 9, c565(0, 0, 0));
  canvas.setTextSize(1); canvas.setTextDatum(top_left);
  int w = tickChars * 6;
  int x0 = SW - ((int)(gameTime * 32) % (w + SW));
  for (int pass = 0; pass < 2; pass++) {
    int x = x0 + pass * w;
    if (pass && x0 + w >= SW) break;
    for (int i = 0; i < nTickSegs; i++) {
      int sw = strlen(tickSegs[i].t) * 6;
      if (x < SW && x + sw > 0) { canvas.setTextColor(tickSegs[i].col); canvas.drawString(tickSegs[i].t, x, y + 1); }
      x += sw;
    }
  }
}

static void renderHUD() {
  drawTickerBar(0);
  canvas.fillRect(0, VH, SW, SH - VH, c565(8, 12, 26));
  canvas.drawFastHLine(0, VH, SW, c565(40, 200, 110));
  canvas.setTextSize(1);
  canvas.setTextDatum(top_left);
  canvas.setTextColor(c565(130, 140, 170));
  canvas.drawString(L("PORTFOLIO", "DEPOT"), 3, VH + 3);
  canvas.drawString("CASH", 108, VH + 3);
  uint16_t hc = hp > 60 ? c565(60, 230, 100) : hp > 30 ? c565(255, 200, 40) : c565(255, 60, 60);
  canvas.setTextColor(hc); canvas.setTextSize(2);
  char b[32];
  snprintf(b, sizeof(b), "%d%%", hp); canvas.drawString(b, 3, VH + 12);
  canvas.setTextColor(ammo > 5 ? c565(120, 230, 140) : c565(255, 80, 80));
  snprintf(b, sizeof(b), "%d", ammo); canvas.drawString(b, 108, VH + 12);
  if (hasKey && !island) {   // Keycard im Besitz
    const uint8_t* kc = KEY_COL[level % 5];
    canvas.fillRect(133, VH + 2, 12, 8, c565(kc[0] / 2, kc[1] / 2, kc[2] / 2)); canvas.fillRect(134, VH + 3, 10, 6, c565(kc[0], kc[1], kc[2]));
    canvas.drawFastHLine(134, VH + 4, 10, c565(240, 240, 245)); canvas.fillRect(135, VH + 6, 3, 2, c565(230, 185, 60));
  }
  canvas.setTextSize(1);
  // P&L
  canvas.setTextColor(c565(80, 255, 130));
  snprintf(b, sizeof(b), "P&L+%ld", pnl); canvas.drawString(b, 148, VH + 3);
  // Bullen-vs-Baeren-Balken
  int bx = 148, bw = 88, tot = bullsAlive + bearsAlive;
  int gw = tot ? bw * bullsAlive / tot : bw / 2;
  canvas.fillRect(bx, VH + 12, gw, 5, c565(30, 200, 80));
  canvas.fillRect(bx + gw, VH + 12, bw - gw, 5, c565(220, 40, 40));
  if (phaseT <= 0 && !island) {   // Marktstimmung: Zeiger wandert Richtung Bullrun (links) bzw. Baerenmarkt (rechts)
    int sx = bx + (int)(bw * (1 - constrain(sentiment, -1.0f, 1.0f)) / 2);
    uint16_t sc = fabsf(sentiment) > 0.6f && ((int)(gameTime * 4) % 2) ? (sentiment > 0 ? c565(80, 255, 130) : c565(255, 80, 80)) : c565(255, 255, 255);
    canvas.fillTriangle(sx - 3, VH + 7, sx + 3, VH + 7, sx, VH + 11, sc);
  }
  // Level-Markt
  const Market& m = levelMarket();
  canvas.setTextColor(manipT > 0 && ((int)(gameTime * 4) % 2) ? c565(255, 60, 200) : levelChg >= 0 ? c565(80, 255, 130) : c565(255, 80, 80));
  if (crashStage) { canvas.setTextColor(crashStage == 3 ? c565(80, 255, 130) : c565(255, 80, 80)); snprintf(b, sizeof(b), "%s %ds", crashStage == 3 ? L("RALLY", "RALLYE") : "CRASH", (int)ceilf(crashStT)); }
  else if (phaseT > 0) { canvas.setTextColor(phase > 0 ? c565(80, 255, 130) : c565(255, 80, 80)); snprintf(b, sizeof(b), "%s %ds", phase > 0 ? "BULLRUN" : L("BEAR TRAP", "BAERENFALLE"), (int)ceilf(phaseT)); }
  else if (manipT > 0) snprintf(b, sizeof(b), "%s ALGO%+.1f%%", level == NUM_LEVELS - 1 ? "MIX" : m.label, levelChg);   // manipulierter Spielkurs
  else snprintf(b, sizeof(b), "%s %s%.2f%%%s", level == NUM_LEVELS - 1 ? "MIX" : m.label, levelChg >= 0 ? "+" : "", levelChg, m.live ? "" : "*");
  canvas.drawString(b, 148, VH + 19);
  if (streamViewers()) { canvas.fillCircle(232, VH + 22, 3, c565(255, 40, 40)); }   // LIVE-Punkt
  // Sparkline Depotwert
  int gx = 52, gy = VH + 4, gwid = 50, gh = 20;
  canvas.drawRect(gx - 1, gy - 1, gwid + 2, gh + 2, c565(30, 40, 70));
  for (int i = 1; i < histN; i++) {
    int x0 = gx + (i - 1) * gwid / (int)sizeof(hist), x1 = gx + i * gwid / (int)sizeof(hist);
    int y0 = gy + gh - 1 - hist[i - 1] * (gh - 1) / 100, y1 = gy + gh - 1 - hist[i] * (gh - 1) / 100;
    canvas.drawLine(x0, y0, x1, y1, hist[i] >= hist[i - 1] ? c565(60, 230, 100) : c565(255, 60, 60));
  }
  // Rahmen-Effekte
  if (hurtT > 0 && hitDirT <= 0) { canvas.drawRect(0, 0, SW, VH, c565(255, 0, 0)); canvas.drawRect(1, 1, SW - 2, VH - 2, c565(255, 0, 0)); canvas.drawRect(2, 2, SW - 4, VH - 4, c565(160, 0, 0)); }
  if (pickT > 0) { canvas.drawRect(0, 0, SW, VH, c565(60, 255, 120)); canvas.drawRect(1, 1, SW - 2, VH - 2, c565(60, 255, 120)); }
  if (phaseT > 0 && !island && ((int)(gameTime * 3) % 2)) {   // Marktphase: pulsierender gruener bzw. roter Rand
    uint16_t pc = phase > 0 ? c565(60, 220, 110) : c565(230, 50, 50);
    canvas.drawRect(0, 0, SW, VH, pc); canvas.drawRect(1, 1, SW - 2, VH - 2, pc); canvas.drawRect(2, 2, SW - 4, VH - 4, pc);
  }
  if (crashStage == 1 || crashStage == 2) {   // Boersencrash: rote Notbeleuchtung (Rahmen) und Margin-Call-Anzeige
    uint16_t rc = (int)(gameTime * 2) % 2 ? c565(200, 0, 0) : c565(110, 0, 0);
    for (int k = 0; k < 4; k++) canvas.drawRect(k, k, SW - 2 * k, VH - 2 * k, rc);
    char cb[48];
    if (crashStage == 1) { snprintf(cb, sizeof(cb), "%s %d", L("TRADING HALT", "HANDELSSTOPP"), (int)ceilf(crashStT)); centerText(cb, 30, 1, c565(255, 80, 80)); }
    else {
      snprintf(cb, sizeof(cb), "MARGIN CALL $%ld/$%ld %ds", min(marginPaid, marginDue), marginDue, (int)ceilf(crashStT)); centerText(cb, 14, 1, c565(255, 200, 40));
      int bw = 120, bx = (SW - bw) / 2, fw = (int)(bw * min(1.0f, (float)marginPaid / max(1L, marginDue)));
      canvas.fillRect(bx, 24, bw, 4, c565(60, 10, 10)); canvas.fillRect(bx, 24, fw, 4, c565(255, 200, 40));
    }
  }
  if (slowT > 0) canvas.drawRect(0, 9, SW, VH - 9, c565(60, 30, 90));
  if (dodgeT > 0)   // Bewegungsstreifen beim Ausweichen
    for (int i = 0; i < 8; i++) { int y = 14 + (i * 23 + (int)(gameTime * 400)) % (VH - 20), x = i % 2 ? SW - 34 + i * 3 : 4 + i * 3; canvas.drawFastHLine(x, y, 18, c565(200, 220, 240)); }
  // KI-Gesicht
  if (aiFaceT > 0 && ((int)(gameTime * 8) % 4)) {
    canvas.fillRoundRect(SW - 30, 12, 26, 26, 4, c565(15, 15, 24));
    canvas.fillCircle(SW - 17, 25, 9, c565(110, 0, 0));
    canvas.fillCircle(SW - 17, 25, 6, c565(255, 30, 20));
    canvas.fillCircle(SW - 17, 25, 2, c565(255, 230, 140));
  }
  // Nachrichten
  if (msgT > 0) {
    canvas.fillRect(0, 10, SW, 11, c565(0, 0, 0));
    canvas.setTextDatum(top_center);
    canvas.setTextColor(strncmp(msg, "ALGO", 4) == 0 ? c565(255, 80, 80) : c565(255, 220, 60));
    canvas.drawString(msg, SW / 2, 12);
    canvas.setTextDatum(top_left);
  }
}

static void drawChartBackground(uint16_t up, uint16_t down) {
  canvas.fillScreen(c565(6, 10, 20));
  for (int y = 10; y < SH; y += 15) canvas.drawFastHLine(0, y, SW, c565(18, 26, 44));
  // echter Verlauf des S&P 500 als Hintergrund-Kerzen
  const Market& m = markets[M_SPX];
  float lo = 1e30f, hi = -1e30f;
  for (int i = 0; i < m.n; i++) { lo = min(lo, m.series[i]); hi = max(hi, m.series[i]); }
  if (hi - lo < 1e-6f) hi = lo + 1;
  int K = SW / 8 + 1, off = (int)(gameTime * 20) % 8;
  for (int k = 0; k < K; k++) {
    int idx = ((k + (int)(gameTime * 20) / 8) % max(1, m.n - 1));
    float o = m.series[idx], c = m.series[min(idx + 1, m.n - 1)];
    int yo = 120 - (int)((o - lo) / (hi - lo) * 95), yc = 120 - (int)((c - lo) / (hi - lo) * 95);
    int top = min(yo, yc), bot = max(yo, yc) + 2;
    int x = k * 8 - off;
    uint16_t col = c >= o ? up : down;
    canvas.drawFastVLine(x + 2, top - 3, bot - top + 6, col);
    canvas.fillRect(x, top, 5, bot - top, col);
  }
}

static void centerText(const char* s, int y, int size, uint16_t col) {
  canvas.setTextDatum(top_center); canvas.setTextSize(size); canvas.setTextColor(col);
  canvas.drawString(s, SW / 2, y);
  canvas.setTextDatum(top_left);
}

static void renderTitle() {
  drawChartBackground(c565(20, 90, 45), c565(100, 25, 25));
  canvas.fillRect(0, 3, SW, 30, c565(0, 0, 0));
  centerText("MARGIN CALL", 6, 3, c565(255, 210, 50));
  canvas.fillRect(8, 36, SW - 16, 96, c565(0, 0, 0));
  centerText(L("BULLS VS BEARS VS YOU VS AI", "BULLEN VS BAEREN VS DU VS KI"), 38, 1, c565(80, 255, 130));
  canvas.setTextSize(1); canvas.setTextDatum(top_left);
  char b[64];
  for (int i = 0; i < M_COUNT; i++) {
    const Market& m = markets[i];
    canvas.setTextColor(c565(200, 200, 215));
    snprintf(b, sizeof(b), "%-3s %10.2f", m.label, m.price); canvas.drawString(b, 14, 50 + i * 9);
    canvas.setTextColor(m.chg >= 0 ? c565(80, 255, 130) : c565(255, 80, 80));
    snprintf(b, sizeof(b), "%s%.2f%%", m.chg >= 0 ? "+" : "", m.chg); canvas.drawString(b, 104, 50 + i * 9);
    canvas.setTextColor(m.live ? c565(255, 60, 60) : c565(120, 120, 140));
    canvas.drawString(m.live ? "LIVE" : "SIM", 160, 50 + i * 9);
  }
  canvas.setTextColor(c565(150, 160, 190));
  if (netConnected()) {
    snprintf(b, sizeof(b), "%s %s  STREAM:", L("WIFI", "WLAN"), netSSID()); canvas.drawString(b, 14, 89);
    canvas.setTextColor(c565(120, 220, 255));
    snprintf(b, sizeof(b), "http://%s  margincall.local", netIP()); canvas.drawString(b, 14, 98);
  } else {
    canvas.drawString(netHasCreds() ? L("WIFI: CONNECTING...", "WLAN: VERBINDE...") : L("WIFI: OFF (SIMULATED QUOTES)", "WLAN: AUS (KURSE SIMULIERT)"), 14, 89);
    canvas.drawString(L("[W] SET UP WIFI", "[W] WLAN EINRICHTEN"), 14, 98);
  }
  canvas.setTextColor(c565(160, 160, 180));
  canvas.drawString(L("E/S A/D W/R SPACE  P PAUSE  M MUSIC", "E/S A/D W/R SPACE  P PAUSE  M MUSIK"), 14, 109);
  int phase = ((int)(gameTime * 2)) % 4;
  if (phase == 1) centerText(L("[E] SMALL INVESTOR [ENTER] SPECULATOR", "[E] KLEINANLEGER [ENTER] SPEKULANT"), 121, 1, c565(255, 255, 255));
  else if (phase == 3) centerText(L("[X] DAY TRADER", "[X] DAY-TRADER"), 121, 1, c565(255, 120, 120));
  else if (phase == 2) centerText(L("[L] DEUTSCH", "[L] ENGLISH"), 121, 1, c565(120, 200, 255));
  else centerText("MADE BY WOLFGANG RABENSTEIN  V" FW_VERSION, 121, 1, c565(110, 120, 150));
}

static void renderEndScreen(const char* title, const char* sub, uint16_t col, bool bull) {
  drawChartBackground(bull ? c565(20, 90, 45) : c565(40, 40, 40), bull ? c565(40, 40, 40) : c565(110, 25, 25));
  canvas.fillRect(0, 18, SW, 92, c565(0, 0, 0));
  centerText(title, 24, 2, col);
  centerText(sub, 46, 1, c565(220, 220, 230));
  char b[48];
  snprintf(b, sizeof(b), "P&L: +$%ld", pnl); centerText(b, 60, 1, c565(80, 255, 130));
  snprintf(b, sizeof(b), "%s: %d", L("LIQUIDATED BY YOU", "VON DIR LIQUIDIERT"), kills); centerText(b, 71, 1, c565(200, 200, 210));
  snprintf(b, sizeof(b), "%s: %d", L("MARKET CLEANUP", "MARKTBEREINIGUNG"), marketKills); centerText(b, 81, 1, c565(200, 200, 210));
  centerText("MADE BY WOLFGANG RABENSTEIN", 112, 1, c565(110, 120, 150));
  if (((int)(gameTime * 2)) % 2) centerText(L("[ENTER] CONTINUE", "[ENTER] WEITER"), 96, 1, c565(255, 255, 255));
}

// ---------------------------------------------------------------- Marktdaten laden
static void allocCanvas() {
  canvas.setColorDepth(16);
  canvas.setPsram(false);
  canvas.createSprite(SW, SH);
  fb = (uint16_t*)canvas.getBuffer();
}

static void fetchProgress(int idx, bool ok) {
  auto& d = M5Cardputer.Display;
  d.setTextSize(1); d.setTextDatum(top_left);
  d.setTextColor(ok ? c565(80, 255, 130) : c565(255, 120, 80), c565(0, 0, 0));
  char b[40];
  snprintf(b, sizeof(b), "%-4s %s", markets[idx].label, ok ? "OK" : L("ERROR - SIMULATED", "FEHLER - SIMULIERT"));
  d.drawString(b, 40, 52 + idx * 12);
}

// Speicher fuer TLS freimachen: Canvas kurz loeschen, direkt aufs Display zeichnen
static void fetchMarketsNow() {
  if (!netConnected()) return;
  canvas.deleteSprite(); fb = nullptr;
  auto& d = M5Cardputer.Display;
  d.fillScreen(c565(0, 0, 0));
  d.setTextDatum(top_center); d.setTextSize(1); d.setTextColor(c565(255, 210, 50));
  d.drawString(L("LOADING MARKET DATA...", "MARKTDATEN WERDEN GELADEN..."), SW / 2, 30);
  marketsFetch(fetchProgress);
  delay(300);
  allocCanvas();
}

// ---------------------------------------------------------------- WLAN-Menue
static char wifiNames[12][33]; static int8_t wifiRssi[12]; static bool wifiSec[12];
static int wifiCount, wifiSel;
static char passBuf[65]; static int passLen; static bool passShow;

static void renderWifiList() {
  canvas.fillScreen(c565(6, 10, 20));
  centerText(L("CHOOSE WIFI", "WLAN WAEHLEN"), 4, 1, c565(255, 210, 50));
  canvas.setTextSize(1); canvas.setTextDatum(top_left);
  if (!wifiCount) { centerText(L("NO NETWORKS FOUND", "KEINE NETZE GEFUNDEN"), 50, 1, c565(255, 120, 80)); }
  int first = max(0, min(wifiSel - 4, wifiCount - 9));
  for (int i = first; i < min(wifiCount, first + 9); i++) {
    int y = 18 + (i - first) * 11;
    if (i == wifiSel) canvas.fillRect(4, y - 1, SW - 8, 10, c565(30, 80, 50));
    canvas.setTextColor(c565(220, 220, 230));
    char b[48]; snprintf(b, sizeof(b), "%-26.26s %4d %s", wifiNames[i], wifiRssi[i], wifiSec[i] ? "*" : "");
    canvas.drawString(b, 8, y);
  }
  canvas.setTextColor(c565(140, 150, 180));
  canvas.drawString(L(";/. SELECT  ENTER OK  ` BACK  F FORGET", ";/. WAEHLEN  ENTER OK  ` ZURUECK  F VERGESSEN"), 4, 125);
}

static void renderWifiPass() {
  canvas.fillScreen(c565(6, 10, 20));
  centerText(L("PASSWORD FOR", "PASSWORT FUER"), 10, 1, c565(255, 210, 50));
  centerText(wifiNames[wifiSel], 22, 1, c565(120, 220, 255));
  canvas.drawRect(10, 44, SW - 20, 16, c565(80, 90, 120));
  char shown[66];
  for (int i = 0; i < passLen; i++) shown[i] = (passShow || i == passLen - 1) ? passBuf[i] : '*';
  shown[passLen] = 0;
  canvas.setTextDatum(top_left); canvas.setTextColor(c565(255, 255, 255)); canvas.setTextSize(1);
  const char* vis = shown + max(0, passLen - 35);
  canvas.drawString(vis, 14, 48);
  if (((int)(gameTime * 3)) % 2) canvas.fillRect(14 + strlen(vis) * 6, 47, 5, 9, c565(80, 255, 130));
  canvas.setTextColor(c565(140, 150, 180));
  canvas.drawString(L("ENTER CONNECT   DEL DELETE", "ENTER VERBINDEN   DEL LOESCHEN"), 14, 76);
  canvas.drawString(L("TAB SHOW/HIDE   ` BACK", "TAB ANZEIGEN/VERBERGEN   ` ZURUECK"), 14, 88);
  canvas.drawString(L("(THE PASSWORD IS NOT STREAMED)", "(DAS PASSWORT WIRD NICHT GESTREAMT)"), 14, 106);
}

static void startWifiScan() {
  canvas.fillScreen(c565(0, 0, 0));
  centerText(L("SEARCHING WIFI NETWORKS...", "SUCHE WLAN-NETZE..."), 60, 1, c565(255, 210, 50));
  canvas.pushSprite(0, 0);
  wifiCount = netScan(wifiNames, wifiRssi, wifiSec, 12);
  wifiSel = 0;
  state = ST_WIFI_LIST;
}

static void tryWifiConnect() {
  canvas.fillScreen(c565(0, 0, 0));
  centerText(L("CONNECTING TO", "VERBINDE MIT"), 50, 1, c565(255, 210, 50));
  centerText(wifiNames[wifiSel], 62, 1, c565(120, 220, 255));
  canvas.pushSprite(0, 0);
  passBuf[passLen] = 0;
  bool ok = netConnect(wifiNames[wifiSel], passBuf, 15000);
  memset(passBuf, 0, sizeof(passBuf)); passLen = 0;
  if (ok) { fetchMarketsNow(); buildTicker(); state = ST_TITLE; sfxPlay(SFX_ID_KACHING); }
  else {
    canvas.fillScreen(c565(0, 0, 0));
    centerText(L("CONNECTION FAILED", "VERBINDUNG FEHLGESCHLAGEN"), 55, 1, c565(255, 80, 80));
    canvas.pushSprite(0, 0);
    sfxPlay(SFX_ID_DECLINED);
    delay(1500);
    state = ST_WIFI_LIST;
  }
}

// ---------------------------------------------------------------- Main
static uint32_t lastMs;
static bool fetchedAtBoot = false;

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setBrightness(160);
  audioInit();
  rngState ^= esp_random() | 1;
  { Preferences lp; if (lp.begin("mcgame", true)) { langDe = lp.getBool("de", false); lp.end(); } }
  applyLang();
  marketsSimulate();
  netInit();
  allocCanvas();
  genWalls();
  genSprites();
  initRowColors();
  buildTicker();
  musicStart();
  lastMs = millis();
}

void loop() {
  M5Cardputer.update();
  audioUpdate();
  uint32_t now = millis();
  float dt = (now - lastMs) / 1000.0f; lastMs = now;
  if (dt > 0.1f) dt = 0.1f;

  // Tasten-Ereignisse (neu gedrueckt) fuer Menues
  auto& kb = M5Cardputer.Keyboard;
  bool fresh = kb.isChange() && kb.isPressed();
  Keyboard_Class::KeysState ks;
  if (fresh) ks = kb.keysState();
  auto hit = [&](char c) { if (!fresh) return false; for (char w : ks.word) if (w == c || w == toupper(c)) return true; return false; };
  bool enterHit = fresh && ks.enter;

  if (state != ST_WIFI_PASS && state != ST_WIFI_LIST && hit('m')) { musicToggle(); say(musicEnabled() ? L("MUSIC ON", "MUSIK AN") : L("MUSIC OFF", "MUSIK AUS"), 1.0f); }

  switch (state) {
    case ST_TITLE:
      gameTime += dt;
      if (!fetchedAtBoot && netConnected()) { fetchedAtBoot = true; fetchMarketsNow(); buildTicker(); }
      renderTitle();
      if (enterHit || hit('x') || hit('e')) {   // E = Kleinanleger, Enter = Spekulant, X = Day-Trader
        hardcore = !enterHit && hit('x'); easy = !enterHit && !hardcore; newGame(); state = ST_PLAY; fireCd = 0.4f; sfxPlay(SFX_ID_KACHING);
        if (hardcore) say(L("DAY TRADER: ONLY ONE LIFE", "DAY-TRADER: NUR EIN LEBEN"), 3.0f);
        else if (!easy) say(L("SPECULATOR: 5 LIVES", "SPEKULANT: 5 LEBEN"), 2.5f);
        else say(L("SMALL INVESTOR: UNLIMITED LIVES", "KLEINANLEGER: UNBEGRENZT LEBEN"), 2.5f);
      }
      else if (hit('w')) startWifiScan();
      else if (hit('l')) {   // Sprache umschalten und merken
        langDe = !langDe; applyLang(); buildTicker(); sfxPlay(SFX_ID_HIT);
        Preferences lp; if (lp.begin("mcgame", false)) { lp.putBool("de", langDe); lp.end(); }
      }
      break;
    case ST_PLAY:
      if (hit('p')) { state = ST_PAUSE; musicSetPaused(true); break; }
      updateGame(dt);
      if (island) renderIsland(); else { renderWalls(); renderSprites(); renderGlitch(); renderFlicker(); }
      renderCrashTint(); renderHurt(); renderWeapon(); renderHitDir(); renderSounds(); renderHUD();
      if (hurtT > 0) { char hb[16]; snprintf(hb, sizeof(hb), "v -%d%%", hurtAmt); centerText(hb, 30 + (int)((0.35f - hurtT) * 60), 2, c565(255, 50, 50)); }
      if (island) {   // HUD in der Steueroase: Gold und Zeit statt Bullen/Baeren und Kurs
        canvas.fillRect(146, VH + 10, SW - 146, 17, c565(8, 12, 26));
        canvas.fillRect(148, VH + 12, 88, 5, c565(58, 48, 16));
        canvas.fillRect(148, VH + 12, 88 * isleCoins / max(1, isleCoinsPlaced), 5, c565(255, 200, 40));
        char b[24]; snprintf(b, sizeof(b), "GOLD %d/%d %d:%02d", isleCoins, isleCoinsPlaced, (int)max(0.0f, isleT) / 60, (int)max(0.0f, isleT) % 60);
        canvas.setTextSize(1); canvas.setTextDatum(top_left); canvas.setTextColor(isleT < 15 ? c565(255, 80, 80) : c565(255, 210, 50));
        canvas.drawString(b, 148, VH + 19);
        // grosser Timer oben rechts im Bild, in den letzten 15 s rot blinkend
        int tl = (int)ceilf(max(0.0f, isleT));
        snprintf(b, sizeof(b), "%d:%02d", tl / 60, tl % 60);
        canvas.fillRect(SW - 62, 23, 60, 30, c565(0, 0, 0));
        canvas.setTextDatum(top_right); canvas.setTextSize(2);
        canvas.setTextColor(isleT < 15 ? (((int)(gameTime * 4) % 2) ? c565(255, 255, 255) : c565(255, 60, 60)) : c565(255, 210, 50));
        canvas.drawString(b, SW - 5, 25);
        canvas.setTextSize(1); canvas.setTextColor(c565(255, 200, 40));
        snprintf(b, sizeof(b), "GOLD %d/%d", isleCoins, isleCoinsPlaced); canvas.drawString(b, SW - 5, 43);
        canvas.setTextDatum(top_left);
      }
      break;
    case ST_PAUSE:
      canvas.fillRect(30, 36, 180, 40, c565(0, 0, 0));
      centerText(L("TRADING HALTED", "HANDEL AUSGESETZT"), 42, 1, c565(255, 210, 50));
      centerText(L("[P] RESUME", "[P] WEITER"), 54, 1, c565(220, 220, 230));
      if (netConnected()) { char b[40]; snprintf(b, sizeof(b), "STREAM: http://%s", netIP()); centerText(b, 65, 1, c565(120, 220, 255)); }
      if (hit('p')) { state = ST_PLAY; musicSetPaused(false); }
      break;
    case ST_LEVELDONE:
      gameTime += dt;
      renderEndScreen("CLOSING BELL!", L("LEVEL CLEARED - PRICE RISING", "LEVEL GESCHAFFT - KURS STEIGT"), c565(80, 255, 130), true);
      if (enterHit) {
        level++;
        if (marketsAgeMs() > 120000) fetchMarketsNow();
        loadLevel(level); if (hardcore) hp = max(hp, 35); else { ammo = max(ammo, 20); hp = max(hp, 40); } state = ST_PLAY; fireCd = 0.4f;
      }
      break;
    case ST_DEAD:
      gameTime += dt;
      if (hardcore || (!easy && deaths >= SOLO_LIVES)) {   // Hardcore bzw. alle Leben weg: der Lauf ist vorbei
        renderEndScreen("GAME OVER", hardcore ? L("DAY TRADER: THE RUN IS OVER", "DAY-TRADER: DER LAUF IST VORBEI") : L("ALL 5 LIVES USED UP", "ALLE 5 LEBEN VERBRAUCHT"), c565(255, 60, 60), false);
        if (enterHit) { state = ST_TITLE; musicStart(); }
      } else {
        static char sub[40];
        if (easy) snprintf(sub, sizeof(sub), "%s", L("YOUR PORTFOLIO HIT ZERO", "DEIN DEPOT IST AUF NULL"));
        else snprintf(sub, sizeof(sub), L("BANKRUPT - %d %s LEFT", "PLEITE - NOCH %d %s"), SOLO_LIVES - deaths, SOLO_LIVES - deaths == 1 ? L("LIFE", "LEBEN") : L("LIVES", "LEBEN"));
        renderEndScreen(L("BANKRUPT", "INSOLVENZ"), sub, c565(255, 60, 60), false);
        if (enterHit) { long pen = pnl > 0 ? min(pnl, max(500L, pnl / 4)) : 0; pnl -= pen; hp = 100; ammo = max(ammo, 30); loadLevel(level); state = ST_PLAY; fireCd = 0.4f; }
      }
      break;
    case ST_WIN:
      gameTime += dt;
      if (islandDone) {   // Auswertung der Steueroase
        char sub[48]; snprintf(sub, sizeof(sub), "%s %d/%d", isleAll ? L("ALL THE GOLD!", "ALLES GOLD!") : "GOLD", isleCoins, isleCoinsPlaced);
        renderEndScreen(L("TAX HAVEN", "STEUEROASE"), isleResult, c565(255, 210, 50), true);
        centerText(sub, 88, 1, c565(255, 210, 50));
        if (enterHit) { state = ST_TITLE; musicStart(); }
      } else {
        char sub[48]; snprintf(sub, sizeof(sub), "%s +$%ld%s", L("BULL MARKET BONUS", "BULLENMARKT-BONUS"), winBonus, deaths ? "" : L(" (NO DEATHS)", " (OHNE TOD)"));
        renderEndScreen(L("BULL MARKET!", "BULLENMARKT!"), sub, c565(255, 210, 50), true);
        if (enterHit) { creditsT = 0; state = ST_CREDITS; }   // weiter: Abspann, dann Steueroase
      }
      break;
    case ST_OUTRO: {
      gameTime += dt; outroT += dt;
      // Kamerafahrt: im Bogen von weit draussen bis nah an den Liegestuhl
      float u = min(1.0f, outroT / (OUTRO_LEN - 3)), e = u * u * (3 - 2 * u);
      float tx = OUTRO_X + 0.9f, ty = OUTRO_Y - 0.2f, ang = 0.45f + e * 1.08f, dist = 10 - e * 7.2f;
      px = tx + cosf(ang) * dist; py = ty + sinf(ang) * dist;
      float dl = sqrtf((tx - px) * (tx - px) + (ty - py) * (ty - py)); dirX = (tx - px) / dl; dirY = (ty - py) / dl; planeX = -dirY * PLANE; planeY = dirX * PLANE;
      isleEyeZ = 1.85f - e * 0.25f; outroScene(outroT);
      renderIsland();
      isleEyeZ = -1; nIsleExtra = 0;
      canvas.fillRect(0, VH, SW, SH - VH, c565(0, 0, 0));
      if (outroT > OUTRO_LEN - 5) centerText(L("EARLY RETIREMENT.", "AUSGESORGT."), VH + 8, 1, c565(255, 210, 50));
      if (enterHit || outroT > OUTRO_LEN) state = ST_WIN;
      break;
    }
    case ST_CREDITS:
      gameTime += dt; creditsT += dt;
      renderCredits();
      if (enterHit || creditsT > CREDITS_LEN) startIsland();
      break;
    case ST_WIFI_LIST:
      gameTime += dt;
      renderWifiList();
      if (hit(';') || hit('e')) wifiSel = max(0, wifiSel - 1);
      if (hit('.') || hit('s')) wifiSel = min(max(0, wifiCount - 1), wifiSel + 1);
      if (hit('`')) state = ST_TITLE;
      else if (hit('f')) { netForget(); state = ST_TITLE; }
      else if (enterHit && wifiCount) {
        passLen = 0; passShow = false;
        if (wifiSec[wifiSel]) state = ST_WIFI_PASS; else tryWifiConnect();
      }
      break;
    case ST_WIFI_PASS:
      gameTime += dt;
      if (fresh) {
        if (ks.enter) { tryWifiConnect(); break; }
        if (ks.del && passLen > 0) passLen--;
        if (ks.tab) passShow = !passShow;
        for (char c : ks.word) {
          if (c == '`' && passLen == 0) { state = ST_WIFI_LIST; break; }
          if (passLen < 64 && c >= 32 && c < 127) passBuf[passLen++] = c;
        }
      }
      if (state == ST_WIFI_PASS) renderWifiPass();
      break;
  }
  audioUpdate();
  // Stream: WLAN-Menues (Passwort!) werden nie uebertragen
  if (fb && state != ST_WIFI_PASS && state != ST_WIFI_LIST && streamWantsFrame()) streamSubmitFrame(fb, SW, SH);
  if (quakeT > 0 && state == ST_PLAY) canvas.pushSprite((int)(esp_random() % 5) - 2, (int)(esp_random() % 3) - 1);   // Boersencrash: Boden bebt
  else canvas.pushSprite(0, 0);
}
