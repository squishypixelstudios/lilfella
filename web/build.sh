#!/bin/sh
# Builds game.wasm and the single-file playable page web/lil-fella.html (needs wasi-sdk 24; set SDK).
SDK=${SDK:-$HOME/wasi/wasi-sdk-24.0-x86_64-linux}
G=../src/game
cd "$(dirname "$0")"
$SDK/bin/clang++ --sysroot=$SDK/share/wasi-sysroot --target=wasm32-wasi -std=c++17 -Os -fno-exceptions -fno-rtti \
  -mexec-model=reactor -Wl,--no-entry -Wl,--gc-sections -Wl,--strip-all -w -o game.wasm web_main.cpp \
  $G/lcd.cpp $G/sprites_data.cpp $G/breeds.cpp $G/pet.cpp $G/save.cpp $G/platform.cpp $G/motion.cpp $G/game.cpp $G/view.cpp $G/pages.cpp $G/eatfx.cpp $G/yeet.cpp $G/synth.cpp || exit 1
python3 - <<'PY'
import base64
t=open('template.html').read()
w=base64.b64encode(open('game.wasm','rb').read()).decode()
open('lil-fella.html','w').write(t.replace('__WASM__',w))
PY
ls -la game.wasm lil-fella.html
rm -f game.wasm
