#!/bin/sh
# Builds game.wasm and the single-file playable page web/lil-fella.html (needs wasi-sdk 24; set SDK).
# The game's source files are listed once, in ../sim/sources.txt (the simulator's Makefile reads it too).
SDK=${SDK:-$HOME/wasi/wasi-sdk-24.0-x86_64-linux}
G=../src/game
cd "$(dirname "$0")"
SRC=$(for f in $(cat ../sim/sources.txt); do printf '%s ' "$G/$f"; done)
LF_VER=$(sed -n 's/.*LF_VERSION = "\(.*\)";.*/\1/p' $G/game.h); export LF_VER
$SDK/bin/clang++ --sysroot=$SDK/share/wasi-sysroot --target=wasm32-wasi -std=c++17 -Os -fno-exceptions -fno-rtti \
  -mexec-model=reactor -Wl,--no-entry -Wl,--gc-sections -Wl,--strip-all -w -o game.wasm web_main.cpp $SRC || exit 1
python3 - <<'PY'
import base64, os
t=open('template.html').read().replace('__VERSION__', os.environ['LF_VER'])
w=base64.b64encode(open('game.wasm','rb').read()).decode()
open('lil-fella.html','w').write(t.replace('__WASM__',w))
PY
ls -la game.wasm lil-fella.html
rm -f game.wasm
