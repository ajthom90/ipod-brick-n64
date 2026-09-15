#!/bin/bash
set -euo pipefail
if [ $# -lt 3 ]; then echo "usage: $0 <rom.z64> <outdir> <seconds>..." >&2; exit 2; fi
ROM="$1"; OUT="$2"; shift 2
[ -f "$ROM" ] || { echo "no such ROM: $ROM" >&2; exit 1; }
mkdir -p "$OUT"
ROM_ABS="$(cd "$(dirname "$ROM")" && pwd)/$(basename "$ROM")"
ROOT="build/hle"
mkdir -p "$ROOT/cfg"
CORE="$ROOT/mupen64plus-core/projects/unix/libmupen64plus.dylib"
GFX="$ROOT/GLideN64/build/plugin/Release/mupen64plus-video-GLideN64.dylib"
[ -f "$CORE" ] && [ -f "$GFX" ] || { echo "rig missing; run scripts/build-hle-rig.sh" >&2; exit 1; }
read -r -d '' JS <<'JSEOF' || true
ObjC.import("CoreGraphics");
const ref = $.CGWindowListCopyWindowInfo($.kCGWindowListOptionOnScreenOnly | $.kCGWindowListExcludeDesktopElements, 0);
const list = ObjC.deepUnwrap(ObjC.castRefToObject(ref));
const rows = list.filter(d => (d.kCGWindowOwnerName||"").toLowerCase().indexOf("mupen") >= 0 && d.kCGWindowLayer === 0)
                 .sort((a,b) => b.kCGWindowBounds.Width - a.kCGWindowBounds.Width);
rows.length ? String(rows[0].kCGWindowNumber) : "";
JSEOF
pkill -f "mupen64plus --corelib" 2>/dev/null || true
sleep 0.5
mupen64plus --corelib "$CORE" --emumode 0 --windowed --resolution 640x480 --noosd \
  --configdir "$ROOT/cfg" --datadir "$ROOT/data" --gfx "$GFX" --rsp dummy --audio dummy "$ROM_ABS" \
  > "$OUT/run.log" 2>&1 &
START=$(date +%s)
for SEC in "$@"; do
  while [ $(( $(date +%s) - START )) -lt "$SEC" ]; do sleep 0.2; done
  WID=$(osascript -l JavaScript -e "$JS")
  if [ -z "$WID" ]; then echo "mupen window not found at ${SEC}s" >&2; pkill -f "mupen64plus --corelib" 2>/dev/null || true; exit 1; fi
  if ! screencapture -x -o -l "$WID" "$OUT/shot-${SEC}s.png"; then
    echo "capture failed at ${SEC}s (Screen Recording permission?)" >&2; pkill -f "mupen64plus --corelib" 2>/dev/null || true; exit 1
  fi
  echo "captured $OUT/shot-${SEC}s.png"
done
pkill -f "mupen64plus --corelib" 2>/dev/null || true
