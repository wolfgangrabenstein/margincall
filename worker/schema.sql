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
  ts     INTEGER NOT NULL           -- Unix-Zeit in Sekunden
);
CREATE INDEX IF NOT EXISTS scores_score ON scores (score DESC);
CREATE INDEX IF NOT EXISTS scores_day   ON scores (day, score DESC);
