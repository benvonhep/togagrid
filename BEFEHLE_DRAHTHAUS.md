# Steuereinheit – Befehlsübersicht Profil **Drahthaus**

Bedienung des Controllers im Profil **Drahthaus** (Konzert *Elephant in the Room*).
Die Grundbedienung ist identisch mit dem Profil **Togalicht** — siehe
[BEFEHLE.md](BEFEHLE.md). Dieses Dokument beschreibt nur, **was im Drahthaus-Profil
anders ist**, plus die vollständige Tastenübersicht zum Ausdrucken.

> Alle Tastennummern sind die **physischen** Positionen (wie man vor dem Pad steht):
> **0** oben rechts, **12** oben links, **3** unten rechts, **15** unten links.

---

## 1. Profil wechseln

| Schritt | Was passiert |
|---|---|
| **0 + 1 + 2 gleichzeitig 5 s halten** (Mod+ · Mod− · Beat) | das ganze Pad blinkt **rot** |
| loslassen | unten links leuchten zwei Tasten |
| **12** tippen (türkis) | → Profil **Togalicht** |
| **13** tippen (orange) | → Profil **Drahthaus** |
| 10 s nichts tippen | Abbruch, Profil bleibt wie es war |

Das aktive Profil leuchtet in der Auswahl heller. Beim Halten der drei Tasten geht
vorher einmal Mod+ / Mod− (hebt sich auf) und ein Beat-Tap raus — normal.

**Alternativ am Handy:** WLAN `togalights` (Passwort `andmagic`), dann
<http://192.168.4.1/> bzw. `toga-ctl.local` → oben im Block **Profil** auf
**Drahthaus** tippen.

Das Profil ist im Controller gespeichert und übersteht Stromlos.
**Erkennungszeichen am Pad:** Taste **11** leuchtet **bernstein** statt türkis.

---

## 2. Tastenbelegung im Drahthaus-Profil

```
 ┌─────────────────┬─────────────────┬─────────────────┬─────────────────┐
 │ 12              │  8              │  4              │  0              │
 │  Auto-Modus     │  Farbe          │  Speed-Sweep    │  Song +         │
 │  (DH-Listen)    │                 │                 │  (Setlist)      │
 ├─────────────────┼─────────────────┼─────────────────┼─────────────────┤
 │ 13              │  9              │  5              │  1              │
 │  Helligkeit     │  Hue-Speed      │  Modul-Spot     │  Song −         │
 │                 │                 │                 │  (Setlist)      │
 ├─────────────────┼─────────────────┼─────────────────┼─────────────────┤
 │ 14              │ 10              │  6              │  2              │
 │  Geschwindigk.  │  Strobe-Speed   │  Blackout       │  Beat-Match     │
 ├─────────────────┼─────────────────┼─────────────────┼─────────────────┤
 │ 15              │ 11              │  7              │  3              │
 │  Strobe-Farbe   │ ★ TEXT-EBENE ★  │  Modus-Strobe   │  Strobe         │
 └─────────────────┴─────────────────┴─────────────────┴─────────────────┘
```

| Taste | Togalicht | **Drahthaus** |
|------:|-----------|---------------|
| 0 / 1 | nächster / vorheriger Modus (alle 123) | nächster / vorheriger **Song der Setlist** |
| 11 | Castle-Strobe „CASTLE / 2026" (halten) | **Text-Ebene öffnen / schließen** (tippen) |
| 12 | Auto-Moods mit Togalicht-Listen | Auto-Moods mit **Drahthaus-Listen** (siehe 5.) |
| Wand | Modus-Nummer wird kurz eingeblendet | **keine Nummern** auf der Wand |

**Alle anderen Tasten sind in beiden Profilen gleich:** Beat (2), Strobe (3),
Speed-Sweep (4), Spot (5), Blackout (6), Modus-Strobe (7), Farbe (8), Hue-Speed (9),
Strobe-Speed (10), Helligkeit (13), Geschwindigkeit (14), Strobe-Farbe (15).
Details dazu in [BEFEHLE.md](BEFEHLE.md).

---

## 3. Text-Strobes (Taste 11)

**Taste 11 tippen** → die Text-Ebene ist offen. Die fünf Text-Tasten liegen in den
**unteren zwei Reihen** und leuchten in ihrer eigenen Farbe, Taste 11 blinkt bernstein.
Die **obere Hälfte bleibt voll bedienbar**: Song +/− (0/1), Farbe (8), Hue-Speed (9),
Helligkeit (13), Auto (12), Speed-Sweep (4), Spot (5) und Strobe-Speed (10).

**Eine Text-Taste halten** → der Text blitzt auf der Wand im Strobe-Takt, die Module
blitzen in der Tint-Farbe mit. Loslassen = aus. Die gehaltene Taste leuchtet weiß.
**Text-Taste halten + 0 / 1** → Strobe schneller / langsamer, wie beim normalen Strobe.

**Taste 11 nochmal tippen** → Ebene zu, normale Belegung ist zurück.

```
 ┌─────────────────┬─────────────────┬─────────────────┬─────────────────┐
 │ 12              │  8              │  4              │  0              │
 │  Auto-Modus     │  Farbe          │  Speed-Sweep    │  Song +         │
 ├─────────────────┼─────────────────┼─────────────────┼─────────────────┤
 │ 13              │  9              │  5              │  1              │
 │  Helligkeit     │  Hue-Speed      │  Modul-Spot     │  Song −         │
 ├─────────────────┼─────────────────┼─────────────────┼─────────────────┤
 │ 14              │ 10              │  6              │  2              │
 │ ★ Elefant       │  Strobe-Speed   │ ★ ALLE GEGEN    │ ★ RAVE!         │
 │   (grau)        │                 │   ALLE (orange) │   (pink)        │
 ├─────────────────┼─────────────────┼─────────────────┼─────────────────┤
 │ 15              │ 11              │  7              │  3              │
 │  Strobe-Farbe   │  EBENE ZU       │ ★ DRAHT / HAUS  │ ★ DHA/RTU/AHS   │
 │                 │  (bernstein)    │   (grün)        │   (blau)        │
 └─────────────────┴─────────────────┴─────────────────┴─────────────────┘
```

| Taste | Text | Bild auf der Wand |
|------:|------|-------------------|
| 2 | **RAVE** | fette Blockbuchstaben über die ganze Wand: „RA" oben, „VE" unten |
| 6 | **ALLE GEGEN ALLE** | alle drei Wörter in einem Bild, untereinander |
| 14 | **Elefant** | Elefanten-Piktogramm, Rüssel schwingt bei jedem Blitz, dreht sich alle 4 Blitze um |
| 3 | **DHA / RTU / AHS** | 3×3-Buchstabenquadrat — spaltenweise gelesen „DRAHTHAUS" |
| 7 | **DRAHT / HAUS** | zwei Zeilen untereinander |

- **Takt** = globales Strobe-Tempo (Taste 10, oder Text-Taste halten + 0/1).
- **Farbe** = pro Text einstellbar am Handy (`toga-ctl.local` → **Farben der Text-Strobes**:
  Farbwähler oder Hex, „Strobe" = zurück auf die Strobe-Farbe von Taste 15). Die Module
  blitzen in derselben Farbe und mit einem Muster, das zum Text passt (3 Blöcke bei
  ALLE GEGEN ALLE, 2 Hälften bei DRAHT/HAUS, Schachbrett beim Quadrat, Elefant springt
  links/rechts, RAVE voll).
- In der Ebene fehlen nur: Beat (2), Strobe (3), Blackout (6), Modus-Strobe (7),
  Geschwindigkeit (14). Ebene zu → alle wieder da.

---

## 4. Setlist (Song + / Song −)

Im Drahthaus-Profil steppen **0 (Song +)** und **1 (Song −)** nur durch die Setlist,
nicht durch alle Modi. Am Ende springt es wieder an den Anfang.

**Standard-Reihenfolge:**

| Pos. | Modus | Song / Zweck |
|-----:|-------|--------------|
| 0 | Kalibrierung (setupGrid) | Aufbau / Grid prüfen |
| 1 | **Schwarz** (Modus 127) | alles aus, nur unten rechts ein kleiner, langsam atmender türkiser Punkt — man sieht, dass die Wand bewusst dunkel ist. Module ganz schwarz. Farbe wie jeder Modus über Taste 8 / Handy |
| 2 | **U-Boot** (konzertMarker) | „Wir stehen am Anfang" — Szene vom Albumcover: Neon-U-Boot mit 4 Tauchern über der roten Wüste (zwei Dünenlinien). Zündet wie eine Neonröhre beim Einschalten (erst einzelne Blitze, dann immer mehr Flackern, nach ~8 s an); danach schwebt das U-Boot, der Propeller dreht, Luftblasen steigen, Sand rieselt, Wind über den Dünen |
| 3 | dh100Elefanten | **100 Elefanten** |
| 4 | dhWhereDidYouSleep | **Where Did You Sleep Last Night** |
| 5 | dhEcholot | **Echolot** |
| 6 | dhCryMeARiver | **Cry Me A River** (feat. Aunty) |
| 7 | dhNichtErschrecken | **Nicht erschrecken** (Gewitter) |
| 8 | dhAlleGegenAlle | **Alle gegen alle** (= Togalicht-Modus 10) |
| 9 | dhSabbia1 | **Sabbia I** (Sandsturm) |
| 10 | dhSabbia2 | **Sabbia II** |
| 11 | dhMuecke | **Mücke** |
| 12 | dhRoentgenauge | **Röntgenauge** |
| 13 | dhDienstagRave | **Dienstag (noch immer dieser Rave)** |
| 14 | dhSong12 | Platzhalter für Song 12 |
| 15–17 | quadMirror · squareSpiral · gridPulse | DJ-Modi |
| – | dhRiesenschrift · dhElephantInTheRoom · dhMonogramm · dhElefantenKopf | **Testmodi (123–126)** — nicht in der Standard-Setlist; am Handy mit „+ Hinzufügen" in die Setlist holen |
| 18–22 | spiralGalaxy · spiralEject · squareImplode · tesseract · kaleidoWheel | DJ-Modi aus Togalicht |

Eine schon gespeicherte Setlist bekommt „Schwarz“ beim ersten Start der neuen
Controller-Firmware einmalig vor das U-Boot gesetzt. Wer es danach entfernt, behält es entfernt.

**Reihenfolge ändern am Handy:** `toga-ctl.local` → Block **Setlist**

| Knopf | Wirkung |
|:-----:|---------|
| ▶ | sofort zu diesem Eintrag springen |
| ▲ / ▼ | Eintrag nach oben / unten schieben |
| ✕ | Eintrag entfernen |
| **+ Hinzufügen** | beliebigen Modus (auch Togalicht-Modi) ans Ende anhängen |

Die Liste (max. 32 Einträge) und die aktuelle Position sind im Controller
gespeichert. Der aktuelle Eintrag ist grün hinterlegt.

---

## 5. Auto-Modus im Drahthaus-Profil (Taste 12)

Gleiche Bedienung wie bei Togalicht (tippen = nächste Stimmung, Song +/− beendet den
Auto-Modus), aber mit **eigenen Listen**: nur die starken Togalicht-Effekte (vor allem
die über Nr. 100) plus die DJ-Modi. Gedacht für Umbaupausen und das DJ-Set.

| Auto | Stimmung | Inhalt |
|-----:|----------|--------|
| 1 | **Ruhe** | mandalaBloom, galaxyCollision, oceanCaustics, mantaGlide, crystalGrowth, jellyfishBloom, gravitationalLens, clouds3d, aurora, lavaLamp, nebulaBreath |
| 2 | **Space** | tesseract, torusKnot, gravitationalLens, celestialDragon, blackHoleAccretion, galaxyCollision, wormholeThroat, supernovaShell, cometSwarm, starTunnel3D, warp, starfieldWarp |
| 3 | **Party** | kaleidoWheel, electricEel, plasmaGlobe, turbineBlades, neonShockwaves, emberVortex, prismShards, honeycombPulse, morphSwarm, quadMirror, squareSpiral, gridPulse, binaryRain, squareRain, vuColumns |
| 4 | **Show** | morphSwarm, celestialDragon, roseWindow, spirographArms, gyroscope, doublePendulum, clockworkGears, serpentNest, phoenixRise, fireflies, pulsarBeacon, solarFlare |
| 5 | **Spezial** | Best-of: alle Modi 100–106 plus die stärksten Showpieces |

Auch im Auto-Modus werden **keine Nummern** auf der Wand eingeblendet.

---

## 6. Was wird wo gespeichert

**Pro Modus** (jeder Modus merkt sich seinen eigenen Wert, in beiden Profilen):

| Einstellung | Taste |
|---|---|
| **Farbe** | 8 (2 s halten, dann +/−) |
| Geschwindigkeit | 14 |
| Hue-Speed | 9 |
| Helligkeitsfaktor | 13 (2 s halten) |

Beim Wechsel auf einen Modus startet er immer genau mit **seiner** Farbe.

**Am Handy:** `toga-ctl.local` → Block **Farben der Konzertmodi**. Für jeden Modus der
Setlist gibt es einen Farbwähler und ein Hex-Feld (z. B. `#ff8000`); **Nativ** setzt
die Originalfarben zurück. Gewählt wird der **Farbton** — der Modus wird so gedreht,
dass seine Hauptfarbe dort landet; Sättigung und Helligkeit des Hex-Werts zählen nicht.
Das ist derselbe Wert wie Taste 8 am Pad, beide zeigen immer denselben Stand. Die
Drahthaus-Songs starten in ihren eigenen Farben (Farbe = 0); einmal eingestellt,
bleibt pro Song die gewählte Farbe.

**Pro Profil** (Togalicht und Drahthaus getrennt):

| Einstellung | Taste |
|---|---|
| Strobe-Farbe | 15 |
| globale Helligkeit | 13 (tippen) |
| Strobe-Tempo | 10 |

**Tipp:** jeden Song einmal mit Musik durchspielen und dort Farbe, Speed, Hue-Speed
und Helligkeitsfaktor einstellen — das bleibt pro Song gespeichert.

---

## 7. Spickzettel fürs Konzert

| Ich will … | Taste |
|---|---|
| nächster / voriger Song | **0** / **1** |
| Text blitzen | **11** tippen → Text-Taste (untere zwei Reihen) halten → **11** tippen |
| normaler Strobe | **3** halten |
| aktuelle Animation blitzen | **7** halten |
| Strobe schneller / langsamer | **3** halten + **0** / **1** |
| Wand schwarz (Modus läuft weiter) | **6** tippen, nochmal = zurück |
| Beat einlernen | **2** im Takt tippen (~4×) · aus: 1× tippen + 2 s warten |
| Automatik für Pausen / DJ | **12** tippen (Stimmung 1→5), Song +/− beendet sie |
| Profil wechseln | **0 + 1 + 2** 5 s halten → **12** Togalicht / **13** Drahthaus |
