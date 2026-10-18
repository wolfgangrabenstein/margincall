#include "audio.h"
#include <M5Cardputer.h>
#include <math.h>
#include "sfx_data.h"

// ---------------------------------------------------------------- Kanaele
static constexpr int CH_MUSIC = 0;

struct SfxDef { const int8_t* data; size_t len; uint8_t ch; };
#define SFX(name, ch) { SFX_##name, sizeof(SFX_##name), ch }
static const SfxDef SFX_TABLE[SFX_COUNT] = {
  SFX(KACHING, 3), SFX(COINS, 3), SFX(SHOT, 1), SFX(HIT, 2), SFX(STAMP, 2), SFX(PHONE, 5),
  SFX(ZAP, 4), SFX(BELL, 6), SFX(CRASH, 6), SFX(DECLINED, 7), SFX(POP, 2),
  SFX(BULL, 7), SFX(ROAR, 7), SFX(GLITCH, 6), SFX(OIL, 4), SFX(LASER, 4), SFX(DODGE, 3), SFX(KEYCARD, 3),
};

void sfxPlay(SfxId id) {
  const SfxDef& s = SFX_TABLE[id];
  M5Cardputer.Speaker.playRaw(s.data, s.len, SFX_RATE, false, 1, s.ch, true);
}

// ---------------------------------------------------------------- Song
// Notenwerte: MIDI-Nummer, 0 = Ton halten, 1 = Pause. Akkorde pro Takt: Grundton (Bass) + 3 Akkordtoene (Arpeggios)
struct Chord { uint8_t root, a, b, c; };
struct Song { uint8_t bpm; const char* kick; const char* bass; bool cow, arp; uint8_t a[64], b[64]; Chord ca[4], cb[4]; };
// 0 Titel/Abspann, 1-5 Level, 6 Steueroase: gleiche Instrumente, eigene Melodie, Akkorde, Tempo und Groove
static const Song SONGS[] = {
  {160, "x...x...x...x...", "R.O.R.O.R.O.R.OR", false, false,   // Titel (Original)
   {69,0,72,0,76,0,72,74,76,0,79,0,77,76,74,72,72,0,69,0,65,0,69,72,77,0,76,0,74,0,72,0,67,0,72,0,76,79,76,72,79,0,84,0,83,81,79,76,79,0,77,79,81,0,79,0,74,0,71,0,74,76,77,1},
   {74,77,81,77,74,77,81,86,81,77,74,77,81,0,79,77,76,80,83,80,76,80,83,88,83,80,76,80,83,0,81,80,81,0,76,0,72,0,76,81,84,0,83,0,81,0,76,0,80,0,83,0,88,0,86,84,83,81,80,78,76,0,0,1},
   {{45,57,60,64}, {41,53,57,60}, {48,55,60,64}, {43,55,59,62}}, {{38,50,53,57}, {40,52,56,59}, {45,57,60,64}, {40,52,56,59}}},
  {152, "x...x...x...x...", "R...O...R...O.O.", false, false,   // Dow Jones: Schalterhalle, festlicher Marsch in C-Dur
   {76,0,79,0,84,0,79,0,76,0,74,76,79,0,0,0,81,0,79,0,76,0,72,0,76,0,81,0,79,0,76,0,77,0,81,0,84,0,81,0,77,0,79,81,79,0,77,0,74,0,79,0,83,0,86,0,83,0,79,0,74,76,77,1},
   {81,84,81,77,81,84,89,0,84,81,77,81,84,0,81,79,83,86,83,79,83,86,91,0,86,83,79,83,86,0,84,83,79,0,76,0,71,0,76,79,83,0,81,0,79,0,76,0,81,0,84,0,88,0,86,84,83,81,79,76,81,0,0,1},
   {{48,55,60,64}, {45,57,60,64}, {41,53,57,60}, {43,55,59,62}}, {{41,53,57,60}, {43,55,59,62}, {40,52,55,59}, {45,57,60,64}}},
  {168, "x...x...x...x.x.", "RORORORORORORORO", false, false,   // S&P 500: Parkett, hektisch in e-Moll
   {76,79,83,79,76,79,83,88,86,83,79,83,76,0,1,76,79,0,76,0,72,0,76,79,84,0,83,0,79,0,76,0,78,81,86,81,78,81,86,90,88,86,81,78,74,0,76,78,75,0,78,0,83,0,81,79,78,0,75,0,71,0,0,1},
   {84,0,83,81,76,0,81,0,84,0,88,0,86,84,83,81,78,0,81,0,86,0,84,83,81,0,78,0,74,0,76,78,79,83,86,83,79,83,86,91,90,86,83,79,86,0,83,0,83,0,87,0,90,0,88,87,85,83,81,78,75,0,0,1},
   {{40,52,55,59}, {48,55,60,64}, {38,50,54,57}, {47,51,54,59}}, {{45,57,60,64}, {38,50,54,57}, {43,55,59,62}, {47,51,54,59}}},
  {150, "x.....x...x.....", "R..R..O.R..R..O.", false, false,   // Nasdaq-100: Tresor, geheimnisvoll in d-Moll
   {74,0,1,81,0,1,77,0,76,0,74,0,69,0,0,1,74,0,1,77,0,1,82,0,81,0,77,0,74,0,0,1,79,0,1,82,0,1,86,0,84,0,82,0,79,0,81,82,81,0,0,0,76,0,73,0,76,0,81,0,79,77,76,1},
   {82,0,81,0,79,0,74,0,79,0,82,0,86,0,84,82,84,0,82,0,79,0,76,0,79,0,84,0,88,0,86,84,81,0,84,0,89,0,88,0,86,0,84,0,81,0,77,0,76,0,81,0,85,0,88,0,86,0,85,0,81,0,0,1},
   {{38,50,53,57}, {46,53,58,62}, {43,55,58,62}, {45,57,61,64}}, {{43,55,58,62}, {48,55,60,64}, {41,53,57,60}, {45,57,61,64}}},
  {140, "x..x..x.x..x..x.", "RR..RR..RR..RRO.", false, false,   // Oel: Raffinerie, schwer und stampfend in g-Moll
   {67,0,0,67,70,0,67,0,74,0,72,0,70,0,69,0,67,0,0,67,70,0,67,0,75,0,74,0,72,0,70,0,65,0,0,65,69,0,72,0,77,0,75,0,74,0,72,0,74,0,0,0,69,0,66,0,69,0,74,0,72,70,69,1},
   {72,0,75,0,79,0,75,0,72,0,67,0,72,0,74,75,74,0,70,0,67,0,70,0,74,0,79,0,77,0,74,0,75,0,79,0,82,0,79,0,75,0,70,0,75,0,77,79,78,0,81,0,86,0,84,0,81,0,78,0,74,0,0,1},
   {{43,55,58,62}, {39,51,55,58}, {41,53,57,60}, {38,50,54,57}}, {{48,55,60,63}, {43,55,58,62}, {39,51,55,58}, {38,50,54,57}}},
  {176, "x...x...x...x...", "R.ROR.ROR.ROR.RO", false, true,   // ALGO-9000: Rechenzentrum, elektronisch in fis-Moll
   {78,81,85,90,85,81,78,81,85,0,90,0,88,0,85,0,86,0,81,0,78,0,81,86,90,0,88,0,86,0,81,0,88,0,83,0,80,0,83,88,92,0,90,0,88,0,83,0,85,0,80,0,77,0,80,85,89,0,0,0,85,0,0,1},
   {83,0,86,0,90,0,86,83,78,0,83,0,86,0,85,83,81,0,86,0,90,0,88,86,81,0,78,0,81,0,83,85,85,0,88,0,93,0,92,90,88,0,85,0,81,0,83,85,80,0,85,0,89,0,92,0,89,0,85,0,80,0,0,1},
   {{42,54,57,61}, {38,50,54,57}, {40,52,56,59}, {49,53,56,61}}, {{47,50,54,59}, {38,50,54,57}, {45,57,61,64}, {49,53,56,61}}},
  {150, "x...x...x...x...", "R..OR.O.R..OR.O.", true, false,   // Steueroase: sonnig, Calypso in F-Dur
   {72,0,77,0,1,81,0,77,0,1,84,0,81,0,77,0,74,0,77,0,1,82,0,77,0,1,86,0,84,0,82,0,76,0,79,0,1,84,0,79,0,1,88,0,86,0,84,82,81,0,0,0,77,0,72,0,77,0,81,0,79,0,77,1},
   {77,0,81,0,86,0,81,0,77,0,74,0,77,0,81,0,77,0,82,0,86,0,82,0,77,0,74,0,77,0,82,0,79,0,82,0,86,0,84,0,82,0,81,0,79,0,77,0,76,0,79,0,84,0,88,0,86,0,84,0,82,79,76,1},
   {{41,53,57,60}, {46,53,58,62}, {48,55,60,64}, {41,53,57,60}}, {{38,50,53,57}, {46,53,58,62}, {43,55,58,62}, {48,55,60,64}}},
};
static constexpr int NUM_SONGS = sizeof(SONGS) / sizeof(SONGS[0]);
static const Song* song = &SONGS[0];

// Teile eines Durchlaufs (je 4 Takte); danach Tonart +1 Halbton ("Bull Run")
enum PartType { P_A, P_A_COW, P_B, P_B_FILL, P_CRAZY, P_A_SLIDE, NUM_PARTS };

static const char KICK_B[] = "x...x...x.x.x..x";
static const char SNARE[]  = "....x.......x...";
static const char FILL[]   = "x.x.xxxxxxxxxxxx";
static const char HAT[]    = "xoxoxoxoxoxoxoxo";
static const char COW[]    = "..x..x....x..x..";
static const char BASS_B[] = "RRORRORRORRORROR";

// ---------------------------------------------------------------- Synth
static constexpr int MUSIC_RATE = 16000;          // Musik-Samplerate
static constexpr int CHUNK = 1024;
static int8_t chunkBuf[3][CHUNK];
static int bufIdx = 0;

static bool enabled = true, running = false, panic = false, paused = false;
static int part = 0, step = 0, loopCount = 0, stepLeft = 0;

static uint32_t noteInc[128];
static uint32_t lfsr = 0xACE1u;
static inline float noise() { lfsr ^= lfsr << 13; lfsr ^= lfsr >> 17; lfsr ^= lfsr << 5; return (int32_t)lfsr * (1.0f / 2147483648.0f); }

struct Voice { uint32_t ph = 0, inc = 0, target = 0; float env = 0, decay = 0.9995f, sustain = 0; uint32_t duty = 0x80000000u; };
static Voice lead, echo, bass, arp;
static float kickEnv, kickF, snareEnv, hatEnv, hatDecay, cowEnv, laserEnv, laserF;
static uint32_t kickPh, snarePh, cowPh1, cowPh2, laserPh;
static float hatPrev;
static uint8_t echoLine[4];
static int arpCounter, arpIdx;
static uint8_t arpNotes[3];
static int lfoCounter; static float vib;

static inline int transpose() { return loopCount % 6; }

static void noteOn(Voice& v, int midi, float sustain, bool slide = false) {
  if (midi < 0) midi = 0; if (midi > 127) midi = 127;
  v.target = noteInc[midi];
  if (!slide || v.inc == 0) v.inc = v.target;
  v.env = 1.0f; v.sustain = sustain;
}

static void nextStep() {
  const bool isB = part == P_B || part == P_B_FILL;
  const int bar = step / 16, s = step % 16;
  const Chord& ch = (isB ? song->cb : song->ca)[bar];
  const int tr = transpose();

  // Melodie
  uint8_t m = (isB ? song->b : song->a)[step];
  if (part == P_CRAZY) m = 1;                         // im Crazy-Teil: nur Arpeggio
  if (m == 1) lead.sustain = 0, lead.decay = 0.997f;
  else if (m > 1) { lead.decay = 0.9996f; noteOn(lead, m + tr, 0.55f, part == P_A_SLIDE); }
  lead.duty = part == P_A_COW ? 0x40000000u : 0x80000000u;

  // Echo-Stimme (3 Steps verzoegert)
  uint8_t e = echoLine[0];
  memmove(echoLine, echoLine + 1, 3); echoLine[3] = (part == P_A_COW || part == P_B_FILL) ? m : 1;
  if (e > 1) { noteOn(echo, e + tr + 12, 0.3f); echo.decay = 0.9993f; echo.duty = 0x20000000u; }

  // Arpeggio im Crazy-Teil
  if (part == P_CRAZY || part == P_B_FILL || (song->arp && s % 8 == 0)) {
    arpNotes[0] = ch.a + tr + 12; arpNotes[1] = ch.b + tr + 12; arpNotes[2] = ch.c + tr + 12;
    arp.env = part == P_CRAZY ? 1.0f : part == P_B_FILL ? 0.5f : 0.3f; arp.decay = 0.99985f;
  }
  if (part == P_CRAZY && s == 0) { laserEnv = 1.0f; laserF = 2400; }

  // Bass
  const char bc = (isB ? BASS_B : song->bass)[s];
  if (bc == 'R') { noteOn(bass, ch.root + tr, 0.4f); bass.decay = 0.9992f; }
  else if (bc == 'O') { noteOn(bass, ch.root + tr + 12, 0.35f); bass.decay = 0.9990f; }
  bass.duty = 0x40000000u;

  // Drums
  const char* kick = (isB || part == P_CRAZY) ? KICK_B : song->kick;
  if (kick[s] == 'x') { kickEnv = 1.0f; kickF = 170; }
  const char* sn = (part == P_B_FILL && bar == 3) ? FILL : SNARE;
  if (sn[s] == 'x' && part != P_CRAZY) snareEnv = (sn == FILL) ? 0.5f + s * 0.03f : 1.0f;
  if (HAT[s] == 'x') { hatEnv = 0.6f; hatDecay = 0.993f; }
  else if (HAT[s] == 'o') { hatEnv = 0.8f; hatDecay = 0.9985f; }
  if ((part == P_A_COW || part == P_A_SLIDE || (song->cow && !isB)) && COW[s] == 'x') cowEnv = 1.0f;

  // weiter
  if (++step >= 64) {
    step = 0;
    if (++part >= NUM_PARTS) { part = 0; loopCount++; }
  }
}

static inline float pulse(Voice& v) {
  v.ph += v.inc;
  return v.ph < v.duty ? 1.0f : -1.0f;
}

static void renderChunk(int8_t* out) {
  const int bpm = panic ? song->bpm * 122 / 100 : song->bpm;
  const int stepLen = MUSIC_RATE * 60 / (bpm * 4);
  for (int i = 0; i < CHUNK; i++) {
    if (--stepLeft <= 0) { stepLeft = stepLen; nextStep(); }

    if (++lfoCounter >= 64) {
      lfoCounter = 0;
      static float lfoPh = 0; lfoPh += 64 * 6.0f * 6.2832f / MUSIC_RATE;   // 6 Hz Vibrato
      vib = sinf(lfoPh);
      // Portamento
      if (lead.inc != lead.target) lead.inc += ((int32_t)lead.target - (int32_t)lead.inc) / 6;
    }
    float v = 0;

    // Lead mit Vibrato
    if (lead.env > 0.001f) {
      uint32_t inc = lead.inc + (int32_t)((lead.inc >> 8) * vib * 1.5f);
      lead.ph += inc;
      float o = lead.ph < lead.duty ? 1.0f : -1.0f;
      lead.env = lead.env * lead.decay; if (lead.env < lead.sustain) lead.env = lead.sustain;
      v += o * lead.env * 24;
    }
    if (echo.env > 0.001f) { echo.env *= echo.decay; if (echo.env < echo.sustain) echo.env = echo.sustain; v += pulse(echo) * echo.env * 9; echo.sustain *= 0.9999f; }
    if (bass.env > 0.001f) { bass.env *= bass.decay; if (bass.env < bass.sustain) bass.env = bass.sustain; v += pulse(bass) * bass.env * 22; bass.sustain *= 0.9997f; }
    if (arp.env > 0.001f) {
      if (++arpCounter >= 320) { arpCounter = 0; arpIdx = (arpIdx + 1) % 3; arp.inc = noteInc[arpNotes[arpIdx] & 127]; }
      arp.env *= arp.decay; arp.duty = 0x60000000u;
      v += pulse(arp) * arp.env * 16;
    }
    // Kick: Sinus-Sweep
    if (kickEnv > 0.001f) {
      kickF = 45 + (kickF - 45) * 0.9965f;
      kickPh += (uint32_t)(kickF * (4294967296.0f / MUSIC_RATE));
      v += sinf(kickPh * (6.2832f / 4294967296.0f)) * kickEnv * 46;
      kickEnv *= 0.99945f;
    }
    // Snare: Rauschen + Ton
    if (snareEnv > 0.001f) {
      snarePh += (uint32_t)(190 * (4294967296.0f / MUSIC_RATE));
      v += (noise() * 0.8f + (snarePh < 0x80000000u ? 0.25f : -0.25f)) * snareEnv * 26;
      snareEnv *= 0.9986f;
    }
    // Hi-Hat: hochpassgefiltertes Rauschen
    if (hatEnv > 0.001f) { float n = noise(); v += (n - hatPrev) * hatEnv * 6; hatPrev = n; hatEnv *= hatDecay; }
    // Cowbell
    if (cowEnv > 0.001f) {
      cowPh1 += (uint32_t)(562 * (4294967296.0f / MUSIC_RATE)); cowPh2 += (uint32_t)(845 * (4294967296.0f / MUSIC_RATE));
      v += ((cowPh1 < 0x80000000u ? 1 : -1) + (cowPh2 < 0x80000000u ? 1 : -1)) * cowEnv * 6;
      cowEnv *= 0.9985f;
    }
    // Laser-Sweep im Crazy-Teil
    if (laserEnv > 0.001f) {
      laserF *= 0.99975f;
      laserPh += (uint32_t)(laserF * (4294967296.0f / MUSIC_RATE));
      v += (laserPh < 0x80000000u ? 1 : -1) * laserEnv * 9;
      laserEnv *= 0.99985f;
    }
    if (v > 127) v = 127; else if (v < -127) v = -127;
    out[i] = (int8_t)v;
  }
}

// ---------------------------------------------------------------- API
void audioInit() {
  for (int n = 0; n < 128; n++) noteInc[n] = (uint32_t)(440.0 * pow(2.0, (n - 69) / 12.0) * 4294967296.0 / MUSIC_RATE);
  memset(echoLine, 1, sizeof(echoLine));
  M5Cardputer.Speaker.setVolume(150);
  M5Cardputer.Speaker.setChannelVolume(CH_MUSIC, 130);
}

void musicStart(int songIdx) {
  song = &SONGS[songIdx >= 0 && songIdx < NUM_SONGS ? songIdx : 0];
  part = step = loopCount = 0; stepLeft = 0; panic = false;   // Tempo bleibt nach dem Spiel nicht im Panik-Modus haengen
  lead = Voice(); echo = Voice(); bass = Voice(); arp = Voice();
  kickEnv = snareEnv = hatEnv = cowEnv = laserEnv = 0;
  memset(echoLine, 1, sizeof(echoLine));
  running = true; paused = false;
}

void musicStop() {
  running = false;
  M5Cardputer.Speaker.stop(CH_MUSIC);
}

void musicToggle() {
  enabled = !enabled;
  if (!enabled) M5Cardputer.Speaker.stop(CH_MUSIC);
}
bool musicEnabled() { return enabled; }
void musicSetPanic(bool p) { panic = p; }
void musicSetPaused(bool p) {
  if (p && !paused) M5Cardputer.Speaker.stop(CH_MUSIC);
  paused = p;
}

void audioUpdate() {
  if (!running || !enabled || paused) return;
  while (M5Cardputer.Speaker.isPlaying(CH_MUSIC) < 2) {
    renderChunk(chunkBuf[bufIdx]);
    M5Cardputer.Speaker.playRaw(chunkBuf[bufIdx], CHUNK, MUSIC_RATE, false, 1, CH_MUSIC, false);
    bufIdx = (bufIdx + 1) % 3;
  }
}
