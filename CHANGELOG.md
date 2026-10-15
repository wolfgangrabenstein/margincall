# MARGIN CALL – Changelog

## Version 3.6.1 – Solo-Knopf im Startmenü (15.10.2026)

- **Eigener Knopf „▶ SOLO SPIELEN“:** Der große, leicht pulsierende Knopf im Startmenü startet den Einzelspieler per Klick oder Enter.
- **Kein versehentlicher Start mehr:** Ein Klick irgendwo ins Titelbild startet kein Spiel mehr.
- **Kürzere Statusanzeige:** Bei abgeschalteten Live-Kurse heißt es jetzt nur noch „SIMULIERT“. Der lange Text ragte im HUD über den Rand.

## Version 3.6.0 – Highscore-Liste (15.10.2026)

- **Highscore-Liste für den Einzelspieler:** Sie zeigt die besten 10 Läufe mit Name, P&L, erreichtem Level bzw. „SIEG“ und Datum.
  - Im Titelbild öffnest du sie über **[H] HIGHSCORES**, per Taste oder Klick.
- **Namen eintragen:** Wer nach einem Sieg in die Top 10 kommt, gibt seinen Namen ein (bis 12 Zeichen). Der zuletzt verwendete Name ist vorausgefüllt. Der neue Eintrag blinkt in der Liste.
- **Aufgeben nach Insolvenz:** Nach einer Insolvenz hast du jetzt die Wahl:
  - **[ENTER] Neuer Kredit:** Weiterspielen mit halbiertem P&L, wie bisher.
  - **[H] Aufgeben & Highscore:** Der Lauf endet, und du kannst dich eintragen.
- **Speicherort:** Die Liste liegt in einer Datei im Benutzerordner und bleibt deshalb auch bei Updates erhalten.
  - Windows: `%APPDATA%\MarginCall\highscores.json`
  - macOS: `~/Library/Application Support/MarginCall/`
  - Linux: `~/.local/share/MarginCall/`
- **Schreibschutz:** Mitspieler im WLAN können deine Liste nicht verändern. Einträge nimmt die App nur vom eigenen Rechner an.

## Version 3.5.3 – Menüs mit der Maus (15.10.2026)

- **Alles per Mausklick bedienbar:**
  - Multiplayer: **Spiel hosten**, **Spiel beitreten**, **Verbinden**, **Zurück**
  - Lobby: Spielmodus über die Pfeile **◀ ▶**, **Figurenwahl** durch Klick auf die Figur oder auf „Wechseln“, **Team Bulle** / **Team Bär**, **Spiel starten**, **Verlassen**
  - Pausenmenü (**Weiter**, **Optionen**, **Beenden**), Multiplayer-Menü im Spiel und Datenschutz-Hinweis
- **Hervorhebung:** Schaltflächen leuchten auf, wenn die Maus darüber ist, und der Mauszeiger wird zur Hand.
- **Tastatur:** Alle Tastenkürzel funktionieren weiterhin.

## Version 3.5.2 – Musik in den Optionen (15.10.2026)

- **Neuer Schalter „Musik“** im Optionsmenü **[O]**: Er schaltet die Musik ein und aus.
- **Gespeichert:** Die Einstellung bleibt auch nach einem Neustart erhalten, ebenso wenn du die Musik mit der Taste **M** umschaltest.

## Version 3.5.1 – Datenschutz und Open Source (15.10.2026)

- **Live-Kurse nur mit Zustimmung:** Beim ersten Start erklärt ein Hinweis, dass die Kurse bei Yahoo Finance abgerufen werden. Mit **Enter** stimmst du zu, mit **N** spielst du mit simulierten Kursen.
- **Neuer Schalter „Live-Kurse“** im Optionsmenü, jederzeit änderbar.
- **Kein automatischer Abruf mehr:** Die App ruft beim Start nichts mehr von sich aus ab, sondern erst, wenn das Spiel Kurse anfragt.
- **Open Source:** MARGIN CALL steht jetzt unter der **MIT-Lizenz**. Neu sind außerdem eine Code signing policy mit Datenschutzerklärung und ein Code of Conduct.
- **Vorbereitung der Windows-Signatur** über die SignPath Foundation: Das Build-Skript reicht die EXE zur Signatur ein, sobald die SignPath-Zugangsdaten hinterlegt sind.

## Version 3.5.0 – Spielfiguren, Esc-Menü und Vollbild (15.10.2026)

- **Zwei Spielfiguren im Multiplayer:** In der Lobby wechselst du mit **[C]** zwischen männlich und weiblich. Rechts in der Lobby siehst du deine Figur groß.
  - **Männlich:** Hoodie in deiner Spielerfarbe mit „$“-Aufdruck, Goldkette, Basecap, Headset, Bart, Jeans und Sneaker.
  - **Weiblich:** langer roter Pferdeschwanz, Kleid in deiner Spielerfarbe mit Goldgürtel, goldene Creolen, Sonnenbrille im Haar und Stiefel.
  - Beide Figuren sehen bewusst ganz anders aus als die Gegner im Anzug.
  - Im Team-Modus tragen sie die Teamfarbe, dazu Hörner (Team Bulle) oder Bärenohren (Team Bär).
  - Die Wahl wird gespeichert und beim Beitreten zu einem Host mitgenommen.
- **Esc pausiert das Spiel.**
  - **Esc im Pausenmenü beendet das Spiel.** Damit ein einzelner Esc-Druck nicht gleich beendet, reagiert das Menü erst nach einer kurzen Pause.
  - **P** pausiert und setzt fort wie bisher.
  - Bei Maussteuerung reicht ein Esc: Der erste Druck gibt die Maus frei und pausiert gleichzeitig.
  - Im Multiplayer öffnen Esc und P das Multiplayer-Menü. Esc darin beendet, **[L]** verlässt das Spiel, **P** führt zurück.
  - Im Titelbild beendet **Esc** das Spiel.
- **Q beendet das Spiel nicht mehr.**
- **F4** schaltet den Vollbildmodus ein und aus.
- **Versionsnummer im Dateinamen:** Die Downloads heißen jetzt z. B. `MarginCall-3.5.0-Windows.exe`, `MarginCall-3.5.0-macOS.dmg`, `MarginCall-3.5.0-Linux.AppImage` und `MarginCall-3.5.0-Cardputer.bin`.

## Version 3.4.1 – WASD am PC (15.10.2026)

- **Neue Standardsteuerung am PC** nach dem üblichen Shooter-Schema:

  | Aktion | Tasten |
  |---|---|
  | Vorwärts / Rückwärts | W / S (zusätzlich ↑ / ↓) |
  | Seitwärts | A / D |
  | Drehen | Maus oder ← / → |
  | Feuern | Leertaste oder linke Maustaste |

- **Ausweichen** per Doppeltipp funktioniert mit den neuen Tasten: W, S, A und D jeweils zweimal schnell antippen.
- **Gespeicherte Belegung:** Die Tastenbelegung wird einmalig auf die neue Standardbelegung zurückgesetzt. Eigene Belegungen lassen sich wie bisher im Optionsmenü **[O]** einstellen.
- **Touch-Knöpfe** auf dem Handy: oben A ▲ D, unten Drehen ⟲ ▼ ⟳.
- Der **Cardputer** bleibt bei **E/S laufen, A/D drehen, W/R seitwärts**.

## Version 3.4 – Eigene App für Windows, macOS und Linux (15.10.2026)

- **Kein Browser mehr:** MARGIN CALL ist jetzt eine eigenständige App mit eigenem Fenster. Es gibt eine Datei pro System:

  | System | Datei |
  |---|---|
  | Windows | `MarginCall-Windows.exe` |
  | macOS | `MarginCall-macOS.dmg` (Intel und Apple Silicon) |
  | Linux | `MarginCall-Linux.AppImage` |

  Live-Kurse, Musik, Sounds und Multiplayer sind eingebaut, zusätzliche Software ist nicht nötig.
- **Beenden:** Das Spiel lässt sich jetzt beenden, über **[Q] Beenden** im Titelbild, im Pausenmenü und im Multiplayer-Menü. Das Schließen des Fensters beendet die App ebenfalls.
- **Multiplayer-Menü:** **[N] Multiplayer** bietet jetzt die Wahl zwischen **[H] Spiel hosten** und **[J] Spiel beitreten**.
  - Beim Beitreten gibst du die Adresse des Hosts ein, die in seiner Lobby steht.
  - Deine Tastenbelegung wird dabei mitgenommen.
  - Endet das Spiel des Hosts, bringt dich **[H]** zurück zu deinem eigenen Spiel.
  - Mitspieler im Browser funktionieren weiterhin.
- **Im Multiplayer** öffnet die Pause-Taste ein kleines Menü mit **[L] Spiel verlassen** und **[Q] Beenden**. Das Spiel läuft dabei weiter.
- **Credits:** „Made by Wolfgang Rabenstein“ steht im Titelbild und auf den Endbildschirmen, am PC und auf dem Cardputer.

## Version 3.3 – Ausweichen (15.10.2026)

- **Ausweichen per Doppeltipp** wie in Unreal Tournament: Tippst du eine Bewegungsrichtung zweimal schnell hintereinander an (innerhalb von 0,25 s), springt deine Figur ein Stück in diese Richtung.
- Das geht **nach vorne, nach hinten, nach links und nach rechts**.
  - Am PC zählen die Tasten aus dem Optionsmenü, also auch die Pfeiltasten.
  - Auf dem Cardputer sind es E, S, W und R.
- Ein Sprung bringt dich etwa 1,75 Felder weit in einer Viertelsekunde, danach kannst du 0,6 s lang nicht erneut ausweichen.
- Klebst du im Öl fest, ist der Sprung nur halb so weit.
- Beim Ausweichen hörst du einen Whoosh-Sound und siehst Bewegungsstreifen am Bildrand.
- Ausweichen funktioniert auch im Multiplayer.

## Version 3.2 – Musik pausiert mit (15.10.2026)

- **Pause:** Pausierst du das Spiel, pausiert auch die Musik, am PC und auf dem Cardputer. Beim Weiterspielen läuft sie an derselben Stelle weiter.
- **PC, Tab im Hintergrund:** Wechselst du in einen anderen Browser-Tab oder minimierst das Fenster, verstummen Musik und Sounds, bis du zurückkommst.
- **PC, Spiel-Server geschlossen:** Wird `MarginCall.exe` beendet, während das Spiel im Browser noch offen ist, hält das Spiel nach wenigen Sekunden an und zeigt „Spiel-Server beendet“. Bisher lief die Musik in diesem Fall im Browser weiter. Startest du die EXE neu, geht es weiter.

## Version 3.1 – PC-Version: Optionen und Multiplayer (15.10.2026)

Dieses Update betrifft nur `MarginCall.exe`. Die Cardputer-Version bleibt auf Stand 3.0.

### Neu: Optionsmenü mit Tastatur- und Mausbelegung

- Das Optionsmenü öffnest du mit **[O]** im Titelbild, in der Pause oder in der Lobby.
- Jede Aktion hat **zwei frei wählbare Belegungen**. Das kann eine Taste oder eine Maustaste sein (links, Mitte, rechts, Maus 4/5).
- Belegbare Aktionen:
  - Vorwärts und Rückwärts
  - Links und rechts drehen
  - Seitwärts links und rechts
  - Feuern
  - Pause
  - Musik
- Ist eine Taste schon belegt, wird sie bei der alten Aktion automatisch entfernt. So gibt es keine Doppelbelegung.
- Maus:
  - Maussteuerung an/aus
  - Empfindlichkeit in 10 Stufen
  - Invertieren
- **Entf** leert eine Belegung. **Standard wiederherstellen** setzt alles zurück.
- Die Einstellungen werden im Browser gespeichert und gelten beim nächsten Start weiter.

### Neu: Multiplayer im WLAN (2–4 Spieler)

- Im Titelbild öffnet **[N]** die Lobby.
  - Der erste Spieler in der Lobby ist Host. Er wählt mit ←/→ den Modus und startet mit Enter.
  - Mitspieler öffnen im selben WLAN die Adresse aus dem Konsolenfenster (`http://<IP>:8080`) und drücken ebenfalls **[N]**.
- **Koop:**
  - Ihr spielt gemeinsam alle 5 Level. Mit jedem Mitspieler werden die Gegner zäher, und es kommt mehr Nachschub.
  - Wer stirbt, steigt nach 6 Sekunden am Levelstart wieder ein, mit 60 % Depotwert und 10 % weniger P&L.
- **Team Bulle gegen Team Bär:**
  - Du wählst mit **[1]** Team Bulle oder mit **[2]** Team Bär.
  - Die Tiere deiner Fraktion greifen dich nicht an und können von dir auch nicht getroffen werden. Sie kämpfen gegen das andere Team.
  - Steigt der Level-Markt heute, macht Team Bulle 25 % mehr Schaden. Fällt er, gilt das für Team Bär.
  - Eine Runde dauert 4 Minuten oder bis ein Team 3.000 Punkte hat.
- **Deathmatch:**
  - Jeder gegen jeden, die Börsen-Gegner mischen weiter mit. Ein Abschuss bringt 500 P&L.
  - Eine Runde dauert 4 Minuten oder bis jemand 3.000 P&L hat.
- In Team und Deathmatch gibt es keine Bosse. Nach jeder Runde geht es im nächsten Level weiter.
- Mitspieler erscheinen als Trader im farbigen Anzug (Grün, Blau, Orange, Pink) mit Namensschild und Lebensbalken. Im Team-Modus tragen Team Bulle goldene Hörner und Team Bär Bärenohren.
- Im Multiplayer gibt es keine Pause.
- Verlässt der Host das Spiel, landen alle wieder im Titelbild.

## Version 3 – „Bullen gegen Bären“ (15.10.2026)

Version 3 verbindet das Spiel mit der echten Börse. Bullen und Bären kämpfen jetzt nicht nur gegen dich, sondern auch gegeneinander. Wie stark welche Seite ist, entscheidet der echte Markt des Tages.

### Auf einen Blick: Version 2 und Version 3

| | Version 2 | Version 3 |
|---|---|---|
| Level | 3 | **5** (neu: Öl-Raffinerie, KI-Rechenzentrum) |
| Gegnertypen | 4 | **9** |
| Börsendaten | keine, alles erfunden | **echte Tageskurse** von Dow Jones, S&P 500, Nasdaq-100 und Rohöl (WTI) |
| Bullen und Bären | nur Bären | **beide**, sie bekämpfen sich gegenseitig |
| Gegner-Nachschub | feste Gegnerzahl pro Level | **laufender Nachschub** je nach Marktlage |
| KI-Gegenspieler | – | **ALGO-9000** stört das Spiel und ist der Endboss |
| WLAN | – | **WLAN-Menü** direkt im Spiel |
| Zuschauen | – | **Bildschirm-Stream** im Browser fürs ganze Netzwerk |
| PC-Version | – | **MarginCall.exe** für Windows mit Live-Kursen |
| Soundeffekte | 11 | **16** |

### Neu: Echte Börsenkurse

- Ist der Cardputer im WLAN, lädt das Spiel beim Start die aktuellen Kurse von **Dow Jones, S&P 500, Nasdaq-100 und Rohöl (WTI)**.
- Ohne WLAN laufen die Kurse simuliert. Im Spiel sind sie dann mit `SIM` bzw. `*` gekennzeichnet.
- Jedes Level hängt an einem Markt:
  - Level 1: Dow Jones
  - Level 2: S&P 500
  - Level 3: Nasdaq-100
  - Level 4: Öl
  - Level 5: Mittelwert der drei Indizes
- **Kräfteverhältnis:** Steigt der Index heute, tauchen mehr und stärkere Bullen auf. Fällt er, gilt das für die Bären.
- **Intraday-Verlauf:** Der echte Tageschart läuft im Hintergrund mit. Seine kurzfristige Richtung bestimmt, ob als Nächstes ein Bulle oder ein Bär nachrückt.
- **Ölpreis:**
  - Steigt Öl, feuern alle Gegner schneller („Energiekrise“).
  - Fällt Öl, bist du schneller unterwegs.
- **Anzeigen:**
  - Ein Laufband oben im Bild zeigt alle vier Kurse.
  - Die Laufschrift-Wände zeigen den echten Tagesverlauf.
  - Die Chart-Tafeln zeigen echte Candlesticks.
- Vor jedem neuen Level werden die Kurse aktualisiert, wenn der letzte Abruf älter als 2 Minuten ist.

### Neu: Bullen gegen Bären

- **Der Bulle** ist ein neuer Gegner mit goldenen Hörnern. Er stürmt mit gesenktem Kopf auf sein Ziel los und rammt es.
- Bullen und Bären greifen dich an, **hassen sich aber gegenseitig noch mehr**. Treffen sie aufeinander, gehen sie zuerst aufeinander los.
- Du kannst sie gegeneinander ausspielen. Gegner, die von anderen erledigt werden, zählen am Levelende als „Marktbereinigung“.
- Short Seller kämpfen für die Bären. Krypto-Blasen kämpfen für die Bullen.
- Ein neuer Balken im HUD zeigt das aktuelle Kräfteverhältnis: grün sind die Bullen, rot die Bären.

### Neu: ALGO-9000, die manipulierende KI

Ab Level 1 greift ALGO-9000 regelmäßig ins Marktgeschehen ein. Im letzten Level passiert das deutlich öfter. Jeder Eingriff wird von einem roten KI-Auge und Störgeräuschen begleitet. Diese Eingriffe gibt es:

- **Fake News:** Das Kurs-Laufband zeigt falsche Kurse, und zusätzliche Gegner tauchen auf.
- **Flash Crash:** Alle Bullen und Bären greifen nur noch dich an, und zwar schneller.
- **Trading-Bots:** Fliegende Drohnen schießen mit Lasern auf dich.
- **Pump & Dump:** Ein Schwarm Krypto-Blasen erscheint.
- **Systemmanipulation:** Das Bild flackert, und für ein paar Sekunden sind Drehen und Seitwärtslaufen vertauscht.
- **Aufhetzen:** Bullen und Bären werden stärker.

In **Level 5 (Rechenzentrum)** ist ALGO-9000 der **Endboss**:
- Er feuert Laserfächer und ruft laufend Bots herbei.
- Bei halber Energie löst er einen Flash Crash aus.
- Der Tresor zum Ausgang öffnet sich erst, wenn ALGO-9000 abgeschaltet ist.

### Neu: Öl-Level und Öl-Gegner

- **Level 4 „Raffinerie“** spielt zwischen Rohrleitungen und Ölfässern.
- **Ölbaron:** trägt Zylinder, Monokel und Zigarre und wirft Ölfässer. Ein Treffer bremst dich kurz aus.
- **Ölschlick:** ein schwarzes, glänzendes Monster. Erwischt es dich, klebst du 3 Sekunden lang fest („Ölpest“).

### Neu: WLAN und Bildschirm-Stream

- Im Titelbild öffnet **[W]** das WLAN-Menü:
  1. Netz mit `;` und `.` auswählen.
  2. Passwort eintippen. **Tab** blendet es ein oder aus.
  3. Mit Enter verbinden.
- Die Zugangsdaten werden auf dem Cardputer gespeichert. Ab dem nächsten Start verbindet er sich automatisch.
- **[F]** im WLAN-Menü löscht die gespeicherten Zugangsdaten.
- **Stream:** Jeder im selben Netzwerk kann im Browser zuschauen, unter `http://margincall.local` oder der IP-Adresse, die im Titelbild und im Pausenmenü steht.
  - Die Zuschauer sehen das Spiel live und darunter die aktuellen Kurse.
  - Schaut jemand zu, leuchtet im HUD ein roter Punkt.
  - Das WLAN-Menü und damit dein Passwort wird **nie** übertragen.

### Neu: PC-Version (`MarginCall.exe`)

- Ein Doppelklick startet das Spiel im Browser, mit echten, minütlich aktualisierten Kursen.
- Die Spiellogik ist dieselbe wie auf dem Cardputer, nur in höherer Auflösung.
- Steuerung:
  - Tastatur wie auf dem Cardputer
  - Zusätzlich die Pfeiltasten
  - Maussteuerung: ins Bild klicken, dann dreht die Maus die Ansicht, Linksklick feuert
- Andere Geräte im selben WLAN können über die angezeigte Adresse (`http://<IP>:8080`) mitspielen, auch auf dem Handy mit Touch-Steuerung.
- Windows warnt beim ersten Start, weil die Datei nicht signiert ist. Klicke dann auf „Weitere Informationen“ und „Trotzdem ausführen“.

### Neue Sounds

| Sound | Wann |
|---|---|
| Bullen-Brüllen | Ein Bulle stürmt los oder trifft |
| Bären-Knurren | Ein Bär schlägt zu |
| Daten-Kreischen | ALGO-9000 greift ein |
| Öl-Gluckern | Ölbaron wirft, Ölschlick trifft |
| Laser | Trading-Bots und ALGO-9000 feuern |

Alle Sounds aus Version 2 sind weiterhin dabei, dazu die Musik, die bei wenig Depotwert schneller wird.

### Geändert

- **Schaden:** Dein Bullen-Blaster macht jetzt 2 Schaden pro Treffer, weil die Gegner robuster geworden sind.
- **Spielende:** Am Levelende und beim Game Over stehen getrennt deine eigenen Abschüsse und die Marktbereinigung.
- **Titelbild:** Es zeigt alle vier Kurse mit Tagesveränderung und dem Hinweis LIVE oder SIM, dazu den WLAN- und Stream-Status.
- **HUD:** Neu sind der Kräftebalken Bullen gegen Bären und die Tagesveränderung des Level-Markts.
- **Siegbildschirm:** Er heißt jetzt „ALGO-9000 besiegt – der Markt ist frei“.

### Unverändert

- Steuerung:
  - E/S vor/zurück
  - A/D drehen
  - W/R seitwärts
  - Leertaste/Enter feuern
  - P Pause
  - M Musik
- Installation über den M5Launcher: `MarginCall.bin` auf die SD-Karte kopieren und über **SD → Install** installieren.

### Bekannte Einschränkungen

- **Kursquelle:** Die Kurse kommen von Yahoo Finance. Fällt die Quelle aus oder ändert sich ihr Format, spielt das Spiel automatisch mit simulierten Kursen weiter.
- **Stream:** Je nach WLAN-Qualität zeigt er nur wenige Bilder pro Sekunde und hat weniger Farbtiefe als das Display.
- **Speicher:** Die Cardputer-Version nutzt den Speicher fast vollständig aus. Läuft beim Kursabruf etwas schief, zeigt der Ladebildschirm „FEHLER – SIMULIERT“, und das Spiel läuft mit simulierten Kursen weiter.
