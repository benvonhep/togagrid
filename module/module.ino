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

// ── State mirrored from the grid's sync ──
uint8_t  gBri        = BRI_DEFAULT;
float    gAnimSpeed  = 1.0f;
uint8_t  gHueBase = 0, gHueAuto = 0, gHueSpeed = 0, gStrobeHue = 0;
uint8_t  gStrobeGap  = STROBE_GAP_DEF;
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
}

// The grid's absolute truth replaces ours wholesale. No clamping needed —
// the grid already clamped, and re-clamping here could only introduce drift.
void applySync(const TogaSyncMsg& s) {
  gBri        = s.brightness;
  gAnimSpeed  = s.animSpeed;
  gHueBase    = s.hueBase;
  gHueAuto    = s.hueAuto;      // snap: both sides rotate at the same rate
  gHueSpeed   = s.hueSpeed;
  gStrobeHue  = s.strobeHue;
  gStrobeGap  = s.strobeGapMs;
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
  prefs.end();

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
  prefs.end();
  return true;
}

// PIN is a template parameter in FastLED — there is no runtime-pin API — so
// every allowed pin needs its own instantiation. addLeds must be called
// exactly once: FastLED.show() drives every registered controller.
static bool addLedsForPin(uint8_t pin) {
  switch (pin) {
    case  4: FastLED.addLeds<LED_TYPE,  4, COLOR_ORDER>(leds, gNumLeds); break;
    case  5: FastLED.addLeds<LED_TYPE,  5, COLOR_ORDER>(leds, gNumLeds); break;
    case 16: FastLED.addLeds<LED_TYPE, 16, COLOR_ORDER>(leds, gNumLeds); break;
    case 17: FastLED.addLeds<LED_TYPE, 17, COLOR_ORDER>(leds, gNumLeds); break;
    case 18: FastLED.addLeds<LED_TYPE, 18, COLOR_ORDER>(leds, gNumLeds); break;
    case 21: FastLED.addLeds<LED_TYPE, 21, COLOR_ORDER>(leds, gNumLeds); break;
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
  bool dirty = false;
  if (c.effectPin != TOGA_CFG_KEEP &&
      (c.effectPin < 90 || c.effectPin == TOGA_EFFECT_OFF || c.effectPin == TOGA_EFFECT_FOLLOW)) {
    gEffectPin = c.effectPin; dirty = true;
  }
  if (c.gain != 0) { gGain = constrain((int)c.gain, 10, 200); dirty = true; }
  if (c.battMon != TOGA_CFG_KEEP) { battMonEn = c.battMon ? 1 : 0; dirty = true; }
  if (!dirty) return;                       // nothing valid in it → no flash write
  prefs.begin("module", false);
  prefs.putUChar("eff",  gEffectPin);
  prefs.putUChar("gain", gGain);
  prefs.putUChar("bmon", battMonEn);
  prefs.end();
  printStatus();                            // the serial console stays honest too
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
  m.uptimeS   = t / 1000;
  m.battMv    = (uint16_t)lroundf(battVolt * 1000.0f);
  m.battFlags = (battMonEn ? TOGA_BATT_F_ENABLED : 0) | (battLowCount > 0 ? TOGA_BATT_F_LOW : 0);
  esp_now_send(TOGA_BCAST, (const uint8_t*)&m, sizeof(m));
}

void printStatus() {
  bool alive = (espLastHeartbeat > 0) && ((millis() - (uint32_t)espLastHeartbeat) < HEARTBEAT_TIMEOUT);
  Serial.println(F("─── TOGA MODULE ───"));
  Serial.printf("mac      %s  ch=%d\n", WiFi.macAddress().c_str(), TOGA_CHANNEL);
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
  Serial.printf("battery  %.2f V  mon=%s  (cutoff %.2f V)\n",
                battVolt, battMonEn ? "on" : "off", BATT_THRESHOLD_MV / 1000.0f * 2.0f);
  Serial.printf("link     %s\n", alive ? "alive" : "NO CONTROLLER");
  Serial.printf("from grid: bri=%u speed=%.1f hue=%u/%u hspd=%u shue=%u gap=%u beat=%.0fms\n",
                gBri, gAnimSpeed, gHueBase, gHueAuto, gHueSpeed, gStrobeHue, gStrobeGap, gBeatPeriod);
  Serial.println(F("cmds: group|w|h|leds|pin|effect|gain|bmon <n> · follow · show · save · reboot"));
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
    else if (!strcmp(cmd, "gain")   && got == 2) { gGain = constrain((int)val, 10, 200); printStatus(); }
    else if (!strcmp(cmd, "bmon")   && got == 2) {   // battery undervoltage lockout on/off
      battMonEn = val ? 1 : 0;
      prefs.begin("module", false); prefs.putUChar("bmon", battMonEn); prefs.end();   // persist now (safety config)
      printStatus();
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
void showModule(uint8_t baseBri) {
  uint8_t off = (uint8_t)(gHueBase + gHueAuto);
  if (off) {
    for (uint16_t i = 0; i < gNumLeds; i++) {
      CRGB c = fx[i];
      if (c.r | c.g | c.b) { CHSV h = rgb2hsv_approximate(c); h.hue += off; c = h; }
      leds[i] = c;
    }
  } else {
    memcpy(leds, fx, gNumLeds * sizeof(CRGB));
  }
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
  fill_solid(leds, gNumLeds, CRGB::Black);
  FastLED.show();

  togaWifiBegin(togaOtaModuleHost());     // STA on togalights (ch1) — bus AND network
  // Our identity for the UI's per-module config and for OTA addressing. Must be
  // set BEFORE the callback goes live: it is what onReceive filters on, and a
  // zero MAC there would silently match nothing it should.
  WiFi.macAddress(gMyMac);
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

  // Above every early return below (the frame cap, and the strobe on every
  // single iteration while held) — a node you cannot flash mid-strobe is a node
  // you cannot flash exactly when you want to.
  togaOtaHandle();
  togaWifiTick();      // the AP may come up after us, or come back

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
  digitalWrite(LED_BLUE, senderAlive);

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

  // ── Strobe: momentary, driven by the controller ──
  bool strobeHeld = senderAlive && espStrobe;
  if (strobeHeld && !strobeActive) { strobeActive = true; strobeOn = false; strobeStart = t; }
  else if (!strobeHeld && strobeActive) {
    strobeActive = false; strobeOn = false;
    FastLED.clear(); FastLED.show();
  }

  // 50 FPS cap — but the strobe runs at full loop rate, else its timing is
  // quantised to 20ms and the flash can never be short.
  if (!strobeActive && t - lastFrame < FRAME_MS) return;
  lastFrame = t;

  if (lastAnimT == 0) lastAnimT = t;
  float frameDt = (float)(t - lastAnimT);
  lastAnimT = t;

  // ── Beat-Glow: pulse in the grid's rhythm, on REAL time. The phase is
  //    snapped from every sync (and the grid has already folded its latency
  //    compensation into it), so we stay in step without redoing the maths.
  if (gBeatPeriod > 0.0f) {
    gBeatPhase += frameDt / gBeatPeriod;
    gBeatPhase -= floorf(gBeatPhase);
    gBeatGlow   = togaBeatFactor(gBeatPhase);   // same sawtooth as the grid
  } else gBeatGlow = 1.0f;

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
  if (senderAlive && espCastleStrobe) {
    static bool csOn = true; static uint32_t csFlip = 0;
    if ((int32_t)(t - csFlip) >= 0) {
      csOn = !csOn;
      csFlip = t + (uint32_t)(csOn ? CASTLE_ON_MS : CASTLE_OFF_MS);
    }
    flashSolid(csOn ? CRGB(CHSV((uint8_t)(gHueBase + gHueAuto), 255, 220)) : CRGB::Black, STROBE_BRI);
    return;
  }

  // ── 3. Mode-strobe: blink our effect on/off, brighter. Same meaning as on
  //    the grid — we do have a current effect, it just never changes.
  bool modeStrobeHeld = senderAlive && espModeStrobe;
  if (modeStrobeHeld) {
    static bool msOn = true; static uint32_t msFlip = 0;
    if ((int32_t)(t - msFlip) >= 0) {
      msOn = !msOn;
      msFlip = t + (uint32_t)(msOn ? MODE_ON_MS : MODE_OFF_MS);
    }
    if (!msOn) { FastLED.clear(); FastLED.show(); return; }
  }

  // ── 3b. Blackout: go dark like the wall. Strobe/castle already returned above
  //    and punch through; skipped while mode-strobe is held so it stays visible.
  //    Gated on senderAlive so a dead controller brings us back on our own.
  if (senderAlive && espBlackout && !modeStrobeHeld) { FastLED.clear(); FastLED.show(); return; }

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
  // One procedural render per frame from the virtual clock. Unlike the old
  // incremental effects (which needed N calls per frame for speed), the 90 1D
  // modes are analytic in gVirtMs — speed already lives in how fast it advances,
  // so a single call keeps the frame budget AND the animation identical. When
  // steps == 0 (Speed 0.1) gVirtMs holds, so re-rendering is a free no-op and
  // the beat glow / hue rotation (real time, in showModule) still refresh.
  modRender(activeEffect(), gVirtMs, gWidth);
  showModule(modeStrobeHeld ? (uint8_t)min((int)gBri * 7 / 4, 255) : gBri);
}
