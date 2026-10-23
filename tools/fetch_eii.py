#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Gadlele
"""Download the CC0 recordings Ditto's sound library is built from into assets/samples-eii/.

The files are already in the tree; this re-creates them from the source:
  https://github.com/sgossner/VSCO-2-CE  (CC0 1.0)
  https://github.com/sgossner/VCSL       (CC0 1.0)
Only the first RANGE bytes of each WAV are downloaded (the library keeps at most ~1.2 s of a note).
The paths are written out in full (no GitHub API listing). CHOIR has no recording: tools/gen_ditto_samples.py
synthesizes it; ORCH HIT is mixed there from the HIT folder's recordings.

  tools/fetch_eii.py            fetch what is missing, rewrite ATTRIBUTION.txt
"""
import sys
import urllib.parse
import urllib.request
from pathlib import Path

DEST = Path(__file__).resolve().parents[1] / "assets" / "samples-eii"
RANGE = 420_000

VCSL = "VCSL"
VSCO = "VSCO-2-CE"
PNO = "Chordophones/Zithers/Grand Piano, Kawai/Sustains/GPiano_sus_{}_v2_rr1_Player.wav"
# set: [(repo, path)], in the order of the zones (low to high); file names keep the note (C4 = 60 or C3 = 60,
# tools/gen_samples.py octave_conv decides per set)
PICK = {
    "PIANO": [(VCSL, PNO.format(n)) for n in ("C2", "C3", "C4", "C5")],
    "STRINGS": [(VSCO, f"Strings/Violin Section/susVib/VlnEns_susVib_{n}_v1.wav") for n in ("A2", "A3", "B4")],
    "BRASS": [(VSCO, f"Brass/Trumpet/sus/Sum_SHTrumpet_sus_{n}_v3_rr1.wav") for n in ("C3", "A#3", "A4")],
    "FLUTE": [(VSCO, f"Woodwinds/Flute/susvib/LDFlute_susvib_{n}_v1_1.wav") for n in ("C4", "C5", "C6")],
    "CELLO": [(VSCO, f"Strings/Cello Section/susvib/susvib_{n}_v1_1.wav") for n in ("C1", "C3")],
    "ORGAN": [(VSCO, f"Keys/Organ/Quiet/NT5_Man3Quiet_{n}_rr1.wav") for n in ("128", "146", "164")],
    "HARPSI": [(VCSL, f"Chordophones/Zithers/Harpsichord, English/Sustains/Normal/"
                      f"ZuckermannKitHarpsi_Normal_Sus_{n}_rr1.wav") for n in ("C2", "C3", "C4")],
    "MARIMBA": [(VSCO, f"Percussion/Marimba/Marimba_hit_Outrigger_{n}_loud_01.wav") for n in ("C2", "C4", "C6")],
    "BELLS": [(VCSL, "Idiophones/Struck Idiophones/Tubular Bells 1/chimes_C3_f_rr1.wav"),
              (VCSL, "Idiophones/Struck Idiophones/Tubular Bells 1/chimes_C4_ff_rr2.wav")],
    "PIZZ": [(VSCO, f"Strings/Violin Section/Pizz/VlnEns_Pizz_{n}_v2_rr1.wav") for n in ("A2", "A3", "B4")],
    "BASS": [(VSCO, f"Strings/Solo Contrabass/Pizz/BKCtbss_Pizz_{n}_rr1.wav") for n in ("E0_v3", "E1_v3", "E2_v1")],
    "TIMPANI": [(VSCO, "Percussion/Timpani/Timpani3_Hit_v4_rr1_Sum.wav")],
    "RECORDER": [(VCSL, f"Aerophones/Edge-blown Aerophones/Baroque Bass Recorder/SusVib/"
                        f"BassRecorder_SusVib_{n}_rr1_Main.wav") for n in ("G2", "C4")],
    "HARP": [(VSCO, f"Strings/Harp/KSHarp_{n}_mf.wav") for n in ("A2", "C5")],
    # ORCH HIT's parts (tools/gen_ditto_samples.py orch_hit): two trumpets in octaves, the violins' spiccato on the
    # root and the fifth below, a timpani stroke
    "HIT": [(VSCO, "Brass/Trumpet/stac/Sum_SHTrumpet_stac_C3_v3_rr1.wav"),
            (VSCO, "Brass/Trumpet/stac/Sum_SHTrumpet_stac_C5_v3_rr1.wav"),
            (VSCO, "Strings/Violin Section/Spic/VlnEns_Spic_C4_v2_rr1.wav"),
            (VSCO, "Strings/Violin Section/Spic/VlnEns_Spic_G2_v2_rr1.wav"),
            (VSCO, "Percussion/Timpani/Timpani2_Hit_v4_rr1_Sum.wav")],
}


def fetch(repo, path, dest):
    if dest.exists():
        return 0
    url = f"https://raw.githubusercontent.com/sgossner/{repo}/master/" + urllib.parse.quote(path)
    req = urllib.request.Request(url, headers={"Range": f"bytes=0-{RANGE - 1}"})
    with urllib.request.urlopen(req) as r:
        data = r.read()
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_bytes(data)
    return len(data)


def main():
    total, credits = 0, []
    for name, files in PICK.items():
        for k, (repo, path) in enumerate(files):
            fn = f"{k:02d}_{path.rsplit('/', 1)[1]}"
            try:
                total += fetch(repo, path, DEST / name / fn)
            except OSError as e:
                sys.exit(f"{name}: {path}: {e}")
            credits.append(f"{name}/{fn}  <-  sgossner/{repo}: {path}")
    (DEST / "ATTRIBUTION.txt").write_text(
        "Ditto's sound library - source material\n"
        "Retrieved: 2026-10-23 (first ~400 KB of each file)\n"
        "Licence: CC0 1.0 Universal (public domain dedication)\n"
        "  https://creativecommons.org/publicdomain/zero/1.0/\n"
        "Sources: Versilian Studios (Sam Gossner)\n"
        "  VSCO-2 Community Edition  https://github.com/sgossner/VSCO-2-CE  (repository licence: CC0-1.0)\n"
        "  VCSL                      https://github.com/sgossner/VCSL       (repository licence: CC0-1.0)\n"
        "CHOIR is synthesized by tools/gen_ditto_samples.py (no recording); ORCH HIT is mixed there from HIT/.\n\n"
        + "\n".join(credits) + "\n")
    print(f"samples-eii: {len(credits)} files, {total} B downloaded -> {DEST}")


if __name__ == "__main__":
    main()
