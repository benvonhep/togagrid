# TOGA — Setup & Flash-Anleitung

Diese Anleitung bringt einen frischen Rechner so weit, dass du **Grid, Controller
und Module** flashen kannst. Einfach die Befehle der Reihe nach ins Terminal
kopieren.

> ⚠️ **Wichtig:** Die Versionen unten sind **fix** und dürfen **nicht** durch
> neuere ersetzt werden. Neuere FastLED- (3.10.x) oder ESP32-Core-Versionen (3.x)
> machen ESP-NOW und den Controller kaputt. Immer genau diese Versionen nehmen.

---

## Schnellstart (mit Skripten)

Für die Ungeduldigen — es gibt fertige Skripte, die alles automatisch machen.
Nur `arduino-cli` musst du vorher installiert haben (siehe [Schritt 1](#1-voraussetzung-arduino-cli)).

**Windows** (Doppelklick oder im `cmd`):
```bat
install.bat              :: Core + Bibliotheken in den richtigen Versionen installieren (einmalig)
flash.bat controller     :: Board anstecken, dann flashen (Port wird automatisch erkannt)
```

**macOS / Linux:**
```bash
./install.sh
./flash.sh controller
```

Das Flash-Skript versteht `grid`, `controller` und `module`. Ist genau ein Board
angesteckt, findet es den Port selbst; sonst Port angeben — Windows z. B.
`flash.bat grid COM3`, macOS/Linux `./flash.sh grid /dev/cu.usbserial-0001`.

Wer lieber alles von Hand macht oder verstehen will, was passiert, folgt einfach
den Schritten unten.

---

## 1. Voraussetzung: arduino-cli

**Windows** — offizieller Weg (klappt immer):
1. Auf <https://arduino.github.io/arduino-cli/latest/installation/> das
   **Windows-Zip** herunterladen.
2. Entpacken, z. B. nach `C:\arduino-cli\` (darin liegt `arduino-cli.exe`).
3. Diesen Ordner in die `PATH`-Umgebungsvariable aufnehmen
   (Start → „Umgebungsvariablen bearbeiten" → `Path` → Neu → `C:\arduino-cli`).
4. **Ein neues Terminal-Fenster öffnen**, damit `PATH` greift.

Alternativ mit einem Paketmanager, falls installiert:
`choco install arduino-cli` (Chocolatey) oder `scoop install arduino-cli` (Scoop).

**macOS** (mit [Homebrew](https://brew.sh)):
```bash
brew install arduino-cli
```

**Linux / anderes** (ohne Homebrew):
```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
# danach ggf. ./bin zum PATH hinzufügen, oder ./bin/arduino-cli benutzen
```

Prüfen, dass es läuft:
```bash
arduino-cli version
```

---

## 2. ESP32-Core installieren (exakt 2.0.17)

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17
```

---

## 3. Bibliotheken installieren (exakte Versionen)

```bash
arduino-cli lib install FastLED@3.6.0
arduino-cli lib install "Adafruit seesaw Library@1.7.9"
arduino-cli lib install "Adafruit GFX Library@1.12.6"
arduino-cli lib install "Adafruit BusIO@1.17.4"
arduino-cli lib install "Adafruit ST7735 and ST7789 Library@1.11.0"
arduino-cli lib install SD@1.3.0
```

> `TogaProto` (die projekteigene Bibliothek) muss **nicht** installiert werden —
> sie liegt im Ordner `libraries/` im Projekt und wird beim Flashen automatisch
> über `--libraries ./libraries` mitgenommen.

**Kontrolle** — sollte genau diese Versionen zeigen:
```bash
arduino-cli lib list
arduino-cli core list
```

---

## 4. Board anschließen & Port finden

Board per USB anstecken, dann:
```bash
arduino-cli board list
```

Suche die Zeile mit dem Board-Port:
- **Windows:** `COM3`, `COM4`, … (unter „Port")
- **macOS/Linux:** `/dev/cu.usbserial-0001` bzw. `usbmodem`/`ttyUSB`/`ttyACM`

**Dieser Port ist bei jedem Rechner/Kabel anders** — im Folgenden als `<PORT>`
bezeichnet.

> 🪟 **Windows: taucht kein COM-Port auf?** Dann fehlt der USB-Seriell-Treiber
> für den Chip auf dem Board. Je nach Board:
> - **CP2102 / CP210x** → „CP210x VCP Driver" von Silicon Labs
> - **CH340 / CH9102** → Treiber von WCH
>
> Nach der Treiber-Installation Board neu anstecken und `arduino-cli board list`
> erneut ausführen. (macOS/Linux brauchen den Treiber i. d. R. nicht.)

---

## 5. Flashen

Immer aus dem **Projekt-Hauptordner** ausführen (der mit den Ordnern `grid/`,
`controller/`, `module/`). `<PORT>` durch deinen echten Port ersetzen.

> 🪟 **Windows:** am einfachsten `flash.bat controller` benutzen (siehe Schnellstart).
> Von Hand im `cmd` geht es genauso, nur ohne das führende `./` und mit
> `set "FQBN=..."` statt `FQBN=...`.

```bash
# Einmal setzen (gilt für die aktuelle Terminal-Sitzung):
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"

# Grid flashen:
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --upload -p <PORT> grid/

# Controller flashen:
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --upload -p <PORT> controller/

# Modul flashen (für JEDES Modul-Board einzeln, jeweils anstecken & Port prüfen):
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --upload -p <PORT> module/
```

Erfolg erkennst du an `Hash of data verified` und `Hard resetting…` am Ende.

> 💡 **Wenn der Upload bei 921600 Baud hängt** (bleibt beim „Connecting…" stehen),
> hänge `,UploadSpeed=115200` ans FQBN:
> ```bash
> FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs,UploadSpeed=115200"
> ```

---

## 6. Wichtige Hinweise

- **Ganze Flotte gleich flashen.** Grid, Controller und alle Module müssen aus
  **demselben Projekt-Stand** geflasht sein. Ein Knoten mit anderer
  Protokoll-Version wird von den anderen gar nicht gehört.
- **Niemals `esptool erase_flash`.** Das löscht auch den NVS (gespeicherte
  Konfiguration wie Modulnamen, Kalibrierung). Der normale Upload lässt den NVS
  in Ruhe.
- **Kanal:** Grid, Controller und Module funken auf einem festen WLAN-Kanal.
  Hängt der Controller am Router und der funkt auf einem anderen Kanal, sieht er
  die Module nicht — dann Router auf den richtigen Kanal stellen (die Weboberfläche
  des Controllers zeigt eine Warnung, falls der Kanal abweicht).

---

## Kurz-Spickzettel

```bash
# Setup (einmalig)
brew install arduino-cli
arduino-cli config init
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17
arduino-cli lib install FastLED@3.6.0 "Adafruit seesaw Library@1.7.9" "Adafruit GFX Library@1.12.6" "Adafruit BusIO@1.17.4" "Adafruit ST7735 and ST7789 Library@1.11.0" SD@1.3.0

# Flashen (Port anpassen)
arduino-cli board list
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --upload -p <PORT> controller/
```
