#!/usr/bin/env bash
#
# TOGA — Setup-Skript
# Installiert ESP32-Core und alle Bibliotheken in den EXAKT richtigen Versionen.
# Einmal ausführen, danach kann mit ./flash.sh geflasht werden.
#
#   ./install.sh
#
set -euo pipefail

# ── Gepinnte Versionen (NICHT upgraden — neuere brechen ESP-NOW/Controller) ──
CORE_VERSION="esp32:esp32@2.0.17"
ESP32_URL="https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json"

# name@version — Leerzeichen in Namen sind ok (ganzer String wird zitiert)
LIBS=(
  "FastLED@3.6.0"
  "Adafruit seesaw Library@1.7.9"
  "Adafruit GFX Library@1.12.6"
  "Adafruit BusIO@1.17.4"
  "Adafruit ST7735 and ST7789 Library@1.11.0"
  "SD@1.3.0"
)

echo "== TOGA Setup =="

# ── arduino-cli vorhanden? ──
if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "FEHLER: arduino-cli ist nicht installiert."
  echo
  if [[ "$(uname)" == "Darwin" ]]; then
    echo "  macOS:  brew install arduino-cli"
  fi
  echo "  Sonst:  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh"
  echo
  echo "Danach dieses Skript erneut ausführen."
  exit 1
fi
echo "arduino-cli: $(arduino-cli version)"

# ── ESP32-Core ──
echo
echo "== ESP32-Core ${CORE_VERSION} =="
arduino-cli config init --overwrite >/dev/null 2>&1 || true
arduino-cli config add board_manager.additional_urls "$ESP32_URL"
arduino-cli core update-index
arduino-cli core install "$CORE_VERSION"

# ── Bibliotheken ──
echo
echo "== Bibliotheken =="
for lib in "${LIBS[@]}"; do
  echo "-> $lib"
  arduino-cli lib install "$lib"
done

# ── Kontrolle ──
echo
echo "== Installiert =="
arduino-cli core list
echo
arduino-cli lib list

echo
echo "Fertig. Jetzt flashen mit:   ./flash.sh controller"
echo "(TogaProto liegt im Projekt unter libraries/ und wird automatisch mitgenommen.)"
