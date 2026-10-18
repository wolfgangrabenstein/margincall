# Online-Server (Cloudflare Worker)

Kleiner Server für MARGIN CALL: Online-Rangliste, Online-Multiplayer und Live-Kurse für die Browser-Version. Ein Cloudflare Worker mit einer D1-Datenbank (SQLite) und Durable Objects für die Spielräume, nutzbar im kostenlosen Cloudflare-Tarif.

| Endpunkt | Zweck |
|---|---|
| `GET /scores?scope=all&limit=100` | Top 100 aller Zeiten |
| `GET /scores?scope=day&limit=100` | Top 100 von heute (Zeitzone Europe/Vienna) |
| `POST /scores` | Eintrag `{name, score, level, won, deaths, ver, hc}` speichern, liefert `{id, hc, rank, total, dayRank, dayTotal}` |
| `GET /rooms?v=3.19.0` | Öffentliche Online-Spiele in der Lobby, nur gleiche Spielversion |
| `GET /room/new?pub=1&v=…` | WebSocket: neues Online-Spiel hosten (`pub=0` = privat, nur per Code) |
| `GET /room/<CODE>?v=…` | WebSocket: Online-Spiel beitreten |
| `GET /market` | Live-Kurse für die Browser-Version (Yahoo Finance, 2 Minuten zwischengespeichert) |

**Online-Multiplayer:** Jeder Raum ist ein Durable Object und leitet die Nachrichten weiter wie der eingebaute Server der App im LAN: Der erste Spieler ist der Host, seine Nachrichten gehen an alle (oder an `to`), die der Mitspieler mit `from` an den Host. Der Raum speichert keine Spielinhalte; die Tabelle `rooms` enthält nur die offenen Spiele für die Liste. Im kostenlosen Tarif zählen 20 WebSocket-Nachrichten als eine von 100.000 Anfragen pro Tag.

**Datenbank für den Online-Multiplayer nachrüsten** (einmalig, für die Live-Datenbank bereits erledigt):
```
npx wrangler d1 execute margincall-highscores --remote --file=migrate-rooms.sql
```

Mit `hc=1` (GET) bzw. `"hc": true` (POST) geht es um die getrennte Hardcore-Rangliste.

**Bestehende Datenbank von vor Version 3.11 nachrüsten** (einmalig, für die Live-Datenbank bereits erledigt):
```
npx wrangler d1 execute margincall-highscores --remote --file=migrate-hc.sql
```

Gespeichert werden Name, Punkte, Level, Sieg, Tode, Spielversion und Zeitpunkt, aber keine IP-Adressen. Gegen Spam gilt pro IP höchstens ein Eintrag alle 15 Sekunden; das wird nur im Arbeitsspeicher gezählt.

## Einrichten (einmalig, ca. 10 Minuten)

Voraussetzungen: ein kostenloses Konto auf [dash.cloudflare.com](https://dash.cloudflare.com/sign-up) und Node.js 20 oder neuer.

1. Wechsle im Terminal in diesen Ordner:
   ```
   cd worker
   ```
2. Melde dich bei Cloudflare an. Der Browser öffnet sich zur Bestätigung:
   ```
   npx wrangler login
   ```
3. Lege die Datenbank an:
   ```
   npx wrangler d1 create margincall-highscores
   ```
   Die Ausgabe enthält eine `database_id`. Trage sie in `wrangler.toml` statt `00000000-…` ein.
4. Lege die Tabelle an:
   ```
   npx wrangler d1 execute margincall-highscores --remote --file=schema.sql
   ```
5. Veröffentliche den Worker:
   ```
   npx wrangler deploy
   ```
   Am Ende steht die Adresse, z. B. `https://margincall-highscores.<dein-name>.workers.dev`.
6. Teste die Adresse im Browser. Sie sollte `{"name":"MARGIN CALL Online-Rangliste",…}` zeigen.
7. Trage die Adresse ohne `/` am Ende in `pc/game.html` bei `const ONLINE_HS_URL = "";` ein und veröffentliche eine neue Spielversion. Erst dann erscheinen im Spiel die Reiter „Online gesamt“ und „Online heute“ sowie die Option „Online-Rangliste“.

## Pflege

- **Einträge ansehen:**
  ```
  npx wrangler d1 execute margincall-highscores --remote --command "SELECT * FROM scores ORDER BY score DESC LIMIT 20"
  ```
- **Einen Eintrag löschen**, z. B. bei einem unpassenden Namen:
  ```
  npx wrangler d1 execute margincall-highscores --remote --command "DELETE FROM scores WHERE id = 123"
  ```
  Alternativ geht das im Cloudflare-Dashboard unter *Storage & Databases → D1 → margincall-highscores → Console*.
- **Code geändert?** Mit `npx wrangler deploy` erneut veröffentlichen.

Hinweis: Das Spiel rechnet auf dem Rechner der Spieler, daher lassen sich Punktzahlen grundsätzlich fälschen. Der Worker prüft nur Plausibilität (Wertebereiche, „Sieg“ nur mit Level 5). Auffällige Einträge lassen sich wie oben löschen.
