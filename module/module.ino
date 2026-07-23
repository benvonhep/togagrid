// ═══════════════════════════════════════════════════════
//  Toga LED Module — ESP-NOW receiver, driven by the controller + grid.
//
//  Listens to two broadcasts and never transmits:
//    Controller (100ms)  → the three momentary strobe flags + liveness.
//                          Low latency, which is why the strobes ride here.
//    Grid       (200ms)  → the grid's complete ABSOLUTE state: brightness,
//                          colour, hue-speed, speed, strobe colour/gap, beat.
//
//  The module applies no +/- steps of its own. Every param it shows —
//  including the MODE — is mirrored from the grid's absolute state, so a
//  module that boots late or misses packets re-locks onto the grid within one
//  sync (~200ms) and can never drift out of tint or out of mode.
//
//  The grid's 90 modes map 1:1 onto 90 resolution-independent 1D modes
//  (activeEffect() + module_effects.h), so a mode change on the controller
//  changes the modules too. A module can be pinned to one mode (or blackout) by
//  hand if wanted — see `effect` / `follow` on the console.
//
//  Per-device config (group, geometry, pin, effect, gain) lives in NVS and
//  is set over serial or from the web UI — the same binary flashes to every
//  module, over the air: ./flash-ota.sh module toga-mod-<mac6>.local
//  Type `help` on the serial console at 115200.
//
//  Board: ESP32 Dev Module · Partition: min_spiffs (OTA needs 2 app slots)
//  FastLED 3.6.0 · core 2.0.17
// ═══════════════════════════════════════════════════════
#include <FastLED.h>
#include <math.h>
#include <Preferences.h>
#include "module_config.h"      // pulls in <toga_proto.h>
#include <toga_ota.h>          // on-demand firmware update; see loop()
#include <WebServer.h>         // fallback: serve the module page over the phone hotspot
#include <ESPmDNS.h>

// ── Fallback module-manager web UI (only used off-controller) ───────────
// When there is no controller, a module joins the phone hotspot and serves the
// SAME page the controller serves (ported from controller.ino). Every module on
// the hotspot is on one channel, so they still hear each other's hellos over
// ESP-NOW — any one of them can list them all. Declared above the first function
// so arduino-cli's hoisted prototype for upsert() (returns ModEntry*) sees the
// type. Active only in NET_FALLBACK; costs nothing on-controller.
WebServer server(80);
#define MAX_MODULES  16
#define OFFLINE_MS   5000        // 5 missed hellos → offline (but stays listed)
#define FORGET_MS    600000      // 10 min offline → drop the row entirely
struct ModEntry {
  bool     used;
  uint8_t  mac[6];
  uint8_t  group, effectPin, effectNow, gain, pin, linkAlive;
  uint16_t w, h;
  uint32_t uptimeS;
  uint16_t battMv;
  uint8_t  battFlags;
  uint8_t  gap;            // output gap: 1 = every LED, N = light every Nth
  uint8_t  floor;          // floor-strip flow: 0 off, 1 fwd, 2 rev
  uint32_t lastSeen;
};
ModEntry mods[MAX_MODULES];
#define HQ_LEN 8
volatile TogaHelloMsg helloQ[HQ_LEN];
volatile uint8_t hqHead = 0, hqTail = 0;

// ── Network mode ──
// Boot → hunt for the controller AP for TOGA_FALLBACK_MS; found → NET_CONTROLLER
// (normal), not found → NET_FALLBACK (join the phone hotspot, serve the page).
// Fallback is a boot-time decision and stays until reboot (see netTick()).
enum NetMode { NET_CONNECTING, NET_CONTROLLER, NET_FALLBACK };
NetMode  gNetMode = NET_CONNECTING;
uint32_t gBootMs  = 0;
char     gHotSsid[33] = TOGA_HOTSPOT_SSID;   // NVS overrides in loadConfig()
char     gHotPass[65] = TOGA_HOTSPOT_PASS;
bool     gIsHost   = false;                  // do we own toga-mod.local? (fallback election)
uint32_t gFbConnAt = 0;                      // when we joined the hotspot (grace before claiming)

// Keep a freshly OTA'd image on probation until it has proven itself, instead
// of letting initArduino() bless it before setup() runs. extern "C" is
// mandatory — see the long note in toga_ota.h.
extern "C" bool verifyRollbackLater() { return true; }

// ── Per-device config (NVS) ──
uint8_t  gGroup  = MODULE_GROUP_DEFAULT;  // which broadcasts we act on
// Which mode to show. EFFECT_FOLLOW = derive it from the grid's mode (the
// normal case); 0..89 pins this module to one mode, TOGA_EFFECT_OFF = blackout.
// Shared with the web UI, which sends this same value — see toga_proto.h.
#define EFFECT_FOLLOW TOGA_EFFECT_FOLLOW
uint8_t  gEffectPin = EFFECT_FOLLOW;
uint8_t  gGain   = 100;                   // per-module brightness trim, %
// Staged geometry: only takes effect on reboot, because the FastLED
// controller is bound to a pin and length at addLeds() time.
uint16_t cfgW = 32, cfgH = 8;
uint8_t  cfgPin = 4;
// Which grid functions we slave to (strobe/brightness/beat/blackout). Default ALL,
// so a fresh module mirrors the grid exactly as before. The web UI toggles these.
uint8_t  gFollowMask = TOGA_FOLLOW_ALL;
// Sparse-strip spacing: light every gGap-th physical LED, blank the rest. 1 =
// every LED (feature off, default). Set from the web UI or serial `gap`. See
// applyGap() in showModule() and TOGA_GAP_* in toga_proto.h.
uint8_t  gGap    = TOGA_GAP_OFF;
// Floor-strip flow (v8): TOGA_FLOOR_OFF/FWD/REV. When not OFF, this module reacts to
// the controller's spot key (button 5) by scrolling its current render one pass along
// the strip in the FWD/REV direction; non-floor modules ignore button 5. See the flow
// block in loop() and gFlowShift in showModule().
uint8_t  gFloorFlow = TOGA_FLOOR_OFF;
char     gName[16] = {0};                 // user label; default mod-<mac3> set in setup()
uint32_t gRebootAt = 0;                   // non-zero → ESP.restart() once past it (LED-count change)
uint32_t gIdentifyUntil = 0;              // non-zero → blink the first 2 LEDs red to locate this strip until then
#define  IDENTIFY_MS 4000

// ── Live geometry ──
uint16_t gWidth = 32, gHeight = 8, gNumLeds = 256;
// Two buffers, deliberately:
//   fx[]   — what the effects paint, always in NATIVE hues.
//   leds[] — what FastLED drives; showModule() renders fx into it.
// The grid gets away with rotating hue in place because every one of its 90
// animations repaints the whole frame (not one uses fadeToBlackBy). Our
// effects DO fade — sinelon/juggle/confetti/starfield keep a trail — so
// rotating in place would re-rotate those trail pixels every frame and smear
// them across the whole colour wheel within a few frames. Keeping fx native
// is what actually holds the module to the grid's tint.
CRGB     fx[MAX_LEDS];
CRGB     leds[MAX_LEDS];
// The FastLED controller bound to our pin (addLedsForPin). Kept so setup() can
// briefly drive a longer black strip than gNumLeds to blank stuck LEDs — see
// the boot-blank in setup() and MODULE_BLANK_LEDS.
CLEDController* gLedCtrl = nullptr;

// ── State mirrored from the grid's sync ──
uint8_t  gBri        = BRI_DEFAULT;
float    gAnimSpeed  = 1.0f;
uint8_t  gHueBase = 0, gHueAuto = 0, gHueSpeed = 0, gStrobeHue = 0;
uint8_t  gStrobeGap  = STROBE_GAP_DEF;
// Mode/castle-strobe on/off, mirrored from the grid (v6). Default to the module's
// own constants until the first sync arrives — matches the grid's default.
uint8_t  gStrobeOnMs  = MODE_ON_MS;
uint8_t  gStrobeOffMs = MODE_OFF_MS;
float    gBeatPeriod = 0.0f, gBeatPhase = 0.0f, gBeatGlow = 1.0f;
CRGB     gSingleColor = CRGB(255, 160, 60);
int16_t  gAnimIndex  = 0;      // the grid's current mode; drives our effect

// Virtual clock: lets the Speed param reach FastLED's beatsin* functions,
// which are otherwise hard-wired to real millis(). Kept so that
// (millis() - gTimebase) == gVirtMs, continuous across speed changes.
//
// gVirtMs is integer ms, NOT a float that grows forever. A float here dies
// on a long-running installation: once it passes 2^29 its ulp exceeds the
// 20ms step, `+= 20` rounds to a no-op, and every beatsin*-driven effect
// freezes on a static frame — permanently, at ~6 days of uptime (~1.5 days
// at Speed 4). uint32_t ms wraps cleanly instead, and beatsin*'s internal
// subtraction is wrap-safe.
uint32_t gTimebase = 0;
uint32_t gVirtMs   = 0;

// ── Serpentine-by-column map (identical to the old xyMap) ──
static inline uint16_t XY(uint16_t x, uint16_t y) {
  return (x & 0x01) ? (uint16_t)(x * gHeight) + (gHeight - 1 - y)
                    : (uint16_t)(x * gHeight) + y;
}
// Bounds-checked pixel accessor. Out-of-range writes land in a scratch pixel
// that is never shown, so a geometry mistake can't corrupt memory.
static CRGB gTrash;
static inline CRGB& ledXY(int x, int y) {
  if (x < 0 || y < 0 || x >= (int)gWidth || y >= (int)gHeight) return gTrash;
  uint16_t i = XY((uint16_t)x, (uint16_t)y);
  return (i < gNumLeds) ? fx[i] : gTrash;
}

#include "module_effects.h"     // needs fx/gNumLeds/ledXY/gTimebase above
#include "module_battery.h"     // Li-Ion monitoring + undervoltage lockout (opt-in via `bmon`)

// Which mode this module is showing right now.
//
// FOLLOW now maps 1:1: grid mode k → module mode k, for all k in 0..89. The
// grid's mode reaches us through the sync's absolute animIndex — no command, no
// step — so a module that boots late or misses packets shows the right mode
// within one sync, exactly like every other param.
//
// A module can also be PINNED to a fixed mode (0..89), or to TOGA_EFFECT_OFF
// (blackout — the successor to the old effect id 8).
static inline uint8_t activeEffect() {
  if (gEffectPin < 90)               return gEffectPin;              // pinned mode
  if (gEffectPin == TOGA_EFFECT_OFF) return TOGA_EFFECT_OFF;         // pinned blackout
  return (gAnimIndex >= 0 && gAnimIndex < 90) ? (uint8_t)gAnimIndex  // FOLLOW: 1:1
                                              : 0;                   // defensive
}

// ── ESP-NOW inbound state ──
volatile bool     espStrobe = false, espModeStrobe = false, espCastleStrobe = false;
volatile bool     espBlackout = false;        // latched blackout, rides the flags like the strobes
volatile bool     espSpot = false;            // momentary moving-spot, rides the flags like the strobes
volatile uint32_t espLastHeartbeat = 0;
volatile TogaSyncMsg syncBuf;
volatile bool     syncPending = false;
volatile TogaModCfgMsg cfgBuf;                // settings addressed at us, from the web UI
volatile bool     cfgPending = false;
static uint8_t    gMyMac[6] = {};             // filled in setup(); the UI addresses us by it

// Runs in the WiFi task: stay short, no Serial, no allocation.
// There is no command queue — the module applies no steps, so the only thing
// it needs from a command is the flags.
void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  TogaCmdMsg m;
  if (togaParseCmd(data, len, gGroup, &m)) {
    espStrobe        = (m.flags & TOGA_F_STROBE)        != 0;
    espModeStrobe    = (m.flags & TOGA_F_MODE_STROBE)   != 0;
    espCastleStrobe  = (m.flags & TOGA_F_CASTLE_STROBE) != 0;
    espBlackout      = (m.flags & TOGA_F_BLACKOUT)      != 0;
    espSpot          = (m.flags & TOGA_F_SPOT)          != 0;
    espLastHeartbeat = millis();
    return;
  }
  TogaSyncMsg s;
  if (togaParseSync(data, len, &s)) {
    memcpy((void*)&syncBuf, &s, sizeof(s));   // buffered: loop() owns the state
    syncPending = true;
    return;
  }
  TogaModCfgMsg c;
  if (togaParseModCfg(data, len, gMyMac, &c)) {   // addressed at us by MAC, or dropped
    memcpy((void*)&cfgBuf, &c, sizeof(c));        // buffered: applying it writes NVS,
    cfgPending = true;                            // which must not happen in this task
    return;
  }
  // In fallback we ARE the web server, so we also collect peer hellos into a
  // ring (loop() merges them into the table). On-controller this is dead weight,
  // so it is gated — a normal module ignores other modules entirely.
  if (gNetMode == NET_FALLBACK) {
    TogaHelloMsg hm;
    if (togaParseHello(data, len, &hm)) {
      uint8_t nh = (uint8_t)((hqHead + 1) % HQ_LEN);
      if (nh != hqTail) { memcpy((void*)&helloQ[hqHead], &hm, sizeof(hm)); hqHead = nh; }
      return;
    }
  }
}

// The grid's absolute truth replaces ours wholesale. No clamping needed —
// the grid already clamped, and re-clamping here could only introduce drift.
void applySync(const TogaSyncMsg& s) {
  // Brightness follow is opt-out: a module not slaving to grid brightness runs at
  // full (its own gGain trim still applies later in showModule()).
  gBri        = (gFollowMask & TOGA_FOLLOW_BRIGHT) ? s.brightness : MAX_BRIGHTNESS;
  gAnimSpeed  = s.animSpeed;
  gHueBase    = s.hueBase;
  gHueAuto    = s.hueAuto;      // snap: both sides rotate at the same rate
  gHueSpeed   = s.hueSpeed;
  gStrobeHue  = s.strobeHue;
  gStrobeGap  = s.strobeGapMs;
  gStrobeOnMs  = s.strobeOnMs  ? s.strobeOnMs  : MODE_ON_MS;   // mirror the grid's mode/castle-strobe
  gStrobeOffMs = s.strobeOffMs ? s.strobeOffMs : MODE_OFF_MS;  // rhythm (0 = pre-v6 grid → keep default)
  gBeatPeriod = (float)s.beatMs;
  gBeatPhase  = s.beatPhase / 255.0f;   // keeps our pulse in phase with the grid
  gSingleColor = CRGB(s.tintR, s.tintG, s.tintB);
  gAnimIndex  = s.animIndex;            // mode follows the grid; see activeEffect()
}

// ── NVS ──
Preferences prefs;

void loadConfig() {
  prefs.begin("module", true);
  gGroup  = prefs.getUChar("grp",  MODULE_GROUP_DEFAULT);
  cfgW    = prefs.getUShort("w",   32);
  cfgH    = prefs.getUShort("h",   8);
  cfgPin  = prefs.getUChar("pin",  4);
  gEffectPin = prefs.getUChar("eff", EFFECT_FOLLOW);
  gGain   = prefs.getUChar("gain", 100);
  battMonEn = prefs.getUChar("bmon", 0) ? 1 : 0;   // default OFF (bench-safe)
  gFollowMask = prefs.getUChar("follow", TOGA_FOLLOW_ALL);   // default: follow everything (v5)
  gGap    = prefs.getUChar("gap", TOGA_GAP_OFF);             // sparse-strip spacing (v7); 1 = every LED
  gFloorFlow = prefs.getUChar("floor", TOGA_FLOOR_OFF);      // floor-strip flow (v8); 0 = not a floor strip
  { String nm = prefs.getString("name", "");                 // user label (v5); "" until named
    strncpy(gName, nm.c_str(), sizeof(gName) - 1); gName[sizeof(gName) - 1] = 0; }
  // Fallback phone-hotspot creds. gHotSsid/gHotPass already hold the compile-time
  // constants; a per-board `hotspot <ssid> <pass>` on serial overrides them here.
  String hss = prefs.getString("hss", "");
  String hpw = prefs.getString("hpw", "");
  prefs.end();
  if (hss.length()) { strncpy(gHotSsid, hss.c_str(), sizeof(gHotSsid) - 1); gHotSsid[sizeof(gHotSsid) - 1] = 0; }
  if (hpw.length()) { strncpy(gHotPass, hpw.c_str(), sizeof(gHotPass) - 1); gHotPass[sizeof(gHotPass) - 1] = 0; }

  if (cfgW < 1 || cfgH < 1 || (uint32_t)cfgW * cfgH > MAX_LEDS) {
    Serial.printf("BAD geometry %ux%u in NVS — falling back to 32x8\n", cfgW, cfgH);
    cfgW = 32; cfgH = 8;
  }
  // Migrate the old "off" pin (effect id 8 = powersave/black) to the new OFF
  // sentinel, so a module previously pinned to blackout stays black after this
  // OTA instead of rendering mode 8 (blackHole).
  if (gEffectPin == 8) gEffectPin = TOGA_EFFECT_OFF;
  if (!(gEffectPin < 90 || gEffectPin == TOGA_EFFECT_OFF || gEffectPin == TOGA_EFFECT_FOLLOW))
    gEffectPin = EFFECT_FOLLOW;                                  // incl. any stale value
  gGain = constrain((int)gGain, 10, 200);
  if (gGap < TOGA_GAP_OFF) gGap = TOGA_GAP_OFF;   // 0 in NVS (never written) → every LED
  if (gFloorFlow > TOGA_FLOOR_MAX) gFloorFlow = TOGA_FLOOR_OFF;   // stale/garbage → off

  gWidth = cfgW; gHeight = cfgH; gNumLeds = (uint16_t)((uint32_t)gWidth * gHeight);
}

bool saveConfig() {
  if ((uint32_t)cfgW * cfgH > MAX_LEDS) {
    Serial.printf("refused: %ux%u = %lu > MAX_LEDS %d\n",
                  cfgW, cfgH, (unsigned long)cfgW * cfgH, MAX_LEDS);
    return false;
  }
  prefs.begin("module", false);
  prefs.putUChar("grp",  gGroup);
  prefs.putUShort("w",   cfgW);
  prefs.putUShort("h",   cfgH);
  prefs.putUChar("pin",  cfgPin);
  prefs.putUChar("eff",  gEffectPin);
  prefs.putUChar("gain", gGain);
  prefs.putUChar("bmon", battMonEn);
  prefs.putUChar("follow", gFollowMask);
  prefs.putUChar("gap",  gGap);
  prefs.putUChar("floor", gFloorFlow);
  prefs.putString("name", gName);
  prefs.end();
  return true;
}

// PIN is a template parameter in FastLED — there is no runtime-pin API — so
// every allowed pin needs its own instantiation. addLeds must be called
// exactly once: FastLED.show() drives every registered controller.
static bool addLedsForPin(uint8_t pin) {
  switch (pin) {
    case  4: gLedCtrl = &FastLED.addLeds<LED_TYPE,  4, COLOR_ORDER>(leds, gNumLeds); break;
    case  5: gLedCtrl = &FastLED.addLeds<LED_TYPE,  5, COLOR_ORDER>(leds, gNumLeds); break;
    case 16: gLedCtrl = &FastLED.addLeds<LED_TYPE, 16, COLOR_ORDER>(leds, gNumLeds); break;
    case 17: gLedCtrl = &FastLED.addLeds<LED_TYPE, 17, COLOR_ORDER>(leds, gNumLeds); break;
    case 18: gLedCtrl = &FastLED.addLeds<LED_TYPE, 18, COLOR_ORDER>(leds, gNumLeds); break;
    case 21: gLedCtrl = &FastLED.addLeds<LED_TYPE, 21, COLOR_ORDER>(leds, gNumLeds); break;
    default: return false;
  }
  FastLED.setCorrection(0xffe0b0);
  return true;
}

// The names live in toga_proto.h: the web UI builds its dropdown from the same
// table, and a list that disagrees with the modules would be worse than none.
const char* effectName(uint8_t e) { return togaEffectName(e); }

// ── Web UI: announce ourselves, and take settings addressed at us ──────────
// Effect and gain persist. The serial console deliberately keeps them live-only
// until `save`, because there you can watch and undo. The web UI is used from
// across the room on a phone: a setting that quietly reverts on the next power
// cycle would be a bug report, not a feature. Only these two keys are written —
// NOT saveConfig(), which would also commit w/h/pin someone may have staged.
void applyModCfg(const TogaModCfgMsg& c) {
  // Identify is a momentary "blink so I can find you" — not a stored effect. Trigger
  // it and fall through; it never touches gEffectPin or NVS.
  if (c.effectPin == TOGA_EFFECT_IDENTIFY) { gIdentifyUntil = millis() + IDENTIFY_MS; Serial.println(F("modcfg: identify blink")); }
  bool dirty = false;
  if (c.effectPin != TOGA_CFG_KEEP &&
      (c.effectPin < 90 || c.effectPin == TOGA_EFFECT_OFF || c.effectPin == TOGA_EFFECT_FOLLOW)) {
    gEffectPin = c.effectPin; dirty = true;
  }
  if (c.gain != 0) { gGain = constrain((int)c.gain, 10, 200); dirty = true; }
  if (c.battMon != TOGA_CFG_KEEP) { battMonEn = c.battMon ? 1 : 0; dirty = true; }
  if (c.followMask != TOGA_FOLLOW_KEEP) { gFollowMask = c.followMask & TOGA_FOLLOW_ALL; dirty = true; }
  if (c.gap != TOGA_GAP_KEEP) { gGap = c.gap; dirty = true; }   // 1 = every LED, N = every Nth (v7)
  if (c.floorFlow != TOGA_FLOOR_KEEP && c.floorFlow <= TOGA_FLOOR_MAX) { gFloorFlow = c.floorFlow; dirty = true; }   // floor-strip flow (v8)
  if (c.name[0] != '\0') { strncpy(gName, c.name, sizeof(gName) - 1); gName[sizeof(gName) - 1] = 0; dirty = true; }
  if (dirty) {                              // persist the live settings (not staged w/h/pin)
    prefs.begin("module", false);
    prefs.putUChar("eff",  gEffectPin);
    prefs.putUChar("gain", gGain);
    prefs.putUChar("bmon", battMonEn);
    prefs.putUChar("follow", gFollowMask);
    prefs.putUChar("gap",  gGap);
    prefs.putUChar("floor", gFloorFlow);
    prefs.putString("name", gName);
    prefs.end();
    printStatus();                          // the serial console stays honest too
  }
  // LED count is geometry — FastLED binds length at addLeds(), so stage w=n/h=1,
  // persist (saveConfig writes w/h too), and reboot to apply. Scheduled, not
  // immediate, so the 3x cfg broadcast and a final hello flush before we drop off.
  if (c.ledCount != 0) {
    cfgW = (uint16_t)constrain((int)c.ledCount, MOD_LEDS_MIN, MAX_LEDS); cfgH = 1;
    if (saveConfig()) {
      Serial.printf("modcfg: LED count -> %u, rebooting to apply\n", cfgW);
      gRebootAt = millis() + 400;
    }
  }
}

// One hello per second. Costs one 32-byte broadcast — the controller sends 10x
// that per second — and nothing in the installation depends on it arriving.
#define HELLO_MS 1000
void sendHello(uint32_t t, bool linkAlive) {
  static uint32_t lastHello = 0;
  static uint16_t seq = 0;
  if (t - lastHello < HELLO_MS) return;
  lastHello = t;
  TogaHelloMsg m = {};
  togaInitHeader(m.h, TOGA_MSG_HELLO, TOGA_GROUP_ALL, ++seq);
  memcpy(m.mac, gMyMac, 6);
  m.group     = gGroup;
  m.effectPin = gEffectPin;
  m.effectNow = activeEffect();
  m.gain      = gGain;
  m.pin       = cfgPin;
  m.linkAlive = linkAlive ? 1 : 0;
  m.width     = gWidth;
  m.height    = gHeight;
  m.followMask = gFollowMask;
  m.gap       = gGap;                        // report the current output gap (v7)
  m.floorFlow = gFloorFlow;                  // report the floor-strip flow role (v8)
  strncpy(m.name, gName, sizeof(m.name));   // m is zero-init, so short names stay NUL-terminated
  m.uptimeS   = t / 1000;
  m.battMv    = (uint16_t)lroundf(battVolt * 1000.0f);
  m.battFlags = (battMonEn ? TOGA_BATT_F_ENABLED : 0) | (battLowCount > 0 ? TOGA_BATT_F_LOW : 0);
  esp_now_send(TOGA_BCAST, (const uint8_t*)&m, sizeof(m));
}

void printStatus() {
  bool alive = (espLastHeartbeat > 0) && ((millis() - (uint32_t)espLastHeartbeat) < HEARTBEAT_TIMEOUT);
  Serial.println(F("─── TOGA MODULE ───"));
  Serial.printf("name     %s\n", gName);
  Serial.printf("mac      %s  ch=%d\n", WiFi.macAddress().c_str(), TOGA_CHANNEL);
  Serial.printf("slave    0x%02x  (strobe=%d bright=%d beat=%d blackout=%d)\n", gFollowMask,
                !!(gFollowMask & TOGA_FOLLOW_STROBE), !!(gFollowMask & TOGA_FOLLOW_BRIGHT),
                !!(gFollowMask & TOGA_FOLLOW_BEAT),   !!(gFollowMask & TOGA_FOLLOW_BLACKOUT));
  Serial.printf("group    %u   (0=all 1=grid 2=modules)\n", gGroup);
  Serial.printf("live     %ux%u = %u leds, pin %u\n", gWidth, gHeight, gNumLeds, cfgPin);
  if (cfgW != gWidth || cfgH != gHeight)
    Serial.printf("staged   %ux%u  — reboot to apply\n", cfgW, cfgH);
  if (gEffectPin == EFFECT_FOLLOW)
    Serial.printf("effect   follow grid mode %d -> %u (%s)\n",
                  gAnimIndex, activeEffect(), effectName(activeEffect()));
  else
    Serial.printf("effect   PINNED %u (%s)  — `follow` to track the grid mode again\n",
                  gEffectPin, effectName(gEffectPin));
  Serial.printf("gain     %u%%\n", gGain);
  if (gGap > TOGA_GAP_OFF)
    Serial.printf("gap      %u  (every %u-th LED lit, %u dark between)\n", gGap, gGap, gGap - 1);
  else
    Serial.printf("gap      off (every LED lit)\n");
  Serial.printf("floor    %s\n", gFloorFlow == TOGA_FLOOR_FWD ? "flow FWD (button 5 scrolls this strip)" :
                                 gFloorFlow == TOGA_FLOOR_REV ? "flow REV (button 5 scrolls this strip)" :
                                 "off (normal module; ignores button 5)");
  Serial.printf("battery  %.2f V  mon=%s  (cutoff %.2f V)\n",
                battVolt, battMonEn ? "on" : "off", BATT_THRESHOLD_MV / 1000.0f * 2.0f);
  Serial.printf("link     %s\n", alive ? "alive" : "NO CONTROLLER");
  Serial.printf("net      %s   fallback hotspot '%s'%s\n",
                gNetMode == NET_CONTROLLER ? "on togacontroller" :
                gNetMode == NET_FALLBACK   ? "FALLBACK (phone hotspot)" : "connecting…",
                gHotSsid, gHotPass[0] ? "" : "  (no password set!)");
  if (gNetMode == NET_FALLBACK)
    Serial.printf("page     http://%s.local/  or  http://%s/  %s\n",
                  gIsHost ? "toga-mod" : "toga-mod (another module)",
                  WiFi.localIP().toString().c_str(),
                  gIsHost ? "← this module hosts toga-mod.local" : "");
  Serial.printf("from grid: bri=%u speed=%.1f hue=%u/%u hspd=%u shue=%u gap=%u beat=%.0fms\n",
                gBri, gAnimSpeed, gHueBase, gHueAuto, gHueSpeed, gStrobeHue, gStrobeGap, gBeatPeriod);
  Serial.println(F("cmds: group|w|h|leds|pin|effect|gain|gap|floor|bmon|slave <n> · follow · name <str> · hotspot <ssid> <pass> · show · save · reboot"));
  Serial.println(F("      floor <0|1|2> = floor-strip flow: 0 off, 1 fwd, 2 rev (button 5 scrolls flagged strips; persists now)"));
  Serial.println(F("      gap <n> = light every n-th LED, blank n-1 between (1 = every LED / off; persists immediately)"));
  Serial.println(F("      slave <mask> = follow bits (1=strobe 2=bright 8=beat 16=blackout; 31=all, 0=independent)"));
  Serial.println(F("      bmon 1/0 = Li-Ion undervoltage lockout on/off (default off; persists immediately)"));
  Serial.println(F("      effect <0-89>|off pins this module · effect -1 (or `follow`) tracks the grid mode"));
  Serial.println(F("      effect/gain apply live · w/h/pin need save+reboot"));
}

// Line-based, non-blocking.
void serialPoll() {
  static char buf[64];
  static uint8_t n = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') { if (n < sizeof(buf) - 1) buf[n++] = c; continue; }
    buf[n] = 0; n = 0;
    char cmd[16] = {0}; long val = 0;
    int got = sscanf(buf, "%15s %ld", cmd, &val);
    if (got < 1) continue;
    if      (!strcmp(cmd, "show") || !strcmp(cmd, "help") || !strcmp(cmd, "?")) printStatus();
    else if (!strcmp(cmd, "group")  && got == 2) { gGroup = constrain((int)val, 0, 2); printStatus(); }
    else if (!strcmp(cmd, "w")      && got == 2) { cfgW = constrain((int)val, 1, MAX_LEDS); printStatus(); }
    else if (!strcmp(cmd, "h")      && got == 2) { cfgH = constrain((int)val, 1, 255); printStatus(); }
    else if (!strcmp(cmd, "leds")   && got == 2) { cfgW = constrain((int)val, MOD_LEDS_MIN, MOD_LEDS_MAX); cfgH = 1; printStatus(); }
    else if (!strcmp(cmd, "pin")    && got == 2) { cfgPin = (uint8_t)val; printStatus(); }   // validated on save
    else if (!strcmp(cmd, "effect")) {
      // "off" = pinned blackout; -1 or "follow" = take the mode from the grid
      // (the default); 0..89 = pin one mode.
      char arg2[16] = {0}; sscanf(buf, "%*s %15s", arg2);
      if      (!strcmp(arg2, "off"))               gEffectPin = TOGA_EFFECT_OFF;
      else if (!strcmp(arg2, "follow") || val < 0) gEffectPin = EFFECT_FOLLOW;
      else if (got == 2)                           gEffectPin = (uint8_t)constrain((int)val, 0, 89);
      printStatus();
    }
    else if (!strcmp(cmd, "follow")) { gEffectPin = EFFECT_FOLLOW; printStatus(); }
    else if (!strcmp(cmd, "name")) {                // user label (persists now, like the web UI)
      char nm[16] = {0};
      if (sscanf(buf, "%*s %15s", nm) == 1) {
        strncpy(gName, nm, sizeof(gName) - 1); gName[sizeof(gName) - 1] = 0;
        prefs.begin("module", false); prefs.putString("name", gName); prefs.end();
      }
      printStatus();
    }
    else if (!strcmp(cmd, "slave") && got == 2) {   // follow bitmask (decimal): 31=all, 0=independent
      gFollowMask = (uint8_t)val & TOGA_FOLLOW_ALL;
      prefs.begin("module", false); prefs.putUChar("follow", gFollowMask); prefs.end();
      printStatus();
    }
    else if (!strcmp(cmd, "gain")   && got == 2) { gGain = constrain((int)val, 10, 200); printStatus(); }
    else if (!strcmp(cmd, "gap")    && got == 2) {   // sparse-strip spacing: light every n-th LED (1 = off)
      gGap = (uint8_t)constrain((int)val, TOGA_GAP_OFF, TOGA_GAP_MAX);
      prefs.begin("module", false); prefs.putUChar("gap", gGap); prefs.end();   // persist now (like the web UI)
      printStatus();
    }
    else if (!strcmp(cmd, "floor")  && got == 2) {   // floor-strip flow: 0=off 1=fwd 2=rev (button 5 scrolls it)
      gFloorFlow = (uint8_t)constrain((int)val, TOGA_FLOOR_OFF, TOGA_FLOOR_MAX);
      prefs.begin("module", false); prefs.putUChar("floor", gFloorFlow); prefs.end();   // persist now
      printStatus();
    }
    else if (!strcmp(cmd, "bmon")   && got == 2) {   // battery undervoltage lockout on/off
      battMonEn = val ? 1 : 0;
      prefs.begin("module", false); prefs.putUChar("bmon", battMonEn); prefs.end();   // persist now (safety config)
      printStatus();
    }
    else if (!strcmp(cmd, "hotspot")) {              // fallback phone-hotspot creds (persist now)
      char ssid[33] = {0}, pass[65] = {0};
      int n = sscanf(buf, "%*s %32s %64s", ssid, pass);
      prefs.begin("module", false);
      if (n >= 1 && !strcmp(ssid, "clear")) {
        prefs.remove("hss"); prefs.remove("hpw");
        strncpy(gHotSsid, TOGA_HOTSPOT_SSID, sizeof(gHotSsid) - 1); gHotSsid[sizeof(gHotSsid) - 1] = 0;
        strncpy(gHotPass, TOGA_HOTSPOT_PASS, sizeof(gHotPass) - 1); gHotPass[sizeof(gHotPass) - 1] = 0;
        Serial.printf("hotspot: cleared → compiled default '%s'\n", gHotSsid);
      } else if (n >= 1) {
        strncpy(gHotSsid, ssid, sizeof(gHotSsid) - 1);          gHotSsid[sizeof(gHotSsid) - 1] = 0;
        strncpy(gHotPass, n >= 2 ? pass : "", sizeof(gHotPass) - 1); gHotPass[sizeof(gHotPass) - 1] = 0;
        prefs.putString("hss", gHotSsid);
        prefs.putString("hpw", gHotPass);
        Serial.printf("hotspot: '%s' saved (used only when no controller is found)\n", gHotSsid);
      } else {
        Serial.println(F("usage: hotspot <ssid> <pass>  |  hotspot clear   (ssid/pass are space-free)"));
      }
      prefs.end();
    }
    else if (!strcmp(cmd, "save"))   { if (saveConfig()) Serial.println(F("saved — reboot to apply w/h/pin")); }
    else if (!strcmp(cmd, "reboot")) { Serial.println(F("rebooting…")); delay(50); ESP.restart(); }
    else Serial.println(F("? — type help"));
  }
}

// Mirrors the grid's showGrid(): rotate every lit pixel by the global hue,
// then apply brightness * beat-glow.
//
// No AGC here, deliberately. The grid's AGC normalises perceived brightness
// across its 90 modes; a module has one. And AGC_TARGET is calibrated
// against the grid's buffer, where most counted pixels are dead spacers —
// the same code on a fully-lit panel computes a different gain, so copying
// it would push the module AWAY from matching. The per-module `gain` trim,
// set once by eye against the running grid, is what actually matches them.
// Renders fx -> leds, rotating every lit pixel by the grid's global hue.
// Non-destructive: fx keeps its native colours, so trails never compound.
// Circular output shift (in LEDs) applied during a floor-strip flow pass; 0 = none.
// Set by the flow block in loop(); makes the whole rendered frame scroll along the
// strip so a floor strip appears to "flow" on a button-5 press.
int16_t gFlowShift = 0;

void showModule(uint8_t baseBri) {
  uint8_t off = (uint8_t)(gHueBase + gHueAuto);
  int sh = gFlowShift;                          // floor-flow scroll (0 = none)
  if (off || sh) {
    for (uint16_t i = 0; i < gNumLeds; i++) {
      CRGB c = fx[i];
      if (off && (c.r | c.g | c.b)) { CHSV h = rgb2hsv_approximate(c); h.hue += off; c = h; }
      uint16_t d = sh ? (uint16_t)(((uint32_t)i + (uint32_t)sh) % gNumLeds) : i;
      leds[d] = c;
    }
  } else {
    memcpy(leds, fx, gNumLeds * sizeof(CRGB));
  }
  // Sparse-strip spacing: keep every gGap-th physical LED, blank the N-1 between
  // (gGap=3 → LED0 lit, LED1+2 dark, LED3 lit …). gGap<=1 is a no-op (every LED).
  // Applied here, on the output buffer, so it rides all 90 modes uniformly; the
  // strobe/castle/identify paths fill leds[] themselves and stay fully lit.
  if (gGap > TOGA_GAP_OFF)
    for (uint16_t i = 0; i < gNumLeds; i++)
      if (i % gGap) leds[i] = CRGB::Black;
  FastLED.setBrightness((uint8_t)constrain(
      (int)lroundf(baseBri * (gGain / 100.0f) * gBeatGlow), 1, MAX_BRIGHTNESS));
  FastLED.show();
}

// `cap` is a ceiling on what actually leaves the LEDs, so it lands after the
// gain trim — capping before it would let a module at gain 200% blow straight
// through the limit.
void flashSolid(CRGB c, uint8_t bri, uint8_t cap = 255) {
  FastLED.setBrightness((uint8_t)constrain((int)lroundf(bri * (gGain / 100.0f)), 1, (int)cap));
  fill_solid(leds, gNumLeds, c);
  FastLED.show();
}

// ═══════════════════════════════════════════════════════
//  Fallback web server + module table (ported from controller.ino).
//  Only alive in NET_FALLBACK: on the phone hotspot we ARE the web UI, listing
//  ourselves and every peer module that also failed to find the controller.
// ═══════════════════════════════════════════════════════
uint16_t txSeq = 0;
ModEntry* upsert(const uint8_t mac[6]) {
  for (int i = 0; i < MAX_MODULES; i++)
    if (mods[i].used && memcmp(mods[i].mac, mac, 6) == 0) return &mods[i];
  ModEntry* slot = nullptr;
  for (int i = 0; i < MAX_MODULES; i++) if (!mods[i].used) { slot = &mods[i]; break; }
  if (!slot) { slot = &mods[0]; for (int i = 1; i < MAX_MODULES; i++) if (mods[i].lastSeen < slot->lastSeen) slot = &mods[i]; }
  memset(slot, 0, sizeof(*slot));
  slot->used = true; memcpy(slot->mac, mac, 6);
  return slot;
}
void drainHellos(uint32_t t) {
  while (hqTail != hqHead) {
    TogaHelloMsg m; memcpy(&m, (const void*)&helloQ[hqTail], sizeof(m));
    hqTail = (uint8_t)((hqTail + 1) % HQ_LEN);
    ModEntry* e = upsert(m.mac);
    e->group = m.group; e->effectPin = m.effectPin; e->effectNow = m.effectNow;
    e->gain = m.gain; e->pin = m.pin; e->linkAlive = m.linkAlive;
    e->w = m.width; e->h = m.height; e->uptimeS = m.uptimeS;
    e->battMv = m.battMv; e->battFlags = m.battFlags;
    e->gap = m.gap;
    e->floor = m.floorFlow;
    e->lastSeen = t;
  }
  for (int i = 0; i < MAX_MODULES; i++)
    if (mods[i].used && (t - mods[i].lastSeen) > FORGET_MS) mods[i].used = false;
}
// We never hear our own hello, so paint our row straight from live state.
void seedSelfRow(uint32_t t) {
  ModEntry* e = upsert(gMyMac);
  e->group = gGroup; e->effectPin = gEffectPin; e->effectNow = activeEffect();
  e->gain = gGain; e->pin = cfgPin; e->linkAlive = 0;         // no controller in fallback
  e->gap = gGap;
  e->floor = gFloorFlow;
  e->w = gWidth; e->h = gHeight; e->uptimeS = t / 1000;
  e->battMv = (uint16_t)lroundf(battVolt * 1000.0f);
  e->battFlags = (battMonEn ? TOGA_BATT_F_ENABLED : 0) | (battLowCount > 0 ? TOGA_BATT_F_LOW : 0);
  e->lastSeen = t;
}
String macStr(const uint8_t m[6]) {
  char b[18]; snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  return String(b);
}
bool parseMac(const String& s, uint8_t out[6]) {
  int v[6];
  if (sscanf(s.c_str(), "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) return false;
  for (int i = 0; i < 6; i++) { if (v[i] < 0 || v[i] > 255) return false; out[i] = (uint8_t)v[i]; }
  return true;
}
// Settings for ONE module, by MAC. Broadcast 3x (unacked, idempotent absolute
// values). The channel-0 peer (see fallbackAddPeer) carries it on the hotspot
// channel — a module addressing itself here is fine, its own onReceive applies it.
void sendModCfg(const uint8_t mac[6], uint8_t effectPin, uint8_t gain, uint8_t battMon = TOGA_CFG_KEEP,
                uint8_t gap = TOGA_GAP_KEEP, uint8_t floorFlow = TOGA_FLOOR_KEEP) {
  TogaModCfgMsg m = {};
  togaInitHeader(m.h, TOGA_MSG_MODCFG, TOGA_GROUP_ALL, ++txSeq);
  memcpy(m.mac, mac, 6);
  m.effectPin = effectPin; m.gain = gain; m.battMon = battMon; m.gap = gap; m.floorFlow = floorFlow;
  for (int i = 0; i < 3; i++) esp_now_send(TOGA_BCAST, (const uint8_t*)&m, sizeof(m));
}
void handleModules() {
  uint32_t t = millis();
  String j = "{\"ch\":" + String(WiFi.channel()) +
             ",\"chWant\":" + String(WiFi.channel()) +        // on the hotspot channel by definition
             ",\"chOk\":true" +
             ",\"gridSeen\":false,\"gridMode\":0,\"effects\":[";
  for (uint8_t e = 0; e < TOGA_EFFECT_COUNT; e++) j += (e ? ",\"" : "\"") + String(togaEffectName(e)) + "\"";
  j += "],\"follow\":" + String(TOGA_EFFECT_FOLLOW) + ",\"mods\":[";
  bool first = true;
  for (int i = 0; i < MAX_MODULES; i++) {
    if (!mods[i].used) continue;
    if (!first) j += ",";
    first = false;
    uint32_t age = t - mods[i].lastSeen;
    j += "{\"mac\":\"" + macStr(mods[i].mac) + "\"" +
         ",\"online\":" + String(age < OFFLINE_MS ? "true" : "false") +
         ",\"age\":" + String(age / 1000) +
         ",\"grp\":" + String(mods[i].group) +
         ",\"eff\":" + String(mods[i].effectPin) +
         ",\"effNow\":" + String(mods[i].effectNow) +
         ",\"gain\":" + String(mods[i].gain) +
         ",\"pin\":" + String(mods[i].pin) +
         ",\"w\":" + String(mods[i].w) + ",\"h\":" + String(mods[i].h) +
         ",\"gap\":" + String(mods[i].gap) +
         ",\"floor\":" + String(mods[i].floor) +
         ",\"link\":" + String(mods[i].linkAlive ? "true" : "false") +
         ",\"up\":" + String(mods[i].uptimeS) + "}";
  }
  j += "]}";
  server.send(200, "application/json", j);
}
void handleSet() {
  uint8_t mac[6];
  if (!server.hasArg("mac") || !parseMac(server.arg("mac"), mac)) { server.send(400, "text/plain", "bad mac"); return; }
  uint8_t eff = TOGA_CFG_KEEP, gain = 0;
  if (server.hasArg("effect")) {
    int e = server.arg("effect").toInt();
    if (e != TOGA_EFFECT_FOLLOW && (e < 0 || e >= TOGA_EFFECT_COUNT)) { server.send(400, "text/plain", "bad effect"); return; }
    eff = (uint8_t)e;
  }
  if (server.hasArg("gain")) {
    int g = server.arg("gain").toInt();
    if (g < 10 || g > 200) { server.send(400, "text/plain", "bad gain"); return; }
    gain = (uint8_t)g;
  }
  uint8_t gap = TOGA_GAP_KEEP;
  if (server.hasArg("gap")) {                          // 1 = every LED (off), N = light every Nth
    int gp = server.arg("gap").toInt();
    if (gp < TOGA_GAP_OFF || gp > TOGA_GAP_MAX) { server.send(400, "text/plain", "bad gap"); return; }
    gap = (uint8_t)gp;
  }
  uint8_t floor = TOGA_FLOOR_KEEP;
  if (server.hasArg("floor")) {                        // 0 off, 1 fwd, 2 rev
    int f = server.arg("floor").toInt();
    if (f < TOGA_FLOOR_OFF || f > TOGA_FLOOR_MAX) { server.send(400, "text/plain", "bad floor"); return; }
    floor = (uint8_t)f;
  }
  sendModCfg(mac, eff, gain, TOGA_CFG_KEEP, gap, floor);
  server.send(200, "application/json", "{\"sent\":true}");
}
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>TOGA Module</title><style>
body{font:15px system-ui,sans-serif;margin:0;padding:12px;background:#111;color:#eee}
h1{font-size:17px;margin:0 0 10px}
.bar{color:#8a8a8a;font-size:13px;margin-bottom:10px}
.m{background:#1c1c1c;border-radius:8px;padding:10px;margin-bottom:8px;border-left:4px solid #2c2c2c}
.m.on{border-left-color:#2ea043}.m.off{border-left-color:#7a1717;opacity:.65}
.mac{font-family:ui-monospace,monospace;font-size:13px}
.meta{color:#8a8a8a;font-size:12px;margin:3px 0 8px}
.row{display:flex;gap:8px;align-items:center;margin-top:6px;flex-wrap:wrap}
select,input[type=range],input[type=number]{background:#2a2a2a;color:#eee;border:1px solid #3a3a3a;border-radius:5px;padding:5px}
input[type=range]{padding:0;flex:1;min-width:120px}
input[type=number]{width:70px}
label{font-size:12px;color:#8a8a8a;min-width:52px}
.pill{font-size:11px;padding:2px 6px;border-radius:9px;background:#2c2c2c;color:#9a9a9a}
.empty{color:#8a8a8a;padding:20px 0;line-height:1.5}
</style></head><body>
<h1>TOGA Module (Fallback)</h1>
<div class="bar" id="bar">…</div>
<div id="list"></div>
<script>
let EFF=[],FOLLOW=255,touching=null;
const macid=m=>m.replace(/:/g,'');
function set(mac,q){fetch('/api/set?mac='+encodeURIComponent(mac)+'&'+q)}
async function tick(){
  let d; try{d=await(await fetch('/api/modules')).json()}catch(e){return}
  EFF=d.effects;FOLLOW=d.follow;
  document.getElementById('bar').textContent =
    'Kein Controller — Notmodus · Kanal '+d.ch+' · '+d.mods.length+' Module';
  const L=document.getElementById('list');
  if(!d.mods.length){L.innerHTML='<div class="empty">Noch kein Modul geh&ouml;rt.</div>';return}
  L.innerHTML=d.mods.map(m=>{
    const id=macid(m.mac);
    const opts=EFF.map((n,i)=>'<option value="'+i+'"'+(m.eff==i?' selected':'')+'>'+n+'</option>').join('')
      +'<option value="'+FOLLOW+'"'+(m.eff==FOLLOW?' selected':'')+'>folgt Grid</option>';
    return '<div class="m '+(m.online?'on':'off')+'">'+
      '<span class="mac">'+m.mac+'</span> '+
      '<span class="pill">'+(m.online?'online':('weg seit '+m.age+'s'))+'</span>'+
      '<div class="meta">'+m.w+'&times;'+m.h+' · Pin '+m.pin+' · Gruppe '+m.grp+
        ' · l&auml;uft: '+(EFF[m.effNow]||'?')+' · up '+m.up+'s</div>'+
      '<div class="row"><label>Effekt</label><select id="e'+id+'">'+opts+'</select></div>'+
      '<div class="row"><label>Gain</label><input type="range" min="10" max="200" value="'+m.gain+
        '" id="g'+id+'"><span id="gv'+id+'">'+m.gain+'%</span></div>'+
      '<div class="row"><label>Gap</label><input type="number" min="1" max="255" value="'+(m.gap||1)+
        '" id="gp'+id+'"><span class="pill">1 = jede LED, N = jede N-te</span></div>'+
      '<div class="row"><label>Boden</label><select id="fl'+id+'">'+
        '<option value="0"'+(m.floor==0?' selected':'')+'>Aus</option>'+
        '<option value="1"'+(m.floor==1?' selected':'')+'>Fluss &#9654;</option>'+
        '<option value="2"'+(m.floor==2?' selected':'')+'>Fluss &#9664;</option>'+
        '</select><span class="pill">Taste 5 l&auml;sst flie&szlig;en</span></div></div>';
  }).join('');
  d.mods.forEach(m=>{
    const id=macid(m.mac);
    const s=document.getElementById('e'+id);
    s.onfocus=()=>{touching=id};                       // dropdown open → pause the re-render
    s.onblur=()=>{if(touching===id)touching=null};
    s.onchange=e=>{set(m.mac,'effect='+e.target.value);touching=null};
    const g=document.getElementById('g'+id);
    g.oninput=e=>{document.getElementById('gv'+id).textContent=e.target.value+'%';touching=id};
    g.onchange=e=>{set(m.mac,'gain='+e.target.value);touching=null};
    const gp=document.getElementById('gp'+id);
    gp.onfocus=()=>{touching=id};                       // editing → pause the re-render
    gp.onblur=()=>{if(touching===id)touching=null};
    gp.onchange=e=>{set(m.mac,'gap='+e.target.value);touching=null};
    const fl=document.getElementById('fl'+id);
    fl.onfocus=()=>{touching=id};
    fl.onblur=()=>{if(touching===id)touching=null};
    fl.onchange=e=>{set(m.mac,'floor='+e.target.value);touching=null};
  });
}
tick();setInterval(()=>{if(!touching)tick()},1500);
</script></body></html>)HTML";
void handleRoot() { server.send_P(200, "text/html", PAGE); }

// ── Network state machine ───────────────────────────────
// Broadcast peer for fallback: channel 0 = "use the STA's current channel" (the
// phone hotspot's), so hellos reach the peers that joined the same hotspot.
void fallbackAddPeer() {
  esp_now_del_peer(TOGA_BCAST);
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, TOGA_BCAST, 6);
  p.channel = 0;
  p.ifidx   = WIFI_IF_STA;
  p.encrypt = false;
  esp_now_add_peer(&p);
}
void enterFallback() {
  gNetMode = NET_FALLBACK;
  Serial.printf("net: no controller in %lus — joining fallback hotspot '%s'\n",
                (unsigned long)(TOGA_FALLBACK_MS / 1000), gHotSsid);
  if (gHotSsid[0] == '\0') { Serial.println(F("net: NO hotspot SSID set — `hotspot <ssid> <pass>`")); }
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  if (gHotSsid[0]) WiFi.begin(gHotSsid, gHotPass);   // no channel → scan for the phone
  fallbackAddPeer();
}
// One shared URL for the whole hotspot: http://toga-mod.local/ — the same
// single page the controller serves, but with no controller present. A given
// mDNS name can belong to only ONE device, so the modules elect a host: the one
// with the lowest MAC among all it currently hears (itself included) claims
// `toga-mod` and answers there; everyone else keeps their unique `toga-mod-xxxxxx`
// name. Every module still serves the identical page on its own IP, so the URL
// is a convenience, not a single point of failure. Zero extra traffic — the
// election runs off the hellos already on the air.
#define TOGA_FALLBACK_HOST "toga-mod"

// Re-point our single mDNS name and re-advertise the page on it.
void mdnsAs(const char* name) {
  MDNS.end();
  MDNS.begin(name);
  MDNS.addService("http", "tcp", 80);
}
// Lowest online MAC in the table (self is seeded every tick) hosts toga-mod.local.
void electionTick(uint32_t t) {
  if (t - gFbConnAt < 2500) return;          // grace: hear the neighbours before claiming
  static uint32_t last = 0;
  if (t - last < 2000) return;
  last = t;
  const uint8_t* best = nullptr;
  for (int i = 0; i < MAX_MODULES; i++) {
    if (!mods[i].used || (t - mods[i].lastSeen) >= OFFLINE_MS) continue;   // online only
    if (!best || memcmp(mods[i].mac, best, 6) < 0) best = mods[i].mac;
  }
  bool amHost = best && memcmp(best, gMyMac, 6) == 0;
  if (amHost != gIsHost) {
    gIsHost = amHost;
    mdnsAs(amHost ? TOGA_FALLBACK_HOST : togaOtaModuleHost());
    Serial.printf("net: %s http://%s.local/\n",
                  amHost ? "HOSTING" : "released", amHost ? TOGA_FALLBACK_HOST : togaOtaModuleHost());
  }
}
void fallbackTick(uint32_t t) {
  static uint32_t lastTry = 0;
  static bool serverInit = false;
  if (WiFi.status() != WL_CONNECTED) {
    if (t - lastTry >= 10000) { lastTry = t; if (gHotSsid[0]) WiFi.begin(gHotSsid, gHotPass); }
    return;
  }
  if (!serverInit) {
    serverInit = true;
    gFbConnAt = t;
    Serial.printf("net: on hotspot '%s' ip %s — page at http://%s.local/ (once elected) or the IP\n",
                  gHotSsid, WiFi.localIP().toString().c_str(), TOGA_FALLBACK_HOST);
    mdnsAs(togaOtaModuleHost());        // start on our unique name; election may promote us
    server.on("/", handleRoot);
    server.on("/api/modules", handleModules);
    server.on("/api/set", handleSet);
    server.onNotFound([]() { server.send(404, "text/plain", "404"); });
    server.begin();
    fallbackAddPeer();                 // channel is settled now — pin the peer to it
  }
  server.handleClient();
  seedSelfRow(t);
  drainHellos(t);
  electionTick(t);                     // decide who owns toga-mod.local
}
void netTick() {
  uint32_t t = millis();
  switch (gNetMode) {
    case NET_CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        gNetMode = NET_CONTROLLER;
        Serial.printf("net: controller AP joined as %s\n", WiFi.localIP().toString().c_str());
      } else if (t - gBootMs >= TOGA_FALLBACK_MS) {
        enterFallback();
      } else {
        togaWifiTick();                // keep probing togacontroller on ch1
      }
      break;
    case NET_CONTROLLER:
      togaWifiTick();                  // rejoin if the AP blinks out
      break;
    case NET_FALLBACK:
      fallbackTick(t);
      break;
  }
}

void setup() {
  // First statement, before ANY other init: if we shut down for undervoltage,
  // go straight back to sleep. RTC RAM survives resets/brownouts, so only a real
  // power cycle (which clears it) ever brings us out — see module_battery.h.
  if (batteryDead) esp_deep_sleep_start();

  Serial.begin(115200);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_BLUE, 0);
  delay(100);

  loadConfig();

  // ADC + prime the IIR filter with one real reading, so battVolt is valid from
  // the first frame and the filter starts at the true level (no slow ramp-in).
  battSetupAdc();
  applyIIR((float)readBattMv());
  battVolt = battFiltered_mV / 1000.0f * 2.0f;
  if (!addLedsForPin(cfgPin)) {
    Serial.printf("BAD pin %u in NVS — falling back to 4 (allowed: 4,5,16,17,18,21)\n", cfgPin);
    cfgPin = 4;
    addLedsForPin(4);
  }
  FastLED.setBrightness(gBri);
  fill_solid(fx,   gNumLeds, CRGB::Black);
  // Blank the whole physical strip once at boot, not just the gNumLeds we drive.
  // FastLED only clocks out gNumLeds pixels, so if this module was just dropped
  // to a shorter strip (a LED-count change reboots to apply) the LEDs past the
  // new count keep their last latched colour and stay stuck lit. Drive
  // MODULE_BLANK_LEDS black once — covering both the count change and a plain
  // restart — then bind the controller back to the live length. Modules only:
  // the grid runs its own firmware and never shrinks a strip.
  fill_solid(leds, MAX_LEDS, CRGB::Black);
  if (gLedCtrl) {
    uint16_t blank = (MODULE_BLANK_LEDS < MAX_LEDS) ? MODULE_BLANK_LEDS : MAX_LEDS;
    gLedCtrl->setLeds(leds, blank);
    FastLED.show();
    gLedCtrl->setLeds(leds, gNumLeds);   // restore the live length for normal rendering
  } else {
    FastLED.show();
  }

  togaWifiBegin(togaOtaModuleHost());     // STA on togacontroller (ch1) — bus AND network
  gBootMs = millis();                      // netTick() falls back to the hotspot after TOGA_FALLBACK_MS
  // Our identity for the UI's per-module config and for OTA addressing. Must be
  // set BEFORE the callback goes live: it is what onReceive filters on, and a
  // zero MAC there would silently match nothing it should.
  WiFi.macAddress(gMyMac);
  // First-boot name: mod-<last 3 MAC bytes>, matching the OTA hostname. Persist it
  // so it survives and the web UI shows something human before anyone renames it.
  if (gName[0] == '\0') {
    snprintf(gName, sizeof(gName), "mod-%02x%02x%02x", gMyMac[3], gMyMac[4], gMyMac[5]);
    prefs.begin("module", false); prefs.putString("name", gName); prefs.end();
  }
  if (esp_now_init() != ESP_OK) { Serial.println(F("ESP-NOW init failed!")); while (1) delay(1000); }
  if (esp_now_register_recv_cb(onReceive) != ESP_OK) { Serial.println(F("recv_cb failed!")); while (1) delay(1000); }
  // We transmit exactly one thing — the hello — so we do need the broadcast peer.
  // A failure here is NOT fatal: it costs us the web UI, not the show.
  if (!togaAddPeer(TOGA_BCAST)) Serial.println(F("broadcast peer failed — no hello, module still runs"));
  togaOtaSetup(togaOtaModuleHost());     // flashable from now on, mid-show, no trigger

  Serial.println();
  Serial.printf("=== TOGA MODULE grp=%u %ux%u pin=%u eff=%s mac=%s ===\n",
                gGroup, gWidth, gHeight, cfgPin,
                gEffectPin == EFFECT_FOLLOW ? "follow" : effectName(gEffectPin),
                WiFi.macAddress().c_str());
  Serial.println(F("type 'help' for config commands"));
}

void loop() {
  serialPoll();                          // stays live in OTA mode: `reboot` is the escape hatch
  if (gRebootAt && (int32_t)(millis() - gRebootAt) >= 0) { Serial.println(F("rebooting…")); delay(50); ESP.restart(); }  // LED-count change

  // Above every early return below (the frame cap, and the strobe on every
  // single iteration while held) — a node you cannot flash mid-strobe is a node
  // you cannot flash exactly when you want to.
  togaOtaHandle();
  netTick();           // hunt for the controller; after 30s, fall back to the phone hotspot
                       // (and, once there, serve the module page — see fallbackTick)

  static uint32_t lastFrame = 0, lastAnimT = 0;
  static bool     strobeActive = false, strobeOn = false;
  static uint32_t strobeStart = 0;   // phase origin: the period is measured from here
  static float    gStepAcc = 0.0f, hueAcc = 0.0f;

  uint32_t t = millis();

  // ── Take the grid's latest absolute state ──
  if (syncPending) {
    TogaSyncMsg s; memcpy(&s, (const void*)&syncBuf, sizeof(s));
    syncPending = false;
    applySync(s);
    hueAcc = (float)gHueAuto;
  }

  // ── Settings for us from the web UI (buffered by the recv callback) ──
  if (cfgPending) {
    TogaModCfgMsg c; memcpy(&c, (const void*)&cfgBuf, sizeof(c));
    cfgPending = false;
    applyModCfg(c);
  }

  // ── Controller alive? (heartbeat within timeout) ──
  bool senderAlive = (espLastHeartbeat > 0) &&
                     ((t - (uint32_t)espLastHeartbeat) < (uint32_t)HEARTBEAT_TIMEOUT);
  // LED: solid = controller heartbeat alive; slow blink = fallback (phone hotspot);
  // off = looking for the controller.
  digitalWrite(LED_BLUE, gNetMode == NET_FALLBACK ? (int)((t / 250) % 2) : (senderAlive ? 1 : 0));

  // ── Announce ourselves so the web UI can list and address us ──
  sendHello(t, senderAlive);

  // Sits here, not at the end of loop(): the strobe path below returns early on
  // every iteration while held, and a module strobed through its first minute
  // would never commit and would roll back on the next reboot.
  togaOtaMarkValidWhenHealthy(t, senderAlive);

  // ── Battery: measure every 10s, filter, and (if enabled) confirm undervoltage
  //    before shutting down. Above the frame cap so it always runs; the ~16ms
  //    sampling burst is the only blocking here and happens once per 10s. ──
  if (t - lastBattCheck >= BATT_CHECK_MS) {
    lastBattCheck = t;
    uint32_t mv_raw  = readBattMv();
    float    mv_filt = applyIIR((float)mv_raw);
    battVolt = (mv_filt / 1000.0f) * 2.0f;         // exposed for the hello telemetry / web UI

    if (!battMonEn) {
      battLowCount = 0;                            // monitoring off → never trips
    } else if (mv_filt < BATT_THRESHOLD_MV) {      // compare the FILTERED value, never the raw
      battLowCount++;
      if (battLowCount >= BATT_CONFIRM) {
        // UVLO: cut the LED current and deep-sleep until a real power cycle.
        Serial.printf("Undervoltage (%.3f V) - deep sleep.\n", battVolt);
        Serial.flush();
        batteryDead = true;                        // RTC RAM: survives everything but power removal
        fill_solid(leds, gNumLeds, CRGB::Black);
        FastLED.show();
        delay(100);
        pinMode(cfgPin, INPUT);                    // tri-state the data pin → sources no current
        esp_deep_sleep_start();
      }
    } else {
      battLowCount = 0;                            // hysteresis: any good reading resets the counter
    }
  }

  // ── Strobe: momentary, driven by the controller (opt-out via follow mask) ──
  bool strobeHeld = senderAlive && espStrobe && (gFollowMask & TOGA_FOLLOW_STROBE);
  if (strobeHeld && !strobeActive) { strobeActive = true; strobeOn = false; strobeStart = t; }
  else if (!strobeHeld && strobeActive) {
    strobeActive = false; strobeOn = false;
    FastLED.clear(); FastLED.show();
  }

  // 50 FPS cap — but the strobe runs at full loop rate, else its timing is
  // quantised to 20ms and the flash can never be short.
  if (!strobeActive && t - lastFrame < FRAME_MS) return;
  lastFrame = t;

  // ── Identify: blink just the first two LEDs RED so you can physically find this
  //    strip (and see which end it starts at) — calm, not a room-filling white
  //    strobe. A slow ~1.1 Hz blink. Punches through every effect, strobe and
  //    blackout (you might be locating a dark one). Momentary; never saved.
  if (gIdentifyUntil) {
    if ((int32_t)(t - gIdentifyUntil) >= 0) { gIdentifyUntil = 0; }   // expired → back to normal
    else {
      FastLED.setBrightness(MAX_BRIGHTNESS);
      fill_solid(leds, gNumLeds, CRGB::Black);
      if ((t / 450) & 1) {                                  // slow blink, ON half of each cycle
        if (gNumLeds > 0) leds[0] = CRGB(255, 0, 0);
        if (gNumLeds > 1) leds[1] = CRGB(255, 0, 0);
      }
      FastLED.show();
      return;
    }
  }

  if (lastAnimT == 0) lastAnimT = t;
  float frameDt = (float)(t - lastAnimT);
  lastAnimT = t;

  // ── Beat-Glow: pulse in the grid's rhythm, on REAL time. The phase is
  //    snapped from every sync (and the grid has already folded its latency
  //    compensation into it), so we stay in step without redoing the maths.
  if ((gFollowMask & TOGA_FOLLOW_BEAT) && gBeatPeriod > 0.0f) {
    gBeatPhase += frameDt / gBeatPeriod;
    gBeatPhase -= floorf(gBeatPhase);
    gBeatGlow   = togaBeatFactor(gBeatPhase);   // same sawtooth as the grid
  } else gBeatGlow = 1.0f;                       // not slaving to the beat → steady

  // ── Auto-rotate the global hue (REAL time, same rate as the grid) ──
  hueAcc += togaHueRate(gHueSpeed) * frameDt;
  if (hueAcc >= 256.0f) hueAcc -= 256.0f;
  gHueAuto = (uint8_t)hueAcc;

  // ── 1. Strobe overrides everything ──
  if (strobeActive) {
    // Absolute grid, identical maths to the grid's — a module's show() takes
    // ~30us per LED, so chaining intervals made a 512px strip run visibly slower
    // than a 32px one. See togaStrobeOn() in <toga_proto.h>.
    bool want = togaStrobeOn(t, strobeStart, gStrobeGap);
    if (want != strobeOn) {
      strobeOn = want;
      CRGB sc = (gStrobeHue == 0) ? CRGB(255, 255, 255) : CRGB(CHSV(gStrobeHue, 255, 255));
      flashSolid(strobeOn ? sc : CRGB::Black, STROBE_BRI, STROBE_MAX_BRI);   // 70% ceiling
    }
    return;
  }

  // ── 2. Castle-strobe: the grid shows static CASTLE/2026 text, which a
  //    module has no font (and often only 8 rows) for. Flash the tint colour
  //    instead — the button is a room-wide moment, so sitting it out would
  //    break it, and the tint reads as distinct from the white strobe above.
  // Absolute-time gate (togaSquareOn) on the grid's synced on/off, NOT a chained
  // `flip = now + on`: the period cannot be stretched by this strip's FastLED.show(),
  // so a 32px module and the wall flash in lockstep. See togaSquareOn() / v6.
  static bool     csHeldPrev = false; static uint32_t csStart = 0;
  bool castleHeld = senderAlive && espCastleStrobe && (gFollowMask & TOGA_FOLLOW_STROBE);
  if (castleHeld && !csHeldPrev) csStart = t;   // phase origin: this press began here
  csHeldPrev = castleHeld;
  if (castleHeld) {
    bool csOn = togaSquareOn(t, csStart, gStrobeOnMs, gStrobeOffMs);
    flashSolid(csOn ? CRGB(CHSV((uint8_t)(gHueBase + gHueAuto), 255, 220)) : CRGB::Black, STROBE_BRI);
    return;
  }

  // ── 3. Mode-strobe: blink our effect on/off, brighter. Same meaning as on
  //    the grid — we do have a current effect, it just never changes.
  static bool     msHeldPrev = false; static uint32_t msStart = 0;
  bool modeStrobeHeld = senderAlive && espModeStrobe && (gFollowMask & TOGA_FOLLOW_STROBE);
  if (modeStrobeHeld && !msHeldPrev) msStart = t;   // phase origin: this press began here (see castle above)
  msHeldPrev = modeStrobeHeld;
  if (modeStrobeHeld) {
    bool msOn = togaSquareOn(t, msStart, gStrobeOnMs, gStrobeOffMs);
    if (!msOn) { FastLED.clear(); FastLED.show(); return; }
  }

  // ── 3b. Blackout: go dark like the wall. Strobe/castle already returned above
  //    and punch through; skipped while mode-strobe is held so it stays visible.
  //    Gated on senderAlive so a dead controller brings us back on our own.
  if (senderAlive && espBlackout && (gFollowMask & TOGA_FOLLOW_BLACKOUT) && !modeStrobeHeld) { FastLED.clear(); FastLED.show(); return; }

  // ── 4. Mode + show ──
  // Advance the virtual clock: Speed scales how fast gVirtMs moves, so it also
  // scales every mode's motion. The 4-step cap keeps a stall from spiralling.
  gStepAcc += frameDt * gAnimSpeed / (float)FRAME_MS;
  int steps = (int)gStepAcc;
  gStepAcc -= steps;
  if (steps > 4) { steps = 4; gStepAcc = 0.0f; }   // don't spiral after a stall
  for (int s = 0; s < steps; s++) {
    gVirtMs  += FRAME_MS;
    gTimebase = t - gVirtMs;                       // ⇒ (millis() - gTimebase) == gVirtMs
  }
  // ── 3c. Floor-strip flow: a module flagged as a floor strip (gFloorFlow != OFF)
  //    reacts to the controller's spot key (phys 5). Each PRESS launches one flow
  //    pass that scrolls this strip's CURRENT render (the grid mode, when slaving)
  //    once end-to-end in the flagged direction, over MOD_SPOT_PERIOD of VIRTUAL
  //    time (so Speed scales it, like every other motion). Non-floor modules ignore
  //    the key entirely — button 5 now only flows the floor strips. The scroll is
  //    applied in showModule() via gFlowShift; strobe/castle/blackout already
  //    returned above and take precedence.
  static bool     espSpotPrev = false;
  static uint32_t flowStartV  = 0;
  static bool     flowActive  = false;
  if (senderAlive && espSpot && !espSpotPrev && gFloorFlow != TOGA_FLOOR_OFF) {
    flowActive = true; flowStartV = gVirtMs;                        // one pass per press
  }
  espSpotPrev = espSpot;
  if (flowActive) {
    uint32_t el = gVirtMs - flowStartV;
    if (el >= MOD_SPOT_PERIOD || gFloorFlow == TOGA_FLOOR_OFF) { flowActive = false; gFlowShift = 0; }
    else {
      uint16_t n = gNumLeds ? gNumLeds : 1;
      uint16_t base = (uint16_t)(((uint32_t)el * n) / MOD_SPOT_PERIOD) % n;      // 0 → n over the pass
      gFlowShift = (gFloorFlow == TOGA_FLOOR_REV) ? (int16_t)((n - base) % n) : (int16_t)base;
    }
  } else gFlowShift = 0;

  // One procedural render per frame from the virtual clock. The 90 1D modes are
  // analytic in gVirtMs — Speed already lives in how fast it advances — so a single
  // call suffices. showModule() then scrolls the frame by gFlowShift (floor flow),
  // rotates it by the grid hue, and applies brightness/beat/gap.
  modRender(activeEffect(), gVirtMs, gWidth);
  showModule(modeStrobeHeld ? (uint8_t)min((int)gBri * 7 / 4, 255) : gBri);
}
