# MARGIN CALL – Changelog

**Sprache:** [English](CHANGELOG.md) · **Deutsch**

## Version 3.18.0 – Keycards, zufällige Level, sichtbare Schüsse (18.10.2026)

- **Zufällige Level:** Bei jedem Spielstart sind die Gegner jedes Levels (außer der Steueroase) zufällig verteilt, und die Tresortür sitzt jedes Mal an einer anderen Stelle der Außenwand.
- **Keycards:** Der Tresor öffnet sich nur mit der Keycard des Levels, und jeder Level hat seine eigene Farbe (gelb, blau, rot, grün, violett).
  - In Leveln mit Boss lässt der Boss die Keycard fallen.
  - In den anderen Leveln fällt sie, nachdem eine zufällige Zahl an Gegnern erledigt wurde.
  - Ohne Keycard meldet die Tür, welche Farbe du brauchst. Hast du sie, zeigt das HUD sie an.
- **Sichtbare Schüsse:** Die Waffe feuert fliegende Geldscheine ab. Schaden gibt es erst, wenn ein Schein trifft, und Wände halten ihn auf.
- **Waffe überarbeitet:** Der Lauf sitzt jetzt auf einer verchromten Halterung im Koffer, mit Messingringen und einer Zuführung aus dem Geldscheinmagazin.
- **Piraten bewachen den Schatz:** Die meisten Piraten bleiben bei den Münzhaufen. Greifst du einen an oder kommst zu nah, wehren er und seine Kameraden sich, verfolgen dich aber nur bis zu einer gewissen Entfernung und kehren dann im Zickzack zum Schatz zurück. Nur wenige jagen dich direkt.
- **Pausenmenü:** Esc setzt das Spiel fort, Q geht ins Hauptmenü.
- **Beenden:** Esc im Hauptmenü fragt jetzt nach, ob du das Spiel wirklich beenden willst. Enter beendet, Esc bricht ab.

## Version 3.17.3 – Munition auf der Steueroase (18.10.2026)

- **Bargeld auf der Insel:** Auf der Steueroase liegen jetzt auch vereinzelt Geldbündel als Munition, zu Beginn 3 (Hardcore 2). Alle 10 bis 14 Sekunden (Hardcore 14 bis 18) kommt ein neues dazu, solange weniger als 2 herumliegen.

## Version 3.17.2 – eigene Musik für jedes Level, Lautstärkeregler (16.10.2026)

- **Jedes Level hat seine eigene Musik**, mit denselben Chiptune-Instrumenten wie bisher, damit alles zusammenpasst:
  - **Dow Jones (Schalterhalle):** festlicher Marsch in C-Dur
  - **S&P 500 (Parkett):** hektisch in e-Moll
  - **Nasdaq-100 (Tresor):** geheimnisvoll in d-Moll
  - **Öl (Raffinerie):** schwer und stampfend in g-Moll
  - **ALGO-9000 (Rechenzentrum):** elektronisch mit Arpeggios in fis-Moll
  - **Steueroase:** sonnige Calypso-Melodie in F-Dur mit Cowbell
  - Titelbildschirm und Abspann behalten das bekannte Titellied.
- **Fehler behoben:** Nach dem Spiel lief die Musik manchmal deutlich zu schnell, weil das hektischere Tempo bei wenig Depot hängen blieb.
- **Lautstärkeregler:** In den Optionen lassen sich Musik und Soundeffekte jetzt in zehn Stufen regeln, statt Musik nur an- oder auszuschalten. Die Taste M schaltet die Musik weiterhin stumm und stellt danach die vorherige Lautstärke wieder her.
- **Ranglisten zurückgesetzt:** Die Online-Rangliste startet neu.

## Version 3.17.1 – Trefferfeedback, schlauere Piraten, neue Waffe (16.10.2026)

- **Schlussszene überarbeitet:**
  - Die Meme-Sonnenbrille sitzt jetzt die ganze Zeit auf und wird nicht mehr vom Kopf verdeckt.
  - Es ist derselbe Trader wie im HUD: braune Haare, blauer Anzug, weißer Kragen, rote Krawatte, barfuß im Liegestuhl.
  - Der Kopf wirkt realistischer und passt in der Größe zur Brille: breites Gesicht, schmaleres Kinn, abgerundeter Hinterkopf, Nase, Ohren, Haare mit Pony und ein Hals.
  - Das verschmitzte Grinsen mit hochgezogenem Mundwinkel ist gut zu sehen.
  - Die Szene ist heller ausgeleuchtet, und die Kamera endet frontal auf Augenhöhe.
- **Mehr und schlauere Piraten in der Steueroase:**
  - Zu Beginn 8 statt 5 (Hardcore 10), gleichzeitig bis zu 12 (Hardcore 14).
  - Sie sind etwas schneller als du, brauchen nach dem Erscheinen aber kurz, bis sie losstürmen.
  - Auch wenn du umzingelt bist, trifft dich höchstens alle 0,5 Sekunden ein Säbel (Hardcore 0,35 s).
  - Drei Rollen:
    - **Wächter** lauern an einem Münzhaufen und greifen erst an, wenn du in die Nähe kommst.
    - **Abfänger** laufen dorthin, wo du gleich sein wirst.
    - **Jäger** verfolgen dich direkt.
- **Neue Waffe:** eine Geldkoffer-Kanone, ein Aktenkoffer mit Chromlauf, Banknotenbündel als Magazin und kleinem Kursdisplay, gehalten von Trader-Händen im Anzug.
- **Neues Dividenden-Icon:** ein Münzstapel mit großem grünen Pfeil nach oben, besser erkennbar als die alte Münze.

- **Treffer spürst du jetzt:**
  - **Roter Bildblitz:** Das ganze Bild färbt sich kurz rot, je mehr Schaden, desto stärker.
  - **Wackeln:** Das Bild wackelt kurz.
  - **Kurssturz:** Der Schaden fällt als „▼ −12 %“ durchs Bild.
- **Trader-Gesicht im HUD:**
  - grinst bei vollem Depot, schwitzt bei mittlerem, gerät bei wenig Depot in Panik
  - verzieht bei Treffern das Gesicht und schaut ab und zu zur Seite
  - hat X-Augen, wenn du pleite bist
- **Cardputer:** roter Bildblitz und Schadenszahl.
- **GitHub-Seite zweisprachig:** README auf Englisch (Hauptseite) und Deutsch (`README.de.md`), oben umschaltbar.

## Version 3.17.0 – „Deal with it“ und eine knallharte Steueroase (16.10.2026)

- **Meme-Sonnenbrille in der Schlussszene:** Der Spieler bekommt die bekannte „Deal with it“-Pixelbrille, schwarze Blöcke mit weißen Glanzpixeln, auf einem größeren, kastigen Kopf, der genau zur Brille passt, mit schrägem Grinsen. Sie schwebt wie im Meme von oben auf sein Gesicht herab, kurz bevor „EARLY RETIREMENT.“ erscheint.
- **Steueroase deutlich härter:**
  - Nur noch 45 statt 75 Sekunden.
  - 42 statt 30 Goldmünzen.
  - Die Piraten halten 60 % mehr aus und sind 15 % schneller.
  - Alle Münzen zu holen ist damit fast unmöglich.

## Version 3.16.1 – Online-Rangliste mit einem Knopfdruck (16.10.2026)

- **Online eintragen ist jetzt vorausgewählt.** Nach einem Lauf stellt **„[ENTER] Speichern & online stellen“** Name und Punkte in die Online-Rangliste. Es wird weiterhin nichts automatisch übertragen: Jeder Upload braucht diesen ausdrücklich beschrifteten Knopf.
- **Nur lokal:** Mit **TAB** speicherst du nur lokal, mit **ESC** gar nicht. Dauerhaft umstellen kannst du das unter *Options → Online leaderboard*. Wer die Online-Rangliste schon früher ausgeschaltet hat, bleibt bei „nur lokal“.
- Datenschutz-Hinweis beim ersten Start, README und Datenschutzerklärung sind angepasst, inklusive Rechtsgrundlage, Widerspruch und Löschung.

## Version 3.16.0 – Piraten, Schlussszene, größere Steueroase, sichtbarer Timer (16.10.2026)

- **Piraten statt Bären in der Steueroase:** drei Typen, die sich nur im Aussehen unterscheiden:
  - **Kapitän:** roter Mantel, Bart, Dreispitz mit Totenkopf
  - **Matrose:** Ringelhemd, Kopftuch, Augenklappe
  - **Freibeuter:** blaue Weste, Ohrring, Haken
  Beim Angriff heben sie den Säbel.
- **Schlussszene:** Nach der Steueroase folgt eine kurze, in Echtzeit gerenderte Szene. Die Kamera fährt im Sonnenuntergang auf deine Yacht zu: Du liegst mit Sonnenbrille im Liegestuhl und schlürfst Cocktails. Mit Enter überspringbar, auch auf dem Cardputer.
- **Steueroase größer und härter:**
  - Die Insel ist rund ein Viertel größer.
  - 30 statt 20 Goldmünzen.
  - Zu Beginn 5 statt 3 Piraten, sie rücken schneller nach: bis zu 8, in Hardcore 10.
  - Nur noch 75 statt 90 Sekunden, alle Münzen zu holen ist damit fast unmöglich.
- **Timer gut sichtbar:** Die Restzeit steht groß oben rechts im Bild, darunter der Münzstand. In den letzten 15 Sekunden wird sie rot und blinkt, auch auf dem Cardputer.
- **Hardcore noch etwas härter:** Gegner haben das 1,8-fache statt 1,6-fache an Energie, feuern 45 % statt 35 % öfter, ihre Geschosse sind 30 % statt 25 % schneller, und du nimmst das 1,65-fache statt 1,5-fache an Schaden.
- **Namen bis 15 Zeichen:** für Highscores (lokal und online) und Nicknames, bisher 12.
- **Spielname nur noch beim Hosten:** Im Multiplayer-Menü gibt es kein Spielnamen-Feld mehr. Der Host benennt sein Spiel in der Lobby mit **[G]**.

## Version 3.15.0 – English by default, German optional (16.10.2026)

- **Das Spiel ist jetzt standardmäßig englisch.** Das gilt für Menüs, HUD, Meldungen, Gegner- und Levelnamen, Abspann, Ranglisten und Multiplayer. Zahlen erscheinen im englischen Format (z. B. 52,178 und +1.08 %).
- **Deutsch bleibt wählbar:**
  - PC/Desktop: unter *Options → Language*
  - Cardputer: mit **L** im Titelbild
  - Die Wahl wird gespeichert.
- Kleinere Verbesserungen.

## Version 3.14.0 (16.10.2026)

- Kleinere Verbesserungen.

## Version 3.13.0 – Abspann und Bonus-Level „Steueroase“ (16.10.2026)

- **Kurzer Abspann nach dem Endboss:** Nach dem Sieg über ALGO-9000 und dem Bullenmarkt-Bonus läuft ein Abspann von rund 15 Sekunden. Mit Enter lässt er sich überspringen.
- **Bonus-Level „Steueroase“:** Danach geht es auf eine Tropeninsel im Freien, mit ganz neuer Low-Poly-Grafik statt dunkler Gänge.
  - **Grafik:** Sonnenuntergang, Sandstrand, türkisfarbenes Flachwasser, glitzerndes Meer, Palmen, eine Strandbar, eine Luxusyacht vor der Küste und drehende Goldmünzen. Ein eigener kleiner 3D-Renderer zeichnet flach schattierte Dreiecke mit Sonnenlicht und Abenddunst.
  - **Ziel:** 90 Sekunden Zeit, 20 Goldmünzen einsammeln, während „Steuerfahnder-Bären“ dich jagen. Jede Münze bringt +500 $, alle 20 zusammen +10.000 $ extra, in Hardcore jeweils 1,5-fach.
  - **Kein Risiko für den Lauf:** Wirst du erwischt oder läuft die Zeit ab, endet nur das Bonus-Level, das gesammelte Gold bleibt dir. Danach folgt wie gewohnt der Highscore.
  - **Start:** Du bekommst volles Depot und +20 Cash.
- **Cardputer:** Abspann und Steueroase laufen auch auf dem Cardputer, mit 10 Feldern Sichtweite und sparsamerer Geometrie.

## Version 3.12.0 – Bullenmarkt-Bonus fürs Durchspielen (16.10.2026)

- **Wer ALGO-9000 besiegt und das Spiel durchspielt, bekommt einen Bullenmarkt-Bonus auf den Highscore:**
  - **+10.000 $** pauschal
  - **+25 %** des bis dahin erspielten P&L
  - **+5.000 $** für eine weiße Weste, also ohne einen einzigen Tod
  - In Hardcore zählt der Bonus wie alle Punkte 1,5-fach.
- Beispiel: Bei 20.900 $ P&L ohne Tod gibt es +20.225 $, in Hardcore +30.506 $.
- Der Siegbildschirm zeigt den Bonus an, in der Rangliste steht der Lauf weiter mit „SIEG“. Bisher brachte der letzte Tresor nur denselben kleinen Bonus wie jedes andere Level.
- Auch auf dem Cardputer, der dafür jetzt die Tode eines Laufs mitzählt.

## Version 3.11.0 – Hardcore-Modus, fairere Bosse (16.10.2026)

- **Hardcore-Modus:** Im Hauptmenü mit **X** (oder dem roten Knopf) starten, nur im Solo-Spiel.
  - **Ein Tod beendet den Lauf:** Statt „Neuer Kredit“ kommt **GAME OVER**, danach wird die Punktzahl eingetragen.
  - **Härtere Gegner:** 60 % mehr Energie, sie feuern 35 % öfter, ihre Geschosse fliegen 25 % schneller, und sie richten 50 % mehr Schaden an. ALGO-9000 greift häufiger an.
  - **Knapper Nachschub:** Start mit 20 statt 30 Cash, kein Auffüllen zwischen den Levels, 40 % weniger Beute. Cash gibt +7 statt +10, eine Dividende +15 % statt +20 %, verstreutes Bargeld liegt seltener herum.
  - **Belohnung:** Alle Punkte zählen 1,5-fach.
  - **Eigene Ranglisten:** Hardcore hat eine eigene lokale Top 10 und eigene Online-Listen (gesamt und heute). Im Highscore-Menü schaltest du mit **X** zwischen Normal und Hardcore um.
- **Bosse lassen sich nicht mehr hinter Sockeln austricksen:** Stand man halb hinter einer Kante, traf man den breiten Boss noch, er schoss aber gar nicht zurück, weil er nur seine Mittellinie prüfte. Jetzt prüfen Bosse und alle Fernkämpfer ihre ganze Breite und schießen von der freien Seite.
- **Erledigte Gegner verschwinden nach 7 statt nach 25 Sekunden.**
- **Esc führt erst ins Hauptmenü:** Aus Pause und Multiplayer-Menü bringt Esc dich ins Hauptmenü, ganz beendet wird das Spiel nur von dort.
- **Cardputer:** Hardcore (X im Titelbild), der Boss-Fix und die 7 Sekunden gibt es auch dort; Ranglisten gibt es auf dem Cardputer nicht.

## Version 3.10.1 – Online-Rangliste ist live (16.10.2026)

- **Die Online-Rangliste ist freigeschaltet.** Nach einem Solo-Lauf mit **Tab** „Auch online eintragen“ wählen. Im Highscore-Menü (**H**) siehst du dann unter [2] und [3] die weltweite Gesamt- und Tagesrangliste mit deinem Platz.
- Server: https://margincall-highscores.margincall.workers.dev (Cloudflare Worker + D1).

## Version 3.10.0 – Online-Rangliste vorbereitet, ruhigeres Licht (16.10.2026)

- **Licht flackert nur noch beim ALGO-Angriff:** Bisher flackerte es bei jeder starken Kursbewegung und während der ganzen Manipulation mehrmals pro Sekunde. Jetzt flackert es nur in den ersten 0,7 Sekunden, wenn ALGO-9000 zuschlägt (auch beim Kippen von Pump & Dump). Im Multiplayer flackert es bei allen gleichzeitig.
- **Online-Rangliste (startet, sobald der Server läuft):** Neben der lokalen Top 10 gibt es eine weltweite Rangliste.
  - **Reiter im Highscore-Menü:** [1] Lokal, [2] Online gesamt (Top 100), [3] Online heute. Umschalten mit 1–3 oder ←/→, blättern mit ↑/↓.
  - **Eigener Platz:** Nach dem Eintragen springt die Liste zu deinem Eintrag und zeigt z. B. „DEIN PLATZ: 23 VON 41“, auch außerhalb der Top 100.
  - **Freiwillig:** Nach jedem Lauf schaltest du mit **Tab** „Auch online eintragen“ ein. Die Einstellung wird gemerkt und ist auch unter *Optionen → Online-Rangliste* änderbar. Übertragen werden nur Name, Punkte, Level, Sieg, Tode und Spielversion.
  - **Server:** ein Cloudflare Worker im Ordner `worker/`. Solange er nicht eingerichtet ist, bleibt alles Online im Spiel ausgeblendet. Anleitung: [worker/README.md](worker/README.md).

## Version 3.9.0 – ALGO-9000 manipuliert den Kurs, Einzelaktien im Ticker (16.10.2026)

- **ALGO-9000 verschiebt den Spielkurs:** Jeder Angriff legt für 15–24 Sekunden einen Auf- oder Abschlag auf den echten Kurs des Levels. Alles, was am Kurs hängt, reagiert sofort darauf: das Verhältnis von Bullen zu Bären, ihre Stärke und im Team-Modus der Schadensbonus. Kurz vor Ende läuft die Manipulation langsam aus.
  - **Flash Crash:** −5 %
  - **Pump & Dump:** erst +4 %, nach 12 Sekunden kippt der Kurs auf −4 %
  - **Fake News:** ±2–4 %
  - **System manipuliert:** ±4 %
  - **Trading-Bots:** −1,5 bis −2,5 %
  - **Bullen & Bären aufgehetzt:** ±3 % in Richtung des echten Tagestrends
- **Manipulation sichtbar:** Der HUD zeigt echten und manipulierten Kurs nebeneinander, z. B. „DOW +1,08 % → ALGO −3,92 %“, blinkend in Magenta. Wand-Charts und Laufband bekommen eine magentafarbene Linie mit dem verbogenen Verlauf. Der Ticker zeigt für den Level-Index den ALGO-Kurs.
- **Licht flackert bei Kursausschlägen:** Bei starken echten Intraday-Bewegungen und bei Manipulation flackert das Licht kurz.
- **Einzelaktien im Ticker:** Hinter jedem Index laufen seine bekanntesten Werte mit Kurs und Tagesveränderung, grün steigend, rot fallend. Beim Dow sind das z. B. Goldman Sachs, Caterpillar und Boeing, beim Nasdaq-100 Nvidia, Microsoft und Tesla, beim Öl Exxon und Chevron. Die Einzelwerte sind nur Anzeige; das Spiel richtet sich weiter allein nach den Leitindizes.
- **Munition und Leben etwas knapper:**
  - Gegner lassen seltener etwas fallen: 38 % statt 45 %, bei Kämpfen zwischen Tieren 22 % statt 30 %.
  - Eine Dividende heilt 20 % statt 25 %.
  - Dafür liegt **vereinzelt Bargeld** abseits herum, etwa alle ein bis zwei Minuten, höchstens zwei Scheine gleichzeitig. Wer fast keine Munition mehr hat, findet schneller etwas.
- **Cardputer:** Alle Neuerungen gibt es auch auf dem Cardputer.

## Version 3.8.1 – Härterer Koop (16.10.2026)

- **Gegner skalieren stärker mit der Spielerzahl:**
  - **+60 % Energie** pro zusätzlichem Spieler, bisher +35 %.
  - **3 zusätzliche Gegner pro Mitspieler** zu Levelbeginn, passend zum Level: Bullen und Bären nach Marktlage, im Öl-Level auch Ölschlick und Ölbarone, im KI-Level Trading-Bots.
  - **Mehr und schnellerer Nachschub:** bis zu 4 Gegner mehr pro Mitspieler gleichzeitig, rund 50 % schneller pro Mitspieler.
- **Beispiel mit 2 Spielern:** 12 statt 9 Gegner zu Beginn, ein Bär hält 10 statt 6 Treffer aus.

## Version 3.8.0 – Nicknames, Spielnamen, Rundenpause, Tab-Statistik (16.10.2026)

- **Nickname für den Multiplayer:** Im Multiplayer-Menü und in der Lobby mit **[K]** (oder Klick) änderbar, bis 12 Zeichen.
  - Der Nickname erscheint in Lobby, Chat, Namensschildern, Meldungen und Statistik.
  - Er wird gespeichert und beim Beitreten mitgenommen.
- **Spielname:** Der Host benennt sein Spiel mit **[G]**, z. B. „Börsenparty“.
  - Der Name steht in der Lobby unter dem Titel.
  - In der Spieleliste unter „Beitreten“ erscheinen Spielname und Host.
- **Rundenpause im Deathmatch und Team-Modus:** Nach Rundenende kommt eine **Pause von 5 Sekunden** mit der Statistik aller Spieler und einem Countdown „Nächstes Level in 5 s“. Danach startet das nächste Level automatisch, der Host kann mit Enter sofort weiter.
- **Statistik im Spiel:** Solange du **Tab** gedrückt hältst, siehst du eine Tabelle aller Spieler mit P&L, Frags, Toden und Kills.
  - Im Solo-Modus zeigt sie deinen Lauf mit P&L, Level, Kills, Toden und Marktbereinigung.
  - Die Taste ist im Optionsmenü belegbar.
- **Bereit mit E:** Mitspieler melden sich in der Lobby mit **E** bereit. Enter und Leertaste ändern den Bereit-Status nicht mehr.
- **Behoben:**
  - Das Depot zeigt nie mehr „0 %“: Bist du ausgeschaltet, steht dort bis zum Wiedereinstieg **PLEITE**.
  - In der Rundenabrechnung überlappten Namen und Statistik.
  - Der Hinweis zum Weiterspielen lag über den Credits.

## Version 3.7.0 – Spielesuche, Lobby-Chat, Bereit-Knopf, Deathmatch-Statistik (16.10.2026)

- **Spiele im Netzwerk finden:** **[N] Multiplayer → [J] Spiel beitreten** zeigt jetzt automatisch alle offenen Spiele im WLAN, mit Host, Modus und Spielerzahl.
  - Ein Klick oder Enter genügt zum Beitreten, IP-Adresse und Port muss man nicht mehr eintippen.
  - Die manuelle Adresseingabe bleibt als Notlösung erhalten.
  - Ein Spiel wird angekündigt, solange der Host in der Lobby ist.
- **Bereit-Knopf:** Mitspieler bestätigen in der Lobby mit **„Bereit“**. Der Host kann erst starten, wenn alle bereit sind. Die Spielerliste zeigt ✔ BEREIT bzw. ✖ WARTET.
- **Chat in der Lobby:** **[T]** oder ein Klick ins Chatfeld öffnet die Eingabe, Enter sendet.
- **Chat im Spiel:** **[T]** öffnet oben ein Chatfenster. Gesendete Nachrichten erscheinen bei allen als Meldung oben im Ticker, z. B. „» BLAU: gg wp“. Während du schreibst, bleibt deine Figur stehen.
  - Die Chat-Taste ist im Optionsmenü frei belegbar.
- **Deathmatch-Statistik:** Das HUD zeigt deine **Frags** (abgeschossene Mitspieler), **Tode** und **Kills** (erledigte Börsen-Gegner), dazu eine Rangliste aller Spieler mit Frags und Toden. Die Rundenabrechnung zeigt dieselben Werte.
- **Vollbild:**
  - Neue Zeile **„Vollbild (F4)“** im Optionsmenü.
  - Die Einstellung wird gespeichert, die App startet dann gleich im Vollbild.
  - Im Hauptmenü steht unten links **[F4] VOLLBILD**, auch anklickbar.
- **Lobby neu aufgeteilt:** links die Spieler, in der Mitte der Chat, rechts die eigene Figur.
- **Teamfarben im Team-Modus:** Spieler heißen nach der Farbe ihres Teams („TRADER GRÜN“ für Team Bulle, „TRADER ROT“ für Team Bär) und erscheinen in dieser Farbe.
  - Das gilt in der Lobby (mit „TEAM BULLE · GRÜN“ / „TEAM BÄR · ROT“), auf den Namensschildern im Spiel und in der Abrechnung.
  - Teamkollegen werden durchnummeriert, z. B. „GRÜN 1“ und „GRÜN 2“.

## Version 3.6.2 – Tode zählen, Maus sofort aktiv, Bärengebrüll (15.10.2026)

- **Tode im Einzelspieler:**
  - Jeder Tod wird gezählt und kostet **25 % des P&L, mindestens 500 $**. Das bisherige Halbieren bei „Neuer Kredit“ entfällt dafür.
  - Der Insolvenz-Bildschirm zeigt die wievielte Insolvenz es ist und wie hoch die Strafe war.
  - Im HUD steht neben dem P&L ein ☠ mit der Zahl der Tode.
  - Die Highscore-Liste hat eine neue Spalte **Tode**.
- **Maussteuerung sofort aktiv:** Beim Spielstart, bei jedem neuen Level, nach „Neuer Kredit“ und nach der Pause muss man nicht mehr erst ins Bild klicken.
  - Im Multiplayer gilt das für den Host. Mitspieler sehen einen Hinweis „Klick ins Bild für Maussteuerung“, weil der Browser das Einfangen der Maus dort erst nach einem eigenen Klick erlaubt.
- **Der Bär brüllt:**
  - wenn er dich auf der Jagd entdeckt,
  - beim Zuschlagen,
  - und zwischendurch, wie der Bulle beim Sturmangriff.

  Das gilt am PC und auf dem Cardputer.

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

- **Ausweichen per Doppeltipp:** Tippst du eine Bewegungsrichtung zweimal schnell hintereinander an (innerhalb von 0,25 s), springt deine Figur ein Stück in diese Richtung.
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
