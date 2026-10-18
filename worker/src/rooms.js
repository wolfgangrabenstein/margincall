// MARGIN CALL – Online-Multiplayer: Spielraeume als Durable Objects
//
// Ein Raum leitet nur Nachrichten weiter, genau wie der eingebaute Server der App im LAN:
// Der erste Spieler ist der Host und rechnet das Spiel; Nachrichten des Hosts gehen an alle (oder an "to"),
// Nachrichten der Mitspieler gehen mit "from" an den Host. Der Raum speichert keine Spielinhalte.
// WebSocket-Hibernation: Ein Raum ohne Nachrichten verbraucht keine Rechenzeit.

import { DurableObject } from "cloudflare:workers";

const MAX_PLAYERS = 4;
const MAX_MSG = 32 * 1024;
const META_EVERY_MS = 10_000;   // Raumliste hoechstens alle 10 s aktualisieren (spart Datenbank-Schreibzugriffe)
export const ROOM_TTL_S = 30;   // Raeume ohne Lebenszeichen aus der Lobby verschwinden nach 30 s aus der Liste

const clean = (v, n) => String(v ?? "").replace(/[\u0000-\u001f<>]/g, "").trim().slice(0, n);

export class Room extends DurableObject {
  constructor(ctx, env) {
    super(ctx, env);
    this.env = env;
    this.lastMeta = 0; this.metaKey = "";
  }

  socks() { return this.ctx.getWebSockets().filter(s => s.readyState === 1 || s.readyState === 0); }
  idOf(s) { return (s.deserializeAttachment() || {}).id || 0; }
  hostId() { const ids = this.socks().map(s => this.idOf(s)).filter(Boolean); return ids.length ? Math.min(...ids) : 0; }
  sendAll(text, except) { for (const s of this.socks()) if (s !== except) try { s.send(text); } catch { } }
  peers() {
    const ids = this.socks().map(s => this.idOf(s)).filter(Boolean).sort((a, b) => a - b);
    this.sendAll(JSON.stringify({ t: "peers", ids, host: ids[0] || 0 }));
    return ids.length;
  }
  refuse(server, msg) {
    this.ctx.acceptWebSocket(server);
    server.serializeAttachment({ id: 0 });
    server.send(JSON.stringify(msg)); server.close(1000, msg.t);
  }

  async fetch(request) {
    if (request.headers.get("Upgrade") !== "websocket") return new Response("WebSocket erwartet", { status: 426 });
    const code = request.headers.get("x-room-code"), create = request.headers.get("x-room-create") === "1";
    const ver = clean(new URL(request.url).searchParams.get("v"), 12);
    const [client, server] = Object.values(new WebSocketPair());
    let info = await this.ctx.storage.get("info");
    const others = this.socks();

    if (create) {
      if (info && info.open && others.length) { this.refuse(server, { t: "retry" }); return new Response(null, { status: 101, webSocket: client }); }
      info = { code, pub: request.headers.get("x-room-pub") === "1", ver, open: true };
      await this.ctx.storage.put("info", info);
    } else if (!info || !info.open || !others.length) { this.refuse(server, { t: "gone" }); return new Response(null, { status: 101, webSocket: client }); }
    else if (info.ver !== ver) { this.refuse(server, { t: "version", v: info.ver }); return new Response(null, { status: 101, webSocket: client }); }
    else if (others.length >= MAX_PLAYERS) { this.refuse(server, { t: "full" }); return new Response(null, { status: 101, webSocket: client }); }

    const used = new Set(others.map(s => this.idOf(s)));
    let id = 1; while (used.has(id)) id++;
    this.ctx.acceptWebSocket(server);
    server.serializeAttachment({ id });
    const host = create ? id : this.hostId();
    server.send(JSON.stringify({ t: "welcome", id, host }));
    server.send(JSON.stringify({ t: "room", code: info.code, pub: info.pub }));
    const n = this.peers();
    if (!create) await this.updateRow({ players: n });
    return new Response(null, { status: 101, webSocket: client });
  }

  async webSocketMessage(ws, msg) {
    if (typeof msg !== "string" || msg.length > MAX_MSG) return;
    const id = this.idOf(ws); if (!id) return;
    let m; try { m = JSON.parse(msg); } catch { return; }
    if (!m || typeof m !== "object") return;
    const host = this.hostId();
    if (id === host) {
      const to = m.to ?? "all"; delete m.to;
      if (m.t === "lobby") await this.lobbyMeta(m);
      const out = JSON.stringify(m);
      for (const s of this.socks()) if (s !== ws && (to === "all" || this.idOf(s) === to)) try { s.send(out); } catch { }
    } else {
      m.from = id;
      const h = this.socks().find(s => this.idOf(s) === host);
      if (h) try { h.send(JSON.stringify(m)); } catch { }
    }
  }

  async webSocketClose(ws) { await this.left(ws); try { ws.close(); } catch { } }
  async webSocketError(ws) { await this.left(ws); }

  async left(ws) {
    const id = this.idOf(ws); if (!id) return;
    ws.serializeAttachment({ id: 0 });
    const info = await this.ctx.storage.get("info");
    const rest = this.socks().filter(s => s !== ws && this.idOf(s));
    const wasHost = !rest.length || id < Math.min(...rest.map(s => this.idOf(s)));
    if (wasHost) {   // Host weg: Raum schliessen, Mitspieler erfahren es ueber die neue Host-Nummer
      if (info) { info.open = false; await this.ctx.storage.put("info", info); }
      const ids = rest.map(s => this.idOf(s)).sort((a, b) => a - b);
      for (const s of rest) try { s.send(JSON.stringify({ t: "peers", ids, host: ids[0] || 0 })); } catch { }
      if (info) await this.env.DB.prepare("DELETE FROM rooms WHERE code = ?1").bind(info.code).run();
    } else {
      this.peers();
      await this.updateRow({ players: rest.length });
    }
  }

  // Lobby-Nachricht des Hosts: Name, Modus und Spielerzahl fuer die oeffentliche Raumliste
  async lobbyMeta(m) {
    const players = Array.isArray(m.players) ? m.players : [];
    const hostRow = players.find(p => Array.isArray(p) && p[0] === this.hostId()) || players[0] || [];
    const meta = { name: clean(m.game, 24), host: clean(hostRow[1], 15), mode: Number.isInteger(m.mode) ? m.mode : 0, players: Math.min(MAX_PLAYERS, players.length || 1) };
    const key = JSON.stringify(meta), now = Date.now();
    if (key === this.metaKey && now - this.lastMeta < META_EVERY_MS) return;
    this.metaKey = key; this.lastMeta = now;
    await this.updateRow(meta);
  }
  async updateRow(f) {
    const info = await this.ctx.storage.get("info"); if (!info || !info.open) return;
    const now = Math.floor(Date.now() / 1000);
    if (f.name !== undefined)
      await this.env.DB.prepare("UPDATE rooms SET name = ?2, host = ?3, mode = ?4, players = ?5, updated = ?6 WHERE code = ?1")
        .bind(info.code, f.name, f.host, f.mode, f.players, now).run();
    else await this.env.DB.prepare("UPDATE rooms SET players = ?2 WHERE code = ?1").bind(info.code, f.players).run();
  }
}
