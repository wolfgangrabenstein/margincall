-- Einmalig fuer Datenbanken von vor Version 3.11: Hardcore-Spalte nachruesten
ALTER TABLE scores ADD COLUMN hc INTEGER NOT NULL DEFAULT 0;
DROP INDEX IF EXISTS scores_score;
DROP INDEX IF EXISTS scores_day;
CREATE INDEX IF NOT EXISTS scores_hc_score ON scores (hc, score DESC);
CREATE INDEX IF NOT EXISTS scores_hc_day   ON scores (hc, day, score DESC);
