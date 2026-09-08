#!/bin/bash
# Launch ares with a ROM, capture only the ares window at the given seconds
# after launch, then quit ares. Needs a logged-in macOS GUI session.
# Usage: scripts/ares-shot.sh <rom.z64> <outdir> <seconds>...
set -euo pipefail
if [ $# -lt 3 ]; then echo "usage: $0 <rom.z64> <outdir> <seconds>..." >&2; exit 2; fi
ROM="$1"; OUT="$2"; shift 2
[ -f "$ROM" ] || { echo "no such ROM: $ROM" >&2; exit 1; }
mkdir -p "$OUT"
ROM_ABS="$(cd "$(dirname "$ROM")" && pwd)/$(basename "$ROM")"

# Find the largest normal-layer window owned by ares via CoreGraphics.
read -r -d '' JS <<'JSEOF' || true
ObjC.import("CoreGraphics");
const ref = $.CGWindowListCopyWindowInfo($.kCGWindowListOptionOnScreenOnly | $.kCGWindowListExcludeDesktopElements, 0);
const list = ObjC.deepUnwrap(ObjC.castRefToObject(ref));
const rows = list.filter(d => d.kCGWindowOwnerName === "ares" && d.kCGWindowLayer === 0)
                 .sort((a, b) => b.kCGWindowBounds.Width - a.kCGWindowBounds.Width);
rows.length ? String(rows[0].kCGWindowNumber) : "";
JSEOF

pkill -x ares 2>/dev/null || true
sleep 0.5
open -a ares --args --system "Nintendo 64" "$ROM_ABS"
START=$(date +%s)
for SEC in "$@"; do
  while [ $(( $(date +%s) - START )) -lt "$SEC" ]; do sleep 0.2; done
  WID=$(osascript -l JavaScript -e "$JS")
  if [ -z "$WID" ]; then
    echo "ares window not found at ${SEC}s" >&2
    pkill -x ares 2>/dev/null || true
    exit 1
  fi
  screencapture -x -o -l "$WID" "$OUT/shot-${SEC}s.png"
  echo "captured $OUT/shot-${SEC}s.png"
done
pkill -x ares 2>/dev/null || true
