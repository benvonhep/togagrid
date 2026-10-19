# Mode names — keep in sync

The grid runs **123 animations** (indices `0..122`). The mode **index and order are
the contract**: a module in FOLLOW mode renders row `k` of its `MODE_TABLE` while
the grid runs anim `k`. If the order changes in one place and not the other, every
strip cites the wrong animation from the changed index onward.

## The two authoritative lists (must match 1:1)

| # | Place | What it holds | Count symbol |
|---|-------|---------------|--------------|
| 1 | [grid/grid.ino](grid/grid.ino) — `case 0..122:` in the anim switch (~line 1030) | the real `anim_*()` called per index | `TOTAL_ANIMS` = 123 ([grid/grid_config.h:44](grid/grid_config.h#L44)) |
| 2 | [module/module_effects.h](module/module_effects.h) — `MODE_TABLE[]` (~line 78) | one `ModeSpec` row per grid anim, same order | `MOD_MODE_COUNT` = 123 ([module/module_effects.h:77](module/module_effects.h#L77)) |

**Invariant:** `TOTAL_ANIMS == MOD_MODE_COUNT` **and** row `k` of `MODE_TABLE`
describes the same animation as `case k:` in `grid.ino`.

> There is a THIRD list: `togaEffectName()` + `TOGA_EFFECT_COUNT` in
> [libraries/TogaProto/src/toga_proto.h](libraries/TogaProto/src/toga_proto.h) — the names the
> phone UI shows (module manager and the Drahthaus setlist). Append there too, and keep
> `TOGA_EFFECT_COUNT == TOTAL_ANIMS == MOD_MODE_COUNT`.
> Effect ids ≥ `TOGA_EFFECT_LOCAL_BASE` (140) are module-local effects, NOT grid modes:
> the grid list may grow to 139 at most without moving those bases.

## When something changes — checklist

- **Add a mode:** append `case 123:` in `grid.ino`, append row 123 to `MODE_TABLE`, append
  the name to `togaEffectName()`, bump `TOTAL_ANIMS`, `MOD_MODE_COUNT` **and** `TOGA_EFFECT_COUNT`
  to 124 (check `MAX_MODES` ≥ new count).
- **Remove a mode:** delete the `case` **and** the `MODE_TABLE` row, renumber every
  later comment, decrement both counts. Never leave a hole.
- **Reorder / rename:** change **both** lists together and re-check the four citation
  axes (colour / density / motion / rhythm) for each moved row.
- After any change, confirm the trailing `/* k name */` comment in `MODE_TABLE`
  matches the `anim_*` at `case k:`.

## Modes 0–71 (source of truth = grid.ino case order)

| # | Name (`anim_*`) | # | Name (`anim_*`) |
|---|-----------------|---|-----------------|
| 0 | setupGrid | 34 | spiralGalaxy |
| 1 | heartbeat | 35 | boids |
| 2 | binaryRain | 36 | fireworks |
| 3 | vortex | 37 | textTOGA |
| 4 | earthquake | 38 | stickman |
| 5 | supernova | 39 | paddleBall |
| 6 | blackHole | 40 | smiley |
| 7 | wormhole | 41 | eye |
| 8 | mitosis | 42 | starfield2 |
| 9 | collider | 43 | warp |
| 10 | shockwaveChain | 44 | shootingStars |
| 11 | pulsar | 45 | blackHole2 |
| 12 | liquidMetal | 46 | pulsar2 |
| 13 | coriolisStorm | 47 | horsehead |
| 14 | aurora | 48 | auroraCurtains ⭐ |
| 15 | lavaLamp | 49 | radar |
| 16 | nerveSystem | 50 | sineWaves |
| 17 | gravityLens | 51 | rocketFire |
| 18 | cymatics | 52 | warningLight |
| 19 | starfieldWarp | 53 | aliens |
| 20 | concentricSquares | 54 | rain2 |
| 21 | squareRain | 55 | levitate |
| 22 | depthMap | 56 | confetti |
| 23 | cosmicDust | 57 | spiralEject |
| 24 | heartglow | 58 | clouds3d |
| 25 | evolution | 59 | pulseHeart |
| 26 | morph | 60 | rainUp |
| 27 | strangeAttractor | 61 | starsFlying |
| 28 | harmonicResonance | 62 | firework2 |
| 29 | snake | 63 | vuColumns |
| 30 | tetris | 64 | vuGrid |
| 31 | bouncingLogo | 65 | superHexagon |
| 32 | starTunnel3D | 66 | squareImplode |
| 33 | rippleRain | 67 | plasmaLightning |
| | | 68 | neonShockwaves ⭐ |
| | | 69 | emberVortex ⭐ |
| | | 70 | prismShards ⭐ |
| | | 71 | nebulaBreath ⭐ |

## The 30 showpiece modes 72–101 (🔥 new 2026-07-26, `anim_new3.h` + `anim_new4.h`)

| # | Name (`anim_*`) | # | Name (`anim_*`) | # | Name (`anim_*`) |
|---|-----------------|---|-----------------|---|-----------------|
| 72 | jellyfishBloom | 82 | quantumOrbitals | 92 | pulsarBeacon |
| 73 | kaleidoWheel | 83 | serpentNest | 93 | wormholeThroat |
| 74 | lightningTree | 84 | crystalGrowth | 94 | spirographArms |
| 75 | dnaHelix | 85 | solarFlare | 95 | honeycombPulse |
| 76 | fireflies | 86 | mandalaBloom | 96 | turbineBlades |
| 77 | blackHoleAccretion | 87 | inkBloom | 97 | mantaGlide |
| 78 | oceanCaustics | 88 | electricEel | 98 | plasmaGlobe |
| 79 | phoenixRise | 89 | galaxyCollision | 99 | supernovaShell |
| 80 | clockworkGears | 90 | gyroscope | 100 | cometSwarm |
| 81 | magneticDipole | 91 | doublePendulum | 101 | roseWindow |

⭐ = new (2026-07-26, `anim_new2.h`). Mode **48** was `earthWindow` (deleted) and is
now `auroraCurtains`. 🔥 **72–101** are the 30 showpiece modes in the spirit of
prismShards/nebulaBreath — differential-speed rotational arms (spirograph, gears,
gyroscope, radar-cousins), organic creatures (jellyfish, eel, manta, serpents,
phoenix), and physics/cosmic set-pieces (accretion disk, pulsar, supernova,
double-pendulum, plasma globe).

## The 5 next-tier modes 102–106 (🚀 new 2026-07-26, `anim_new5.h`)

| # | Name (`anim_*`) | New technique |
|---|-----------------|---------------|
| 102 | tesseract | 4D hypercube, projected 4D→3D→2D |
| 103 | torusKnot | 3D parametric (2,3) knot with depth shading |
| 104 | morphSwarm | particle swarm easing between figures (ring/heart/star/spiral/knot) |
| 105 | gravitationalLens | starfield warped by 1/r light-bending + Einstein/photon ring |
| 106 | celestialDragon | long follow-the-leader 3D serpent, depth = thickness+brightness |

## The Drahthaus concert pack 107–122 (🐘 new 2026-10-08, `anim_konzert.h`)

One mode per song of *Elephant in the Room* (album order), a setlist marker, and three
DJ modes. These are the **Drahthaus profile's** manual modes (the controller's setlist
walks them with `TOGA_CMD_MODESET`); in the Togalicht profile they are simply modes 107+.

| # | Name (`anim_*`) | Song / purpose |
|---|-----------------|----------------|
| 107 | konzertMarker | setlist position 1: the album-cover scene — lens-shaped submarine (filled panel + hot neon rim, domed tower, antenna, turning propeller, portholes on the strip lines) floating over a red desert (sky glowing toward the horizon, two dune-crest lines), four divers swaying below; bubbles rise, sand trickles from the hull, wind runs along the crests. 6 tubes switch on like real neon (single flashes → stuttering bursts → catch, ~8 s), then hum with rare relapses |
| 108 | dh100Elefanten | 100 Elefanten — 100 different pink elephants (mostly huge, every ~4th small), small set-back eyes, trunk raises and sprays water, behind green prairie grass |
| 109 | dhWhereDidYouSleep | Where Did You Sleep Last Night — **= Togalicht mode 8 (mitosis)**, same anim function |
| 110 | dhEcholot | Echolot — sonar pings, turning beam, echo blips |
| 111 | dhCryMeARiver | Cry Me A River — river streaming down, tears splashing |
| 112 | dhNichtErschrecken | Nicht erschrecken — thunderstorm every 0.3–4 s: leader + return stroke + re-strikes, forks of forks, double / cloud-to-cloud / sheet strikes |
| 113 | dhAlleGegenAlle | Alle gegen alle — **= Togalicht mode 10 (shockwaveChain)**, same anim function |
| 114 | dhSabbia1 | Sabbia I — sandstorm in red/orange/yellow |
| 115 | dhSabbia2 | Sabbia II — **= Togalicht mode 3 (vortex)**, same anim function |
| 116 | dhMuecke | Mücke — bug zapper: 1–3 mosquitoes; caught ones burn in crackling bursts of flashes (random size, sometimes full wall), fall to the floor in another colour; new ones only after all landed |
| 117 | dhRoentgenauge | Röntgenauge — x-ray eye: lids that close, blue eyeball, large orange-red iris, black limbus, white pupil, green horizontal + vertical scan lines |
| 118 | dhDienstagRave | Dienstag (noch immer dieser Rave) — **= quadMirror (120)**, same anim function |
| 119 | dhSong12 | placeholder for the 12th song (square tunnel) — rebuild once known |
| 120 | quadMirror | DJ: 4-fold mirrored blocks, crossfading on the beat |
| 121 | squareSpiral | DJ: square spiral drawing itself in and out |
| 122 | gridPulse | DJ: checkerboard pulse, parity flips per beat |

In the **Drahthaus profile** the five auto moods play different lists (`AUTO_MOODS_DH` in
[grid/grid.ino](grid/grid.ino)): only the strong Togalicht modes (100+ tier first, then the best
showpieces) plus the DJ modes 120–122. The **colour (hue base) is stored per mode** (`hueBaseByMode[]`, NVS `hbm`), like speed and
hue-speed. Strobe colour, global brightness and strobe speed are remembered **per profile**
(`Settings.pv[2]`, NVS key `pv`). The grid broadcasts the per-mode colour table once a second
(`TOGA_MSG_HUETAB`); the controller web UI shows a colour picker per setlist mode and sets it with
`TOGA_P_MODEHUE` (hex → FastLED hue − `TOGA_MODE_NATIVE_HUE[mode]`). Keep `TOGA_MODE_NATIVE_HUE`
in toga_proto.h in step with the MODE_TABLE hue column when rows change.

The text strobes (RAVE, ALLE GEGEN ALLE, Elefant, DHA/RTU/AHS, DRAHT/HAUS) are **not modes**: they
live in [grid/grid_text.h](grid/grid_text.h) `drawTextStrobe()` and are triggered by the
controller's text layer (`TogaCmdMsg.textStrobe`).

## Drahthaus test pack 123–126 (big lettering / word art, `anim_konzert.h`)

| # | Name (`anim_*`) | What |
|---|-----------------|------|
| 123 | dhRiesenschrift | "DRAHTHAUS" in giant 5×7 block letters (one font pixel = one grid square), smooth scroll, orange→yellow, shine band, copper drop shadow |
| 124 | dhElephantRoom | "ELEPHANT IN THE ROOM" as 90s word art: rainbow letters, 3-step extrusion, travelling sine wave |
| 125 | dhMonogramm | giant "DH" filling the wall, neon contour with a running light, beat punch, inverts every 8th beat |
| 126 | dhElefantenKopf | giant pink elephant head front view: flapping ears, blinking eyes, tusks, swinging trunk that trumpets water |

## 127 — Schwarz (dark standby, `anim_konzert.h`)

| # | Name (`anim_*`) | What |
|---|-----------------|------|
| 127 | schwarz | everything off except one small dot on the bottom-right crossing, breathing slowly in soft teal (follows the mode colour); bypasses showGrid so the AGC cannot blow it up; modules special-cased to black (`TOGA_MODE_SCHWARZ`). Drahthaus setlist: right before the submarine |

Module: `A_NEON` (row 107) and `A_STORM` (row 112) are new module archetypes that echo the wall.

`MODE_TABLE`, `MOD_MODE_COUNT`, `TOTAL_ANIMS` and `TOGA_EFFECT_COUNT` are all now **128** — the table is FULL
(hard limit 128: hue table, `TOGA_P_MODEHUE` arg = mode·256+hue in an int16). A further mode needs those widened first.
Auto-mood **5 (Spezial)** and Space/Show playlists include the 2026-07 modes ([grid/grid.ino](grid/grid.ino) `AUTO_*`).
