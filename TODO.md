# TODO / Ideen

_Übernommen aus `controller tastenbelegun.rtf` vor dem Aufräumen des Projekts._

## Offen
- [ ] Reset-Button-Kombination
- [ ] Button für globales Strobe (inkl. kleiner Lichter)
- [ ] Color-Switching-Modus
- [ ] Helligkeits-Settings-Helper prüfen (Default-Setting)
- [ ] Beat-Match auf der Anlage gegenprüfen: sitzt der Peak auf dem Schlag?
      Sonst `BEAT_LATENCY_MS` in `libraries/TogaProto/src/toga_proto.h` justieren
      (zu spät → höher, zu früh → niedriger)

## Wichtig beim Flashen

**Alle Knoten zusammen flashen.** `TOGA_VERSION` steht jetzt auf 2 (war 1). Ein
Knoten mit alter Firmware wird von den neuen ignoriert – und umgekehrt. Genau
das ist der Zweck: ein Modul auf alter Firmware sendete Pakete, die als
gültige Befehle durchgingen und **den Strobe (Taste 15) mitten im Halten
beendet** sowie die Sync-Farben der Control Unit überschrieben haben. Symptom
war „Strobe blitzt kurz, dann wieder der aktive Modus" + Taste 15 wird dunkel.

Meldet das Grid auf Serial `WARN: 2. Befehls-Sender …`, funkt ein zweiter Knoten
Befehle mit – dann ist genau dieser Fehler wieder da.

## Erledigt (Referenz)
- [x] Beat-Match neu gebaut: Tempo-Erkennung im Grid statt im Controller, Peak
      sitzt per Phasen-Anker auf dem Tap, Sägezahn statt Exponential-Decay,
      kein Scharfschalten mehr (`grid/grid_beat.h`)
- [x] Auto-Modus (Taste 0): 4 Stimmungen — Ruhe · Space · Party · Show
- [x] Modus-Nummer oben rechts einblenden (~0,5 s, ohne Namens-Scroll)
- [x] Helligkeit pro Animation speichern (+ Default)
- [x] Automatischer Helligkeitsausgleich zwischen Modi (AGC)
- [x] Smiley wie das Auge mit smooth shapes (2×2-Augen)
- [x] Castle-Button (statischer „CASTLE / 2026"-Strobe, Taste 13)
- [x] Space-/Alien-Modi (starfield, warp, asteroids, nebulae, aliens, invaders, …)
