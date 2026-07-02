# Controller – Tastenbelegung (NeoTrellis 4×4)

Der Controller ist ein Adafruit NeoTrellis mit 16 Tasten (4×4). Er sendet
Befehle per ESP-NOW an das Grid. Quelle: [`controller.ino`](controller.ino).

## Layout

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

## Belegung

| Taste | Funktion | Bedienung | Details |
|------:|----------|-----------|---------|
| 0  | Modus-Anzeige | – | Status / aktueller Modus |
| 1  | Parameter: **Strobe-Farbe** | 2 s halten → einstellen | danach `+`/`−` ändern |
| 2  | – | – | frei |
| 3  | **Modus +** | tippen | nächster Modus |
| 4  | Parameter: **Helligkeit** | 2 s halten → einstellen | |
| 5  | Parameter: **Hue-Speed** | 2 s halten → einstellen | Auto-Farbrotation |
| 6  | – | – | frei |
| 7  | **Modus −** | tippen | vorheriger Modus |
| 8  | Parameter: **Geschwindigkeit** | 2 s halten → einstellen | |
| 9  | – | – | frei |
| 10 | – | – | frei |
| 11 | **Beat-Tap** (Tempo) | 3 s halten zum Aktivieren, dann tippen | Tap-Tempo → Geschwindigkeit |
| 12 | Parameter: **Farbe** | 2 s halten → einstellen | Hue-Basis |
| 13 | Strobe **„CASTLE / 2026"** | halten | statischer Text-Strobe |
| 14 | **Modus-Strobe** | halten | blinkt die aktuelle Animation |
| 15 | **Strobe** | halten | weiß/farbige Quadrate |

## Bedienmodell

- **Standard:** `+` (3) / `−` (7) wechseln den Modus.
- **Parameter einstellen:** eine Parameter-Taste (4, 8, 12, 1, 5) 2 s halten →
  der Parameter blinkt, alle anderen Tasten sind dunkel außer `+`/`−`. Dann ändern
  `+`/`−` nur diesen Parameter (mit Beschleunigung). Nach 3 s ohne Aktion zurück
  zum Standard.
- **Beat-Tap (11):** 3 s halten zum Aktivieren, danach im Takt tippen → globale
  Geschwindigkeit. Verlässt den Modus nach 3 s Inaktivität.
- **Strobes (13, 14, 15):** wirken nur solange gehalten (momentary).

## Parameter-Ziele (ESP-NOW `target`)

| target | Parameter |
|-------:|-----------|
| 0 | Helligkeit |
| 1 | Geschwindigkeit |
| 2 | Farbe (Hue-Basis) |
| 3 | Strobe-Farbe |
| 4 | Hue-Speed |
