"""Erzeugt desktop/app-icon.png (1024x1024): Candlestick-Chart mit goldenem Bullen-Horn-Pfeil.
Nur Standardbibliothek. Danach:  npx tauri icon app-icon.png  (im Ordner desktop)
"""
import math
import os
import struct
import zlib

N = 1024
px = bytearray(N * N * 4)


def put(x, y, r, g, b, a=255):
    if 0 <= x < N and 0 <= y < N:
        i = (y * N + x) * 4
        px[i:i + 4] = bytes((r, g, b, a))


def rect(x0, y0, x1, y1, c):
    for y in range(max(0, y0), min(N, y1)):
        for x in range(max(0, x0), min(N, x1)):
            put(x, y, *c)


# abgerundetes Quadrat mit dunklem Verlauf
R = 190
for y in range(N):
    for x in range(N):
        dx = max(R - x, 0, x - (N - 1 - R))
        dy = max(R - y, 0, y - (N - 1 - R))
        if dx * dx + dy * dy > R * R:
            continue
        t = y / N
        put(x, y, int(8 + 14 * t), int(12 + 22 * t), int(28 + 30 * t))

# Gitterlinien
for gy in range(200, 900, 140):
    rect(110, gy, 914, gy + 4, (26, 40, 66))

# Kerzen: steigender Markt
candles = [(170, 640, 760, 590, 800, 0), (290, 560, 700, 520, 730, 1), (410, 600, 650, 560, 690, 0),
           (530, 470, 600, 430, 640, 0), (650, 400, 520, 360, 560, 1), (770, 260, 420, 220, 450, 0)]
for x, top, bot, wt, wb, red in candles:
    col = (230, 60, 60) if red else (40, 220, 110)
    rect(x + 38, wt, x + 50, wb, col)
    rect(x, top, x + 88, bot, col)

# goldener Pfeil (Trend) mit dicker Linie
pts = [(150, 820), (360, 640), (500, 690), (840, 300)]
for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
    steps = int(math.hypot(x1 - x0, y1 - y0))
    for i in range(steps):
        cx, cy = x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps
        for oy in range(-16, 17):
            for ox in range(-16, 17):
                if ox * ox + oy * oy <= 256:
                    put(int(cx + ox), int(cy + oy), 255, 205, 50)
# Pfeilspitze
tip, a = (870, 265), math.atan2(300 - 690, 840 - 500)
for y in range(150, 420):
    for x in range(700, 960):
        # Dreieck: Abstand entlang/quer zur Richtung
        rx, ry = x - tip[0], y - tip[1]
        along = -(rx * math.cos(a) + ry * math.sin(a))
        across = abs(-rx * math.sin(a) + ry * math.cos(a))
        if 0 <= along <= 150 and across <= along * 0.6:
            put(x, y, 255, 205, 50)

raw = b"".join(b"\x00" + bytes(px[y * N * 4:(y + 1) * N * 4]) for y in range(N))
png = b"\x89PNG\r\n\x1a\n"
for tag, data in [(b"IHDR", struct.pack(">IIBBBBB", N, N, 8, 6, 0, 0, 0)), (b"IDAT", zlib.compress(raw, 9)), (b"IEND", b"")]:
    png += struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "desktop", "app-icon.png")
open(out, "wb").write(png)
print("ok", os.path.normpath(out), len(png))
