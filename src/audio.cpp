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
  SFX(BULL, 7), SFX(ROAR, 7), SFX(GLITCH, 6), SFX(OIL, 4), SFX(LASER, 4), SFX(DODGE, 3),
};

void sfxPlay(SfxId id) {
  const SfxDef& s = SFX_TABLE[id];
  M5Cardputer.Speaker.playRaw(s.data, s.len, SFX_RATE, false, 1, s.ch, true);
}

// ---------------------------------------------------------------- Song
// Notenwerte: MIDI-Nummer, 0 = Ton halten, 1 = Pause
static const uint8_t MEL_A[64] = {
  69, 0, 72, 0, 76, 0, 72, 74, 76, 0, 79, 0, 77, 76, 74, 72,
  72, 0, 69, 0, 65, 0, 69, 72, 77, 0, 76, 0, 74, 0, 72, 0,
  67, 0, 72, 0, 76, 79, 76, 72, 79, 0, 84, 0, 83, 81, 79, 76,
  79, 0, 77, 79, 81, 0, 79, 0, 74, 0, 71, 0, 74, 76, 77, 1,
};
static const uint8_t MEL_B[64] = {
  74, 77, 81, 77, 74, 77, 81, 86, 81, 77, 74, 77, 81, 0, 79, 77,
  76, 80, 83, 80, 76, 80, 83, 88, 83, 80, 76, 80, 83, 0, 81, 80,
  81, 0, 76, 0, 72, 0, 76, 81, 84, 0, 83, 0, 81, 0, 76, 0,
  80, 0, 83, 0, 88, 0, 86, 84, 83, 81, 80, 78, 76, 0, 0, 1,
};
// Akkorde pro Takt: Grundton (Bass) + 3 Akkordtoene (fuer Arpeggios)
struct Chord { uint8_t root, a, b, c; };
static const Chord CH_A[4] = {{45, 57, 60, 64}, {41, 53, 57, 60}, {48, 55, 60, 64}, {43, 55, 59, 62}};   // Am F C G
static const Chord CH_B[4] = {{38, 50, 53, 57}, {40, 52, 56, 59}, {45, 57, 60, 64}, {40, 52, 56, 59}};   // Dm E Am E

// Teile eines Durchlaufs (je 4 Takte); danach Tonart +1 Halbton ("Bull Run")
enum PartType { P_A, P_A_COW, P_B, P_B_FILL, P_CRAZY, P_A_SLIDE, NUM_PARTS };

static const char KICK_A[] = "x...x...x...x...";
static const char KICK_B[] = "x...x...x.x.x..x";
static const char SNARE[]  = "....x.......x...";
static const char FILL[]   = "x.x.xxxxxxxxxxxx";
static const char HAT[]    = "xoxoxoxoxoxoxoxo";
static const char COW[]    = "..x..x....x..x..";
static const char BASS_A[] = "R.O.R.O.R.O.R.OR";
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
  const Chord& ch = (isB ? CH_B : CH_A)[bar];
  const int tr = transpose();

  // Melodie
  uint8_t m = (isB ? MEL_B : MEL_A)[step];
  if (part == P_CRAZY) m = 1;                         // im Crazy-Teil: nur Arpeggio
  if (m == 1) lead.sustain = 0, lead.decay = 0.997f;
  else if (m > 1) { lead.decay = 0.9996f; noteOn(lead, m + tr, 0.55f, part == P_A_SLIDE); }
  lead.duty = part == P_A_COW ? 0x40000000u : 0x80000000u;

  // Echo-Stimme (3 Steps verzoegert)
  uint8_t e = echoLine[0];
  memmove(echoLine, echoLine + 1, 3); echoLine[3] = (part == P_A_COW || part == P_B_FILL) ? m : 1;
  if (e > 1) { noteOn(echo, e + tr + 12, 0.3f); echo.decay = 0.9993f; echo.duty = 0x20000000u; }

  // Arpeggio im Crazy-Teil
  if (part == P_CRAZY || part == P_B_FILL) {
    arpNotes[0] = ch.a + tr + 12; arpNotes[1] = ch.b + tr + 12; arpNotes[2] = ch.c + tr + 12;
    arp.env = part == P_CRAZY ? 1.0f : 0.5f; arp.decay = 0.99985f;
  }
  if (part == P_CRAZY && s == 0) { laserEnv = 1.0f; laserF = 2400; }

  // Bass
  const char bc = (isB ? BASS_B : BASS_A)[s];
  if (bc == 'R') { noteOn(bass, ch.root + tr, 0.4f); bass.decay = 0.9992f; }
  else if (bc == 'O') { noteOn(bass, ch.root + tr + 12, 0.35f); bass.decay = 0.9990f; }
  bass.duty = 0x40000000u;

  // Drums
  const char* kick = (isB || part == P_CRAZY) ? KICK_B : KICK_A;
  if (kick[s] == 'x') { kickEnv = 1.0f; kickF = 170; }
  const char* sn = (part == P_B_FILL && bar == 3) ? FILL : SNARE;
  if (sn[s] == 'x' && part != P_CRAZY) snareEnv = (sn == FILL) ? 0.5f + s * 0.03f : 1.0f;
  if (HAT[s] == 'x') { hatEnv = 0.6f; hatDecay = 0.993f; }
  else if (HAT[s] == 'o') { hatEnv = 0.8f; hatDecay = 0.9985f; }
  if ((part == P_A_COW || part == P_A_SLIDE) && COW[s] == 'x') cowEnv = 1.0f;

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
  const int bpm = panic ? 196 : 160;
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

void musicStart() {
  part = step = loopCount = 0; stepLeft = 0;
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
