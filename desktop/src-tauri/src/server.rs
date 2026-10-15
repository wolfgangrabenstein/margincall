//! Eingebauter Spiel-Server: liefert das Spiel aus, holt die Live-Kurse und
//! leitet die Multiplayer-Nachrichten weiter (WebSocket-Relay).
//! Gleiches Verhalten wie pc/server.py, nur ohne Python.

use base64::Engine;
use serde_json::{json, Value};
use sha1::{Digest, Sha1};
use std::collections::BTreeMap;
use std::io::{self, BufRead, BufReader, Read, Write};
use std::net::{Shutdown, TcpListener, TcpStream, UdpSocket};
use std::sync::{Arc, Mutex, OnceLock};
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

const GAME_HTML: &str = include_str!("../../../pc/game.html");
const WS_GUID: &str = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const MAX_PLAYERS: u32 = 4;
const CACHE_SECONDS: u64 = 60;
const SYMBOLS: [(&str, &str); 4] = [("%5EDJI", "5m"), ("%5EGSPC", "5m"), ("%5ENDX", "5m"), ("CL%3DF", "15m")];

static PORT: OnceLock<u16> = OnceLock::new();
static CACHE: OnceLock<Mutex<Option<(Instant, String)>>> = OnceLock::new();
static RELAY: OnceLock<Mutex<BTreeMap<u32, Arc<Client>>>> = OnceLock::new();

/// Startet den Server auf dem ersten freien Port ab 8080 und liefert den Port.
pub fn start() -> u16 {
    let (listener, port) = (8080..8100)
        .find_map(|p| TcpListener::bind(("0.0.0.0", p)).ok().map(|l| (l, p)))
        .expect("kein freier Port zwischen 8080 und 8099");
    let _ = PORT.set(port);
    std::thread::spawn(move || {
        for stream in listener.incoming().flatten() {
            std::thread::spawn(move || {
                let _ = handle(stream);
            });
        }
    });
    // Kurse werden erst geladen, wenn das Spiel sie anfragt (nur mit Zustimmung des Spielers).
    port
}

// ------------------------------------------------------------------ HTTP
fn handle(stream: TcpStream) -> io::Result<()> {
    stream.set_nodelay(true).ok();
    let mut reader = BufReader::new(stream.try_clone()?);
    let mut line = String::new();
    reader.read_line(&mut line)?;
    let method = line.split_whitespace().next().unwrap_or("GET").to_string();
    let path = line.split_whitespace().nth(1).unwrap_or("/").split('?').next().unwrap_or("/").to_string();
    let mut headers = BTreeMap::new();
    loop {
        let mut h = String::new();
        if reader.read_line(&mut h)? == 0 {
            break;
        }
        let h = h.trim_end();
        if h.is_empty() {
            break;
        }
        if let Some((k, v)) = h.split_once(':') {
            headers.insert(k.trim().to_ascii_lowercase(), v.trim().to_string());
        }
    }
    let mut out = stream;
    let is_ws = headers.get("upgrade").map(|v| v.eq_ignore_ascii_case("websocket")).unwrap_or(false);
    match path.as_str() {
        "/" | "/index.html" => respond(&mut out, 200, "text/html; charset=utf-8", GAME_HTML.as_bytes()),
        "/api/ping" => respond(&mut out, 200, "text/plain", b"ok"),
        "/api/info" => {
            let body = json!({ "ip": lan_ip(), "port": PORT.get() }).to_string();
            respond(&mut out, 200, "application/json", body.as_bytes())
        }
        "/api/market" => respond(&mut out, 200, "application/json", markets_json().as_bytes()),
        "/api/highscores" if method == "POST" => {
            // Nur vom eigenen Rechner: Mitspieler im WLAN duerfen die Liste nicht veraendern
            let local = out.peer_addr().map(|a| a.ip().is_loopback()).unwrap_or(false);
            let len: usize = headers.get("content-length").and_then(|v| v.parse().ok()).unwrap_or(0);
            if !local || len > 4096 {
                return respond(&mut out, 403, "text/plain", b"forbidden");
            }
            let mut body = vec![0u8; len];
            reader.read_exact(&mut body)?;
            let list = highscore_add(&body);
            respond(&mut out, 200, "application/json", list.as_bytes())
        }
        "/api/highscores" => respond(&mut out, 200, "application/json", highscores_load().to_string().as_bytes()),
        "/ws" if is_ws => websocket(out, reader, &headers),
        _ => respond(&mut out, 404, "text/plain", b"not found"),
    }
}

// ------------------------------------------------------------------ Highscores (Einzelspieler)
// Datei im Benutzerordner, damit die Liste App-Updates und Portwechsel ueberlebt.
const HIGHSCORE_MAX: usize = 10;
static HS_LOCK: Mutex<()> = Mutex::new(());

fn highscore_file() -> std::path::PathBuf {
    let base = if cfg!(windows) {
        std::env::var("APPDATA").ok()
    } else if cfg!(target_os = "macos") {
        std::env::var("HOME").ok().map(|h| format!("{h}/Library/Application Support"))
    } else {
        std::env::var("XDG_DATA_HOME").ok().or_else(|| std::env::var("HOME").ok().map(|h| format!("{h}/.local/share")))
    };
    std::path::PathBuf::from(base.unwrap_or_else(|| ".".into())).join("MarginCall").join("highscores.json")
}

fn highscores_load() -> Value {
    std::fs::read_to_string(highscore_file())
        .ok()
        .and_then(|s| serde_json::from_str::<Value>(&s).ok())
        .filter(|v| v.is_array())
        .unwrap_or_else(|| json!([]))
}

/// Prueft den neuen Eintrag, sortiert ihn ein, behaelt die besten 10 und speichert.
fn highscore_add(body: &[u8]) -> String {
    let _guard = HS_LOCK.lock().unwrap();
    let mut list: Vec<Value> = highscores_load().as_array().cloned().unwrap_or_default();
    if let Ok(e) = serde_json::from_slice::<Value>(body) {
        let name: String = e["name"]
            .as_str()
            .unwrap_or("")
            .chars()
            .filter(|c| c.is_alphanumeric() || " -_.".contains(*c))
            .take(12)
            .collect::<String>()
            .trim()
            .to_string();
        let score = e["score"].as_i64().unwrap_or(-1);
        let level = e["level"].as_i64().unwrap_or(1).clamp(1, 5);
        if !name.is_empty() && (0..=100_000_000).contains(&score) {
            let date = SystemTime::now().duration_since(UNIX_EPOCH).map(|d| d.as_secs()).unwrap_or(0);
            list.push(json!({ "name": name, "score": score, "level": level, "won": e["won"].as_bool().unwrap_or(false), "date": date }));
            list.sort_by(|a, b| b["score"].as_i64().cmp(&a["score"].as_i64()));
            list.truncate(HIGHSCORE_MAX);
            let file = highscore_file();
            if let Some(dir) = file.parent() {
                let _ = std::fs::create_dir_all(dir);
            }
            let _ = std::fs::write(&file, Value::from(list.clone()).to_string());
        }
    }
    Value::from(list).to_string()
}

fn respond(s: &mut TcpStream, code: u16, ctype: &str, body: &[u8]) -> io::Result<()> {
    let reason = match code {
        200 => "OK",
        403 => "Forbidden",
        _ => "Not Found",
    };
    let head = format!(
        "HTTP/1.1 {code} {reason}\r\nContent-Type: {ctype}\r\nContent-Length: {}\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n",
        body.len()
    );
    s.write_all(head.as_bytes())?;
    s.write_all(body)?;
    s.flush()
}

fn lan_ip() -> String {
    UdpSocket::bind("0.0.0.0:0")
        .and_then(|s| {
            s.connect("8.8.8.8:80")?;
            s.local_addr()
        })
        .map(|a| a.ip().to_string())
        .unwrap_or_else(|_| "127.0.0.1".into())
}

// ------------------------------------------------------------------ Kurse
fn fetch_one(symbol: &str, interval: &str) -> Option<Value> {
    let url = format!("https://query1.finance.yahoo.com/v8/finance/chart/{symbol}?range=1d&interval={interval}");
    let v: Value = ureq::get(&url)
        .set("User-Agent", "Mozilla/5.0 (MarginCall Desktop)")
        .timeout(Duration::from_secs(10))
        .call()
        .ok()?
        .into_json()
        .ok()?;
    let r = &v["chart"]["result"][0];
    let round2 = |x: f64| (x * 100.0).round() / 100.0;
    let mut closes: Vec<f64> = r["indicators"]["quote"][0]["close"].as_array()?.iter().filter_map(|x| x.as_f64()).map(round2).collect();
    while closes.len() > 96 {
        closes = closes.chunks(2).filter(|c| c.len() == 2).map(|c| round2((c[0] + c[1]) / 2.0)).collect();
    }
    Some(json!({
        "p": r["meta"]["regularMarketPrice"].as_f64()?,
        "prev": r["meta"]["chartPreviousClose"].as_f64()?,
        "s": closes,
    }))
}

fn markets_json() -> String {
    let cache = CACHE.get_or_init(|| Mutex::new(None));
    if let Some((t, s)) = &*cache.lock().unwrap() {
        if t.elapsed() < Duration::from_secs(CACHE_SECONDS) {
            return s.clone();
        }
    }
    let list: Vec<Value> = SYMBOLS.iter().map(|(sym, iv)| fetch_one(sym, iv).unwrap_or(Value::Null)).collect();
    let any = list.iter().any(|v| !v.is_null());
    let now = SystemTime::now().duration_since(UNIX_EPOCH).map(|d| d.as_secs()).unwrap_or(0);
    let out = json!({ "time": now, "markets": list }).to_string();
    if any {
        *cache.lock().unwrap() = Some((Instant::now(), out.clone()));
    }
    out
}

// ------------------------------------------------------------------ Multiplayer-Relay
struct Client {
    stream: Mutex<TcpStream>,
}

fn relay() -> &'static Mutex<BTreeMap<u32, Arc<Client>>> {
    RELAY.get_or_init(|| Mutex::new(BTreeMap::new()))
}

fn ws_frame(op: u8, data: &[u8]) -> Vec<u8> {
    let n = data.len();
    let mut f = Vec::with_capacity(n + 10);
    f.push(0x80 | op);
    if n < 126 {
        f.push(n as u8);
    } else if n < 65536 {
        f.push(126);
        f.extend_from_slice(&(n as u16).to_be_bytes());
    } else {
        f.push(127);
        f.extend_from_slice(&(n as u64).to_be_bytes());
    }
    f.extend_from_slice(data);
    f
}

fn ws_send(c: &Client, text: &str) {
    let frame = ws_frame(0x1, text.as_bytes());
    if let Ok(mut s) = c.stream.lock() {
        let _ = s.write_all(&frame);
        let _ = s.flush();
    }
}

/// Liest den naechsten Text-Frame; None bei Verbindungsende.
fn ws_recv<R: Read>(r: &mut R, c: &Client) -> io::Result<Option<String>> {
    let mut msg = Vec::new();
    loop {
        let mut h = [0u8; 2];
        r.read_exact(&mut h)?;
        let fin = h[0] & 0x80 != 0;
        let op = h[0] & 0x0F;
        let masked = h[1] & 0x80 != 0;
        let mut n = (h[1] & 0x7F) as u64;
        if n == 126 {
            let mut b = [0u8; 2];
            r.read_exact(&mut b)?;
            n = u16::from_be_bytes(b) as u64;
        } else if n == 127 {
            let mut b = [0u8; 8];
            r.read_exact(&mut b)?;
            n = u64::from_be_bytes(b);
        }
        if n > 16 * 1024 * 1024 {
            return Ok(None);
        }
        let mut mask = [0u8; 4];
        if masked {
            r.read_exact(&mut mask)?;
        }
        let mut payload = vec![0u8; n as usize];
        r.read_exact(&mut payload)?;
        if masked {
            for (i, b) in payload.iter_mut().enumerate() {
                *b ^= mask[i & 3];
            }
        }
        match op {
            0x8 => return Ok(None),
            0x9 => {
                if let Ok(mut s) = c.stream.lock() {
                    let _ = s.write_all(&ws_frame(0xA, &payload));
                }
            }
            0x0 | 0x1 | 0x2 => {
                msg.extend_from_slice(&payload);
                if fin {
                    return Ok(Some(String::from_utf8_lossy(&msg).into_owned()));
                }
            }
            _ => {}
        }
    }
}

fn broadcast_peers() {
    let (msg, targets) = {
        let m = relay().lock().unwrap();
        let ids: Vec<u32> = m.keys().copied().collect();
        let host = ids.first().copied().unwrap_or(0);
        (json!({ "t": "peers", "ids": ids, "host": host }).to_string(), m.values().cloned().collect::<Vec<_>>())
    };
    for c in targets {
        ws_send(&c, &msg);
    }
}

fn route(cid: u32, text: &str) {
    let Ok(mut msg) = serde_json::from_str::<Value>(text) else { return };
    let clients = relay().lock().unwrap().clone();
    let host = clients.keys().next().copied().unwrap_or(0);
    if cid == host {
        let to = msg.as_object_mut().and_then(|o| o.remove("to")).unwrap_or_else(|| Value::from("all"));
        let out = msg.to_string();
        if to == "all" {
            for (id, c) in &clients {
                if *id != cid {
                    ws_send(c, &out);
                }
            }
        } else if let Some(id) = to.as_u64() {
            if let Some(c) = clients.get(&(id as u32)) {
                ws_send(c, &out);
            }
        }
    } else if let Some(h) = clients.get(&host) {
        if let Some(o) = msg.as_object_mut() {
            o.insert("from".into(), Value::from(cid));
        }
        ws_send(h, &msg.to_string());
    }
}

fn websocket(mut stream: TcpStream, mut reader: BufReader<TcpStream>, headers: &BTreeMap<String, String>) -> io::Result<()> {
    let key = headers.get("sec-websocket-key").cloned().unwrap_or_default();
    let mut sha = Sha1::new();
    sha.update(key.as_bytes());
    sha.update(WS_GUID.as_bytes());
    let accept = base64::engine::general_purpose::STANDARD.encode(sha.finalize());
    let head = format!("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: {accept}\r\n\r\n");
    stream.write_all(head.as_bytes())?;
    stream.flush()?;

    let client = Arc::new(Client { stream: Mutex::new(stream) });
    let cid = {
        let mut m = relay().lock().unwrap();
        match (1..=MAX_PLAYERS).find(|i| !m.contains_key(i)) {
            Some(i) => {
                m.insert(i, client.clone());
                i
            }
            None => 0,
        }
    };
    if cid == 0 {
        ws_send(&client, r#"{"t":"full"}"#);
        return Ok(());
    }
    let host = relay().lock().unwrap().keys().next().copied().unwrap_or(cid);
    ws_send(&client, &json!({ "t": "welcome", "id": cid, "host": host }).to_string());
    broadcast_peers();

    while let Ok(Some(text)) = ws_recv(&mut reader, &client) {
        route(cid, &text);
    }

    relay().lock().unwrap().remove(&cid);
    broadcast_peers();
    if let Ok(s) = client.stream.lock() {
        let _ = s.shutdown(Shutdown::Both);
    }
    Ok(())
}
