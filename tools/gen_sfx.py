"""Erzeugt src/sfx_data.h: synthetisierte Bank- und Geld-Sounds als int8-PCM (22050 Hz).

Alle Klaenge sind physikalisch angelehnt nachgebaut (Glocken-/Muenz-Obertoene,
gefiltertes Rauschen, Mechanik-Klicks) - keine fremden Samples, keine Lizenzfragen.
Aufruf:  py tools/gen_sfx.py
"""
import math
import os
import random
import wave

SR = 22050
rng = random.Random(42)
TAU = 2 * math.pi


def buf(sec):
    return [0.0] * int(sec * SR)


def add_bell(out, start, f0, partials, amp=1.0, length=None):
    """Angeschlagener Metallkoerper: partials = [(ratio, amp, decay/s), ...]"""
    s0 = int(start * SR)
    n = len(out) - s0 if length is None else min(len(out) - s0, int(length * SR))
    ph = [rng.random() * TAU for _ in partials]
    for i in range(n):
        t = i / SR
        v = 0.0
        for k, (r, a, d) in enumerate(partials):
            f = f0 * r
            if f >= SR / 2:
                continue
            v += a * math.exp(-d * t) * math.sin(TAU * f * t + ph[k])
        atk = min(1.0, i / 20.0)
        out[s0 + i] += amp * atk * v


def add_noise(out, start, sec, amp, decay=0.0, lp=1.0, hp=0.0, env=None):
    """Rauschen mit Einpol-Tiefpass (lp 0..1) und optionalem Hochpass."""
    s0 = int(start * SR)
    n = min(len(out) - s0, int(sec * SR))
    y = 0.0
    prev = 0.0
    for i in range(n):
        t = i / SR
        x = rng.uniform(-1, 1)
        y += lp * (x - y)
        v = y
        if hp > 0:
            v = y - prev * hp
            prev = y
        e = math.exp(-decay * t)
        if env:
            e *= env(t)
        out[s0 + i] += amp * e * v


def add_sweep(out, start, sec, f_from, f_to, amp, decay=0.0, wave="sin", curve=1.0):
    s0 = int(start * SR)
    n = min(len(out) - s0, int(sec * SR))
    ph = 0.0
    for i in range(n):
        t = i / SR
        p = (t / sec) ** curve
        f = f_from + (f_to - f_from) * p
        ph += TAU * f / SR
        if wave == "sin":
            v = math.sin(ph)
        elif wave == "sq":
            v = 1.0 if math.sin(ph) >= 0 else -1.0
        else:  # saw
            v = ((ph / TAU) % 1.0) * 2 - 1
        out[s0 + i] += amp * math.exp(-decay * t) * v


def lowpass(out, a):
    y = 0.0
    for i, x in enumerate(out):
        y += a * (x - y)
        out[i] = y


def fade_out(out, sec):
    n = int(sec * SR)
    for i in range(n):
        out[len(out) - n + i] *= 1 - i / n


def coin_ping(out, start, amp, base=None, short=False):
    f = base or rng.uniform(3600, 4600)
    parts = [(1.0, 1.0, 18), (1.47, 0.6, 26), (2.09, 0.45, 34), (2.56, 0.3, 45)]
    if short:
        parts = [(r, a, d * 3) for r, a, d in parts]
    add_bell(out, start, f, parts, amp, length=0.5)
    add_noise(out, start, 0.004, amp * 0.6, hp=0.9)


# ------------------------------------------------------------------ Sounds
def snd_kaching():
    """Mechanische Registrierkasse: Tasten-Klack, 'Ding', Schublade rollt + knallt, Muenzen klimpern."""
    o = buf(1.05)
    add_noise(o, 0.0, 0.006, 0.9, hp=0.95)                      # Tastenklick
    add_bell(o, 0.0, 420, [(1, 0.6, 60), (2.3, 0.3, 90)], 0.8, 0.05)  # Holz/Mechanik
    add_noise(o, 0.035, 0.01, 0.6, lp=0.5)                       # Hebel
    bell = [(1.0, 1.0, 3.2), (1.003, 0.6, 3.4), (2.32, 0.55, 5.5), (4.25, 0.3, 9.0), (5.1, 0.12, 14)]
    add_bell(o, 0.05, 2250, bell, 0.9)                            # DING
    # Schublade rollt heraus (Rumpeln mit Rollen-Modulation)
    add_noise(o, 0.17, 0.28, 0.55, lp=0.08,
              env=lambda t: min(1, t / 0.05) * (0.6 + 0.4 * math.sin(TAU * 34 * t)))
    # Schublade schlaegt an
    add_sweep(o, 0.45, 0.14, 110, 55, 0.9, decay=28)
    add_noise(o, 0.45, 0.05, 0.8, decay=60, lp=0.3)
    for k in range(5):                                            # Muenzen im Fach
        coin_ping(o, 0.455 + k * 0.018 + rng.uniform(0, 0.01), 0.22 - k * 0.03, short=True)
    fade_out(o, 0.15)
    return o


def snd_coins():
    """Muenzen fallen auf den Tresen und trudeln aus."""
    o = buf(0.75)
    times = [0, 0.085, 0.15, 0.2, 0.235, 0.26]
    for k, t in enumerate(times):
        coin_ping(o, t, 0.9 * (0.82 ** k))
    # Muenze dreht sich aus: immer schnellere, leisere Klicks
    t, gap, a = 0.3, 0.045, 0.35
    while t < 0.68 and gap > 0.004:
        coin_ping(o, t, a, base=5200, short=True)
        t += gap
        gap *= 0.82
        a *= 0.9
    fade_out(o, 0.08)
    return o


def snd_shot():
    """'Money Gun': Wumms + Geldscheinzaehler-Flattern."""
    o = buf(0.2)
    add_sweep(o, 0.0, 0.07, 170, 45, 1.0, decay=35)
    add_noise(o, 0.0, 0.008, 0.9, hp=0.9)
    t = 0.006
    while t < 0.19:
        add_noise(o, t, 0.005, 0.55 * (1 - t / 0.2), decay=500, lp=0.6, hp=0.7)
        t += 1 / 75
    fade_out(o, 0.03)
    return o


def snd_hit():
    """Treffer: Papier-Klatscher."""
    o = buf(0.09)
    add_noise(o, 0, 0.03, 1.0, decay=90, lp=0.35)
    add_sweep(o, 0, 0.05, 260, 120, 0.7, decay=50)
    return o


def snd_stamp():
    """Gegner liquidiert: schwerer Bank-Stempel auf Holztresen ('BAMM')."""
    o = buf(0.32)
    add_noise(o, 0.0, 0.004, 0.8, hp=0.9)
    add_sweep(o, 0.0, 0.18, 95, 48, 1.0, decay=18)
    add_bell(o, 0.0, 230, [(1, 0.5, 30), (1.78, 0.35, 40), (2.9, 0.2, 55)], 0.8, 0.2)
    add_noise(o, 0.0, 0.04, 0.9, decay=70, lp=0.25)
    add_noise(o, 0.07, 0.12, 0.12, decay=20, hp=0.8)          # Papier raschelt
    fade_out(o, 0.05)
    return o


def snd_phone():
    """Margin Call: altes Telefon mit zwei Glocken und Kloeppel (~20 Hz)."""
    o = buf(0.55)
    parts = [(1, 1.0, 9), (2.71, 0.5, 14), (5.2, 0.25, 22)]
    t, k = 0.0, 0
    while t < 0.42:
        add_bell(o, t, 1150 if k % 2 == 0 else 1390, parts, 0.45, length=0.13)
        t += 0.025
        k += 1
    fade_out(o, 0.12)
    return o


def snd_zap():
    """Verkaufsorder / Gegner-Schuss: fallender Rechteck-Ton."""
    o = buf(0.17)
    add_sweep(o, 0, 0.17, 1300, 220, 0.5, decay=8, wave="sq", curve=0.6)
    lowpass(o, 0.35)
    return o


def snd_bell():
    """NYSE-Eroeffnungs-/Schlussglocke: elektrische Glocke, Kloeppel ~17 Hz."""
    o = buf(1.9)
    parts = [(1.0, 1.0, 2.5), (2.76, 0.6, 4.5), (5.40, 0.35, 8), (8.93, 0.15, 12), (1.006, 0.5, 2.6)]
    t = 0.0
    while t < 1.35:
        add_bell(o, t, 640, parts, 0.16, length=0.55)
        add_noise(o, t, 0.003, 0.08, hp=0.9)
        t += 1 / 17
    fade_out(o, 0.3)
    return o


def snd_crash():
    """Game Over: Boersencrash - trauriges 'Wah-wah-wah-waaah' + Rumpeln."""
    o = buf(1.5)
    notes = [(0.0, 392), (0.3, 370), (0.6, 349), (0.9, 330)]
    for i, (st, f) in enumerate(notes):
        ln = 0.28 if i < 3 else 0.6
        s0 = int(st * SR)
        ph = 0.0
        for n in range(int(ln * SR)):
            t = n / SR
            vib = 1 + (0.02 * math.sin(TAU * 6 * t) * min(1, t / 0.15) if i == 3 else 0)
            ph += TAU * f * vib / SR
            saw = ((ph / TAU) % 1.0) * 2 - 1
            env = min(1, t / 0.03) * (1 - t / ln) ** 0.5
            o[s0 + n] += 0.5 * env * saw
    lowpass(o, 0.12)
    add_noise(o, 0.0, 1.5, 0.25, lp=0.03, env=lambda t: math.sin(math.pi * t / 1.5))
    fade_out(o, 0.1)
    return o


def snd_declined():
    """Keine Liquiditaet: Karte abgelehnt - zwei tiefe Pieptoene."""
    o = buf(0.24)
    add_sweep(o, 0.0, 0.09, 440, 440, 0.4, wave="sq")
    add_sweep(o, 0.12, 0.11, 330, 330, 0.4, wave="sq")
    lowpass(o, 0.4)
    return o


def snd_pop():
    """Krypto-Blase platzt."""
    o = buf(0.14)
    add_sweep(o, 0, 0.04, 250, 1400, 0.9, decay=40)
    add_noise(o, 0, 0.02, 0.8, decay=150, hp=0.6)
    add_sweep(o, 0.03, 0.1, 1800, 600, 0.25, decay=30)
    return o


def formant_voice(sec, f_curve, formants, breath=0.15, growl=0.0, growl_hz=30):
    """Tierstimme: Saegezahn-Quelle durch Resonanzfilter (Formanten) + Atemrauschen."""
    o = buf(sec)
    n = len(o)
    ph = 0.0
    src = [0.0] * n
    for i in range(n):
        t = i / SR
        f = f_curve(t / sec)
        ph += TAU * f / SR
        saw = ((ph / TAU) % 1.0) * 2 - 1
        env = min(1, t / 0.06) * min(1, (sec - t) / 0.15)
        am = 1 - growl * (0.5 + 0.5 * math.sin(TAU * growl_hz * t))
        src[i] = env * am * (saw + breath * rng.uniform(-1, 1))
    for fc, bw, gain in formants:   # 2-Pol-Resonator je Formant
        r = math.exp(-math.pi * bw / SR)
        c1, c2 = 2 * r * math.cos(TAU * fc / SR), -r * r
        y1 = y2 = 0.0
        for i in range(n):
            y = (1 - r) * src[i] + c1 * y1 + c2 * y2
            y2, y1 = y1, y
            o[i] += gain * y
    return o


def snd_bull():
    """Bulle: tiefes Brüllen mit Tonhoehenbogen + Schnauben."""
    o = formant_voice(0.85, lambda p: 95 + 70 * math.sin(math.pi * min(1, p * 1.3)),
                      [(420, 120, 1.0), (900, 160, 0.6), (2300, 300, 0.25)], breath=0.3)
    add_noise(o, 0.0, 0.12, 0.25, lp=0.3, env=lambda t: math.sin(math.pi * t / 0.12))   # Schnauben
    return o


def snd_roar():
    """Baer: knurrendes Bruellen."""
    return formant_voice(0.7, lambda p: 70 + 25 * math.sin(math.pi * p),
                         [(350, 140, 1.0), (750, 200, 0.7), (1800, 400, 0.3)], breath=0.8, growl=0.6, growl_hz=28)


def snd_glitch():
    """KI ALGO-9000 greift ein: Modem-/Daten-Kreischen mit Bitcrush."""
    o = buf(0.7)
    t = 0.0
    while t < 0.66:
        ln = rng.choice([0.012, 0.02, 0.03])
        f = rng.choice([1200, 1800, 2400, 600, 3000, 980])
        add_sweep(o, t, ln, f, f * rng.uniform(0.8, 1.2), 0.35, wave="sq")
        t += ln
    add_noise(o, 0.0, 0.7, 0.15, hp=0.5)
    for i in range(len(o)):          # Bitcrusher
        o[i] = round(o[i] * 4) / 4
    fade_out(o, 0.05)
    return o


def snd_oil():
    """Oelfass / Oelschlick: dickes Gluckern."""
    o = buf(0.5)
    for k in range(4):
        add_sweep(o, k * 0.1 + rng.uniform(0, 0.02), 0.09, 140, 420, 0.7, decay=18, curve=2)
    lowpass(o, 0.2)
    return o


def snd_laser():
    """Trading-Bot-Laser."""
    o = buf(0.14)
    add_sweep(o, 0, 0.14, 2600, 700, 0.5, decay=10, wave="sq", curve=0.5)
    add_sweep(o, 0, 0.14, 2650, 720, 0.3, decay=10, wave="sq", curve=0.5)
    lowpass(o, 0.5)
    return o


def snd_dodge():
    """Ausweichen: Whoosh (Rauschen mit Lautstaerke-Bogen) + kurzer Sprung-Thump."""
    o = buf(0.3)
    add_noise(o, 0.0, 0.3, 0.9, lp=0.35, hp=0.5, env=lambda t: math.sin(math.pi * min(1.0, t / 0.26)) ** 2)
    add_sweep(o, 0.0, 0.08, 140, 60, 0.6, decay=30)
    fade_out(o, 0.05)
    return o


SOUNDS = [
    ("KACHING", snd_kaching), ("COINS", snd_coins), ("SHOT", snd_shot), ("HIT", snd_hit),
    ("STAMP", snd_stamp), ("PHONE", snd_phone), ("ZAP", snd_zap), ("BELL", snd_bell),
    ("CRASH", snd_crash), ("DECLINED", snd_declined), ("POP", snd_pop),
    ("BULL", snd_bull), ("ROAR", snd_roar), ("GLITCH", snd_glitch), ("OIL", snd_oil), ("LASER", snd_laser),
    ("DODGE", snd_dodge),
]


def to_int8(o):
    peak = max(1e-6, max(abs(x) for x in o))
    g = 0.97 / peak
    return [max(-127, min(127, int(round(x * g * 127)))) for x in o]


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    path = os.path.join(here, "..", "src", "sfx_data.h")
    lines = ["// AUTOMATISCH ERZEUGT von tools/gen_sfx.py - nicht von Hand bearbeiten", "#pragma once",
             "#include <stdint.h>", f"#define SFX_RATE {SR}", ""]
    total = 0
    wav_dir = os.path.join(here, "..", "sound_preview")
    os.makedirs(wav_dir, exist_ok=True)
    for name, fn in SOUNDS:
        data = to_int8(fn())
        total += len(data)
        with wave.open(os.path.join(wav_dir, name.lower() + ".wav"), "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(1)
            w.setframerate(SR)
            w.writeframes(bytes((v + 128) & 255 for v in data))
        lines.append(f"static const int8_t SFX_{name}[{len(data)}] = {{")
        for i in range(0, len(data), 32):
            lines.append("  " + ",".join(str(v) for v in data[i:i + 32]) + ",")
        lines.append("};")
    with open(path, "w", encoding="ascii") as f:
        f.write("\n".join(lines) + "\n")
    print(f"{len(SOUNDS)} Sounds, {total} Bytes -> {os.path.normpath(path)}")


if __name__ == "__main__":
    main()
