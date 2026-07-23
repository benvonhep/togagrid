// ══════════════════════════════════════════════════════════════
//  module_effects.h — 90 resolution-independent 1D modes.
//  (#included by module.ino after the globals; do not compile alone.)
//
//  A module in FOLLOW mode renders module mode k for grid mode k (0..89),
//  1:1. Each mode is a self-contained PROCEDURAL 1D animation — never a
//  downsample of the grid's framebuffer (there are no pixels on the wire).
//  Visual identity with the 2D grid is impossible and not the goal; a
//  recognisable CITATION is: matching motion, direction, symmetry, rhythm,
//  colour family and density.
//
//  Architecture: 24 parametrised archetype renderers + a MODE_TABLE[90]
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
  A_MARQUEE, A_QUAKE, A_COLLIDE, A_SWARM, A_SPIN, A_GAZE
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

// Authoritative 90-entry mapping. Names in the trailing comment are the grid's
// animation at that index (index 39 renders plasmaLightning — grid's
// anim_agentField is an alias for it). PROGMEM: read one row per frame.
static const ModeSpec MODE_TABLE[90] PROGMEM = {
  /* 0 setupGrid       */ { A_BLINK,     0,  0, 100,  0, 0,               0, 3 },
  /* 1 heartbeat       */ { A_PULSE,     0,255, 180,  0, FLAG_SYM,       40, 2 },
  /* 2 binaryRain      */ { A_RAIN,     96,220, 120, 80, FLAG_SPARKLE,   60,40 },
  /* 3 tectonic        */ { A_QUAKE,    20,200,  45,  0, 0,              12,25 },
  /* 4 vortex          */ { A_WAVE,    180,220, 165,  0, 0,               0, 3 },
  /* 5 earthquake      */ { A_QUAKE,    25,200, 100,  0, 0,              30,70 },
  /* 6 dnaHelix        */ { A_WAVE,    130,240, 105,  0, 0,               0, 2 },
  /* 7 supernova       */ { A_BURST,    20,200, 100,  0, FLAG_SYM,      200,20 },
  /* 8 blackHole       */ { A_GRADIENT, 20,255, 190,  0, FLAG_SYM,       80, 0 },
  /* 9 wormhole        */ { A_PULSE,   190,240, 250,  0, FLAG_SYM|FLAG_REVERSE, 60,3 },
  /*10 mitosis         */ { A_PULSE,    85,200,  17,  0, FLAG_SYM,       50, 2 },
  /*11 collider        */ { A_COLLIDE, 130,255,  46,  0, 0,               0, 0 },
  /*12 shockwaveChain  */ { A_PULSE,   200,220,  90,  0, 0,              40, 4 },
  /*13 pulsar          */ { A_SWEEP,   150,200, 250,  0, 0,              30,60 },
  /*14 liquidMetal     */ { A_WAVE,     20,220,  50,  0, 0,               0, 2 },
  /*15 iceCrown        */ { A_CHASE,   160,200,   8,  0, 0,              40,20 },
  /*16 coriolisStorm   */ { A_WAVE,    148,255,  80,  0, FLAG_SYM,        0, 5 },
  /*17 crystalFracture */ { A_FLASH,   158,250, 100,  0, 0,              30,60 },
  /*18 aurora          */ { A_NOISE,   120,200,  60,  0, 0,              40,18 },
  /*19 lavaLamp        */ { A_PLASMA,   10,230,  40,  0, 0,              40,60 },
  /*20 nerveSystem     */ { A_COMET,   255,220, 220,  4, 0,              60, 1 },
  /*21 gravityLens     */ { A_WAVE,    180,220,   8,  0, 0,               0, 2 },
  /*22 tidalWave       */ { A_WAVE,    150,230,  21,  0, 0,               0, 3 },
  /*23 cymatics        */ { A_STRIPES,  60,220, 100,  0, 0,              80,40 },
  /*24 starfieldWarp   */ { A_RAIN,    200,180, 217, 50, FLAG_SYM,       50,30 },
  /*25 darkMatter      */ { A_NOISE,   200,240,  50,  0, FLAG_DIM,       40,20 },
  /*26 concentricSq    */ { A_PULSE,   120,220,  90,  0, FLAG_SYM,       40, 4 },
  /*27 squareRain      */ { A_TWINKLE, 255,220, 100, 60, 0,              40,180},
  /*28 diamondPulse    */ { A_PULSE,   150,230, 250,  0, FLAG_SYM,       40, 3 },
  /*29 checkerboard    */ { A_STRIPES, 120,230,  20,  0, 0,             120,50 },
  /*30 rotatingCube    */ { A_SPIN,      0,220,  24,  0, 0,              60, 0 },
  /*31 sunsetHorizon   */ { A_GRADIENT, 10,180,   8,  0, 0,             120, 0 },
  /*32 depthMap        */ { A_GRADIENT,150,180,  33,  0, FLAG_SYM,       60, 0 },
  /*33 cosmicDust      */ { A_NOISE,   200,210,  50,  0, 0,              50,10 },
  /*34 heartglow       */ { A_PULSE,     5,255,  45,  0, FLAG_SYM,       60, 1 },
  /*35 evolution       */ { A_PLASMA,  100,220,  56,  0, 0,              40,80 },
  /*36 morph           */ { A_PLASMA,  150,210,  65,  0, 0,              40,80 },
  /*37 strangeAttract  */ { A_COMET,    90,220, 220,  1, 0,              50, 0 },
  /*38 harmonicReson   */ { A_PLASMA,  170,210,  83,  0, 0,              50,90 },
  /*39 plasmaLightning */ { A_FLASH,   150,230, 110,  0, 0,              40,60 },
  /*40 tetris          */ { A_BLOCKS,   96,220, 100,  0, 0,               8, 0 },
  /*41 snake           */ { A_COMET,    96,220, 143,  1, 0,              40, 0 },
  /*42 iconShow        */ { A_MARQUEE,  40,230, 150,  0, 0,              77, 8 },
  /*43 bouncingLogo    */ { A_COMET,     0,230, 250,  1, 0,              30, 1 },
  /*44 starTunnel3D    */ { A_RAIN,    200,160, 104, 50, FLAG_SYM,       40,20 },
  /*45 rippleRain      */ { A_PULSE,   150,230, 135,  0, 0,              40, 3 },
  /*46 spiralGalaxy    */ { A_WAVE,    200,210,  84,  0, FLAG_SYM,        0, 2 },
  /*47 plasmaField     */ { A_PLASMA,    0,230,  79,  0, FLAG_RAINBOW,   40,255},
  /*48 breatheGradient */ { A_GRADIENT,120,190,  57,  1, 0,             150, 0 },
  /*49 boids           */ { A_SWARM,   110,220, 100, 12, 0,              40, 0 },
  /*50 fireworks       */ { A_BURST,   255,230, 110,  0, 0,              40,14 },
  /*51 reactionDiff    */ { A_NOISE,   180,200,  60,  0, 0,              45,20 },
  /*52 textTOGA        */ { A_MARQUEE,  40,255,  90,  0, 0,              41, 6 },
  /*53 textCASTLE      */ { A_MARQUEE, 140,255,  90,  0, 0,             113, 7 },
  /*54 text2026        */ { A_MARQUEE, 210,255,  90,  0, 0,             205, 5 },
  /*55 stickman        */ { A_CHASE,    35,180, 110,  0, 0,              50,10 },
  /*56 paddleBall      */ { A_COMET,    40,220, 110,  1, 0,              25, 1 },
  /*57 smiley          */ { A_BLINK,    45,235,  80,  0, 0,             800, 4 },
  /*58 eye             */ { A_GAZE,    140,225,  90,  0, 0,               0, 0 },
  /*59 starfield2      */ { A_TWINKLE, 170,180, 110, 50, FLAG_SYM,       40,150},
  /*60 warp            */ { A_RAIN,    174,180, 150, 50, FLAG_SYM,       70,20 },
  /*61 asteroids       */ { A_COMET,    20, 40,  15,  1, 0,              40, 0 },
  /*62 shootingStars   */ { A_TWINKLE, 160,180, 120, 40, FLAG_SPARKLE,   30,120},
  /*63 blackHole2      */ { A_PULSE,    28,255,  46,  0, FLAG_SYM|FLAG_DIM, 50,2 },
  /*64 pulsar2         */ { A_SWEEP,   160,200,  57,  2, 0,              25,60 },
  /*65 pulsarNebula    */ { A_SWEEP,   180,200,  48,  0, 0,              30,50 },
  /*66 orionNebula     */ { A_NOISE,   180,190,  55,  0, FLAG_SPARKLE,   50,12 },
  /*67 horsehead       */ { A_NOISE,    24,200,  50,  0, 0,              50,15 },
  /*68 twinStars       */ { A_PULSE,    30,200,  37,  0, FLAG_SYM,       40, 2 },
  /*69 planets         */ { A_SOLID,    20,255,  29,  0, 0,              80, 2 },
  /*70 earthWindow     */ { A_NOISE,   100,220,  60,  0, 0,              50,20 },
  /*71 shipPanel       */ { A_BLINK,     0,255,  90,  0, 0,             600, 6 },
  /*72 radar           */ { A_SWEEP,    96,255,  37,  0, 0,              20,70 },
  /*73 sineWaves       */ { A_WAVE,    150,220,  79,  0, 0,               0, 2 },
  /*74 rocketFire      */ { A_FIRE,     45,255, 110,  0, FLAG_REVERSE,   60,120},
  /*75 warningLight    */ { A_SWEEP,    20,255,  72,  0, 0,              40,50 },
  /*76 aliens          */ { A_SWARM,    95,220,  90,  8, 0,              30,250},
  /*77 spaceInvaders   */ { A_CHASE,   150,210,  25,  0, 0,              60,20 },
  /*78 rain2           */ { A_RAIN,    150,120, 186, 50, 0,              40,30 },
  /*79 levitate        */ { A_RAIN,     40, 90,  74, 40, FLAG_REVERSE,   50,40 },
  /*80 confetti        */ { A_TWINKLE, 255,230, 100, 60, 0,              40,150},
  /*81 spiralEject     */ { A_BURST,   180,220, 100,  0, FLAG_SYM,      100,16 },
  /*82 clouds3d        */ { A_NOISE,   150,180,  45,  0, 0,              50,10 },
  /*83 pulseHeart      */ { A_PULSE,     0,240, 100,  0, FLAG_SYM|FLAG_BEAT, 50,1 },
  /*84 rainUp          */ { A_RAIN,    150,120, 186, 50, FLAG_REVERSE,   40,30 },
  /*85 starsFlying     */ { A_TWINKLE, 160,180, 100, 50, 0,              40,120},
  /*86 firework2       */ { A_BURST,   255,220, 100,  0, 0,             130,10 },
  /*87 hStripesFlow    */ { A_STRIPES, 150,220, 250,  0, 0,              90,55 },
  /*88 vuColumns       */ { A_VU,       96,255, 100,  0, FLAG_BEAT,       0,60 },
  /*89 vuGrid          */ { A_VU,       96,255, 100,  0, FLAG_BEAT,       1,60 },
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

// ── Moving spot: one-shot trigger (TOGA_F_SPOT rising edge), NOT a MODE_TABLE
//    entry. Each controller spot-key press launches ONE bright soft dot that
//    glides the strip once (short warm trail) and then vanishes, returning to
//    the normal effect. Presses in quick succession queue several dots, so a
//    few taps send a few spots chasing down the strip together. Paints native
//    hues into fx just like the archetypes, so showModule tint/brightness/beat
//    apply and panels render each as a moving vertical bar via modColRGB.
#define MOD_MAX_SPOTS   6            // dots that can be in flight at once
#define MOD_SPOT_PERIOD 1500u        // ms of virtual clock per full traversal
#define MOD_SPOT_GAP    300u         // hold the key → a fresh dot every this many ms (a stream, not one)

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
  for(int i = 0; i < MOD_MAX_SPOTS; i++){
    if(!gSpotOn[i]) continue;
    uint32_t age = t - gSpotStart[i];
    float pos = (float)age / (float)MOD_SPOT_PERIOD * (float)(Mn - 1);
    if(pos - 2.0f > (float)(Mn - 1)){ gSpotOn[i] = false; continue; }  // head + trail have exited
    splatLed(pos,        CRGB(255, 255, 255), 1.0f);  // head
    splatLed(pos - 1.0f, CRGB(255, 200, 120), 0.5f);  // trail
    splatLed(pos - 2.0f, CRGB(255, 160,  80), 0.22f);
    any = true;
  }
  return any;
}

// ══════════════════════════════════════════════════════════════
//  Dispatch. Called once per frame with the virtual clock (gVirtMs). fx is
//  cleared here; SYM is handled inside modPix/pForCol; SPARKLE is a post-pass.
// ══════════════════════════════════════════════════════════════
void modRender(uint16_t mode, uint32_t t, uint16_t n){
  Mn = gWidth ? gWidth : 1;
  static uint32_t lastT = 0;
  gDtV = (uint32_t)(t - lastT); if(gDtV > 200u) gDtV = 200u; lastT = t;
  gReset = (mode != gLastMode); gLastMode = mode; gCurMode = mode;

  fill_solid(fx, gNumLeds, CRGB::Black);
  if(mode == TOGA_EFFECT_OFF || mode >= 90) return;      // OFF / out of range → black

  ModeSpec s; memcpy_P(&s, &MODE_TABLE[mode], sizeof(s));
  Msym = (s.flags & FLAG_SYM) != 0;

  if(mode == 0){                                          // setupGrid calibration (static)
    modColRGB(0, 140, 0, 0);
    if(Mn > 1) modColRGB(Mn - 1, 0, 140, 0);
    modColRGB((Mn - 1) / 2, 120, 120, 120);
    return;
  }

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
    default:         archSolid(s, t, Mn);    break;
  }

  if(s.flags & FLAG_SPARKLE){
    int g = 1 + Mn / 40;                        // count ∝ length (density); rate ∝ Speed (dtProb)
    for(int k = 0; k < g; k++) if(random8() < dtProb(50)){ int i = random16(Mn); modColRGB(i, 50, 50, 50); }
  }
}
