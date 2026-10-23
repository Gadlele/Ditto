#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Gadlele
# The website's pictures, from the firmware (tools/site_shots.c): web/site/img's gallery PNGs, splash.gif and icon.png.
#   tools/site_shots.sh            (after ./build.sh: build/gen; needs Pillow)
# device.png (the emulator's page with a note held) is a screenshot of build/emu/ditto.html, taken by hand.
set -e
cd "$(dirname "$0")/.."
OUT=build/site_shots
mkdir -p "$OUT" build/host
${CC:-cc} -UGOLANA -DGOLANA=1 -DGSP_NO_VERSION=1 -O1 -w -Ibuild/gen -Ifirmware/src -o build/host/site_shots tools/site_shots.c -lm
build/host/site_shots "$OUT"
python3 - "$OUT" web/site/img <<'PY'
import sys
from pathlib import Path
from PIL import Image
src, dst = Path(sys.argv[1]), Path(sys.argv[2])
for n in ("home_notes", "edit", "sndb", "mixer", "scl", "env", "lfo", "patch", "fx", "start", "save"):
    Image.open(src / f"{n}.ppm").convert("RGBA").save(dst / f"{n}.png")
# the power-on (without the version) at twice its size (the screen's pixels kept), its last frame held 3 s so the name can be read
frames = [Image.open(p).resize((480, 480), Image.NEAREST).convert("P", palette=Image.ADAPTIVE, colors=64)
          for p in sorted(src.glob("splash_*.ppm"))]
frames[0].save(dst / "splash.gif", save_all=True, append_images=frames[1:], duration=[50] * (len(frames) - 1) + [3000], loop=0)
last = Image.open(sorted(src.glob("splash_*.ppm"))[-1])            # the icon: the record, from the last frame
last.crop((120 - 66, 82 - 66, 120 + 66, 82 + 66)).resize((64, 64), Image.LANCZOS).save(dst / "icon.png")
print(f"site images: {dst}")
PY
