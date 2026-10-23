#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Ditto for Android: the browser emulator inside a WebView app (no FM-1 needed; Web MIDI is not available in a WebView).
#   ./build.sh once (the generated tables in build/gen), then
#   sh android/build_apk.sh      (needs clang, the Android SDK (ANDROID_HOME), JDK 17+ and Gradle)
#                                ->  android/app/build/outputs/apk/debug/app-debug.apk
set -e
cd "$(dirname "$0")/.."
[ -f build/emu/ditto.html ] || sh web/emu/build_wasm.sh
rm -rf android/app/src/main/assets/emu
mkdir -p android/app/src/main/assets/emu
python3 - build/emu/ditto.html android/app/src/main/assets/emu/index.html <<'PY'
import sys
s = open(sys.argv[1], encoding="utf-8").read()   # (a page made for the Artifact host: give it a document)
open(sys.argv[2], "w", encoding="utf-8").write(
    '<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
    '<meta name="viewport" content="width=device-width, initial-scale=1">\n</head>\n<body>\n' + s + '\n</body>\n</html>\n')
PY
cd android
if [ -x ./gradlew ]; then ./gradlew assembleDebug; else gradle assembleDebug; fi
echo "apk: android/app/build/outputs/apk/debug/app-debug.apk"
