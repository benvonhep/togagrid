# Steuereinheit – Befehlsübersicht

Alle Befehle, die die Control Unit (NeoTrellis 4×4) per ESP-NOW an das Grid
sendet. Quellen: [`controller/controller.ino`](controller/controller.ino),
[`grid/grid.ino`](grid/grid.ino), [`grid/grid_config.h`](grid/grid_config.h).

## 1. Tasten der Control Unit

🟢 = **noch frei** · ⚪ = belegt

|  |  |  |  |
|:-:|:-:|:-:|:-:|
| 🟢 **0**<br>_frei_ | ⚪ **1**<br>Strobe-Farbe | 🟢 **2**<br>_frei_ | ⚪ **3**<br>Plus `+` |
| ⚪ **4**<br>Helligkeit | ⚪ **5**<br>Hue-Speed | 🟢 **6**<br>_frei_ | ⚪ **7**<br>Minus `−` |
| ⚪ **8**<br>Geschwindigkeit | 🟢 **9**<br>_frei_ | 🟢 **10**<br>_frei_ | ⚪ **11**<br>Beat-Match |
| ⚪ **12**<br>Farbe | ⚪ **13**<br>Castle-Strobe | ⚪ **14**<br>Modus-Strobe | ⚪ **15**<br>Strobe |

**Frei: 🟢 0 · 2 · 6 · 9 · 10** — fünf Tasten für neue Funktionen
(siehe offene Punkte in [TODO.md](TODO.md): Reset-Kombi, globales Strobe,
Auto-Modus, Color-Switching).

| Taste | Funktion | Bedienung | Was zum Grid geht | LED im Standard |
|------:|----------|-----------|-------------------|-----------------|
| 🟢 0  | **frei** | – | – | aus |
| 1  | Parameter **Strobe-Farbe** | 2 s halten → Einstellmodus | schaltet `+`/`−` auf die Strobe-Farbe um | aktuelle Strobe-Farbe |
| 🟢 2  | **frei** | – | – | aus |
| 3  | **Plus (+)** | tippen / halten (Repeat) | einen Modus weiter – oder den eingestellten Parameter erhöhen | türkis |
| 4  | Parameter **Helligkeit** | 2 s halten → Einstellmodus | schaltet `+`/`−` auf die Helligkeit um | Meter blau→rot |
| 5  | Parameter **Hue-Speed** | 2 s halten → Einstellmodus | schaltet `+`/`−` auf die Hue-Speed um | Meter blau→rot |
| 🟢 6  | **frei** | – | – | aus |
| 7  | **Minus (−)** | tippen / halten (Repeat) | einen Modus zurück – oder den eingestellten Parameter senken | türkis |
| 8  | Parameter **Geschwindigkeit** | 2 s halten → Einstellmodus | schaltet `+`/`−` auf die Geschwindigkeit um | Meter blau→rot |
| 🟢 9  | **frei** | – | – | aus |
| 🟢 10 | **frei** | – | – | aus |
| 11 | **Beat-Match** | 2 s halten zum Scharfschalten, dann im Takt tippen | das getappte Tempo als **Glow-Rhythmus** (nicht Geschwindigkeit) | dunkelrot |
| 12 | Parameter **Farbe** (Hue-Basis) | 2 s halten → Einstellmodus | schaltet `+`/`−` auf die Farbe um | aktueller Grid-Tint |
| 13 | **Castle-Strobe** „CASTLE / 2026" | halten (momentary) | Castle-Strobe an, solange gehalten | blaugrün |
| 14 | **Modus-Strobe** | halten (momentary) | Modus-Strobe an, solange gehalten | violett |
| 15 | **Strobe** (Quadrate) | halten (momentary) · **+ `+`/`−` → Pause regeln** | Strobe an, solange gehalten; `+`/`−` ändern die Pause zwischen den Blitzen | aktuelle Strobe-Farbe |

## 2. Befehle an das Grid

| Befehl | Ausgelöst durch | Mitgeschickt | Wirkung im Grid |
|--------|-----------------|--------------|-----------------|
| Heartbeat | automatisch alle 100 ms + sofort bei jeder Strobe-Flanke | die drei Strobe-Zustände | hält die Verbindung; bleibt er länger als 400 ms aus, gilt der Sender als weg |
| Modus-Schritt | `+` / `−` im Standard | Richtung vorwärts / rückwärts | Modus wechseln (läuft über alle 90 Modi um), gespeicherte Helligkeit des Modus laden, Modus-Nummer oben rechts einblenden |
| Parameter-Schritt | `+` / `−` im Einstellmodus, oder Strobe halten + `+`/`−` | welcher Parameter, Richtung | ändert genau diesen Parameter – siehe Parameter-Tabelle |
| Beat-Tempo setzen | Beat-Match (Taste 11), 2 s nach dem letzten Tap | die Beat-Periode in ms | setzt den **Glow-Rhythmus**; das Grid fährt graduell auf das neue Tempo (kein Sprung) |

Zusätzlich meldet **jede** Nachricht – auch der Heartbeat – die drei
Strobe-Zustände mit:

| Strobe | Taste | Wirkung |
|--------|------:|---------|
| Strobe | 15 | weiße/farbige Quadrate, solange gehalten |
| Modus-Strobe | 14 | blinkt die aktuelle Animation |
| Castle-Strobe | 13 | statischer Text-Strobe „CASTLE / 2026" |

## 3. Parameter

| Parameter | Taste | Schritt | Bereich | Verhalten am Ende |
|-----------|------:|---------|---------|-------------------|
| Helligkeit | 4  | ±5   | 1 … 128 | begrenzt, pro Modus gespeichert |
| Geschwindigkeit | 8  | ±0.1 | 0.1 … 4.0 | begrenzt |
| Farbe (Hue-Basis) | 12 | ±6   | ganzer Farbkreis | läuft um |
| Strobe-Farbe | 1  | ±6   | ganzer Farbkreis | läuft um |
| Hue-Speed (Auto-Rotation) | 5  | ±1   | 0 … 40  | begrenzt |
| **Strobe-Pause** | 15 halten + `+`/`−` | ±5 ms | 30 … 250 ms | begrenzt |

Jede Parameteränderung wird im Grid persistent gespeichert.

## 4. Bedienmodell (Zustände der Control Unit)

| Zustand | Eintritt | `+`/`−` wirken auf | Austritt |
|---------|----------|--------------------|----------|
| Standard | Start | **Modus** | – |
| Einstellmodus | Parameter-Taste 2 s halten | **den aktiven Parameter** | dieselbe Taste erneut tippen (kein Timeout) |
| Beat-Match | Taste 11 für 2 s halten | – (nur Tippen zählt) | 2 s ohne Tap → Tempo wird übernommen; oder 3 s ganz ohne Aktivität |

Solange **Taste 15 gehalten** wird, wirken `+`/`−` immer auf die Strobe-Pause –
unabhängig vom Zustand.

Im Einstellmodus blinkt die aktive Parameter-Taste (~4 Hz) in ihrer eigenen
Farbe, alle anderen Tasten sind dunkel – außer `+`/`−` und (beim Einstellen der
Strobe-Farbe) Taste 15 als Live-Vorschau.

**Auto-Repeat von `+`/`−` im Einstellmodus:** nach 500 ms Halten alle 80 ms, ab
2 s alle 40 ms, ab 4 s alle 16 ms. Beim Regeln der Strobe-Pause: nach 400 ms
alle 60 ms.

**Beat-Match:** 2 s halten → scharf (Taste blinkt), dann im Takt tippen. 2 s nach
dem letzten Tap wird aus dem Mittel aller Intervalle (bis zu 8 Taps) das Tempo
berechnet und gesendet – 20 … 300 BPM. Das Grid legt daraufhin einen
Glow-Puls über **alle** Animationen (auf `BEAT_MIN` = 45 % zwischen den Beats)
und fährt graduell auf ein neu getapptes Tempo. Die Animations­geschwindigkeit
bleibt davon **unberührt**. Das Tempo überlebt einen Neustart.

**Strobe (15):** der Blitz ist fix und so kurz wie möglich
(`STROBE_FLASH_MS` = 10 ms – kürzer geht nicht, ein LED-Refresh über alle
12 Strips dauert ~10–15 ms). Nur die Pause ist regelbar.

## 5. Rückmeldung vom Grid an die Control Unit

Das Grid meldet seinen Zustand zurück, damit die Tasten-LEDs echte Werte und
Farben anzeigen:

| Rückgemeldet wird | wofür |
|-------------------|-------|
| Helligkeit, Geschwindigkeit, aktueller Modus | Meter-Anzeige auf den Parameter-Tasten |
| Farbe, Hue-Speed, Strobe-Farbton | Farb-Parameter |
| exakte aktuelle Tint-Farbe | Taste 12 und der Blitz des Modus-Strobes |
| exakte aktuelle Strobe-Farbe | Tasten 1 und 15 |
| Strobe-Helligkeit, An-/Aus-Zeiten, Anzahl Quadrate | Strobe-Einstellungen |
