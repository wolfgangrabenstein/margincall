-- MARGIN CALL Online-Rangliste
CREATE TABLE IF NOT EXISTS scores (
  id     INTEGER PRIMARY KEY AUTOINCREMENT,
  name   TEXT    NOT NULL,
  score  INTEGER NOT NULL,
  level  INTEGER NOT NULL,
  won    INTEGER NOT NULL DEFAULT 0,
  deaths INTEGER NOT NULL DEFAULT 0,
  ver    TEXT    NOT NULL DEFAULT '',
  day    TEXT    NOT NULL,          -- YYYY-MM-DD (Europe/Vienna)
  ts     INTEGER NOT NULL,          -- Unix-Zeit in Sekunden
  hc     INTEGER NOT NULL DEFAULT 0 -- 0 = Normal, 1 = Hardcore, 2 = Einfach (je eigene Rangliste)
);
CREATE INDEX IF NOT EXISTS scores_hc_score ON scores (hc, score DESC);
CREATE INDEX IF NOT EXISTS scores_hc_day   ON scores (hc, day, score DESC);

-- Online-Multiplayer: offene Spielraeume (oeffentlich in der Liste, private nur ueber den Code)
CREATE TABLE IF NOT EXISTS rooms (
  code    TEXT    PRIMARY KEY,       -- 5 Zeichen, z. B. K7QXM
  name    TEXT    NOT NULL DEFAULT '',
  host    TEXT    NOT NULL DEFAULT '',
  mode    INTEGER NOT NULL DEFAULT 0,  -- 0 Koop, 1 Team, 2 Deathmatch
  players INTEGER NOT NULL DEFAULT 1,
  pub     INTEGER NOT NULL DEFAULT 1,  -- 1 = in der oeffentlichen Liste
  ver     TEXT    NOT NULL DEFAULT '',
  updated INTEGER NOT NULL           -- Unix-Zeit in Sekunden, letztes Lebenszeichen aus der Lobby
);
CREATE INDEX IF NOT EXISTS rooms_list ON rooms (pub, ver, updated);
