#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Ditto in the browser without Emscripten: the firmware's sources (web/emu/felucca_web.c) compiled with clang's
# wasm32 target against the few C library declarations in web/emu/wasm-libc, then one self-contained page.
#   web/emu/build_wasm.sh   ->  build/emu/felucca.wasm, build/emu/ditto.html
# Run after one ./build.sh (or after tools/build.py's header generation: build/gen).
set -e
cd "$(dirname "$0")/../.."
[ -f build/gen/felucca_tables.h ] || { echo "wasm: build/gen is missing (run ./build.sh once)"; exit 1; }
VERSION=$(sed -n 's/^#define FELUCCA_VERSION "\([^"]*\)".*/\1/p' firmware/src/felucca.c)
mkdir -p build/emu
clang --target=wasm32 -O2 -ffp-contract=off -std=gnu99 -w -nostdlib -isystem web/emu/wasm-libc \
    -fvisibility=hidden -ffunction-sections -fdata-sections "-DFELUCCA_VERSION=\"$VERSION\"" -Ibuild/gen -Ifirmware/src \
    web/emu/felucca_web.c web/emu/wasm-libc/libc_min.c \
    -Wl,--no-entry -Wl,--export-dynamic -Wl,--gc-sections -Wl,--allow-undefined \
    -Wl,-z,stack-size=1048576 -Wl,--initial-memory=33554432 -o build/emu/felucca.wasm
python3 web/emu/make_ditto_page.py build/emu/felucca.wasm build/emu/ditto.html
