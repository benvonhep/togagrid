# Controller – Tastenbelegung (NeoTrellis 4×4)

Der Controller ist ein Adafruit NeoTrellis mit 16 Tasten (4×4). Er sendet
Befehle per ESP-NOW an das Grid. Quelle: [`controller.ino`](controller.ino).

> **Pad physisch um 90° im Uhrzeigersinn montiert.** Die NeoTrellis-Tastennummern
> (die Zahlen unten) sind die Chip-Nummern — durch die Drehung liegt Taste **12**
> oben links, **0** oben rechts usw. Das Layout unten zeigt die **echten physischen
> Positionen**, also genau das, was man vor sich sieht und drückt. Im Code bleibt
> intern die logische Nummerierung; `PHYS2LOG`/`LOG2PHYS` in `controller.ino`
> gleichen die Drehung an Eingang und LED-Ausgang aus (verifiziert: oben links =
> Auto, oben rechts = Modus +).

## Layout (echte physische Positionen; Zahl = NeoTrellis-Tastennummer)

```
 ┌───────┬───────┬───────┬───────┐
 │ 12    │ 8     │ 4     │ 0     │
 │ Auto  │ Farbe │ frei  │ Mod + │
 ├───────┼───────┼───────┼───────┤
 │ 13    │ 9     │ 5     │ 1     │
 │Hellig.│HueSpd │ frei  │ Mod − │
 ├───────┼───────┼───────┼───────┤
 │ 14    │ 10    │ 6     │ 2     │
 │Geschw.│ frei  │Black- │ Beat  │
 │       │       │ out   │       │
 ├───────┼───────┼───────┼───────┤
 │ 15    │ 11    │ 7     │ 3     │
 │StFarbe│Castle │ModStr │Strobe │
 └───────┴───────┴───────┴───────┘
```

## Belegung

| Taste | Funktion | Bedienung | Details |
|------:|----------|-----------|---------|
| 0  | **Modus +** | tippen | nächster Modus |
| 1  | **Modus −** | tippen | vorheriger Modus |
| 2  | **Beat-Tap** (Tempo) | im Takt tippen (~4 Taps) · 1× tippen + 2 s warten = aus | Tap-Tempo → Helligkeits-Puls (**nicht** Geschwindigkeit) |
| 3  | **Strobe** | halten | weiß/farbige Blitze; bei jedem Druck einer von 5 Looks (Quadrate · Voll · Streifen · Ringe · Viertel) |
| 4  | – | – | frei |
| 5  | – | – | frei |
| 6  | **Blackout** | tippen (Umschalter) | alles schwarz (Grid **und** Module); erneut tippen = zurück. Modus lässt sich weiter wechseln (läuft im Dunkeln weiter) und Strobes blitzen durch. Nur diese Taste + Strobe/Modus-Strobe/Castle/Auto bleiben am Pad beleuchtet |
| 7  | **Modus-Strobe** | halten | blinkt die aktuelle Animation |
| 8  | Parameter: **Farbe** | 2 s halten → einstellen | Hue-Basis, danach `+`/`−` ändern |
| 9  | Parameter: **Hue-Speed** | 2 s halten → einstellen | Auto-Farbrotation; **pro Modus gespeichert** |
| 10 | – | – | frei |
| 11 | Strobe **„CASTLE / 2026"** | halten | statischer Text-Strobe |
| 12 | **Auto-Modus** | tippen | schaltet durch 4 Stimmungen (1→2→3→4→1), blinkt in deren Farbe; Wechseltempo = Geschwindigkeit (14) |
| 13 | **Helligkeit** (3-stufig) | tippen / 2 s / 10 s halten | tippen = **global**, 2 s = **Modus-Faktor**, 10 s = **Controller-Pad** — siehe unten |
| 14 | Parameter: **Geschwindigkeit** | 2 s halten → einstellen | **pro Modus gespeichert** |
| 15 | Parameter: **Strobe-Farbe** | 2 s halten → einstellen | danach `+`/`−` ändern |

## Kombis

| Kombi | Funktion |
|-------|----------|
| **3 halten + `+`/`−`** (Strobe + 0/1) | Pause zwischen den Strobe-Blitzen regeln (30–250 ms) |

Bewusst eine Modifier-Kombi auf einer **belegten** Taste, damit 4 · 5 · 10
frei bleiben (6 ist jetzt Blackout).

**OTA braucht keine Kombi.** Diese Einheit macht den AccessPoint `togalights`
auf Kanal 1 auf — dem Kanal, auf dem der ESP-NOW-Bus ohnehin läuft. Jeder Knoten
ist damit gleichzeitig im Netz und auf dem Bus und jederzeit flashbar, ohne die
Show zu verlassen (Details: [BEFEHLE.md](../BEFEHLE.md) §9).

## Bedienmodell

- **Standard:** `+` (0) / `−` (1) wechseln den Modus.
- **Auto-Modus (12):** tippen → nächste Stimmung (1–4, danach wieder 1). Die
  Taste blinkt in der Eigenfarbe der Stimmung, gedreht um den aktuellen
  Grid-Tint. Wie schnell gewechselt wird, regelt die **Geschwindigkeit** (14).
  Ein manueller Moduswechsel mit `+`/`−` schaltet ihn ab; Taste 12 selbst
  schaltet nie aus.
- **Parameter einstellen:** eine Parameter-Taste (14, 15, 8, 9) 2 s halten →
  der Parameter blinkt, alle anderen Tasten sind dunkel außer `+`/`−`. Dann ändern
  `+`/`−` nur diesen Parameter (mit Beschleunigung). Nach 3 s ohne Aktion zurück
  zum Standard. (**Helligkeit (13)** hat ihr eigenes Modell, siehe unten.)
- **Pro-Modus-Speicher:** Jeder Modus merkt sich seine eigene **Geschwindigkeit**,
  seinen **Hue-Speed** und seinen **Helligkeits-Faktor** — beim Moduswechsel wird
  der jeweils gespeicherte Wert wiederhergestellt. Alles bleibt über Stromausfall
  erhalten (im Grid-NVS). Farbe (Hue-Basis) und Strobe-Farbe bleiben global.
- **Helligkeit (13):** zwei Ebenen plus Controller-Dimmung. Die effektive
  Helligkeit eines Modus = **global × Modus-Faktor**.
  - **tippen** (< 2 s) → **globale Helligkeit** (0–100 %, wirkt auf **alle** Modi).
    Taste pulsiert **langsam**. `+`/`−` ändern.
  - **2 s halten** → **Modus-Faktor** (0–1) nur des aktuellen Modus. Taste
    pulsiert **schnell**. `+`/`−` ändern. Jeder Modus merkt sich seinen Faktor.
  - **10 s halten** → **Controller-Pad-Helligkeit** (damit der Controller nicht
    zu hell ist). Taste leuchtet **konstant weiß**. `+`/`−` dimmen das Pad selbst.
  - Beim Halten wechselt die Taste live (langsam → schnell → weiß), sodass man
    sieht, wo man landet. **Erneutes Tippen** verlässt den jeweiligen Modus.
  - Die **globale** Helligkeit dimmt auch die Tasten des Controllers mit.
  - Alles bleibt über Stromausfall erhalten (global + Faktoren im Grid, Pad-Faktor
    im Controller).
- **Beat-Tap (2):** kein Aktivieren — einfach im Takt tippen. Nach ~4 Taps
  steht das Tempo, die Taste pulsiert dann selbst im erkannten Takt. Der Puls
  läuft weiter, bis neue Taps ein neues Tempo setzen. Er moduliert nur die
  **Helligkeit**, nicht die Geschwindigkeit.
  **Aus:** einmal tippen und 2 s nicht mehr — die Taste hört auf zu pulsieren.
  Ab zwei Taps ist es immer ein Tempo, nie ein Ausschalten.
- **Strobes (3 Strobe, 7 Modus-Strobe, 11 Castle-Strobe):** wirken nur solange gehalten (momentary).

## Parameter-Ziele (ESP-NOW `target`)

| target | Parameter |
|-------:|-----------|
| 0 | Helligkeit **global** (Master 0–100 %) |
| 1 | Geschwindigkeit |
| 2 | Farbe (Hue-Basis) |
| 3 | Strobe-Farbe |
| 4 | Hue-Speed |
| 5 | Strobe-Gap (nur Kombi 3 Strobe + `+`/`−`) |
| 6 | Helligkeits-**Faktor** des aktuellen Modus (0–1) |
