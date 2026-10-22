// MARGIN CALL – Online-Rangliste, Online-Multiplayer und Kurse fuer die Browser-Version (Cloudflare Worker + D1)
//
//   GET  /scores?scope=all|day&hc=0|1|2&limit=100   Top-Liste (Gesamt bzw. heute; Normal, Hardcore bzw. Einfach)
//   POST /scores  {name, score, level, won, deaths, ver, hc, mode}   (mode: 0 Normal, 1 Hardcore, 2 Einfach)
//        -> {id, rank, total, dayRank, dayTotal}
//
// Normal, Hardcore und Einfach sind getrennte Ranglisten (Spalte hc: 0, 1, 2).
// Gespeichert werden nur Name, Punkte, Level, Sieg, Tode, Modus, Spielversion und Zeitpunkt –
// keine IP-Adressen. Der Tag richtet sich nach der Zeitzone Europe/Vienna.
//
//   GET  /rooms?v=<Version>        oeffentliche Online-Spiele in der Lobby (gleiche Spielversion)
//   GET  /room/new?pub=1&v=…       (WebSocket) neues Online-Spiel hosten, oeffentlich (pub=1) oder privat; der Host kann das in der Lobby umschalten
//   GET  /room/<CODE>?v=…          (WebSocket) Online-Spiel beitreten
//   GET  /market                   Live-Kurse fuer die Browser-Version (Yahoo Finance, 2 Minuten zwischengespeichert)

import { Room, ROOM_TTL_S } from "./rooms.js";
export { Room };

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

// ---- Online-Multiplayer
const CODE_CHARS = "ABCDEFGJLMNQRSTUVWXYZ23456789";   // ohne H, K, P (Tastenkuerzel im Spiel), O/0 und I/1
const cleanVer = v => String(v || "").replace(/[^0-9.]/g, "").slice(0, 12);
async function rooms(env, ver) {
  const { results } = await env.DB.prepare(
    "SELECT code, name, host, mode, players FROM rooms WHERE pub = 1 AND ver = ?1 AND updated > ?2 AND players < 4 AND name <> '' ORDER BY updated DESC LIMIT 20")
    .bind(ver, Math.floor(Date.now() / 1000) - ROOM_TTL_S).all();
  return results;
}
async function openRoom(request, env, url) {
  if (request.headers.get("Upgrade") !== "websocket") return json({ error: "WebSocket erwartet" }, 426);
  const ver = cleanVer(url.searchParams.get("v")), pub = url.searchParams.get("pub") === "1";
  let code = "";
  for (let t = 0; t < 8 && !code; t++) {
    const c = Array.from(crypto.getRandomValues(new Uint8Array(5)), b => CODE_CHARS[b % CODE_CHARS.length]).join("");
    if (!(await env.DB.prepare("SELECT code FROM rooms WHERE code = ?1").bind(c).first())) code = c;
  }
  if (!code) return json({ error: "kein Code frei" }, 503);
  // alte Eintraege aufraeumen, dann den Raum anlegen (private Raeume stehen auch drin, damit der Code reserviert ist)
  const now = Math.floor(Date.now() / 1000);
  await env.DB.batch([
    env.DB.prepare("DELETE FROM rooms WHERE updated < ?1").bind(now - 6 * 3600),
    env.DB.prepare("INSERT INTO rooms (code, name, host, mode, players, pub, ver, updated) VALUES (?1, '', '', 0, 1, ?2, ?3, ?4)").bind(code, pub ? 1 : 0, ver, now),
  ]);
  const h = new Headers(request.headers);
  h.set("x-room-code", code); h.set("x-room-create", "1"); h.set("x-room-pub", pub ? "1" : "0");
  return env.ROOM.get(env.ROOM.idFromName(code)).fetch(new Request(request.url, { headers: h }));
}
function joinRoom(request, env, code) {
  if (request.headers.get("Upgrade") !== "websocket") return json({ error: "WebSocket erwartet" }, 426);
  const h = new Headers(request.headers);
  h.set("x-room-code", code); h.delete("x-room-create"); h.delete("x-room-pub");
  return env.ROOM.get(env.ROOM.idFromName(code)).fetch(new Request(request.url, { headers: h }));
}

// ---- Live-Kurse fuer die Browser-Version (gleiches Format wie /api/market der App)
// 5 Tage abfragen und die letzten Punkte behalten (Indizes ~1 Handelstag in 5 Minuten, Oel 24 h in 15 Minuten):
// so zeigen die Wand-Charts auch kurz nach Boersenstart einen vollen Verlauf statt nur weniger Punkte
const SYMBOLS = [["%5EDJI", "5m", 78], ["%5EGSPC", "5m", 78], ["%5ENDX", "5m", 78], ["CL%3DF", "15m", 96]];
const STOCKS = "GS,CAT,HD,JPM,BA,KO,AAPL,AMZN,GOOGL,META,LLY,BRK-B,NVDA,MSFT,AVGO,TSLA,NFLX,AMD,XOM,CVX";
let marketCache = { t: 0, body: null };
const yahoo = u => fetch(u, { headers: { "User-Agent": "Mozilla/5.0 (MarginCall)" } }).then(r => { if (!r.ok) throw new Error(String(r.status)); return r.json(); });
async function market() {
  if (marketCache.body && Date.now() - marketCache.t < 120_000) return marketCache.body;
  const markets = await Promise.all(SYMBOLS.map(async ([sym, iv, keep]) => {
    try {
      const d = (await yahoo("https://query1.finance.yahoo.com/v8/finance/chart/" + sym + "?range=5d&interval=" + iv)).chart.result[0];
      const s = d.indicators.quote[0].close.filter(x => x !== null).map(x => Math.round(x * 100) / 100).slice(-keep);
      return { p: d.meta.regularMarketPrice, prev: d.meta.previousClose ?? d.meta.chartPreviousClose, s };
    } catch { return null; }
  }));
  const stocks = {};
  try {
    const d = await yahoo("https://query1.finance.yahoo.com/v8/finance/spark?symbols=" + STOCKS + "&range=1d&interval=1d");
    for (const [sym, x] of Object.entries(d)) {
      const c = (x.close || []).filter(v => v !== null);
      if (c.length && x.chartPreviousClose) stocks[sym] = { p: c[c.length - 1], prev: x.chartPreviousClose };
    }
  } catch { }
  const body = { time: Math.floor(Date.now() / 1000), markets, stocks };
  if (markets.some(Boolean)) marketCache = { t: Date.now(), body };
  return body;
}

// ---- Rangliste
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
  const ver = cleanVer(e.ver);
  const hc = e.mode === 2 ? 2 : e.hc === true || e.mode === 1 ? 1 : 0;

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
  return json({ id: row.id, hc: hc === 1, mode: hc, rank: all.results[0].rank, total: all.results[0].total, dayRank: dayR.results[0].rank, dayTotal: dayR.results[0].total });
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (request.method === "OPTIONS") return new Response(null, { status: 204, headers: CORS });
    if (url.pathname === "/scores" && request.method === "GET") {
      const scope = url.searchParams.get("scope") === "day" ? "day" : "all";
      const limit = Math.min(100, Math.max(1, parseInt(url.searchParams.get("limit") || "100", 10) || 100));
      return json(await top(env, scope, { "1": 1, "2": 2 }[url.searchParams.get("hc")] || 0, limit));
    }
    if (url.pathname === "/scores" && request.method === "POST") return add(env, request);
    if (url.pathname === "/rooms") return json(await rooms(env, cleanVer(url.searchParams.get("v"))));
    if (url.pathname === "/room/new") return openRoom(request, env, url);
    const rm = url.pathname.match(/^\/room\/([A-Z0-9]{5})$/);
    if (rm) return joinRoom(request, env, rm[1]);
    if (url.pathname === "/market") return json(await market());
    if (url.pathname === "/") return json({ name: "MARGIN CALL Online-Rangliste", day: today() });
    return json({ error: "nicht gefunden" }, 404);
  },
};
