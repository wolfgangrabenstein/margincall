// MARGIN CALL – Online-Rangliste (Cloudflare Worker + D1)
//
//   GET  /scores?scope=all|day&hc=0|1&limit=100   Top-Liste (Gesamt bzw. heute; normal bzw. Hardcore)
//   POST /scores  {name, score, level, won, deaths, ver, hc}
//        -> {id, rank, total, dayRank, dayTotal}
//
// Normal und Hardcore sind getrennte Ranglisten (Spalte hc).
// Gespeichert werden nur Name, Punkte, Level, Sieg, Tode, Modus, Spielversion und Zeitpunkt –
// keine IP-Adressen. Der Tag richtet sich nach der Zeitzone Europe/Vienna.

const MAX_SCORE = 50_000_000;
const MIN_GAP_MS = 15_000;          // pro IP hoechstens ein Eintrag alle 15 s (nur im Speicher)
const recent = new Map();

const CORS = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
  "Access-Control-Allow-Headers": "Content-Type",
  "Access-Control-Max-Age": "86400",
};
const json = (data, status = 200) =>
  new Response(JSON.stringify(data), { status, headers: { "Content-Type": "application/json; charset=utf-8", ...CORS } });

const today = () => new Intl.DateTimeFormat("sv-SE", { timeZone: "Europe/Vienna" }).format(new Date());   // YYYY-MM-DD
const cleanName = v => String(v || "").replace(/[^\p{L}\p{N} ._\-]/gu, "").replace(/\s+/g, " ").trim().slice(0, 15).toUpperCase();
const int = (v, lo, hi) => Number.isInteger(v) && v >= lo && v <= hi ? v : null;

async function top(env, scope, hc, limit) {
  const where = scope === "day" ? "WHERE hc = ?1 AND day = ?2" : "WHERE hc = ?1";
  const stmt = env.DB.prepare(
    `SELECT id, name, score, level, won, deaths, ts FROM scores ${where} ORDER BY score DESC, ts ASC LIMIT ${limit}`);
  const { results } = await (scope === "day" ? stmt.bind(hc, today()) : stmt.bind(hc)).all();
  return results.map(r => ({ ...r, won: !!r.won }));
}

async function add(env, request) {
  const ip = request.headers.get("CF-Connecting-IP") || "";
  const now = Date.now();
  if (ip && now - (recent.get(ip) || 0) < MIN_GAP_MS) return json({ error: "zu schnell" }, 429);

  let e;
  try { e = await request.json(); } catch { return json({ error: "ungueltig" }, 400); }
  const name = cleanName(e.name);
  const score = int(e.score, 1, MAX_SCORE), level = int(e.level, 1, 5), deaths = int(e.deaths ?? 0, 0, 999);
  if (!name || score === null || level === null || deaths === null) return json({ error: "ungueltig" }, 400);
  // Plausibilitaet: ein Sieg setzt Level 5 voraus
  const won = e.won === true && level === 5 ? 1 : 0;
  const ver = String(e.ver || "").replace(/[^0-9.]/g, "").slice(0, 12);
  const hc = e.hc === true ? 1 : 0;

  if (ip) { recent.set(ip, now); if (recent.size > 5000) recent.clear(); }
  const day = today();
  const row = await env.DB.prepare(
    "INSERT INTO scores (name, score, level, won, deaths, ver, day, ts, hc) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9) RETURNING id")
    .bind(name, score, level, won, deaths, ver, day, Math.floor(now / 1000), hc).first();

  // Platz = Anzahl besserer Eintraege im selben Modus + 1 (bei Gleichstand zaehlt der fruehere)
  const [all, dayR] = await env.DB.batch([
    env.DB.prepare("SELECT (SELECT COUNT(*) FROM scores WHERE hc = ?2 AND score > ?1) + 1 AS rank, (SELECT COUNT(*) FROM scores WHERE hc = ?2) AS total").bind(score, hc),
    env.DB.prepare("SELECT (SELECT COUNT(*) FROM scores WHERE hc = ?3 AND day = ?2 AND score > ?1) + 1 AS rank, (SELECT COUNT(*) FROM scores WHERE hc = ?3 AND day = ?2) AS total").bind(score, day, hc),
  ]);
  return json({ id: row.id, hc: !!hc, rank: all.results[0].rank, total: all.results[0].total, dayRank: dayR.results[0].rank, dayTotal: dayR.results[0].total });
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (request.method === "OPTIONS") return new Response(null, { status: 204, headers: CORS });
    if (url.pathname === "/scores" && request.method === "GET") {
      const scope = url.searchParams.get("scope") === "day" ? "day" : "all";
      const limit = Math.min(100, Math.max(1, parseInt(url.searchParams.get("limit") || "100", 10) || 100));
      return json(await top(env, scope, url.searchParams.get("hc") === "1" ? 1 : 0, limit));
    }
    if (url.pathname === "/scores" && request.method === "POST") return add(env, request);
    if (url.pathname === "/") return json({ name: "MARGIN CALL Online-Rangliste", day: today() });
    return json({ error: "nicht gefunden" }, 404);
  },
};
