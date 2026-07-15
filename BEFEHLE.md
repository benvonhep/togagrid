# Steuereinheit – Befehlsübersicht

Alle Befehle, die die Control Unit (NeoTrellis 4×4) per ESP-NOW an das Grid
sendet. Quellen: [`controller/controller.ino`](controller/controller.ino),
[`grid/grid.ino`](grid/grid.ino), [`grid/grid_config.h`](grid/grid_config.h).

## 1. Tasten der Control Unit

```
 ┌────┬────┬────┬────┐
 │  0 │  1 │  2 │  3 │
 ├────┼────┼────┼────┤
 │  4 │  5 │  6 │  7 │
 ├────┼────┼────┼────┤
 │  8 │  9 │ 10 │ 11 │
 ├────┼────┼────┼────┤
 │ 12 │ 13 │ 14 │ 15 │
 └────┴────┴────┴────┘
```

| Taste | Funktion | Bedienung | Was zum Grid geht | LED im Standard |
|------:|----------|-----------|-------------------|-----------------|
| 0  | frei | – | – | aus |
| 1  | Parameter **Strobe-Farbe** | 2 s halten → Einstellmodus | schaltet `+`/`−` auf die Strobe-Farbe um | aktuelle Strobe-Farbe |
| 2  | frei | – | – | aus |
| 3  | **Plus (+)** | tippen / halten (Repeat) | einen Modus weiter – oder den eingestellten Parameter erhöhen | türkis |
| 4  | Parameter **Helligkeit** | 2 s halten → Einstellmodus | schaltet `+`/`−` auf die Helligkeit um | Meter blau→rot |
| 5  | Parameter **Hue-Speed** | 2 s halten → Einstellmodus | schaltet `+`/`−` auf die Hue-Speed um | Meter blau→rot |
| 6  | frei | – | – | aus |
| 7  | **Minus (−)** | tippen / halten (Repeat) | einen Modus zurück – oder den eingestellten Parameter senken | türkis |
| 8  | Parameter **Geschwindigkeit** | 2 s halten → Einstellmodus | schaltet `+`/`−` auf die Geschwindigkeit um | Meter blau→rot |
| 9  | frei | – | – | aus |
| 10 | frei | – | – | aus |
| 11 | **Beat-Tap** | 3 s halten zum Aktivieren, dann im Takt tippen | das getappte Tempo als absolute Geschwindigkeit | dunkelrot |
| 12 | Parameter **Farbe** (Hue-Basis) | 2 s halten → Einstellmodus | schaltet `+`/`−` auf die Farbe um | aktueller Grid-Tint |
| 13 | **Castle-Strobe** „CASTLE / 2026" | halten (momentary) | Castle-Strobe an, solange gehalten | blaugrün |
| 14 | **Modus-Strobe** | halten (momentary) | Modus-Strobe an, solange gehalten | violett |
| 15 | **Strobe** (Quadrate) | halten (momentary) | Strobe an, solange gehalten | aktuelle Strobe-Farbe |

## 2. Befehle an das Grid

| Befehl | Ausgelöst durch | Mitgeschickt | Wirkung im Grid |
|--------|-----------------|--------------|-----------------|
| Heartbeat | automatisch alle 100 ms + sofort bei jeder Strobe-Flanke | die drei Strobe-Zustände | hält die Verbindung; bleibt er länger als 400 ms aus, gilt der Sender als weg |
| Modus-Schritt | `+` / `−` im Standard | Richtung vorwärts / rückwärts | Modus wechseln (läuft über alle 90 Modi um), gespeicherte Helligkeit des Modus laden, Modus-Overlay einblenden |
| Parameter-Schritt | `+` / `−` im Einstellmodus | welcher Parameter, Richtung | ändert genau diesen Parameter – siehe Parameter-Tabelle |
| Tempo setzen | Beat-Tap (Taste 11) | die berechnete Geschwindigkeit | setzt die Geschwindigkeit absolut statt schrittweise |

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

Jede Parameteränderung wird im Grid persistent gespeichert.

## 4. Bedienmodell (Zustände der Control Unit)

| Zustand | Eintritt | `+`/`−` wirken auf | Austritt |
|---------|----------|--------------------|----------|
| Standard | Start | **Modus** | – |
| Einstellmodus | Parameter-Taste 2 s halten | **den aktiven Parameter** | dieselbe Taste erneut tippen (kein Timeout) |
| Beat-Tap | Taste 11 für 3 s halten | – (nur Tippen zählt) | 3 s ohne Tap; letztes Tempo bleibt |

Im Einstellmodus blinkt die aktive Parameter-Taste (~4 Hz) in ihrer eigenen
Farbe, alle anderen Tasten sind dunkel – außer `+`/`−` und (beim Einstellen der
Strobe-Farbe) Taste 15 als Live-Vorschau.

**Auto-Repeat von `+`/`−` im Einstellmodus:** nach 500 ms Halten alle 80 ms, ab
2 s alle 40 ms, ab 4 s alle 16 ms.

**Beat-Tap:** ab dem 2. Tap wird aus dem Mittel der letzten bis zu 5 Taps das
Tempo berechnet (120 BPM = Geschwindigkeit 1.0), begrenzt auf 0.1 … 4.0.

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
