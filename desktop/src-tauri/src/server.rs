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
// Grenzen gegen unbegrenzten Speicherverbrauch durch fehlerhafte oder boeswillige Clients im LAN
const HTTP_MAX_LINE: u64 = 8 * 1024;      // laengste Anfrage- bzw. Header-Zeile
const HTTP_MAX_HEADERS: usize = 64;       // hoechstens so viele Header je Anfrage
const HTTP_TIMEOUT: Duration = Duration::from_secs(10);   // Zeit fuer Anfragezeile und Header
const WS_MAX_MSG: usize = 64 * 1024;      // groesste WebSocket-Nachricht, alle Fragmente zusammen (Spiel: unter 16 KiB, Online-Server: 32 KiB)
const MAX_GAMES: usize = 64;              // hoechstens so viele gefundene LAN-Spiele merken
const WS_WRITE_TIMEOUT: Duration = Duration::from_secs(5);   // so lange darf das Senden an einen Mitspieler hoechstens haengen
const CACHE_SECONDS: u64 = 60;
const SYMBOLS: [(&str, &str); 4] = [("%5EDJI", "5m"), ("%5EGSPC", "5m"), ("%5ENDX", "5m"), ("CL%3DF", "15m")];
// Einzelwerte fuer den Ticker (nur Anzeige); gleiche Liste wie STOCKS in pc/game.html
const STOCKS: &str = "GS,CAT,HD,JPM,BA,KO,AAPL,AMZN,GOOGL,META,LLY,BRK-B,NVDA,MSFT,AVGO,TSLA,NFLX,AMD,XOM,CVX";

static PORT: OnceLock<u16> = OnceLock::new();
static CACHE: OnceLock<Mutex<Option<(Instant, String)>>> = OnceLock::new();
static RELAY: OnceLock<Mutex<BTreeMap<u32, Arc<Client>>>> = OnceLock::new();

/// Startet den Server auf dem ersten freien Port ab 8080 und liefert den Port.
/// Nach einem Update (Argument --port=N) wird derselbe Port wie vorher genommen, sobald die alte Version ihn freigibt:
/// Einstellungen und Bestenlisten der Spielansicht haengen an der Adresse samt Port.
pub fn start() -> u16 {
    update_cleanup();
    let want: Option<u16> = std::env::args().find_map(|a| a.strip_prefix("--port=").and_then(|p| p.parse().ok()));
    let again = want.and_then(|p| {
        (0..50).find_map(|_| TcpListener::bind(("0.0.0.0", p)).ok().map(|l| (l, p)).or_else(|| {
            std::thread::sleep(Duration::from_millis(100));
            None
        }))
    });
    let (listener, port) = again
        .or_else(|| (8080..8100).find_map(|p| TcpListener::bind(("0.0.0.0", p)).ok().map(|l| (l, p))))
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
    discovery_start(port);
    port
}

// ------------------------------------------------------------------ Spielesuche im LAN (UDP-Broadcast)
// Ein Host in der Lobby kuendigt sein Spiel jede Sekunde per Broadcast an; jede App lauscht
// und zeigt die gefundenen Spiele unter "Spiel beitreten" an.
const DISCOVERY_PORT: u16 = 47800;
static HOSTING: Mutex<Option<(Instant, Value)>> = Mutex::new(None);
static GAMES: OnceLock<Mutex<BTreeMap<String, (Instant, Value)>>> = OnceLock::new();

fn games() -> &'static Mutex<BTreeMap<String, (Instant, Value)>> {
    GAMES.get_or_init(|| Mutex::new(BTreeMap::new()))
}

fn clean(v: &Value, max: usize) -> String {
    v.as_str().unwrap_or("").chars().filter(|c| !c.is_control()).take(max).collect()
}

fn discovery_start(port: u16) {
    // Lauschen (mehrere Apps auf einem Rechner duerfen denselben Port nutzen)
    std::thread::spawn(|| {
        use socket2::{Domain, Protocol, Socket, Type};
        let sock = match Socket::new(Domain::IPV4, Type::DGRAM, Some(Protocol::UDP)) {
            Ok(s) => s,
            Err(_) => return,
        };
        let _ = sock.set_reuse_address(true);
        let _ = sock.set_broadcast(true);
        let addr: std::net::SocketAddr = ([0, 0, 0, 0], DISCOVERY_PORT).into();
        if sock.bind(&addr.into()).is_err() {
            return;
        }
        let udp: UdpSocket = sock.into();
        let mut buf = [0u8; 1024];
        while let Ok((n, from)) = udp.recv_from(&mut buf) {
            let Ok(v) = serde_json::from_slice::<Value>(&buf[..n]) else { continue };
            if v["game"] != "margincall" {
                continue;
            }
            let Some(p) = v["port"].as_u64().filter(|p| (1..=65535).contains(p)) else { continue };
            // Ein Spiel kann ueber mehrere Netzwerkadapter ankommen: per Kennung zusammenfassen,
            // Adresse = die vom Host selbst gemeldete LAN-Adresse
            let reported = clean(&v["ip"], 40);
            let host_ip = if reported.parse::<std::net::Ipv4Addr>().is_ok() { reported } else { from.ip().to_string() };
            let id = clean(&v["id"], 40);
            let key = if id.is_empty() { format!("{host_ip}:{p}") } else { id };
            let entry = json!({
                "addr": format!("{host_ip}:{p}"),
                "name": clean(&v["name"], 24),
                "host": clean(&v["host"], 24),
                "mode": clean(&v["mode"], 32),
                "players": v["players"].as_u64().unwrap_or(1).min(4),
                "version": clean(&v["version"], 12),
            });
            let mut g = games().lock().unwrap();
            if g.len() >= MAX_GAMES && !g.contains_key(&key) {   // Liste klein halten: Abgelaufene entfernen, sonst Neues ignorieren
                g.retain(|_, (t, _)| t.elapsed() < Duration::from_secs(5));
                if g.len() >= MAX_GAMES {
                    continue;
                }
            }
            g.insert(key, (Instant::now(), entry));
        }
    });
    // Ankuendigen, solange das Spiel in der Lobby hostet
    std::thread::spawn(move || {
        let Ok(udp) = UdpSocket::bind("0.0.0.0:0") else { return };
        let _ = udp.set_broadcast(true);
        loop {
            std::thread::sleep(Duration::from_secs(1));
            let msg = {
                let h = HOSTING.lock().unwrap();
                match &*h {
                    Some((t, info)) if t.elapsed() < Duration::from_secs(4) => {
                        let mut m = info.clone();
                        m["game"] = json!("margincall");
                        m["port"] = json!(port);
                        m["version"] = json!(env!("CARGO_PKG_VERSION"));
                        m["id"] = json!(instance_id());
                        m["ip"] = json!(lan_ip());
                        Some(m.to_string())
                    }
                    _ => None,
                }
            };
            let Some(msg) = msg else { continue };
            let _ = udp.send_to(msg.as_bytes(), ("255.255.255.255", DISCOVERY_PORT));
            // zusaetzlich an die Broadcast-Adresse des eigenen /24-Netzes (manche Router verwerfen 255.255.255.255)
            if let Ok(ip) = lan_ip().parse::<std::net::Ipv4Addr>() {
                let o = ip.octets();
                let _ = udp.send_to(msg.as_bytes(), (std::net::Ipv4Addr::new(o[0], o[1], o[2], 255), DISCOVERY_PORT));
            }
        }
    });
}

/// Eindeutige Kennung dieses Spiel-Servers (damit das eigene Spiel nicht in der Liste erscheint)
fn instance_id() -> &'static str {
    static ID: OnceLock<String> = OnceLock::new();
    ID.get_or_init(|| {
        let ns = SystemTime::now().duration_since(UNIX_EPOCH).map(|d| d.as_nanos()).unwrap_or(0);
        format!("{}-{}", std::process::id(), ns)
    })
}

/// Gefundene Spiele der letzten 5 Sekunden, ohne das eigene
fn games_json() -> String {
    let own = instance_id();
    let mut g = games().lock().unwrap();
    g.retain(|_, (t, _)| t.elapsed() < Duration::from_secs(5));
    let list: Vec<Value> = g.iter().filter(|(k, _)| k.as_str() != own).map(|(_, (_, v))| v.clone()).collect();
    Value::from(list).to_string()
}

// ------------------------------------------------------------------ HTTP
/// Liest eine HTTP-Zeile mit Laengengrenze: None bei Verbindungsende, Fehler bei ueberlanger Zeile.
fn http_line<R: BufRead>(reader: &mut R) -> io::Result<Option<String>> {
    let mut s = String::new();
    let n = reader.by_ref().take(HTTP_MAX_LINE).read_line(&mut s)?;
    if n == 0 {
        return Ok(None);
    }
    if !s.ends_with('\n') && n as u64 >= HTTP_MAX_LINE {
        return Err(io::Error::new(io::ErrorKind::InvalidData, "HTTP-Zeile zu lang"));
    }
    Ok(Some(s))
}

/// Anfragezeile und Header einer HTTP-Anfrage; None bei leerer Verbindung, Fehler bei zu vielen Headern.
fn http_head<R: BufRead>(reader: &mut R) -> io::Result<Option<(String, BTreeMap<String, String>)>> {
    let Some(line) = http_line(reader)? else { return Ok(None) };
    let mut headers = BTreeMap::new();
    let mut count = 0;   // Zeilen zaehlen, nicht Eintraege (doppelte Namen und Zeilen ohne : zaehlen mit)
    while let Some(h) = http_line(reader)? {
        let h = h.trim_end();
        if h.is_empty() {
            break;
        }
        count += 1;
        if count > HTTP_MAX_HEADERS {
            return Err(io::Error::new(io::ErrorKind::InvalidData, "zu viele HTTP-Header"));
        }
        if let Some((k, v)) = h.split_once(':') {
            headers.insert(k.trim().to_ascii_lowercase(), v.trim().to_string());
        }
    }
    Ok(Some((line, headers)))
}

fn handle(stream: TcpStream) -> io::Result<()> {
    stream.set_nodelay(true).ok();
    let mut reader = BufReader::new(stream.try_clone()?);
    reader.get_ref().set_read_timeout(Some(HTTP_TIMEOUT)).ok();   // wer Anfrage und Header nicht zuegig schickt, fliegt raus
    let Some((line, headers)) = http_head(&mut reader)? else { return Ok(()) };
    let method = line.split_whitespace().next().unwrap_or("GET").to_string();
    let path = line.split_whitespace().nth(1).unwrap_or("/").split('?').next().unwrap_or("/").to_string();
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
        // Updates: nur vom eigenen Rechner (die Spielansicht der App), nie von Mitspielern im WLAN
        "/api/update" | "/api/update/install" | "/api/update/restart" if !out.peer_addr().map(|a| a.ip().is_loopback()).unwrap_or(false) => {
            respond(&mut out, 403, "text/plain", b"forbidden")
        }
        "/api/update" => respond(&mut out, 200, "application/json", update_status().as_bytes()),
        "/api/update/install" if method == "POST" => {
            update_install();
            respond(&mut out, 200, "application/json", update_status().as_bytes())
        }
        "/api/update/restart" if method == "POST" => {
            let ok = update_restart();
            respond(&mut out, if ok { 200 } else { 403 }, "text/plain", if ok { b"ok" } else { b"not ready" })
        }
        "/api/highscores" | "/api/highscores-hc" | "/api/highscores-easy" if method == "POST" => {
            // Nur vom eigenen Rechner: Mitspieler im WLAN duerfen die Liste nicht veraendern
            let local = out.peer_addr().map(|a| a.ip().is_loopback()).unwrap_or(false);
            let len: usize = headers.get("content-length").and_then(|v| v.parse().ok()).unwrap_or(0);
            if !local || len > 4096 {
                return respond(&mut out, 403, "text/plain", b"forbidden");
            }
            let mut body = vec![0u8; len];
            reader.read_exact(&mut body)?;
            let list = highscore_add(&body, hs_mode(&path));
            respond(&mut out, 200, "application/json", list.as_bytes())
        }
        "/api/highscores" | "/api/highscores-hc" | "/api/highscores-easy" => respond(&mut out, 200, "application/json", highscores_load(hs_mode(&path)).to_string().as_bytes()),
        "/api/games" => respond(&mut out, 200, "application/json", games_json().as_bytes()),
        "/api/host" if method == "POST" => {
            // Lobby-Status des eigenen Spiels (nur vom eigenen Rechner); active=false beendet die Ankuendigung
            let local = out.peer_addr().map(|a| a.ip().is_loopback()).unwrap_or(false);
            let len: usize = headers.get("content-length").and_then(|v| v.parse().ok()).unwrap_or(0);
            if !local || len > 1024 {
                return respond(&mut out, 403, "text/plain", b"forbidden");
            }
            let mut body = vec![0u8; len];
            reader.read_exact(&mut body)?;
            let v: Value = serde_json::from_slice(&body).unwrap_or(Value::Null);
            *HOSTING.lock().unwrap() = if v["active"].as_bool() == Some(true) {
                Some((Instant::now(), json!({ "name": clean(&v["name"], 24), "host": clean(&v["host"], 24), "mode": clean(&v["mode"], 32), "players": v["players"].as_u64().unwrap_or(1).min(4) })))
            } else {
                None
            };
            respond(&mut out, 200, "text/plain", b"ok")
        }
        "/ws" if is_ws => websocket(out, reader, &headers),
        _ => respond(&mut out, 404, "text/plain", b"not found"),
    }
}

// ------------------------------------------------------------------ Highscores (Einzelspieler)
// Datei im Benutzerordner, damit die Liste App-Updates und Portwechsel ueberlebt; Hardcore und Einfach haben eigene Listen.
const HIGHSCORE_MAX: usize = 10;
static HS_LOCK: Mutex<()> = Mutex::new(());

/// 0 = Normal, 1 = Hardcore, 2 = Einfach
fn hs_mode(path: &str) -> u8 {
    match path {
        "/api/highscores-hc" => 1,
        "/api/highscores-easy" => 2,
        _ => 0,
    }
}

fn highscore_file(mode: u8) -> std::path::PathBuf {
    let base = if cfg!(windows) {
        std::env::var("APPDATA").ok()
    } else if cfg!(target_os = "macos") {
        std::env::var("HOME").ok().map(|h| format!("{h}/Library/Application Support"))
    } else {
        std::env::var("XDG_DATA_HOME").ok().or_else(|| std::env::var("HOME").ok().map(|h| format!("{h}/.local/share")))
    };
    std::path::PathBuf::from(base.unwrap_or_else(|| ".".into())).join("MarginCall").join(match mode {
        1 => "highscores-hardcore.json",
        2 => "highscores-easy.json",
        _ => "highscores.json",
    })
}

fn highscores_load(mode: u8) -> Value {
    std::fs::read_to_string(highscore_file(mode))
        .ok()
        .and_then(|s| serde_json::from_str::<Value>(&s).ok())
        .filter(|v| v.is_array())
        .unwrap_or_else(|| json!([]))
}

/// Prueft den neuen Eintrag, sortiert ihn ein, behaelt die besten 10 und speichert.
fn highscore_add(body: &[u8], mode: u8) -> String {
    let _guard = HS_LOCK.lock().unwrap();
    let mut list: Vec<Value> = highscores_load(mode).as_array().cloned().unwrap_or_default();
    if let Ok(e) = serde_json::from_slice::<Value>(body) {
        let name: String = e["name"]
            .as_str()
            .unwrap_or("")
            .chars()
            .filter(|c| c.is_alphanumeric() || " -_.".contains(*c))
            .take(15)
            .collect::<String>()
            .trim()
            .to_string();
        let score = e["score"].as_i64().unwrap_or(-1);
        let level = e["level"].as_i64().unwrap_or(1).clamp(1, 5);
        if !name.is_empty() && (0..=100_000_000).contains(&score) {
            let date = SystemTime::now().duration_since(UNIX_EPOCH).map(|d| d.as_secs()).unwrap_or(0);
            let deaths = e["deaths"].as_i64().unwrap_or(0).clamp(0, 999);
            list.push(json!({ "name": name, "score": score, "level": level, "won": e["won"].as_bool().unwrap_or(false), "deaths": deaths, "date": date }));
            list.sort_by(|a, b| b["score"].as_i64().cmp(&a["score"].as_i64()));
            list.truncate(HIGHSCORE_MAX);
            let file = highscore_file(mode);
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
    // 5 Tage abfragen und die letzten Punkte behalten, damit der Verlauf kurz nach Boersenstart nicht fast leer ist
    let url = format!("https://query1.finance.yahoo.com/v8/finance/chart/{symbol}?range=5d&interval={interval}");
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
    let keep = if interval == "5m" { 78 } else { 96 };   // ~1 Handelstag (Indizes) bzw. 24 h (Oel)
    if closes.len() > keep {
        closes.drain(..closes.len() - keep);
    }
    Some(json!({
        "p": r["meta"]["regularMarketPrice"].as_f64()?,
        "prev": r["meta"]["previousClose"].as_f64().or_else(|| r["meta"]["chartPreviousClose"].as_f64())?,
        "s": closes,
    }))
}

/// Alle Einzelwerte mit einer Anfrage: { "GS": { "p": 812.3, "prev": 805.1 }, ... }
fn fetch_stocks() -> Value {
    let url = format!("https://query1.finance.yahoo.com/v8/finance/spark?symbols={STOCKS}&range=1d&interval=1d");
    let Some(v) = ureq::get(&url)
        .set("User-Agent", "Mozilla/5.0 (MarginCall Desktop)")
        .timeout(Duration::from_secs(10))
        .call()
        .ok()
        .and_then(|r| r.into_json::<Value>().ok())
    else {
        return json!({});
    };
    let mut out = serde_json::Map::new();
    if let Some(obj) = v.as_object() {
        for (sym, x) in obj {
            let p = x["close"].as_array().and_then(|c| c.iter().rev().find_map(|y| y.as_f64()));
            if let (Some(p), Some(prev)) = (p, x["chartPreviousClose"].as_f64()) {
                out.insert(sym.clone(), json!({ "p": p, "prev": prev }));
            }
        }
    }
    Value::Object(out)
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
    let out = json!({ "time": now, "markets": list, "stocks": fetch_stocks() }).to_string();
    if any {
        *cache.lock().unwrap() = Some((Instant::now(), out.clone()));
    }
    out
}

// ------------------------------------------------------------------ Updates
// Die App fragt (nur mit Zustimmung des Spielers, die Spielansicht entscheidet das) bei GitHub nach der neuesten Version.
// Heruntergeladen wird nur die Datei fuer das eigene System; sie muss Groesse und SHA-256 aus der GitHub-Antwort erfuellen.
// Windows und Linux tauschen die Programmdatei aus und starten neu; auf macOS wird das Disk-Image geoeffnet.
const UPDATE_API: &str = "https://api.github.com/repos/wolfgangrabenstein/margincall/releases/latest";
const UPDATE_MAX_BYTES: u64 = 300 * 1024 * 1024;

#[derive(Clone, Default)]
struct UpdateInfo { version: String, name: String, url: String, size: u64, sha256: String }
struct UpdateState { checked: Option<Instant>, info: Option<UpdateInfo>, phase: &'static str, got: u64, error: String, target: Option<std::path::PathBuf> }
static UPDATE: Mutex<UpdateState> = Mutex::new(UpdateState { checked: None, info: None, phase: "idle", got: 0, error: String::new(), target: None });

/// Dateiendung der Release-Datei fuer dieses System
fn update_suffix() -> Option<&'static str> {
    if cfg!(windows) { Some("-Windows.exe") } else if cfg!(target_os = "macos") { Some("-macOS.dmg") } else if cfg!(target_os = "linux") { Some("-Linux.AppImage") } else { None }
}

/// "3.40.1" > "3.39.0"?
fn version_newer(a: &str, b: &str) -> bool {
    let p = |v: &str| v.trim_start_matches('v').split('.').map(|x| x.parse::<u32>().unwrap_or(0)).collect::<Vec<_>>();
    p(a) > p(b)
}

fn update_fetch() -> Result<Option<UpdateInfo>, String> {
    let suffix = update_suffix().ok_or("system")?;
    let v: Value = ureq::get(UPDATE_API)
        .set("User-Agent", concat!("MarginCall/", env!("CARGO_PKG_VERSION")))
        .set("Accept", "application/vnd.github+json")
        .timeout(Duration::from_secs(10))
        .call()
        .map_err(|e| e.to_string())?
        .into_json()
        .map_err(|e| e.to_string())?;
    let version = v["tag_name"].as_str().unwrap_or("").trim_start_matches('v').to_string();
    if version.is_empty() || !version_newer(&version, env!("CARGO_PKG_VERSION")) {
        return Ok(None);
    }
    let asset = v["assets"].as_array().and_then(|a| a.iter().find(|x| x["name"].as_str().map(|n| n.ends_with(suffix)).unwrap_or(false)));
    let Some(a) = asset else { return Ok(None) };   // Datei fuer dieses System (noch) nicht da
    let sha256 = a["digest"].as_str().and_then(|d| d.strip_prefix("sha256:")).unwrap_or("").to_ascii_lowercase();
    let size = a["size"].as_u64().unwrap_or(0);
    let url = a["browser_download_url"].as_str().unwrap_or("").to_string();
    if sha256.len() != 64 || size == 0 || size > UPDATE_MAX_BYTES || !url.starts_with("https://github.com/wolfgangrabenstein/margincall/releases/download/") {
        return Err("asset".into());
    }
    Ok(Some(UpdateInfo { version, name: clean(&a["name"], 80), url, size, sha256 }))
}

/// Stand fuer die Spielansicht; fragt GitHub hoechstens alle 30 Minuten
fn update_status() -> String {
    let due = { let u = UPDATE.lock().unwrap(); u.phase == "idle" && u.checked.map(|t| t.elapsed() > Duration::from_secs(1800)).unwrap_or(true) };
    if due {
        let r = update_fetch();
        let mut u = UPDATE.lock().unwrap();
        u.checked = Some(Instant::now());
        match r { Ok(i) => { u.info = i; u.error.clear(); } Err(e) => u.error = e }
    }
    let u = UPDATE.lock().unwrap();
    let i = u.info.clone().unwrap_or_default();
    json!({
        "current": env!("CARGO_PKG_VERSION"), "latest": i.version, "available": u.info.is_some(),
        "phase": u.phase, "got": u.got, "size": i.size, "error": u.error,
        "restart": !cfg!(target_os = "macos"),
    }).to_string()
}

/// Herunterladen, pruefen und einsetzen (im Hintergrund; der Stand ist ueber /api/update abrufbar)
fn update_install() {
    let info = {
        let mut u = UPDATE.lock().unwrap();
        if u.phase != "idle" && u.phase != "error" { return; }
        let Some(i) = u.info.clone() else { return };
        u.phase = "download"; u.got = 0; u.error.clear();
        i
    };
    std::thread::spawn(move || {
        let r = update_download(&info).and_then(|data| update_apply(&info, &data));
        let mut u = UPDATE.lock().unwrap();
        match r { Ok(p) => { u.phase = if cfg!(target_os = "macos") { "opened" } else { "ready" }; u.target = Some(p); } Err(e) => { u.phase = "error"; u.error = e; } }
    });
}

fn update_download(i: &UpdateInfo) -> Result<Vec<u8>, String> {
    use sha2::{Digest, Sha256};
    let resp = ureq::get(&i.url).set("User-Agent", concat!("MarginCall/", env!("CARGO_PKG_VERSION"))).timeout(Duration::from_secs(600)).call().map_err(|e| e.to_string())?;
    let mut r = resp.into_reader().take(i.size + 1);
    let mut data = Vec::with_capacity(i.size as usize);
    let mut buf = vec![0u8; 64 * 1024];
    loop {
        let n = r.read(&mut buf).map_err(|e| e.to_string())?;
        if n == 0 { break; }
        data.extend_from_slice(&buf[..n]);
        UPDATE.lock().unwrap().got = data.len() as u64;
    }
    if data.len() as u64 != i.size { return Err("size".into()); }
    let hash: String = Sha256::digest(&data).iter().map(|b| format!("{b:02x}")).collect();
    if hash != i.sha256 { return Err("checksum".into()); }
    Ok(data)
}

/// Neue Datei an ihren Platz bringen; liefert das Programm, das danach gestartet wird (macOS: das geoeffnete Disk-Image)
fn update_apply(i: &UpdateInfo, data: &[u8]) -> Result<std::path::PathBuf, String> {
    if cfg!(target_os = "macos") {
        let dir = std::env::var("HOME").map(|h| std::path::PathBuf::from(h).join("Downloads")).map_err(|_| "home".to_string())?;
        let file = dir.join(&i.name);
        std::fs::write(&file, data).map_err(|e| e.to_string())?;
        std::process::Command::new("open").arg(&file).spawn().map_err(|e| e.to_string())?;
        return Ok(file);
    }
    // Linux: die AppImage-Datei selbst (das laufende Programm liegt in einem eingehaengten Abbild); Windows: die EXE
    let exe = if cfg!(target_os = "linux") { std::env::var_os("APPIMAGE").map(std::path::PathBuf::from).ok_or("appimage")? } else { std::env::current_exe().map_err(|e| e.to_string())? };
    swap_exe(&exe, data)?;
    Ok(exe)
}

/// Programmdatei ersetzen, auch waehrend sie laeuft
fn swap_exe(exe: &std::path::Path, data: &[u8]) -> Result<(), String> {
    let new = exe.with_extension("update");
    std::fs::write(&new, data).map_err(|e| e.to_string())?;
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        std::fs::set_permissions(&new, std::fs::Permissions::from_mode(0o755)).map_err(|e| e.to_string())?;
    }
    if cfg!(windows) {
        // Eine laufende EXE darf umbenannt, aber nicht ueberschrieben werden; die alte wird beim naechsten Start geloescht
        let old = exe.with_extension("old");
        let _ = std::fs::remove_file(&old);
        std::fs::rename(&exe, &old).map_err(|e| { let _ = std::fs::remove_file(&new); e.to_string() })?;
        if let Err(e) = std::fs::rename(&new, &exe) { let _ = std::fs::rename(&old, &exe); return Err(e.to_string()); }
    } else {
        std::fs::rename(&new, &exe).map_err(|e| e.to_string())?;
    }
    Ok(())
}

/// Neue Version auf demselben Port starten und diese beenden
fn update_restart() -> bool {
    let target = { let u = UPDATE.lock().unwrap(); if u.phase != "ready" { return false; } u.target.clone() };
    let (Some(exe), Some(port)) = (target, PORT.get().copied()) else { return false };
    std::thread::spawn(move || {
        std::thread::sleep(Duration::from_millis(300));   // Antwort an die Spielansicht geht noch raus
        if std::process::Command::new(&exe).arg(format!("--port={port}")).spawn().is_ok() {
            std::process::exit(0);
        }
        let mut u = UPDATE.lock().unwrap();
        u.phase = "error"; u.error = "start".into();
    });
    true
}

/// Reste eines Updates aufraeumen (Windows: die umbenannte alte EXE)
fn update_cleanup() {
    if let Ok(exe) = std::env::current_exe() {
        let _ = std::fs::remove_file(exe.with_extension("old"));
        let _ = std::fs::remove_file(exe.with_extension("update"));
    }
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
        // Nimmt ein Mitspieler nichts mehr an (Schreib-Timeout), Verbindung schliessen: sonst haengt das Weiterleiten fuer alle
        if s.write_all(&frame).and_then(|_| s.flush()).is_err() {
            let _ = s.shutdown(Shutdown::Both);
        }
    }
}

/// Liest den naechsten Text-Frame; None bei Verbindungsende oder Protokollverstoss (Nachricht ueber WS_MAX_MSG,
/// Control-Frame ueber 125 Byte oder fragmentiert, Fortsetzung ohne Anfang, neuer Anfang mitten in einer Nachricht).
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
        // Grenzen vor jedem Lesen und Reservieren: Control-Frames hoechstens 125 Byte und nie fragmentiert,
        // Daten-Frames hoechstens WS_MAX_MSG fuer die ganze Nachricht (alle Fragmente zusammen)
        if op >= 0x8 && (n > 125 || !fin) {
            return Ok(None);
        }
        if n > WS_MAX_MSG as u64 || msg.len() as u64 + n > WS_MAX_MSG as u64 {
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
                    if s.write_all(&ws_frame(0xA, &payload)).is_err() {   // Gegenseite nimmt nichts an: Verbindung beenden
                        let _ = s.shutdown(Shutdown::Both);
                        return Ok(None);
                    }
                }
            }
            0xA => {}
            0x0 | 0x1 | 0x2 => {
                if (op == 0x0) == msg.is_empty() {   // Fortsetzung ohne Anfang bzw. neuer Anfang mitten in einer Nachricht
                    return Ok(None);
                }
                msg.extend_from_slice(&payload);
                if fin {
                    return Ok(Some(String::from_utf8_lossy(&msg).into_owned()));
                }
            }
            _ => return Ok(None),   // reservierter Opcode
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
    if !msg.is_object() {
        return;   // das Spiel schickt nur JSON-Objekte
    }
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
    reader.get_ref().set_read_timeout(None).ok();   // im Spiel darf eine Verbindung beliebig lange still sein
    let key = headers.get("sec-websocket-key").cloned().unwrap_or_default();
    let mut sha = Sha1::new();
    sha.update(key.as_bytes());
    sha.update(WS_GUID.as_bytes());
    let accept = base64::engine::general_purpose::STANDARD.encode(sha.finalize());
    let head = format!("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: {accept}\r\n\r\n");
    stream.write_all(head.as_bytes())?;
    stream.flush()?;

    stream.set_write_timeout(Some(WS_WRITE_TIMEOUT)).ok();
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

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;
    use std::net::TcpListener;

    /// Client-Attrappe mit echter, sonst unbenutzter Verbindung (Gegenseite liest die Antworten)
    fn client() -> (Client, TcpStream) {
        let l = TcpListener::bind("127.0.0.1:0").unwrap();
        let s = TcpStream::connect(l.local_addr().unwrap()).unwrap();
        let (peer, _) = l.accept().unwrap();
        (Client { stream: Mutex::new(s) }, peer)
    }

    /// WebSocket-Frame, wie ihn ein Browser schickt (maskiert)
    fn frame(fin: bool, op: u8, data: &[u8]) -> Vec<u8> {
        let mut f = vec![(if fin { 0x80 } else { 0 }) | op];
        let n = data.len();
        if n < 126 {
            f.push(0x80 | n as u8);
        } else if n < 65536 {
            f.push(0x80 | 126);
            f.extend_from_slice(&(n as u16).to_be_bytes());
        } else {
            f.push(0x80 | 127);
            f.extend_from_slice(&(n as u64).to_be_bytes());
        }
        let mask = [1, 2, 3, 4];
        f.extend_from_slice(&mask);
        f.extend(data.iter().enumerate().map(|(i, b)| b ^ mask[i & 3]));
        f
    }

    fn recv(bytes: Vec<u8>) -> io::Result<Option<String>> {
        let (c, _peer) = client();
        ws_recv(&mut Cursor::new(bytes), &c)
    }

    #[test]
    fn ws_nachricht_und_fragmente() {
        assert_eq!(recv(frame(true, 0x1, b"hallo")).unwrap(), Some("hallo".into()));
        let mut b = frame(false, 0x1, b"ab");
        b.extend(frame(false, 0x0, b"c"));
        b.extend(frame(true, 0x0, b"d"));
        assert_eq!(recv(b).unwrap(), Some("abcd".into()));
    }

    #[test]
    fn ws_groesse_begrenzt() {
        assert!(recv(frame(true, 0x1, &vec![b'x'; WS_MAX_MSG])).unwrap().is_some());   // genau an der Grenze
        assert_eq!(recv(frame(true, 0x1, &vec![b'x'; WS_MAX_MSG + 1])).unwrap(), None);
        let mut b = vec![0x81, 0x80 | 127];   // angekuendigte Riesenlaenge: abgelehnt, bevor etwas gelesen oder reserviert wird
        b.extend_from_slice(&(u64::MAX / 2).to_be_bytes());
        assert_eq!(recv(b).unwrap(), None);
        let mut b = frame(false, 0x1, &vec![b'x'; WS_MAX_MSG]);   // Fragmente zusammen ueber der Grenze
        b.extend(frame(true, 0x0, b"y"));
        assert_eq!(recv(b).unwrap(), None);
    }

    #[test]
    fn ws_protokollfehler() {
        assert_eq!(recv(frame(true, 0x9, &[0u8; 126])).unwrap(), None);   // Ping ueber 125 Byte
        assert_eq!(recv(frame(false, 0x9, b"x")).unwrap(), None);         // fragmentierter Ping
        assert_eq!(recv(frame(true, 0x0, b"x")).unwrap(), None);          // Fortsetzung ohne Anfang
        let mut b = frame(false, 0x1, b"a");
        b.extend(frame(true, 0x1, b"b"));
        assert_eq!(recv(b).unwrap(), None);   // neuer Anfang mitten in einer Nachricht
        assert_eq!(recv(frame(true, 0x3, b"x")).unwrap(), None);   // reservierter Opcode
        assert_eq!(recv(frame(true, 0x8, b"")).unwrap(), None);    // Close
    }

    #[test]
    fn ws_ping_wird_beantwortet() {
        let (c, mut peer) = client();
        let mut b = frame(true, 0x9, b"hi");
        b.extend(frame(true, 0xA, b"pong"));   // Pong des Clients wird ignoriert
        b.extend(frame(true, 0x1, b"ok"));
        assert_eq!(ws_recv(&mut Cursor::new(b), &c).unwrap(), Some("ok".into()));
        let mut pong = [0u8; 4];
        peer.read_exact(&mut pong).unwrap();
        assert_eq!(pong, [0x8A, 2, b'h', b'i']);
    }

    #[test]
    fn laufende_exe_ersetzen() {
        // Kopie eines Windows-Programms starten und waehrend der Laufzeit ersetzen
        let dir = std::env::temp_dir().join("mc-swap-test");
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(&dir).unwrap();
        let exe = dir.join("prog.exe");
        let src = if cfg!(windows) { std::path::PathBuf::from(std::env::var("SystemRoot").unwrap()).join("System32").join("PING.EXE") } else { std::path::PathBuf::from("/bin/sleep") };
        std::fs::copy(&src, &exe).unwrap();
        let mut child = std::process::Command::new(&exe).args(if cfg!(windows) { vec!["-n", "5", "127.0.0.1"] } else { vec!["3"] }).stdout(std::process::Stdio::null()).spawn().unwrap();
        swap_exe(&exe, b"neue Version").unwrap();
        assert_eq!(std::fs::read(&exe).unwrap(), b"neue Version");
        assert!(!exe.with_extension("update").exists());
        let _ = child.kill();
        let _ = child.wait();
        let _ = std::fs::remove_dir_all(&dir);
    }

    #[test]
    fn versionen_vergleichen() {
        assert!(version_newer("3.40.0", "3.39.9"));
        assert!(version_newer("v3.39.10", "3.39.9"));
        assert!(!version_newer("3.39.0", "3.39.0"));
        assert!(!version_newer("3.38.1", "3.39.0"));
    }

    #[test]
    fn http_zeilen_und_header_begrenzt() {
        let (line, h) = http_head(&mut Cursor::new(b"GET /api/ping HTTP/1.1\r\nHost: x\r\nUpgrade: WebSocket\r\n\r\n".to_vec())).unwrap().unwrap();
        assert!(line.starts_with("GET /api/ping"));
        assert_eq!(h.get("host").map(String::as_str), Some("x"));
        assert_eq!(h.get("upgrade").map(String::as_str), Some("WebSocket"));
        assert!(http_head(&mut Cursor::new(vec![b'a'; HTTP_MAX_LINE as usize + 1])).is_err());   // Zeile ohne Ende
        let mut many = b"GET / HTTP/1.1\r\n".to_vec();
        for i in 0..=HTTP_MAX_HEADERS {
            many.extend(format!("h{i}: v\r\n").as_bytes());
        }
        many.extend(b"\r\n");
        assert!(http_head(&mut Cursor::new(many)).is_err());   // zu viele Header
        let mut same = b"GET / HTTP/1.1
".to_vec();
        for _ in 0..=HTTP_MAX_HEADERS {
            same.extend(b"a: b
");
        }
        same.extend(b"
");
        assert!(http_head(&mut Cursor::new(same)).is_err());   // gleicher Name immer wieder
        assert!(http_head(&mut Cursor::new(Vec::new())).unwrap().is_none());   // leere Verbindung
    }
}
