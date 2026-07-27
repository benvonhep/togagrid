@echo off
REM ============================================================
REM  TOGA - Flash-Skript (Windows)
REM  Kompiliert einen Sketch und laedt ihn per USB aufs Board.
REM
REM    flash.bat <ziel> [port]
REM
REM    <ziel>  = grid | controller | module
REM    [port]  = optional, z. B. COM3
REM              Fehlt der Port, wird das einzige Board benutzt.
REM
REM  Beispiele:
REM    flash.bat controller
REM    flash.bat grid COM3
REM ============================================================
setlocal enabledelayedexpansion

set "FQBN=esp32:esp32:esp32:PartitionScheme=min_spiffs"
REM Haengt der Upload bei 921600, naechste Zeile aktivieren (REM davor entfernen):
REM set "FQBN=esp32:esp32:esp32:PartitionScheme=min_spiffs,UploadSpeed=115200"

set "TARGET=%~1"
set "PORT=%~2"

if /i "%TARGET%"=="grid"       goto ok
if /i "%TARGET%"=="controller" goto ok
if /i "%TARGET%"=="module"     goto ok
echo Verwendung: flash.bat ^<grid^|controller^|module^> [port]
pause
exit /b 1
:ok

REM -- ins Projektverzeichnis wechseln (egal von wo aufgerufen) --
cd /d "%~dp0"

if not exist "%TARGET%\" (
  echo FEHLER: Ordner '%TARGET%\' nicht gefunden. Skript im Projekt-Hauptordner ausfuehren.
  pause
  exit /b 1
)

REM -- Port automatisch finden, falls nicht angegeben --
if not "%PORT%"=="" goto doflash
for /f "tokens=1" %%P in ('arduino-cli board list ^| findstr /R /C:"COM[0-9]"') do set "PORT=%%P"
if "%PORT%"=="" (
  echo FEHLER: Kein Board gefunden. Board anstecken, oder Port angeben:
  echo   flash.bat %TARGET% COM3
  echo.
  echo Taucht kein COM-Port auf? Dann fehlt evtl. der USB-Treiber
  echo   ( CP210x: Silicon Labs  /  CH340: WCH ) - siehe SETUP.md.
  pause
  exit /b 1
)
echo Port automatisch erkannt: %PORT%
:doflash

echo == Flashe '%TARGET%' auf %PORT% ==
arduino-cli compile --fqbn "%FQBN%" --libraries ./libraries --upload -p %PORT% %TARGET%

echo.
echo Fertig. Bei Erfolg stand oben 'Hash of data verified'.
pause
