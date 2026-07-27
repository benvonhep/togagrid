@echo off
REM ============================================================
REM  TOGA - Setup-Skript (Windows)
REM  Installiert ESP32-Core und alle Bibliotheken in den EXAKT
REM  richtigen Versionen. Einmal per Doppelklick oder im cmd
REM  ausfuehren. Danach mit flash.bat flashen.
REM ============================================================
setlocal

echo == TOGA Setup ==

REM -- arduino-cli vorhanden? --
where arduino-cli >nul 2>&1
if errorlevel 1 (
  echo.
  echo FEHLER: arduino-cli ist nicht installiert.
  echo.
  echo   Zip von https://arduino.github.io/arduino-cli/latest/installation/
  echo   herunterladen, entpacken und den Ordner in die PATH-Variable aufnehmen.
  echo   Dann ein NEUES Terminal-Fenster oeffnen.
  echo   (Details in SETUP.md, Schritt 1.)
  echo.
  echo Danach dieses Skript erneut ausfuehren.
  pause
  exit /b 1
)
arduino-cli version

REM -- ESP32-Core 2.0.17 --
echo.
echo == ESP32-Core 2.0.17 ==
arduino-cli config init --overwrite
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17

REM -- Bibliotheken (gepinnte Versionen - NICHT upgraden) --
echo.
echo == Bibliotheken ==
arduino-cli lib install FastLED@3.6.0
arduino-cli lib install "Adafruit seesaw Library@1.7.9"
arduino-cli lib install "Adafruit GFX Library@1.12.6"
arduino-cli lib install "Adafruit BusIO@1.17.4"
arduino-cli lib install "Adafruit ST7735 and ST7789 Library@1.11.0"
arduino-cli lib install SD@1.3.0

REM -- Kontrolle --
echo.
echo == Installiert ==
arduino-cli core list
echo.
arduino-cli lib list

echo.
echo Fertig. Jetzt flashen mit:   flash.bat controller
echo (TogaProto liegt im Projekt unter libraries\ und wird automatisch mitgenommen.)
pause
