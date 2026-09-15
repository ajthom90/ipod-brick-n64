#!/bin/bash
set -euo pipefail
ROOT="build/hle"; mkdir -p "$ROOT/data" "$ROOT/cfg"
export PATH="$(brew --prefix)/bin:$PATH"
command -v pkg-config >/dev/null || brew install pkgconf
command -v cmake >/dev/null || brew install cmake
CORE="$ROOT/mupen64plus-core"
[ -d "$CORE" ] || git clone --depth 1 https://github.com/mupen64plus/mupen64plus-core.git "$CORE"
[ -f "$CORE/projects/unix/libmupen64plus.dylib" ] || ( cd "$CORE/projects/unix" && make all -j"$(sysctl -n hw.ncpu)" OSD=0 )
GL="$ROOT/GLideN64"
[ -d "$GL" ] || git clone --depth 1 https://github.com/gonetz/GLideN64.git "$GL"
[ -f "$GL/build/plugin/Release/mupen64plus-video-GLideN64.dylib" ] || ( mkdir -p "$GL/build" && cd "$GL/build" && cmake ../src -DMUPENPLUSAPI=On -DNOHQ=On -DVEC4_OPT=On -DCRC_OPT=On -DCMAKE_BUILD_TYPE=Release && make -j"$(sysctl -n hw.ncpu)" )
cp "$GL/ini/GLideN64.ini" "$GL/ini/GLideN64.custom.ini" "$ROOT/data/" 2>/dev/null || true
cp "$(brew --prefix)/share/mupen64plus/"* "$ROOT/data/" 2>/dev/null || true
echo "HLE rig ready in $ROOT"
