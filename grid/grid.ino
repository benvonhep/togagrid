#include <FastLED.h>
#include <math.h>
// grid_config.h pulls in <toga_proto.h>, which defines the ESP-NOW wire
// structs. It must stay up here: arduino-cli injects generated prototypes
// near the top of the sketch and they have to see these types first.
#include "grid_config.h"     // all tunable constants: hardware, pins, brightness, AGC, geometry
#include "grid_beat.h"       // tap-tempo tracker (button 11 -> beatFactor())
#include <toga_ota.h>        // always-on firmware update over the AP; see loop()

// This is the board on a wall. Keep a freshly OTA-ed image on probation until it
// has proven itself, instead of letting initArduino() bless it before setup()
// runs — that is the difference between a bad flash self-healing and a ladder.
// extern "C" is mandatory; see the long note in toga_ota.h.
extern "C" bool verifyRollbackLater() { return true; }

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
uint8_t hueSpeedByMode[MAX_MODES];   // per-mode hue-speed 0..HUE_SPEED_MAX
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

// Central show: rotates every lit pixel's hue by (base+auto), measures total light for
// the AGC, sets the normalized brightness, then pushes to LEDs.
void showGrid(){
  uint8_t off=(uint8_t)(gHueBase+gHueAuto);
  uint32_t lum=0;
  for(int e=0;e<NUM_ELEC;e++) for(int i=0;i<LEDS_PER_STRIP;i++){
    CRGB* p=&ledsX[e][i]; if(off && (p->r|p->g|p->b)){ CHSV h=rgb2hsv_approximate(*p); h.hue+=off; *p=h; }
    lum += p->r+p->g+p->b;
    p=&ledsY[e][i];       if(off && (p->r|p->g|p->b)){ CHSV h=rgb2hsv_approximate(*p); h.hue+=off; *p=h; }
    lum += p->r+p->g+p->b;
  }
  // AGC: scale a global gain so every mode lands near a common perceived level.
  const float N=(float)NUM_ELEC*LEDS_PER_STRIP*2.0f;
  float avg=(float)lum/N;                                        // avg sum-of-channels/pixel
  float gainTarget=constrain(AGC_TARGET/fmaxf(avg,1.0f),AGC_MIN,AGC_MAX);
  if(gAgcSnap){ gAgcGain=gainTarget; gAgcSnap=false; }           // instant on mode change
  else         gAgcGain+=(gainTarget-gAgcGain)*AGC_K;            // else slow-smooth
  FastLED.setBrightness((uint8_t)constrain((int)lroundf(gBaseBri*gAgcGain*gBeatGlow),1,MAX_BRIGHTNESS));
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
volatile bool     espBlackout     = false;   // latched blackout (button 10); rides the flags like the strobes
volatile uint8_t  espAutoMode     = TOGA_AUTO_OFF;   // 0 = off, 1..4 = mood (button 0)
volatile uint32_t espLastHeartbeat = 0;
static uint8_t    senderMAC[6]     = {};
static bool       senderKnown      = false;
static uint16_t   lastCmdSeq       = 0;
static bool       lastCmdSeqValid  = false;
static uint8_t    gMyMac[6]        = {};             // set in setup(), before the cb goes live

void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  TogaCmdMsg msg;
  if(!togaParseCmd(data, len, MY_TOGA_GROUP, &msg)) return;   // magic + version + group
  espStrobe        = (msg.flags & TOGA_F_STROBE)        != 0;
  espModeStrobe    = (msg.flags & TOGA_F_MODE_STROBE)   != 0;
  espCastleStrobe  = (msg.flags & TOGA_F_CASTLE_STROBE) != 0;
  espBlackout      = (msg.flags & TOGA_F_BLACKOUT)      != 0;
  espAutoMode      = (msg.autoMode<=TOGA_AUTO_COUNT) ? msg.autoMode : TOGA_AUTO_OFF;
  uint32_t rx = millis();
  espLastHeartbeat = rx;
  if(msg.cmd != TOGA_CMD_HEARTBEAT){
    // The controller sends each one-shot command 3x (broadcast has no MAC-layer
    // retry). All repeats carry one seq, so apply the first and drop the rest.
    bool dup = lastCmdSeqValid && (msg.h.seq == lastCmdSeq);
    lastCmdSeq = msg.h.seq; lastCmdSeqValid = true;
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
  uint8_t globalBri;              // global master, raw 1..MAX_BRIGHTNESS ("0..100%")
  uint8_t modeFactor[MAX_MODES];  // per-mode factor 0..255 (0..1); scales the global master
  uint8_t strobeBrightness;
  uint8_t strobeOnMs;
  uint8_t strobeOffMs;
  uint8_t strobeSquares;
  uint8_t strobeGapMs;    // button-15 strobe: gap between flashes (hold 15 + +/-)
  float   animSpeed;      // ACTIVE mode's speed 0.1..4.0 (mirrors speedByMode[animIndex])
  float   speedByMode[MAX_MODES];  // per-mode remembered speed
  uint16_t beatMs;        // tapped beat period for the glow, 0 = off
};
Settings cfg;

// Fold the global master and the active mode's factor into the one effective
// value everything downstream reads (AGC base, strobe scaling, sync to modules).
// Call after any change to globalBri, modeFactor[animIndex], or animIndex.
void recomputeBrightness() {
  cfg.brightness = (uint8_t)constrain(
      lround((long)cfg.globalBri * cfg.modeFactor[cfg.animIndex] / 255.0),
      1, MAX_BRIGHTNESS);
  FastLED.setBrightness(cfg.brightness);
}

// Restore the ACTIVE values for the current mode: brightness (global*factor),
// speed and hue-speed each come from that mode's own remembered slot. Call
// whenever animIndex changes.
void applyModeSettings() {
  cfg.animSpeed = cfg.speedByMode[cfg.animIndex];
  gHueSpeed     = hueSpeedByMode[cfg.animIndex];
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
  prefs.putBytes("spdm", cfg.speedByMode, sizeof(cfg.speedByMode));   // per-mode speed
  prefs.putBytes("hspm", hueSpeedByMode, sizeof(hueSpeedByMode));     // per-mode hue-speed
  prefs.putUShort("beat", cfg.beatMs);        // beat-glow tempo
  prefs.putUChar("hue",  gHueBase);
  prefs.putUChar("shue", gStrobeHue);
  prefs.end();
}

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
  bool spdOk           = prefs.getBytes("spdm", cfg.speedByMode, sizeof(cfg.speedByMode))
                           == sizeof(cfg.speedByMode);   // per-mode speed blob
  bool hspOk           = prefs.getBytes("hspm", hueSpeedByMode, sizeof(hueSpeedByMode))
                           == sizeof(hueSpeedByMode);     // per-mode hue-speed blob
  cfg.beatMs           = prefs.getUShort("beat", 0);
  gHueBase             = prefs.getUChar("hue",  0);
  gStrobeHue           = prefs.getUChar("shue", 0);
  prefs.end();
  cfg.animIndex        = constrain(cfg.animIndex, 0, TOTAL_ANIMS-1);
  cfg.globalBri        = constrain(cfg.globalBri, 1, MAX_BRIGHTNESS);
  // Per-mode blobs: seed defaults if missing, then clamp every slot (NVS is not trusted).
  if(!facOk) for(int i=0;i<MAX_MODES;i++) cfg.modeFactor[i]=BRICOEF_MAX;
  for(int i=0;i<MAX_MODES;i++) cfg.modeFactor[i]=constrain(cfg.modeFactor[i],BRICOEF_MIN,BRICOEF_MAX);
  if(!spdOk) for(int i=0;i<MAX_MODES;i++) cfg.speedByMode[i]=1.0f;
  for(int i=0;i<MAX_MODES;i++) cfg.speedByMode[i]=constrain(cfg.speedByMode[i],0.1f,4.0f);
  if(!hspOk) for(int i=0;i<MAX_MODES;i++) hueSpeedByMode[i]=0;
  for(int i=0;i<MAX_MODES;i++) hueSpeedByMode[i]=constrain(hueSpeedByMode[i],0,HUE_SPEED_MAX);
  applyModeSettings();                                            // active brightness/speed/hue-speed for this mode
  cfg.strobeBrightness = constrain(cfg.strobeBrightness, 1, 255);
  cfg.strobeOnMs       = constrain(cfg.strobeOnMs, 5, 200);
  cfg.strobeOffMs      = constrain(cfg.strobeOffMs, 5, 200);
  cfg.strobeSquares    = constrain(cfg.strobeSquares, 1, 122);
  cfg.strobeGapMs      = constrain(cfg.strobeGapMs, STROBE_GAP_MIN, STROBE_GAP_MAX);
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

void resetAnimState(uint32_t t) {
  rainInit=false;shockInit=false;novaInit=false;blobInit=false;
  nerveInit=false;starInit=false;pendInit=false;sqRainInit=false;golInit=false;
  evoInit=false;morphInit=false;wvInit=false;lorInit=false;hrInit=false;nfInit=false;plInit=false;
  tetInit=false;snakeInit=false;logoInit=false;stunInit=false;rripInit=false;boidInit=false;fwInit=false;rdInit=false;
  stickInit=false;padInit=false;eyeInit=false;
  strikeTime=t;collideStrike=t;
  epiX=(random(80)-40);epiY=(random(80)-40);
  gAgcSnap=true;                 // snap AGC to the new mode's level (Task 2b)
  clearAll();delay(100);
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  loadSettings();
  Serial.printf("Loaded: anim=%d bri=%d strobeBri=%d on=%dms off=%dms\n",
    cfg.animIndex+1,cfg.brightness,cfg.strobeBrightness,cfg.strobeOnMs,cfg.strobeOffMs);

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

  togaWifiBegin("toga-grid");                        // STA on togalights (ch1) — bus AND network
  WiFi.macAddress(gMyMac);                           // before the cb goes live: it filters on this
  Serial.printf("Receiver MAC: %s  ch=%d\n", WiFi.macAddress().c_str(), TOGA_CHANNEL);
  if(esp_now_init() != ESP_OK) { Serial.println("ESP-NOW init failed!"); while(1) delay(1000); }
  esp_now_register_recv_cb(onReceive);
  if(!togaAddPeer(TOGA_BCAST)) { Serial.println("Broadcast peer failed!"); while(1) delay(1000); }
  togaOtaSetup("toga-grid");                         // flashable from now on, mid-show, no trigger
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
static const uint8_t AUTO_CALM[]  = {18,19,48,47,31,82,73,79,22,34,14,23};
static const uint8_t AUTO_SPACE[] = {24,60,44,46,63,9,7,65,66,67,68,69,70,61,62,59,85,25};
static const uint8_t AUTO_PARTY[] = {2,27,50,86,80,88,89,12,13,29,87,81,5,45,84,78};
static const uint8_t AUTO_SHOW[]  = {40,41,55,56,57,58,76,77,42,43,52,53,54,72,71,75};
struct AutoMood { const uint8_t* list; uint8_t n; uint16_t everyMs; };
static const AutoMood AUTO_MOODS[TOGA_AUTO_COUNT] = {   // everyMs = dwell at Speed 1.0
  {AUTO_CALM,  sizeof(AUTO_CALM),  45000},   // 1 Ruhe   — langsam, flächig
  {AUTO_SPACE, sizeof(AUTO_SPACE), 30000},   // 2 Space  — Sterne, Nebel, Planeten
  {AUTO_PARTY, sizeof(AUTO_PARTY), 12000},   // 3 Party  — schnell, hart geschnitten
  {AUTO_SHOW,  sizeof(AUTO_SHOW),  20000},   // 4 Show   — Spiele, Gesichter, Text
};

// How long the current mood holds a mode: the table value divided by the Speed
// knob (8), so the one knob that already means "faster" also cuts faster —
// Speed 4.0 → 4x quicker, Speed 0.1 → 10x longer. Read fresh every frame rather
// than latched at the last step, so turning the knob bites on the CURRENT mode
// instead of the next one: at 45s a latched value would feel broken.
uint32_t autoDwellMs(uint8_t mood){
  return (uint32_t)(AUTO_MOODS[mood-1].everyMs / constrain(cfg.animSpeed,0.1f,4.0f));
}

// Switch to entry `pos` of `mood` (1-based), exactly as a manual step would —
// minus the save. An out-of-range list entry would be a typo above, not input.
void autoShowMode(uint8_t mood, uint8_t pos, uint32_t t){
  const AutoMood& md=AUTO_MOODS[mood-1];
  cfg.animIndex=constrain((int)md.list[pos%md.n],0,TOTAL_ANIMS-1);
  applyModeSettings();                             // brightness/speed/hue-speed for this mode
  resetAnimState(t); triggerOverlay(t,cfg.animIndex);
}

// ══════════════════════════════════════════════════════════════
//  Strobe looks (button 15). Five ways to draw ONE flash; which one is used is
//  drawn fresh on every PRESS, so a held strobe keeps its look and the next
//  press brings a new one. The flash itself stays as short as the LED refresh
//  allows and only the gap is tunable — see the render block in loop().
// ══════════════════════════════════════════════════════════════
#define STROBE_LOOKS    5
#define SL_SQUARES      0   // random squares (the original look)
#define SL_FULL         1   // the whole wall
#define SL_STRIPS       2   // random whole strips
#define SL_RINGS        3   // square rings, middle → outside
#define SL_QUADRANTS    4   // one random quarter
#define STROBE_STRIP_N  6   // strips lit per flash in SL_STRIPS

// Random, but never the same look twice in a row: deal the five out in a
// shuffled bag and only reshuffle once the bag is empty. Plain random() would
// repeat a look ~20% of the time, and a repeat reads as "the button did
// nothing" — which is the one thing this must never look like.
uint8_t nextStrobeLook(){
  static uint8_t bag[STROBE_LOOKS], left=0, last=255;
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
    case SL_FULL:
      for(int e=0;e<NUM_ELEC;e++){
        for(int i=0;i<LEDS_PER_STRIP;i++){ ledsX[e][i]=sc; ledsY[e][i]=sc; }
        ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;
        ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;
      }
      break;
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
    resetAnimState(t); saveSettings(); triggerOverlay(t,cfg.animIndex);
  } else if(m.cmd==TOGA_CMD_PARAM){                 // parameter step
    int d=m.arg;
    switch(m.target){
      case TOGA_P_BRI: cfg.globalBri=(uint8_t)constrain((int)cfg.globalBri+d*BRI_STEP,1,MAX_BRIGHTNESS); recomputeBrightness(); break;  // global master
      case TOGA_P_BRICOEF: cfg.modeFactor[cfg.animIndex]=(uint8_t)constrain((int)cfg.modeFactor[cfg.animIndex]+d*BRICOEF_STEP,BRICOEF_MIN,BRICOEF_MAX); recomputeBrightness(); break;  // this mode's factor
      case TOGA_P_SPEED: cfg.animSpeed=constrain(cfg.animSpeed+d*0.1f,0.1f,4.0f); cfg.speedByMode[cfg.animIndex]=cfg.animSpeed; break;  // per-mode speed
      case TOGA_P_COLOR: gHueBase  =(uint8_t)(gHueBase  +d*6); break;   // Color (wraps)
      case TOGA_P_STROBEHUE: gStrobeHue=(uint8_t)(gStrobeHue+d*6); break;   // Strobe colour (wraps)
      case TOGA_P_HUESPEED: gHueSpeed =(uint8_t)constrain((int)gHueSpeed+d,0,HUE_SPEED_MAX); hueSpeedByMode[cfg.animIndex]=gHueSpeed; break; // per-mode hue-speed
      case TOGA_P_STROBEGAP: cfg.strobeGapMs=(uint8_t)constrain((int)cfg.strobeGapMs+d*STROBE_GAP_STEP,
                                                 STROBE_GAP_MIN,STROBE_GAP_MAX); break; // gap between flashes
    }
    saveSettings();
  } else if(m.cmd==TOGA_CMD_BEAT_TAP){              // one tap of button 11 → glow rhythm (NOT speed)
    beatOnTap(t);                                   // t = arrival time: the phase anchor
    beatDirty=true; beatDirtyAt=millis();
  } else if(m.cmd==TOGA_CMD_BEAT){                  // period set directly, without tapping
    beatSetPeriod((float)m.arg,t);
    beatDirty=true; beatDirtyAt=millis();
  }
}

void loop() {
  // Above every early return below (the frame cap, and the strobe on every
  // single iteration while held) — a grid you cannot flash mid-strobe is a grid
  // you cannot flash exactly when you want to.
  togaOtaHandle();
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

  // Sits here, not at the end of loop(): the strobe path below returns early on
  // every iteration while held, and a grid strobed through its first minute
  // would never commit and would roll back on the next reboot.
  togaOtaMarkValidWhenHealthy(t, senderAlive);

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
    if(!strobeActive){
      if(mood!=moodPrev){                                       // switched mood, or turned off
        moodPrev=mood; moodPos=0;
        // Nothing is drawn when the mood turns OFF: that only ever happens on a
        // manual +/- step, which is already flashing the mode number the press
        // asked for, or on the sender going away, where the wall announcing "A0"
        // would be reporting the controller's silence as a setting.
        if(mood){
          autoShowMode(mood,moodPos,t); moodStart=t;
          // AFTER autoShowMode: it flashes the MODE number, and the mood is the
          // answer to the press that just happened. The mood's first mode is
          // arriving anyway, and its number will flash on the next auto step.
          triggerOverlayMood(t,mood);
        }
      } else if(mood && (t-moodStart)>=autoDwellMs(mood)){      // dwell follows the Speed knob
        moodPos=(uint8_t)((moodPos+1)%AUTO_MOODS[mood-1].n);
        autoShowMode(mood,moodPos,t); moodStart=t;
      }
    }
  }

  // 50 FPS cap for animations — but the strobe must run at full loop rate, else its
  // timing is quantised to 20ms and the flash can never be short (that was the bug).
  if(!strobeActive && t-lastFrame<20) return;
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
  if(beatDirty && (t-beatDirtyAt)>=BEAT_SAVE_QUIET){
    beatDirty=false;
    uint16_t per=(uint16_t)lroundf(bPeriod);
    if(per!=cfg.beatMs){ cfg.beatMs=per; saveSettings(); }
  }
  static float virtualT = 0.0f;
  virtualT += frameDt * cfg.animSpeed;
  uint32_t at = (uint32_t)virtualT;
  float    dt = frameDt * cfg.animSpeed;

  // ── Auto-rotate the global hue by Hue-speed ───────
  static float hueAcc=0; hueAcc+=togaHueRate(gHueSpeed)*frameDt; if(hueAcc>=256.0f)hueAcc-=256.0f; gHueAuto=(uint8_t)hueAcc;

  // ── Broadcast our absolute state every 200ms ──
  // The controller uses it to drive its meters; the modules mirror it wholesale.
  // Because it is absolute (never a step), a module that reboots or misses
  // packets re-locks onto the grid within one sync and can never drift.
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
    sync.beatMs           = (uint16_t)lroundf(bPeriod);     // live tempo, not the saved one
    // The phase we are RENDERING, latency compensation already folded in — so a
    // module lands on the same brightness as the grid without redoing the maths.
    sync.beatPhase        = (uint8_t)(beatPhase(t)*255.0f);
    CRGB tc=CRGB(CHSV((uint8_t)(gHueBase+gHueAuto),255,220));   // exact tint colour (rainbow map + auto)
    sync.tintR=tc.r; sync.tintG=tc.g; sync.tintB=tc.b;
    CRGB sc=(gStrobeHue==0)?CRGB(200,200,200):CRGB(CHSV(gStrobeHue,255,220));
    sync.sColR=sc.r; sync.sColG=sc.g; sync.sColB=sc.b;
    sync.animIndex        = (int16_t)cfg.animIndex;
    sync.animSpeed        = cfg.animSpeed;
    esp_now_send(TOGA_BCAST,(const uint8_t*)&sync,sizeof(sync));
  }

  // ── Strobe rendering (overrides blackout while held) ─
  if(strobeActive){
    // Short fixed flash, user-tunable gap (hold 15 + press +/-). The period is
    // exactly STROBE_FLASH_MS+gap on an absolute grid, so a slow FastLED.show()
    // cannot stretch it — see togaStrobeOn() in <toga_proto.h>. Redraw only on a
    // state change: `want` is true for the whole flash window, not just its edge.
    bool want = togaStrobeOn(t,strobeStart,cfg.strobeGapMs);
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
  bool castleHeld = senderAlive && espCastleStrobe;
  if(castleHeld){
    static bool csOn=true; static uint32_t csFlip=0;
    if((int32_t)(t-csFlip)>=0){ csOn=!csOn; csFlip=t+(uint32_t)(csOn?cfg.strobeOnMs:cfg.strobeOffMs); }
    FastLED.setBrightness(cfg.strobeBrightness);
    clearFrame();
    if(csOn){
      drawText1Centered("CASTLE",2,CRGB(CHSV(160,235,235)));   // upper line
      drawText1Centered("2026",  7,CRGB(CHSV( 35,235,235)));   // lower line
    }
    FastLED.show();
    return;
  }

  // ── Mode strobe: flash the CURRENT animation on/off, a bit brighter (button 14) ──
  bool modeStrobeHeld = senderAlive && espModeStrobe;
  if(modeStrobeHeld){
    static bool msOn=true; static uint32_t msFlip=0;
    if((int32_t)(t-msFlip)>=0){ msOn=!msOn; msFlip=t+(uint32_t)(msOn?cfg.strobeOnMs:cfg.strobeOffMs); }
    if(!msOn){                                   // off phase → dark
      FastLED.clear();
      for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
      FastLED.show();
      return;
    }
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

  // ── Normal animation ──────────────────────────────
  // Set the base brightness; showGrid() multiplies it by the AGC gain (Task 2b).
  gBaseBri = modeStrobeHeld ? (uint8_t)min((int)cfg.brightness*7/4,255) : cfg.brightness;
  FastLED.setBrightness(gBaseBri);   // fallback for anims that FastLED.show() without showGrid()
  gOverlayPainted=false;             // showGrid() sets it if it paints the mode number
  switch(cfg.animIndex){
    case 0:  anim_setupGrid(at);           break;
    case 1:  anim_heartbeat(at);           break;
    case 2:  anim_binaryRain(at,dt);       break;
    case 3:  anim_tectonic(at);            break;
    case 4:  anim_vortex(at);              break;
    case 5:  anim_earthquake(t);           break;
    case 6:  anim_dnaHelix(at);            break;
    case 7:  anim_supernova(at);           break;
    case 8:  anim_blackHole(at);           break;
    case 9:  anim_wormhole(at);            break;
    case 10: anim_mitosis(at);             break;
    case 11: anim_collider(at);            break;
    case 12: anim_shockwaveChain(at);      break;
    case 13: anim_pulsar(at);              break;
    case 14: anim_liquidMetal(at);         break;
    case 15: anim_iceCrown(at);            break;
    case 16: anim_coriolisStorm(at);       break;
    case 17: anim_crystalFracture(at);     break;
    case 18: anim_aurora(at);              break;
    case 19: anim_lavaLamp(at,dt);         break;
    case 20: anim_nerveSystem(at,dt);      break;
    case 21: anim_gravityLens(at);         break;
    case 22: anim_tidalWave(at);           break;
    case 23: anim_cymatics(at);            break;
    case 24: anim_starfieldWarp(at,dt);    break;
    case 25: anim_darkMatter(at);          break;
    case 26: anim_concentricSquares(at);   break;
    case 27: anim_squareRain(at);          break;
    case 28: anim_diamondPulse(at);        break;
    case 29: anim_checkerboardMelt(at);    break;
    case 30: anim_rotatingCube(at);        break;
    case 31: anim_sunsetHorizon(at);       break;
    case 32: anim_depthMap(at);            break;
    case 33: anim_cosmicDust(at);          break;
    case 34: anim_heartglow(at);           break;
    case 35: anim_evolution(at);           break;
    case 36: anim_morph(at);               break;
    case 37: anim_strangeAttractor(at,dt); break;
    case 38: anim_harmonicResonance(at,dt);break;
    case 39: anim_agentField(at,dt);       break;
    case 40: anim_tetris(at,dt);           break;
    case 41: anim_snake(at,dt);            break;
    case 42: anim_iconShow(at);            break;
    case 43: anim_bouncingLogo(at,dt);     break;
    case 44: anim_starTunnel3D(at,dt);     break;
    case 45: anim_rippleRain(at,dt);       break;
    case 46: anim_spiralGalaxy(at);        break;
    case 47: anim_plasmaField(at);         break;
    case 48: anim_breatheGradient(at);     break;
    case 49: anim_boids(at,dt);            break;
    case 50: anim_fireworks(at,dt);        break;
    case 51: anim_reactionDiff(at,dt);     break;
    case 52: anim_textTOGA(at);            break;
    case 53: anim_textCASTLE(at);          break;
    case 54: anim_text2026(at);            break;
    case 55: anim_stickman(at,dt);         break;
    case 56: anim_paddleBall(at,dt);       break;
    case 57: anim_smiley(at);              break;
    case 58: anim_eye(at,dt);              break;
    case 59: anim_starfield2(at);          break;
    case 60: anim_warp(at);                break;
    case 61: anim_asteroids(at);           break;
    case 62: anim_shootingStars(at);       break;
    case 63: anim_blackHole2(at);          break;
    case 64: anim_pulsar2(at);             break;
    case 65: anim_pulsarNebula(at);        break;
    case 66: anim_orionNebula(at);         break;
    case 67: anim_horsehead(at);           break;
    case 68: anim_twinStars(at);           break;
    case 69: anim_planets(at);             break;
    case 70: anim_earthWindow(at);         break;
    case 71: anim_shipPanel(at);           break;
    case 72: anim_radar(at);               break;
    case 73: anim_sineWaves(at);           break;
    case 74: anim_rocketFire(at);          break;
    case 75: anim_warningLight(at);        break;
    case 76: anim_aliens(at);              break;
    case 77: anim_spaceInvaders(at);       break;
    case 78: anim_rain2(at);               break;
    case 79: anim_levitate(at);            break;
    case 80: anim_confetti(at);            break;
    case 81: anim_spiralEject(at);         break;
    case 82: anim_clouds3d(at);            break;
    case 83: anim_pulseHeart(at);          break;
    case 84: anim_rainUp(at);              break;
    case 85: anim_starsFlying(at);         break;
    case 86: anim_firework2(at);           break;
    case 87: anim_hStripesFlow(at);        break;
    case 88: anim_vuColumns(at);           break;
    case 89: anim_vuGrid(at);              break;
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
