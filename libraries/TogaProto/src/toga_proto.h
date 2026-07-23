// ══════════════════════════════════════════════════════════════════════
//  toga_proto.h — the ESP-NOW wire format shared by controller, grid and
//  modules. This is the ONLY definition of the protocol; all three
//  sketches include it, so the format cannot drift between them.
//
//  Topology
//    Controller ──broadcast TogaCmdMsg (100ms heartbeat + commands 3x)──► Grid + Modules
//    Grid       ──broadcast TogaSyncMsg (200ms, absolute state)─────────► Controller + Modules
//    Modules    ──broadcast TogaHelloMsg (1/s, "I exist")───────────────► Webserver
//    Webserver  ──broadcast TogaModCfgMsg (on demand, addressed by MAC)─► one Module
//
//  The grid is the single source of truth. Commands are relative steps
//  (+1/-1), so ONLY the grid applies them; modules mirror the grid's
//  absolute state from TogaSyncMsg and therefore can never drift out of
//  sync, no matter how many packets they miss or how late they boot.
//
//  Modules used to transmit nothing at all. They now announce themselves once
//  a second — a report, never a command — because a UI cannot list devices it
//  never hears from. The invariant that matters is untouched: a module still
//  applies no steps of its own and still mirrors the grid wholesale.
//
//  CHANNEL — the thing that decides whether any of this works at all. An ESP32
//  has one radio, so joining a WLAN drags it onto that WLAN's channel. That is
//  why nothing here may go near a router: it would take the node off the bus
//  silently. The CONTROLLER therefore hosts the WLAN itself (togaApBegin) and
//  pins it to TOGA_CHANNEL — the channel the bus already runs on. Joining it
//  moves nobody, so every node is on the network AND on the bus at once.
//  That is what makes every node flashable mid-show (toga_ota.h), and it means
//  the installation needs no router anywhere.
// ══════════════════════════════════════════════════════════════════════
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

// The ESP-NOW receive callback changed shape in arduino-esp32 3.x
// (mac_addr -> esp_now_recv_info_t). Every receiver here uses the 2.x form,
// and the breakage would be silent, so refuse to build instead.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#error "Pinned to arduino-esp32 2.0.17 — the ESP-NOW recv callback signature differs on 3.x."
#endif

#define TOGA_MAGIC    0x4754u   // 'T','G'

// BUMP THIS whenever a wire struct changes shape or a message type is added.
// It is the only thing that keeps a node running yesterday's firmware from
// being understood by one running today's — and "understood" is the dangerous
// case, not "rejected". Version 1 outlived a rewrite of TogaCmdMsg (separate
// strobe bools -> flags+autoMode) and the arrival of HELLO/MODCFG, so a stale
// node's packet still passed validation and was applied as if it were current.
// That is a real fault, not a theoretical one: a module on old firmware cleared
// the grid's strobe flag mid-flash and blanked the controller's synced colours.
//
// The size check is NOT a backstop for this: TogaHelloMsg is 32 bytes, exactly
// like TogaSyncMsg, and TogaModCfgMsg is 16, exactly like TogaCmdMsg. Only
// `type` and this version separate them.
//
// Changing this requires reflashing EVERY node — same rule as TOGA_CHANNEL.
// v3: message type 5 came and went (see below); the structs are unchanged.
// v4: battery telemetry added to TogaHelloMsg (repurposed reserved bytes, size
//     unchanged) and a battMon toggle appended to TogaModCfgMsg (16 -> 18 bytes);
//     AND module effects went 9 -> 90 (a module now renders a dedicated mode per
//     grid mode). The structs are unchanged by the effects half, but effectPin/
//     effectNow values 9..89 now mean real modes — an unpatched module would read
//     them as >=9 and silently fall back to FOLLOW. Both halves land under v4.
#define TOGA_VERSION  4

// The channel the bus runs on AND the channel the controller's AP is pinned to.
// Those being the same number is the whole architecture: it is what lets a node
// be associated and on the bus at the same time. Changing it requires reflashing
// EVERY node at the same time.
#define TOGA_CHANNEL  1

// ── Groups: which nodes a command is addressed to ──
#define TOGA_GROUP_ALL      0   // everyone
#define TOGA_GROUP_GRID     1
#define TOGA_GROUP_MODULES  2

// ── Message types ──
#define TOGA_MSG_CMD    1       // controller -> broadcast
#define TOGA_MSG_SYNC   2       // grid -> broadcast (grid is the only sync source)
#define TOGA_MSG_HELLO  3       // module -> broadcast (1/s): "I exist, here is my config"
#define TOGA_MSG_MODCFG 4       // webserver -> broadcast: settings for ONE module, by MAC
// 5 was TOGA_MSG_OTA, back when OTA meant taking a node out of the show. The AP
// is on TOGA_CHANNEL now, so every node is permanently flashable without
// leaving the bus and nobody has to be asked. Don't reuse the id.

// ── Module modes (shared so the web UI's list cannot drift from reality) ──
// A module in FOLLOW mode renders one dedicated 1D mode per grid mode, so the
// count is the grid's mode count. The names below are the grid animation names
// at each index (labels only — a module's render is a citation, not a copy).
#define TOGA_EFFECT_COUNT  90
#define TOGA_EFFECT_OFF    200  // pinned blackout (replaces the old effect id 8 "off")
#define TOGA_EFFECT_FOLLOW 255  // not a pinned effect: derive it from the grid's mode
#define TOGA_CFG_KEEP      254  // TogaModCfgMsg: leave this field as it is

// PROGMEM-friendly on ESP32 (flash is memory-mapped; the table lives in .rodata,
// not RAM). Index 39 is plasmaLightning — the grid's anim_agentField is an alias
// for it.
static inline const char* togaEffectName(uint8_t e){
  static const char* const N[TOGA_EFFECT_COUNT] = {
    "setupGrid","heartbeat","binaryRain","tectonic","vortex","earthquake","dnaHelix",
    "supernova","blackHole","wormhole","mitosis","collider","shockwaveChain","pulsar",
    "liquidMetal","iceCrown","coriolisStorm","crystalFracture","aurora","lavaLamp",
    "nerveSystem","gravityLens","tidalWave","cymatics","starfieldWarp","darkMatter",
    "concentricSq","squareRain","diamondPulse","checkerboard","rotatingCube",
    "sunsetHorizon","depthMap","cosmicDust","heartglow","evolution","morph",
    "strangeAttract","harmonicReson","plasmaLightning","tetris","snake","iconShow",
    "bouncingLogo","starTunnel3D","rippleRain","spiralGalaxy","plasmaField",
    "breatheGradient","boids","fireworks","reactionDiff","textTOGA","textCASTLE",
    "text2026","stickman","paddleBall","smiley","eye","starfield2","warp","asteroids",
    "shootingStars","blackHole2","pulsar2","pulsarNebula","orionNebula","horsehead",
    "twinStars","planets","earthWindow","shipPanel","radar","sineWaves","rocketFire",
    "warningLight","aliens","spaceInvaders","rain2","levitate","confetti","spiralEject",
    "clouds3d","pulseHeart","rainUp","starsFlying","firework2","hStripesFlow",
    "vuColumns","vuGrid"
  };
  if(e < TOGA_EFFECT_COUNT)    return N[e];
  if(e == TOGA_EFFECT_OFF)     return "aus";
  if(e == TOGA_EFFECT_FOLLOW)  return "folgt Grid";
  return "?";
}

// ── Commands (TogaCmdMsg.cmd) ──
#define TOGA_CMD_HEARTBEAT 0    // carries only the strobe flags + liveness
#define TOGA_CMD_MODE      1    // arg = +1/-1 mode step   (grid only; modules ignore)
#define TOGA_CMD_PARAM     2    // arg = +1/-1 step of .target
#define TOGA_CMD_BEAT      3    // arg = beat period in ms, set directly (no tapping)
#define TOGA_CMD_BEAT_TAP  4    // one beat tap, sent the instant it happens; arg unused

// ── Param ids (TogaCmdMsg.target when cmd == TOGA_CMD_PARAM) ──
#define TOGA_P_BRI        0
#define TOGA_P_SPEED      1
#define TOGA_P_COLOR      2
#define TOGA_P_STROBEHUE  3
#define TOGA_P_HUESPEED   4
#define TOGA_P_STROBEGAP  5
#define TOGA_P_BRICOEF    6   // per-mode brightness factor (0..1), independent of the global master

// ── Momentary flags (TogaCmdMsg.flags), sent on EVERY packet incl. heartbeat ──
#define TOGA_F_STROBE        0x01   // button 15
#define TOGA_F_MODE_STROBE   0x02   // button 14
#define TOGA_F_CASTLE_STROBE 0x04   // button 13
// Blackout is a LATCH, not momentary: the controller toggles it on each press and
// holds the state, but it rides the flags the same way — on every packet, so a
// lost one costs at most 100ms. Adding a flag bit changes no struct shape and no
// message type, so it does NOT bump TOGA_VERSION (see the rule above): old firmware
// only ever masks 0x01/0x02/0x04 and never SETS 0x08, so an un-updated node simply
// ignores blackout and stays lit — graceful, never mis-understood.
#define TOGA_F_BLACKOUT      0x08   // free key (phys 6 / logical 10)

// ── Auto mood (TogaCmdMsg.autoMode), also on EVERY packet ──
// 0 = off, 1..TOGA_AUTO_COUNT = the mood the grid should cycle through.
// State, not an event: it rides along on the heartbeat like the flags do, so a
// lost packet costs at most 100ms and there is nothing to re-send or ack.
#define TOGA_AUTO_OFF     0
#define TOGA_AUTO_COUNT   4

// Each mood's OWN hue — what it looks like at Colour=0. Both callers add the
// grid's live tint (hueBase+hueAuto) on top, so the readout wears the colour
// the wall is actually wearing and the four moods stay apart while it turns.
//
// Shared rather than duplicated because two nodes name the same mood in colour:
// the controller's button 0 blinks it, and the grid flashes "A1".."A4" in it on
// every mood change. Two tables would drift, and the wall would then contradict
// the button about which mood is running.
//
// NOT a wire value — tuning these needs no TOGA_VERSION bump. But note the two
// ends render the hue through DIFFERENT maps (the trellis uses a classic
// 6-region HSV, the grid uses FastLED's rainbow), so these four are close but
// never pixel-identical. They were picked to land on the same four colours in
// both; re-check both ends when changing one.
//                                                     calm space party show
static const uint8_t TOGA_AUTO_HUE[TOGA_AUTO_COUNT] = { 140,  195,   10,  85 };

static const uint8_t TOGA_BCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// ══════════════════════════════════════════════════════════════════════
//  Shared behaviour constants.
//  These live here rather than in grid_config.h because grid and modules
//  must agree on them exactly. They are relative steps and clamps: a
//  mismatch would not show up as a constant offset but would compound
//  over a night of use.
// ══════════════════════════════════════════════════════════════════════
#define MAX_BRIGHTNESS   128   // hard cap (~50% — white/yellow stays safe)
#define BRI_DEFAULT       40   // global master default (raw, 1..MAX_BRIGHTNESS)
#define BRI_STEP           5   // +/- step when adjusting the global brightness

// ── Per-mode brightness factor (0..1 stored as 0..255 fixed-point) ──
// effective = globalBri * modeFactor/255, clamped to 1..MAX_BRIGHTNESS.
#define BRICOEF_MAX      255   // 1.0 — mode at full global brightness
#define BRICOEF_MIN       26   // ~0.1 floor so a mode never blacks out entirely
#define BRICOEF_STEP      13   // +/- step (~5%)

// ── Controller pad brightness factor (its own dimming, 0..255) ──
// The controller scales its NeoTrellis LEDs by the grid's globalBri AND this.
#define CTRL_BRI_MAX     255
#define CTRL_BRI_MIN      20   // keep the pad readable at its dimmest
#define CTRL_BRI_STEP     13
#define CTRL_BRI_DEFAULT 120   // comfortably dim out of the box

// ── Beat-Glow (tap-tempo pulse laid over the animations) ──
// Shape is a SAWTOOTH: full brightness exactly ON the beat, then a linear fall
// to BEAT_MIN until the next one. Deliberately not a sine — a sine ramps back
// up *before* the beat, which reads as rushing the music.
#define BEAT_MIN        0.25f  // dimmest point, reached just before the next beat
#define BEAT_MIN_MS      200   // fastest accepted beat (300 BPM)
#define BEAT_MAX_MS     2000   // slowest accepted beat (30 BPM)

// Latency compensation: the peak is pulled this many ms EARLIER than the beat,
// predicted forward over the locked period. Covers the whole chain the tap does
// not — trellis poll (0-20ms), ESP-NOW, the grid's 50fps frame, LED refresh.
// Raise it if the flash still feels late, lower it if it now feels early.
#define BEAT_LATENCY_MS   70

// The brightness factor for a position within the beat. phase 0.0 = on the
// beat (1.0), phase 1.0 = just before the next (BEAT_MIN). Grid and modules
// both call this, so their pulses cannot take on different shapes.
static inline float togaBeatFactor(float phase){
  if(phase < 0.0f) phase = 0.0f; else if(phase > 1.0f) phase = 1.0f;
  return 1.0f - (1.0f - BEAT_MIN) * phase;
}

// ── Hue auto-rotation (Hue-Speed knob, button 5) ──
// The knob is 0..HUE_SPEED_MAX; the SCALE IS GEOMETRIC, not linear: one press
// changes the rotation speed by ~8%, everywhere on the dial. It used to be a
// linear 0..40, and that dial was unusable — its whole range lived between 13s
// and 0.3s per revolution, so the very first step already had the wall spinning
// and a slow drift could not be dialled at all. Same top speed as before, but
// the bottom half of the dial now covers the minutes-per-revolution range.
#define HUE_SPEED_MAX   100
#define HUE_RATE_MIN    0.000427f  // knob 1:   hue units/ms → full circle ~10 min
#define HUE_RATE_MAX    0.8f       // knob 100: full circle ~0.3s (the old maximum)
#define HUE_RATE_K      0.07611f   // ln(HUE_RATE_MAX/HUE_RATE_MIN)/(HUE_SPEED_MAX-1)

// Hue units per millisecond for a knob value; 0 = no rotation. Grid and modules
// both rotate on their own clock and call this, so a mismatch here would not
// look like an offset — their colours would slowly walk apart all night.
static inline float togaHueRate(uint8_t s){
  if(s == 0) return 0.0f;
  if(s > HUE_SPEED_MAX) s = HUE_SPEED_MAX;
  return HUE_RATE_MIN * expf(HUE_RATE_K * (float)(s - 1));
}

// Strobe: flash as short as the LED refresh allows, gap is user-tunable.
// STROBE_FLASH_MS + gap IS the period, exactly — see togaStrobeOn().
#define STROBE_FLASH_MS   10

// Is the strobe lit at time `t`? The flash sits at the start of every period,
// measured from `start` on an ABSOLUTE grid.
//
// It used to chain intervals instead (`next = now + flash`, then `next = now +
// gap`), and that quietly made the FRAME RATE depend on the STRIP LENGTH: each
// flip was scheduled only after FastLED.show() returned, so the show duration —
// ~30us per LED, i.e. ~1ms on a 32px module but ~15ms on a 512px one — was
// added to every single cycle. Long strips ran measurably slower than short
// ones, and nothing pulled them back.
//
// Modulo of absolute time cannot drift and cannot be stretched by a slow show():
// a node that misses a window catches the next one on the correct beat. The
// period is then a property of the setting alone, identical on every node.
// (What show() still owns is the DUTY CYCLE: a strip that needs 15ms to clock
// out cannot be lit for only 10. That is the wire, not the schedule.)
static inline bool togaStrobeOn(uint32_t t, uint32_t start, uint16_t gapMs){
  uint32_t period = (uint32_t)STROBE_FLASH_MS + gapMs;
  if(period == 0) return false;                  // defensive: never divide by zero
  return ((uint32_t)(t - start) % period) < (uint32_t)STROBE_FLASH_MS;
}

// Button-15 strobe: hard ceiling on the flash brightness, 70% of full scale.
// The flash is the brightest thing the installation ever does — every pixel at
// once, usually white — so it does not get the full 255 that its shortness
// would otherwise allow. Shared, and applied AFTER a module's gain trim: the
// point is a ceiling on what the wall emits, and a module at gain 200% would
// walk straight through a cap applied any earlier.
// Does NOT apply to the castle strobe (13) or the mode strobe (14).
#define STROBE_MAX_BRI   178
#define STROBE_GAP_DEF    90
#define STROBE_GAP_MIN    30
#define STROBE_GAP_MAX   250
#define STROBE_GAP_STEP    5

#define HEARTBEAT_TIMEOUT 400  // ms; sender considered gone after this

// ══════════════════════════════════════════════════════════════════════
//  Wire structs.
//
//  NOT __attribute__((packed)): both ends are the same GCC / xtensa 32-bit
//  LE ABI, so padding is only a hazard if it goes unverified. Explicit
//  `reserved` fields mean there is no implicit padding at all, and the
//  static_asserts below fail the build loudly if the layout ever drifts.
//  packed would buy portability we don't need while forcing byte-wise
//  access and making &m.arg undefined behaviour.
// ══════════════════════════════════════════════════════════════════════
struct TogaHeader {
  uint16_t magic;      //  0  TOGA_MAGIC
  uint8_t  version;    //  2  TOGA_VERSION
  uint8_t  type;       //  3  TOGA_MSG_*
  uint8_t  group;      //  4  TOGA_GROUP_* (0 = everyone)
  uint8_t  reserved;   //  5  must be 0
  uint16_t seq;        //  6  all repeats of one logical command share a seq
};
static_assert(sizeof(TogaHeader) == 8, "TogaHeader layout drifted");

struct TogaCmdMsg {
  TogaHeader h;        //  0
  uint8_t  cmd;        //  8  TOGA_CMD_*
  uint8_t  target;     //  9  TOGA_P_*  (when cmd == TOGA_CMD_PARAM)
  uint8_t  flags;      // 10  TOGA_F_*  (on every packet, heartbeat included)
  uint8_t  autoMode;   // 11  0 = off, 1..TOGA_AUTO_COUNT (every packet, like flags)
  int16_t  arg;        // 12
  uint16_t reserved2;  // 14  must be 0
};
static_assert(sizeof(TogaCmdMsg) == 16,             "TogaCmdMsg layout drifted");
static_assert(offsetof(TogaCmdMsg, arg) == 12,      "TogaCmdMsg layout drifted");
static_assert(offsetof(TogaCmdMsg, autoMode) == 11, "TogaCmdMsg layout drifted");

// The grid's complete absolute state. Modules mirror this instead of
// replaying steps, which is what makes drift structurally impossible.
struct TogaSyncMsg {
  TogaHeader h;                 //  0
  float    animSpeed;           //  8  0.1..4.0 (4-aligned by construction)
  int16_t  animIndex;           // 12  narrowed from int: no `int` on the wire
  uint8_t  brightness;          // 14  1..MAX_BRIGHTNESS
  uint8_t  hueBase;             // 15  Color knob
  uint8_t  hueAuto;             // 16  grid's live auto-rotation offset
  uint8_t  hueSpeed;            // 17  0..HUE_SPEED_MAX (geometric — see togaHueRate)
  uint8_t  strobeHue;           // 18  0 = white
  uint8_t  strobeGapMs;         // 19
  uint16_t beatMs;              // 20  0 = glow off
  uint8_t  beatPhase;           // 22  grid's phase, 0..255 == 0.0..1.0
  uint8_t  tintR, tintG, tintB; // 23  exact current global-tint colour
  uint8_t  sColR, sColG, sColB; // 26  exact current strobe colour
  uint8_t  globalBrightness;    // 29  the master 1..MAX_BRIGHTNESS (controller scales its pad by this)
  uint8_t  reserved3[2];        // 30  must be 0
};
static_assert(sizeof(TogaSyncMsg) == 32,             "TogaSyncMsg layout drifted");
static_assert(offsetof(TogaSyncMsg, animSpeed) == 8, "TogaSyncMsg layout drifted");
static_assert(offsetof(TogaSyncMsg, beatMs) == 20,   "TogaSyncMsg layout drifted");
// A module announcing itself, ~1/s. This is the ONE thing a module transmits,
// and it is deliberately not an exception to "modules never apply steps": it
// reports, it never commands. Nothing depends on it — miss every hello and the
// installation runs on; only the web UI's list goes stale. That is why it is
// fire-and-forget with no ack and no retry.
struct TogaHelloMsg {
  TogaHeader h;                 //  0
  uint8_t  mac[6];              //  8  our STA MAC — the identity the UI addresses
  uint8_t  group;               // 14
  uint8_t  effectPin;           // 15  TOGA_EFFECT_FOLLOW or the pinned effect id
  uint8_t  effectNow;           // 16  what is actually on screen right now
  uint8_t  gain;                // 17  10..200 %
  uint8_t  pin;                 // 18  LED data pin
  uint8_t  linkAlive;           // 19  1 = THIS module hears the controller
  uint16_t width, height;       // 20
  uint32_t uptimeS;             // 24  spots a module that is silently rebooting
  uint16_t battMv;              // 28  cell voltage in mV, 0 = unknown/none (v4)
  uint8_t  battFlags;           // 30  bit0 = monitoring enabled, bit1 = undervoltage pending (v4)
  uint8_t  reserved;            // 31  must be 0
};
static_assert(sizeof(TogaHelloMsg) == 32,           "TogaHelloMsg layout drifted");
static_assert(offsetof(TogaHelloMsg, mac) == 8,     "TogaHelloMsg layout drifted");
static_assert(offsetof(TogaHelloMsg, width) == 20,  "TogaHelloMsg layout drifted");
static_assert(offsetof(TogaHelloMsg, battMv) == 28, "TogaHelloMsg layout drifted");

// ── Battery flags (TogaHelloMsg.battFlags) ──
#define TOGA_BATT_F_ENABLED 0x01
#define TOGA_BATT_F_LOW     0x02

// Settings for exactly ONE module. Broadcast like everything else — the module
// whose MAC matches applies it, the rest drop it. Addressing by MAC rather than
// adding a unicast peer per module keeps the radio path identical for every
// packet in the system, and the MAC is what the module already announces.
struct TogaModCfgMsg {
  TogaHeader h;                 //  0
  uint8_t  mac[6];              //  8  target module
  uint8_t  effectPin;           // 14  effect id, TOGA_EFFECT_FOLLOW, or TOGA_CFG_KEEP
  uint8_t  gain;                // 15  10..200, or 0 = keep
  uint8_t  battMon;             // 16  TOGA_CFG_KEEP = leave, 0 = disable, 1 = enable (v4)
  uint8_t  reserved2;           // 17  must be 0
};
static_assert(sizeof(TogaModCfgMsg) == 18,       "TogaModCfgMsg layout drifted");
static_assert(offsetof(TogaModCfgMsg, mac) == 8, "TogaModCfgMsg layout drifted");

static_assert(sizeof(TogaCmdMsg)   <= ESP_NOW_MAX_DATA_LEN, "cmd too big");
static_assert(sizeof(TogaSyncMsg)  <= ESP_NOW_MAX_DATA_LEN, "sync too big");
static_assert(sizeof(TogaHelloMsg) <= ESP_NOW_MAX_DATA_LEN, "hello too big");
static_assert(sizeof(TogaModCfgMsg)<= ESP_NOW_MAX_DATA_LEN, "modcfg too big");

// ══════════════════════════════════════════════════════════════════════
//  Helpers. The parse helpers run in the WiFi task from the receive
//  callback: keep them short, no Serial, no allocation.
// ══════════════════════════════════════════════════════════════════════
static inline void togaInitHeader(TogaHeader& h, uint8_t type, uint8_t group, uint16_t seq){
  h.magic = TOGA_MAGIC; h.version = TOGA_VERSION;
  h.type = type; h.group = group; h.reserved = 0; h.seq = seq;
}

// Magic + version + type + exact length. Replaces every bare `len != sizeof(...)`.
static inline bool togaCheck(const uint8_t* data, int len, uint8_t type, size_t expectLen){
  if(data == nullptr || len != (int)expectLen) return false;
  TogaHeader h; memcpy(&h, data, sizeof(h));   // memcpy: `data` is not guaranteed aligned
  return h.magic == TOGA_MAGIC && h.version == TOGA_VERSION && h.type == type;
}

// Is this command addressed to me? group 0 means everyone.
static inline bool togaParseCmd(const uint8_t* data, int len, uint8_t myGroup, TogaCmdMsg* out){
  if(!togaCheck(data, len, TOGA_MSG_CMD, sizeof(TogaCmdMsg))) return false;
  memcpy(out, data, sizeof(*out));
  if(out->h.group != TOGA_GROUP_ALL && out->h.group != myGroup) return false;
  return true;
}

static inline bool togaParseSync(const uint8_t* data, int len, TogaSyncMsg* out){
  if(!togaCheck(data, len, TOGA_MSG_SYNC, sizeof(TogaSyncMsg))) return false;
  memcpy(out, data, sizeof(*out));
  return true;
}

static inline bool togaParseHello(const uint8_t* data, int len, TogaHelloMsg* out){
  if(!togaCheck(data, len, TOGA_MSG_HELLO, sizeof(TogaHelloMsg))) return false;
  memcpy(out, data, sizeof(*out));
  return true;
}

// Only the addressed module parses this one — the MAC check is the whole point,
// so it lives in here rather than in every caller.
static inline bool togaParseModCfg(const uint8_t* data, int len, const uint8_t myMac[6], TogaModCfgMsg* out){
  if(!togaCheck(data, len, TOGA_MSG_MODCFG, sizeof(TogaModCfgMsg))) return false;
  memcpy(out, data, sizeof(*out));
  return memcmp(out->mac, myMac, 6) == 0;
}

// ══════════════════════════════════════════════════════════════════════
//  The WLAN. The CONTROLLER hosts it; everyone else joins it.
//
//  THE WHOLE TRICK IS THE CHANNEL. An ESP32 has one radio, so joining a WLAN
//  drags it onto that WLAN's channel — which is why nothing here was ever
//  allowed near a router: it would silently take the node off the ESP-NOW bus.
//  But this AP is OURS and we pin it to TOGA_CHANNEL, the very channel the bus
//  already runs on. Associating therefore moves nobody anywhere, and every node
//  can be on the network AND on the bus at the same time.
//
//  That is what makes OTA free: no trigger, no leaving the show (see toga_ota.h).
//  It also means the installation needs no router at all — plug it in anywhere.
// ══════════════════════════════════════════════════════════════════════
#define TOGA_AP_SSID  "togalights"
#define TOGA_AP_PASS  "andmagic"     // >= 8 chars, else softAP() silently opens the AP

// CONTROLLER ONLY. Call BEFORE esp_now_init().
//
// AP_STA, not AP: ESP-NOW peers are registered on WIFI_IF_STA fleet-wide
// (togaAddPeer), so the STA interface has to exist. It never associates — it
// just rides along on the AP's channel, which is the one we want anyway.
static inline void togaApBegin(){
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);                       // an AP that sleeps drops the bus
  // The channel argument is the load-bearing one: it pins the radio to
  // TOGA_CHANNEL, which is what lets the whole fleet stay on the bus while
  // associated. max_connection 8: grid + webserver + N modules + a laptop.
  WiFi.softAP(TOGA_AP_SSID, TOGA_AP_PASS, TOGA_CHANNEL, 0 /*not hidden*/, 8);
}

// EVERYONE ELSE. Call BEFORE esp_now_init().
//
// hostname is how this board is identified on the network and by OTA — it must
// be unique per board (see togaOtaHost* in toga_ota.h).
static inline void togaWifiBegin(const char* hostname){
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);       // broadcast is never retried — don't sleep through it
  WiFi.setHostname(hostname);
  WiFi.setAutoReconnect(true);
  // Passing the channel explicitly is NOT an optimisation, it is the invariant:
  // without it WiFi.begin() scans every channel looking for the SSID, and a
  // radio hopping across 13 channels is deaf to the bus for most of a second —
  // every second, forever, if the controller happens to be off. With it, the
  // STA only ever probes TOGA_CHANNEL, so a missing AP costs us nothing at all.
  WiFi.begin(TOGA_AP_SSID, TOGA_AP_PASS, TOGA_CHANNEL);
  // Deliberately NOT waiting for the association: ESP-NOW does not need it, and
  // the show must start whether or not the controller is up yet.
}

// Call once per loop() on every node that joined (not the controller).
//
// setAutoReconnect() is NOT enough, and this is the trap: it only reconnects
// after a connection that once succeeded. The first WiFi.begin() at boot
// routinely fails — the controller may still be booting, or being flashed, or
// simply switched on later — and nothing ever retries. The node then runs the
// whole night on the bus, perfectly happy, and is silently unflashable, which
// is the one thing you would never notice until you needed it.
//
// Retrying costs nothing: WiFi.begin() with an explicit channel does not scan.
static inline void togaWifiTick(){
  static uint32_t lastTry = 0;
  static bool wasUp = false;
  uint32_t t = millis();
  bool up = (WiFi.status() == WL_CONNECTED);

  if(up != wasUp){                       // report the edges, not the state: a console, not a log
    wasUp = up;
    if(up) Serial.printf("wifi: %s as %s\n", WiFi.localIP().toString().c_str(), WiFi.getHostname());
    else   Serial.println(F("wifi: lost the AP — still on the bus, retrying"));
  }
  if(up) return;
  if(t - lastTry < 10000) return;        // 10s: slow enough to be free, fast enough to be there
  lastTry = t;
  WiFi.begin(TOGA_AP_SSID, TOGA_AP_PASS, TOGA_CHANNEL);
}

// Call AFTER esp_now_init(). Idempotent.
// Receiving broadcast needs no peer — only sending does.
static inline bool togaAddPeer(const uint8_t mac[6]){
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, mac, 6);
  p.channel = TOGA_CHANNEL;   // invariant: == the channel the STA is parked on
  p.ifidx   = WIFI_IF_STA;
  p.encrypt = false;          // mandatory: ESP-NOW cannot encrypt broadcast
  esp_err_t e = esp_now_add_peer(&p);
  return e == ESP_OK || e == ESP_ERR_ESPNOW_EXIST;
}
