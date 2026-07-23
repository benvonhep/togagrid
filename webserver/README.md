# Toga Webserver — Module einzeln ansteuern

Ein eigener ESP32, der **nur zuhört und eine Weboberfläche ausliefert**. Er
treibt keine LEDs und steuert die Show nicht. Zieht man ihn ab, läuft die
Installation unverändert weiter.

- **Liste:** jedes Modul, das sich meldet, erscheint — mit MAC, Geometrie, Pin,
  Gruppe, laufendem Effekt, Gain, Uptime und ob es den Controller hört.
- **Einstellen:** pro Modul **Effekt** (fest pinnen oder `folgt Grid`) und
  **Gain** (10–200 %). Beides wird im Modul dauerhaft gespeichert.
- Module, die verstummen, bleiben als **offline** in der Liste stehen (10 min) —
  ein ausgefallenes Modul ist genau das, was man sehen will.

## ⚠ Der Router muss auf Kanal 1 stehen

Ein ESP32 hat **ein** Radio. Wer sich in ein WLAN einbucht, übernimmt dessen
Kanal — und ESP-NOW hört immer nur den Kanal, auf dem das Radio gerade ist.
Alle anderen Knoten stehen fest auf `TOGA_CHANNEL` (= 1) und assoziieren sich
bewusst mit nichts, damit ihnen niemand den Bus wegzieht.

Dieser Knoten ist die einzige Ausnahme: Er geht ins WLAN und erbt den
Router-Kanal. Steht der Router **nicht** auf Kanal 1, dann verbindet sich das
Board normal, liefert die Seite normal aus — und sieht **kein einziges Modul**.
Das ist keine Softwarefrage, das ist ein Radio und zwei Kanäle.

Die Seite zeigt den Kanal oben an und warnt rot, wenn er nicht passt, statt eine
leere Liste zu zeigen und dich einen Hardwarefehler suchen zu lassen.

Geht der Router nicht auf Kanal 1, bleiben zwei ehrliche Wege:
- diesen Knoten stattdessen einen **eigenen AP auf Kanal 1** aufmachen lassen
  (Handy verbindet sich direkt damit — funktioniert auch ohne Router), oder
- `TOGA_CHANNEL` auf den Router-Kanal setzen und **jeden** Knoten neu flashen.

## Einrichten

1. WLAN in [`togaws_config.h`](togaws_config.h) eintragen (SSID, Passwort, Hostname).
2. Flashen:
   ```
   arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app \
     --libraries ./libraries --upload --port /dev/cu.usbserial-XXXX webserver/
   ```
3. Aufrufen: `http://toga.local/` (oder die IP aus der seriellen Konsole).

Die serielle Konsole (115200) listet die Module alle 5 s ebenfalls — der
Rückfallweg, wenn ausgerechnet das WLAN das Kaputte ist.

## Wie es funktioniert

| Nachricht | Von → An | Takt |
|-----------|----------|------|
| `TogaHelloMsg` | Modul → alle | 1×/s — „ich existiere, hier ist meine Konfiguration" |
| `TogaModCfgMsg` | Webserver → **ein** Modul (per MAC) | nur bei Bedienung, 3× gesendet |

Module haben vorher **nie** gesendet. Sie melden sich jetzt einmal pro Sekunde —
ein Bericht, nie ein Befehl. Die Invariante, auf der die Driftfreiheit beruht,
bleibt unangetastet: ein Modul wendet weiterhin keine eigenen Schritte an und
spiegelt den Grid-Zustand vollständig. Geht ein Hello verloren, veraltet nur die
Liste; die Show merkt nichts.

Adressiert wird per **MAC im Broadcast** — jedes Modul verwirft, was nicht an
seine MAC gerichtet ist. Damit ist der Funkweg für jedes Paket im System
identisch, und die MAC ist ohnehin das, was das Modul schon meldet.

Die Effekt-Namen der Dropdown-Liste kommen aus `togaEffectName()` in
[`toga_proto.h`](../libraries/TogaProto/src/toga_proto.h) — dieselbe Tabelle, die
die Module benutzen. Eine Liste, die den Modulen widerspricht, wäre schlimmer
als gar keine.

## Was der alte Webserver war

Der Vorgänger (`module/esp32_webserver_toga/`, noch in der Git-Historie) war
kein Modul-Manager: er war **selbst ein Modul** mit Web-UI für sein eigenes
32×8-Panel, sein ESP-NOW-Callback hat Pakete nur auf die Konsole gedruckt.
Er benutzte FastLED 3.10 (`fl::XYMap`) und die arduino-esp32-3.x-Callback-Signatur
und lässt sich gegen den hier festgenagelten Stack (FastLED 3.6.0 + core 2.0.17)
gar nicht mehr übersetzen. Deshalb neu geschrieben statt ausgecheckt.
