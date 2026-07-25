# Mode names — keep in sync

The grid runs **72 animations** (indices `0..71`). The mode **index and order are
the contract**: a module in FOLLOW mode renders row `k` of its `MODE_TABLE` while
the grid runs anim `k`. If the order changes in one place and not the other, every
strip cites the wrong animation from the changed index onward.

## The two authoritative lists (must match 1:1)

| # | Place | What it holds | Count symbol |
|---|-------|---------------|--------------|
| 1 | [grid/grid.ino](grid/grid.ino) — `case 0..71:` in the anim switch (~line 888) | the real `anim_*()` called per index | `TOTAL_ANIMS` = 72 ([grid/grid_config.h:44](grid/grid_config.h#L44)) |
| 2 | [module/module_effects.h](module/module_effects.h) — `MODE_TABLE[]` (~line 78) | one `ModeSpec` row per grid anim, same order | `MOD_MODE_COUNT` = 72 ([module/module_effects.h:77](module/module_effects.h#L77)) |

**Invariant:** `TOTAL_ANIMS == MOD_MODE_COUNT` **and** row `k` of `MODE_TABLE`
describes the same animation as `case k:` in `grid.ino`.

> The **module-manager UI** ([controller/controller.ino](controller/controller.ino)) refers to
> modes **by index only** — it carries no name list, so it needs no edit when names
> change. But if you **add / remove / reorder** modes, bump the index range there too.

## When something changes — checklist

- **Add a mode:** append `case 68:` in `grid.ino`, append row 68 to `MODE_TABLE`,
  bump `TOTAL_ANIMS` **and** `MOD_MODE_COUNT` to 69 (check `MAX_MODES` ≥ new count).
- **Remove a mode:** delete the `case` **and** the `MODE_TABLE` row, renumber every
  later comment, decrement both counts. Never leave a hole.
- **Reorder / rename:** change **both** lists together and re-check the four citation
  axes (colour / density / motion / rhythm) for each moved row.
- After any change, confirm the trailing `/* k name */` comment in `MODE_TABLE`
  matches the `anim_*` at `case k:`.

## The 72 modes (source of truth = grid.ino case order)

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

⭐ = new 2026-07-26. Mode **48** was `earthWindow` (deleted per request) and is now
`auroraCurtains`, the first of 5 new energy/plasma modes in the style of
`plasmaLightning` (67). The other four are **68–71** (`anim_new2.h`). Row **30** is
`tetris`. `MODE_TABLE` (module_effects.h) has matching rows 0..71 (MOD_MODE_COUNT=72);
`TOGA_EFFECT_COUNT` in toga_proto.h is also 72.
