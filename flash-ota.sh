#!/usr/bin/env bash
# ══════════════════════════════════════════════════════════════
#  flash-ota.sh — build a sketch and push it over the air.
#
#  Usage:   ./flash-ota.sh <sketch> <host>
#
#  Your Mac must be on the togalights WLAN (the controller hosts it).
#  e.g.     ./flash-ota.sh module toga-mod-7d75c0.local
#           ./flash-ota.sh webserver toga.local          # always OTA-able
#           ./flash-ota.sh grid toga-grid.local
#
#  No trigger, no OTA mode: the controller's AP is pinned to TOGA_CHANNEL, so
#  every node is on the network AND on the ESP-NOW bus at the same time and is
#  flashable mid-show. The controller has to be powered for the AP to exist.
#
#  Two things that will bite you once:
#   · espota opens a listener HERE and the ESP connects BACK. macOS asks once
#     whether to allow incoming connections for python3 — say yes, or this
#     hangs at "Sending invitation".
#   · A board must have been USB-flashed with min_spiffs at least once. You
#     cannot OTA your way onto a new partition table; huge_app has no second
#     app slot to land in.
# ══════════════════════════════════════════════════════════════
set -euo pipefail

SKETCH="${1:?usage: flash-ota.sh <sketch> <host>}"
HOST="${2:?usage: flash-ota.sh <sketch> <host>}"

cd "$(dirname "$0")"
CORE=~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.17
ESPOTA="$CORE/tools/espota.py"
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
BUILD="/tmp/toga-build/$SKETCH"
OTA_PASS="toga-ota"        # must match TOGA_OTA_PASS in libraries/TogaProto/src/toga_ota.h

echo "── building $SKETCH ──"
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --build-path "$BUILD" "$SKETCH/"

echo "── pushing to $HOST ──"
python3 "$ESPOTA" -r -i "$HOST" -p 3232 --auth="$OTA_PASS" -f "$BUILD/$SKETCH.ino.bin"

# Keep what you shipped: a bad-but-booting image marks itself valid and rollback
# will NOT save you — being able to diff against the last good binary will.
mkdir -p .ota-archive
cp "$BUILD/$SKETCH.ino.bin" ".ota-archive/$SKETCH-$(git rev-parse --short HEAD)$(git diff --quiet || echo -dirty).bin"
echo "── done; archived under .ota-archive/ ──"
