#include <FastLED.h>
#include <math.h>
// grid_config.h pulls in <toga_proto.h>, which defines the ESP-NOW wire
// structs. It must stay up here: arduino-cli injects generated prototypes
// near the top of the sketch and they have to see these types first.
#include "grid_config.h"     // all tunable constants: hardware, pins, brightness, AGC, geometry
#include "grid_beat.h"       // tap-tempo tracker (button 11 -> beatFactor())

CRGB ledsX[NUM_ELEC][LEDS_PER_STRIP];
CRGB ledsY[NUM_ELEC][LEDS_PER_STRIP];

void setLED(bool isX, int strip, int led, CRGB colour) {
  int elec=(strip-1)/2; bool isOdd=(strip%2==1); bool forward; int bufIdx;
  if(isX)forward=!isOdd; else forward=isOdd;
  if(isOdd)bufIdx=forward?(led-1):(121-led);
  else bufIdx=forward?(135+led):(257-led);  // even-Y return rows: one more disabled LED at the turn (align)
  if(isX)ledsX[elec][bufIdx]=colour; else ledsY[elec][bufIdx]=colour;
}

void clearAll() {
  FastLED.clear(true);delay(10);FastLED.clear(true);
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
  FastLED.show();
}

// ── Global colour controls (Color knob + auto Hue-speed) ──
// gHueSpeed is the ACTIVE mode's hue-speed; each mode keeps its own in
// hueSpeedByMode[] (restored on mode change), exactly like the brightness factor.
uint8_t gHueBase=0, gHueAuto=0, gHueSpeed=0, gStrobeHue=0;
uint8_t gCoverage=0;   // live fraction of active LEDs lit, 0..255 — sent in sync so FOLLOW modules match it
uint8_t gFrameHue=0;   // dominant hue of the rendered wall this frame (post-rotation) — sent so FOLLOW modules wear the real grid colour
uint8_t hueSpeedByMode[MAX_MODES];   // per-mode hue-speed 0..HUE_SPEED_MAX
// Text-strobe colours (web UI): per text id, own hue/sat or (bit clear) the strobe colour.
uint8_t gTxOn=0, gTxHue[8]={0}, gTxSat[8]={0};
static inline CRGB textStrobeColour(uint8_t id){
  if(id<8 && (gTxOn&(1<<id))) return gTxSat[id]==0 ? CRGB(CRGB::White) : CRGB(CHSV(gTxHue[id],gTxSat[id],255));
  return gStrobeHue==0 ? CRGB(CRGB::White) : CRGB(CHSV(gStrobeHue,255,255));
}
bool    gHueTabDue=true;             // send the per-mode colour table now (after a change), not only on the 1 s tick
uint8_t hueBaseByMode[MAX_MODES];    // per-mode COLOUR (Colour key, phys 8) — restored on mode change (v11)
bool    gHueReset=false;             // applyModeSettings() → loop() zeroes the auto-rotation so a mode starts on its own colour

// ── Per-mode speed RANGE ──────────────────────────────────────────
// The Speed knob (button 14) reads 0.1..4.0 for every mode, but the visual speed
// that "0.1" and "4.0" MEAN differs per mode: some tops were too slow, some floors
// too fast. So the knob is a normalized dial that maps linearly onto a per-mode
// [lo..hi] effective-speed window. Default {0.1,4.0} reproduces the old behaviour
// exactly; overrides in initModeSpeed() raise a ceiling ("max Nx") or drop a floor
// ("min ¼") independently. The remapped value is what drives virtualT AND what is
// sent to the modules (sync.animSpeed), so grid and strips stay in lockstep.
struct SpeedRange { float lo, hi; };
SpeedRange gModeSpeed[MAX_MODES];
void initModeSpeed(){
  for(int i=0;i<MAX_MODES;i++){ gModeSpeed[i].lo=0.1f; gModeSpeed[i].hi=4.0f; }
  auto R=[&](int m,float lo,float hi){ if(m>=0&&m<MAX_MODES){gModeSpeed[m].lo=lo; gModeSpeed[m].hi=hi;} };
  //   mode                     lo      hi     (user request)
  R( 4, 0.10f,  4.0f);   // earthquake — now speed-responsive (switch passes `at`)
  R( 8, 0.10f,  8.0f);   // mitosis            max 2x
  R(11, 0.025f, 4.0f);   // pulsar             min ¼
  R(12, 0.10f,  6.0f);   // liquidMetal        max +50%
  R(15, 0.10f,  8.0f);   // lavaLamp           max 2x
  R(17, 0.10f,  8.0f);   // gravityLens        max 2x
  R(21, 0.05f,  4.0f);   // squareRain         min −50%
  R(22, 0.10f, 12.0f);   // depthMap           max 3x
  R(23, 0.10f, 20.0f);   // cosmicDust         max 5x
  R(24, 0.10f,  8.0f);   // heartglow          max 2x
  R(25, 0.02f,  4.0f);   // evolution          min 20%
  R(26, 0.05f,  8.0f);   // morph              max 2x, min slower
  R(27, 0.05f, 20.0f);   // strangeAttractor   max 5x, min ½
  R(30, 0.10f,  8.0f);   // tetris             max 2x
  R(31, 0.10f, 11.0f);   // bouncingLogo       max ~3x
  R(32, 0.10f,  8.0f);   // starTunnel3D       max 2x
  R(34, 0.10f, 12.0f);   // spiralGalaxy       max 3x
  R(35, 0.10f, 11.0f);   // boids              max ~3x
  R(40, 0.10f, 12.0f);   // smiley             max 3x
  R(43, 0.10f,  8.0f);   // warp               max 2x
  R(44, 0.10f,  8.0f);   // shootingStars      max 2x
  R(46, 0.10f,  6.0f);   // pulsar2            max 1.5x
  R(53, 0.10f,  6.0f);   // aliens             a touch faster
  R(56, 0.10f, 12.0f);   // confetti           max 3x
  R(57, 0.10f, 11.0f);   // spiralEject        max ~3x
  R(58, 0.025f, 4.0f);   // clouds3d           min ¼
  R(61, 0.10f,  6.0f);   // starsFlying        max 1.5x
  R(62, 0.20f,  6.0f);   // firework2          max 1.5x, min 2x faster
  R(66, 0.20f,  8.0f);   // squareImplode      max 2x, min 2x faster
  R(67, 0.50f,  4.0f);   // plasmaLightning    lift floor so slow still shows
  // Drahthaus pack: the slow songs get a low floor, the rave a high ceiling.
  R(109, 0.10f, 8.0f);   // dhWhereDidYouSleep = mitosis (same window as mode 8)
  R(111, 0.10f, 3.0f);   // dhCryMeARiver       river flow
  R(114, 0.05f, 3.0f);   // dhSabbia1           hourglass trickle
  R(116, 0.20f, 6.0f);   // dhMuecke            the mosquito can get frantic
  R(118, 0.20f, 8.0f);   // dhDienstagRave = quadMirror, max 2x
  R(120, 0.20f, 8.0f);   // quadMirror          DJ: max 2x
  R(122, 0.20f, 8.0f);   // gridPulse           DJ: max 2x
}
// Map the 0.1..4.0 knob onto this mode's [lo..hi] window (linear).
float togaModeSpeed(int mode, float knob){
  const SpeedRange& s = gModeSpeed[constrain(mode,0,MAX_MODES-1)];
  float f = (constrain(knob,0.1f,4.0f) - 0.1f) / 3.9f;   // 0..1
  return s.lo + (s.hi - s.lo) * f;
}
// ── Auto brightness normalization state (Task 2b) ──
uint8_t gBaseBri=40;     // per-frame base brightness the AGC multiplies (set by loop)
float   gAgcGain=1.0f;   // smoothed AGC gain
bool    gAgcSnap=false;  // snap gain straight to target next frame (set on mode change)
// ── Beat-Glow: tapped tempo pulses the whole frame (does NOT touch animSpeed) ──
// The tempo and phase live in grid_beat.h; this is just the value sampled once
// per frame, so every LED in a frame is scaled by the same factor.
float   gBeatGlow  =1.0f;  // brightness multiplier from the beat pulse (0.25..1.0)

// ── Mode-number overlay ──
// It lives in grid_text.h, which is included further down (it needs the drawing
// primitives), so forward-declare what showGrid() needs. It HAS to be painted
// inside showGrid(): drawing it after the animation's show() meant a second
// full FastLED.show() per frame — the first one without the number, the second
// with. A show over 12 strips takes 10-15ms, so the number was dark for about
// half of every frame and visibly strobed. One show, one number, steady.
extern uint32_t overlayUntil;
void drawNumberTopRight(uint32_t t);
bool gOverlayPainted=false;   // did showGrid() paint it this frame?
bool gHideOverlay=false;      // Drahthaus profile: no mode-number / mood readout on the wall (v11)

// ── Bloom / shine ──────────────────────────────────────────────────────────
// Spills a fraction of every lit LED onto its two strip-neighbours, giving the
// hard-pixel legacy modes the same soft glowing "shine" the fx-based modes get
// for free from their anti-aliasing. Blurs ALONG each physical strip only: each
// electrode buffer holds two contiguous active regions ([0..120] and [136..256],
// the two strips it drives) separated by dead spacer LEDs — clamp at the region
// edges so light never bleeds across a strip or into a spacer. Runs before the
// hue/AGC pass (so the AGC re-normalises the added glow) and before the crisp
// mode-number overlay (so the readout stays sharp). gBloom (0..255) is live-
// tunable; 0 = off. Subtle by design — it lifts the look without smearing shapes.
// The three "new-style" knobs, all live-tunable (0 = off):
uint8_t gBloom   = 56;   // base neighbour glow spill, ALL modes (≈0.22)
uint8_t gHot     = 150;  // legacy only: white-hot highlight strength (bright pixels bleach toward white)
uint8_t gShimmer = 36;   // legacy only: gentle shimmer amplitude ("flaky" life)
bool    gLegacy  = false; // set per-frame in showGrid: the pre-showpiece modes (<68) get the retrofit
static uint8_t gBloomEff=56;
static void bloomRegion(CRGB* b,int lo,int hi){
  static CRGB tmp[121];
  int n=hi-lo+1;
  for(int i=0;i<n;i++) tmp[i]=b[lo+i];
  for(int i=0;i<n;i++){
    const CRGB& L=tmp[i>0?i-1:0]; const CRGB& R=tmp[i<n-1?i+1:n-1];
    int r=b[lo+i].r+(((int)L.r+R.r)*gBloomEff>>8);
    int g=b[lo+i].g+(((int)L.g+R.g)*gBloomEff>>8);
    int bl=b[lo+i].b+(((int)L.b+R.b)*gBloomEff>>8);
    b[lo+i].r=r>255?255:r; b[lo+i].g=g>255?255:g; b[lo+i].b=bl>255?255:bl;
  }
}
static void bloomFrame(){
  gBloomEff = gLegacy ? (uint8_t)min((int)gBloom+34,120) : gBloom;   // stronger glow for the flat legacy modes
  if(!gBloomEff) return;
  for(int e=0;e<NUM_ELEC;e++){
    bloomRegion(ledsX[e],0,120); bloomRegion(ledsX[e],136,256);
    bloomRegion(ledsY[e],0,120); bloomRegion(ledsY[e],136,256);
  }
}
// Per-pixel "legacy polish": bleach the bright pixels toward white (glowing hot
// cores) and lay a gentle out-of-phase shimmer over them — the shiny/flaky surface
// the fx-based modes get for free, retrofitted onto the flatter older modes without
// touching their motion or shapes. No-op on the new showpiece modes (gLegacy=false).
static inline void polishPixel(CRGB& p,int idx,int e,uint8_t shimT){
  if(!(p.r|p.g|p.b)) return;
  uint8_t mx=p.r>p.g?(p.r>p.b?p.r:p.b):(p.g>p.b?p.g:p.b);      // brightest channel
  if(mx>90 && gHot){                                            // white-hot only on the bright pixels
    uint16_t w=((uint16_t)(mx-90)*gHot)/165;                   // 0..gHot as mx runs 90..255
    p.r+=(uint8_t)(((mx-p.r)*w)>>8); p.g+=(uint8_t)(((mx-p.g)*w)>>8); p.b+=(uint8_t)(((mx-p.b)*w)>>8);
  }
  if(gShimmer){                                                 // rolling shimmer; idx*37 → neighbours out of phase
    uint8_t s=sin8((uint8_t)(idx*37+e*90+shimT));
    uint16_t f=255-(((uint16_t)gShimmer*(uint8_t)(255-s))>>8);  // in [255-gShimmer , 255]
    p.r=(uint8_t)((p.r*f)>>8); p.g=(uint8_t)((p.g*f)>>8); p.b=(uint8_t)((p.b*f)>>8);
  }
}

// Central show: rotates every lit pixel's hue by (base+auto), measures total light for
// the AGC, sets the normalized brightness, then pushes to LEDs.
void showGrid(){
  bloomFrame();                      // soft glow first, so the AGC normalises it and the overlay stays crisp
  uint8_t off=(uint8_t)(gHueBase+gHueAuto);
  uint8_t shimT=(uint8_t)(millis()>>4);   // shimmer clock (~4s wrap)
  uint32_t lum=0;
  uint32_t litCnt=0;               // # of active LEDs meaningfully lit → drives the coverage sync
  uint32_t sumR=0,sumG=0,sumB=0;   // running colour of the lit pixels → dominant frame hue (post-rotation)
  const int COVER_THR=24;          // sum-of-channels a pixel must beat to count as "lit" (skips AA fringe)
  for(int e=0;e<NUM_ELEC;e++) for(int i=0;i<LEDS_PER_STRIP;i++){
    CRGB* p=&ledsX[e][i]; if(off && (p->r|p->g|p->b)){ CHSV h=rgb2hsv_approximate(*p); h.hue+=off; *p=h; }
    if(gLegacy) polishPixel(*p,i,e,shimT);
    { int s=p->r+p->g+p->b; lum+=s; if(s>COVER_THR){ litCnt++; sumR+=p->r; sumG+=p->g; sumB+=p->b; } }
    p=&ledsY[e][i];       if(off && (p->r|p->g|p->b)){ CHSV h=rgb2hsv_approximate(*p); h.hue+=off; *p=h; }
    if(gLegacy) polishPixel(*p,i,e+5,shimT);
    { int s=p->r+p->g+p->b; lum+=s; if(s>COVER_THR){ litCnt++; sumR+=p->r; sumG+=p->g; sumB+=p->b; } }
  }
  // Dominant wall colour: average the lit pixels (already hue-rotated above) and keep its
  // hue. FOLLOW modules paint their animation in this hue so they wear the grid's real
  // colour, not a hand-authored guess. Nothing lit → hold the last hue (avoids /0 flicker).
  if(litCnt){ CRGB avg((uint8_t)(sumR/litCnt),(uint8_t)(sumG/litCnt),(uint8_t)(sumB/litCnt)); gFrameHue=rgb2hsv_approximate(avg).hue; }
  // Coverage = lit active LEDs / total active LEDs. Each electrode buffer holds two
  // 121-LED strips (odd+even) for X and again for Y; dead spacer/return LEDs are
  // always black so they never reach COVER_THR and drop out of the count for free.
  const uint32_t activeN = (uint32_t)NUM_ELEC*2u*2u*LEDS_ACTIVE;   // 6*2*2*121 = 2904
  gCoverage = (uint8_t)constrain((int)((litCnt*255u + activeN/2)/activeN),0,255);
  // AGC: scale a global gain so every mode lands near a common perceived level.
  const float N=(float)NUM_ELEC*LEDS_PER_STRIP*2.0f;
  float avg=(float)lum/N;                                        // avg sum-of-channels/pixel
  float gainTarget=constrain(AGC_TARGET/fmaxf(avg,1.0f),AGC_MIN,AGC_MAX);
  if(gAgcSnap){ gAgcGain=gainTarget; gAgcSnap=false; }           // instant on mode change
  else         gAgcGain+=(gainTarget-gAgcGain)*AGC_K;            // else slow-smooth
  FastLED.setBrightness((uint8_t)constrain((int)lroundf(gBaseBri*gAgcGain*gBeatGlow),0,MAX_BRIGHTNESS));
  // Mode number last: after the hue rotation above, so it is never tinted, and
  // after the AGC measurement, so a bright glyph cannot dim the whole wall.
  // Only ever SET the flag here — loop() clears it once per frame. Clearing it
  // here would strand it at `true` on the one mode that never calls showGrid().
  if((uint32_t)millis()<overlayUntil){ drawNumberTopRight(millis()); gOverlayPainted=true; }
  FastLED.show();
}

// 15. SHOCKWAVE CHAIN (dimmed 35%)
#define NUM_SHOCKS 4

// 37. SQUARE RAIN
// Random squares of varying sizes (1x1 to 4x4) flash on and fade out.
// Like raindrops hitting a grid surface. Each drop independent.
#define SQRAIN_COUNT 30

// ══════════════════════════════════════════════
// ANIMATION 57: WAVE PHYSICS
// Real 2D wave equation on a 32x32 grid.
// Stones dropped at random intervals create ring waves
// that propagate, reflect off boundaries, and interfere.
// System never repeats — timing and position are random,
// but physics is deterministic and beautiful.
// ══════════════════════════════════════════════

#define WV_W 32
#define WV_H 32
// Trail: store recent positions
#define LOR_TRAIL 80

// ══════════════════════════════════════════════
// ANIMATION 59: HARMONIC RESONANCE
// Multiple oscillators with drifting frequencies.
// When frequencies align → constructive interference → bright.
// When they diverge → destructive → dark.
// The system breathes between resonance and silence.
// ══════════════════════════════════════════════

#define HR_OSC 5

// ══════════════════════════════════════════════
// ANIMATION 60: CONTINUOUS CELLULAR AUTOMATON
// Each cell has a float value (not just 0/1).
// Rules are soft weighted averages — no hard thresholds.
// System self-organises into complex stable patterns
// that drift and evolve over time.
// ══════════════════════════════════════════════

// ══════════════════════════════════════════════
// ANIMATION 60: NEURAL FIRE
// Excitable medium — cells have 3 states:
//   REST: can be excited by neighbours
//   FIRED: bright flash, excites neighbours
//   REFRACTORY: recovering, cannot fire
// Creates propagating waves, spiral waves, chaos.
// Parameters drift between these regimes organically.
// ══════════════════════════════════════════════

#define NF_W 24
#define NF_H 24
#define NF_REST 0
#define NF_FIRED 1
#define NF_REFRACT 2

// ══════════════════════════════════════════════
// ANIMATION 61: PLASMA LIGHTNING
// Energy field builds up across the grid.
// When threshold exceeded, Lichtenberg-figure lightning
// branches through the strips — real branching discharge.
// Uses the arm strobe system from the beginning + energy field.
// ══════════════════════════════════════════════

#define PL_N 12

// Lightning bolt: a branching path through the intersection grid
#define PL_BOLT_MAX 30
#define PL_MAX_BOLTS 4

// ═══════════════════════════════════════════════════════
//  LED Grid Receiver — ESP-NOW from NeoTrellis Sender
//  Board: ESP32 (MAC: f4:65:0b:e8:98:60)
//  Sender NeoTrellis ESP32 MAC: 44:1d:64:fa:b0:f8
// ═══════════════════════════════════════════════════════

#include <esp_now.h>
#include <WiFi.h>

// ── Volatile state updated by ESP-NOW callback ────────
volatile TogaCmdMsg cmdQ[16];                  // command ring buffer
volatile uint32_t cmdQT[16];                   // arrival time of each queued command
volatile uint8_t  cmdHead=0, cmdTail=0;
volatile bool     espStrobe       = false;
volatile bool     espModeStrobe   = false;
volatile bool     espCastleStrobe = false;
volatile uint8_t  espTextStrobe   = 0;       // TOGA_TEXT_* while a Drahthaus text key is held (v11)
volatile uint8_t  espProfile      = TOGA_PROFILE_TOGA;   // controller profile, on every packet (v11)
volatile bool     espBlackout     = false;   // latched blackout (button 10); rides the flags like the strobes
volatile bool     espSpot         = false;   // moving-spot key (phys 5); modules chase dots, wall drives a tunnel
volatile bool     espSweep        = false;   // latched speed-sweep (phys 4 / logical 2); animSpeed rides a triangle
bool              gSweepOn        = false;   // espSweep gated by senderAlive — loop-owned; a dead controller releases it
volatile uint8_t  espAutoMode     = TOGA_AUTO_OFF;   // 0 = off, 1..4 = mood (button 0)
volatile uint32_t espLastHeartbeat = 0;
static uint8_t    senderMAC[6]     = {};
static bool       senderKnown      = false;
// Recently applied command seqs (dedupe of the controller's 3 spread copies).
#define CMD_SEEN_N  8
#define CMD_SEEN_MS 500
struct CmdSeen { uint16_t seq; uint32_t at; bool valid; };
static CmdSeen    cmdSeen[CMD_SEEN_N] = {};
static uint8_t    cmdSeenPos       = 0;
static uint8_t    gMyMac[6]        = {};             // set in setup(), before the cb goes live

void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  TogaCmdMsg msg;
  if(!togaParseCmd(data, len, MY_TOGA_GROUP, &msg)) return;   // magic + version + group
  espStrobe        = (msg.flags & TOGA_F_STROBE)        != 0;
  espModeStrobe    = (msg.flags & TOGA_F_MODE_STROBE)   != 0;
  espCastleStrobe  = (msg.flags & TOGA_F_CASTLE_STROBE) != 0;
  espTextStrobe    = (msg.textStrobe<=TOGA_TEXT_COUNT) ? msg.textStrobe : 0;
  espProfile       = (msg.profile==TOGA_PROFILE_DRAHTHAUS) ? TOGA_PROFILE_DRAHTHAUS : TOGA_PROFILE_TOGA;
  espBlackout      = (msg.flags & TOGA_F_BLACKOUT)      != 0;
  espSpot          = (msg.flags & TOGA_F_SPOT)          != 0;
  espSweep         = (msg.flags & TOGA_F_SWEEP)         != 0;
  espAutoMode      = (msg.autoMode<=TOGA_AUTO_COUNT) ? msg.autoMode : TOGA_AUTO_OFF;
  uint32_t rx = millis();
  espLastHeartbeat = rx;
  if(msg.cmd != TOGA_CMD_HEARTBEAT){
    // The controller sends each one-shot command 3x (broadcast has no MAC-layer
    // retry), the repeats spread over ~25ms so one burst of interference cannot
    // eat all three. All copies carry one seq: apply the first, drop the rest.
    // The copies of two quick presses can interleave (A B A' B'), so remember the
    // last few seqs, not just the last — and only for a short window, so a
    // rebooted controller restarting its seq at 0 is never mistaken for a repeat.
    bool dup = false;
    for(uint8_t i=0;i<CMD_SEEN_N;i++)
      if(cmdSeen[i].valid && cmdSeen[i].seq==msg.h.seq && (rx-cmdSeen[i].at)<CMD_SEEN_MS){ dup=true; break; }
    if(!dup){ cmdSeen[cmdSeenPos]={msg.h.seq,rx,true}; cmdSeenPos=(cmdSeenPos+1)%CMD_SEEN_N; }
    // Stamp the arrival time: loop() drains this queue only once per frame, so
    // by the time a command is applied it can be 20ms stale. Irrelevant for a
    // +1 step, fatal for a beat tap — the tap IS the phase reference.
    if(!dup){ uint8_t nh=(cmdHead+1)&15; if(nh!=cmdTail){ memcpy((void*)&cmdQ[cmdHead],&msg,sizeof(msg)); cmdQT[cmdHead]=rx; cmdHead=nh; } }
  }
  // Learn only after full validation, so stray ESP-NOW traffic can't poison
  // this permanently. Logging only — the sync goes out as a broadcast.
  //
  // We do NOT filter on this MAC, and that is a deliberate trade: locking to the
  // first talker would let a node that boots earlier than the controller take
  // the bus and never give it back. But a SECOND commanding node is always a
  // fault — every command here is momentary state (strobe flags, auto mode),
  // so two senders means the last packet wins and buttons let go by themselves.
  // That is invisible from the outside and cost real debugging time once, so
  // say it out loud instead of leaving it to be rediscovered.
  if(senderKnown && memcmp(senderMAC, mac, 6) != 0) {
    static uint32_t lastWarn = 0;                      // rate-limited: this is a hot callback
    if(rx - lastWarn > 2000) {
      lastWarn = rx;
      Serial.printf("WARN: 2. Befehls-Sender %02X:%02X:%02X:%02X:%02X:%02X "
                    "(bekannt: %02X:%02X:%02X:%02X:%02X:%02X) — Strobe/Auto flackern!\n",
        mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
        senderMAC[0],senderMAC[1],senderMAC[2],senderMAC[3],senderMAC[4],senderMAC[5]);
    }
  }
  if(!senderKnown) {
    memcpy(senderMAC, mac, 6);
    senderKnown = true;
    Serial.printf("Sender: %02X:%02X:%02X:%02X:%02X:%02X\n",
      mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  }
}

// ── Persistent settings ───────────────────────────────
#include <Preferences.h>
Preferences prefs;

struct Settings {
  int     animIndex;
  uint8_t brightness;             // EFFECTIVE brightness = globalBri * modeFactor[animIndex]/255
  uint8_t globalBri;              // global master, raw 0..MAX_BRIGHTNESS (0 = off .. 100%)
  uint8_t modeFactor[MAX_MODES];  // per-mode factor 0..255 (0..1); scales the global master
  uint8_t strobeBrightness;
  uint8_t strobeOnMs;
  uint8_t strobeOffMs;
  uint8_t strobeSquares;
  uint8_t strobeGapMs;    // (legacy) button-15 gap; strobeSpeed drives the timing now
  uint8_t strobeSpeed;    // ONE global strobe speed 0..STROBE_SPEED_MAX; derives strobeOnMs/OffMs for ALL three strobes
  uint16_t autoDwellS;    // auto-mode dwell per mode, seconds (web UI, TOGA_P_AUTOINT)
  uint16_t sweepPeriodS;  // speed-sweep min→max→min period, seconds (web UI, TOGA_P_SWEEPINT)
  float   animSpeed;      // ACTIVE mode's speed 0.1..4.0 (mirrors speedByMode[animIndex])
  float   speedByMode[MAX_MODES];  // per-mode remembered speed
  uint16_t beatMs;        // tapped beat period for the glow, 0 = off
  // Per-PROFILE globals (v11): the Colour knob, strobe colour, global master and
  // strobe speed each profile had when it was last active, so switching Togalicht
  // ↔ Drahthaus brings back exactly that profile's look. `profile` is the slot the
  // live values above belong to right now. (Per-MODE speed / hue-speed / factor are
  // already separate because the two profiles run different modes.)
  struct ProfileVals { uint8_t globalBri, strobeSpeed, hueBase, strobeHue; } pv[2];
  uint8_t profile;
};
Settings cfg;
static inline void profileStash(){ cfg.pv[cfg.profile].globalBri=cfg.globalBri; cfg.pv[cfg.profile].strobeSpeed=cfg.strobeSpeed; cfg.pv[cfg.profile].hueBase=gHueBase; cfg.pv[cfg.profile].strobeHue=gStrobeHue; }

// Fold the global master and the active mode's factor into the one effective
// value everything downstream reads (AGC base, strobe scaling, sync to modules).
// Call after any change to globalBri, modeFactor[animIndex], or animIndex.
void recomputeBrightness() {
  cfg.brightness = (uint8_t)constrain(
      lround((long)cfg.globalBri * cfg.modeFactor[cfg.animIndex] / 255.0),
      0, MAX_BRIGHTNESS);                          // 0 allowed so the master can go fully off
  FastLED.setBrightness(cfg.brightness);
}

// Restore the ACTIVE values for the current mode: brightness (global*factor),
// speed and hue-speed each come from that mode's own remembered slot. Call
// whenever animIndex changes.
void applyModeSettings() {
  cfg.animSpeed = cfg.speedByMode[cfg.animIndex];
  gHueSpeed     = hueSpeedByMode[cfg.animIndex];
  gHueBase      = hueBaseByMode[cfg.animIndex];   // each mode keeps its own colour
  gHueReset     = true;
  recomputeBrightness();
}

void saveSettings() {
  prefs.begin("grid", false);
  prefs.putInt("anim",   cfg.animIndex);
  prefs.putUChar("gbri", cfg.globalBri);                            // global master
  prefs.putBytes("mfac", cfg.modeFactor, sizeof(cfg.modeFactor));   // per-mode brightness factor
  prefs.putUChar("sbri", cfg.strobeBrightness);
  prefs.putUChar("son",  cfg.strobeOnMs);
  prefs.putUChar("soff", cfg.strobeOffMs);
  prefs.putUChar("ssq",  cfg.strobeSquares);
  prefs.putUChar("sgap", cfg.strobeGapMs);
  prefs.putUChar("sspd", cfg.strobeSpeed);    // global strobe speed
  prefs.putUShort("aint", cfg.autoDwellS);    // auto-mode dwell (s)
  prefs.putUShort("wint", cfg.sweepPeriodS);  // sweep period (s)
  prefs.putBytes("spdm", cfg.speedByMode, sizeof(cfg.speedByMode));   // per-mode speed
  prefs.putBytes("hspm", hueSpeedByMode, sizeof(hueSpeedByMode));     // per-mode hue-speed
  prefs.putBytes("hbm",  hueBaseByMode,  sizeof(hueBaseByMode));      // per-mode colour
  prefs.putUChar("txon", gTxOn); prefs.putBytes("txh", gTxHue, 8); prefs.putBytes("txs", gTxSat, 8);   // text-strobe colours
  prefs.putUShort("beat", cfg.beatMs);        // beat-glow tempo
  prefs.putUChar("hue",  gHueBase);
  prefs.putUChar("shue", gStrobeHue);
  profileStash();                             // the live values ARE the active profile's slot
  prefs.putUChar("prof", cfg.profile);
  prefs.putBytes("pv",   cfg.pv, sizeof(cfg.pv));
  prefs.end();
}

// Settings go to NVS LAZILY. Every mode step / param step used to call
// saveSettings() on the spot: ~1KB of blobs per press, 16x/s while a +/- key
// auto-repeats. Each flash write stalls the loop (cache off), and when an NVS
// page fills up the erase stalls it for 100+ ms — commands queue up meanwhile
// and land in a burst: "doesn't react … then suddenly does". Now a change only
// marks the settings dirty; loop() writes them ONCE after SETTINGS_SAVE_QUIET ms
// without a further change (never mid-strobe). The live value applies instantly
// either way — only persistence waits. Cost: a power cut inside that window
// forgets the very last change.
static bool     settingsDirty   = false;
static uint32_t settingsDirtyAt = 0;
#define SETTINGS_SAVE_QUIET 1500
static inline void markSettingsDirty(){ settingsDirty=true; settingsDirtyAt=millis(); }

void loadSettings() {
  prefs.begin("grid", true);
  cfg.animIndex        = prefs.getInt("anim",   0);
  cfg.globalBri        = prefs.getUChar("gbri", BRI_DEFAULT);   // global master
  bool facOk           = prefs.getBytes("mfac", cfg.modeFactor, sizeof(cfg.modeFactor))
                           == sizeof(cfg.modeFactor);   // per-mode brightness-factor blob
  cfg.strobeBrightness = prefs.getUChar("sbri", 200);
  cfg.strobeOnMs       = prefs.getUChar("son",  40);
  cfg.strobeOffMs      = prefs.getUChar("soff", 40);
  cfg.strobeSquares    = prefs.getUChar("ssq",  30);
  cfg.strobeGapMs      = prefs.getUChar("sgap", STROBE_GAP_DEF);
  cfg.strobeSpeed      = prefs.getUChar("sspd", STROBE_SPEED_DEF);   // global strobe speed
  cfg.autoDwellS       = prefs.getUShort("aint", AUTO_INT_DEF);      // auto-mode dwell (s)
  cfg.sweepPeriodS     = prefs.getUShort("wint", SWEEP_INT_DEF);     // sweep period (s)
  bool spdOk           = prefs.getBytes("spdm", cfg.speedByMode, sizeof(cfg.speedByMode))
                           == sizeof(cfg.speedByMode);   // per-mode speed blob
  bool hspOk           = prefs.getBytes("hspm", hueSpeedByMode, sizeof(hueSpeedByMode))
                           == sizeof(hueSpeedByMode);     // per-mode hue-speed blob
  cfg.beatMs           = prefs.getUShort("beat", 0);
  gHueBase             = prefs.getUChar("hue",  0);
  gStrobeHue           = prefs.getUChar("shue", 0);
  bool hbmOk           = prefs.getBytes("hbm", hueBaseByMode, sizeof(hueBaseByMode)) == sizeof(hueBaseByMode);
  gTxOn = prefs.getUChar("txon", 0);
  if(prefs.getBytes("txh", gTxHue, 8)!=8 || prefs.getBytes("txs", gTxSat, 8)!=8){ gTxOn=0; memset(gTxHue,0,8); memset(gTxSat,0,8); }
  cfg.profile          = prefs.getUChar("prof", TOGA_PROFILE_TOGA);
  bool pvOk            = prefs.getBytes("pv", cfg.pv, sizeof(cfg.pv)) == sizeof(cfg.pv);
  prefs.end();
  if(cfg.profile!=TOGA_PROFILE_DRAHTHAUS) cfg.profile=TOGA_PROFILE_TOGA;
  if(!pvOk){ uint8_t p=cfg.profile; cfg.profile=0; profileStash(); cfg.profile=1; profileStash(); cfg.profile=p; }   // first boot: both slots = the live values
  else { const Settings::ProfileVals& v=cfg.pv[cfg.profile]; cfg.globalBri=v.globalBri; cfg.strobeSpeed=v.strobeSpeed; gStrobeHue=v.strobeHue; }   // colour is per MODE now (hueBaseByMode)
  cfg.animIndex        = constrain(cfg.animIndex, 0, TOTAL_ANIMS-1);
  cfg.globalBri        = constrain(cfg.globalBri, BRI_STEP, MAX_BRIGHTNESS);   // min = one step above off; blackout owns fully-off
  // Per-mode blobs: seed defaults if missing, then clamp every slot (NVS is not trusted).
  if(!facOk) for(int i=0;i<MAX_MODES;i++) cfg.modeFactor[i]=BRICOEF_MAX;
  for(int i=0;i<MAX_MODES;i++) cfg.modeFactor[i]=constrain(cfg.modeFactor[i],BRICOEF_MIN,BRICOEF_MAX);
  if(!spdOk) for(int i=0;i<MAX_MODES;i++) cfg.speedByMode[i]=1.0f;
  for(int i=0;i<MAX_MODES;i++) cfg.speedByMode[i]=constrain(cfg.speedByMode[i],0.1f,4.0f);
  if(!hspOk) for(int i=0;i<MAX_MODES;i++) hueSpeedByMode[i]=0;
  // First boot with per-mode colour: the Togalicht modes keep the look they had (the
  // old global colour), the Drahthaus pack starts on its native colours (0).
  if(!hbmOk) for(int i=0;i<MAX_MODES;i++) hueBaseByMode[i]=(i<107)?gHueBase:0;
  for(int i=0;i<MAX_MODES;i++) hueSpeedByMode[i]=constrain(hueSpeedByMode[i],0,HUE_SPEED_MAX);
  applyModeSettings();                                            // active brightness/speed/hue-speed for this mode
  cfg.strobeBrightness = constrain(cfg.strobeBrightness, 1, 255);
  cfg.strobeSquares    = constrain(cfg.strobeSquares, 1, 122);
  cfg.strobeGapMs      = constrain(cfg.strobeGapMs, STROBE_GAP_MIN, STROBE_GAP_MAX);
  // Strobe timing is derived from the ONE global speed: on/off for strobe(15),
  // mode(14) and castle(13) all come from here, so they share the exact rhythm.
  cfg.strobeSpeed      = constrain(cfg.strobeSpeed, STROBE_SPEED_MIN, STROBE_SPEED_MAX);
  togaStrobeTiming(cfg.strobeSpeed, &cfg.strobeOnMs, &cfg.strobeOffMs);
  cfg.autoDwellS       = constrain(cfg.autoDwellS,   TIMING_INT_MIN, TIMING_INT_MAX);
  cfg.sweepPeriodS     = constrain(cfg.sweepPeriodS, TIMING_INT_MIN, TIMING_INT_MAX);
  // Beat-glow tempo: 0 stays off, otherwise clamp to a sane range and start the
  // clock. The phase is arbitrary until the first tap anchors it.
  if(cfg.beatMs){ cfg.beatMs=constrain(cfg.beatMs,BEAT_MIN_MS,BEAT_MAX_MS);
                  beatSetPeriod((float)cfg.beatMs,millis()); }
}
#define STUN_N 18
#define RRIP_N 7
#define BOID_N 10
#define FW_N 48
#define RUN_OBS 4


// ── Modularized code (drawing primitives, font/text, animations) ──
#include "grid_protos.h"
#include "grid_draw.h"
#include "grid_text.h"
#include "animations/anim_field.h"
#include "animations/anim_geometry.h"
#include "animations/anim_life.h"
#include "animations/anim_games.h"
#include "animations/anim_faces.h"
#include "animations/anim_new.h"
#include "animations/anim_new2.h"    // 5 new energy/plasma modes (68..71) + replaces 48
#include "animations/anim_new3.h"    // 15 showpiece modes (72..86)
#include "animations/anim_new4.h"    // 15 showpiece modes (87..101)
#include "animations/anim_new5.h"    // 5 next-tier modes (102..106): 3D/4D, morph, lensing
#include "animations/anim_konzert.h" // Drahthaus concert pack (107..122): marker, 12 song modes, 3 DJ modes

void resetAnimState(uint32_t t) {
  rainInit=false;shockInit=false;novaInit=false;blobInit=false;
  nerveInit=false;starInit=false;pendInit=false;sqRainInit=false;golInit=false;
  evoInit=false;morphInit=false;wvInit=false;lorInit=false;hrInit=false;nfInit=false;plInit=false;
  tetInit=false;snakeInit=false;logoInit=false;stunInit=false;rripInit=false;boidInit=false;fwInit=false;rdInit=false;
  stickInit=false;padInit=false;eyeInit=false;shxInit=false;
  kzInit=false;                  // Drahthaus pack (anim_konzert.h): every mode re-seeds on entry
  strikeTime=t;collideStrike=t;
  epiX=(random(80)-40);epiY=(random(80)-40);
  gAgcSnap=true;                 // snap AGC to the new mode's level (Task 2b)
  clearAll();delay(100);
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  initModeSpeed();
  loadSettings();
  Serial.printf("Loaded: anim=%d bri=%d strobeBri=%d on=%dms off=%dms\n",
    cfg.animIndex,cfg.brightness,cfg.strobeBrightness,cfg.strobeOnMs,cfg.strobeOffMs);

  FastLED.addLeds<WS2812B,PIN_X0,GRB>(ledsX[0],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_X1,GRB>(ledsX[1],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_X2,GRB>(ledsX[2],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_X3,GRB>(ledsX[3],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_X4,GRB>(ledsX[4],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_X5,GRB>(ledsX[5],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_Y0,GRB>(ledsY[0],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_Y1,GRB>(ledsY[1],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_Y2,GRB>(ledsY[2],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_Y3,GRB>(ledsY[3],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_Y4,GRB>(ledsY[4],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,PIN_Y5,GRB>(ledsY[5],LEDS_PER_STRIP);
  FastLED.setBrightness(cfg.brightness);
  clearAll();

  togaWifiBegin("toga-grid");                        // STA on togacontroller (ch1) — bus AND network
  WiFi.macAddress(gMyMac);                           // before the cb goes live: it filters on this
  Serial.printf("Receiver MAC: %s  ch=%d\n", WiFi.macAddress().c_str(), TOGA_CHANNEL);
  if(esp_now_init() != ESP_OK) { Serial.println("ESP-NOW init failed!"); while(1) delay(1000); }
  esp_now_register_recv_cb(onReceive);
  if(!togaAddPeer(TOGA_BCAST)) { Serial.println("Broadcast peer failed!"); while(1) delay(1000); }
  Serial.printf("Grid ready. %d animations.\n", TOTAL_ANIMS);
}

// ══════════════════════════════════════════════════════════════
//  Auto mood (controller button 0): four curated playlists.
//  The controller owns the on/off state and sends it on every packet; the
//  grid only walks the list. Each mood is a hand-picked set of modes plus the
//  dwell time that suits it — calm scenes get time to breathe, party ones cut
//  fast. Any manual mode step arrives with autoMode=0 and ends the automatic.
//
//  Auto steps are deliberately NOT persisted: a step every 12s all night would
//  burn the NVS write budget for nothing, and the mode worth restoring after a
//  reboot is the last one picked by hand, not wherever the automatic stopped.
// ══════════════════════════════════════════════════════════════
// Lists refreshed 2026-07-26 to weave in the 30 new showpiece modes (68..101).
static const uint8_t AUTO_CALM[]  = {14,15,58,50,78,72,71,87,97,55,24,81,84,86,12};       // langsam, flächig, atmend
static const uint8_t AUTO_SPACE[] = {19,43,32,34,77,89,92,93,7,45,99,100,85,44,61,82,75,102,103,105};  // Sterne, Nebel, Kosmos
static const uint8_t AUTO_PARTY[] = {2,21,36,62,56,63,64,10,68,74,88,98,96,73,95,69,70,57,33,60};  // schnell, elektrisch
static const uint8_t AUTO_SHOW[]  = {29,38,39,40,41,53,30,31,37,49,80,90,94,91,76,83,79,104};  // Figuren, Mechanik, Kreaturen
// 5th mood — the best of the new showpiece packs (the prismShards/nebulaBreath lineage + next tier).
static const uint8_t AUTO_SPECIAL[]= {70,71,72,77,94,98,91,89,86,88,82,92,101,90,85,99,73,79,100,84,102,103,104,105,106}; // Spezial — Best-of
struct AutoMood { const uint8_t* list; uint8_t n; uint16_t everyMs; };
static const AutoMood AUTO_MOODS[TOGA_AUTO_COUNT] = {   // everyMs = LEGACY/unused (autoDwellMs uses cfg.autoDwellS)
  {AUTO_CALM,    sizeof(AUTO_CALM),    60000},   // 1 Ruhe    — langsam, flächig
  {AUTO_SPACE,   sizeof(AUTO_SPACE),   60000},   // 2 Space   — Sterne, Nebel, Planeten
  {AUTO_PARTY,   sizeof(AUTO_PARTY),   60000},   // 3 Party   — schnell, hart geschnitten
  {AUTO_SHOW,    sizeof(AUTO_SHOW),    60000},   // 4 Show    — Spiele, Gesichter, Text, Mechanik
  {AUTO_SPECIAL, sizeof(AUTO_SPECIAL), 60000},   // 5 Spezial — Best-of der neuen Showpieces
};
// Drahthaus profile: the same five buttons, but only the strong Togalicht modes —
// the 100+ tier first, then the best showpieces — plus the concert's own DJ modes.
// The manual list in that profile is the setlist, so the automatic is where the
// Togalicht effects live during the concert (between songs, DJ set).
static const uint8_t DH_CALM[]   = {86,89,78,97,84,72,105,58,14,15,71};
static const uint8_t DH_SPACE[]  = {102,103,105,106,77,89,93,99,100,32,43,19};
static const uint8_t DH_PARTY[]  = {73,88,98,96,68,69,70,95,104,120,121,122,2,21,63};
static const uint8_t DH_SHOW[]   = {104,106,101,94,90,91,80,83,79,76,92,85};
static const uint8_t DH_SPECIAL[]= {100,101,102,103,104,105,106,99,98,96,94,92,91,89,88,86,85,84,82,79,77,73,72,70,71,68};
static const AutoMood AUTO_MOODS_DH[TOGA_AUTO_COUNT] = {
  {DH_CALM,    sizeof(DH_CALM),    60000},
  {DH_SPACE,   sizeof(DH_SPACE),   60000},
  {DH_PARTY,   sizeof(DH_PARTY),   60000},
  {DH_SHOW,    sizeof(DH_SHOW),    60000},
  {DH_SPECIAL, sizeof(DH_SPECIAL), 60000},
};
// A macro, not a function: arduino-cli hoists a prototype for every .ino function
// above the struct it returns, and `const AutoMood&` would then be an unknown type.
#define moodOf(mood) ((cfg.profile==TOGA_PROFILE_DRAHTHAUS ? AUTO_MOODS_DH : AUTO_MOODS)[(mood)-1])

// Runtime play order for the active mood: a shuffled permutation of the mood's
// list indices. Rebuilt every time the automatic (re)starts AND again after each
// full pass, so both the order and the mode it opens on are different each time.
// Sized for the longest mood list with head-room. See shuffleMood() + the auto
// block in loop().
static uint8_t moodOrder[32];
static void shuffleMood(uint8_t mood, int avoidFirst){   // avoidFirst: animIndex to NOT open on (-1 = don't care)
  const AutoMood& md=moodOf(mood);
  for(uint8_t i=0;i<md.n;i++) moodOrder[i]=i;
  for(int i=md.n-1;i>0;i--){ int j=random(i+1);          // Fisher-Yates on random() (seeded at boot)
    uint8_t tmp=moodOrder[i]; moodOrder[i]=moodOrder[j]; moodOrder[j]=tmp; }
  // Never reopen on the same mode two runs in a row — swap slot 0 with another.
  if(avoidFirst>=0 && md.n>1 && md.list[moodOrder[0]]==(uint8_t)avoidFirst){
    int j=1+random(md.n-1);
    uint8_t tmp=moodOrder[0]; moodOrder[0]=moodOrder[j]; moodOrder[j]=tmp;
  }
}

// How long the current mood holds a mode: a fixed dwell (default 60s), set from
// the web UI (cfg.autoDwellS) and the same for every mood, sweeping or not.
// Independent of the Speed knob — that scales the animation, not the auto dwell.
uint32_t autoDwellMs(uint8_t mood){
  (void)mood; return (uint32_t)cfg.autoDwellS * 1000u;   // web-UI value, same whether sweeping or not
}

// Switch to shuffle-position `pos` of `mood` (1-based), exactly as a manual step
// would — minus the save. `pos` walks moodOrder[], not the raw list, so the order
// follows the current shuffle.
void autoShowMode(uint8_t mood, uint8_t pos, uint32_t t){
  const AutoMood& md=moodOf(mood);
  cfg.animIndex=constrain((int)md.list[moodOrder[pos%md.n]],0,TOTAL_ANIMS-1);
  applyModeSettings();                             // brightness/speed/hue-speed for this mode
  resetAnimState(t);
  // NO mode-number overlay in auto mode: the wall should just play, silently.
  // Manual +/- steps (applyCommand) still flash the number; the mood letter
  // (A1..A4) still flashes on a button-0 press via triggerOverlayMood().
}

// ══════════════════════════════════════════════════════════════
//  Strobe looks (button 15). Five ways to draw ONE flash; which one is used is
//  drawn fresh on every PRESS, so a held strobe keeps its look and the next
//  press brings a new one. The flash itself stays as short as the LED refresh
//  allows and only the gap is tunable — see the render block in loop().
// ══════════════════════════════════════════════════════════════
#define STROBE_LOOKS    4   // geometric looks dealt from the no-repeat shuffle-bag (0..3)
#define SL_SQUARES      0   // random squares (the original look)
#define SL_STRIPS       1   // random whole strips
#define SL_RINGS        2   // square rings, middle → outside
#define SL_QUADRANTS    3   // one random quarter
#define SL_TOGA         4   // "TOGA" banner text — rare; NOT in the bag (see nextStrobeLook)
#define STROBE_TOGA_1_IN 15 // ~1-in-N presses flash the TOGA text instead of a geometric look
#define STROBE_STRIP_N  6   // strips lit per flash in SL_STRIPS

// Random, but never the same look twice in a row: deal the five out in a
// shuffled bag and only reshuffle once the bag is empty. Plain random() would
// repeat a look ~20% of the time, and a repeat reads as "the button did
// nothing" — which is the one thing this must never look like.
uint8_t nextStrobeLook(){
  static uint8_t bag[STROBE_LOOKS], left=0, last=255;
  // Rare special: ~1-in-STROBE_TOGA_1_IN presses flash the "TOGA" banner instead of a
  // geometric look. Rolled independently of the no-repeat bag (which only holds the four
  // geometric looks), so TOGA can surface any time without disturbing their rotation.
  if(random(STROBE_TOGA_1_IN)==0) return SL_TOGA;
  if(left==0){
    for(uint8_t i=0;i<STROBE_LOOKS;i++) bag[i]=i;
    for(int i=STROBE_LOOKS-1;i>0;i--){ int j=random(i+1); uint8_t tmp=bag[i]; bag[i]=bag[j]; bag[j]=tmp; }
    // A fresh bag may open on the look the last one closed with — the one seam
    // where a repeat can still slip through. Swap it away.
    if(bag[0]==last){ int j=1+random(STROBE_LOOKS-1); uint8_t tmp=bag[0]; bag[0]=bag[j]; bag[j]=tmp; }
    left=STROBE_LOOKS;
  }
  last=bag[STROBE_LOOKS-left]; left--;
  return last;
}

// One cell of the 11x11 grid = its four border segments; that is all the
// physical LEDs a "square" has.
void strobeCell(int row,int col,CRGB sc){
  int r1=row+1, c1=col+1, r2=row+2, c2=col+2;
  for(int led=INTS[col]+1;led<INTS[col+1];led++){ setLED(false,r1,led,sc); setLED(false,r2,led,sc); }
  for(int led=INTS[row]+1;led<INTS[row+1];led++){ setLED(true,c1,led,sc); setLED(true,c2,led,sc); }
}

// Draw flash `n` of the current press in `look`. Called with the buffer already
// cleared; must stay cheap enough to finish inside a flash.
void strobeFlash(uint8_t look, uint16_t n, CRGB sc){
  switch(look){
    case SL_TOGA: {
      // Wider 12-segment font (real capital T, not the 1-cell 7-seg): "TOGA" is 4 glyphs
      // × 3 node-cols and spans the full 0..11 lattice, so leftC=0 centres it. Strobe
      // colour, buffer already cleared by the caller.
      int leftC=(12-3*4)/2; if(leftC<0) leftC=0;
      drawTextCells("TOGA",leftC,TEXT_TOP,sc);
      break; }
    case SL_STRIPS:
      for(int s=0;s<STROBE_STRIP_N;s++){
        int strip=1+random(12); bool isX=random(2);      // a strip may be picked twice: harmless
        for(int led=1;led<=LEDS_ACTIVE;led++) setLED(isX,strip,led,sc);
      }
      break;
    case SL_RINGS: {
      int ring=n%6;                                       // 11x11 cells → 6 rings around the middle
      for(int r=0;r<11;r++) for(int c=0;c<11;c++)
        if(max(abs(r-5),abs(c-5))==ring) strobeCell(r,c,sc);
      break; }
    case SL_QUADRANTS: {
      int q=random(4); int r0=(q&1)?6:0, c0=(q&2)?6:0;    // row/col 5 stays dark: keeps the quarters apart
      for(int r=r0;r<r0+5;r++) for(int c=c0;c<c0+5;c++) strobeCell(r,c,sc);
      break; }
    default: {                                           // SL_SQUARES
      uint8_t count=(uint8_t)min((int)cfg.strobeSquares,121);
      static uint8_t sqIdx[121];
      for(int i=0;i<121;i++) sqIdx[i]=i;
      for(int i=0;i<(int)count;i++){ int j=i+random(121-i); uint8_t tmp=sqIdx[i]; sqIdx[i]=sqIdx[j]; sqIdx[j]=tmp; }
      for(int s=0;s<(int)count;s++) strobeCell(sqIdx[s]/11, sqIdx[s]%11, sc);
      break; }
  }
}

// Beat tempo is written to NVS lazily. Every tap used to mean a settings write;
// a night of tapping would burn through the flash's erase budget for a value
// nobody reads until the next boot. Mark it dirty instead and let loop() write
// it once the tapping has stopped.
static bool     beatDirty    = false;
static uint32_t beatDirtyAt  = 0;
#define BEAT_SAVE_QUIET 10000   // ms of no tempo change before it goes to NVS

// Apply one command from the controller. `t` is the command's ARRIVAL time, not
// the current frame time — see the stamp in onReceive().
void applyCommand(const TogaCmdMsg& m, uint32_t t){
  if(m.cmd==TOGA_CMD_MODE){                        // mode step
    cfg.animIndex=(cfg.animIndex+m.arg+TOTAL_ANIMS)%TOTAL_ANIMS;
    applyModeSettings();                           // brightness/speed/hue-speed for this mode
    resetAnimState(t); markSettingsDirty(); triggerOverlay(t,cfg.animIndex);
  } else if(m.cmd==TOGA_CMD_MODESET){              // absolute jump (Drahthaus setlist / web UI, v11)
    int idx=m.arg; if(idx<0||idx>=TOTAL_ANIMS) return;
    // Already there: the controller resends an absolute target until our sync
    // confirms it, so a repeat must be a no-op — no restart, no blank.
    if(idx==cfg.animIndex) return;
    Serial.printf("cmd %u: mode %d -> %d\n", m.h.seq, cfg.animIndex, idx);
    cfg.animIndex=idx;
    applyModeSettings();
    resetAnimState(t); markSettingsDirty(); triggerOverlay(t,cfg.animIndex);
  } else if(m.cmd==TOGA_CMD_TEXTCOL){              // one text strobe's colour (web UI)
    uint8_t id=m.target&0x7F; if(id<1||id>TOGA_TEXT_COUNT) return;
    if(m.target&0x80) gTxOn&=~(1<<id);
    else { gTxOn|=(1<<id); gTxHue[id]=(uint8_t)((m.arg>>8)&0xFF); gTxSat[id]=(uint8_t)(m.arg&0xFF); }
    gHueTabDue=true; markSettingsDirty();
  } else if(m.cmd==TOGA_CMD_PARAM){                 // parameter step
    int d=m.arg;
    switch(m.target){
      case TOGA_P_BRI: cfg.globalBri=(uint8_t)constrain((int)cfg.globalBri+d*BRI_STEP,BRI_STEP,MAX_BRIGHTNESS); recomputeBrightness(); break;  // global master; floor one step above off (blackout does off)
      case TOGA_P_BRICOEF: cfg.modeFactor[cfg.animIndex]=(uint8_t)constrain((int)cfg.modeFactor[cfg.animIndex]+d*BRICOEF_STEP,BRICOEF_MIN,BRICOEF_MAX); recomputeBrightness(); break;  // this mode's factor
      case TOGA_P_SPEED: cfg.animSpeed=constrain(cfg.animSpeed+d*0.1f,0.1f,4.0f); cfg.speedByMode[cfg.animIndex]=cfg.animSpeed; break;  // per-mode speed
      case TOGA_P_COLOR: gHueBase  =(uint8_t)(gHueBase  +d*6); hueBaseByMode[cfg.animIndex]=gHueBase; gHueTabDue=true; break;   // Color (wraps), per mode
      case TOGA_P_STROBEHUE: {                                          // Strobe colour: cycle a fixed palette so WHITE (slot 0) is always reachable
        static const uint8_t SH[]={0,255,16,32,64,96,128,160,192,224}; // 0=white; then red,orange,yellow,green,cyan,blue,purple,magenta
        const int N=sizeof(SH); int idx=0;
        for(int k=0;k<N;k++){ if(SH[k]==gStrobeHue){ idx=k; break; } } // find current slot (unknown value → falls back to white)
        gStrobeHue=SH[(idx+d+N)%N];                                    // step by ±1 and wrap — 0 is a real stop again
      } break;
      case TOGA_P_HUESPEED: gHueSpeed =(uint8_t)constrain((int)gHueSpeed+d,0,HUE_SPEED_MAX); hueSpeedByMode[cfg.animIndex]=gHueSpeed; break; // per-mode hue-speed
      case TOGA_P_STROBEGAP: cfg.strobeGapMs=(uint8_t)constrain((int)cfg.strobeGapMs+d*STROBE_GAP_STEP,
                                                 STROBE_GAP_MIN,STROBE_GAP_MAX); break; // legacy — no longer drives timing
      case TOGA_P_STROBESPEED:   // ONE global strobe speed: re-derive on/off for all three strobes
        cfg.strobeSpeed=(uint8_t)constrain((int)cfg.strobeSpeed+d*STROBE_SPEED_STEP,STROBE_SPEED_MIN,STROBE_SPEED_MAX);
        togaStrobeTiming(cfg.strobeSpeed,&cfg.strobeOnMs,&cfg.strobeOffMs); break;
      case TOGA_P_AUTOINT:  cfg.autoDwellS  =(uint16_t)constrain((int)d,TIMING_INT_MIN,TIMING_INT_MAX); break; // ABSOLUTE seconds (web UI)
      case TOGA_P_MODEHUE: {                       // ABSOLUTE colour of one mode, from the web UI
        int md=(m.arg>>8)&0x7F, h=m.arg&0xFF;
        if(md<TOTAL_ANIMS){ hueBaseByMode[md]=(uint8_t)h; if(md==cfg.animIndex) gHueBase=(uint8_t)h; gHueTabDue=true; }
      } break;
      case TOGA_P_SWEEPINT: cfg.sweepPeriodS=(uint16_t)constrain((int)d,TIMING_INT_MIN,TIMING_INT_MAX); break; // ABSOLUTE seconds (web UI)
    }
    markSettingsDirty();
  } else if(m.cmd==TOGA_CMD_BEAT_TAP){              // one tap of button 11 → glow rhythm (NOT speed)
    beatOnTap(t);                                   // t = arrival time: the phase anchor
    beatDirty=true; beatDirtyAt=millis();
  } else if(m.cmd==TOGA_CMD_BEAT){                  // period set directly, without tapping
    beatSetPeriod((float)m.arg,t);
    beatDirty=true; beatDirtyAt=millis();
  }
}

void loop() {
  togaWifiTick();      // the AP may come up after us, or come back

  static uint32_t lastFrame     = 0;
  static uint32_t lastT         = 0;
  static bool     strobeActive  = false;
  static bool     strobeOn      = false;
  static uint32_t strobeStart   = 0;            // phase origin: the period is measured from here
  static uint8_t  strobeLook    = SL_SQUARES;   // picked on each press, held for that press
  static uint16_t strobeCount   = 0;            // flashes so far in this press (SL_RINGS walks on it)
  static int      preStrobeAnim = 0;
  static uint8_t  preStrobeBri  = 40;

  uint32_t t  = millis();
  lastT       = t;

  // ── Sender alive? (heartbeat within timeout) ──────
  bool senderAlive = (espLastHeartbeat > 0) &&
                     ((t - (uint32_t)espLastHeartbeat) < (uint32_t)HEARTBEAT_TIMEOUT);
  gSweepOn = senderAlive && espSweep;   // sweep only while the controller is present (like blackout/spot)

  // ── Strobe: momentary, driven by the controller ──
  bool strobeHeld = senderAlive && espStrobe;
  if(strobeHeld && !strobeActive){
    preStrobeAnim=cfg.animIndex; preStrobeBri=cfg.brightness;
    strobeActive=true; strobeOn=false; strobeStart=t;
    strobeLook=nextStrobeLook(); strobeCount=0;   // every press a different look
  } else if(!strobeHeld && strobeActive){
    strobeActive=false; strobeOn=false;
    cfg.animIndex=preStrobeAnim; cfg.brightness=preStrobeBri;
    FastLED.setBrightness(cfg.brightness);
    FastLED.clear();
    for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
    FastLED.show();
  }

  // ── Apply queued commands from the controller ──────
  while(cmdTail!=cmdHead){
    TogaCmdMsg m; memcpy(&m,(const void*)&cmdQ[cmdTail],sizeof(m));
    uint32_t rx=cmdQT[cmdTail]; cmdTail=(cmdTail+1)&15;
    applyCommand(m,rx);          // arrival time, not `t`: a tap must not inherit this frame's delay
  }
  // Drahthaus profile: the wall shows no numbers. Follows the controller (it owns
  // the profile and sends it on every packet); a dead controller → Toga defaults.
  gHideOverlay = senderAlive && espProfile==TOGA_PROFILE_DRAHTHAUS;
  if(gHideOverlay) overlayUntil=0;
  // Profile switch (the controller owns it): park this profile's globals, bring
  // the other's back. Auto moods and the readout follow cfg.profile from here on.
  if(senderAlive && espProfile!=cfg.profile){
    profileStash();
    cfg.profile=espProfile;
    const Settings::ProfileVals& v=cfg.pv[cfg.profile];
    cfg.globalBri  =(uint8_t)constrain((int)v.globalBri,BRI_STEP,MAX_BRIGHTNESS);
    cfg.strobeSpeed=(uint8_t)constrain((int)v.strobeSpeed,STROBE_SPEED_MIN,STROBE_SPEED_MAX);
    gStrobeHue=v.strobeHue;                     // colour is per MODE (hueBaseByMode), not per profile
    togaStrobeTiming(cfg.strobeSpeed,&cfg.strobeOnMs,&cfg.strobeOffMs);
    recomputeBrightness();
    markSettingsDirty();
  }

  // ── Beat: a lone tap, then 2s of silence, switches the glow off ──
  // Sits here next to where taps go IN, and before the strobe block below —
  // that one returns early, and holding a strobe must not freeze the timeout.
  if(beatTick(t)){ beatDirty=true; beatDirtyAt=t; }   // persist the 0 like any tempo change

  // ── Auto mood: walk the active mood's playlist on its own interval ──
  // Skipped while the strobe is held: that path saves/restores animIndex around
  // itself, so a step now would be undone on release anyway. The timer keeps
  // running, so the next mode lands as soon as the strobe is let go.
  {
    uint8_t mood = senderAlive ? espAutoMode : TOGA_AUTO_OFF;   // sender gone → automatic stops
    static uint8_t  moodPrev=TOGA_AUTO_OFF, moodPos=0;
    static uint32_t moodStart=0;                                // when the current mode came up
    static int      lastAutoStart=-1;                           // animIndex the automatic last opened on
    if(!strobeActive){
      if(mood!=moodPrev){                                       // switched mood, or turned off
        uint8_t wasMood=moodPrev;                               // remember to persist the mode when auto ends by hand
        moodPrev=mood; moodPos=0;
        // Nothing is drawn when the mood turns OFF: that only ever happens on a
        // manual +/- step, which is already flashing the mode number the press
        // asked for, or on the sender going away, where the wall announcing "A0"
        // would be reporting the controller's silence as a setting.
        if(mood){
          shuffleMood(mood,lastAutoStart);                      // fresh order, opening on a new mode
          lastAutoStart=moodOf(mood).list[moodOrder[0]];
          autoShowMode(mood,moodPos,t); moodStart=t;
          // autoShowMode no longer draws the mode number (auto mode plays silent).
          // The mood letter is the answer to the button-0 press that just happened.
          triggerOverlayMood(t,mood);
        } else if(wasMood!=TOGA_AUTO_OFF && senderAlive){
          // Automatic just ended by hand (+/-): the mode showing right now becomes
          // the active one and is persisted, so it stays put and survives a reboot.
          markSettingsDirty();
        }
      } else if(mood && (t-moodStart)>=autoDwellMs(mood)){      // fixed 60s dwell
        moodPos++;
        if(moodPos>=moodOf(mood).n){ moodPos=0; shuffleMood(mood,-1); }  // reshuffle every full pass
        autoShowMode(mood,moodPos,t); moodStart=t;
      }
    }
  }

  // 50 FPS cap for animations — but ALL THREE strobes must run at full loop rate.
  // Otherwise their on/off edges are only sampled as often as a frame is allowed
  // (every 20ms) AND, worse, as often as the current mode can actually render: on
  // a heavy mode a single frame takes 40-80ms, so togaSquareOn was read that
  // rarely and the SAME strobe looked fast on a light mode, slow/absent on a heavy
  // one. The button-15 strobe was always exempt (strobeActive); the mode- and
  // castle-strobe now are too, so all three flash one uniform rhythm on every mode.
  bool strobeCapBypass = strobeActive ||
                         (senderAlive && (espModeStrobe || espCastleStrobe || espTextStrobe));
  // Deferred settings save (see markSettingsDirty): once the knobs have been quiet
  // for SETTINGS_SAVE_QUIET ms. Held back while any strobe runs — a flash stall
  // there would be a visible hiccup in the flash rhythm; it goes out on release.
  // Signed difference: the stamp is taken with millis() AFTER `t` (inside
  // applyCommand), so an unsigned t-stamp would wrap to ~4e9 and save at once.
  if(settingsDirty && !strobeCapBypass && (int32_t)(t-settingsDirtyAt)>=SETTINGS_SAVE_QUIET){
    settingsDirty=false; saveSettings();
  }
  if(!strobeCapBypass && t-lastFrame<20) return;
  lastFrame=t;

  // Accumulate scaled animation time
  // Use absolute time scaled by speed for animations
  static uint32_t lastAnimT = 0;
  if(lastAnimT==0) lastAnimT=t;
  float frameDt = (float)(t - lastAnimT);
  lastAnimT = t;

  // ── Beat-Glow: pulse the frame in the tapped rhythm. REAL time, never
  //    animSpeed — and sampled once here so one frame has one factor.
  //    Runs forever on the locked tempo; only new taps change it.
  gBeatGlow = beatFactor(t);

  // Tempo has settled → put it in NVS so it survives a reboot.
  // Signed: beatDirtyAt is stamped with millis() after `t` — unsigned, it wrapped
  // and every single beat tap was written to flash on the spot.
  if(beatDirty && (int32_t)(t-beatDirtyAt)>=BEAT_SAVE_QUIET){
    beatDirty=false;
    uint16_t per=(uint16_t)lroundf(bPeriod);
    if(per!=cfg.beatMs){ cfg.beatMs=per; markSettingsDirty(); }
  }
  // Speed-sweep override: while on, the animation speed rides a min→max→min triangle
  // instead of the mode's saved speed. The clock is ABSOLUTE (t), so switching mode
  // just picks the sweep up where it is; auto-mode sweeps at half the 120s dwell.
  float knobSpeed = gSweepOn ? togaSweepSpeed(t, (uint32_t)cfg.sweepPeriodS * 1000u)
                             : cfg.animSpeed;
  float animSpeed = togaModeSpeed(cfg.animIndex, knobSpeed);   // per-mode [lo..hi] remap
  static float virtualT = 0.0f;
  virtualT += frameDt * animSpeed;
  uint32_t at = (uint32_t)virtualT;
  float    dt = frameDt * animSpeed;

  // ── Auto-rotate the global hue by Hue-speed ───────
  // At Hue-speed 0 the colour must HOLD and only the Color key (button 8) may change
  // it. The instant rotation stops, fold the current auto-offset into the base — no
  // visible jump — and zero the offset; from then on the wall shows exactly gHueBase
  // and never drifts. (RAM-only fold: no NVS write, so an auto-mode mode that lands
  // on hue-speed 0 does not hammer flash. gHueBase persists on the next manual save.)
  static float hueAcc=0; static uint8_t prevHueSpeed=0;
  if(gHueReset){ gHueReset=false; hueAcc=0; gHueAuto=0; }   // new mode → start exactly on its saved colour
  if(gHueSpeed==0){
    if(prevHueSpeed!=0){ gHueBase=(uint8_t)(gHueBase+gHueAuto); hueBaseByMode[cfg.animIndex]=gHueBase; gHueAuto=0; hueAcc=0; }
  } else {
    hueAcc+=togaHueRate(gHueSpeed)*frameDt; if(hueAcc>=256.0f)hueAcc-=256.0f; gHueAuto=(uint8_t)hueAcc;
  }
  prevHueSpeed=gHueSpeed;

  // ── Broadcast our absolute state every 200ms ──
  // The controller uses it to drive its meters; the modules mirror it wholesale.
  // Because it is absolute (never a step), a module that reboots or misses
  // packets re-locks onto the grid within one sync and can never drift.
  // Per-mode colour table for the web UI's colour pickers: 1/s, and right after a change.
  static uint32_t lastHueTab=0;
  if(gHueTabDue || (t-lastHueTab)>1000){
    gHueTabDue=false; lastHueTab=t;
    TogaHueTabMsg ht={};
    togaInitHeader(ht.h,TOGA_MSG_HUETAB,MY_TOGA_GROUP,0);
    ht.count=TOTAL_ANIMS; ht.txOn=gTxOn;
    memcpy(ht.txHue,gTxHue,8); memcpy(ht.txSat,gTxSat,8);
    memcpy(ht.hue,hueBaseByMode,min((int)sizeof(ht.hue),(int)MAX_MODES));
    esp_now_send(TOGA_BCAST,(const uint8_t*)&ht,sizeof(ht));
  }
  static uint32_t lastSync=0;
  static uint16_t syncSeq=0;
  if((t-lastSync)>200){
    lastSync=t;
    TogaSyncMsg sync={};
    togaInitHeader(sync.h,TOGA_MSG_SYNC,MY_TOGA_GROUP,++syncSeq);
    sync.brightness       = cfg.brightness;        // effective (modules mirror this)
    sync.globalBrightness = cfg.globalBri;         // master (controller scales its pad by this)
    sync.hueBase          = gHueBase;
    sync.hueAuto          = gHueAuto;                  // modules snap their rotation to this
    sync.hueSpeed         = gHueSpeed;
    sync.strobeHue        = gStrobeHue;
    sync.strobeGapMs      = cfg.strobeGapMs;
    sync.strobeOnMs       = cfg.strobeOnMs;        // mode/castle-strobe on/off — modules mirror these (v6)
    sync.strobeOffMs      = cfg.strobeOffMs;       // so they flash the SAME rhythm as the wall, any strip length
    sync.coverage         = gCoverage;             // live lit-LED fraction — FOLLOW modules match their ratio (v9)
    sync.beatMs           = (uint16_t)lroundf(bPeriod);     // live tempo, not the saved one
    // The phase we are RENDERING, latency compensation already folded in — so a
    // module lands on the same brightness as the grid without redoing the maths.
    sync.beatPhase        = (uint8_t)(beatPhase(t)*255.0f);
    CRGB tc=CRGB(CHSV(gFrameHue,255,220));   // dominant colour actually on the wall this frame (rotation already folded in)
    sync.tintR=tc.r; sync.tintG=tc.g; sync.tintB=tc.b;
    CRGB sc=(gStrobeHue==0)?CRGB(200,200,200):CRGB(CHSV(gStrobeHue,255,220));
    sync.sColR=sc.r; sync.sColG=sc.g; sync.sColB=sc.b;
    sync.animIndex        = (int16_t)cfg.animIndex;
    sync.animSpeed        = animSpeed;               // swept value while sweeping → modules sweep in lockstep
    sync.modeFactor       = cfg.modeFactor[cfg.animIndex];   // controller blinks +/- when this mode's factor is maxed
    sync.strobeSpeed      = cfg.strobeSpeed;                  // controller's strobe-speed meter / at-limit
    sync.autoDwellS       = cfg.autoDwellS;                   // web UI echoes these back for display
    sync.sweepPeriodS     = cfg.sweepPeriodS;
    esp_now_send(TOGA_BCAST,(const uint8_t*)&sync,sizeof(sync));
  }

  // ── Strobe rendering (overrides blackout while held) ─
  if(strobeActive){
    // Same on/off rhythm as the mode- and castle-strobe (one global strobe speed),
    // on an absolute grid so a slow FastLED.show() cannot stretch it — see
    // togaSquareOn() in <toga_proto.h>. Redraw only on a state change: `want` is
    // true for the whole flash window, not just its edge.
    bool want = togaSquareOn(t,strobeStart,cfg.strobeOnMs,cfg.strobeOffMs);
    if(want!=strobeOn){
      strobeOn=want;
      if(strobeOn){
        CRGB sc = gStrobeHue==0 ? CRGB(CRGB::White) : CRGB(CHSV(gStrobeHue,255,255));  // Strobe colour (0 = white)
        FastLED.setBrightness((uint8_t)min((int)cfg.strobeBrightness,STROBE_MAX_BRI));  // 70% ceiling
        FastLED.clear();
        strobeFlash(strobeLook,strobeCount++,sc);
        FastLED.show();
      } else {
        FastLED.clear();
        for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
        FastLED.show();
      }
    }
    return;
  }

  // ── Second strobe (button 13): flash static CASTLE / 2026, no scroll (Task 5) ──
  // Absolute-time gate (togaSquareOn), NOT a chained `flip = now + on`: a slow
  // FastLED.show() cannot stretch the period, so the wall and every module — of
  // any strip length — flash this in lockstep. See togaSquareOn() in <toga_proto.h>.
  static bool     csHeldPrev=false; static uint32_t csStart=0; static bool csOnPrev=false;
  bool castleHeld = senderAlive && espCastleStrobe;
  if(castleHeld && !csHeldPrev){ csStart=t; csOnPrev=false; }  // phase origin + force first paint
  csHeldPrev = castleHeld;
  if(castleHeld){
    // Redraw ONLY on an on/off edge (like button-15). Running at full loop rate
    // now (cap bypassed above), a redraw-every-loop would spam FastLED.show();
    // the edge gate keeps the rhythm crisp and the load flat on every mode.
    bool csOn = togaSquareOn(t, csStart, cfg.strobeOnMs, cfg.strobeOffMs);
    if(csOn != csOnPrev){
      csOnPrev = csOn;
      FastLED.setBrightness(cfg.strobeBrightness);
      clearFrame();
      if(csOn){
        drawText1Centered("CASTLE",2,CRGB(CHSV(160,235,235)));   // upper line
        drawText1Centered("2026",  7,CRGB(CHSV( 35,235,235)));   // lower line
      }
      FastLED.show();
    }
    return;
  }

  // ── Drahthaus text strobe (v11): key-13 layer in the Drahthaus profile, one of six
  //    texts/pictograms (TOGA_TEXT_*). Same absolute-time gate and edge-only redraw
  //    as the castle strobe; `txCount` numbers the flashes so a look can sequence
  //    (the DRAHTHAUS scroll, the elephant's mirror step).
  static bool txHeldPrev=false; static uint32_t txStart=0; static bool txOnPrev=false;
  static uint16_t txCount=0; static uint8_t txPrevId=0;
  uint8_t txId = senderAlive ? espTextStrobe : 0;
  bool textHeld = txId!=0;
  if(textHeld && (!txHeldPrev || txId!=txPrevId)){ txStart=t; txOnPrev=false; txCount=0; }  // new press / new text → phase origin
  txHeldPrev=textHeld; txPrevId=txId;
  if(textHeld){
    bool txOn = togaSquareOn(t, txStart, cfg.strobeOnMs, cfg.strobeOffMs);
    if(txOn != txOnPrev){
      txOnPrev = txOn;
      FastLED.setBrightness(cfg.strobeBrightness);
      clearFrame();
      if(txOn){
        drawTextStrobe(txId, txCount++, textStrobeColour(txId));   // per-text colour from the app, else the strobe colour
      }
      FastLED.show();
    }
    return;
  }

  // ── Mode strobe: flash the CURRENT animation on/off, a bit brighter (button 14) ──
  // Absolute-time gate like the castle-strobe above — same length-independence.
  static bool     msHeldPrev=false; static uint32_t msStart=0; static bool msOnPrev=false;
  bool modeStrobeHeld = senderAlive && espModeStrobe;
  if(modeStrobeHeld && !msHeldPrev){ msStart=t; msOnPrev=false; }  // phase origin + force first ON paint
  msHeldPrev = modeStrobeHeld;
  if(modeStrobeHeld){
    bool msOn = togaSquareOn(t, msStart, cfg.strobeOnMs, cfg.strobeOffMs);
    if(!msOn){                                   // OFF window → dark, blank once on the edge, then hold
      if(msOnPrev){
        FastLED.clear();
        for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
        FastLED.show();
        msOnPrev=false;
      }
      return;
    }
    // ON window: render the LIVE animation exactly ONCE per flash (at the edge),
    // then hold that frame. A heavy mode re-rendered every loop stretched its own
    // flash and bogged the controller, so the SAME strobe ran slower on heavier
    // modes. virtualT keeps advancing between flashes, so the held image is still
    // fresh; togaSquareOn keeps the rhythm absolute and uniform across all modes.
    if(msOnPrev) return;    // already painted this ON window → hold it
    msOnPrev=true;
    // fall through once to the animation switch below (brighter; see gBaseBri)
  }

  // ── Blackout: the wall goes dark, but the mode keeps evolving underneath
  //    (virtualT already advanced above), so releasing it snaps straight back to
  //    the running animation. Strobe and castle-strobe already returned above, so
  //    they punch through the dark; skipped while mode-strobe is held so it stays
  //    visible too. Gated on senderAlive: a controller that dies or is switched
  //    off releases the blackout and the wall comes back on its own.
  if(senderAlive && espBlackout && !modeStrobeHeld){
    FastLED.clear();
    for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
    FastLED.show();
    return;
  }

  // ── Spot tunnel (phys 5, TOGA_F_SPOT). TAP vs HOLD:
  //    · a short TAP shoots the spot ONLY on the modules — the wall stays on its
  //      current mode (grid untouched).
  //    · HOLDING (≥ SPOT_HOLD_MS) additionally drives the wall through a tunnel of
  //      square frames growing centre → rim, in time with the module dots.
  //    So the grid only joins in once the key is genuinely held. Sits below
  //    blackout; the underlying mode keeps evolving (virtualT already advanced),
  //    so releasing snaps back to it. Real-time clock inside → same drive at any Speed.
  #define SPOT_HOLD_MS 350
  static uint32_t spotDownAt=0; static bool spotWasDown=false;
  if(espSpot && !spotWasDown) spotDownAt=t;            // rising edge → start the hold timer
  spotWasDown = espSpot;
  if(senderAlive && espSpot && (t-spotDownAt)>=SPOT_HOLD_MS){
    gBaseBri = cfg.brightness;      // showGrid() scales this by the AGC gain
    anim_spotTunnel(t);
    return;
  }

  // ── Normal animation ──────────────────────────────
  // Set the base brightness; showGrid() multiplies it by the AGC gain (Task 2b).
  gBaseBri = modeStrobeHeld ? (uint8_t)min((int)cfg.brightness*7/4,255) : cfg.brightness;
  FastLED.setBrightness(gBaseBri);   // fallback for anims that FastLED.show() without showGrid()
  gOverlayPainted=false;             // showGrid() sets it if it paints the mode number
  gLegacy = cfg.animIndex < 68;      // pre-showpiece modes get the shiny/flaky retrofit in showGrid()
  switch(cfg.animIndex){
    case 0:  anim_setupGrid(at);           break;
    case 1:  anim_heartbeat(at);           break;
    case 2:  anim_binaryRain(at,dt);       break;
    case 3:  anim_vortex(at);              break;
    case 4:  anim_earthquake(at);          break;   // scaled time → Speed knob now works
    case 5:  anim_supernova(at);           break;
    case 6:  anim_blackHole(at);           break;
    case 7:  anim_wormhole(at);            break;
    case 8:  anim_mitosis(at);             break;
    case 9:  anim_collider(at);            break;
    case 10: anim_shockwaveChain(at);      break;
    case 11: anim_pulsar(at);              break;
    case 12: anim_liquidMetal(at);         break;
    case 13: anim_coriolisStorm(at);       break;
    case 14: anim_aurora(at);              break;
    case 15: anim_lavaLamp(at,dt);         break;
    case 16: anim_nerveSystem(at,dt);      break;
    case 17: anim_gravityLens(at);         break;
    case 18: anim_cymatics(at);            break;
    case 19: anim_starfieldWarp(at,dt);    break;
    case 20: anim_concentricSquares(at);   break;
    case 21: anim_squareRain(at);          break;
    case 22: anim_depthMap(at);            break;
    case 23: anim_cosmicDust(at);          break;
    case 24: anim_heartglow(at);           break;
    case 25: anim_evolution(at);           break;
    case 26: anim_morph(at);               break;
    case 27: anim_strangeAttractor(at,dt); break;
    case 28: anim_harmonicResonance(at,dt);break;
    case 29: anim_snake(at,dt);            break;
    case 30: anim_tetris(at,dt);           break;
    case 31: anim_bouncingLogo(at,dt);     break;
    case 32: anim_starTunnel3D(at,dt);     break;
    case 33: anim_rippleRain(at,dt);       break;
    case 34: anim_spiralGalaxy(at);        break;
    case 35: anim_boids(at,dt);            break;
    case 36: anim_fireworks(at,dt);        break;
    case 37: anim_textTOGA(at);            break;
    case 38: anim_stickman(at,dt);         break;
    case 39: anim_paddleBall(at,dt);       break;
    case 40: anim_smiley(at);              break;
    case 41: anim_eye(at,dt);              break;
    case 42: anim_starfield2(at);          break;
    case 43: anim_warp(at);                break;
    case 44: anim_shootingStars(at);       break;
    case 45: anim_blackHole2(at);          break;
    case 46: anim_pulsar2(at);             break;
    case 47: anim_horsehead(at);           break;
    case 48: anim_auroraCurtains(at,dt);   break;   // was earthWindow (deleted) → new energy mode
    case 49: anim_radar(at);               break;
    case 50: anim_sineWaves(at);           break;
    case 51: anim_rocketFire(at);          break;
    case 52: anim_warningLight(at);        break;
    case 53: anim_aliens(at);              break;
    case 54: anim_rain2(at);               break;
    case 55: anim_levitate(at);            break;
    case 56: anim_confetti(at);            break;
    case 57: anim_spiralEject(at);         break;
    case 58: anim_clouds3d(at);            break;
    case 59: anim_pulseHeart(at);          break;
    case 60: anim_rainUp(at);              break;
    case 61: anim_starsFlying(at);         break;
    case 62: anim_firework2(at);           break;
    case 63: anim_vuColumns(at);           break;
    case 64: anim_vuGrid(at);              break;
    case 65: anim_superHexagon(at);        break;
    case 66: anim_squareImplode(at);       break;
    case 67: anim_plasmaLightning(at,dt);  break;
    case 68: anim_neonShockwaves(at,dt);   break;   // new
    case 69: anim_emberVortex(at,dt);      break;   // new
    case 70: anim_prismShards(at,dt);      break;   // new
    case 71: anim_nebulaBreath(at,dt);     break;   // new
    case 72: anim_jellyfishBloom(at,dt);      break;
    case 73: anim_kaleidoWheel(at,dt);        break;
    case 74: anim_lightningTree(at,dt);       break;
    case 75: anim_dnaHelix(at,dt);            break;
    case 76: anim_fireflies(at,dt);           break;
    case 77: anim_blackHoleAccretion(at,dt);  break;
    case 78: anim_oceanCaustics(at,dt);       break;
    case 79: anim_phoenixRise(at,dt);         break;
    case 80: anim_clockworkGears(at,dt);      break;
    case 81: anim_magneticDipole(at,dt);      break;
    case 82: anim_quantumOrbitals(at,dt);     break;
    case 83: anim_serpentNest(at,dt);         break;
    case 84: anim_crystalGrowth(at,dt);       break;
    case 85: anim_solarFlare(at,dt);          break;
    case 86: anim_mandalaBloom(at,dt);        break;
    case 87: anim_inkBloom(at,dt);            break;
    case 88: anim_electricEel(at,dt);         break;
    case 89: anim_galaxyCollision(at,dt);     break;
    case 90: anim_gyroscope(at,dt);           break;
    case 91: anim_doublePendulum(at,dt);      break;
    case 92: anim_pulsarBeacon(at,dt);        break;
    case 93: anim_wormholeThroat(at,dt);      break;
    case 94: anim_spirographArms(at,dt);      break;
    case 95: anim_honeycombPulse(at,dt);      break;
    case 96: anim_turbineBlades(at,dt);       break;
    case 97: anim_mantaGlide(at,dt);          break;
    case 98: anim_plasmaGlobe(at,dt);         break;
    case 99: anim_supernovaShell(at,dt);      break;
    case 100: anim_cometSwarm(at,dt);         break;
    case 101: anim_roseWindow(at,dt);         break;
    case 102: anim_tesseract(at,dt);          break;
    case 103: anim_torusKnot(at,dt);          break;
    case 104: anim_morphSwarm(at,dt);         break;
    case 105: anim_gravitationalLens(at,dt);  break;
    case 106: anim_celestialDragon(at,dt);    break;
    // ── Drahthaus concert pack (anim_konzert.h) — keep MODE_TABLE + togaEffectName in step ──
    case 107: anim_konzertMarker(at,dt);      break;
    case 108: anim_dh100Elefanten(at,dt);     break;
    case 109: anim_mitosis(at);               break;   // Where Did You Sleep Last Night = Togalicht mode 8
    case 110: anim_dhEcholot(at,dt);          break;
    case 111: anim_dhCryMeARiver(at,dt);      break;
    case 112: anim_dhNichtErschrecken(at,dt); break;
    case 113: anim_shockwaveChain(at);        break;   // Alle gegen alle = Togalicht mode 10
    case 114: anim_dhSabbia1(at,dt);          break;
    case 115: anim_vortex(at);                break;   // Sabbia II = Togalicht mode 3
    case 116: anim_dhMuecke(at,dt);           break;
    case 117: anim_dhRoentgenauge(at,dt);     break;
    case 118: anim_quadMirror(at,dt);         break;   // Dienstag (noch immer dieser Rave) = quadMirror
    case 119: anim_dhSong12(at,dt);           break;
    case 120: anim_quadMirror(at,dt);         break;
    case 121: anim_squareSpiral(at,dt);       break;
    case 122: anim_gridPulse(at,dt);          break;
    // ── Drahthaus test pack: big lettering / word art ──
    case 123: anim_dhRiesenschrift(at,dt);    break;
    case 124: anim_dhElephantRoom(at,dt);     break;
    case 125: anim_dhMonogramm(at,dt);        break;
    case 126: anim_dhElefantenKopf(at,dt);    break;
    case 127: anim_schwarz(at);               break;   // TOGA_MODE_SCHWARZ — dark standby, one dim dot
  }

  // ── Mode-change readout ──
  // showGrid() paints the number as part of its single show, which is what keeps
  // it steady. This is only the fallback for anim_setupGrid: the calibration view
  // must not be recoloured, so it shows by hand and never calls showGrid(). It
  // pays the old double-show flicker — on the one mode where that is harmless.
  if(t < overlayUntil && !gOverlayPainted){
    drawNumberTopRight(t);
    FastLED.show();
  }
}
