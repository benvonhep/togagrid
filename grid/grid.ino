#include <FastLED.h>
#include <math.h>
#include "grid_config.h"     // all tunable constants: hardware, pins, brightness, AGC, geometry

// ── ESP-NOW messages (defined early so function prototypes see them) ──
typedef struct {
  uint8_t cmd;      // 0=heartbeat, 1=mode step, 2=param step, 3=set speed
  uint8_t target;   // param id for cmd==2: 0=BRI 1=SPEED 2=COLOR 3=STROBEHUE 4=HUESPEED
  int16_t arg;      // mode/param: +1 / -1 ; set-speed: animSpeed*1000
  bool    strobe;   // momentary strobe held (white/colour squares)
  bool    modeStrobe; // momentary: strobe the current animation on/off
  bool    castleStrobe; // momentary: flash static CASTLE / 2026 (button 13)
} GridMsg;
typedef struct {
  uint8_t brightness, strobeBrightness, strobeOnMs, strobeOffMs, strobeSquares;
  uint8_t hueBase, hueSpeed, strobeHue;
  uint8_t tintR, tintG, tintB;      // exact current global-tint colour
  uint8_t sColR, sColG, sColB;      // exact current strobe colour
  int     animIndex;
  float   animSpeed;
} SyncMsg;

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
uint8_t gHueBase=0, gHueAuto=0, gHueSpeed=0, gStrobeHue=0;
// ── Auto brightness normalization state (Task 2b) ──
uint8_t gBaseBri=40;     // per-frame base brightness the AGC multiplies (set by loop)
float   gAgcGain=1.0f;   // smoothed AGC gain
bool    gAgcSnap=false;  // snap gain straight to target next frame (set on mode change)
// ── Beat-Glow: tapped tempo pulses the whole frame (does NOT touch animSpeed) ──
float   gBeatPeriod=0.0f;  // current beat period in ms (0 = glow off)
float   gBeatTarget=0.0f;  // newly tapped period; gBeatPeriod ramps toward it
float   gBeatPhase =0.0f;  // 0..1 position within the current beat
float   gBeatGlow  =1.0f;  // brightness multiplier from the beat pulse
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
volatile GridMsg  cmdQ[16];                    // command ring buffer
volatile uint8_t  cmdHead=0, cmdTail=0;
volatile bool     espStrobe       = false;
volatile bool     espModeStrobe   = false;
volatile bool     espCastleStrobe = false;
volatile uint32_t espLastHeartbeat = 0;
static uint8_t    senderMAC[6]     = {};
static bool       senderKnown      = false;
static bool       syncPeerAdded    = false;

void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  if(len != sizeof(GridMsg)) return;
  GridMsg msg;
  memcpy(&msg, data, sizeof(msg));
  espStrobe        = msg.strobe;
  espModeStrobe    = msg.modeStrobe;
  espCastleStrobe  = msg.castleStrobe;
  espLastHeartbeat = millis();
  if(msg.cmd != 0){ uint8_t nh=(cmdHead+1)&15; if(nh!=cmdTail){ memcpy((void*)&cmdQ[cmdHead],&msg,sizeof(msg)); cmdHead=nh; } }
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
  uint8_t brightness;             // active mode's live brightness (mirrors briByMode[animIndex])
  uint8_t briByMode[MAX_MODES];   // per-mode remembered brightness (Task 2)
  uint8_t strobeBrightness;
  uint8_t strobeOnMs;
  uint8_t strobeOffMs;
  uint8_t strobeSquares;
  uint8_t strobeGapMs;    // button-15 strobe: gap between flashes (hold 15 + +/-)
  float   animSpeed;      // 0.1..4.0, 1.0=normal
  uint16_t beatMs;        // tapped beat period for the glow, 0 = off
};
Settings cfg;

void saveSettings() {
  prefs.begin("grid", false);
  prefs.putInt("anim",   cfg.animIndex);
  prefs.putUChar("bri",  cfg.brightness);
  prefs.putBytes("brimode", cfg.briByMode, sizeof(cfg.briByMode));  // per-mode brightness (Task 2)
  prefs.putUChar("sbri", cfg.strobeBrightness);
  prefs.putUChar("son",  cfg.strobeOnMs);
  prefs.putUChar("soff", cfg.strobeOffMs);
  prefs.putUChar("ssq",  cfg.strobeSquares);
  prefs.putUChar("sgap", cfg.strobeGapMs);
  prefs.putFloat("spd",  cfg.animSpeed);
  prefs.putUShort("beat", cfg.beatMs);        // beat-glow tempo
  prefs.putUChar("hue",  gHueBase);
  prefs.putUChar("hspd", gHueSpeed);
  prefs.putUChar("shue", gStrobeHue);
  prefs.end();
}

void loadSettings() {
  prefs.begin("grid", true);
  cfg.animIndex        = prefs.getInt("anim",   0);
  cfg.brightness       = prefs.getUChar("bri",  40);
  bool briOk           = prefs.getBytes("brimode", cfg.briByMode, sizeof(cfg.briByMode))
                           == sizeof(cfg.briByMode);   // per-mode brightness blob (Task 2)
  cfg.strobeBrightness = prefs.getUChar("sbri", 200);
  cfg.strobeOnMs       = prefs.getUChar("son",  40);
  cfg.strobeOffMs      = prefs.getUChar("soff", 40);
  cfg.strobeSquares    = prefs.getUChar("ssq",  30);
  cfg.strobeGapMs      = prefs.getUChar("sgap", STROBE_GAP_DEF);
  cfg.animSpeed        = prefs.getFloat("spd",  1.0f);
  cfg.beatMs           = prefs.getUShort("beat", 0);
  gHueBase             = prefs.getUChar("hue",  0);
  gHueSpeed            = prefs.getUChar("hspd", 0);
  gStrobeHue           = prefs.getUChar("shue", 0);
  prefs.end();
  cfg.animIndex        = constrain(cfg.animIndex, 0, TOTAL_ANIMS-1);
  // Per-mode brightness (Task 2): seed defaults if the blob was missing, then clamp.
  if(!briOk) for(int i=0;i<MAX_MODES;i++) cfg.briByMode[i]=BRI_DEFAULT;
  for(int i=0;i<MAX_MODES;i++) cfg.briByMode[i]=constrain(cfg.briByMode[i],1,MAX_BRIGHTNESS);
  cfg.brightness       = constrain(cfg.briByMode[cfg.animIndex], 1, MAX_BRIGHTNESS); // active = slot
  cfg.strobeBrightness = constrain(cfg.strobeBrightness, 1, 255);
  cfg.strobeOnMs       = constrain(cfg.strobeOnMs, 5, 200);
  cfg.strobeOffMs      = constrain(cfg.strobeOffMs, 5, 200);
  cfg.strobeSquares    = constrain(cfg.strobeSquares, 1, 122);
  cfg.strobeGapMs      = constrain(cfg.strobeGapMs, STROBE_GAP_MIN, STROBE_GAP_MAX);
  cfg.animSpeed        = constrain(cfg.animSpeed, 0.1f, 4.0f);
  // Beat-glow tempo: 0 stays off, otherwise clamp to a sane range and start the clock.
  if(cfg.beatMs){ cfg.beatMs=constrain(cfg.beatMs,BEAT_MIN_MS,BEAT_MAX_MS);
                  gBeatPeriod=gBeatTarget=(float)cfg.beatMs; }
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

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.printf("Receiver MAC: %s\n", WiFi.macAddress().c_str());
  if(esp_now_init() != ESP_OK) { Serial.println("ESP-NOW init failed!"); while(1) delay(1000); }
  esp_now_register_recv_cb(onReceive);
  Serial.printf("Grid ready. %d animations.\n", TOTAL_ANIMS);
}

// Apply one command from the controller.
void applyCommand(const GridMsg& m, uint32_t t){
  if(m.cmd==1){                                    // mode step
    cfg.animIndex=(cfg.animIndex+m.arg+TOTAL_ANIMS)%TOTAL_ANIMS;
    cfg.brightness=cfg.briByMode[cfg.animIndex];   // restore this mode's brightness (Task 2)
    FastLED.setBrightness(cfg.brightness);
    resetAnimState(t); saveSettings(); triggerOverlay(t,cfg.animIndex);
  } else if(m.cmd==2){                              // parameter step
    int d=m.arg;
    switch(m.target){
      case 0: cfg.brightness=(uint8_t)constrain((int)cfg.brightness+d*BRI_STEP,1,MAX_BRIGHTNESS); cfg.briByMode[cfg.animIndex]=cfg.brightness; FastLED.setBrightness(cfg.brightness); break;
      case 1: cfg.animSpeed=constrain(cfg.animSpeed+d*0.1f,0.1f,4.0f); break;
      case 2: gHueBase  =(uint8_t)(gHueBase  +d*6); break;   // Color (wraps)
      case 3: gStrobeHue=(uint8_t)(gStrobeHue+d*6); break;   // Strobe colour (wraps)
      case 4: gHueSpeed =(uint8_t)constrain((int)gHueSpeed+d,0,40); break; // Hue speed
      case 5: cfg.strobeGapMs=(uint8_t)constrain((int)cfg.strobeGapMs+d*STROBE_GAP_STEP,
                                                 STROBE_GAP_MIN,STROBE_GAP_MAX); break; // gap between flashes
    }
    saveSettings();
  } else if(m.cmd==3){                              // tap-tempo → beat-glow period in ms (NOT speed)
    int per=constrain((int)m.arg,BEAT_MIN_MS,BEAT_MAX_MS);
    gBeatTarget=(float)per;                         // ramped toward in loop(), never jumps
    if(gBeatPeriod<=0.0f) gBeatPeriod=gBeatTarget;  // very first tempo: adopt straight away
    cfg.beatMs=(uint16_t)per; saveSettings();
  }
}

void loop() {
  static uint32_t lastFrame     = 0;
  static uint32_t lastT         = 0;
  static bool     strobeActive  = false;
  static bool     strobeOn      = false;
  static uint32_t strobeFlip    = 0;
  static int      preStrobeAnim = 0;
  static uint8_t  preStrobeBri  = 40;

  uint32_t t  = millis();
  lastT       = t;

  // ── Sender alive? (heartbeat within timeout) ──────
  bool senderAlive = (espLastHeartbeat > 0) &&
                     ((t - (uint32_t)espLastHeartbeat) < (uint32_t)HEARTBEAT_TIMEOUT);

  // ── Strobe: momentary, driven by the controller ──
  bool strobeHeld = senderAlive && espStrobe;
  if(strobeHeld && !strobeActive){
    preStrobeAnim=cfg.animIndex; preStrobeBri=cfg.brightness;
    strobeActive=true; strobeOn=false; strobeFlip=t;
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
    GridMsg m; memcpy(&m,(const void*)&cmdQ[cmdTail],sizeof(m)); cmdTail=(cmdTail+1)&15;
    applyCommand(m,t);
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

  // ── Beat-Glow: pulse the frame in the tapped rhythm. Uses REAL time (not
  //    animSpeed), and eases toward a newly tapped tempo instead of jumping.
  if(gBeatPeriod>0.0f){
    gBeatPeriod += (gBeatTarget-gBeatPeriod)*BEAT_RAMP;   // graduell erhöht/verringert
    gBeatPhase  += frameDt/gBeatPeriod;
    gBeatPhase  -= floorf(gBeatPhase);                    // wrap 0..1, phase stays continuous
    gBeatGlow    = BEAT_MIN+(1.0f-BEAT_MIN)*expf(-gBeatPhase*BEAT_DECAY);
  } else gBeatGlow=1.0f;
  static float virtualT = 0.0f;
  virtualT += frameDt * cfg.animSpeed;
  uint32_t at = (uint32_t)virtualT;
  float    dt = frameDt * cfg.animSpeed;

  // ── Auto-rotate the global hue by Hue-speed ───────
  static float hueAcc=0; hueAcc+=gHueSpeed*frameDt*0.02f; if(hueAcc>=256.0f)hueAcc-=256.0f; gHueAuto=(uint8_t)hueAcc;

  // ── Send sync to sender every 200ms so it knows current state ──
  static uint32_t lastSync=0;
  if(senderKnown && (t-lastSync)>200){
    lastSync=t;
    // Add sender as peer if not already
    if(!syncPeerAdded){
      esp_now_peer_info_t peer={};
      memcpy(peer.peer_addr,senderMAC,6);
      peer.channel=0;peer.encrypt=false;
      if(esp_now_add_peer(&peer)==ESP_OK)syncPeerAdded=true;
    }
    if(syncPeerAdded){
      SyncMsg sync;
      sync.brightness       = cfg.brightness;
      sync.strobeBrightness = cfg.strobeBrightness;
      sync.strobeOnMs       = cfg.strobeOnMs;
      sync.strobeOffMs      = cfg.strobeOffMs;
      sync.strobeSquares    = cfg.strobeSquares;
      sync.hueBase          = gHueBase;
      sync.hueSpeed         = gHueSpeed;
      sync.strobeHue        = gStrobeHue;
      CRGB tc=CRGB(CHSV((uint8_t)(gHueBase+gHueAuto),255,220));   // exact tint colour (rainbow map + auto)
      sync.tintR=tc.r; sync.tintG=tc.g; sync.tintB=tc.b;
      CRGB sc=(gStrobeHue==0)?CRGB(200,200,200):CRGB(CHSV(gStrobeHue,255,220));
      sync.sColR=sc.r; sync.sColG=sc.g; sync.sColB=sc.b;
      sync.animIndex        = cfg.animIndex;
      sync.animSpeed        = cfg.animSpeed;
      esp_now_send(senderMAC,(uint8_t*)&sync,sizeof(sync));
    }
  }

  // ── Strobe rendering (overrides blackout while held) ─
  if(strobeActive){
    if(t>strobeFlip){
      strobeOn=!strobeOn;
      // Short fixed flash, user-tunable gap (hold 15 + press +/-).
      strobeFlip=t+(uint32_t)(strobeOn?STROBE_FLASH_MS:cfg.strobeGapMs);
      if(strobeOn){
        CRGB sc = gStrobeHue==0 ? CRGB(CRGB::White) : CRGB(CHSV(gStrobeHue,255,255));  // Strobe colour (0 = white)
        FastLED.setBrightness(cfg.strobeBrightness);
        FastLED.clear();
        if(cfg.strobeSquares >= 122) {
          for(int e=0;e<NUM_ELEC;e++){
            for(int i=0;i<LEDS_PER_STRIP;i++){ledsX[e][i]=sc;ledsY[e][i]=sc;}
            ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;
            ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;
          }
        } else {
          uint8_t count = cfg.strobeSquares;
          static uint8_t sqIdx[121];
          for(int i=0;i<121;i++) sqIdx[i]=i;
          for(int i=0;i<(int)count;i++){int j=i+random(121-i);uint8_t tmp=sqIdx[i];sqIdx[i]=sqIdx[j];sqIdx[j]=tmp;}
          for(int s=0;s<(int)count;s++){
            int row=sqIdx[s]/11;int col=sqIdx[s]%11;
            int r1=row+1;int c1=col+1;int r2=row+2;int c2=col+2;
            for(int led=INTS[col]+1;led<INTS[col+1];led++){setLED(false,r1,led,sc);setLED(false,r2,led,sc);}
            for(int led=INTS[row]+1;led<INTS[row+1];led++){setLED(true,c1,led,sc);setLED(true,c2,led,sc);}
          }
        }
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

  // ── Normal animation ──────────────────────────────
  // Set the base brightness; showGrid() multiplies it by the AGC gain (Task 2b).
  gBaseBri = modeStrobeHeld ? (uint8_t)min((int)cfg.brightness*7/4,255) : cfg.brightness;
  FastLED.setBrightness(gBaseBri);   // fallback for anims that FastLED.show() without showGrid()
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

  // ── Mode-change readout: flash the number in the top-right on
  //    top of the running animation for ~0.5s (Task 1). ──
  if(t < overlayUntil){
    drawNumberTopRight(t);
    FastLED.show();
  }
}
