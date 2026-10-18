#pragma once
// Audio fuer MARGIN CALL: Bank-/Geld-Samples + live synthetisierte Chiptune-Musik

enum SfxId {
  SFX_ID_KACHING,   // Registrierkasse (Cash eingesammelt, Spielstart)
  SFX_ID_COINS,     // Muenzen (Dividende)
  SFX_ID_SHOT,      // Money Gun
  SFX_ID_HIT,       // Treffer
  SFX_ID_STAMP,     // Gegner liquidiert
  SFX_ID_PHONE,     // Margin Call (Spieler getroffen)
  SFX_ID_ZAP,       // Gegner-Schuss
  SFX_ID_BELL,      // Eroeffnungs-/Schlussglocke
  SFX_ID_CRASH,     // Game Over
  SFX_ID_DECLINED,  // keine Munition / Tresor gesperrt
  SFX_ID_POP,       // Krypto-Blase platzt
  SFX_ID_BULL,      // Bulle bruellt (Angriff)
  SFX_ID_ROAR,      // Baer knurrt
  SFX_ID_GLITCH,    // KI ALGO-9000 greift ein
  SFX_ID_OIL,       // Oelfass / Oelschlick
  SFX_ID_LASER,     // Trading-Bot-Laser
  SFX_ID_DODGE,     // Ausweichen (Whoosh)
  SFX_ID_KEYCARD,   // Keycard aufgehoben (Zugang gewaehrt)
  SFX_COUNT
};

void audioInit();
void audioUpdate();               // jeden Frame aufrufen (fuettert den Musik-Stream)
void sfxPlay(SfxId id);
void musicStart(int song = 0);    // ab Songanfang; 0 Titel/Abspann, 1-5 Level, 6 Steueroase
void musicStop();
void musicToggle();
bool musicEnabled();
void musicSetPanic(bool panic);   // schnelleres Tempo bei wenig Depotwert
void musicSetPaused(bool paused);  // Spiel-Pause: Musik anhalten, danach an derselben Stelle weiter
