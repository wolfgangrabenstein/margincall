#pragma once
// Netzwerk fuer MARGIN CALL: WLAN, Live-Boersendaten, Bildschirm-Stream

#include <stdint.h>

// ---------------------------------------------------------------- Maerkte
enum { M_DOW, M_SPX, M_NDX, M_OIL, M_COUNT };
static constexpr int SERIES_MAX = 96;

struct Market {
  const char* symbol;   // Yahoo-Symbol (URL-kodiert)
  const char* label;    // Anzeige im Spiel
  float price, prev, chg;   // chg = Tagesveraenderung in %
  float series[SERIES_MAX]; // Intraday-Verlauf
  int n;
  bool live;            // true = echte Daten, false = simuliert
};
extern Market markets[M_COUNT];

// Einzelwerte je Index – nur fuer den Ticker, auf das Spiel wirken allein die Leitindizes
static constexpr int STOCK_COUNT = 20;
struct Stock {
  const char* symbol;   // Yahoo-Symbol
  const char* label;    // Anzeige im Ticker
  uint8_t market;       // zugehoeriger Index (M_DOW ...)
  float price, chg;     // price = 0 bei simulierten Werten
  bool live;
};
extern Stock stocks[STOCK_COUNT];

void marketsSimulate();                              // Fallback ohne Internet
int  marketsFetch(void (*progress)(int idx, bool ok)); // blockierend, liefert Anzahl Live-Maerkte
uint32_t marketsAgeMs();                             // Zeit seit letztem erfolgreichem Abruf

// ---------------------------------------------------------------- WLAN
void netInit();                         // gespeicherte Zugangsdaten laden + verbinden (nicht blockierend)
bool netConnected();
bool netHasCreds();
const char* netSSID();
const char* netIP();
int  netScan(char names[][33], int8_t* rssi, bool* secured, int maxCount);   // blockierend
bool netConnect(const char* ssid, const char* pass, uint32_t timeoutMs);      // blockierend, speichert bei Erfolg
void netForget();

// ---------------------------------------------------------------- Updates
// Neueste Firmware bei GitHub; Download nur auf Wunsch, mit gepruefter TLS-Verbindung, Groesse und SHA-256
struct FwUpdate { char version[16]; char name[48]; char url[200]; uint32_t size; char sha256[65]; };
bool updateCheck(const char* current, FwUpdate& out);   // blockierend; true = neuere Version gefunden
// blockierend; schreibt auf die SD-Karte (Datei out.name im Hauptordner); 0 = ok, sonst Fehlercode
int  updateDownload(const FwUpdate& u, void (*progress)(uint32_t got, uint32_t total));

// ---------------------------------------------------------------- Stream
// Browser im selben Netz: http://margincall.local  bzw.  http://<IP>
void streamPause(bool pause);           // waehrend TLS-Abruf Puffer freigeben
bool streamWantsFrame();
void streamSubmitFrame(const uint16_t* fbSwapped565, int w, int h);
int  streamViewers();                   // Anzahl aktiver Zuschauer (letzte 3 s)
