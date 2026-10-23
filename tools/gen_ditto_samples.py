#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Gadlele
"""Build Ditto's sound library (the DITTO engine's SOUND A / SOUND B) into a C header.

The sounds of a mid-eighties sampler's library, from CC0 recordings (assets/samples-eii/, tools/fetch_eii.py,
Versilian Studios) and two made here:
  CHOIR     no CC0 choir exists: eight voices singing "ah", synthesized (additive, each harmonic weighted by the
            vowel's formants; every singer a little out of tune, with a vibrato of their own, and a breath)
  ORCH HIT  mixed from HIT/: two trumpets in octaves, the violins' spiccato on the root and the fifth below and a
            timpani stroke, struck together

Stored as IMA ADPCM (4 bit) at RATE[set] (22,050 Hz, the low sets 16,000 Hz), in Felucca's zone layout (eng_sample.c
smp_zone_t): each set's zones split the keyboard between their roots. The sampler's own character (8-bit companding,
no interpolation, the DAC's grain) is NOT in the data: the engine adds it as it plays (eng_golana.c CHARACTER), so
CLEAN can play the recordings as they are.

  gen_ditto_samples.py OUT.h

The header is cached (build/gen_ditto_samples.cache) under a hash of every input file and this script.
"""
import hashlib
import math
import random
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_samples as gs  # noqa: E402
from sampleio import detect_hz, hz_to_midi, ima_encode, key_split, onset, peak, resample, yin  # noqa: E402

SRC = Path(__file__).resolve().parents[1]
LIB = SRC / "assets" / "samples-eii"
CACHE = SRC / "build" / "gen_ditto_samples.cache"
TR = 22050
GRID = 32                           # START: the decoder's state at k * n / GRID of each zone (eng_golana.c fo_start)

# (name, kind, seconds kept per zone): kind "sus" a looped sustain, "oneshot" a decaying note, "choir" / "hit" made
# here. The order is the SOUND list's: append only (programs store the index)
SETS = [("PIANO", "oneshot", 0.62), ("STRINGS", "sus", 0.80), ("CHOIR", "choir", 0.90), ("BRASS", "sus", 0.70),
        ("FLUTE", "sus", 0.70), ("CELLO", "sus", 0.85), ("ORGAN", "sus", 0.55), ("HARPSI", "oneshot", 0.60),
        ("MARIMBA", "oneshot", 0.45), ("BELLS", "oneshot", 0.85), ("PIZZ", "oneshot", 0.35), ("BASS", "oneshot", 0.70),
        ("TIMPANI", "oneshot", 0.90), ("ORCH HIT", "hit", 0.55), ("RECORDER", "sus", 0.70), ("HARP", "oneshot", 0.70)]
RATE = {"CELLO": 16000, "BASS": 16000, "TIMPANI": 16000, "RECORDER": 16000}
LOOP_AT = 0.40                      # a sustain loops from this fraction of the kept note to its end ..
LOOP_XF = 0.06                      # .. its end crossfaded into the loop's start over this many seconds
CHOIR_ROOTS = (48, 57, 67)          # C3 (men), A3, G4 (women)
TIMPANI_AT = (0.15, 0.35)           # a timpani's pitch: YIN over this part of the stroke (s after the onset), the
                                    # attack and the end of the name disagree (Timpani3: ~37.5, between C#2 and D2)


def norm(x):
    pk = peak(x)
    return [int(v * 30000 / pk) for v in x]


def fade_out(x, sec, sr):
    n, f = len(x), min(len(x) // 3, int(sec * sr))
    for i in range(f):
        x[n - f + i] *= 1 - i / f
    return x


def sus_loop(x, sr, xf=LOOP_XF):
    """a crossfaded sustain loop in the steady part -> (x, (ls, le))"""
    n = len(x)
    ls, le, f = int(LOOP_AT * n), n - 1, int(xf * sr)
    f = min(f, ls)
    for i in range(f):
        a = i / f
        x[le - f + i] = x[le - f + i] * (1 - a) + x[ls - f + i] * a
    return x, (ls, le)


def recorded(name, kind, keep, sr_to):
    """a recorded set's zones -> [(samples, loop or None, root)]"""
    files = sorted((LIB / name).glob("*.wav"))
    conv = gs.octave_conv(files)
    out = []
    for p in files:
        sr, x = gs.wav(p)
        x = x[max(0, onset(x) - 16):]
        nn = gs.name_note(p.name)
        if name == "TIMPANI":
            root = hz_to_midi(yin(x[int(TIMPANI_AT[0] * sr):int(TIMPANI_AT[1] * sr)], sr, fmax=400, fmin=40))
        elif nn and name != "ORGAN":
            root = (nn[0] + conv) * 12 + nn[1]
        else:
            root = hz_to_midi(detect_hz(x, sr))
        x = resample(x, sr, sr_to)[:int(keep * sr_to)]
        if kind == "sus":
            x, loop = sus_loop(x, sr_to)
        else:
            x, loop = fade_out(x, 0.08 if keep < 0.6 else 0.15, sr_to), None
        out.append((norm(x), loop, root))
    return out


# --------------------------------------------------------------------------------------------- CHOIR --- #
# "ah": formant centre (Hz), bandwidth (Hz), level (dB); men and women
VOWEL_M = ((650, 80, 0), (1080, 90, -6), (2650, 120, -7), (2900, 130, -8), (3250, 140, -22))
VOWEL_F = ((800, 80, 0), (1150, 90, -4), (2800, 120, -20), (3500, 130, -36), (4950, 140, -60))


def envelope(f, vowel):
    """the vowel's spectral envelope at frequencies f (array)"""
    h = np.zeros_like(f)
    for fc, bw, db in vowel:
        h += 10 ** (db / 20) / np.sqrt(1 + ((f - fc) / (bw / 2)) ** 2)
    return h


def choir_note(root, keep, sr, seed):
    rng = random.Random(seed)
    nrng = np.random.default_rng(seed)
    n = int(keep * sr)
    t = np.arange(n) / sr
    f0 = 440 * 2 ** ((root - 69) / 12)
    vowel = VOWEL_M if root < 55 else VOWEL_F
    out = np.zeros(n)
    for _ in range(8):
        cents = rng.uniform(-11, 11)
        vr, vd, vp = rng.uniform(4.7, 5.8), rng.uniform(10, 22), rng.uniform(0, 2 * math.pi)
        wr, wd = rng.uniform(0.3, 0.9), rng.uniform(2, 5)                 # a slow wander of the pitch
        dev = cents + vd * np.minimum(1, t / 0.25) * np.sin(2 * math.pi * vr * t + vp) + \
            wd * np.sin(2 * math.pi * wr * t + rng.uniform(0, 6.3))
        f = f0 * 2 ** (dev / 1200)
        ph = 2 * math.pi * np.cumsum(f) / sr + rng.uniform(0, 6.3)
        amp = 1 + 0.12 * np.sin(2 * math.pi * rng.uniform(0.2, 0.7) * t + rng.uniform(0, 6.3))
        k = 1
        while k * f0 < 0.42 * sr:
            out += amp * envelope(np.array([k * f0]), vowel)[0] * k ** -0.6 * np.sin(k * ph)
            k += 1
    # the breath: white noise shaped by the vowel, a little
    spec = np.fft.rfft(nrng.standard_normal(n))
    spec *= envelope(np.fft.rfftfreq(n, 1 / sr), vowel)
    breath = np.fft.irfft(spec, n)
    out += breath * (0.10 * np.abs(out).max() / (np.abs(breath).max() + 1e-9))
    out *= np.minimum(1, t / 0.07)                                       # the voices come in over 70 ms
    return list(out)


def choir(keep, sr):
    zones = []
    for k, root in enumerate(CHOIR_ROOTS):
        x, loop = sus_loop(choir_note(root, keep, sr, 1931 + k), sr, 0.20)
        zones.append((norm(x), loop, root))
    return zones


# ------------------------------------------------------------------------------------------ ORCH HIT --- #
HIT_GAIN = (1.0, 0.55, 0.85, 0.6, 0.9)          # trumpet C, trumpet C two octaves up, violins C, violins G, timpani


def orch_hit(keep, sr):
    files = sorted((LIB / "HIT").glob("*.wav"))
    n = int(keep * sr)
    mix = [0.0] * n
    for p, g in zip(files, HIT_GAIN):
        s, x = gs.wav(p)
        x = resample(x[max(0, onset(x) - 8):], s, sr)[:n]
        pk = peak(x)
        for i, v in enumerate(x):
            mix[i] += v * g / pk
    s, x = gs.wav(files[2])                     # the violins' C: the hit's root
    root = round(hz_to_midi(detect_hz(x[onset(x):], s)))
    for i in range(n):                          # a hit dies fast: an exponential fall after the first 60 ms
        tt = i / sr
        mix[i] *= 1.0 if tt < 0.06 else math.exp(-(tt - 0.06) / 0.16)
    return [(norm(fade_out(mix, 0.05, sr)), None, root)]


# ------------------------------------------------------------------------------------------- the header --- #
def build():
    blob, zones, sets, kinds = bytearray(), [], [], []
    for name, kind, keep in SETS:
        sr = RATE.get(name, TR)
        if kind == "choir":
            ent = choir(keep, sr)
        elif kind == "hit":
            ent = orch_hit(keep, sr)
        else:
            ent = recorded(name, kind, keep, sr)
        ent.sort(key=lambda e: e[2])
        z0 = len(zones)
        for (lo, hi), (s, loop, root) in zip(key_split([int(round(e[2])) for e in ent]), ent):
            ls, le = loop if loop else (len(s), len(s))
            enc, st = ima_encode(s, ls)
            zones.append(dict(off=len(blob), n=len(s), ls=ls, le=le, looped=bool(loop), sr=sr,
                              root16=int(round(root * 16)), pred=st[0], idx=st[1], lo=lo, hi=hi))
            blob += enc
            if len(blob) & 1:
                blob.append(0)
        sets.append((name, z0, len(ent)))
        kinds.append(kind)
    return blob, zones, sets


def header(blob, zones, sets):
    L = ["/* generated by tools/gen_ditto_samples.py: Ditto's sound library (IMA ADPCM, eng_sample.c's zones) */",
         "#pragma once", "#include <stdint.h>", "",
         f"static const uint8_t DT_DATA[{len(blob)}] = {{"]
    for i in range(0, len(blob), 32):
        L.append("    " + ",".join(map(str, blob[i:i + 32])) + ",")
    L.append("};")
    L.append("static const smp_zone_t DT_ZONES[] = {")
    for e in zones:
        rate = int(round(e["sr"] / 44100 * 65536))
        L.append(f"    {{{e['off']}, {e['n']}, {e['ls']}, {e['le']}, {rate}, {e['root16']}, {e['pred']}, "
                 f"{e['idx']}, {e['lo']}, {e['hi']}, {1 if e['looped'] else 0}}},")
    L.append("};")
    L.append(f"#define DT_GRID {GRID}")
    L.append(f"/* each zone's decoder state at k * n / {GRID} (predictor & 0xFFFF | index << 16): START */")
    L.append(f"static const uint32_t DT_ZGRID[][{GRID}] = {{")
    for e in zones:
        data = blob[e["off"]:e["off"] + (e["n"] + 1) // 2]
        st = gs.ima_states(data, [k * e["n"] // GRID for k in range(GRID)])
        L.append("    {" + ", ".join(map(str, st)) + "},")
    L.append("};")
    L.append("static const smp_set_t DT_SETS[] = {")
    for name, z0, nz in sets:
        L.append(f'    {{"{name}", {z0}, {nz}}},')
    L.append("};")
    L.append(f"#define DT_NSETS {len(sets)}")
    L.append("enum { " + ", ".join("DT_S_" + n.replace(" ", "_") for n, _, _ in sets) + " };   /* SOUND A's numbers */")
    L.append("#define DT_SET_NAMES_INIT " + ", ".join(f'"{n}"' for n, _, _ in sets))
    return "\n".join(L) + "\n"


def summary(blob, zones, sets):
    secs = sum(z["n"] / z["sr"] for z in zones)
    return f"ditto sounds: {len(sets)} sets, {len(zones)} zones, {secs:.1f} s, {len(blob)} B ADPCM"


def input_key():
    h = hashlib.sha256()
    here = Path(__file__).resolve().parent
    for p in (here / "gen_ditto_samples.py", here / "gen_samples.py", here / "sampleio.py"):
        h.update(p.read_bytes())
    h.update(repr((sys.version_info[:2], np.__version__)).encode())
    for p in sorted(LIB.glob("*/*.wav")):
        h.update(str(p.relative_to(SRC)).encode() + b"\0")
        h.update(hashlib.sha256(p.read_bytes()).digest())
    return h.hexdigest()


def main(out):
    if not any(LIB.glob("*/*.wav")):
        sys.exit("ditto sounds: assets/samples-eii/ is empty (tools/fetch_eii.py)")
    key = input_key()
    try:
        ck, summ, text = CACHE.read_text().split("\n", 2)
        if ck == key:
            Path(out).write_text(text)
            print(summ + " (cached)")
            return
    except (OSError, ValueError):
        pass
    blob, zones, sets = build()
    text = header(blob, zones, sets)
    Path(out).parent.mkdir(parents=True, exist_ok=True)
    Path(out).write_text(text)
    summ = summary(blob, zones, sets)
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    CACHE.write_text(f"{key}\n{summ}\n{text}")
    print(summ)
    for name, z0, nz in sets:
        print(f"  {name:9s}", " ".join(f"{z['root16'] / 16:5.1f}[{z['lo']}-{z['hi']}]{'L' if z['looped'] else ''}"
                                       f" {z['n'] / z['sr']:.2f}s" for z in zones[z0:z0 + nz]))


if __name__ == "__main__":
    main(sys.argv[1])
