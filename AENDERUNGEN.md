# Änderungen seit dem letzten Flash — Zusammenfassung & Test-Checkliste

**Stand:** 26.07.2026 · **Protokoll:** `TOGA_VERSION 8 → 10` · **Modi:** 67 → **72** (Modus 48 = neu, 68–71 = neu)

> ⚠️ **GANZE FLOTTE NEU FLASHEN** (Grid **+** Controller **+** ALLE Module). Bei
> gemischten Versionen werden nicht-passende Knoten nicht gehört. Alle drei Sketches
> kompilieren sauber (Grid/Modul/Controller je ~45 % Flash). Nach dem Flashen in der
> Web-UI die **Versions-Pills** prüfen.

**Flash-Befehle** (per USB, Board am Kabel):
```
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --upload -p <port> grid/
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --upload -p <port> controller/
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --upload -p <port> module/   # für JEDES Modul
```

---

## ★★ Zentrale Fixes (dieser Durchgang)

- ☐ **Auto-Modus blendet KEINE Modus-Nummern mehr ein** (still; `A1…A4` nur bei Taste 12).
- ☐ **Strobe-Fix — alle drei gleich schnell auf JEDEM Modus** (Modus/Castle/Strobe, volle Loop-Rate; Modus-Strobe rendert 1× pro Blitz, kein Ruckeln mehr). Test: Modus-Strobe auf schwerem (52) vs. leichtem Modus = gleicher Takt.
- ☐ **Pro-Modus Geschwindigkeits-Bereich (neu):** die Speed-Taste (0.1…4.0) mappt jetzt je Modus auf ein eigenes Min/Max-Fenster (`gModeSpeed`/`togaModeSpeed` in grid.ino). Damit sind ALLE „max/min Geschwindigkeit"-Wünsche zentral umgesetzt:
  - **max schneller:** 8·2x, 15·2x, 17·2x, 22·3x, 23·5x, 24·2x, 30·2x, 31·3x, 32·2x, 34·3x, 35·3x, 40·3x, 43·2x, 44·2x, 46·1.5x, 56·3x, 57·3x, 61·1.5x, 62·1.5x, 66·2x, 12·+50%
  - **min langsamer:** 11·¼, 25·20%, 58·¼, 21·−50%
  - **min schneller / beides:** 26, 27 (max 5x + min ½), 62 (min 2x), 66 (min 2x), 67 (Boden angehoben, damit langsam noch was zeigt)
  Test: bei diesen Modi Speed ganz hoch/runter → spürbar schnelleres Maximum bzw. langsameres Minimum.
- ☐ **Modus 4 (earthquake):** reagiert jetzt überhaupt auf die Speed-Taste (lief vorher auf Echtzeit) **+** häufigere Beben (1500 ms) und dauerhafte Grundaktivität → keine langen dunklen Phasen mehr.
- ☐ **Button 5 (Spot):** **TAP** = nur die Module schießen den Spot durch, Grid bleibt auf seinem Modus. **HALTEN** (≥350 ms) = zusätzlich fährt das Grid die Quadrate klein→groß.

## Grid-Animationen (einzeln)

- ☐ **9 collider:** greller Schluss-Blitz nur noch ~1/5 so hell.
- ☐ **27 strangeAttractor:** deutlich größerer Bewegungsradius (füllt mehr Fläche).
- ☐ **38 stickman:** springt jetzt mittig in die LÜCKE zwischen den Hindernissen, immer mit Abstand — nie hinein.
- ☐ **40 smiley:** Mund in klar anderer Farbe (kräftiges Rot) als das Gesicht.
- ☐ **41 eye:** Augapfel-Weiß zur Kontur hin abgeschattet (Mitte am hellsten); Kontur = leuchtendes Azurblau, heller als das Weiß; Iris = Smaragd-Türkis, ebenfalls heller/abgesetzt. Bewegung unverändert.
- ☐ **51 rocketFire:** um 180° gedreht — Rakete fliegt jetzt nach OBEN, Flamme unten.
- ☐ **53 aliens:** vertikaler Hüpfer ~2,5× größer.
- ☐ **59 pulseHeart:** um 180° gedreht (Herz steht aufrecht), tiefere Kerbe zwischen den Backen, weicherer Puls.
- ☐ **65 superHexagon:** Hintergrund deutlich dunkler → Hexagon-Ringe heben sich klar ab (nicht ganz aus).

## ★ 5 neue Modi (Stil von Modus 67, jeder anders) — `anim_new2.h`

- ☐ **48 auroraCurtains** (ersetzt earthWindow): fließende Polarlicht-Vorhänge.
- ☐ **68 neonShockwaves:** kollidierende Neon-Schockwellen-Ringe.
- ☐ **69 emberVortex:** wirbelnder Feuer-/Glut-Vortex mit weißglühendem Kern.
- ☐ **70 prismShards:** Regenbogen-Kristallklingen, explodieren & rasten zusammen.
- ☐ **71 nebulaBreath:** atmender Tiefraum-Nebel mit Tendrils & Sternenfunkeln.
  → Test: Modi 48, 68–71 durchschalten; jeder soll „sehr geil" und klar unterschiedlich wirken.

## Bodeneffekt (Module) — `module_effects.h`, 28 Zeilen getunt

Durchgängig: weniger LEDs / dunkler, **zentraler Ursprung** (Funken aus der Mitte
nach vorne+hinten), **Kometen** statt Vollbeleuchtung, **blockig** wo das Grid blockig ist,
**Sparkle** wo das Grid funkelt. Betroffen u.a.:
- ☐ Kometen/zentral: 2, 3 (Richtung außen→innen), 10, 15 (Blobs zueinander), 19, 32 (Punktpaare), 42, 43, 45, 60
- ☐ Zentral + sparkle/flowy: 5, 13, 25, 26, 27, 58
- ☐ Blockig gestuft: 20, 66, 67
- ☐ Weniger/dunkler: 21 (fast statisch), 40 (smooth statt Blitz), 54, 55
- ☐ 6 blackHole: spiegelt jetzt Außen- UND Zentrumsfarbe · 18 cymatics: schnelles Schwappen · 47 horsehead: Lauflicht vorne→hinten · 65: reicht weiter zur Mitte
- (unverändert, weil gut: 36, 61, 62)

> Bekannte Näherung: A_BLOCKS (20/66/67) kennt kein exaktes „aus der Mitte"; die
> blockige Stufung stimmt, der zentrale Ursprung ist dort nur angenähert.

## Protokoll / Zähler

- ☐ `TOTAL_ANIMS` = `MOD_MODE_COUNT` = `TOGA_EFFECT_COUNT` = **72** (Grid/Modul/Proto synchron).
- ☐ Effektnamen 48/68–71 in beiden Web-UIs korrekt.
- ☐ (schon zuvor) `TOGA_VERSION 8→10`, Sync/Hello/ModCfg-Felder, Speed-Sweep, Web-UI-Erweiterungen, Unterspannungsschutz 3,0 V, Fallback 10 s. **MAX_BRIGHTNESS 128→255** — Stromversorgung bei Vollweiß prüfen!

---

## Noch offen / nur angenähert (Hardware-Feinschliff)

- Punkt 10 (Spot volle Streifenlänge): renderSpots läuft bereits über `gWidth-1` — auf Hardware prüfen, ob das der eingestellten LED-Zahl entspricht.
- Punkt 35 „min Hue-Speed zu schnell", Punkt 20 „Folge-Modus mit umgekehrter Richtung": nicht umgesetzt (bräuchte eigenes Hue-Speed-Mapping bzw. einen weiteren Modus).
- 52 warningLight „ruckelt extrem": Speed-Bereich unverändert; das Ruckeln war eher Render-/Loop-Last — durch den Strobe-Render-Fix teils entschärft, am Modus selbst nichts geändert.
- Alle **subjektiven** Punkte (Farbwahl, „smooth genug", „sehr geil") brauchen einen Blick am echten Grid und ggf. Nachjustierung.
