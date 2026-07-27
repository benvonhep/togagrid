#!/usr/bin/env bash
#
# TOGA — Flash-Skript
# Kompiliert einen Sketch und lädt ihn per USB auf ein Board.
#
#   ./flash.sh <ziel> [port]
#
#   <ziel>  = grid | controller | module
#   [port]  = optional, z. B. /dev/cu.usbserial-0001
#             Fehlt der Port, wird das einzige angeschlossene USB-Board benutzt.
#
# Beispiele:
#   ./flash.sh controller
#   ./flash.sh grid /dev/cu.usbserial-0001
#
set -euo pipefail

FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
# Hängt der Upload bei 921600, die nächste Zeile einkommentieren:
# FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs,UploadSpeed=115200"

# ── Argumente ──
TARGET="${1:-}"
PORT="${2:-}"

case "$TARGET" in
  grid|controller|module) ;;
  *)
    echo "Verwendung: ./flash.sh <grid|controller|module> [port]"
    exit 1
    ;;
esac

# ── ins Projektverzeichnis wechseln (egal von wo aufgerufen) ──
cd "$(dirname "$0")"

if [[ ! -d "$TARGET" ]]; then
  echo "FEHLER: Ordner '$TARGET/' nicht gefunden. Skript im Projekt-Hauptordner ausführen."
  exit 1
fi

# ── Port automatisch finden, falls nicht angegeben ──
if [[ -z "$PORT" ]]; then
  PORTS=$(arduino-cli board list 2>/dev/null | grep -Eo '/dev/(cu\.usb(serial|modem)[^ ]*|ttyUSB[0-9]+|ttyACM[0-9]+)' || true)
  COUNT=$(printf '%s\n' "$PORTS" | grep -c . || true)
  if [[ "$COUNT" -eq 0 ]]; then
    echo "FEHLER: Kein USB-Board gefunden. Board anstecken, oder Port angeben:"
    echo "  ./flash.sh $TARGET /dev/cu.usbserial-XXXX"
    exit 1
  elif [[ "$COUNT" -gt 1 ]]; then
    echo "Mehrere Boards gefunden — bitte Port angeben:"
    printf '%s\n' "$PORTS"
    echo "  ./flash.sh $TARGET <einer davon>"
    exit 1
  fi
  PORT="$PORTS"
  echo "Port automatisch erkannt: $PORT"
fi

# ── Flashen ──
echo "== Flashe '$TARGET' auf $PORT =="
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --upload -p "$PORT" "$TARGET/"

echo
echo "Fertig. Bei Erfolg stand oben 'Hash of data verified'."
