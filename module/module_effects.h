// ══════════════════════════════════════════════════════════════
//  module_effects.h — 68 resolution-independent 1D modes (one per grid anim).
//  (#included by module.ino after the globals; do not compile alone.)
//
//  A module in FOLLOW mode renders module mode k for grid mode k (0..67),
//  1:1 — MODE_TABLE is kept in the EXACT order of the grid's animation switch,
//  so row k always cites the animation the wall is actually running. Each mode
//  is a self-contained PROCEDURAL 1D animation — never a
//  downsample of the grid's framebuffer (there are no pixels on the wire).
//  Visual identity with the 2D grid is impossible and not the goal; a
//  recognisable CITATION is: matching motion, direction, symmetry, rhythm,
//  colour family and density.
//
//  Architecture: 24 parametrised archetype renderers + a MODE_TABLE[68]
//  assigning one archetype + params to each grid mode.
//
//  RESOLUTION INDEPENDENCE (every archetype, no exceptions):
//   · all geometry in normalised p∈[0,1]; converted to LEDs only at the write
//   · sub-pixel ANTI-ALIASED writes (modPix) — a moving dot glides at n=20
//   · element counts scale with n (modCount), floored at 1 so 12px is never blank
//   · lengths are FRACTIONS of the strip; rates are strip-lengths per second
//   · n<MOD_DEGENERATE_N: positional archetypes fall back to whole-strip modulation
//   · every (n-1) divisor guarded; n==1 renders one modulated pixel
//
//  Renderers paint NATIVE hues into fx[] only. showModule() rotates fx→leds by
//  (gHueBase+gHueAuto) and applies gBeatGlow on the way out — so a renderer must
//  never read those globals nor touch leds[], or the tint double-applies.
//
//  Panels (h>1): the 1D coordinate is the COLUMN; the same value is written down
//  the whole column via ledXY(). n for all maths is gWidth, not gWidth*gHeight.
// ══════════════════════════════════════════════════════════════
#pragma once

// ── Archetype ids (index into the switch in modRender) ──
// New ids are APPENDED after A_BLINK so every existing id stays stable and no
// MODE_TABLE row shifts.
enum {
  A_SOLID=0, A_GRADIENT, A_WAVE, A_PLASMA, A_NOISE, A_RAIN, A_TWINKLE, A_COMET,
  A_PULSE, A_SWEEP, A_BURST, A_FLASH, A_STRIPES, A_BLOCKS, A_CHASE, A_VU,
  A_FIRE, A_BLINK,
  A_MARQUEE, A_QUAKE, A_COLLIDE, A_SWARM, A_SPIN, A_GAZE,
  A_NEON, A_STORM          // Drahthaus: neon tube striking (107), thunderstorm (112)
};

// ── Flags ──
#define FLAG_SYM      0x01   // mirror around strip centre
#define FLAG_REVERSE  0x02   // invert direction of travel
#define FLAG_BEAT     0x04   // phase-lock motion to gBeatPhase
#define FLAG_RAINBOW  0x08   // hue sweeps the wheel across p
#define FLAG_SPARKLE  0x10   // overlay sparse white glitter
#define FLAG_DIM      0x20   // render at reduced amplitude

struct ModeSpec {
  uint8_t arch;     // archetype id
  uint8_t hue;      // base hue; 255 = per-element pseudo-random hue
  uint8_t sat;      // base saturation
  uint8_t speed;    // relative rate, 100 = 1.0x
  uint8_t density;  // elements per 100 LEDs, or an archetype-specific count/rate
  uint8_t flags;    // bitfield
  uint8_t p1, p2;   // archetype-specific
};

// Authoritative mapping — ONE ROW PER GRID ANIMATION, in the EXACT order of the
// grid's mode switch (grid.ino case 0..66). A module in FOLLOW mode renders row
// k while the grid runs anim k, so the trailing name IS the grid function at
// that index; MOD_MODE_COUNT must equal the grid's TOTAL_ANIMS. Each row is
// tuned to CITE its animation on the four axes the eye reads across a 2D→1D gap:
//   · COLOUR   — native hue family/saturation of the grid mode (both grid and
//                module rotate output by the same gHueBase+gHueAuto, so a native
//                hue here lands on the same wheel position the wall shows).
//   · DENSITY  — on/off ratio: full-field grid modes use whole-strip archetypes
//                (WAVE/PLASMA/NOISE/GRADIENT/VU), sparse ones use particle
//                archetypes at a matching element count (RAIN/TWINKLE/COMET/BURST).
//   · MOTION   — radial→SYM, inward→SYM|REVERSE, rotation→SPIN/SWEEP, linear→
//                RAIN/CHASE/MARQUEE, explosive→BURST, chaotic→COMET/SWARM.
//   · RHYTHM   — steady vs pulsing vs stochastic vs beat-locked, via speed/flags.
// PROGMEM: read one row per frame.
#define MOD_MODE_COUNT 128
static const ModeSpec MODE_TABLE[MOD_MODE_COUNT] PROGMEM = {
  /* 0 setupGrid       */ { A_BLINK,     0,  0, 100,  0, 0,                             0, 3 },  // static red/green lattice (special-cased in modRender)
  /* 1 heartbeat       */ { A_PULSE,     0,255, 180,  0, FLAG_SYM,                     25, 4 },  // red double-thump rings from centre (more/wider rings, low-bri visible)
  /* 2 binaryRain      */ { A_COMET,    96,220, 110,  3, FLAG_SPARKLE,                 60, 0 },  // green matrix comets falling (count fixed, LED-independent)
  /* 3 vortex          */ { A_COMET,   190,220,  90,  3, FLAG_REVERSE,                 70, 0 },  // blue-purple flow outer→inner (END→START) like the grid spiral
  /* 4 earthquake      */ { A_QUAKE,    25,200, 100,  0, 0,                            30,70 },  // amber seismic ripples + jolts
  /* 5 supernova       */ { A_BURST,    15,200, 110,  0, FLAG_SYM,                    110,14 },  // red-white star blast + radial sparks from centre (fewer sparks)
  /* 6 blackHole       */ { A_PLASMA,   30,230,  60,  0, 0,                            40,90 },  // 2-hue accretion gradient: amber outer → centre hue (span 90)
  /* 7 wormhole        */ { A_PULSE,   200,240, 150,  0, FLAG_SYM|FLAG_REVERSE,        40, 4 },  // violet rings tunnelling inward
  /* 8 mitosis         */ { A_PULSE,    90,200,  20,  0, FLAG_SYM,                     60, 2 },  // slow green cell split/breathe
  /* 9 collider        */ { A_COLLIDE, 130,255,  46,  0, 0,                            0, 0 },   // two beams converge → flash
  /*10 shockwaveChain  */ { A_COMET,   210,220,  90,  3, FLAG_SYM,                     80, 0 },  // blue-purple comet flow emanating from centre
  /*11 pulsar          */ { A_SWEEP,   150,200, 220,  0, FLAG_SYM,                     40,60 },  // fast cyan beam radiating from the centre lamp
  /*12 liquidMetal     */ { A_PLASMA,   20,170,  45,  0, 0,                            40,60 },  // slow desaturated molten-metal drift
  /*13 coriolisStorm   */ { A_WAVE,    148,240,  80,  0, FLAG_SYM|FLAG_SPARKLE,          0, 5 },  // symmetric blue storm bands (bri/colour spread) + sparkle
  /*14 aurora          */ { A_NOISE,    85,200,  55,  0, FLAG_RAINBOW,                 45,16 },  // green→magenta shimmering curtains
  /*15 lavaLamp        */ { A_COMET,   255,230,  40,  3, FLAG_SYM,                    110, 1 },  // slow multicolour blobs drifting out/in toward centre (per-blob hue)
  /*16 nerveSystem     */ { A_COMET,   255,220, 220,  6, 0,                            50, 1 },  // random synaptic sparks darting/bouncing
  /*17 gravityLens     */ { A_WAVE,    180,220,  10,  0, 0,                             0, 3 },  // very slow blue warped field
  /*18 cymatics        */ { A_VU,       60,220, 140,  0, 0,                             0,60 },  // fast sloshing level bar (accents, not all-on)
  /*19 starfieldWarp   */ { A_COMET,   200,180, 180,  2, FLAG_SYM|FLAG_SPARKLE,        45, 0 },  // very few spark-tail points flying fore+aft from centre
  /*20 concentricSquares*/{ A_BLOCKS,    0,220,  60,  0, FLAG_SYM|FLAG_RAINBOW,         6, 0 },  // blocky STEPPED rainbow rings reaching to the ends (not a smooth chase)
  /*21 squareRain      */ { A_TWINKLE, 255,220,  20, 50, 0,                            40,220},  // near-static multicolour distribution (grid doesn't move → speed way down)
  /*22 depthMap        */ { A_GRADIENT,  0,200,  30,  1, FLAG_SYM,                    160, 0 },  // radial red-centre → blue-edge, breathing
  /*23 cosmicDust      */ { A_NOISE,   210,205,  45,  0, 0,                            50,10 },  // soft blue-purple nebula drift
  /*24 heartglow       */ { A_PULSE,     5,255,  40,  0, FLAG_SYM,                     60, 1 },  // slow warm heartbeat glow in the dark
  /*25 evolution       */ { A_PLASMA,  100,220,  50,  0, FLAG_SYM|FLAG_SPARKLE|FLAG_DIM, 40,90 },  // central flowy flicker (dimmer, sparkle)
  /*26 morph           */ { A_PLASMA,  150,210,  55,  0, FLAG_SYM|FLAG_SPARKLE|FLAG_DIM, 45,90 },  // central flowy flicker (dimmer, sparkle) — as mode 25
  /*27 strangeAttractor*/ { A_COMET,    90,220, 180,  1, FLAG_SYM|FLAG_SPARKLE,        70, 0 },  // central chaotic comet + sparkle flicker
  /*28 harmonicResonance*/{ A_PLASMA,  170,210,  80,  0, 0,                            50,90 },  // throbbing interference field
  /*29 snake           */ { A_COMET,    96,220, 143,  1, 0,                            40, 0 },  // green worm head + trailing body
  /*30 iconShow        */ { A_MARQUEE,  40,230,  60,  0, FLAG_RAINBOW,                 77, 8 },  // glowing emblem cells, hue cycling
  /*31 bouncingLogo    */ { A_COMET,     0,230, 120,  1, 0,                            40, 1 },  // slow bouncing logo dot
  /*32 starTunnel3D    */ { A_COMET,   160,170, 120,  3, FLAG_SYM,                     50, 0 },  // symmetric point-pairs held apart from centre (a cut through the squares)
  /*33 rippleRain      */ { A_PULSE,   150,230, 110,  0, 0,                            40, 3 },  // raindrop rings expanding
  /*34 spiralGalaxy    */ { A_SPIN,    165,200,  50,  0, 0,                           200, 0 },  // slow turning galaxy, blue→pink
  /*35 boids           */ { A_SWARM,   110,220, 100, 12, 0,                            40, 0 },  // flocking swarm of firefly dots
  /*36 fireworks       */ { A_BURST,   255,230, 110,  0, 0,                            40,14 },  // random colour bursts + fall
  /*37 textTOGA        */ { A_MARQUEE,  40,255,  90,  0, 0,                            41, 6 },  // amber scrolling-text banner feel
  /*38 stickman        */ { A_CHASE,    35,180, 110,  0, 0,                            50,10 },  // amber runner scrolling horizontally
  /*39 paddleBall      */ { A_COMET,    40,220, 110,  1, 0,                            25, 1 },  // bouncing ball
  /*40 smiley          */ { A_PULSE,    40,235,  40,  0, FLAG_SYM,                     60, 2 },  // gentle smooth warm glow from centre (no flashy blink)
  /*41 eye             */ { A_GAZE,    112,225,  90,  0, 0,                             0, 0 },  // teal iris on cream, saccade + blink
  /*42 starfield2      */ { A_COMET,   160,180, 110,  2, FLAG_SYM,                     50, 0 },  // very few sparks flying from centre outward both ways
  /*43 warp            */ { A_COMET,   160,180, 150,  3, FLAG_SYM,                    120, 0 },  // long spark streaks flying outward from centre
  /*44 shootingStars   */ { A_TWINKLE, 150,180, 110, 40, FLAG_SPARKLE,                 30,120},  // quiet blue twinkle field + meteors
  /*45 blackHole2      */ { A_PLASMA,    0,230,  70,  0, FLAG_SYM,                     40,255},  // central-origin full-spectrum plasma
  /*46 pulsar2         */ { A_SWEEP,   160,200,  57,  2, FLAG_SYM,                     25,60 },  // twin blue beams radiating from the centre
  /*47 horsehead       */ { A_CHASE,    24,200,  80,  0, 0,                            45,25 },  // amber directional chase front→back (riding the river)
  /*48 auroraCurtains  */ { A_WAVE,    110,210,  70,  0, FLAG_RAINBOW,                 50,10 },  // flowing aurora colour bands (green→teal→magenta)
  /*49 radar           */ { A_SWEEP,    96,255,  37,  0, FLAG_SYM,                     20,70 },  // green radar sweep radiating from the centre
  /*50 sineWaves       */ { A_WAVE,    150,220,  79,  0, 0,                             0, 3 },  // aqua interfering sine bands
  /*51 rocketFire      */ { A_FIRE,     45,255, 110,  0, FLAG_REVERSE,                 60,120},  // roaring flame plume (downward)
  /*52 warningLight    */ { A_SWEEP,    38,140,  72,  0, FLAG_SYM,                     40,50 },  // pale amber lighthouse beam from the centre lamp
  /*53 aliens          */ { A_SWARM,    95,220,  90,  8, 0,                            30,200},  // green cartoon aliens hopping
  /*54 rain2           */ { A_RAIN,    150,120, 150, 25, FLAG_DIM,                     45,30 },  // sparser dimmer real drops (not all-on)
  /*55 levitate        */ { A_RAIN,     40, 90,  74, 22, FLAG_REVERSE|FLAG_DIM,        50,40 },  // sparser dimmer warm embers drifting up
  /*56 confetti        */ { A_TWINKLE, 255,230, 100, 60, 0,                            40,150},  // festive multicolour confetti
  /*57 spiralEject     */ { A_BURST,   180,220, 100,  0, FLAG_SYM,                    100,16 },  // purple spinning galaxy flinging sparks
  /*58 clouds3d        */ { A_NOISE,   150,180,  45,  0, FLAG_SPARKLE,                 50,10 },  // drifting pale-blue haze + sparkle
  /*59 pulseHeart      */ { A_PULSE,     0,240, 100,  0, FLAG_SYM|FLAG_BEAT,           50, 1 },  // beating red heart (beat-locked)
  /*60 rainUp          */ { A_COMET,   150,120, 150,  3, FLAG_REVERSE|FLAG_SPARKLE,    60, 0 },  // sparkly upward comets rising (fewer LEDs)
  /*61 starsFlying     */ { A_TWINKLE, 160,180, 100, 50, 0,                            40,120},  // omnidirectional twinkling starfield
  /*62 firework2       */ { A_BURST,   255,220, 100,  0, 0,                           130,10 },  // launching + bursting fireworks
  /*63 vuColumns       */ { A_VU,       96,255, 100,  0, FLAG_BEAT,                     0,60 },  // green→red VU bars (beat-locked)
  /*64 vuGrid          */ { A_VU,       96,255, 100,  0, FLAG_BEAT,                     1,60 },  // dual-axis VU grid (beat-locked)
  /*65 superHexagon    */ { A_PULSE,     0,220, 180,  0, FLAG_SYM|FLAG_REVERSE|FLAG_RAINBOW, 70, 6 },  // hex walls rushing inward, reaching further to the middle
  /*66 squareImplode   */ { A_BLOCKS,    0,220,  90,  0, FLAG_SYM|FLAG_REVERSE|FLAG_RAINBOW,  6, 0 },  // blocky STEPPED implode (not a smooth chase)
  /*67 plasmaLightning */ { A_BLOCKS,  150,230, 110,  0, 0,                             6, 0 },  // blocky blue stepping (less sparky)
  /*68 neonShockwaves  */ { A_BURST,   190,255, 120,  0, FLAG_SYM,                    100,14 },  // colliding neon rings from the centre
  /*69 emberVortex     */ { A_SPIN,     18,240, 100,  0, 0,                            40,60 },  // swirling fire/ember vortex
  /*70 prismShards     */ { A_BURST,     0,220, 100,  0, FLAG_SYM|FLAG_RAINBOW,       100,16 },  // rainbow crystal shards explode/reassemble
  /*71 nebulaBreath    */ { A_NOISE,   200,180,  30,  0, FLAG_SPARKLE,                 55,10 },  // breathing deep-space nebula + stars
  /*72 jellyfishBloom  */ { A_PULSE,   150,190,  80,  0, FLAG_SYM,                     45, 3 },  // soft teal bell pulse + drifting tentacles
  /*73 kaleidoWheel    */ { A_SPIN,      0,220,  90,  0, FLAG_RAINBOW,                180, 0 },  // spinning jewelled kaleidoscope
  /*74 lightningTree   */ { A_BURST,   160,120, 130,  0, FLAG_SPARKLE,                 90,14 },  // white-blue fractal bolt flashes
  /*75 dnaHelix        */ { A_CHASE,   170,210,  90,  0, FLAG_SYM,                     45,12 },  // twin twisting strands + rungs
  /*76 fireflies       */ { A_TWINKLE,  50,200,  90, 40, 0,                            40,150},  // warm swarm blinking in/out of sync
  /*77 blackHoleAccr.  */ { A_SPIN,     25,230,  90,  0, 0,                            40,60 },  // whirling infalling accretion disk
  /*78 oceanCaustics   */ { A_NOISE,   130,200,  70,  0, 0,                            45,12 },  // shimmering blue-green light veins
  /*79 phoenixRise     */ { A_FIRE,     20,255, 110,  0, 0,                            60,120},  // beating molten firebird wings
  /*80 clockworkGears  */ { A_SPIN,     30,200,  80,  0, 0,                            60, 0 },  // interlocking brass gears turning
  /*81 magneticDipole  */ { A_GRADIENT,  0,200,  40,  1, FLAG_SYM,                    160, 0 },  // red↔blue looping field lines
  /*82 quantumOrbitals */ { A_PLASMA,  190,200,  60,  0, FLAG_SPARKLE,                 45,90 },  // morphing violet electron clouds
  /*83 serpentNest     */ { A_SWARM,    90,220, 100, 10, 0,                            40, 0 },  // tangle of gliding snakes
  /*84 crystalGrowth   */ { A_BURST,   255,200,  70,  0, FLAG_RAINBOW,                 90,16 },  // faceted crystals blooming/dissolving
  /*85 solarFlare      */ { A_PLASMA,   20,230,  70,  0, 0,                            40,90 },  // churning sun + erupting coronal loop
  /*86 mandalaBloom    */ { A_SPIN,     40,210,  60,  0, FLAG_RAINBOW,                150, 0 },  // breathing counter-rotating petals
  /*87 inkBloom        */ { A_PULSE,   255,210,  60,  0, FLAG_SYM,                     60, 3 },  // ink drops diffusing in water
  /*88 electricEel     */ { A_COMET,   145,200, 110,  1, FLAG_SPARKLE,                 45, 0 },  // sine-body eel firing sparks
  /*89 galaxyCollision */ { A_SPIN,    200,200,  70,  0, 0,                           160, 0 },  // two spiral galaxies merging
  /*90 gyroscope       */ { A_SPIN,    120,200,  90,  0, 0,                           140, 0 },  // nested rings tumbling in 3D
  /*91 doublePendulum  */ { A_COMET,   200,210, 150,  1, FLAG_SPARKLE,                 50, 0 },  // chaotic swinging tip trail
  /*92 pulsarBeacon    */ { A_SWEEP,   155,180, 120,  0, FLAG_SYM,                     25,60 },  // twin beams + expanding shells
  /*93 wormholeThroat  */ { A_PULSE,   190,210, 120,  0, FLAG_SYM|FLAG_REVERSE,        45, 4 },  // rings streaming inward down a throat
  /*94 spirographArms  */ { A_SPIN,     60,200, 110,  0, 0,                           130, 0 },  // differential arms tracing loops
  /*95 honeycombPulse  */ { A_BLOCKS,    0,220,  80,  0, FLAG_SYM|FLAG_RAINBOW,         6, 0 },  // radial colour wave through cells
  /*96 turbineBlades   */ { A_SPIN,     30,190, 160,  0, 0,                            90, 0 },  // spinning curved turbine blades
  /*97 mantaGlide      */ { A_COMET,   180,180,  70,  1, 0,                            40, 1 },  // gliding ray + glowing tail spots
  /*98 plasmaGlobe     */ { A_SWEEP,   200,170,  90,  0, FLAG_SYM|FLAG_SPARKLE,        40,60 },  // electric tendrils from the core
  /*99 supernovaShell  */ { A_BURST,    10,210, 100,  0, FLAG_SYM,                    110,14 },  // flash + expanding filament shell
  /*100 cometSwarm     */ { A_COMET,   160,190, 100,  4, 0,                            45, 0 },  // comets orbiting a blazing star
  /*101 roseWindow     */ { A_SPIN,      0,220,  60,  0, FLAG_RAINBOW,                150, 0 },  // rotating stained-glass rose
  /*102 tesseract      */ { A_SPIN,    160,200,  90,  0, 0,                           140, 0 },  // rotating 4D hypercube wireframe
  /*103 torusKnot      */ { A_SPIN,    190,210, 100,  0, FLAG_RAINBOW,                150, 0 },  // glowing 3D torus knot
  /*104 morphSwarm     */ { A_SWARM,     0,210,  90, 14, FLAG_RAINBOW,                 40, 0 },  // swarm morphing between figures
  /*105 gravLens       */ { A_TWINKLE, 200,120,  80, 40, FLAG_SPARKLE,                 35,120},  // starfield bent around a black hole
  /*106 celestialDragon*/ { A_COMET,   150,200, 100,  1, 0,                            50, 0 },  // long 3D serpent-dragon
  // ── Drahthaus concert pack (107..122) — grid: animations/anim_konzert.h ──
  /*107 konzertMarker  */ { A_NEON,     18,230, 100,  0, 0,                              0, 0 },  // the submarine marker: an orange neon tube that switches on like the wall's
  /*108 dh100Elefanten */ { A_MARQUEE, 238,170,  70,  0, 0,                             60, 8 },  // pink procession passing through
  /*109 dhWhereDidYouSleep*/{ A_PULSE,  90,200,  20,  0, FLAG_SYM,                     60, 2 },  // = mitosis (row 8): slow green cell split/breathe
  /*110 dhEcholot      */ { A_BURST,   120,230,  90,  0, FLAG_SYM|FLAG_BEAT,           90, 6 },  // sonar pings radiating from the centre
  /*111 dhCryMeARiver  */ { A_RAIN,    140,220,  60,  6, 0,                            55, 0 },  // teal river streaming down, drops
  /*112 dhNichtErschrecken*/{ A_STORM, 180,140, 100,  0, 0,                              0, 0 },  // thunderstorm: random multi-flash strikes, white/blue/violet
  /*113 dhAlleGegenAlle*/ { A_COMET,   210,220,  90,  3, FLAG_SYM,                     80, 0 },  // = shockwaveChain (row 10): blue-purple comet flow from centre
  /*114 dhSabbia1      */ { A_NOISE,    14,230,  90,  0, FLAG_SPARKLE,                 40,12 },  // sandstorm: red-orange dunes drifting, grainy
  /*115 dhSabbia2      */ { A_COMET,   190,220,  90,  3, FLAG_REVERSE,                 70, 0 },  // = vortex (row 3): blue-purple flow outer→inner
  /*116 dhMuecke       */ { A_COMET,    60,120, 160,  1, 0,                            25, 0 },  // one tiny erratic bright dot
  /*117 dhRoentgenauge */ { A_GAZE,     28,200,  80,  0, 0,                           140, 0 },  // x-ray eye: amber iris gaze
  /*118 dhDienstagRave */ { A_BLOCKS,  255,230, 110,  0, FLAG_SYM|FLAG_BEAT,            6, 0 },  // = quadMirror (row 120): mirrored random blocks on the beat
  /*119 dhSong12       */ { A_SWEEP,   180,200,  80,  0, FLAG_SYM,                     40,50 },  // placeholder: square tunnel outward
  /*120 quadMirror     */ { A_BLOCKS,  255,230, 110,  0, FLAG_SYM|FLAG_BEAT,            6, 0 },  // DJ: mirrored random blocks on the beat
  /*121 squareSpiral   */ { A_CHASE,   200,220, 100,  0, FLAG_RAINBOW,                 40, 0 },  // DJ: a spiral drawing itself in and out
  /*122 gridPulse      */ { A_STRIPES,   0,220, 120,  0, FLAG_BEAT|FLAG_RAINBOW,       32,50 },  // DJ: 8 scrolling bands = the checkerboard pulse
  // ── Drahthaus test pack (123..126) ──
  /*123 dhRiesenschrift*/ { A_MARQUEE,  28,230,  60,  0, 0,                            90, 6 },  // giant DRAHTHAUS letters scrolling: orange blocks marching
  /*124 dhElephantRoom */ { A_MARQUEE,   0,230,  70,  0, FLAG_RAINBOW,                 55,10 },  // word-art rainbow letters marching
  /*125 dhMonogramm    */ { A_PULSE,    28,230, 100,  0, FLAG_SYM|FLAG_BEAT,           90, 2 },  // DH monogram pulsing on the beat
  /*126 dhElefantenKopf*/ { A_PULSE,   236,170,  50,  0, FLAG_SYM,                    120, 2 },  // giant pink elephant head: slow flapping breath
  /*127 schwarz        */ { A_SOLID,   130,170, 100,  0, 0,                              0, 0 },  // dark standby — special-cased to BLACK in modRender (row only keeps the count)
};

// ══════════════════════════════════════════════════════════════
//  LOCAL (independent) modes — v10. Rendered when a module is NOT following the
//  grid (effectPin != FOLLOW). Purpose-built to look good on ONE strip: the
//  first six wear a FIXED colour (native hue 0, so showModule's local-hue
//  rotation paints them the picked colour) and pulsate/fade; the last four are
//  self-cycling RAINBOW modes that sweep the wheel on their own clock. Simple,
//  not a citation of any grid mode. The order MUST match togaLocalEffectName()
//  in toga_proto.h row-for-row (index k ⇄ TOGA_EFFECT_LOCAL_BASE + k).
//   arch          hue sat  spd dens flags                p1   p2
static const ModeSpec LOCAL_MODES[TOGA_LOCAL_COUNT] PROGMEM = {
  /* 0 Solid       */ { A_SOLID,    0,255, 100,  0, 0,                      0,   0 },  // steady fixed colour
  /* 1 Atmen       */ { A_SOLID,    0,255,  36,  0, 0,                    210,   0 },  // slow breathe: pulsate + fade in colour
  /* 2 Puls        */ { A_PULSE,    0,255,  90,  0, FLAG_SYM,             45,   3 },  // rings swelling out from centre, fading
  /* 3 Komet       */ { A_COMET,    0,230, 120,  1, 0,                     55,   0 },  // one head gliding, soft fading tail
  /* 4 Funkeln     */ { A_TWINKLE,  0,210, 100, 55, 0,                     40, 150 },  // sparse sparkles fading in the colour
  /* 5 Feuer       */ { A_FIRE,    45,255, 110,  0, 0,                     60, 120 },  // warm flame flicker (tints with colour)
  /* 6 Regenbogen  */ { A_GRADIENT, 0,255,  55,  0, 0,                      0,   0 },  // whole strip one hue, cycling the wheel
  /* 7 Rb-Fluss    */ { A_GRADIENT, 0,255,  70,  0, 0,                    255,   0 },  // full rainbow flowing along the strip
  /* 8 Rb-Plasma   */ { A_PLASMA,   0,220,  55,  0, 0,                     40, 255 },  // soft full-spectrum plasma drift
  /* 9 Rb-Funkeln  */ { A_TWINKLE, 255,230, 100, 60, 0,                     40, 150 },  // multicolour confetti sparkles
  /*10 Fließen     */ { A_WAVE,     0,255,  79,  0, 0,                      0,   3 },  // smooth bright bands flowing along the strip (floor dir)
  /*11 Lauflicht   */ { A_CHASE,    0,255, 110,  0, 0,                     50,  10 },  // running dots chasing down the strip (floor dir)
};

// Local effects whose motion has a clear direction along the strip → the floor
// setting (▶/◀) flips them. The rest (Solid/Atmen/Puls/Funkeln/Feuer/plasma) have
// no meaningful travel direction and are left alone. Applied after render below.
static inline bool localIsDirectional(uint8_t idx){
  return idx == 3 || idx == 7 || idx == 10 || idx == 11;   // Komet, Rb-Fluss, Fließen, Lauflicht
}
// Mirror the rendered frame across the strip centre (per column, so panels flip too).
// A spatial reflection reverses apparent travel direction without touching any
// archetype's internals — one place to honour the floor direction for every effect.
static void modMirrorX(){
  for(uint16_t c = 0; c < gWidth / 2; c++){
    uint16_t c2 = gWidth - 1 - c;
    for(uint16_t y = 0; y < gHeight; y++){ CRGB t = ledXY(c, y); ledXY(c, y) = ledXY(c2, y); ledXY(c2, y) = t; }
  }
}

// Dance auto-programs (v10): each cycles a curated list of LOCAL_MODES indices,
// one every AUTO_DWELL_MS of VIRTUAL time (Speed scales it). Order matches
// togaAutoProgName(): 0 Mix, 1 Farbe (fixed-colour fades), 2 Regenbogen.
#define AUTO_DWELL_MS  14000u
static const uint8_t AUTO_MIX[]     = { 1, 2, 3, 4, 6, 7, 9, 8 };   // colour fades + rainbows
static const uint8_t AUTO_COLOR[]   = { 1, 2, 3, 4 };               // pulsate/fade in the fixed colour
static const uint8_t AUTO_RAINBOW[] = { 6, 7, 8, 9 };               // the rainbow modes
struct AutoProg { const uint8_t* list; uint8_t len; };
static const AutoProg AUTO_PROGS[TOGA_LOCAL_AUTO_COUNT] = {
  { AUTO_MIX,     (uint8_t)(sizeof(AUTO_MIX))     },
  { AUTO_COLOR,   (uint8_t)(sizeof(AUTO_COLOR))   },
  { AUTO_RAINBOW, (uint8_t)(sizeof(AUTO_RAINBOW)) },
};

// ── Per-frame render state (set by modRender before dispatch) ──
static uint16_t Mn        = 1;        // strip length in COLUMNS (gWidth)
static bool     Msym      = false;    // FLAG_SYM active this frame
static bool     gReset    = true;     // mode changed since last frame → reseed state
static uint16_t gCurMode  = 0xFFFF;
static uint16_t gLastMode = 0xFFFF;
static uint32_t gDtV      = 0;        // virtual ms elapsed since last frame (speed-scaled)

// ── Persistent archetype state (reset on mode change) ──
static uint8_t  gHeat[MAX_LEDS];                 // FIRE
static float    gLife[MOD_MAX_ELEMENTS];         // TWINKLE life 1→0
static uint16_t gLifePos[MOD_MAX_ELEMENTS];
static uint8_t  gLifeHue[MOD_MAX_ELEMENTS];

// ══════════════════════════════════════════════════════════════
//  Write helpers. Everything funnels through modColRGB, which writes the
//  whole COLUMN via the bounds-checked ledXY — so panels (h>1) show the 1D
//  animation as vertical bars and an out-of-range column is harmless.
// ══════════════════════════════════════════════════════════════
static inline float clamp01(float x){ return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }

static inline void modColRGB(int cx, uint8_t r, uint8_t g, uint8_t b){
  if(cx < 0 || cx >= (int)gWidth) return;
  for(uint16_t y = 0; y < gHeight; y++){
    CRGB& p = ledXY(cx, y);
    p.r = qadd8(p.r, r); p.g = qadd8(p.g, g); p.b = qadd8(p.b, b);
  }
}
static inline void modColA(int cx, const CRGB& c, float amp){
  if(amp <= 0.f) return; if(amp > 1.f) amp = 1.f;
  modColRGB(cx, (uint8_t)(c.r*amp), (uint8_t)(c.g*amp), (uint8_t)(c.b*amp));
}
static inline void modWhole(const CRGB& c, float amp){
  for(int i = 0; i < (int)Mn; i++) modColA(i, c, amp);
}
// Anti-aliased single-point write in LED space (§3.2 rule 2).
static inline void splatLed(float fpos, const CRGB& c, float amp){
  if(amp <= 0.f) return; if(amp > 1.f) amp = 1.f;
  int i0 = (int)floorf(fpos); float fr = fpos - i0;
  modColRGB(i0,   (uint8_t)(c.r*amp*(1.f-fr)), (uint8_t)(c.g*amp*(1.f-fr)), (uint8_t)(c.b*amp*(1.f-fr)));
  modColRGB(i0+1, (uint8_t)(c.r*amp*fr),       (uint8_t)(c.g*amp*fr),       (uint8_t)(c.b*amp*fr));
}
// Anti-aliased point at normalised p. Under SYM, p is DISTANCE FROM CENTRE
// (0=centre, 1=edge) and is mirrored to both halves — the one cue that reads
// as "same thing as a radial grid animation".
static inline void modPix(float p, const CRGB& c, float amp){
  p = clamp01(p);
  if(!Msym){ splatLed(p * (Mn > 1 ? Mn - 1 : 0), c, amp); }
  else {
    float ctr = (Mn - 1) * 0.5f, off = p * ctr;
    splatLed(ctr + off, c, amp);
    if(off > 0.001f) splatLed(ctr - off, c, amp);
  }
}
// Anti-aliased span fill in normalised p (non-SYM archetypes only).
static inline void modBand(float p0, float p1, const CRGB& c, float amp){
  if(Mn == 0) return;
  float a = p0 * (Mn - 1), b = p1 * (Mn - 1);
  if(b < a){ float t = a; a = b; b = t; }
  int i0 = (int)floorf(a), i1 = (int)ceilf(b);
  for(int i = i0; i <= i1; i++){
    if(i < 0 || i >= (int)Mn) continue;
    float lo = fmaxf((float)i, a), hi = fminf((float)i + 1.f, b);
    float cover = hi - lo; if(cover <= 0.f) continue; if(cover > 1.f) cover = 1.f;
    modColA(i, c, amp * cover);
  }
}
// Soft symmetric-aware front of half-width widP centred on posP.
static inline void drawFront(float posP, float widP, const CRGB& c, float amp){
  if(widP < 1e-4f) widP = 1e-4f;
  float step = 1.f / (Mn > 1 ? Mn - 1 : 1);
  int reach = (int)ceilf(widP / step) + 1;
  for(int d = -reach; d <= reach; d++){
    float pp = posP + d * step;
    if(pp < 0.f || pp > 1.f) continue;
    float f = 1.f - fabsf(d * step) / widP; if(f <= 0.f) continue;
    modPix(pp, c, amp * f);
  }
}

// ── Small numeric helpers ──
static inline uint32_t spd(const ModeSpec& s){ return s.speed ? s.speed : 100; }
static inline uint32_t per(uint32_t base, const ModeSpec& s){ uint32_t p = base * 100u / spd(s); return p ? p : 1; }
static inline uint8_t  ph8(uint32_t t, uint32_t period){ if(!period) return 0; return (uint8_t)(((t % period) * 256u) / period); }
static inline float    ph01(uint32_t t, uint32_t period){ if(!period) return 0.f; return (float)(t % period) / (float)period; }
static inline int      modCount(uint8_t density){ int c = ((int)density * (int)Mn + 50) / 100; if(c < 1) c = 1; if(c > MOD_MAX_ELEMENTS) c = MOD_MAX_ELEMENTS; return c; }
static inline float    pForCol(int i){ if(Msym){ float ctr = (Mn - 1) * 0.5f; return ctr > 0.f ? fabsf(i - ctr) / ctr : 0.f; } return Mn > 1 ? (float)i / (Mn - 1) : 0.f; }
static inline CRGB     modC(const ModeSpec& s, uint8_t hue, uint8_t val){ return s.sat == 0 ? CRGB(val, val, val) : (CRGB)CHSV(hue, s.sat, val); }

// Per-frame stochastic rates (twinkle spawn, flash, fire spark/cool, sparkle) are
// rolled once per RENDER, so their events/second silently tracked the frame rate —
// a strip long enough that FastLED.show() drops it below 50fps sparkled/twinkled
// slower, and the same mode looked different on a short vs a long strip. Scale the
// per-frame threshold/amount by the virtual ms elapsed this frame (gDtV, already
// Speed-weighted like the twinkle DECAY term) over the nominal frame, so the rate
// is a property of Speed alone — frame-rate- and length-independent. Identity at
// 50fps Speed 1 (gDtV≈FRAME_MS), so the default look is unchanged.
static inline int     dtScale(int base){ return base * (int)gDtV / (int)FRAME_MS; }
static inline uint8_t dtProb(uint8_t base){ int v = dtScale(base); return v > 255 ? 255 : (uint8_t)v; }

// Degenerate-length fallback (§3.2 rule 6): whole-strip brightness modulation
// driven by the same phase/rate the positional archetype would have used.
static inline void modFallback(const ModeSpec& s, uint32_t t){
  uint8_t v = sin8(ph8(t, per(1500, s)));
  uint8_t hue = (s.hue == 255) ? 0 : s.hue;
  modWhole(modC(s, hue, v), (s.flags & FLAG_DIM) ? 0.4f : 1.f);
}

// ══════════════════════════════════════════════════════════════
//  The 18 base archetypes (6 more appended below). Each: (spec, virtual-ms t, n). Paint native hues.
// ══════════════════════════════════════════════════════════════

// 0 SOLID — whole strip one colour, optional breathe (p1=depth). p2>=2 adds
//   bright moving blobs (planets).
static void archSolid(const ModeSpec& s, uint32_t t, uint16_t n){
  uint8_t v = 255;
  if(s.p1){ uint8_t b = sin8(ph8(t, per(4000, s))); v = 255 - scale8(s.p1, 255 - b); }
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  if(s.flags & FLAG_RAINBOW){
    for(int i = 0; i < (int)Mn; i++){ float p = pForCol(i); modColA(i, modC(s, s.hue + (uint8_t)(p*255), v), dim); }
  } else modWhole(modC(s, s.hue, v), dim);
  for(int k = 0; k < (int)s.p2 && k < 4; k++){
    float pos = ph01(t + k*3000u, per(9000, s));
    modPix(pos, modC(s, s.hue + k*100, 255), dim);
  }
}

// 1 GRADIENT — hue ramp along p (p1=span), drifting. SYM brightens the centre.
static void archGradient(const ModeSpec& s, uint32_t t, uint16_t n){
  uint8_t drift = ph8(t, per(6000, s));
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  if(s.density){ dim *= 0.45f + 0.55f * (sin8(ph8(t, per(5000, s))) / 255.f); }   // breathe
  for(int i = 0; i < (int)Mn; i++){
    float p = pForCol(i);
    uint8_t hue = s.hue + (uint8_t)(p * s.p1) + drift;
    uint8_t v = Msym ? (uint8_t)(60 + 195 * (1.f - p)) : 255;
    modColA(i, modC(s, hue, v), dim);
  }
}

// 2 WAVE — sum of p2 travelling sines (p1=spatial freq, 0→60).
static void archWave(const ModeSpec& s, uint32_t t, uint16_t n){
  int harm = s.p2 ? s.p2 : 1; uint8_t freq = s.p1 ? s.p1 : 60;
  int dir = (s.flags & FLAG_REVERSE) ? -1 : 1;
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  for(int i = 0; i < (int)Mn; i++){
    float p = pForCol(i); float acc = 0.f;
    for(int h = 1; h <= harm; h++){
      uint8_t ang = (uint8_t)(p * freq * h) + (uint8_t)(dir * ph8(t, per(3000/h + 300, s)));
      acc += ((int)sin8(ang) - 128) / 128.f / h;
    }
    float v = 0.5f + 0.45f * acc; v = clamp01(v);
    uint8_t hue = (s.flags & FLAG_RAINBOW) ? (uint8_t)(p*200 + ph8(t, per(9000, s))) : s.hue;
    modColA(i, modC(s, hue, (uint8_t)(v*255)), dim);
  }
}

// 3 PLASMA — 4 superposed sines at incommensurate freqs (p1=freq, p2=hue span).
static void archPlasma(const ModeSpec& s, uint32_t t, uint16_t n){
  uint8_t f = s.p1 ? s.p1 : 40; uint8_t span = s.p2;
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  for(int i = 0; i < (int)Mn; i++){
    float p = pForCol(i);
    uint8_t a = sin8((uint8_t)(p*f)      + ph8(t, per(3000, s)));
    uint8_t b = sin8((uint8_t)(p*f*2+40) + ph8(t, per(5000, s)));
    uint8_t c = sin8((uint8_t)(p*f/2+90) + ph8(t, per(7000, s)));
    uint8_t d = sin8((uint8_t)(p*f*3+10) - ph8(t, per(4000, s)));
    uint8_t v = (uint8_t)(((int)a + b + c + d) / 4);
    uint8_t hue = s.hue + (span ? scale8(v, span) : 0);
    modColA(i, modC(s, hue, 255), dim);
  }
}

// 4 NOISE — inoise8 field scrolling along p (p1=spatial scale, p2=time scale).
static void archNoise(const ModeSpec& s, uint32_t t, uint16_t n){
  uint16_t sc = 400 + (uint16_t)s.p1 * 12;
  uint16_t ts = (uint16_t)((t * (s.p2 ? s.p2 : 20)) / 1000u);
  float dim = (s.flags & FLAG_DIM) ? 0.35f : 1.f;
  for(int i = 0; i < (int)Mn; i++){
    float p = pForCol(i);
    uint16_t x = (uint16_t)(p * sc);
    uint8_t v = inoise8(x, ts); v = scale8(v, v);
    uint8_t hue = (s.flags & FLAG_RAINBOW) ? (uint8_t)(s.hue + p*160)
                                           : (uint8_t)(s.hue + scale8(inoise8(x + 3000, ts), 40) - 20);
    modColA(i, modC(s, hue, v), dim);
  }
}

// 5 RAIN — particles travelling one way with a trail (p1=trail frac, p2=jitter).
static void archRain(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  int cnt = modCount(s.density ? s.density : 40);
  float trail = s.p1 / 255.f; if(trail < 0.03f) trail = 0.08f;
  int dir = (s.flags & FLAG_REVERSE) ? -1 : 1;
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  float invN = 1.f / (Mn > 1 ? Mn - 1 : 1);
  int steps = (int)ceilf(trail * Mn) + 1;
  for(int k = 0; k < cnt; k++){
    uint8_t seed = k * 37 + 11;
    float sp = 0.7f + 0.6f * (seed & 7) / 7.f + (s.p2 / 255.f);
    uint32_t period = (uint32_t)(per(2600, s) / sp); if(!period) period = 1;
    float php = ph01(t + seed * 97u, period);
    float pos = dir > 0 ? php : 1.f - php;
    uint8_t hue = (s.hue == 255) ? (uint8_t)(seed * 7) : s.hue;
    for(int dd = 0; dd < steps; dd++){
      float pp = pos - dir * dd * invN;
      if(pp < 0.f || pp > 1.f) continue;
      modPix(pp, modC(s, hue, 255), (1.f - (float)dd / steps) * dim);
    }
  }
}

// 6 TWINKLE — random points fading in place (p2=decay divisor). Stateful.
static void archTwinkle(const ModeSpec& s, uint32_t t, uint16_t n){
  if(gReset) for(int i = 0; i < MOD_MAX_ELEMENTS; i++) gLife[i] = 0.f;
  int cnt = modCount(s.density ? s.density : 40);
  float decay = 1.f / (float)(s.p2 ? s.p2 : 150);
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  for(int k = 0; k < cnt; k++){
    if(gLife[k] <= 0.f){
      if(random8() < dtProb(40)){ gLife[k] = 1.f; gLifePos[k] = random16(Mn); gLifeHue[k] = (s.hue == 255) ? random8() : (uint8_t)(s.hue + random8(30)); }
    } else {
      gLife[k] -= decay * gDtV; if(gLife[k] < 0.f) gLife[k] = 0.f;
    }
    if(gLife[k] > 0.f){
      float p = (Mn > 1) ? gLifePos[k] / (float)(Mn - 1) : 0.f;
      modPix(p, modC(s, gLifeHue[k], 255), gLife[k] * dim);
    }
  }
}

// 7 COMET — density=head count, bright heads + trails, wrap or bounce (p2=bounce).
static void archComet(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  int heads = s.density ? s.density : 1; if(heads > 6) heads = 6;
  float trail = s.p1 / 255.f; if(trail < 0.03f) trail = 0.1f;
  bool bounce = s.p2 != 0;
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  float invN = 1.f / (Mn > 1 ? Mn - 1 : 1);
  int steps = (int)ceilf(trail * Mn) + 2;
  for(int k = 0; k < heads; k++){
    uint8_t seed = k * 53 + 7;
    uint32_t period = per(2200, s) + seed * 11u;
    float php = ph01(t + seed * 131u, period);
    float pos; int td;
    if(bounce){ pos = php < 0.5f ? php * 2.f : 2.f - php * 2.f; td = php < 0.5f ? 1 : -1; }
    else { pos = php; td = 1; }
    uint8_t hue = (s.hue == 255) ? (uint8_t)(seed * 9) : (uint8_t)(s.hue + (s.flags & FLAG_RAINBOW ? (uint8_t)(pos*120) : 0));
    for(int dd = 0; dd < steps; dd++){
      float pp = pos - td * dd * invN;
      if(pp < 0.f || pp > 1.f) continue;
      float f = 1.f - (float)dd / steps;
      modPix(pp, modC(s, hue, 255), f * f * dim);
    }
  }
}

// 8 PULSE — expanding fronts from origin (p1=width frac, p2=front count). BEAT-lockable.
static void archPulse(const ModeSpec& s, uint32_t t, uint16_t n){
  int cnt = s.p2 ? s.p2 : 1; if(cnt > 12) cnt = 12;
  float wid = s.p1 / 255.f; if(wid < 0.02f) wid = 0.06f;
  float base = ((s.flags & FLAG_BEAT) && gBeatPeriod > 0.f) ? gBeatPhase : ph01(t, per(1800, s));
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  for(int k = 0; k < cnt; k++){
    float ph = base + (float)k / cnt; ph -= floorf(ph);
    float pos = (s.flags & FLAG_REVERSE) ? 1.f - ph : ph;
    uint8_t hue = s.hue + (s.flags & FLAG_RAINBOW ? (uint8_t)(pos*200) : (uint8_t)(ph*40));
    float amp = (1.f - ph); amp *= amp;
    drawFront(pos, wid, modC(s, hue, 255), amp * dim);
  }
}

// 9 SWEEP — hard beam(s) at constant rate (density=beam count, p1=width, p2=trail decay).
static void archSweep(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  int nb = s.density ? s.density : 1; if(nb > 4) nb = 4;
  uint32_t period = per(1500, s);
  float base = ph01(t, period);
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  float invN = 1.f / (Mn > 1 ? Mn - 1 : 1);
  float dec = 1.f - (s.p2 / 255.f) * 0.9f; if(dec < 0.05f) dec = 0.05f;
  int steps = (int)ceilf(Mn * 0.6f) + 1;
  for(int k = 0; k < nb; k++){
    float ph = base + (float)k / nb; ph -= floorf(ph);
    float pos = (s.flags & FLAG_REVERSE) ? 1.f - ph : ph;
    float f = 1.f;
    for(int dd = 0; dd < steps; dd++){
      float pp = pos - dd * invN;
      if(pp >= 0.f && pp <= 1.f) modPix(pp, modC(s, s.hue, 255), f * dim);
      f *= dec; if(f < 0.02f) break;
    }
  }
}

// 10 BURST — periodic explosive events + decay (p1=period*20ms, p2=spark count).
static void archBurst(const ModeSpec& s, uint32_t t, uint16_t n){
  int sparks = s.p2 ? s.p2 : 12; if(sparks > MOD_MAX_ELEMENTS) sparks = MOD_MAX_ELEMENTS;
  uint32_t period = s.p1 ? per((uint32_t)s.p1 * 20u, s) : per(3000, s);
  uint32_t ev = t / period;
  float ph = (float)(t % period) / (float)period;
  float origin = (s.flags & FLAG_SYM) ? 0.f : ((ev * 97731u) % 1000u) / 1000.f;
  uint8_t ehue = (s.hue == 255) ? (uint8_t)(ev * 45) : s.hue;
  float decay = 1.f - ph; if(decay < 0.f) decay = 0.f;
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  if(ph < 0.1f) modPix(origin, CRGB(255,255,255), (1.f - ph*10.f) * dim);   // core flash
  for(int k = 0; k < sparks; k++){
    uint8_t sd = k * 61 + (uint8_t)ev;
    float dirp = ((sd & 255) / 255.f - 0.5f) * 2.f;
    float reach = 0.15f + 0.35f * ((sd >> 3 & 7) / 7.f);
    float pos = origin + dirp * reach * ph;
    if(pos < 0.f || pos > 1.f) continue;
    modPix(pos, modC(s, (uint8_t)(ehue + (sd & 31)), 255), decay * decay * dim);
  }
}

// 11 FLASH — stochastic hard flashes on a dim bed (p1=prob, p2=bed level).
static void archFlash(const ModeSpec& s, uint32_t t, uint16_t n){
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  if(s.p2) modWhole(modC(s, (uint8_t)(s.hue + 140), s.p2), dim);            // bluish bed
  uint8_t prob = dtProb(s.p1 ? s.p1 : 30);   // rate ∝ Speed, not frame rate / strip length
  int count = 1 + Mn / 12;
  float invN = 1.f / (Mn > 1 ? Mn - 1 : 1);
  for(int k = 0; k < count; k++){
    if(random8() < prob){
      float pos = random16(Mn) * invN;
      uint8_t hue = s.hue + random8(30);
      modPix(pos, modC(s, hue, 255), dim);
      modPix(pos + invN, modC(s, hue, 220), 0.7f * dim);
    }
  }
}

// 12 STRIPES — crisp scrolling blocks, duty-cycled (p1=period frac, p2=duty %).
static void archStripes(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  float pf = s.p1 / 255.f; if(pf < 0.05f) pf = 0.2f;
  float duty = (s.p2 ? s.p2 : 50) / 100.f;
  float scroll = ph01(t, per(2500, s)) * ((s.flags & FLAG_REVERSE) ? -1.f : 1.f);
  int stripes = (int)ceilf(1.f / pf); if(stripes < 1) stripes = 1; if(stripes > MOD_MAX_ELEMENTS) stripes = MOD_MAX_ELEMENTS;
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  for(int k = 0; k <= stripes; k++){
    float p0 = k * pf + scroll; float a = p0 - floorf(p0);
    float b = a + pf * duty;
    uint8_t hue = s.hue + (s.flags & FLAG_RAINBOW ? (uint8_t)(k*30) : 0);
    if(b <= 1.f) modBand(a, b, modC(s, hue, 255), dim);
    else { modBand(a, 1.f, modC(s, hue, 255), dim); modBand(0.f, b - 1.f, modC(s, hue, 255), dim); }
  }
}

// 13 BLOCKS — segments stacking from one end, white flash on completion (p1=segments).
static void archBlocks(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  int segs = s.p1 ? s.p1 : 6; if(segs > (int)Mn) segs = Mn; if(segs < 1) segs = 1;
  float fillph = ph01(t, per(4000, s));
  int filled = (int)(fillph * segs);
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  for(int k = 0; k < filled && k < segs; k++){
    float p0 = (float)k / segs, p1v = (float)(k + 1) / segs;
    uint8_t hue = s.hue + k * (s.flags & FLAG_RAINBOW ? 24 : 6);
    modBand(p0, p1v, modC(s, hue, 255), dim);
  }
  if(fillph > 0.95f) modWhole(CRGB(255,255,255), (fillph - 0.95f) * 20.f * dim);
}

// 14 CHASE — evenly spaced marchers (p1=spacing frac, p2=marcher width).
static void archChase(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  float spacing = s.p1 / 255.f; if(spacing < 0.03f) spacing = 0.15f;
  float mw = s.p2 / 255.f; if(mw < 0.01f) mw = 0.03f;
  float march = ph01(t, per(2000, s)) * ((s.flags & FLAG_REVERSE) ? -1.f : 1.f);
  int cnt = (int)ceilf(1.f / spacing); if(cnt < 1) cnt = 1; if(cnt > MOD_MAX_ELEMENTS) cnt = MOD_MAX_ELEMENTS;
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  for(int k = 0; k < cnt; k++){
    float pos = k * spacing + march; pos -= floorf(pos);
    uint8_t hue = s.hue + (s.flags & FLAG_RAINBOW ? (uint8_t)(k*40) : 0);
    modBand(pos, pos + mw, modC(s, hue, 255), dim);
  }
}

// 15 VU — level bar from one/both ends, beat-driven (p1=0 single/1 dual, p2=decay).
static void archVU(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  bool dual = s.p1 != 0;
  float level = ((s.flags & FLAG_BEAT) && gBeatPeriod > 0.f)
                  ? (1.f - gBeatPhase)
                  : (0.5f + 0.5f * (sin8(ph8(t, per(700, s))) / 255.f));
  if(level < 0.05f) level = 0.05f;
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  int lit = (int)(level * Mn);
  float invN = 1.f / (Mn > 1 ? Mn - 1 : 1);
  for(int i = 0; i < lit; i++){
    float p = i * invN;
    modColA(i, modC(s, (uint8_t)(96 - p*96), 255), dim);                    // green→red
    if(dual) modColA(Mn - 1 - i, modC(s, (uint8_t)(160 - p*160), 255), dim);// cyan→blue
  }
}

// 16 FIRE — heat diffusion + cooling + sparking (p1=cooling, p2=sparking). Stateful.
static void archFire(const ModeSpec& s, uint32_t t, uint16_t n){
  int N = Mn; if(N < 1) N = 1; if(N > MAX_LEDS) N = MAX_LEDS;
  if(gReset) memset(gHeat, 0, sizeof(gHeat));
  uint8_t cooling = s.p1 ? s.p1 : 60, sparking = s.p2 ? s.p2 : 120;
  // Cooling ceiling and spark probability BOTH scale by dt (same factor), so the
  // fire's evolution rate is frame-rate/length-independent while its equilibrium
  // height — the balance of the two — is preserved. (cooling*10)/N is the existing
  // per-strip normalisation; dt-scaling is layered on top.
  int coolCeil = dtScale(((cooling * 10) / N) + 2); if(coolCeil < 1) coolCeil = 1; if(coolCeil > 255) coolCeil = 255;
  for(int i = 0; i < N; i++){ uint8_t c = random8(0, (uint8_t)coolCeil); gHeat[i] = qsub8(gHeat[i], c); }
  for(int i = N - 1; i >= 2; i--) gHeat[i] = (gHeat[i-1] + gHeat[i-2] + gHeat[i-2]) / 3;
  if(random8() < dtProb(sparking)){ int y = random8(N < 7 ? N : 7); gHeat[y] = qadd8(gHeat[y], random8(160, 255)); }
  for(int i = 0; i < N; i++){
    int idx = (s.flags & FLAG_REVERSE) ? (N - 1 - i) : i;
    CRGB c = HeatColor(gHeat[i]);
    modColRGB(idx, c.r, c.g, c.b);
  }
}

// 17 BLINK — rhythmic on/off across p2 groups (p1=on ms). Mode 0 is calibration.
static void archBlink(const ModeSpec& s, uint32_t t, uint16_t n){
  int grp = s.p2 ? s.p2 : 2; if(grp > MOD_MAX_ELEMENTS) grp = MOD_MAX_ELEMENTS;
  uint32_t period = per((s.p1 ? s.p1 : 400) * 2u, s);
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  for(int gi = 0; gi < grp; gi++){
    uint8_t ph = ph8(t + (uint32_t)gi * (period / grp), period);
    if(ph >= 128) continue;
    float p0 = (float)gi / grp, p1v = (float)(gi + 1) / grp;
    uint8_t hue = s.hue + gi * (s.flags & FLAG_RAINBOW ? 40 : 16);
    modBand(p0, p1v, modC(s, hue, 255), dim);
  }
}

// ══════════════════════════════════════════════════════════════
//  Extra archetypes (18..23) — dedicated 1D citations for grid modes that had
//  no faithful fit and were previously shoehorned into BLINK/CHASE/PULSE.
//  Same contract as above: native hues into fx[] only, resolution- and
//  frame-rate-independent, honour SYM/REVERSE/DIM.
// ══════════════════════════════════════════════════════════════

// 18 MARQUEE — a fixed pseudo-glyph on/off pattern scrolling along the strip at
//    reading cadence (p1=glyph seed, p2=cells across strip). Cites scrolling text
//    banners / icon cycles: you see blocks marching past, not readable letters.
static void archMarquee(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  int cells = s.p2 ? s.p2 : 6; if(cells < 2) cells = 2; if(cells > 40) cells = 40;
  uint32_t seed = s.p1 ? s.p1 : 37;
  int dir = (s.flags & FLAG_REVERSE) ? -1 : 1;
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  float scroll = ph01(t, per(9000, s)) * (float)cells * dir;   // cells of travel per loop
  for(int i = 0; i < (int)Mn; i++){
    float p = (Mn > 1) ? (float)i / (Mn - 1) : 0.f;
    int cell = (int)floorf(p * cells + scroll);
    uint32_t h = (uint32_t)cell * 2654435761u ^ (seed * 40503u);
    if(((h >> 13) & 7u) >= 3u) continue;                        // ~3/8 cells lit → words+gaps
    uint8_t hue = s.hue + (s.flags & FLAG_RAINBOW ? (uint8_t)((cell & 7) * 20) : 0);
    modColA(i, modC(s, hue, 255), dim);
  }
}

// 19 QUAKE — continuous seismic tremor (fast noise brightness shiver on a dim bed)
//    punctuated by rare violent jolts (p1=jolt rate, p2=tremor speed). Cites
//    earthquake (fast) and tectonic (slow, big — via low speed/p1/p2).
static void archQuake(const ModeSpec& s, uint32_t t, uint16_t n){
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  uint16_t ts = (uint16_t)((t * (s.p2 ? s.p2 : 60)) / 100u);   // fast tremor clock
  for(int i = 0; i < (int)Mn; i++){
    float p = pForCol(i);
    uint16_t x = (uint16_t)(p * 800);
    uint8_t tremor = inoise8(x, ts);
    uint8_t v = qadd8((uint8_t)40, scale8(tremor, 90));          // dim bed + shiver
    modColA(i, modC(s, s.hue + (tremor >> 4), v), dim);
  }
  if(random8() < dtProb(s.p1 ? s.p1 : 25)){                     // rare jolt: sharp crack
    float c = random8() / 255.f;
    float w = 0.12f + 0.20f * (random8() / 255.f);
    drawFront(c, w, modC(s, s.hue, 255), dim);
  }
}

// 20 COLLIDE — two beams launch from the two ends, converge at centre, flash white
//    on impact, then reset. Cites collider (two particle beams meeting).
static void archCollide(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  float ph = ph01(t, per(1600, s));
  float invN = 1.f / (Mn > 1 ? Mn - 1 : 1);
  if(ph < 0.8f){                                                // approach
    float k = ph / 0.8f;
    float posL = k * 0.5f, posR = 1.f - k * 0.5f;
    for(int dd = 0; dd < 4; dd++){
      float f = (1.f - dd * 0.25f) * dim;
      modPix(posL - dd * invN, modC(s, s.hue,           255), f);
      modPix(posR + dd * invN, modC(s, s.hue + 128,     255), f);
    }
  } else {                                                      // impact flash spreading out
    float f = 1.f - (ph - 0.8f) / 0.2f;
    drawFront(0.5f, 0.1f + (1.f - f) * 0.4f, CRGB(255,255,255), f * dim);
  }
}

// 21 SWARM — density soft blobs on inoise-driven wandering paths that loosely
//    cluster around a shared wandering centre — a flock, not a march (p1=drift
//    speed, p2=bob period·20ms for a bobbing swarm). Cites boids / aliens.
static void archSwarm(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  int cnt = modCount(s.density ? s.density : 10);
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  uint16_t ts = (uint16_t)((t * (s.p1 ? s.p1 : 40)) / 1000u);
  float center = inoise8(1000, ts) / 255.f;
  for(int k = 0; k < cnt; k++){
    uint8_t seed = k * 41 + 13;
    float own = inoise8(seed * 300, ts + seed * 7) / 255.f - 0.5f;
    float pos = clamp01(center + own * 0.6f);
    uint8_t bob = s.p2 ? sin8(ph8(t + seed * 90u, per((uint32_t)s.p2 * 20u + 200u, s))) : 255;
    float amp = (0.5f + 0.5f * bob / 255.f) * dim;
    uint8_t hue = (s.hue == 255) ? (uint8_t)(seed * 9) : (uint8_t)(s.hue + (seed & 15));
    modPix(pos, modC(s, hue, 255), amp);
  }
}

// 22 SPIN — a point on a spinning wheel projected onto the strip: sinusoidal
//    velocity (fast through centre, slow/reversing at the edges) with a facet
//    180° opposite, brightness tracking near/far side. The 1D cue for rotation
//    (p1=trail length). Cites rotatingCube.
static void archSpin(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  float ang = ph01(t, per(2500, s)) * 6.2831853f;
  float pos1 = 0.5f + 0.5f * sinf(ang),           f1 = 0.5f + 0.5f * cosf(ang);
  float pos2 = 0.5f + 0.5f * sinf(ang + 3.14159f), f2 = 0.5f + 0.5f * cosf(ang + 3.14159f);
  float invN = 1.f / (Mn > 1 ? Mn - 1 : 1);
  int steps = (int)ceilf((s.p1 ? s.p1 : 60) / 255.f * Mn) + 1;
  for(int dd = 0; dd < steps; dd++){
    float tf = 1.f - (float)dd / steps;
    modPix(pos1 - dd * invN, modC(s, s.hue,      255), tf * f1 * dim);
    modPix(pos2 + dd * invN, modC(s, s.hue + 40, 255), tf * f2 * dim);
  }
}

// Deterministic per-segment gaze target for archGaze (no state needed).
static inline float gazeTarget(uint32_t i){ uint32_t h = i * 2654435761u + 12345u; return ((h >> 9) & 255) / 255.f; }

// 23 GAZE — a soft iris blob on a dim "sclera" bed that saccades to held targets
//    (quick move, then hold) and periodically blinks (whole strip dips to near
//    dark and reopens). Cites eye.
static void archGaze(const ModeSpec& s, uint32_t t, uint16_t n){
  if(Mn < MOD_DEGENERATE_N){ modFallback(s, t); return; }
  float dim = (s.flags & FLAG_DIM) ? 0.4f : 1.f;
  uint32_t hold = per(1400, s); if(!hold) hold = 1;
  uint32_t seg = t / hold;
  float local = (float)(t % hold) / (float)hold;
  float e = local / 0.35f; if(e > 1.f) e = 1.f; e = e * e * (3.f - 2.f * e);   // fast saccade, then hold
  float pos = gazeTarget(seg) + (gazeTarget(seg + 1) - gazeTarget(seg)) * e;
  float bp = ph01(t, per(3600, s)), open = 1.f;
  if(bp > 0.90f){ float x = (bp - 0.90f) / 0.10f; open = fabsf(x - 0.5f) * 2.f; }  // close then open
  for(int i = 0; i < (int)Mn; i++) modColA(i, modC(s, s.hue, 255), 0.12f * open * dim);  // sclera bed
  drawFront(clamp01(pos), 0.10f, modC(s, s.hue + 20, 255), open * dim);                  // iris
}

// Neon tube (107): the strip is the tube and it is SWITCHED ON like the wall's
// submarine: single short flashes with long dark pauses first, then the strikes
// come faster and turn into stuttering bursts (with a phosphor afterglow) until
// the tube catches (~4.5 s) and hums. Afterwards a short nervous stutter now and
// then. Re-entering the mode (no call for 0.4 s) switches it on afresh.
// Real time, so it is the same at any Speed.
static void archNeon(const ModeSpec& s, uint32_t t, uint16_t n){
  static uint32_t lastT=0, start=0, nextEv=0, relapseAt=0; static uint8_t flashes=0, relapse=0;
  static bool on=false, lit=false; static float inst=0.f, glow=0.f;
  float dms=(float)(t-lastT);
  if(t-lastT>400){ start=t+300+random(200); nextEv=start; lit=false; on=false; flashes=0; relapse=0; glow=0.f; dms=20.f; }
  lastT=t;
  if(t<start) return;
  if(!lit){
    float p=(float)(t-start)/4500.f; if(p>1.f) p=1.f;
    if(t>=nextEv){
      if(on){ on=false;
        if(flashes>0) nextEv=t+12+random(15+(int)(35.f*(1.f-p)));
        else if(p>=1.f){ lit=true; on=true; inst=1.f; relapseAt=t+9000+random(22000); }
        else { float g=(1.f-p)*(1.f-p)*1100.f+70.f; nextEv=t+(uint32_t)(g*(0.45f+1.1f*random8()/255.f)); }
      } else {
        if(flashes==0) flashes=1+random(1+(int)(p*p*8.f));
        flashes--; on=true;
        inst=(random8()/255.f<0.3f+0.5f*p) ? 1.f : 0.3f+0.6f*random8()/255.f;
        nextEv=t+14+random(16+(int)(90.f*p*p));
      }
    }
  } else if(relapse>0){
    if(t>=nextEv){ on=!on;
      if(on){ inst=0.45f+0.55f*random8()/255.f; nextEv=t+18+random(70); }
      else { relapse--; nextEv=t+14+random(random8()<38?400:60); }
      if(relapse==0){ on=true; inst=1.f; relapseAt=t+9000+random(22000); } }
  } else {
    on=true; inst=0.93f+0.07f*(inoise8((uint16_t)(t/3))/255.f);
    if(t>=relapseAt){ relapse=2+random(6); on=false; nextEv=t+15; }
  }
  if(on) glow=inst; else glow*=expf(-dms/20.f);
  if(glow<=0.004f) return;
  modWhole(modC(s, s.hue, 255), glow);
}
// Thunderstorm (112): almost dark; strikes at random (0.3–4 s apart) as a burst
// of 2–7 flashes with random gaps and strengths. A flash lights the whole strip
// or one random section (the part of the bolt this strip "sees"), white to
// blue/violet; the strike's colour is drawn fresh each time.
static void archStorm(const ModeSpec& s, uint32_t t, uint16_t n){
  (void)n;
  static uint32_t nextStrike=0, flashEnd=0, nextFlash=0; static int left=0; static float inten=0, p0=0, p1=1; static uint8_t hue=170, sat=100;
  if(left==0 && t>=nextStrike){ left=2+random(6); nextFlash=t; hue=150+random(70); sat=random(2)?0:120+random(120); }
  if(left>0 && t>=nextFlash){
    flashEnd=t+18+random(70); inten=0.35f+0.65f*random8()/255.f;
    if(random(3)==0){ p0=0.f; p1=1.f; } else { p0=random8()/255.f*0.7f; p1=p0+0.15f+random8()/255.f*0.35f; }
    left--; nextFlash=flashEnd+15+random(left>0?220:1);
    if(left==0) nextStrike=t+300+random(3700);
  }
  modWhole(CRGB(CHSV(s.hue,220,255)), 0.035f);                 // the dark sky
  if(t<flashEnd){ CRGB c = sat==0 ? CRGB(255,255,255) : CRGB(CHSV(hue,sat,255));
    modBand(p0, fminf(p1,1.f), c, inten*(0.6f+0.4f*random8()/255.f)); }
}

// ── Moving spot: one-shot trigger (TOGA_F_SPOT rising edge), NOT a MODE_TABLE
//    entry. Each controller spot-key press launches ONE bright soft dot that
//    glides the strip once (short warm trail) and then vanishes, returning to
//    the normal effect. Presses in quick succession queue several dots, so a
//    few taps send a few spots chasing down the strip together. Paints native
//    hues into fx just like the archetypes, so showModule tint/brightness/beat
//    apply and panels render each as a moving vertical bar via modColRGB.
#define MOD_MAX_SPOTS   6            // dots that can be in flight at once
#define MOD_SPOT_PERIOD 900u         // ms of REAL time per full strip traversal (kept in step with grid anim_spotTunnel period)
#define MOD_SPOT_GAP    260u         // hold the key → a fresh dot every this many ms (a stream, not one)

static uint32_t gSpotStart[MOD_MAX_SPOTS];   // launch time (gVirtMs) per active dot
static bool     gSpotOn[MOD_MAX_SPOTS] = { false };

// Launch a new dot at virtual time t. Reuses a free slot; if all are busy,
// overwrites the oldest so a rapid burst never gets silently dropped.
void spotLaunch(uint32_t t){
  int slot = -1;
  for(int i = 0; i < MOD_MAX_SPOTS; i++) if(!gSpotOn[i]){ slot = i; break; }
  if(slot < 0){
    slot = 0; uint32_t oldest = gSpotStart[0];
    for(int i = 1; i < MOD_MAX_SPOTS; i++)
      if((uint32_t)(t - gSpotStart[i]) > (uint32_t)(t - oldest)){ oldest = gSpotStart[i]; slot = i; }
  }
  gSpotStart[slot] = t; gSpotOn[slot] = true;
}

bool spotsActive(){
  for(int i = 0; i < MOD_MAX_SPOTS; i++) if(gSpotOn[i]) return true;
  return false;
}

// Render every in-flight dot; retire those that have run off the end (trail
// included). Returns true while any remain so the caller keeps the override on.
bool renderSpots(uint32_t t){
  Mn = gWidth ? gWidth : 1;
  fill_solid(fx, gNumLeds, CRGB::Black);
  bool any = false;
  if(Mn < MOD_DEGENERATE_N){                         // too few LEDs to travel → one flash per dot
    float amp = 0.f;
    for(int i = 0; i < MOD_MAX_SPOTS; i++){
      if(!gSpotOn[i]) continue;
      uint32_t age = t - gSpotStart[i];
      if(age >= MOD_SPOT_PERIOD){ gSpotOn[i] = false; continue; }
      amp += sinf((float)age / (float)MOD_SPOT_PERIOD * 3.14159f);    // 0→1→0 over the period
      any = true;
    }
    if(amp > 1.f) amp = 1.f;
    if(any) modWhole(CRGB(255, 255, 255), amp);
    return any;
  }
  // Travel direction follows the floor setting: FWD (or unflagged) glides 0→end,
  // REV glides end→0. The warm trail always sits BEHIND the head, so its offset
  // flips with the direction.
  bool rev = (gFloorFlow == TOGA_FLOOR_REV);
  float td  = rev ? +1.0f : -1.0f;                    // trail offset (behind the head)
  for(int i = 0; i < MOD_MAX_SPOTS; i++){
    if(!gSpotOn[i]) continue;
    uint32_t age = t - gSpotStart[i];
    float travel = (float)age / (float)MOD_SPOT_PERIOD * (float)(Mn - 1);   // 0 → end over the pass
    float pos    = rev ? (float)(Mn - 1) - travel : travel;
    bool exited  = rev ? (pos + 2.0f < 0.0f) : (pos - 2.0f > (float)(Mn - 1));
    if(exited){ gSpotOn[i] = false; continue; }        // head + trail have run off the end
    splatLed(pos,             CRGB(255, 255, 255), 1.0f);   // head
    splatLed(pos + td,        CRGB(255, 200, 120), 0.5f);   // trail
    splatLed(pos + td * 2.0f, CRGB(255, 160,  80), 0.22f);
    any = true;
  }
  return any;
}

// ══════════════════════════════════════════════════════════════
//  Coverage match (v9). Trim the module's lit fraction down to the grid's live
//  coverage so "half the grid lit → half the strip lit". Operates per COLUMN (a
//  strip is one column tall; a panel draws each column uniformly), keeping the
//  brightest ceil(cov·width) columns and blacking the rest. It only ever REMOVES
//  light: a render already sparser than the grid is left untouched — nothing is
//  invented. Called from the FOLLOW render path after modRender().
// ══════════════════════════════════════════════════════════════
static inline uint8_t modColBriMax(int c){
  uint8_t mx = 0;
  for(uint16_t y = 0; y < gHeight; y++){ CRGB& p = ledXY(c, y);
    uint8_t m = p.r > p.g ? (p.r > p.b ? p.r : p.b) : (p.g > p.b ? p.g : p.b);
    if(m > mx) mx = m; }
  return mx;
}
void modCoverGate(uint8_t cov){
  if(cov >= 255) return;                                   // grid ~fully lit → keep everything
  int w = gWidth ? gWidth : 1;
  int target = (int)(((uint32_t)cov * (uint32_t)w + 127u) / 255u);   // columns to keep lit (rounded)
  if(target <= 0){                                         // grid dark → blank the strip
    for(int c = 0; c < w; c++) for(uint16_t y = 0; y < gHeight; y++) ledXY(c, y) = CRGB(0,0,0);
    return;
  }
  uint16_t hist[256]; for(int b = 0; b < 256; b++) hist[b] = 0;
  int litNow = 0;
  for(int c = 0; c < w; c++){ uint8_t mx = modColBriMax(c); hist[mx]++; if(mx) litNow++; }
  if(litNow <= target) return;                             // already sparse enough → leave as-is
  int cum = 0, thr = 1;                                    // brightness cut so ~`target` columns survive
  for(int b = 255; b >= 1; b--){ cum += hist[b]; if(cum >= target){ thr = b; break; } }
  for(int c = 0; c < w; c++) if(modColBriMax(c) < thr)
    for(uint16_t y = 0; y < gHeight; y++) ledXY(c, y) = CRGB(0,0,0);
}

// ══════════════════════════════════════════════════════════════
//  Dispatch. Called once per frame with the virtual clock (gVirtMs). fx is
//  cleared here; SYM is handled inside modPix/pForCol; SPARKLE is a post-pass.
// ══════════════════════════════════════════════════════════════

// Per-frame preamble shared by the grid-mode and local-mode render paths: set the
// column count and the speed-weighted dt (gDtV). Exactly ONE render entry runs per
// frame, so lastT advances once and gDtV stays a true per-frame delta.
static inline void modFrameBegin(uint32_t t){
  Mn = gWidth ? gWidth : 1;
  static uint32_t lastT = 0;
  gDtV = (uint32_t)(t - lastT); if(gDtV > 200u) gDtV = 200u; lastT = t;
}

// Run one ModeSpec into fx: sets Msym, dispatches to its archetype, overlays
// sparkle. Shared by MODE_TABLE (grid citations) and LOCAL_MODES (independent).
static void modDispatch(const ModeSpec& s, uint32_t t){
  Msym = (s.flags & FLAG_SYM) != 0;
  switch(s.arch){
    case A_SOLID:    archSolid(s, t, Mn);    break;
    case A_GRADIENT: archGradient(s, t, Mn); break;
    case A_WAVE:     archWave(s, t, Mn);     break;
    case A_PLASMA:   archPlasma(s, t, Mn);   break;
    case A_NOISE:    archNoise(s, t, Mn);    break;
    case A_RAIN:     archRain(s, t, Mn);     break;
    case A_TWINKLE:  archTwinkle(s, t, Mn);  break;
    case A_COMET:    archComet(s, t, Mn);    break;
    case A_PULSE:    archPulse(s, t, Mn);    break;
    case A_SWEEP:    archSweep(s, t, Mn);    break;
    case A_BURST:    archBurst(s, t, Mn);    break;
    case A_FLASH:    archFlash(s, t, Mn);    break;
    case A_STRIPES:  archStripes(s, t, Mn);  break;
    case A_BLOCKS:   archBlocks(s, t, Mn);   break;
    case A_CHASE:    archChase(s, t, Mn);    break;
    case A_VU:       archVU(s, t, Mn);       break;
    case A_FIRE:     archFire(s, t, Mn);     break;
    case A_BLINK:    archBlink(s, t, Mn);    break;
    case A_MARQUEE:  archMarquee(s, t, Mn);  break;
    case A_QUAKE:    archQuake(s, t, Mn);    break;
    case A_COLLIDE:  archCollide(s, t, Mn);  break;
    case A_SWARM:    archSwarm(s, t, Mn);    break;
    case A_SPIN:     archSpin(s, t, Mn);     break;
    case A_GAZE:     archGaze(s, t, Mn);     break;
    case A_NEON:     archNeon(s, t, Mn);     break;
    case A_STORM:    archStorm(s, t, Mn);    break;
    default:         archSolid(s, t, Mn);    break;
  }
  if(s.flags & FLAG_SPARKLE){
    int g = 1 + Mn / 40;                        // count ∝ length (density); rate ∝ Speed (dtProb)
    for(int k = 0; k < g; k++) if(random8() < dtProb(50)){ int i = random16(Mn); modColRGB(i, 50, 50, 50); }
  }
}

// Grid-citation render: mode k renders MODE_TABLE[k] (FOLLOW / pinned grid mode).
void modRender(uint16_t mode, uint32_t t, uint16_t n){
  modFrameBegin(t);
  gReset = (mode != gLastMode); gLastMode = mode; gCurMode = mode;

  fill_solid(fx, gNumLeds, CRGB::Black);
  if(mode == TOGA_EFFECT_OFF || mode >= MOD_MODE_COUNT) return;  // OFF / out of range → black
  if(mode == TOGA_MODE_SCHWARZ) return;                   // dark standby: the wall shows its dot, strips stay black

  if(mode == 0){                                          // setupGrid calibration (static)
    modColRGB(0, 140, 0, 0);
    if(Mn > 1) modColRGB(Mn - 1, 0, 140, 0);
    modColRGB((Mn - 1) / 2, 120, 120, 120);
    return;
  }

  ModeSpec s; memcpy_P(&s, &MODE_TABLE[mode], sizeof(s));
  modDispatch(s, t);
}

// Local (independent) render: local effect index k renders LOCAL_MODES[k]. A key
// disjoint from the grid modes (0x2000|k) drives gReset so switching between a
// grid mode and a local mode, or between two local modes, reseeds archetype state.
void modRenderLocal(uint8_t idx, uint32_t t){
  modFrameBegin(t);
  uint16_t key = (uint16_t)(0x2000 | idx);
  gReset = (key != gLastMode); gLastMode = key; gCurMode = key;

  fill_solid(fx, gNumLeds, CRGB::Black);
  if(idx >= TOGA_LOCAL_COUNT) return;

  ModeSpec s; memcpy_P(&s, &LOCAL_MODES[idx], sizeof(s));
  modDispatch(s, t);
  // Floor direction: a strip flagged REV runs its directional effects the other way.
  if(gFloorFlow == TOGA_FLOOR_REV && localIsDirectional(idx)) modMirrorX();
}

// Resolve a dance auto-program to the local effect it should show right now: step
// through the program's list one entry every AUTO_DWELL_MS of VIRTUAL time (so
// Speed scales the change rate, like every other module motion).
static inline uint8_t autoResolveLocal(uint8_t prog, uint32_t t){
  if(prog >= TOGA_LOCAL_AUTO_COUNT) return 0;
  const AutoProg& p = AUTO_PROGS[prog];
  if(p.len == 0) return 0;
  uint32_t step = (t / AUTO_DWELL_MS) % p.len;
  return p.list[step];
}

// Single render entry used by loop(): route an effect id to the grid-citation,
// local-effect, or dance-auto path. `eff` is activeEffect() (already resolved for
// FOLLOW). Local/auto ids sit above the grid modes (see toga_proto.h).
void modRenderAny(uint8_t eff, uint32_t t){
  if(togaIsAutoEffect(eff))  { modRenderLocal(autoResolveLocal(eff - TOGA_EFFECT_AUTO_BASE, t), t); return; }
  if(togaIsLocalEffect(eff)) { modRenderLocal(eff - TOGA_EFFECT_LOCAL_BASE, t); return; }
  modRender(eff, t, gWidth);                              // grid mode / OFF / out of range
}
