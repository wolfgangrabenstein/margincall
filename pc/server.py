"""MARGIN CALL - PC-Version.

Startet einen kleinen Spiel-Server, holt die aktuellen Kurse (Dow, S&P 500, Nasdaq-100, Rohoel WTI)
und oeffnet das Spiel im Browser. Andere im selben Netz koennen ueber http://<IP>:8080 mitspielen.

Multiplayer: Der Server ist nur ein Relay (WebSocket /ws). Der erste Spieler in der Lobby ist Host,
sein Browser berechnet das Spiel; alle anderen schicken ihre Eingaben an den Host.

Nur Python-Standardbibliothek. Als EXE bauen:  py -m PyInstaller --onefile --add-data "game.html;." server.py
"""
import base64
import hashlib
import json
import os
import socket
import struct
import sys
import threading
import time
import urllib.request
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

PORT = int(os.environ.get("MARGINCALL_PORT", "8080"))
CACHE_SECONDS = 60
MAX_PLAYERS = 4
SYMBOLS = [("DOW", "%5EDJI", "5m"), ("S&P", "%5EGSPC", "5m"), ("NDX", "%5ENDX", "5m"), ("OEL", "CL%3DF", "15m")]
WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

_lock = threading.Lock()
_cache = {"time": 0, "data": None}


def resource(name):
    base = getattr(sys, "_MEIPASS", os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base, name)


# ------------------------------------------------------------------ Kurse
def fetch_one(symbol, interval):
    url = f"https://query1.finance.yahoo.com/v8/finance/chart/{symbol}?range=1d&interval={interval}"
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0 (MarginCall PC)"})
    with urllib.request.urlopen(req, timeout=10) as r:
        d = json.load(r)["chart"]["result"][0]
    closes = [round(x, 2) for x in d["indicators"]["quote"][0]["close"] if x is not None]
    while len(closes) > 96:
        closes = [round((closes[i] + closes[i + 1]) / 2, 2) for i in range(0, len(closes) - 1, 2)]
    m = d["meta"]
    return {"p": m["regularMarketPrice"], "prev": m["chartPreviousClose"], "s": closes}


def get_markets():
    with _lock:
        if _cache["data"] and time.time() - _cache["time"] < CACHE_SECONDS:
            return _cache["data"]
    result = []
    for label, sym, iv in SYMBOLS:
        try:
            result.append(fetch_one(sym, iv))
        except Exception as e:  # einzelner Markt nicht verfuegbar -> Spiel simuliert ihn
            print(f"  [!] {label}: Kurs nicht abrufbar ({e})")
            result.append(None)
    data = {"time": int(time.time()), "markets": result}
    with _lock:
        if any(result):
            _cache.update(time=time.time(), data=data)
    return data


# ------------------------------------------------------------------ Multiplayer-Relay
class WSConn:
    def __init__(self, handler):
        self.h = handler
        self.wlock = threading.Lock()
        self.alive = True

    def send(self, text):
        data = text.encode("utf-8")
        n = len(data)
        head = bytearray([0x81])
        if n < 126:
            head.append(n)
        elif n < 65536:
            head.append(126)
            head += struct.pack(">H", n)
        else:
            head.append(127)
            head += struct.pack(">Q", n)
        try:
            with self.wlock:
                self.h.wfile.write(bytes(head) + data)
                self.h.wfile.flush()
        except OSError:
            self.alive = False

    def _read(self, n):
        buf = b""
        while len(buf) < n:
            chunk = self.h.rfile.read(n - len(buf))
            if not chunk:
                raise ConnectionError
            buf += chunk
        return buf

    def recv(self):
        """Liefert den naechsten Text-Frame oder None bei Verbindungsende."""
        message = b""
        while True:
            b1, b2 = self._read(2)
            fin, op = b1 & 0x80, b1 & 0x0F
            n = b2 & 0x7F
            if n == 126:
                n = struct.unpack(">H", self._read(2))[0]
            elif n == 127:
                n = struct.unpack(">Q", self._read(8))[0]
            mask = self._read(4) if b2 & 0x80 else b"\0\0\0\0"
            payload = bytes(c ^ mask[i & 3] for i, c in enumerate(self._read(n)))
            if op == 8:
                return None
            if op == 9:   # Ping -> Pong
                with self.wlock:
                    self.h.wfile.write(bytes([0x8A, len(payload)]) + payload)
                continue
            if op in (0, 1, 2):
                message += payload
                if fin:
                    return message.decode("utf-8", "replace")


class Relay:
    def __init__(self):
        self.lock = threading.Lock()
        self.clients = {}   # id -> WSConn

    def host(self):
        return min(self.clients) if self.clients else 0

    def join(self, conn):
        with self.lock:
            free = [i for i in range(1, MAX_PLAYERS + 1) if i not in self.clients]
            if not free:
                return 0
            cid = free[0]
            self.clients[cid] = conn
        conn.send(json.dumps({"t": "welcome", "id": cid, "host": self.host()}))
        self.peers()
        return cid

    def leave(self, cid):
        with self.lock:
            self.clients.pop(cid, None)
        self.peers()

    def peers(self):
        with self.lock:
            msg = json.dumps({"t": "peers", "ids": sorted(self.clients), "host": self.host()})
            targets = list(self.clients.values())
        for c in targets:
            c.send(msg)

    def route(self, cid, text):
        try:
            msg = json.loads(text)
        except ValueError:
            return
        with self.lock:
            host = self.host()
            clients = dict(self.clients)
        if cid == host:
            to = msg.pop("to", "all")
            out = json.dumps(msg, separators=(",", ":"))
            if to == "all":
                for i, c in clients.items():
                    if i != cid:
                        c.send(out)
            elif to in clients:
                clients[to].send(out)
        elif host in clients:
            msg["from"] = cid
            clients[host].send(json.dumps(msg, separators=(",", ":")))


relay = Relay()


# ------------------------------------------------------------------ HTTP
class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def _send(self, code, body, ctype):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = self.path.split("?")[0]
        if path in ("/", "/index.html"):
            with open(resource("game.html"), "rb") as f:
                self._send(200, f.read(), "text/html; charset=utf-8")
        elif path == "/api/ping":
            self._send(200, b"ok", "text/plain")
        elif path == "/api/info":
            self._send(200, json.dumps({"ip": lan_ip(), "port": PORT}).encode(), "application/json")
        elif path == "/api/market":
            self._send(200, json.dumps(get_markets()).encode(), "application/json")
        elif path == "/ws" and self.headers.get("Upgrade", "").lower() == "websocket":
            self.websocket()
        else:
            self._send(404, b"not found", "text/plain")

    def websocket(self):
        key = self.headers.get("Sec-WebSocket-Key", "")
        accept = base64.b64encode(hashlib.sha1((key + WS_GUID).encode()).digest()).decode()
        self.send_response(101, "Switching Protocols")
        self.send_header("Upgrade", "websocket")
        self.send_header("Connection", "Upgrade")
        self.send_header("Sec-WebSocket-Accept", accept)
        self.end_headers()
        self.wfile.flush()
        self.connection.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        conn = WSConn(self)
        cid = relay.join(conn)
        if not cid:
            conn.send(json.dumps({"t": "full"}))
            self.close_connection = True
            return
        print(f"  Spieler {cid} verbunden ({self.client_address[0]})")
        try:
            while conn.alive:
                text = conn.recv()
                if text is None:
                    break
                relay.route(cid, text)
        except (ConnectionError, OSError):
            pass
        finally:
            relay.leave(cid)
            print(f"  Spieler {cid} getrennt")
            self.close_connection = True

    def log_message(self, *args):
        pass


class Server(ThreadingHTTPServer):
    daemon_threads = True


def lan_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except OSError:
        return "127.0.0.1"


def main():
    print("=" * 56)
    print("  MARGIN CALL - Boersen-Shooter (PC-Version)")
    print("=" * 56)
    print("  Lade aktuelle Kurse ...")
    data = get_markets()
    for (label, _, _), m in zip(SYMBOLS, data["markets"]):
        if m:
            chg = (m["p"] - m["prev"]) / m["prev"] * 100
            print(f"   {label:4} {m['p']:>12,.2f}  {chg:+.2f} %")
    try:
        server = Server(("0.0.0.0", PORT), Handler)
    except OSError:
        print(f"\n  Port {PORT} ist belegt - laeuft das Spiel schon? Oeffne http://localhost:{PORT}")
        input("\n  Enter zum Beenden ...")
        return
    print(f"\n  Spiel:            http://localhost:{PORT}")
    print(f"  Mitspieler (WLAN): http://{lan_ip()}:{PORT}")
    print("\n  Dieses Fenster offen lassen. Schliessen = Spiel-Server beenden.\n")
    if not os.environ.get("MARGINCALL_NO_BROWSER"):
        threading.Timer(0.8, lambda: webbrowser.open(f"http://localhost:{PORT}")).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
