# MARGIN CALL

![MARGIN CALL – Bulls vs. Bears vs. You vs. AI](docs/banner.png)

**Sprache:** [English](README.md) · **Deutsch**

Ein Retro-Ego-Shooter rund um die Börse: Bullen und Bären kämpfen gegeneinander und gegen dich, die echten Tageskurse von Dow Jones, S&P 500, Nasdaq-100 und Rohöl bestimmen das Kräfteverhältnis, und die KI ALGO-9000 manipuliert den Markt.

**Bullrun und Bärenmarkt:** Erlegst du viele Bären, kippt der Markt in einen **Bullrun** (schneller, mehr Bargeld, 1,5-fache Punkte, dazu eine Bullen-Stampede, der du ausweichen musst). Erlegst du viele Bullen, folgt ein **Bärenmarkt** mit **Börsencrash**: Der Boden bebt, Bargeld schlittert weg, Bären greifen im Rudel an – dafür bringen Bären doppelte **Short-Gewinne**. Der echte Kurs gibt die Grundstimmung vor; ein Zeiger über der Bullen/Bären-Leiste zeigt, wohin der Markt kippt.

## Im Browser spielen

Ganz ohne Installation: **[MARGIN CALL im Browser spielen](https://wolfgangrabenstein.github.io/margincall/?lang=de)**. Solo und Online-Multiplayer funktionieren auch dort; für Multiplayer im eigenen Netzwerk braucht es die App.

## Downloads

Aktuelle Version **3.26.2** – Datei anklicken, der Download startet sofort:

| Download | System |
|---|---|
| [**MarginCall-3.26.2-Windows.exe**](https://github.com/wolfgangrabenstein/margincall/releases/download/v3.26.2/MarginCall-3.26.2-Windows.exe) | Windows 10/11 |
| [**MarginCall-3.26.2-macOS.dmg**](https://github.com/wolfgangrabenstein/margincall/releases/download/v3.26.2/MarginCall-3.26.2-macOS.dmg) | macOS 10.15 und neuer (Intel und Apple Silicon) |
| [**MarginCall-3.26.2-Linux.AppImage**](https://github.com/wolfgangrabenstein/margincall/releases/download/v3.26.2/MarginCall-3.26.2-Linux.AppImage) | Linux (x86_64) |
| [**MarginCall-3.26.2-Cardputer.bin**](https://github.com/wolfgangrabenstein/margincall/releases/download/v3.26.2/MarginCall-3.26.2-Cardputer.bin) | M5Stack Cardputer ADV, über den M5Launcher von der SD-Karte |

Neuere Version erschienen? Immer aktuell: [neueste Version](https://github.com/wolfgangrabenstein/margincall/releases/latest) · ältere: [alle Releases](https://github.com/wolfgangrabenstein/margincall/releases) · Änderungen: [CHANGELOG](CHANGELOG.de.md)

## Screenshots

| Titelbild mit Live-Kursen | Level 1: Bullen, Bären und fliegende Geldscheine | Online spielen |
|---|---|---|
| ![Titelbild](docs/screenshot-title.png) | ![Spielszene](docs/screenshot-level1.png) | ![Online spielen](docs/screenshot-online.png) |

Made by Wolfgang Rabenstein · Open Source unter der [MIT-Lizenz](LICENSE) · [Code signing policy](CODE_SIGNING_POLICY.md)

## Erster Start

- **Windows:** SmartScreen meldet eine unbekannte App. Klicke auf „Weitere Informationen“ und dann auf „Trotzdem ausführen“. Fragt die Firewall nach, erlaube den Zugriff im privaten Netzwerk (nötig für den Multiplayer).
- **macOS:** Die App ist nicht von Apple notarisiert.
  1. Öffne die `.dmg`-Datei und ziehe MARGIN CALL in den Ordner „Programme“.
  2. Führe im Terminal einmal `xattr -cr "/Applications/MARGIN CALL.app"` aus.
  3. Starte die App danach normal.
- **Linux:**
  1. Mache die Datei ausführbar: `chmod +x MarginCall-Linux.AppImage`.
  2. Starte sie mit `./MarginCall-Linux.AppImage`.
  3. Bleibt das Fenster leer, fehlt eventuell `libfuse2` (Ubuntu: `sudo apt install libfuse2`).

## Steuerung (PC)

| | Standard |
|---|---|
| Laufen | W / S oder ↑ / ↓ |
| Seitwärts | A / D |
| Drehen | Maus (ins Bild klicken) oder ← / → |
| Feuern | Leertaste oder linke Maustaste |
| Ausweichen | Richtung doppelt antippen |
| Pause | Esc oder P |
| Weiterspielen | Esc oder P im Pausenmenü |
| Zurück ins Hauptmenü | Q im Pausenmenü |
| Beenden | Q im Hauptmenü, dann mit Y bestätigen (N bricht ab) |
| Solo spielen: Einfach, Normal oder Hardcore | Enter im Hauptmenü, dann E (Einfach: schwächere Gegner, unbegrenzt Leben), Enter (Normal: 5 Leben) oder X (Hardcore: ein Leben) |
| Credits | C im Hauptmenü |
| Sprache (Englisch/Deutsch) | L im Hauptmenü oder Optionen → Sprache; Cardputer: L im Titelbild |
| Vollbild | F4 |
| Musik | M |
| Optionen (Tastenbelegung) | O |
| Spielfigur wählen (Multiplayer) | C in der Lobby |
| Highscores | H im Titelbild |
| Chat (Multiplayer) | T |
| Statistik einblenden | Tab (gedrückt halten) |

Alle Tasten lassen sich im Optionsmenü umbelegen. Auf dem Cardputer gilt weiterhin: E/S laufen, A/D drehen, W/R seitwärts.

## Online-Multiplayer (2–4 Spieler übers Internet)

1. Wähle **[N] Multiplayer → [O] Online spielen** (in der Browser-Version: **[N] Multiplayer**) und stimme einmal dem Datenschutzhinweis zu.
2. **Öffentliches Spiel:** Drücke **[H]**. Dein Spiel erscheint in der Liste offener Spiele für alle mit derselben Spielversion.
3. **Privates Spiel:** Drücke **[P]** und gib den fünfstelligen Code, der oben rechts in der Lobby steht, an deine Freunde weiter.
4. **Beitreten:** Wähle ein Spiel aus der Liste oder tippe den Code ein und drücke **Enter**.

Online-Spielen ist kostenlos und läuft über einen kleinen Spiel-Server bei Cloudflare. Das kostenlose Tageskontingent teilen sich alle Spieler weltweit; ist es aufgebraucht, pausiert Online-Spielen bis 00:00 UTC (das eigene Netzwerk funktioniert weiter).

## Multiplayer (2–4 Spieler im selben Netzwerk)

1. Ein Spieler wählt **[N] Multiplayer → [H] Spiel hosten**. Die Lobby zeigt die Adresse, z. B. `192.168.1.20:8080`.
2. Die anderen wählen **[N] Multiplayer → [J] Spiel beitreten** und klicken das Spiel in der Liste an. Wird es nicht gefunden (manche Router blockieren das), geben sie die Adresse aus der Lobby des Hosts ein oder öffnen sie im Browser.
3. Mitspieler melden sich mit **E** (oder Klick) **bereit**. In der Lobby kann man mit **T** chatten, mit **K** den eigenen Nickname ändern; der Host ändert mit **G** den Spielnamen.
4. Der Host wählt den Modus (Koop, Team Bulle vs. Team Bär oder Deathmatch) und startet, sobald alle bereit sind.

## Projektaufbau

| Ordner | Inhalt |
|---|---|
| `pc/game.html` | Das Spiel (Rendering, Logik, Sounds, Musik, Multiplayer) |
| `pc/server.py` | Entwicklungs-Server (Python), gleiches Verhalten wie der App-Server |
| `desktop/` | Desktop-App (Tauri 2): eingebauter Server in Rust plus Spielfenster |
| `src/`, `platformio.ini` | Cardputer-Firmware (PlatformIO, Arduino) |
| `tools/` | Generatoren für Soundeffekte und das App-Icon |
| `docs/` | Titelbild und Screenshots für die GitHub-Seite |
| `worker/` | Online-Rangliste (Cloudflare Worker + D1), Einrichtung siehe [worker/README.md](worker/README.md) |

### Selbst bauen

- **Desktop:** Benötigt Node 20+, Rust und unter Linux die WebKitGTK-Pakete. Bauen mit `cd desktop && npm install && npx tauri build`.
- **Cardputer:** Bauen mit `pio run`.
- **Alle Plattformen:** Das erledigt automatisch der GitHub-Workflow `.github/workflows/build.yml`.

## Datenschutz

Live-Kurse werden nur geladen, wenn du beim ersten Start zustimmst. Dann fragt das Spiel öffentliche Kursdaten bei Yahoo Finance ab, wobei Yahoo wie bei jedem Webseitenaufruf deine IP-Adresse sieht. Ändern kannst du das jederzeit unter *Optionen → Live-Kurse*. In die Online-Rangliste kommt nur, was du ausdrücklich hochlädst: Nach einem Lauf gehen dein selbst gewählter Name und deine Punkte erst an den Ranglisten-Server, wenn du „Speichern & online stellen“ (Enter) drückst. Mit TAB speicherst du nur lokal, mit ESC gar nicht; dauerhaft umstellen geht unter *Options → Online leaderboard*. Online-Multiplayer verbindet sich erst, nachdem du einem eigenen Hinweis zugestimmt hast: Dein Nickname, der Spielname und das laufende Spiel werden über den MARGIN-CALL-Server bei Cloudflare an die Mitspieler weitergeleitet, der keine Spielinhalte speichert. Sonst überträgt das Spiel nichts; im Multiplayer im eigenen Netzwerk verbindet es sich nur mit dem Host, den du auswählst. Details: [Code signing policy → Privacy policy](CODE_SIGNING_POLICY.md#privacy-policy--datenschutz).

## Code signing policy

Free code signing provided by [SignPath.io](https://about.signpath.io/), certificate by [SignPath Foundation](https://signpath.org/). Ablauf, Rollen und Datenschutz: [CODE_SIGNING_POLICY.md](CODE_SIGNING_POLICY.md).

## Lizenz & Mitmachen

MARGIN CALL steht unter der [MIT-Lizenz](LICENSE). Beiträge sind willkommen; bitte beachte den [Code of Conduct](CODE_OF_CONDUCT.md).
