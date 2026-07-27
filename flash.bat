@echo off
REM ============================================================
REM  TOGA - Flash-Skript (Windows)
REM  Kompiliert einen Sketch und laedt ihn per USB aufs Board.
REM
REM    flash.bat                 -> Menue (Grid / Controller / Modul / Alles)
REM    flash.bat <ziel> [port]   -> direkt
REM
REM    <ziel>  = grid | controller | module | all
REM    [port]  = optional, z. B. COM3
REM
REM  Beispiele:
REM    flash.bat                 (fragt nach)
REM    flash.bat grid
REM    flash.bat controller COM3
REM ============================================================
setlocal enabledelayedexpansion

set "FQBN=esp32:esp32:esp32:PartitionScheme=min_spiffs"
REM Haengt der Upload bei 921600, naechste Zeile aktivieren (REM davor entfernen):
REM set "FQBN=esp32:esp32:esp32:PartitionScheme=min_spiffs,UploadSpeed=115200"

cd /d "%~dp0"

set "TARGET=%~1"
set "PORT=%~2"

REM -- Kein Ziel angegeben -> Menue --
if not "%TARGET%"=="" goto have_target
echo Was soll geflasht werden?
echo   1^) Grid
echo   2^) Controller
echo   3^) Modul
echo   4^) Alles nacheinander (je Board einzeln anstecken)
set /p CHOICE=Auswahl [1-4]:
if "%CHOICE%"=="1" set "TARGET=grid"
if "%CHOICE%"=="2" set "TARGET=controller"
if "%CHOICE%"=="3" set "TARGET=module"
if "%CHOICE%"=="4" set "TARGET=all"
if "%TARGET%"=="" ( echo Ungueltige Auswahl. & pause & exit /b 1 )
:have_target

if /i "%TARGET%"=="all"        goto flash_all
if /i "%TARGET%"=="grid"       goto single
if /i "%TARGET%"=="controller" goto single
if /i "%TARGET%"=="module"     goto single
echo Verwendung: flash.bat ^<grid^|controller^|module^|all^> [port]
pause
exit /b 1

:single
call :flash_one "%TARGET%" "%PORT%"
goto done

:flash_all
for %%T in (grid controller module) do (
  echo.
  echo ================================================
  echo  Naechstes Board: %%T
  echo  Bitte NUR das %%T-Board anstecken.
  set /p GO=Enter zum Flashen ^(oder s + Enter zum Ueberspringen^):
  if /i "!GO!"=="s" (
    echo  -^> %%T uebersprungen.
  ) else (
    call :flash_one "%%T" ""
  )
)
echo.
echo Alle gewaehlten Boards durch.
goto done

REM -- Subroutine: flasht EIN Board. %1 = ziel, %2 = port (optional) --
:flash_one
set "T=%~1"
set "P=%~2"
if not exist "%T%\" ( echo FEHLER: Ordner '%T%\' fehlt. Skript im Projekt-Hauptordner ausfuehren. & exit /b 1 )
if not "%P%"=="" goto fo_flash
set "P="
for /f "tokens=1" %%Q in ('arduino-cli board list ^| findstr /R /C:"COM[0-9]"') do set "P=%%Q"
if "%P%"=="" (
  echo FEHLER: Kein Board gefunden. Board anstecken, oder Port angeben.
  echo Taucht kein COM-Port auf? Dann fehlt evtl. der USB-Treiber
  echo   ^( CP210x: Silicon Labs  /  CH340: WCH ^) - siehe SETUP.md.
  exit /b 1
)
echo Port automatisch erkannt: %P%
:fo_flash
echo == Flashe '%T%' auf %P% ==
arduino-cli compile --fqbn "%FQBN%" --libraries ./libraries --upload -p %P% %T%
exit /b %errorlevel%

:done
echo.
echo Fertig.
pause
