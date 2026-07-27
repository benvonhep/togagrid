#!/usr/bin/env bash
#
# TOGA — Flash-Skript
# Kompiliert einen Sketch und lädt ihn per USB auf ein Board.
#
#   ./flash.sh                 -> Menü (Grid / Controller / Modul / Alles)
#   ./flash.sh <ziel> [port]   -> direkt
#
#   <ziel>  = grid | controller | module | all
#   [port]  = optional, z. B. /dev/cu.usbserial-0001
#             Fehlt der Port, wird das einzige angeschlossene USB-Board benutzt.
#
# Beispiele:
#   ./flash.sh                 (fragt nach)
#   ./flash.sh grid
#   ./flash.sh controller /dev/cu.usbserial-0001
#
set -euo pipefail

FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
# Hängt der Upload bei 921600, die nächste Zeile aktivieren:
# FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs,UploadSpeed=115200"

# ── ins Projektverzeichnis wechseln (egal von wo aufgerufen) ──
cd "$(dirname "$0")"

# Flasht EIN Board. $1 = ziel, $2 = port (optional; sonst automatisch).
flash_one() {
  target="$1"
  port="${2:-}"
  if [ ! -d "$target" ]; then
    echo "FEHLER: Ordner '$target/' nicht gefunden. Skript im Projekt-Hauptordner ausführen."
    return 1
  fi
  if [ -z "$port" ]; then
    ports=$(arduino-cli board list 2>/dev/null | grep -Eo '/dev/(cu\.usb(serial|modem)[^ ]*|ttyUSB[0-9]+|ttyACM[0-9]+)' || true)
    count=$(printf '%s\n' "$ports" | grep -c . || true)
    if [ "$count" -eq 0 ]; then
      echo "FEHLER: Kein Board gefunden. Board anstecken (ggf. Treiber) und erneut versuchen."
      return 1
    elif [ "$count" -gt 1 ]; then
      echo "Mehrere Boards gefunden:"
      printf '  %s\n' $ports
      echo "Bitte Port explizit angeben:  ./flash.sh $target <port>"
      return 1
    fi
    port="$ports"
    echo "Port automatisch erkannt: $port"
  fi
  echo "== Flashe '$target' auf $port =="
  arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --upload -p "$port" "$target"
  echo "-> '$target' fertig (bei Erfolg stand oben 'Hash of data verified')."
}

TARGET="${1:-}"
PORT="${2:-}"

# ── Kein Ziel angegeben → Menü ──
if [ -z "$TARGET" ]; then
  echo "Was soll geflasht werden?"
  echo "  1) Grid"
  echo "  2) Controller"
  echo "  3) Modul"
  echo "  4) Alles nacheinander (je Board einzeln anstecken)"
  printf "Auswahl [1-4]: "
  read choice
  case "$choice" in
    1) TARGET="grid" ;;
    2) TARGET="controller" ;;
    3) TARGET="module" ;;
    4) TARGET="all" ;;
    *) echo "Ungültige Auswahl."; exit 1 ;;
  esac
fi

case "$TARGET" in
  grid|controller|module)
    flash_one "$TARGET" "$PORT"
    ;;
  all)
    for t in grid controller module; do
      echo
      echo "================================================"
      echo " Nächstes Board: $t"
      echo " Bitte NUR das $t-Board anstecken."
      printf " Enter zum Flashen (oder 's' + Enter zum Überspringen): "
      read go
      case "$go" in
        s|S) echo " -> $t übersprungen." ; continue ;;
      esac
      flash_one "$t" || { echo "Abbruch bei '$t'."; exit 1; }
    done
    echo
    echo "Alle gewählten Boards durch."
    ;;
  *)
    echo "Verwendung: ./flash.sh <grid|controller|module|all> [port]"
    exit 1
    ;;
esac
