// MARGIN CALL - ein Boersen-Raycaster (Doom-Klon) fuer den M5Stack Cardputer ADV
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

// ---------------------------------------------------------------- Konstanten
static constexpr int SW = 240, SH = 135;
static constexpr int VH = 108;          // Hoehe der 3D-Ansicht, darunter HUD
static constexpr int TEX = 32;
static constexpr int MAP_W = 20, MAP_H = 20;
static constexpr int NUM_LEVELS = 5;
static constexpr float PLANE = 0.66f;
static constexpr float PROJ = SW / (2.0f * PLANE);   // quadratische Pixel

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
       S_DEAD, S_PROJ, S_PROJ_OIL, S_LASER, S_CASH, S_DIV, NUM_SPR };
static uint16_t sprTex[NUM_SPR][TEX * TEX];          // geswappt, 0 = transparent

static uint32_t tmp[TEX * TEX];                      // RGB888-Arbeitspuffer (0 = transparent)
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
static void storeSprite(int idx) {
  for (int i = 0; i < TEX * TEX; i++) {
    uint32_t c = tmp[i];
    if (!(c >> 24)) { sprTex[idx][i] = 0; continue; }
    uint16_t v = swp(c565((c >> 16) & 255, (c >> 8) & 255, c & 255));
    sprTex[idx][i] = v ? v : swp(0x0841);  // echtes Schwarz != transparent
  }
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
    ttri(10, 8 + hy, 2, 1 + hy, 7, 11 + hy, gold); ttri(22, 8 + hy, 30, 1 + hy, 25, 11 + hy, gold);
    tcirc(16, 10 + hy, 6.5f, hide);
    tellipse(16, 15 + hy, 4.5f, 3, RGB(190, 130, 110));
    tpx(14, 15 + hy, RGB(0, 0, 0)); tpx(18, 15 + hy, RGB(0, 0, 0));
    tcirc(16, 18 + hy, 1.8f, gold); tcirc(16, 18 + hy, 0.9f, RGB(190, 130, 110));
    trect(12, 8 + hy, 2, 2, RGB(255, 255, 255)); trect(19, 8 + hy, 2, 2, RGB(255, 255, 255));
    tpx(13, 9 + hy, RGB(200, 0, 0)); tpx(19, 9 + hy, RGB(200, 0, 0));
    trect(15, 21, 3, 5, RGB(30, 210, 70)); ttri(12, 21, 21, 21, 16.5f, 16.5f, RGB(30, 210, 70));   // Aufwaerts-Pfeil
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
  // --- Krypto-Blase
  for (int f = 0; f < 2; f++) {
    tclear(T);
    float r = f ? 13.0f : 11.5f;
    tcirc(16, 16, r, f ? RGB(80, 220, 230) : RGB(60, 210, 130));
    tcirc(16, 16, r - 1.5f, f ? RGB(30, 120, 140) : RGB(25, 110, 70));
    tcirc(16, 16, 6.5f, RGB(245, 180, 30));
    tcirc(16, 16, 5, RGB(255, 210, 70));
    uint32_t bc = RGB(150, 90, 0);
    trect(14, 12, 2, 9, bc); trect(14, 12, 4, 1, bc); trect(14, 16, 4, 1, bc); trect(14, 20, 4, 1, bc);
    trect(18, 13, 1, 3, bc); trect(18, 17, 1, 3, bc); tpx(15, 11, bc); tpx(15, 21, bc);
    tcirc(9, 9, 2, RGB(255, 255, 255));
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
  tclear(T);
  tcirc(16, 21, 10, RGB(180, 120, 10));
  tcirc(16, 21, 8.5f, RGB(255, 200, 50));
  trect(14, 19, 4, 8, RGB(20, 170, 60)); ttri(10, 20, 22, 20, 16, 13, RGB(20, 170, 60));
  tcirc(12, 16, 1.5f, RGB(255, 255, 220));
  storeSprite(S_DIV);
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
  "DOW JONES: SCHALTERHALLE", "S&P 500: HANDELSPARKETT", "NASDAQ-100: HEDGEFONDS-TRESOR",
  "OEL: RAFFINERIE", "ALGO-9000: RECHENZENTRUM"};
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

struct Enemy {
  float x, y; uint8_t type, faction; int16_t hp; bool alive, awake, gone, halfDone;
  float cd, anim, hurt, attackAnim, think, deadT, str, chargeT, chargeCd, mdx, mdy, spawnCd;
  int16_t target;   // -1 = Spieler, -2 = keins, sonst Index
};
struct Item { float x, y; uint8_t type; bool active, stray; };   // 0 = Cash, 1 = Dividende; stray = verstreutes Bargeld
struct Proj { float x, y, dx, dy; bool active; uint8_t dmg, faction, spr; };

static constexpr int MAX_E = 48, MAX_I = 48, MAX_P = 32;
static Enemy enemies[MAX_E]; static int nEnemies;
static Item items[MAX_I];    static int nItems;
static Proj projs[MAX_P];

static float px, py, dirX, dirY, planeX, planeY;
static int hp, ammo, level, kills, marketKills;
static long pnl;
static float fireCd, flashT, hurtT, pickT, msgT, bobT, gameTime, slowT;
static float dodgeT, dodgeCd, dodgeX, dodgeY;   // Ausweichen per Doppeltipp
static char msg[48];
static float zbuf[SW];
static uint8_t hist[48]; static int histN; static float histT;

// Markt- und KI-Zustand
static float levelChg, oilChg, pBullBase, momentum, replayT;
static int replayIdx;
static float spawnT, cashT, aiT, aiFaceT, fakeT, crashT, glitchT, invertT;
// ALGO-Manipulation: Aufschlag auf den echten Kurs (Prozentpunkte), laeuft nach manipT Sekunden aus
static float realChg, manipChg, manipT, dumpAt, flickerT;
// Ticker als farbige Segmente: Index, danach seine Einzelwerte
struct TickSeg { char t[28]; uint16_t col; };
static TickSeg tickSegs[M_COUNT * 2 + STOCK_COUNT];
static int nTickSegs, tickChars;
static int bullsAlive, bearsAlive;

enum State { ST_TITLE, ST_PLAY, ST_PAUSE, ST_LEVELDONE, ST_DEAD, ST_WIN, ST_WIFI_LIST, ST_WIFI_PASS };
static State state = ST_TITLE;

static void say(const char* s, float t = 2.0f) { strncpy(msg, s, sizeof(msg) - 1); msg[sizeof(msg) - 1] = 0; msgT = t; }
static float dist2Player(float x, float y) { float dx = x - px, dy = y - py; return dx * dx + dy * dy; }
static void sfxNear(SfxId id, float x, float y, float range = 7.0f) { if (dist2Player(x, y) < range * range) sfxPlay(id); }

static bool isBoss(const Enemy& e) { return TYPES[e.type].boss; }
static bool hostileFactions(uint8_t a, uint8_t b) { return (a == F_BULL && b == F_BEAR) || (a == F_BEAR && b == F_BULL); }

// ---------------------------------------------------------------- Live-Markt
static const Market& levelMarket() { return markets[LEVEL_MARKET[level]]; }

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
  levelChg = realChg + manipEff();
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

// Wand-Charts und Ticker nachzeichnen, wenn sich die Manipulation spuerbar aendert;
// Licht flackert bei echten Kursspruengen (Intraday-Momentum) und bei Manipulation
static void marketVisuals(float dt) {
  float eff = manipEff();
  if (fabsf(eff - manipDrawn) > 0.2f || (eff == 0 && manipDrawn != 0)) {
    manipDrawn = eff; genTickerTex(levelMarket()); genChartTex(levelMarket()); buildTicker();
  }
  float vol = fabsf(momentum) * 2 + fabsf(eff) * 0.4f;
  flickerT -= dt;
  if (flickerT <= 0 && vol > 0.5f && frand() < min(0.25f, vol * 0.03f)) flickerT = 0.05f + frand() * 0.1f;
}

static float animalStrength(bool bull) {
  float s = 1.0f + max(0.0f, (bull ? 1 : -1) * levelChg) * 0.25f;
  return min(s, 1.6f);
}
static float fireRateMul() { return oilChg > 0 ? 1.0f + min(oilChg, 4.0f) * 0.1f : 1.0f; }
static float playerSpeedMul() {
  float m = oilChg < 0 ? 1.0f + min(-oilChg, 4.0f) * 0.05f : 1.0f;
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
  e.alive = true; e.target = -2;
  e.str = type == E_BULL ? animalStrength(true) : (type == E_BEAR ? animalStrength(false) : 1.0f);
  e.hp = (int16_t)roundf(TYPES[type].hp * e.str);
  e.cd = 1.0f + frand(); e.anim = frand() * 6; e.think = frand() * 0.4f;
  e.spawnCd = 6;
  float a = frand() * 6.283f; e.mdx = cosf(a); e.mdy = sinf(a);
  return i;
}

static bool lineOfSight(float x0, float y0, float x1, float y1) {
  float dx = x1 - x0, dy = y1 - y0, d = sqrtf(dx * dx + dy * dy);
  int steps = (int)(d * 5) + 1;
  for (int i = 1; i < steps; i++) {
    float t = (float)i / steps;
    if (solidCell((int)(x0 + dx * t), (int)(y0 + dy * t))) return false;
  }
  return true;
}

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
static void spawnNear(uint8_t type, int count) {
  for (int k = 0; k < count; k++) { float x, y; if (findHiddenCell(x, y, 3.5f)) spawnEnemy(type, x, y); }
}

static void loadLevel(int lv) {
  nEnemies = nItems = 0; kills = marketKills = 0;
  for (auto& p : projs) p.active = false;
  applyMarket();
  for (int y = 0; y < MAP_H; y++)
    for (int x = 0; x < MAP_W; x++) {
      char c = LEVELS[lv][y][x];
      float cx = x + 0.5f, cy = y + 0.5f;
      world[y][x] = '.';
      switch (c) {
        case 'P': px = cx; py = cy; break;
        case 'A': spawnEnemy(frand() < pBullBase ? E_BULL : E_BEAR, cx, cy); break;
        case 'u': spawnEnemy(E_BULL, cx, cy); break;
        case 'b': spawnEnemy(E_BEAR, cx, cy); break;
        case 's': spawnEnemy(E_SELLER, cx, cy); break;
        case 'c': spawnEnemy(E_BUBBLE, cx, cy); break;
        case 'H': spawnEnemy(E_SHARK, cx, cy); break;
        case 'o': spawnEnemy(E_BARON, cx, cy); break;
        case 'l': spawnEnemy(E_SLICK, cx, cy); break;
        case 't': spawnEnemy(E_BOT, cx, cy); break;
        case 'K': spawnEnemy(E_ALGO, cx, cy); break;
        case '$': case '+':
          if (nItems < MAX_I) items[nItems++] = {cx, cy, (uint8_t)(c == '+'), true, false};
          break;
        default: world[y][x] = c;
      }
    }
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
  char b[48]; snprintf(b, sizeof(b), "LEVEL %d: %s", lv + 1, LEVEL_NAMES[lv]);
  say(b, 3.0f);
  sfxPlay(SFX_ID_BELL);   // Eroeffnungsglocke
  musicStart();
}

static void newGame() { hp = 100; ammo = 30; pnl = 0; level = 0; loadLevel(0); }

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

static void damagePlayer(int d, const char* why) {
  if (state != ST_PLAY) return;
  hp -= d; hurtT = 0.35f; sfxPlay(SFX_ID_PHONE);
  if (why) say(why, 1.2f);
  if (hp <= 0) { hp = 0; state = ST_DEAD; musicStop(); sfxPlay(SFX_ID_CRASH); }
}

static void spawnProj(float x, float y, float ang, float spd, uint8_t dmg, uint8_t faction, uint8_t spr) {
  for (auto& p : projs) if (!p.active) { p = {x, y, cosf(ang) * spd, sinf(ang) * spd, true, dmg, faction, spr}; return; }
}

static void dropItem(float x, float y, uint8_t type, bool stray = false) {
  int i = 0;   // aufgesammelte Plaetze wiederverwenden
  while (i < nItems && items[i].active) i++;
  if (i == nItems) { if (nItems >= MAX_I) return; nItems++; }
  items[i] = {x, y, type, true, stray};
}

static void killEnemy(Enemy& e, bool byPlayer) {
  e.alive = false; e.deadT = 0;
  const TypeInfo& ti = TYPES[e.type];
  if (e.type == E_BUBBLE || e.type == E_BOT) e.gone = true;
  if (byPlayer) {
    kills++; pnl += ti.pts;
    char b[48];
    switch (e.type) {
      case E_BEAR:   say("BAER ERLEGT!"); break;
      case E_BULL:   say("BULLE ERLEGT!"); break;
      case E_SELLER: say("SHORT SELLER LIQUIDIERT"); break;
      case E_BUBBLE: say("BLASE GEPLATZT"); break;
      case E_SHARK:  say("HEDGEFONDS ZERSCHLAGEN! ZUM TRESOR!", 3); break;
      case E_ALGO:   say("ALGO-9000 ABGESCHALTET! ZUM TRESOR!", 3); break;
      default: snprintf(b, sizeof(b), "%s ERLEDIGT", ti.name); say(b);
    }
  } else {
    marketKills++;
    if (dist2Player(e.x, e.y) < 64) say(e.type == E_BEAR || e.type == E_SELLER ? "BULLEN TRAMPELN BAEREN NIEDER" : "BAEREN ZERFLEISCHEN BULLEN", 1.5f);
  }
  sfxNear(e.type == E_BUBBLE ? SFX_ID_POP : e.type == E_SLICK ? SFX_ID_OIL : SFX_ID_STAMP, e.x, e.y, 10);
  float dropChance = byPlayer ? (e.type == E_SELLER || e.type == E_BARON ? 1.0f : 0.38f) : 0.22f;
  if (ti.boss) dropItem(e.x, e.y, 1);
  else if (frand() < dropChance) dropItem(e.x, e.y, frand() < 0.15f ? 1 : 0);
}

static void hurtEnemy(Enemy& t, int dmg, bool byPlayer) {
  if (!t.alive) return;
  t.hp -= dmg; t.hurt = 0.15f; t.awake = true;
  if (t.hp <= 0) killEnemy(t, byPlayer);
  else if (byPlayer) sfxPlay(SFX_ID_HIT);
}

// ---------------------------------------------------------------- KI ALGO-9000
// Meldung passt in msg[48]: "ALGO: FLASH CRASH! KURS -5.00%"
static void manipulate(float chg, float t, const char* what) {
  manipChg = chg; manipT = t; dumpAt = 0; marketEffect();
  char b[48]; snprintf(b, sizeof(b), "ALGO: %s KURS %+.2f%%", what, chg);
  say(b, 2.5f);
}
static void aiEvent(int forced = -1) {
  int ev = forced >= 0 ? forced : rnd() % 6;
  aiFaceT = 2.2f; glitchT = max(glitchT, 0.6f);
  sfxPlay(SFX_ID_GLITCH);
  switch (ev) {
    case 0: fakeT = 10; spawnNear(frand() < 0.5f ? E_BULL : E_BEAR, 2); manipulate((frand() < 0.5f ? -1 : 1) * (2 + frand() * 2), 20, "FAKE NEWS!"); break;
    case 1: crashT = 8; manipulate(-5, 15, "FLASH CRASH!"); break;
    case 2: spawnNear(E_BOT, 2 + rnd() % 2); manipulate(-1.5f - frand(), 20, "BOTS AKTIV"); break;
    case 3: spawnNear(E_BUBBLE, 3); manipulate(4, 24, "PUMP & DUMP!"); dumpAt = 12; break;
    case 4: glitchT = 4; invertT = 3; manipulate((frand() < 0.5f ? -1 : 1) * 4, 15, "MANIPULIERT!"); break;
    default:
      for (int i = 0; i < nEnemies; i++) if (enemies[i].alive && (enemies[i].type == E_BULL || enemies[i].type == E_BEAR)) { enemies[i].str *= 1.2f; enemies[i].awake = true; }
      spawnNear(E_BULL, 1); spawnNear(E_BEAR, 1);
      manipulate(realChg < 0 ? -3 : 3, 20, "AUFGEHETZT!");
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
    bool bossAlive = false;
    for (int i = 0; i < nEnemies; i++) if (enemies[i].alive && isBoss(enemies[i])) bossAlive = true;
    if (bossAlive) { if (msgT <= 0) { say("TRESOR GESPERRT - BOSS LEBT NOCH!", 1.5f); sfxPlay(SFX_ID_DECLINED); } }
    else {
      state = level >= NUM_LEVELS - 1 ? ST_WIN : ST_LEVELDONE; pnl += 500 + hp * 5;
      musicStop(); sfxPlay(SFX_ID_BELL); if (state == ST_WIN) sfxPlay(SFX_ID_KACHING);
      return;
    }
  }
  moveEntity(px, py, mx, my, r);
}

static void shoot() {
  if (fireCd > 0) return;
  if (ammo <= 0) { say("KEINE LIQUIDITAET! CASH SAMMELN", 1.2f); fireCd = 0.4f; sfxPlay(SFX_ID_DECLINED); return; }
  ammo--; fireCd = 0.28f; flashT = 0.12f; sfxPlay(SFX_ID_SHOT);
  float invDet = 1.0f / (planeX * dirY - dirX * planeY);
  int best = -1; float bestZ = zbuf[SW / 2];
  for (int i = 0; i < nEnemies; i++) {
    Enemy& e = enemies[i];
    if (!e.alive) continue;
    float sx = e.x - px, sy = e.y - py;
    float tX = invDet * (dirY * sx - dirX * sy), tY = invDet * (-planeY * sx + planeX * sy);
    if (tY < 0.2f || tY >= bestZ) continue;
    float scr = SW / 2 * (1 + tX / tY), half = PROJ / tY * 0.3f * TYPES[e.type].scale + 2;
    if (fabsf(scr - SW / 2) < half) { best = i; bestZ = tY; }
  }
  if (best >= 0) hurtEnemy(enemies[best], 2, true);
}

// ---------------------------------------------------------------- Gegner-KI
static void chooseTarget(Enemy& e, int self) {
  float best = 1e9f; int bt = -2;
  float dp = sqrtf(dist2Player(e.x, e.y));
  if (dp < 11 && lineOfSight(e.x, e.y, px, py)) { best = dp; bt = -1; }
  if (crashT <= 0) {
    for (int j = 0; j < nEnemies; j++) {
      if (j == self) continue;
      const Enemy& o = enemies[j];
      if (!o.alive || !hostileFactions(e.faction, o.faction)) continue;
      float dx = o.x - e.x, dy = o.y - e.y, d = sqrtf(dx * dx + dy * dy);
      if (d > 8 || d * 0.6f > best) continue;
      if (!lineOfSight(e.x, e.y, o.x, o.y)) continue;
      best = d * 0.6f; bt = j;   // Bullen und Baeren hassen sich mehr als dich
    }
  }
  e.target = bt;
}

static void meleeHit(Enemy& e, int tgt, int dmgPlayer, int dmgEnemy, const char* why) {
  if (tgt == -1) damagePlayer((int)(dmgPlayer * e.str), why);
  else { hurtEnemy(enemies[tgt], (int)ceilf(dmgEnemy * e.str), false); sfxNear(e.faction == F_BULL ? SFX_ID_BULL : SFX_ID_ROAR, e.x, e.y, 6); }
}

static void updateEnemy(Enemy& e, int idx, float dt) {
  e.anim += dt; e.hurt -= dt; e.cd -= dt * fireRateMul(); e.attackAnim -= dt; e.chargeCd -= dt; e.think -= dt;
  const float spdMul = crashT > 0 ? 1.4f : 1.0f;
  const bool animal = e.faction == F_BULL || e.faction == F_BEAR;

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

  if (!animal) {
    bool los = d < 12 && lineOfSight(e.x, e.y, px, py);
    if (!e.awake) { if (los && d < 9) e.awake = true; else return; }
    switch (e.type) {
      case E_SELLER:
        if (d > 4.0f || !los) moveEntity(e.x, e.y, nx * 1.1f * dt * spdMul, ny * 1.1f * dt * spdMul, 0.25f);
        else if (d < 2.2f) moveEntity(e.x, e.y, -nx * 0.9f * dt, -ny * 0.9f * dt, 0.25f);
        break;
      case E_SHARK:
        if (d > 3.0f || !los) moveEntity(e.x, e.y, nx * 1.2f * dt, ny * 1.2f * dt, 0.35f);
        if (d < 0.9f && e.cd <= 0) { e.cd = 1.0f; damagePlayer(20, "FEINDLICHE UEBERNAHME!"); }
        if (los && e.cd <= 0) {
          e.cd = 1.3f; e.attackAnim = 0.4f;
          for (int k = -1; k <= 1; k++) spawnProj(e.x, e.y, ang + k * 0.18f, 4.0f, 10, F_NEUTRAL, S_PROJ);
          sfxNear(SFX_ID_ZAP, e.x, e.y, 12);
        }
        return;
      case E_BARON:
        if (d > 4.5f || !los) moveEntity(e.x, e.y, nx * 1.0f * dt, ny * 1.0f * dt, 0.25f);
        else if (d < 2.5f) moveEntity(e.x, e.y, -nx * 0.8f * dt, -ny * 0.8f * dt, 0.25f);
        if (los && e.cd <= 0 && d < 10) {
          e.cd = 2.0f + frand(); e.attackAnim = 0.4f;
          spawnProj(e.x, e.y, ang, 3.4f, 10, F_NEUTRAL, S_PROJ_OIL);
          sfxNear(SFX_ID_OIL, e.x, e.y, 10);
        }
        return;
      case E_SLICK:
        if (d > 0.7f) moveEntity(e.x, e.y, nx * 0.9f * dt * spdMul, ny * 0.9f * dt * spdMul, 0.25f);
        else if (e.cd <= 0) { e.cd = 1.2f; e.attackAnim = 0.3f; slowT = 3; sfxPlay(SFX_ID_OIL); damagePlayer(8, "OELPEST! DU KLEBST FEST"); }
        return;
      case E_BOT: {
        float w = sinf(e.anim * 3) * 0.6f;
        if (d > 3.2f || !los) moveEntity(e.x, e.y, (nx - ny * w) * 2.0f * dt, (ny + nx * w) * 2.0f * dt, 0.2f);
        else moveEntity(e.x, e.y, -ny * w * 1.5f * dt, nx * w * 1.5f * dt, 0.2f);
        if (los && e.cd <= 0 && d < 10) {
          e.cd = 1.3f + frand() * 0.5f; e.attackAnim = 0.25f;
          spawnProj(e.x, e.y, ang + (frand() - 0.5f) * 0.1f, 6.0f, 6, F_AI, S_LASER);
          sfxNear(SFX_ID_LASER, e.x, e.y, 10);
        }
        return;
      }
      case E_ALGO:
        e.spawnCd -= dt;
        if (los && e.cd <= 0) {
          e.cd = 1.2f; e.attackAnim = 0.5f;
          for (int k = -2; k <= 2; k++) spawnProj(e.x, e.y, ang + k * 0.14f, 5.0f, 8, F_AI, S_LASER);
          sfxPlay(SFX_ID_LASER);
        }
        if (e.spawnCd <= 0) { e.spawnCd = 8; spawnNear(E_BOT, 2); }
        if (!e.halfDone && e.hp < TYPES[E_ALGO].hp / 2) { e.halfDone = true; aiEvent(1); }
        return;
      default: break;
    }
    // Short Seller schiesst
    if (e.type == E_SELLER && los && e.cd <= 0 && d < 10) {
      e.cd = 1.6f + frand(); e.attackAnim = 0.3f;
      spawnProj(e.x, e.y, ang + (frand() - 0.5f) * 0.12f, 4.5f, 8, F_BEAR, S_PROJ);
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
  const char* hitMsg = e.faction == F_BULL ? "BULLE RAMMT DICH! KURSSPRUNG" : "BAERENTATZE! KURSVERLUST";
  switch (e.type) {
    case E_BEAR:
      // Gebruell auf der Jagd (spawnCd dient beim Baer als Gebruell-Timer)
      e.spawnCd -= dt;
      if (e.target == -1 && e.spawnCd <= 0 && d < 8) { e.spawnCd = 5 + frand() * 4; sfxNear(SFX_ID_ROAR, e.x, e.y, 9); }
      if (d > 0.75f) moveEntity(e.x, e.y, nx * 1.5f * dt * spdMul, ny * 1.5f * dt * spdMul, 0.25f);
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
        if (d < 0.75f) { e.chargeT = 0; e.cd = 1.0f; meleeHit(e, e.target, 15, 3, hitMsg); }
      } else if (d > 0.75f) {
        moveEntity(e.x, e.y, nx * 1.3f * dt * spdMul, ny * 1.3f * dt * spdMul, 0.25f);
        if (e.chargeCd <= 0 && d > 1.5f && d < 6 && lineOfSight(e.x, e.y, tx, ty)) {
          e.chargeT = 0.7f; e.chargeCd = 3.5f + frand() * 2; e.mdx = nx; e.mdy = ny; e.attackAnim = 0.7f;
          sfxNear(SFX_ID_BULL, e.x, e.y, 8);
        }
      } else if (e.cd <= 0) { e.cd = 1.1f; e.attackAnim = 0.3f; meleeHit(e, e.target, 10, 2, hitMsg); }
      break;
    case E_SELLER:
      if (d > 4.0f) moveEntity(e.x, e.y, nx * 1.1f * dt, ny * 1.1f * dt, 0.25f);
      else if (d < 2.2f) moveEntity(e.x, e.y, -nx * 0.9f * dt, -ny * 0.9f * dt, 0.25f);
      if (e.cd <= 0 && d < 10 && lineOfSight(e.x, e.y, tx, ty)) {
        e.cd = 1.6f + frand(); e.attackAnim = 0.3f;
        spawnProj(e.x, e.y, ang + (frand() - 0.5f) * 0.12f, 4.5f, 8, F_BEAR, S_PROJ);
        sfxNear(SFX_ID_ZAP, e.x, e.y, 10);
      }
      break;
    case E_BUBBLE: {
      float w = sinf(e.anim * 5) * 0.8f;
      moveEntity(e.x, e.y, (nx - ny * w) * 2.2f * dt * spdMul, (ny + nx * w) * 2.2f * dt * spdMul, 0.25f);
      if (d < 0.65f) {
        e.alive = false; e.gone = true; sfxNear(SFX_ID_POP, e.x, e.y, 10);
        if (e.target == -1) damagePlayer(20, "KRYPTO-CRASH! -20%");
        else hurtEnemy(enemies[e.target], 4, false);
      }
      break;
    }
  }
}

static void updateGame(float dt) {
  gameTime += dt;
  musicSetPanic(hp < 30);
  fireCd -= dt; flashT -= dt; hurtT -= dt; pickT -= dt; msgT -= dt; slowT -= dt;
  aiFaceT -= dt; crashT -= dt; glitchT -= dt; invertT -= dt;
  if (fakeT > 0) { fakeT -= dt; if (fakeT <= 0 || ((int)(gameTime * 4) % 2)) buildTicker(); }
  if (manipT > 0) {
    manipT -= dt;
    if (dumpAt > 0 && manipT <= dumpAt) {
      dumpAt = 0; manipChg = -manipChg;
      char b[48]; snprintf(b, sizeof(b), "ALGO: DUMP! KURS %+.2f%%", manipChg); say(b, 2.5f); sfxPlay(SFX_ID_GLITCH);
    }
    marketEffect();
  }
  marketVisuals(dt);

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

  // Ausweichen: dieselbe Richtung zweimal schnell antippen (wie in Unreal Tournament)
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
    if (!it.active) continue;
    if (dist2Player(it.x, it.y) > 0.3f) continue;
    if (it.type == 0) { if (ammo >= 99) continue; ammo = min(99, ammo + 10); say("CASHFLOW +10", 1.2f); }
    else { if (hp >= 100) continue; hp = min(100, hp + 20); say("DIVIDENDE AUSGESCHUETTET +20%", 1.2f); }
    it.active = false; pickT = 0.25f; pnl += 25; sfxPlay(it.type == 0 ? SFX_ID_KACHING : SFX_ID_COINS);
  }

  // Intraday-Replay: Momentum des echten Kursverlaufs steuert den Nachschub
  replayT -= dt;
  const Market& m = levelMarket();
  if (replayT <= 0 && m.n > 3) {
    replayT = 3;
    replayIdx = (replayIdx + 1) % m.n;
    int i0 = max(0, replayIdx - 3);
    momentum = (m.series[replayIdx] - m.series[i0]) / m.series[i0] * 100.0f;
  }

  // Gegner
  bullsAlive = bearsAlive = 0;
  for (int i = 0; i < nEnemies; i++) {
    Enemy& e = enemies[i];
    if (e.gone) continue;
    if (!e.alive) { e.deadT += dt; if (e.deadT > 25) e.gone = true; continue; }
    if (e.faction == F_BULL) bullsAlive++; else if (e.faction == F_BEAR) bearsAlive++;
    updateEnemy(e, i, dt);
    if (state != ST_PLAY) return;
  }

  // Allgegenwaertige Bullen und Baeren: Nachschub nach Marktlage
  spawnT -= dt;
  if (spawnT <= 0) {
    spawnT = 7 + frand() * 5;
    if (bullsAlive + bearsAlive < 9) {
      float p = constrain(pBullBase + momentum * 1.5f, 0.1f, 0.9f);
      spawnAnimal(frand() < p);
    }
  }

  // vereinzeltes Bargeld abseits des Spielers, damit man nie ganz ohne Munition dasteht;
  // wer fast leer ist, findet schneller etwas, mehr als 2 solcher Scheine liegen nie herum
  cashT -= dt;
  if (cashT <= 0) {
    cashT = ammo <= 3 ? 15 + frand() * 10 : 45 + frand() * 30;
    int strays = 0;
    for (int i = 0; i < nItems; i++) if (items[i].active && items[i].stray) strays++;
    float x, y;
    if (strays < 2 && findHiddenCell(x, y, 6)) dropItem(x, y, 0, true);
  }

  // KI-Ereignisse
  aiT -= dt;
  if (aiT <= 0) { aiT = level == NUM_LEVELS - 1 ? 22 + frand() * 12 : 40 + frand() * 30; aiEvent(); }

  // Projektile
  for (auto& p : projs) {
    if (!p.active) continue;
    p.x += p.dx * dt; p.y += p.dy * dt;
    if (solidCell((int)p.x, (int)p.y)) { p.active = false; continue; }
    if (dist2Player(p.x, p.y) < 0.12f) {
      p.active = false;
      damagePlayer(p.dmg, p.spr == S_LASER ? "ALGO-ORDER AUSGEFUEHRT!" : p.spr == S_PROJ_OIL ? "OELFASS! ENERGIEKRISE" : "MARGIN CALL!");
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

struct Spr { float x, y; int16_t tex; float scale; bool hurt; float dist; };
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
    sprs[n++] = {e.x, e.y, (int16_t)t, sc, e.alive && e.hurt > 0, 0};
  }
  for (int i = 0; i < nItems; i++) if (items[i].active) sprs[n++] = {items[i].x, items[i].y, (int16_t)(items[i].type ? S_DIV : S_CASH), 0.5f, false, 0};
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
    bool floating = s.tex == S_PROJ || s.tex == S_PROJ_OIL || s.tex == S_LASER || s.tex == S_BOT0 || s.tex == S_BOT1;
    int vMove = floating ? 0 : (full - size) / 2;
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

static void renderFlicker() {   // Stromschwankung: 3D-Bild kurz halb so hell
  if (flickerT <= 0) return;
  for (int i = 0; i < SW * VH; i++) fb[i] = swp((swp(fb[i]) >> 1) & 0x7BEF);
}

static void renderWeapon() {
  int bob = (int)(fabsf(sinf(bobT)) * 4);
  int by = VH - 34 + bob + (flashT > 0 ? 3 : 0);
  if (flashT > 0) {
    canvas.fillCircle(120, by - 4, 11, c565(255, 170, 0));
    canvas.fillCircle(120, by - 4, 6, c565(255, 255, 180));
    canvas.setTextColor(c565(40, 160, 40)); canvas.setTextSize(1);
    canvas.drawString("$", 104, by - 14); canvas.drawString("$", 132, by - 12);
  }
  canvas.fillRect(113, by, 14, 22, c565(60, 62, 72));
  canvas.fillRect(115, by, 4, 22, c565(120, 122, 135));
  canvas.fillRect(111, by - 2, 18, 4, c565(200, 160, 40));
  canvas.fillRoundRect(98, VH - 14 + bob, 44, 16, 5, c565(110, 64, 30));
  canvas.fillTriangle(100, VH - 10 + bob, 78, VH - 30 + bob, 104, VH - 4 + bob, c565(235, 195, 60));
  canvas.fillTriangle(140, VH - 10 + bob, 162, VH - 30 + bob, 136, VH - 4 + bob, c565(235, 195, 60));
  canvas.fillTriangle(78, VH - 30 + bob, 84, VH - 22 + bob, 80, VH - 24 + bob, c565(255, 245, 200));
  canvas.fillTriangle(162, VH - 30 + bob, 156, VH - 22 + bob, 160, VH - 24 + bob, c565(255, 245, 200));
  canvas.fillCircle(110, VH - 7 + bob, 2, c565(255, 40, 40));
  canvas.fillCircle(130, VH - 7 + bob, 2, c565(255, 40, 40));
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
  canvas.drawString("DEPOT", 3, VH + 3);
  canvas.drawString("CASH", 108, VH + 3);
  uint16_t hc = hp > 60 ? c565(60, 230, 100) : hp > 30 ? c565(255, 200, 40) : c565(255, 60, 60);
  canvas.setTextColor(hc); canvas.setTextSize(2);
  char b[32];
  snprintf(b, sizeof(b), "%d%%", hp); canvas.drawString(b, 3, VH + 12);
  canvas.setTextColor(ammo > 5 ? c565(120, 230, 140) : c565(255, 80, 80));
  snprintf(b, sizeof(b), "%d", ammo); canvas.drawString(b, 108, VH + 12);
  canvas.setTextSize(1);
  // P&L
  canvas.setTextColor(c565(80, 255, 130));
  snprintf(b, sizeof(b), "P&L+%ld", pnl); canvas.drawString(b, 148, VH + 3);
  // Bullen-vs-Baeren-Balken
  int bx = 148, bw = 88, tot = bullsAlive + bearsAlive;
  int gw = tot ? bw * bullsAlive / tot : bw / 2;
  canvas.fillRect(bx, VH + 12, gw, 5, c565(30, 200, 80));
  canvas.fillRect(bx + gw, VH + 12, bw - gw, 5, c565(220, 40, 40));
  // Level-Markt
  const Market& m = levelMarket();
  canvas.setTextColor(manipT > 0 && ((int)(gameTime * 4) % 2) ? c565(255, 60, 200) : levelChg >= 0 ? c565(80, 255, 130) : c565(255, 80, 80));
  if (manipT > 0) snprintf(b, sizeof(b), "%s ALGO%+.1f%%", level == NUM_LEVELS - 1 ? "MIX" : m.label, levelChg);   // manipulierter Spielkurs
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
  if (hurtT > 0) { canvas.drawRect(0, 0, SW, VH, c565(255, 0, 0)); canvas.drawRect(1, 1, SW - 2, VH - 2, c565(255, 0, 0)); canvas.drawRect(2, 2, SW - 4, VH - 4, c565(160, 0, 0)); }
  if (pickT > 0) { canvas.drawRect(0, 0, SW, VH, c565(60, 255, 120)); canvas.drawRect(1, 1, SW - 2, VH - 2, c565(60, 255, 120)); }
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
  centerText("BULLEN VS BAEREN VS DU VS KI", 38, 1, c565(80, 255, 130));
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
    snprintf(b, sizeof(b), "WLAN %s  STREAM:", netSSID()); canvas.drawString(b, 14, 89);
    canvas.setTextColor(c565(120, 220, 255));
    snprintf(b, sizeof(b), "http://%s  margincall.local", netIP()); canvas.drawString(b, 14, 98);
  } else {
    canvas.drawString(netHasCreds() ? "WLAN: VERBINDE..." : "WLAN: AUS (KURSE SIMULIERT)", 14, 89);
    canvas.drawString("[W] WLAN EINRICHTEN", 14, 98);
  }
  canvas.setTextColor(c565(160, 160, 180));
  canvas.drawString("E/S A/D W/R SPACE  P PAUSE  M MUSIK", 14, 109);
  if (((int)(gameTime * 2)) % 2) centerText("[ENTER] HANDEL STARTEN", 121, 1, c565(255, 255, 255));
  else centerText("MADE BY WOLFGANG RABENSTEIN", 121, 1, c565(110, 120, 150));
}

static void renderEndScreen(const char* title, const char* sub, uint16_t col, bool bull) {
  drawChartBackground(bull ? c565(20, 90, 45) : c565(40, 40, 40), bull ? c565(40, 40, 40) : c565(110, 25, 25));
  canvas.fillRect(0, 18, SW, 92, c565(0, 0, 0));
  centerText(title, 24, 2, col);
  centerText(sub, 46, 1, c565(220, 220, 230));
  char b[48];
  snprintf(b, sizeof(b), "P&L: +$%ld", pnl); centerText(b, 60, 1, c565(80, 255, 130));
  snprintf(b, sizeof(b), "VON DIR LIQUIDIERT: %d", kills); centerText(b, 71, 1, c565(200, 200, 210));
  snprintf(b, sizeof(b), "MARKTBEREINIGUNG: %d", marketKills); centerText(b, 81, 1, c565(200, 200, 210));
  centerText("MADE BY WOLFGANG RABENSTEIN", 112, 1, c565(110, 120, 150));
  if (((int)(gameTime * 2)) % 2) centerText("[ENTER] WEITER", 96, 1, c565(255, 255, 255));
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
  snprintf(b, sizeof(b), "%-4s %s", markets[idx].label, ok ? "OK" : "FEHLER - SIMULIERT");
  d.drawString(b, 40, 52 + idx * 12);
}

// Speicher fuer TLS freimachen: Canvas kurz loeschen, direkt aufs Display zeichnen
static void fetchMarketsNow() {
  if (!netConnected()) return;
  canvas.deleteSprite(); fb = nullptr;
  auto& d = M5Cardputer.Display;
  d.fillScreen(c565(0, 0, 0));
  d.setTextDatum(top_center); d.setTextSize(1); d.setTextColor(c565(255, 210, 50));
  d.drawString("MARKTDATEN WERDEN GELADEN...", SW / 2, 30);
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
  centerText("WLAN WAEHLEN", 4, 1, c565(255, 210, 50));
  canvas.setTextSize(1); canvas.setTextDatum(top_left);
  if (!wifiCount) { centerText("KEINE NETZE GEFUNDEN", 50, 1, c565(255, 120, 80)); }
  int first = max(0, min(wifiSel - 4, wifiCount - 9));
  for (int i = first; i < min(wifiCount, first + 9); i++) {
    int y = 18 + (i - first) * 11;
    if (i == wifiSel) canvas.fillRect(4, y - 1, SW - 8, 10, c565(30, 80, 50));
    canvas.setTextColor(c565(220, 220, 230));
    char b[48]; snprintf(b, sizeof(b), "%-26.26s %4d %s", wifiNames[i], wifiRssi[i], wifiSec[i] ? "*" : "");
    canvas.drawString(b, 8, y);
  }
  canvas.setTextColor(c565(140, 150, 180));
  canvas.drawString(";/. WAEHLEN  ENTER OK  ` ZURUECK  F VERGESSEN", 4, 125);
}

static void renderWifiPass() {
  canvas.fillScreen(c565(6, 10, 20));
  centerText("PASSWORT FUER", 10, 1, c565(255, 210, 50));
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
  canvas.drawString("ENTER VERBINDEN   DEL LOESCHEN", 14, 76);
  canvas.drawString("TAB ANZEIGEN/VERBERGEN   ` ZURUECK", 14, 88);
  canvas.drawString("(DAS PASSWORT WIRD NICHT GESTREAMT)", 14, 106);
}

static void startWifiScan() {
  canvas.fillScreen(c565(0, 0, 0));
  centerText("SUCHE WLAN-NETZE...", 60, 1, c565(255, 210, 50));
  canvas.pushSprite(0, 0);
  wifiCount = netScan(wifiNames, wifiRssi, wifiSec, 12);
  wifiSel = 0;
  state = ST_WIFI_LIST;
}

static void tryWifiConnect() {
  canvas.fillScreen(c565(0, 0, 0));
  centerText("VERBINDE MIT", 50, 1, c565(255, 210, 50));
  centerText(wifiNames[wifiSel], 62, 1, c565(120, 220, 255));
  canvas.pushSprite(0, 0);
  passBuf[passLen] = 0;
  bool ok = netConnect(wifiNames[wifiSel], passBuf, 15000);
  memset(passBuf, 0, sizeof(passBuf)); passLen = 0;
  if (ok) { fetchMarketsNow(); buildTicker(); state = ST_TITLE; sfxPlay(SFX_ID_KACHING); }
  else {
    canvas.fillScreen(c565(0, 0, 0));
    centerText("VERBINDUNG FEHLGESCHLAGEN", 55, 1, c565(255, 80, 80));
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

  if (state != ST_WIFI_PASS && state != ST_WIFI_LIST && hit('m')) { musicToggle(); say(musicEnabled() ? "MUSIK AN" : "MUSIK AUS", 1.0f); }

  switch (state) {
    case ST_TITLE:
      gameTime += dt;
      if (!fetchedAtBoot && netConnected()) { fetchedAtBoot = true; fetchMarketsNow(); buildTicker(); }
      renderTitle();
      if (enterHit) { newGame(); state = ST_PLAY; fireCd = 0.4f; sfxPlay(SFX_ID_KACHING); }
      else if (hit('w')) startWifiScan();
      break;
    case ST_PLAY:
      if (hit('p')) { state = ST_PAUSE; musicSetPaused(true); break; }
      updateGame(dt);
      renderWalls(); renderSprites(); renderGlitch(); renderFlicker(); renderWeapon(); renderHUD();
      break;
    case ST_PAUSE:
      canvas.fillRect(30, 36, 180, 40, c565(0, 0, 0));
      centerText("HANDEL AUSGESETZT", 42, 1, c565(255, 210, 50));
      centerText("[P] WEITER", 54, 1, c565(220, 220, 230));
      if (netConnected()) { char b[40]; snprintf(b, sizeof(b), "STREAM: http://%s", netIP()); centerText(b, 65, 1, c565(120, 220, 255)); }
      if (hit('p')) { state = ST_PLAY; musicSetPaused(false); }
      break;
    case ST_LEVELDONE:
      gameTime += dt;
      renderEndScreen("CLOSING BELL!", "LEVEL GESCHAFFT - KURS STEIGT", c565(80, 255, 130), true);
      if (enterHit) {
        level++;
        if (marketsAgeMs() > 120000) fetchMarketsNow();
        loadLevel(level); ammo = max(ammo, 15); state = ST_PLAY; fireCd = 0.4f;
      }
      break;
    case ST_DEAD:
      gameTime += dt;
      renderEndScreen("INSOLVENZ", "DEIN DEPOT IST AUF NULL", c565(255, 60, 60), false);
      if (enterHit) { hp = 100; ammo = max(ammo, 30); pnl = pnl / 2; loadLevel(level); state = ST_PLAY; fireCd = 0.4f; }
      break;
    case ST_WIN:
      gameTime += dt;
      renderEndScreen("BULLENMARKT!", "ALGO-9000 BESIEGT - DER MARKT IST FREI", c565(255, 210, 50), true);
      if (enterHit) { state = ST_TITLE; musicStart(); }
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
  canvas.pushSprite(0, 0);
}
