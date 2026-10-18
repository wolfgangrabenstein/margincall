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
