# TOGAGRID ("Toga Lights") — FULL REPRODUCTION SPECIFICATION

> **Purpose of this document.** This is a complete, reproduction-grade specification of the entire `togagrid` / "Toga Lights" installation, written so that another AI, given only this text, can rebuild the whole system to behave identically. It covers every piece of hardware, the exact wiring/pinout, the toolchain and build/flash procedure, the complete ESP-NOW wire protocol (verbatim), all four firmwares, and all 90 grid animations with their exact per-frame math and constants.
>
> **Honest caveat on exactness.** The protocol, configs, fonts, effect tables, and key engine code below are reproduced **verbatim** — those reproduce byte-identically. The 90 procedural grid animations are specified **algorithmically** (every constant, formula, spawn rule, and color computation): reimplementing them reproduces the *same visual behaviour*, but pixel-for-pixel identity of a procedural animation is only guaranteed by the original C++ source. Where that matters, the animation is described precisely enough to re-derive it, and the original file is named.

---

# PART 0 — SYSTEM OVERVIEW

**togagrid / "Toga Lights"** is a wireless ESP32 LED art installation for a **"CASTLE / 2026"** themed event. It runs on a private WiFi with no router and no internet. Four node *types* communicate over **ESP-NOW broadcast** on **WiFi channel 1**:

```
Controller ──broadcast TogaCmdMsg (100ms heartbeat + commands 3x)──► Grid + Modules
Grid       ──broadcast TogaSyncMsg (200ms, absolute state)─────────► Controller + Modules
Modules    ──broadcast TogaHelloMsg (1/s, "I exist")───────────────► Webserver
Webserver  ──broadcast TogaModCfgMsg (on demand, addressed by MAC)─► one Module
```

- **1× Controller** — an Adafruit NeoTrellis 4×4 (16 RGB keys) handheld pad. The only human interface. **It also hosts the WiFi AP** (`togalights`) on channel 1.
- **1× Grid** — a large woven LED wall (12 X-strips + 12 Y-strips of WS2812B). The **single source of truth**: runs 90 animations, applies commands, broadcasts absolute state.
- **N× Modules** — satellite ESP32 boards, each driving its own WS2812B strip/panel. Pure followers, mirror the grid.
- **1× Webserver** (optional) — a listen-only ESP32 serving a phone web UI to configure the modules. Drives no LEDs.

**Core invariant:** commands are *relative steps* (mode +1, brightness −5…). **Only the grid applies them.** The grid then broadcasts its **absolute** state; everyone else mirrors it, so drift is structurally impossible — a node that misses packets or boots late re-locks within ~200 ms.

**Why the controller hosts the AP:** an ESP32 has one radio; joining a WLAN drags it to that WLAN's channel. So no node may join a router. The controller runs the AP pinned to channel 1 = the ESP-NOW bus channel, so every node is simultaneously associated *and* on the bus → every node is OTA-flashable mid-show, and the installation needs no router.

**Repo root:** `/Users/benvonweb/Documents/benvonhep/togagrid/jogi`

**Directory layout:**
```
jogi/
├── BEFEHLE.md, TODO.md, flash-ota.sh, licht todos 16jul.rtf
├── controller/  controller.ino, TASTENBELEGUNG.md
├── grid/        grid.ino, grid_config.h, grid_beat.h, grid_draw.h, grid_text.h, grid_protos.h
│   └── animations/  anim_field.h, anim_geometry.h, anim_life.h, anim_games.h, anim_faces.h, anim_new.h
├── module/      module.ino, module_config.h, module_effects.h
├── webserver/   webserver.ino, togaws_config.h, README.md
└── libraries/TogaProto/  library.properties, src/toga_proto.h, src/toga_ota.h
```
No PlatformIO/Makefile/CMake. Build is **arduino-cli only**.

---

# PART 1 — HARDWARE & WIRING (exact)

## 1.1 Grid (the LED wall) — `grid/grid_config.h`
- **MCU:** ESP32 dev module. (Grid MAC noted in code: `f4:65:0b:e8:98:60`.)
- **LEDs:** WS2812B, **GRB** color order, **FastLED 3.6.0**.
- **Structure:** `NUM_ELEC = 6` driver boards. Each board drives **one X strip and one Y strip**. → 12 X-strips + 12 Y-strips, addressed by strip index **1..12** (strip→board = `(strip-1)/2`).
- **Per strip:** `LEDS_ACTIVE = 121` lit LEDs + `LEDS_SPACER = 15` dead LEDs at the return bend; buffer `LEDS_PER_STRIP = 258` (active + spacer + return; strips fold back). Buffers: `CRGB ledsX[6][258]`, `CRGB ledsY[6][258]`. Buffer indices **255,256,257** are always-black spacer/turn LEDs.
- **Data GPIO pins (verbatim):**
  - `PIN_X0 14, PIN_X1 15, PIN_X2 17, PIN_X3 16, PIN_X4 13, PIN_X5 18`
  - `PIN_Y0 21, PIN_Y1 22, PIN_Y2 23, PIN_Y3 25, PIN_Y4 26, PIN_Y5 19`
  - `addLeds` order in setup: X0..X5 then Y0..Y5, each `FastLED.addLeds<WS2812B, PIN, GRB>(buf, 258)`.
- **Geometry (verbatim):**
  - `const float CENTER = 61.0f;` `const float MAX_DIST = 72.0f;`
  - `const float X_POS[12] = {-51,-42,-32,-23,-14,-5, 5,14,23,32,42,51};` (same for `Y_POS[12]`)
  - `const int INTS[12] = {10,19,29,38,47,56,66,75,84,93,103,112};` — LED indices where a crossing strip intersects.
  - The 12×12 crossings form an **11×11 cell lattice** used as a low-res canvas. LED `n` (1..121) on a strip has world coordinate `n - CENTER` (≈ −60..+60) along the strip; the strip's fixed coordinate is `X_POS[strip-1]` / `Y_POS[strip-1]`.

**`setLED(bool isX, int strip, int led, CRGB)` — serpentine mapping (verbatim logic):**
```c
int  elec  = (strip-1)/2;         // 1,2→0; 3,4→1; … 11,12→5
bool isOdd = (strip % 2 == 1);
bool forward = isX ? !isOdd : isOdd;   // X: even strips forward; Y: odd strips forward
int bufIdx;
if(isOdd) bufIdx = forward ? (led-1)   : (121-led);
else      bufIdx = forward ? (135+led) : (257-led);   // even row offset embeds the 15-LED spacer
(isX ? ledsX : ledsY)[elec][bufIdx] = colour;
```

## 1.2 Controller — `controller/controller.ino`
- **MCU:** ESP32. (MAC noted: `44:1d:64:fa:b0:f8`.)
- **Input/display:** one **Adafruit NeoTrellis 4×4** over **I2C** (`Wire.begin()`; `trellis.begin()`, halts if not found). 16 keypad buttons, each an RGB NeoPixel — the *only* display. No LCD/OLED, no encoders.
- **Special role:** hosts the `togalights` AP on channel 1.

## 1.3 Module — `module/module_config.h`
- **MCU:** ESP32 dev module (drives its own LEDs).
- **LEDs:** `LED_TYPE = WS2812B`, `COLOR_ORDER = GRB`, `FastLED.setCorrection(0xffe0b0)`. Buffer ceiling `MAX_LEDS = 512` (past ~400 px a `FastLED.show()` no longer fits inside the strobe flash window).
- **Geometry is per-device, stored in NVS**, defaults `w=32 × h=8 = 256` LEDs (`h=1` = plain strip). Layout: **serpentine by column** (`XY`, odd columns reversed).
- **LED data pin runtime-selectable, must be one of GPIO 4, 5, 16, 17, 18, 21** (default 4) — each is a separately compiled `addLeds<>` instantiation.
- **Status LED:** onboard blue LED on **GPIO2** (`LED_BLUE 2`) — lit while the controller heartbeat is alive. (Old GPIO0 red LED dropped — it's a boot-strapping pin.)
- Frame pacing `FRAME_MS 20` (50 FPS). Blink timings `MODE_ON_MS/MODE_OFF_MS/CASTLE_ON_MS/CASTLE_OFF_MS = 40`. `STROBE_BRI 255`.

## 1.4 Webserver — `webserver/`
- A dedicated ESP32 that **drives no LEDs** (no FastLED, no LED pins). WiFi radio + HTTP server only. Unplugging it doesn't affect the show.

---

# PART 2 — TOOLCHAIN, BUILD & FLASH (exact, non-negotiable)

**Tooling:** `arduino-cli` only.

**Pinned stack — enforced with `#error` at compile time:**
- **FastLED 3.6.0 exactly.** `grid_config.h` and `module_config.h`: `#if FASTLED_VERSION != 3006000 #error`. 3.10.x silently breaks ESP-NOW/the controller link.
- **arduino-esp32 core 2.0.17.** `toga_proto.h`: `#if ESP_ARDUINO_VERSION_MAJOR >= 3 #error` (core 3.x changed the ESP-NOW recv callback signature). Core path hardcoded in `flash-ota.sh`: `~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.17`.
- **Board/FQBN:** `esp32:esp32:esp32` (ESP32 Dev Module).
- **Partition scheme: `min_spiffs`** (`PartitionScheme=min_spiffs`) — gives two app slots (`app0`+`app1`, 1.875 MB each) so OTA works. `huge_app` has ONE app slot and disables OTA — **do not use huge_app**. (Grid, the largest sketch, fills ~46% of one min_spiffs app slot. SPIFFS shrinks harmlessly — no sketch uses it.)

**One-time setup:**
```bash
ln -s "$PWD/libraries/TogaProto" ~/Documents/Arduino/libraries/TogaProto
```
Every board must be **USB-flashed once with min_spiffs** before OTA works:
```bash
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --upload -p <port> grid/
```

**`flash-ota.sh <sketch> <host>` (verbatim behaviour):**
```bash
CORE=~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.17
ESPOTA="$CORE/tools/espota.py"
FQBN="esp32:esp32:esp32:PartitionScheme=min_spiffs"
BUILD="/tmp/toga-build/$SKETCH"
OTA_PASS="toga-ota"
arduino-cli compile --fqbn "$FQBN" --libraries ./libraries --build-path "$BUILD" "$SKETCH/"
python3 "$ESPOTA" -r -i "$HOST" -p 3232 --auth="$OTA_PASS" -f "$BUILD/$SKETCH.ino.bin"
# archives to .ota-archive/<sketch>-<git-short-sha>[-dirty].bin
```
Port **3232**, auth **`toga-ota`** (= `TOGA_OTA_PASS`).

**OTA prerequisites / gotchas:**
- Your Mac must be joined to `togalights` (no internet — intentional). The controller must be powered (it hosts the AP; without it, no AP, no OTA — but the ESP-NOW bus keeps running).
- macOS firewall must allow incoming **python3** (the ESP connects *back* to the Mac), else it hangs at "Sending invitation".
- **Never `esptool erase_flash`** — it wipes per-device NVS config (module geometry, grid settings). NVS sits at the same address in both partition schemes; OTA doesn't touch it.
- OTA safety net (`toga_ota.h`): a fresh image runs on probation (`verifyRollbackLater()` returns true), self-commits after **60 s + one bus packet** (`TOGA_OTA_COMMIT_FAST_MS=60000`) or **5 min** unconditionally (`TOGA_OTA_COMMIT_MAX_MS=300000`); a crash/bootloop before that auto-rolls-back. Each sketch has `extern "C" bool verifyRollbackLater(){return true;}` below its includes, and calls `togaOtaMarkValidWhenHealthy(t, heardBus)` once per frame. This catches crashes, NOT "boots but renders garbage" — hence the binary archive.

**Hostnames (mDNS + OTA):** grid `toga-grid.local`, controller `toga-ctl.local`, module `toga-mod-<last-3-MAC-bytes-hex>.local`, webserver `toga.local`.

**Wire-format changes:** `TOGA_VERSION` is currently **4** (v4 added battery telemetry to `TogaHelloMsg` via its reserved bytes, size unchanged, and a `battMon` toggle to `TogaModCfgMsg`, 16→18 bytes). Any change to a struct or message type requires bumping it and **reflashing grid + controller + all modules + webserver together**. A version-mismatched node is invisible to all others (or worse, its stale packet passes validation and is applied — the historical strobe-flicker bug).

---

# PART 3 — SHARED WIRE PROTOCOL (`libraries/TogaProto/src/toga_proto.h`, VERBATIM)

`TogaProto` is a header-only Arduino library (`library.properties`: `name=TogaProto version=1.0.0 architectures=esp32 includes=toga_proto.h`). It is the single definition of the protocol, network setup, effect names, and shared behaviour constants — included by all four sketches so the wire format cannot drift.

## 3.1 Constants

```c
#define TOGA_MAGIC    0x4754u   // 'T','G'
#define TOGA_VERSION  4   // v4: battery telemetry in hello + battMon toggle in modcfg
#define TOGA_CHANNEL  1         // ESP-NOW bus channel AND the AP channel

#define TOGA_GROUP_ALL      0
#define TOGA_GROUP_GRID     1
#define TOGA_GROUP_MODULES  2

#define TOGA_MSG_CMD    1       // controller -> broadcast
#define TOGA_MSG_SYNC   2       // grid -> broadcast
#define TOGA_MSG_HELLO  3       // module -> broadcast (1/s)
#define TOGA_MSG_MODCFG 4       // webserver -> broadcast, addressed by MAC
// 5 was TOGA_MSG_OTA (retired). Do not reuse.

#define TOGA_EFFECT_COUNT  9
#define TOGA_EFFECT_FOLLOW 255  // module: follow the grid's mode, not a pinned effect
#define TOGA_CFG_KEEP      254  // TogaModCfgMsg: leave this field unchanged

// togaEffectName(e): 0 singleColor,1 starfield,2 glimmer,3 rainbowGlit,4 confetti,
//                    5 sinelon,6 juggle,7 bpm,8 off, 255 "follow"

#define TOGA_CMD_HEARTBEAT 0    // carries only strobe flags + liveness
#define TOGA_CMD_MODE      1    // arg = +1/-1 mode step (grid only)
#define TOGA_CMD_PARAM     2    // arg = +1/-1 step of .target
#define TOGA_CMD_BEAT      3    // arg = beat period in ms (direct)
#define TOGA_CMD_BEAT_TAP  4    // one tap, arg unused

#define TOGA_P_BRI        0
#define TOGA_P_SPEED      1
#define TOGA_P_COLOR      2
#define TOGA_P_STROBEHUE  3
#define TOGA_P_HUESPEED   4
#define TOGA_P_STROBEGAP  5
#define TOGA_P_BRICOEF    6     // per-mode brightness factor (0..1)

#define TOGA_F_STROBE        0x01   // button 15
#define TOGA_F_MODE_STROBE   0x02   // button 14
#define TOGA_F_CASTLE_STROBE 0x04   // button 13

#define TOGA_AUTO_OFF     0
#define TOGA_AUTO_COUNT   4
static const uint8_t TOGA_AUTO_HUE[4] = { 140, 195, 10, 85 };  // calm, space, party, show

static const uint8_t TOGA_BCAST[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// ── shared behaviour constants (grid + modules must agree exactly) ──
#define MAX_BRIGHTNESS   128   // hard cap (~50%)
#define BRI_DEFAULT       40
#define BRI_STEP           5
#define BRICOEF_MAX      255   // 1.0
#define BRICOEF_MIN       26   // ~0.1 floor
#define BRICOEF_STEP      13   // ~5%
#define CTRL_BRI_MAX     255   // controller pad self-dim
#define CTRL_BRI_MIN      20
#define CTRL_BRI_STEP     13
#define CTRL_BRI_DEFAULT 120

#define BEAT_MIN        0.25f  // dimmest point of the sawtooth glow
#define BEAT_MIN_MS      200   // fastest beat (300 BPM)
#define BEAT_MAX_MS     2000   // slowest beat (30 BPM)
#define BEAT_LATENCY_MS   70   // peak pulled this many ms earlier than the beat
// togaBeatFactor(phase) = 1.0f - (1.0f - BEAT_MIN)*clamp(phase,0,1)   // sawtooth 1.0→0.25

#define HUE_SPEED_MAX   100
#define HUE_RATE_MIN    0.000427f  // knob 1: ~10 min/rev
#define HUE_RATE_MAX    0.8f       // knob 100: ~0.3 s/rev
#define HUE_RATE_K      0.07611f   // ln(MAX/MIN)/(HUE_SPEED_MAX-1)
// togaHueRate(s): 0 if s==0; else HUE_RATE_MIN*expf(HUE_RATE_K*(s-1)) (geometric dial)

#define STROBE_FLASH_MS   10
// togaStrobeOn(t,start,gapMs): period = 10+gapMs; return ((t-start)%period) < 10  (absolute-time modulo, drift-free)
#define STROBE_MAX_BRI   178   // 70% ceiling; applied AFTER a module's gain trim; NOT for castle/mode strobe
#define STROBE_GAP_DEF    90
#define STROBE_GAP_MIN    30
#define STROBE_GAP_MAX   250
#define STROBE_GAP_STEP    5
#define HEARTBEAT_TIMEOUT 400  // ms; sender gone after this
```

## 3.2 Wire structs (NOT packed; explicit reserved fields; layout locked by `static_assert`)

```c
struct TogaHeader {          // 8 bytes
  uint16_t magic;   // TOGA_MAGIC
  uint8_t  version; // TOGA_VERSION
  uint8_t  type;    // TOGA_MSG_*
  uint8_t  group;   // TOGA_GROUP_* (0 = everyone)
  uint8_t  reserved;// 0
  uint16_t seq;     // all repeats of one logical command share a seq
};
struct TogaCmdMsg {          // 16 bytes
  TogaHeader h;
  uint8_t  cmd;      // TOGA_CMD_*
  uint8_t  target;   // TOGA_P_* when cmd==PARAM
  uint8_t  flags;    // TOGA_F_* (every packet incl. heartbeat)
  uint8_t  autoMode; // 0..TOGA_AUTO_COUNT (every packet)
  int16_t  arg;
  uint16_t reserved2;
};
struct TogaSyncMsg {         // 32 bytes — the grid's complete absolute state
  TogaHeader h;
  float    animSpeed;        // 0.1..4.0
  int16_t  animIndex;
  uint8_t  brightness;       // effective 1..MAX_BRIGHTNESS
  uint8_t  hueBase;          // Color knob
  uint8_t  hueAuto;          // live auto-rotation offset
  uint8_t  hueSpeed;         // 0..HUE_SPEED_MAX
  uint8_t  strobeHue;        // 0 = white
  uint8_t  strobeGapMs;
  uint16_t beatMs;           // 0 = glow off
  uint8_t  beatPhase;        // 0..255 == 0.0..1.0
  uint8_t  tintR, tintG, tintB;  // exact current tint colour
  uint8_t  sColR, sColG, sColB;  // exact current strobe colour
  uint8_t  globalBrightness;     // master 1..MAX_BRIGHTNESS
  uint8_t  reserved3[2];
};
struct TogaHelloMsg {        // 32 bytes — a module announcing itself ~1/s
  TogaHeader h;
  uint8_t  mac[6];           // STA MAC — the identity the UI addresses
  uint8_t  group;
  uint8_t  effectPin;        // TOGA_EFFECT_FOLLOW or the pinned effect id
  uint8_t  effectNow;        // what's on screen now
  uint8_t  gain;             // 10..200 %
  uint8_t  pin;              // LED data pin
  uint8_t  linkAlive;        // 1 = this module hears the controller
  uint16_t width, height;
  uint32_t uptimeS;
  uint8_t  reserved[4];
};
struct TogaModCfgMsg {       // 16 bytes — settings for ONE module (by MAC)
  TogaHeader h;
  uint8_t  mac[6];
  uint8_t  effectPin;        // effect id, TOGA_EFFECT_FOLLOW, or TOGA_CFG_KEEP
  uint8_t  gain;             // 10..200, or 0 = keep
};
```

## 3.3 Helper functions (all `static inline`, run in the WiFi task — no Serial, no alloc)
- `togaInitHeader(h, type, group, seq)` — sets magic/version/type/group/reserved=0/seq.
- `togaCheck(data,len,type,expectLen)` — `data!=null && len==expectLen && magic==TOGA_MAGIC && version==TOGA_VERSION && type==type` (memcpy the header, data is unaligned).
- `togaParseCmd(data,len,myGroup,out)` — check CMD; copy; accept iff `group==ALL || group==myGroup`.
- `togaParseSync/Hello(data,len,out)` — check + copy.
- `togaParseModCfg(data,len,myMac,out)` — check MODCFG + copy + `memcmp(out->mac,myMac,6)==0`.

## 3.4 WiFi / ESP-NOW setup
```c
#define TOGA_AP_SSID  "togalights"
#define TOGA_AP_PASS  "andmagic"   // >=8 chars
// CONTROLLER ONLY, before esp_now_init():
togaApBegin():  WiFi.mode(WIFI_AP_STA); WiFi.setSleep(false);
                WiFi.softAP(TOGA_AP_SSID,TOGA_AP_PASS, TOGA_CHANNEL, 0/*not hidden*/, 8/*max*/);
// EVERYONE ELSE, before esp_now_init():
togaWifiBegin(hostname): WiFi.mode(WIFI_STA); WiFi.setSleep(false); WiFi.setHostname(hostname);
                WiFi.setAutoReconnect(true); WiFi.begin(TOGA_AP_SSID,TOGA_AP_PASS, TOGA_CHANNEL);
                // channel is explicit so the STA never scans (deaf to the bus otherwise); does NOT wait for assoc
togaWifiTick(): once/loop on joiners; every 10s if !WL_CONNECTED, re-WiFi.begin(...,TOGA_CHANNEL); logs edges only
togaAddPeer(mac): after esp_now_init(); peer channel=TOGA_CHANNEL, ifidx=WIFI_IF_STA, encrypt=false (broadcast can't encrypt)
```

## 3.5 OTA (`toga_ota.h`)
```c
#define TOGA_OTA_PASS "toga-ota"
#define TOGA_OTA_PORT 3232
togaOtaModuleHost(): "toga-mod-%02x%02x%02x" of MAC bytes [3][4][5]
togaOtaSetup(hostname): ArduinoOTA setPort(3232)/setHostname/setPassword; begin(); (non-blocking, no assoc needed)
togaOtaHandle(): ArduinoOTA.handle() — MUST be above every early return in loop()
#define TOGA_OTA_COMMIT_FAST_MS  60000   // frames + a bus packet -> commit
#define TOGA_OTA_COMMIT_MAX_MS  300000   // frames alone this long -> commit anyway
togaOtaMarkValidWhenHealthy(t, heardBus): commits (esp_ota_mark_app_valid_cancel_rollback) once healthy
// Each sketch needs, below its includes:  extern "C" bool verifyRollbackLater(){ return true; }
```

---

# PART 4 — CONTROLLER FIRMWARE (`controller/controller.ino`)

Sends every command to group 0 (all). Renders live "meters" on its own keys from the grid's echoed `TogaSyncMsg`. Hosts the AP.

**Includes:** `Arduino.h`, `toga_proto.h`, `toga_ota.h`, `Wire.h`, `Adafruit_NeoTrellis.h`. Then `extern "C" bool verifyRollbackLater(){return true;}`.

**Globals:** `Adafruit_NeoTrellis trellis; #define HOLD_TO_SELECT 2000;` synced-state struct `gs` (initial `{brightness=40, animSpeed=1.0, hueBase=0,hueAuto=0,hueSpeed=0,strobeHue=0, tint=255,0,0, sCol=200,200,200, beatMs=0,beatPhase=0,beatAt=0}`); `bool btnDown[16]={}; uint32_t btnDownT[16]={};` `uint8_t autoMode=TOGA_AUTO_OFF;` `enum{ST_DEFAULT,ST_ADJUST}; int state, activeBtn=-1, activeTarget=-1, suppressBtn=-1;` repeat timers `plusPressT/minusPressT/plusRepT/minusRepT`; `volatile bool gsSyncSeen=false; uint16_t txSeq=0;` `uint32_t C(r,g,b)=trellis.pixels.Color(...)`.

**setup():** `Serial.begin(115200); delay(200);` `Wire.begin();` `if(!trellis.begin()) while(1) delay(1000);` for i in 0..15 activate RISING+FALLING edges and `registerCallback(i,handleKey)`; `togaApBegin();` print AP/mac; `if(esp_now_init()!=ESP_OK) while(1)…;` `esp_now_register_recv_cb(onReceive);` `if(!togaAddPeer(TOGA_BCAST)) while(1)…;` `togaOtaSetup("toga-ctl");`

**Pad rotated 90° CCW ("nach links").** The physical pad is turned, so functions sit on rotated keys. The code keeps LOGICAL numbering (table below); two arrays translate at the I/O edges: `PHYS2LOG[16]={3,7,11,15,2,6,10,14,1,5,9,13,0,4,8,12}` (in `handleKey`) and `LOG2PHYS[16]={12,8,4,0,13,9,5,1,14,10,6,2,15,11,7,3}` (in `renderLEDs`). Physical layout: `0`=Mode+, `1`=Mode−, `2`=Beat, `3`=Strobe, `7`=Mode-Strobe, `8`=Color, `9`=Hue-Speed, `11`=Castle, `12`=Auto, `13`=Brightness, `14`=Speed, `15`=Strobe-colour; free `4·5·6·10`.

**4×4 key map (LOGICAL index 0..15 as used in code):**
| Key | Function |
|---|---|
| 0 | **Auto mode** — tap cycles moods 1→2→3→4→1 (`autoMode = autoMode%TOGA_AUTO_COUNT + 1`), sends heartbeat immediately; never turns itself off; blinks ~2 Hz in mood colour |
| 1 | Param **Color** (hue base) — hold 2 s to adjust |
| 2 | free |
| 3 | **Plus +** — next mode / increase active param |
| 4 | Param **Brightness** — hold 2 s |
| 5 | Param **Hue-Speed** — hold 2 s |
| 6 | free |
| 7 | **Minus −** — prev mode / decrease active param |
| 8 | Param **Speed** — hold 2 s; also governs auto-mode switch tempo |
| 9,10 | free |
| 11 | **Beat-Tap** — sends `TOGA_CMD_BEAT_TAP` immediately on press; no local tempo math |
| 12 | Param **Strobe colour** — hold 2 s |
| 13 | **Castle-Strobe** — hold (momentary) |
| 14 | **Mode-Strobe** — hold (momentary) |
| 15 | **Strobe** — hold (momentary); hold 15 + `+`/`−` adjusts strobe gap |

`paramTarget(b)`: 4→BRI, 8→SPEED, 1→COLOR, 12→STROBEHUE, 5→HUESPEED (else −1).

**State machine:** DEFAULT: `+`/`−` step mode. Hold a param key ≥ `HOLD_TO_SELECT`(2000 ms) → ADJUST that param (unless `suppressBtn`). Tap the active param key again → back to DEFAULT and set `suppressBtn=b` (cleared on its release). **No 3 s timeout** (docs claim one; code has none). `atLimit()` is defined (BRI 1..128, SPEED 0.1..4.0, HUESPEED 0..100; COLOR/STROBEHUE wrap) but is **not called** anywhere.

**Auto-repeat (ADJUST):** first press sends immediately; after hold ≥500 ms, rate = 80 ms (`hold<2000`), 40 ms (`<4000`), 16 ms (`≥4000`). **Strobe-gap combo (hold 15 + +/-):** first immediate, then after 400 ms every 60 ms (fixed). DEFAULT +/- = one send per press.

**sendCmd(cmd,target,arg):**
```c
TogaCmdMsg m={}; togaInitHeader(m.h,TOGA_MSG_CMD,TOGA_GROUP_ALL,++txSeq);
m.cmd=cmd; m.target=target; m.arg=arg;
m.flags=(btnDown[15]?TOGA_F_STROBE:0)|(btnDown[14]?TOGA_F_MODE_STROBE:0)|(btnDown[13]?TOGA_F_CASTLE_STROBE:0);
m.autoMode=autoMode;
int reps=(cmd==TOGA_CMD_HEARTBEAT)?1:3;   // one-shots sent 3x under one seq
for(i<reps) esp_now_send(TOGA_BCAST,(uint8_t*)&m,sizeof(m));
```
Heartbeat every 100 ms in loop. Manual `+`/`−` mode step in DEFAULT first sets `autoMode=TOGA_AUTO_OFF` (so the same packet carries auto=off). Strobe keys (13/14/15) also send an immediate `sendCmd(0,0,0)` on both press and release edges (fast flag propagation).

**onReceive():** `togaParseSync` → copy every field into `gs`; `gs.beatAt = millis()` (local); `gsSyncSeen=true` (OTA health proof).

**LED feedback (`renderLEDs(t)`):** blink `(t/125)%2` (~4 Hz param), slow `(t/250)%2` (~2 Hz auto). Helpers: `hsvC(h,s,v)` classic 6-region HSV; `heatCol(frac,v)=hsvC(170*(1-frac),255,v)` (blue→red meter); `paramCol(b,v)`: BRI `(bri-1)/127`, SPEED `(spd-0.1)/3.9`, HUESPEED `hueSpeed/100` → heatCol; COLOR → `rgbScale(tint,v)`; STROBEHUE → strobeCol(v). `autoCol(m,v)=hsvC(TOGA_AUTO_HUE[m-1]+gs.hueBase+gs.hueAuto,255,v)`. `beatCol(t)`: rest `C(40,0,0)`; else phase = `gs.beatPhase/255 + (t-gs.beatAt)/gs.beatMs` wrapped, value `togaBeatFactor(ph)*200`, `C(v,0,0)`. Press feedback: 15→`strobeCol(255)`, 14→full tint, 13→`C(0,140,160)`, generic→`C(70,20,0)`.

**loop():** `togaOtaHandle()` every iter; heartbeat every 100 ms; `togaOtaMarkValidWhenHealthy(t,gsSyncSeen)`; `reportApClients(t)` (prints AP client count on change, ≥5 s); poll gate 20 ms → `trellis.read()`, hold-to-arm scan, +/- dispatch (priority: hold-15 gap, DEFAULT mode, ADJUST param), `renderLEDs(t)`.

---

# PART 5 — GRID FIRMWARE (`grid/grid.ino` + headers)

**Includes (order):** `<FastLED.h>`, `<math.h>`, `"grid_config.h"`, `"grid_beat.h"`, `<toga_ota.h>`; later `<esp_now.h>`, `<WiFi.h>`, `<Preferences.h>`; then `grid_protos.h`, `grid_draw.h`, `grid_text.h`, `animations/anim_{field,geometry,life,games,faces,new}.h`. Plus `extern "C" bool verifyRollbackLater(){return true;}`. `MY_TOGA_GROUP = TOGA_GROUP_GRID`.

**Key globals:** `CRGB ledsX[6][258], ledsY[6][258];` `uint8_t gHueBase=0,gHueAuto=0,gHueSpeed=0,gStrobeHue=0;` AGC: `uint8_t gBaseBri=40; float gAgcGain=1.0; bool gAgcSnap=false;` beat: `float gBeatGlow=1.0;` ESP-NOW: `volatile TogaCmdMsg cmdQ[16]; volatile uint32_t cmdQT[16]; volatile uint8_t cmdHead,cmdTail;` `volatile bool espStrobe,espModeStrobe,espCastleStrobe; volatile uint8_t espAutoMode=0; volatile uint32_t espLastHeartbeat;` sender-MAC learn/warn state; `Preferences prefs; struct Settings cfg; bool beatDirty; uint32_t beatDirtyAt;`

**Persistence** (`Preferences` namespace `"grid"`), `struct Settings`: `int animIndex; uint8_t brightness (effective); uint8_t globalBri; uint8_t modeFactor[96]; uint8_t strobeBrightness,strobeOnMs,strobeOffMs,strobeSquares,strobeGapMs; float animSpeed; uint16_t beatMs;`. Keys/defaults: `anim`(0), `gbri`(40), `mfac`(blob 96B → all 255), `sbri`(200), `son`(40), `soff`(40), `ssq`(30), `sgap`(90), `spd`(1.0), `beat`(0), `hue`(0), `hspd`(0), `shue`(0). Clamps on load: animIndex 0..89, globalBri 1..128, gHueSpeed 0..100, modeFactor[] 26..255, strobeBrightness 1..255, strobeOn/Off 5..200, strobeSquares 1..122, strobeGapMs 30..250, animSpeed 0.1..4.0, beatMs (if≠0) 200..2000. `recomputeBrightness(): cfg.brightness = constrain(round(globalBri*modeFactor[animIndex]/255.0),1,128); FastLED.setBrightness(cfg.brightness);`. Saves are synchronous on each mode/param step; **beat tempo is lazy** (`BEAT_SAVE_QUIET=10000` ms after last tap). **Auto-mood steps are NOT persisted.**

**onReceive():** `togaParseCmd(...,MY_TOGA_GROUP,...)`; set the 3 strobe flags + `espAutoMode` + `espLastHeartbeat`; for non-heartbeat commands dedup by seq (controller sends 3×) and push `{msg, arrivalTime}` into the 16-slot ring `cmdQ/cmdQT` (index mask `&15`, drop on full). Rate-limited (>2 s) German warning if a second commanding MAC appears; learns the first sender's MAC after full validation.

**applyCommand(m, arrivalT):**
- MODE(1): `animIndex=(animIndex+arg+90)%90; recomputeBrightness(); resetAnimState(t); saveSettings(); triggerOverlay(t,animIndex);`
- PARAM(2) by target: BRI `globalBri += arg*5` clamp 1..128 → recompute; BRICOEF(6) `modeFactor[animIndex] += arg*13` clamp 26..255 → recompute; SPEED `animSpeed += arg*0.1` clamp 0.1..4.0; COLOR `gHueBase += arg*6` (wrap); STROBEHUE `gStrobeHue += arg*6` (wrap); HUESPEED `gHueSpeed += arg` clamp 0..100; STROBEGAP `strobeGapMs += arg*5` clamp 30..250. Then `saveSettings()`.
- BEAT_TAP(4): `beatOnTap(arrivalT)` (arrival is the phase anchor); BEAT(3): `beatSetPeriod(arg, arrivalT)`. Both set `beatDirty`.

**loop() timing/flow:**
1. `togaOtaHandle(); togaWifiTick();`
2. `senderAlive = espLastHeartbeat>0 && (t-espLastHeartbeat)<400;` `togaOtaMarkValidWhenHealthy(t,senderAlive);`
3. Button-15 strobe edge detection (runs **before** the FPS cap): on rising, save `preStrobeAnim/preStrobeBri`, pick `nextStrobeLook()`; on falling, restore and clear.
4. Drain command ring: `applyCommand(m, cmdQT[tail])`.
5. `beatTick(t)` (lone-tap-off) → sets beatDirty.
6. **Auto-mood walker** (skipped while strobe held): `mood = senderAlive?espAutoMode:0`. On mood change reset to pos 0, on dwell elapse advance `moodPos=(moodPos+1)%n`. Dwell = `everyMs / constrain(animSpeed,0.1,4.0)`. Playlists (mode indices used directly, `constrain(list[pos],0,89)`):
   - CALM `{18,19,48,47,31,82,73,79,22,34,14,23}` every 45000 ms
   - SPACE `{24,60,44,46,63,9,7,65,66,67,68,69,70,61,62,59,85,25}` every 30000 ms
   - PARTY `{2,27,50,86,80,88,89,12,13,29,87,81,5,45,84,78}` every 12000 ms
   - SHOW `{40,41,55,56,57,58,76,77,42,43,52,53,54,72,71,75}` every 20000 ms
7. **FPS cap:** `if(!strobeActive && t-lastFrame<20) return;` (50 FPS; only button-15 strobe bypasses this).
8. Time base: `frameDt = t-lastAnimT;` `gBeatGlow=beatFactor(t)` (real time); virtual clock `virtualT += frameDt*animSpeed; at=(uint32_t)virtualT; dt=frameDt*animSpeed;` hue rotate `hueAcc += togaHueRate(gHueSpeed)*frameDt; gHueAuto=(uint8_t)hueAcc;`
9. **Sync broadcast every 200 ms:** builds `TogaSyncMsg` with brightness, hueBase, hueAuto, hueSpeed, strobeHue, strobeGapMs, `beatMs=round(bPeriod)` (LIVE), `beatPhase=round(beatPhase(t)*255)` (latency folded), exact `tint=CHSV(hueBase+hueAuto,255,220)`, exact `sCol = hue0?(200,200,200):CHSV(strobeHue,255,220)`, animIndex, animSpeed → `esp_now_send(TOGA_BCAST,...)`. (Note: `globalBrightness` field is left 0 in the current code.)
10. **Strobe renders:** button 15 (`togaStrobeOn`, 5 looks from a shuffled bag, white/hue, brightness capped 178); button 13 castle (static "CASTLE" hue160 row 2 + "2026" hue35 row 7, on/off = strobeOnMs/OffMs, full strobeBrightness); button 14 mode-strobe (blink current anim, on-phase base brightness ×7/4).
11. Otherwise dispatch `switch(cfg.animIndex)` — the 90 animations (Part 6).

**`showGrid()` global post-processor** (called by nearly every animation):
```c
uint8_t off = gHueBase + gHueAuto;
uint32_t lum=0;
for each of ledsX/ledsY [6][258]: if(off && pixel lit){ CHSV h=rgb2hsv_approximate(*p); h.hue+=off; *p=h; } lum += r+g+b;
float N = 6*258*2 = 3096.0f;  float avg = lum/N;
float gainTarget = constrain(45.0f/max(avg,1), 0.35f, 1.5f);        // AGC_TARGET/MIN/MAX
if(gAgcSnap){ gAgcGain=gainTarget; gAgcSnap=false; } else gAgcGain += (gainTarget-gAgcGain)*0.05f;  // AGC_K
FastLED.setBrightness(constrain(round(gBaseBri * gAgcGain * gBeatGlow), 1, 128));
if(millis()<overlayUntil){ drawNumberTopRight(millis()); }
FastLED.show();   // single show per frame
```
AGC constants (grid_config.h): `AGC_TARGET 45.0, AGC_MIN 0.35, AGC_MAX 1.5, AGC_K 0.05`. `gAgcSnap=true` is set on every mode change (in `resetAnimState`) so brightness snaps rather than fades across modes.

**Strobe looks (`strobeFlash(look,n,sc)`):** dealt from a Fisher-Yates shuffled bag (no immediate repeat). `SL_SQUARES 0` (min(strobeSquares,121) random cells via `strobeCell`), `SL_FULL 1` (whole wall), `SL_STRIPS 2` (`STROBE_STRIP_N=6` random strips), `SL_RINGS 3` (concentric square ring `n%6`), `SL_QUADRANTS 4` (random 5×5 quarter). `strobeCell(row,col,sc)` lights one 11×11 cell's 4 border segments via `INTS[]`.

**Text/fonts:** see Part 8.

---

# PART 6 — THE 90 GRID ANIMATIONS

**Dispatch:** `switch(cfg.animIndex)` in grid.ino. Each case calls `anim_<name>(...)`. Most take `at` (the animSpeed-scaled virtual clock, ms); some also take `dt` (scaled per-frame delta); **case 5 (`anim_earthquake`) uses raw real `t`** so its rate ignores animSpeed. Every function begins `float tf = t*0.001f;`. Functions render into `ledsX/ledsY` via `setLED`/`fillCell`/`fxAdd`, then call `showGrid()` (which applies global hue rotation + AGC + beat-glow — **animations never read hue/beat globals directly**). `random(k)` = Arduino uniform [0,k). `frand()` = uniform float [0,1]. `fxhash(n)=fract(sin(n)*43758.5453)`.

**Rendering model:** most "field" animations do two passes — X pass (`x=X_POS[xi]`, `y=n-CENTER`) and Y pass (`y=Y_POS[yi]`, `x=n-CENTER`) — evaluating the same scalar field so the woven grid is coherent.

## Index → function map (authoritative)
```
0 setupGrid        1 heartbeat       2 binaryRain      3 tectonic        4 vortex
5 earthquake(rawT) 6 dnaHelix        7 supernova       8 blackHole       9 wormhole
10 mitosis         11 collider       12 shockwaveChain 13 pulsar         14 liquidMetal
15 iceCrown        16 coriolisStorm  17 crystalFracture 18 aurora        19 lavaLamp
20 nerveSystem     21 gravityLens    22 tidalWave      23 cymatics       24 starfieldWarp
25 darkMatter      26 concentricSquares 27 squareRain  28 diamondPulse   29 checkerboardMelt
30 rotatingCube    31 sunsetHorizon  32 depthMap       33 cosmicDust     34 heartglow
35 evolution       36 morph          37 strangeAttractor 38 harmonicResonance 39 agentField(=plasmaLightning)
40 tetris          41 snake          42 iconShow       43 bouncingLogo   44 starTunnel3D
45 rippleRain      46 spiralGalaxy   47 plasmaField    48 breatheGradient 49 boids
50 fireworks       51 reactionDiff   52 textTOGA       53 textCASTLE     54 text2026
55 stickman        56 paddleBall     57 smiley         58 eye            59 starfield2
60 warp            61 asteroids      62 shootingStars  63 blackHole2     64 pulsar2
65 pulsarNebula    66 orionNebula    67 horsehead      68 twinStars      69 planets
70 earthWindow     71 shipPanel      72 radar          73 sineWaves      74 rocketFire
75 warningLight    76 aliens         77 spaceInvaders  78 rain2          79 levitate
80 confetti        81 spiralEject    82 clouds3d       83 pulseHeart     84 rainUp
85 starsFlying     86 firework2      87 hStripesFlow   88 vuColumns      89 vuGrid
```

## 6.1 `anim_field.h` (cases 1–25)
- **1 heartbeat:** two red ring pulses on a 1000 ms loop, `speed=0.055`, ring half-width 0.06; pulse1 `cycle<600`, pulse2 `200<cycle<800` at 0.7 amp; `dist=|(x,y)|/MAX_DIST`; hue 0, `sat=255-bri*180`.
- **2 binaryRain(at,dt):** Matrix rain on 12 X-strips. Per column: `pos+=speed*dt`, `speed=0.03+rand*0.001`(..0.099), `len=15+rand(30)`; head bright (trail<2 → 255) else `fade²*180`; sparkle 1/20 +60; hue `96+(n%3)*5` sat 220. Y-strips: sparse green noise (3/100, val random(30)).
- **3 tectonic:** wandering bright orange fault cross over dim grey rumble. Fault center `CENTER+sin(t*0.0003)*20 / cos(t*0.0002)*20`, width `4+sin(t*0.0007)*2`; rumble `sin(t*0.001+strip*0.7+n*0.05)*0.5+0.5 * 25`; fault hue 20, sat `200-fb*200`.
- **4 vortex:** 3-arm rainbow spiral. `tight=0.15+sin(tf*0.3)*0.08`; `sp=fract(angle*3/2π - dist*tight - tf*0.5)`; `bri=pow(sin(sp*π),2.5)*sin(clamp(dist/MAX_DIST)*π)`; hue `(180+dist*0.8+tf*20)%255` sat 220.
- **5 earthquake(rawT):** new epicenter every 3000 ms (`random(80)-40`); `wp=elapsed*30`, `decay=exp(-elapsed*0.8)`; `w=max(sin((dist-wp)*0.3)*decay,0)+max(sin((dist-wp*0.6)*0.2)*decay*0.4,0)`; hue 25 sat `200-total*100`. **Rate not scaled by animSpeed.**
- **6 dnaHelix:** two counter-phase sine strands (hue 130 & 300, sat 240). Width `8+sin(tf*0.7)*3`; centers `sin(x*0.12∓tf*2[+π])*40`; `b=pow(1-|y-s|/w,2)*220`; additive.
- **7 supernova(at):** every 4000 ms: white core (`coreBri=1-elapsed*0.8`, `exp(-dist*0.04)`), orange shockwave (`elapsed*55`, width 5), 20 spark streaks (angle random, speed `0.02+rand*0.001`, angular gate 0.15, width 3). Hue 20 if ring>core else 0.
- **8 blackHole(at):** event horizon `8+sin(tf*0.5)*2` (black inside); orange disk `exp(-(dist-eh)*0.08)` with 6 spiral lobes `pow(|sin(diskSpin*π*6)|,3)`; two vertical jets (hue 150) near `|angle|≈π/2`, gate 0.15, gain 1.5.
- **9 wormhole(at):** log-spaced receding tunnel. `lD=log(1+dist*9)/log(10)`; `ring=fract(lD*8 - tf*2)`; twist `sin(angle*3+dist*5-tf*1.5)*0.3+0.7`; `bri=pow(sin(ring*π),3)*twist*(1-dist*0.3)`; hue `190+dist*40+ring*30` sat 240.
- **10 mitosis(at):** green cell splitting. `split=sin(tf*0.6)*35`; two blobs radii `15+sin*5`; `v=max(0,1-d/br)`; membrane rim `pow(v*(1-v)*4,2)`; `bri=total*0.4+membrane*0.8`; hue `85+total*30`, sat `200-membrane*100`.
- **11 collider(at):** 3500 ms cycle. Convergence (`phase<0.7`): two beams spiral inward from rim (radius `MAX_DIST*(1-inward)`, width 4, 180° apart, spin `inward*π*3`); burst (`≥0.7`): shockwave `burst*MAX_DIST*1.5` + central flash. Hue `130+phase*100` / `30+(phase-0.7)*200`.
- **12 shockwaveChain(at):** `NUM_SHOCKS=4` random ring sources, re-fire every 2000 ms, wave speed 45, quadratic ring width 8, decay `1-el*0.5`, hue `200+i*20` sat 220, `b*210`.
- **13 pulsar(at):** lighthouse beam. `sweep=fract(tf*3)*2π`, trail 200°, `trailFade²`, radial `sin(nD*π)`; hue `150+trailDeg*0.05` sat `180+trailFade*75`.
- **14 liquidMetal(at):** thick 2-arm mercury spiral. `sp=fract(angle*2/2π - dist*0.06 - tf*0.15)`; `bri=pow(0.5+0.5*sin(sp*2π),6)*radBand*sin(nD*π)`; hue `tf*4+nD*30+sp*20` sat `140+bri*100`.
- **15 iceCrown(at):** 12 rotating crystal spikes. `rotSpeed=0.08`; per-spike length `0.4+0.5*|sin(index*1.7+tf*0.2)|`; hard angular edge (gate 0.08, pow 3); radial `1-nD/spikeLen`; hue 160, sat `150+nD*90`.
- **16 coriolisStorm(at):** hurricane, 5 bands, differential rotation `tf*(1.5-nD*1.2)`; eye radius 0.12, eyewall 0.20; band `0.6+0.4*sin(nD*π*5-tf*0.4)`; hue `148+nD*15`.
- **17 crystalFracture(at):** hard blue fracture lines, 6 arms, jagged threshold `0.07+0.04*sin(a*1.3+dist*0.2)`; hue 158 sat `80+nD*170`.
- **18 aurora(at):** vertical curtains drifting horizontally. band `sin(x*0.04+tf*0.12)+sin(x*0.07-tf*0.08+1.3)`; shimmer `sin(nY*π*6+tf*2+x*0.1)*0.3+0.7`; curtain `sin(nY*π)`; hue `tf*12+band*160+sin(...)*50` sat `180+curtain*75`.
- **19 lavaLamp(at,dt):** 5 metaballs (radius `15+10*sin(t*0.0005+i*1.3)`, velocity `(rand-10)*0.003*dt`, wall bounce ±55), cubic falloff, dominant-blob hue, sat `200+total*55`.
- **20 nerveSystem(at,dt):** 24 impulses (one per strip), speed `0.05+rand*0.001`, bounce at ends 125/−4, trailLen 10 (head 255, `fade²*220`), per-impulse random hue.
- **21 gravityLens(at):** orbiting mass (`sin/cos(tf*0.2/0.15)*30`) bends background `sin(dist*0.15-tf*0.5)*sin(angle*4+tf*0.3)`; Einstein ring at radius 20 width 4 gain 1.5; hue `180+pattern*60`.
- **22 tidalWave(at):** 3 traveling swells `sin(x*{0.05,0.08,0.03}∓tf*{0.4,0.6,0.25})`; foam `max(0,wave-0.75)*4`; foam hue 140 (whiten), else `160-wave*20`.
- **23 cymatics(at):** Chladni figures — `|cos(modeX·nx·π)cos(modeY·ny·π)| - |cos(modeX2·)cos(modeY2·)|`, modes `2+sin*1.5` etc.; `bri=pow(1-v,3)` (bright nodes); crossing accent hue = `accentPhase*255`.
- **24 starfieldWarp(at,dt):** 24 stars streak outward, `dist+=speed*dt` (speed `0.03+rand*0.001`), respawn >MAX_DIST+10, project onto nearest X and Y strip (snap <8), `b=starBri*warp`, hue `200+i*3` sat 180. Hard `FastLED.clear()`.
- **25 darkMatter(at):** mostly black; faint interference `sin(x*0.06+tf*0.05)*sin(y*0.06-tf*0.04)+sin(dist*0.08-tf*0.03)*0.5`; slow spotlight reveal (orbit ±50 freqs 0.17/0.13, radius 25); `bri=structure*0.08 + structure*reveal*0.9`; hue `200+nD*30+tf*5`.
- *(Defined but NOT dispatched in this file: acidTrip, solarFlares, mandala, dualVortex, collapsingFunnel, quantumFoam, deepAbyss, fibonacci, magneticField, pendulumChaos — dormant code.)*

## 6.2 `anim_geometry.h` (cases 26–34)
- **26 concentricSquares(at):** 5 nested square outlines pinging outward. `phase=fract(tf*0.5 - half*0.15)`; `bri=pow(sin(phase*π),2)*220`; hue `tf*30+half*20`. (`drawGridSquare`.)
- **27 squareRain(at):** 30 random square outlines (size 1..4) flashing (`dropLife=600` ms, fast rise 10% then linear fade), per-drop random hue. State arrays `sqR/sqC/sqSize/sqBirth/sqHue[30]`.
- **28 diamondPulse(at):** Manhattan-distance diamond rings `fract((|x|+|y|)/MAX_DIST - tf*0.4, 0.25)`, `pow(sin(ring*π),3)`, plus a rotated (`tf*0.1`) second set at 0.5 weight; hue `manhattan*200+tf*25`.
- **29 checkerboardMelt(at):** all 121 cells as outlines in a bright/dim checker (`200`/`80`), per-cell hue melt `(row*11+col)*2 + sin(tf*0.3+…)*40 + tf*15`.
- **30 rotatingCube(at):** isometric wireframe cube, `rotX=tf*0.4, rotY=tf*0.6`, half-size 4, iso proj `0.866/0.5`, 3 faces `{4,5,6,7}/{3,2,6,7}/{1,5,6,2}` hues `tf*20{+0,+60,+120}` bri `220/160/100`; only axis-aligned edges drawn.
- **31 sunsetHorizon(at):** vertical gradient with drifting horizon `sin(tf*0.07)*0.3`; glow band `exp(-|pos|*6)*0.6`; two-segment hue (200→0 sky→horizon, 0→25 horizon→ground) + `tf*5`.
- **32 depthMap(at):** rotating (`tf*0.06`) radial gradient, breath `0.7+0.3*sin(tf*0.35)`, banding `0.8+0.2*sin(dist*π*5-tf*0.3)`, `bri=(1-pow(dist,1.4))*banding*breath`; hue `dist*160+tf*4` sat `150+dist*105`.
- **33 cosmicDust(at):** 3 additive Gaussian clouds (centers orbit, Gaussian coeffs 0.0008/0.0007/0.0009, amps 0.8/0.7/0.6, hues 200/240/170).
- **34 heartglow(at):** one slow warm heartbeat, 4 s cycle; envelope (rise<0.15, decay to 0.3, tail) smoothstepped; expanding ring (`radius=cycle*MAX_DIST*1.3`, width 12) + core `exp(-dist*0.07)*pulse*0.8` + ambient `exp(-dist*0.03)*0.08`; hue `5+pulse*15` sat `255-total*180`.
- *(Defined but NOT dispatched here: squareTunnel, fourfoldSymmetry, gameOfLife, squareBreathing, fractalSquares, silk, deepBreath, oilSlick, emberGlow, northernMist, moltenGold. Also `struct Gene{float value,target,rate,baseRate,memory;}` — used by anim_life.h evolution.)*

## 6.3 `anim_life.h` (cases 35, 36, 37, 38, 39)
- **35 evolution(at):** continuously evolving multi-layer spiral galaxy driven by 16 `Gene`s (arm count, rotation, tightness, width, layer weights, organic freq, color spread, sat, mood, chaos…). Genes drift toward targets, mutate every 5–15 s. Per-LED: two spiral layers (primary `pow` arm profile ×`edgeFade=4·nD·(1-nD)`, counter-rotating layer 2) + organic triangle-wave layer + noise; mood cross-fades spirals↔organic. Uses fract+triangle waves instead of sin, and x²/x³/x⁴ instead of pow (keep these approximations for the exact look). Init seeds `{0.3,0.2,0.3,0.4,0.5,0.3,0.4,0.2,0.2,0.3,0.4,0.6,0.05,0.3,0.3,0.1}`.
- **36 morph(at):** computes 6 full-field topologies (spiral / rings / linear waves / interference / sectors / two blobs) and cross-fades them with 6 independently drifting weights; every 12–22 s a mutation picks a new dominant topology. Blob centers orbit `sin/cos(tf*{0.13,0.09,0.11,0.14})*{40,35}`.
- **37 strangeAttractor(at,dt):** real-time Lorenz attractor (σ,ρ,β = drifting 8..12 / 20..30 / 2.667), integrated `dtS=dt*0.001*8`, 5 substeps, clamped; 80-sample comet trail (age² brightness, hue = z-height), projected onto nearest strips (`FastLED.clear()` each frame).
- **38 harmonicResonance(at,dt):** 5 sine oscillators with drifting incommensurate freqs, summed via 5 spatial mappings (horizontal/vertical/diagonal/radial/hyperbolic), `bri=pow((sum+3.5)/7, 3)`; hue `sum*200+tf*8+nX*30`.
- **39 agentField = plasmaLightning(at,dt):** charging 12×12 energy lattice; a cell over threshold discharges a branching lightning bolt (random-walk toward high-energy neighbors, up to 30 steps, up to 4 bolts), drawn on strip segments via `INTS[]` (hue 140–179), fading over TTL; dim blue background glow.
- *(anim_wavePhysics/reactionDiffusion and anim_neuralFire/continuousCA defined here but not dispatched. `#define`s: WV_W/H 32, HR_OSC 5, NF_W/H 24, PL_N 12, PL_BOLT_MAX 30, PL_MAX_BOLTS 4, NUM_SHOCKS 4, SQRAIN_COUNT 30, STUN_N 18, RRIP_N 7, BOID_N 10, FW_N 48, RUN_OBS 4.)*

## 6.4 `anim_games.h` (cases 40–51) — cell-canvas games via `fillCell(row0..10,col0..10,c)`
- **40 tetris(at,dt):** self-playing Tetris in an 11×11 well. 7 tetrominoes `TET_SHAPES[7][4][2]`; AI (`tetEval = 0.80*lines - 0.51*agg - 0.36*holes - 0.18*bump`, El-Tetris) plans best rotation+column; piece rotates/slides/falls one step every 300 ms; full rows flash white twice (170 ms, count 4) then clear; top-out clears the board. Locked cells `CHSV(hue,220,160)`, falling `CHSV(hue,255,235)`.
- **41 snake(at,dt):** greedy self-playing Snake, tick 140 ms; body green (head 255, tail fades), food red `CHSV(0,255,220)`; reset to length-3 when trapped.
- **42 iconShow(at):** cycles 6 icon bitmaps (HEART/SMILEY/STAR/ARROW/INVADER/NOTE, 11×11), 2600 ms each, fade in 400/out 500, hue `at/22`. `drawIcon(rows[11], c)`: col 0 = bit 10.
- **43 bouncingLogo(at,dt):** 5×5 heart drifting/bouncing (`vx=0.017, vy=0.012 *dt`), hue +43 on each wall hit.
- **44 starTunnel3D(at,dt):** 18 stars streaking from center (5,5), `rad += dt*0.00016*(rad*2.5+0.25)`, dim trail, hue `160+rr*80`.
- **45 rippleRain(at,dt):** up to 7 Chebyshev square rings from random interior cells, `rad += dt*0.006` to 8, new every 340 ms, per-ripple hue.
- **46 spiralGalaxy(at):** 2-arm spiral over the whole grid (per-strip `setLED`), `arm=sin(ang*2 - r*7.5 + tf*1.6)`, core glow r<0.14, hue `165+r*70+tf*9`.
- **47 plasmaField(at):** classic 4-sine plasma `sin(x*0.09+tf)+sin(y*0.09+tf*1.1)+sin((x+y)*0.07+tf*0.8)+sin(dist*0.10-tf*1.3)`, hue `nv*255+tf*20` sat 230.
- **48 breatheGradient(at):** diagonal hue gradient `base+(x+y)*0.9`, brightness breathes `0.45+0.55*(0.5+0.5*sin(tf*0.6))`, `base=tf*6`.
- **49 boids(at,dt):** 10 flocking birds (cohesion 0.0006, alignment 0.03, separation 0.0018, sep-radius²=90, speed 0.012..0.05, wrap ±55), hue = heading.
- **50 fireworks(at,dt):** shell bursts every 800 ms, ~14 gravity sparks (`vy+=0.00003*dt`, life `-=dt*0.0009`), per-burst hue.
- **51 reactionDiff(at,dt):** Gray-Scott (Da 1.0, Db 0.5, f 0.055, k 0.062), 2 iterations/frame (frame-count driven, NOT animSpeed), toroidal, seeded 3×3 center, hue `150+B*260`.

## 6.5 `anim_faces.h` (cases 0, 52–58)
- **0 setupGrid(at):** calibration — every strip's 12 crossing LEDs: X red, Y green, origin white; raw `FastLED.show()` (no recolor/AGC).
- **52/53/54 textTOGA/CASTLE/2026(at):** 12-segment stroke font word, static-centered if ≤11 node-cols else scrolls (1 node-col per 700 ms virtual). `col=CHSV(baseHue + at/60, 255, 220)`, base hues 40/140/210.
- **55 stickman(at,dt):** auto-jumping runner over scrolling obstacles (4 types), ground row 9, runner col 2, jump physics `jumpV=0.020, gravity 0.00005*dt`, legs toggle every 140 ms, spawn every 1500 ms.
- **56 paddleBall(at,dt):** self-playing Breakout, 3×11 bricks, auto-tracking paddle (row 10), ball bounces, refills when cleared.
- **57 smiley(at):** round smooth face (`smArc`), 4 expressions cycling every 1600 ms, eyes via `drawEye` (SOLID/HLINE/VLINE/CROSS), mouth arcs.
- **58 eye(at,dt):** per-LED soft-field eye — almond sclera (half-axes A=56,B=27), moving teal iris (r 13.5) + dark pupil (r 6.5), gaze retargets every 1200 ms, blink every 2800 ms (460 ms triangular lid), softness 5.0.

## 6.6 `anim_new.h` (cases 59–89) — all stateless, pure functions of `at`; additive `fxSplat/fxStreak/fxRing/fxNebula/fxPlanet` over `clearFrame()`; randomness via `fxhash(const*i)`
- **59 starfield2:** 70 stars accelerating out `r=ph²·62`, bluish low-sat.
- **60 warp:** 55 stars with motion-blur streaks (`fxStreak` head→tail).
- **61 asteroids:** 5 grey rocks drifting left→right, orbiting bright glint.
- **62 shootingStars:** 45 twinkling stars + 3 diagonal streaks.
- **63 blackHole2:** dark core, two orange accretion rings, 10 swirling hot particles.
- **64 pulsar2:** rotating twin beams from a blue-white core (`base=tf*2.4`).
- **65 pulsarNebula:** pulsar beams inside `fxNebula(...,180,200,180,40,44,0.9)`.
- **66 orionNebula:** three layered gas clouds (hues 155/215/20) + 6 stars.
- **67 horsehead:** dark horsehead silhouette (`headTop=-6-10·exp(-((x+4)²)/60)`) over `inoise8` glow (hue 24).
- **68 twinStars:** binary pair orbiting (`R=22`, warm hue 30/35 & teal 150) + purple gas bridge.
- **69 planets:** 20 bg stars + two `fxPlanet` worlds (r 12 hue 20; r 8 hue 150).
- **70 earthWindow:** shaded globe r=26, scrolling `inoise8` continents (land hue 85 / ocean 150), circular frame ring.
- **71 shipPanel:** 4×5 grid of indicator lights blinking at varied rates (hues 0/90/150).
- **72 radar:** green sweeping beam (`a=fract(tf*0.25)*2π`), range rings, 4 blips flashing `exp(-rel*2.2)` when swept.
- **73 sineWaves:** traveling/interfering horizontal bands `sin(x*0.14+tf*1.5)+0.7*sin(...)`, hue `140+w*30`.
- **74 rocketFire:** upward plume of 50 cooling particles (`hue=45*life`) + bright nozzle.
- **75 warningLight:** rotating amber beacon (`a=tf*3.0`), 3-wide spread, hue 20.
- **76 aliens:** 3 bobbing green alien heads (hue 95) with blinking eyes.
- **77 spaceInvaders:** 3×4 marching invaders (`fxInvader`), side-to-side + drop, 2-frame legs, row hues 150/95/30.
- **78 rain2:** 40 droplets falling with short trails (hue 150).
- **79 levitate:** 45 motes drifting upward, flickering, fading mid-travel (hue 40).
- **80 confetti:** 60 multicolor points popping (`sin(ph·π)`) and drifting down, random hue.
- **81 spiralEject:** spinning spiral ejecting 60 particles from a white core, hue `180+ph*80`.
- **82 clouds3d:** two layered `inoise8` cloud fields drifting, hue ≈150.
- **83 pulseHeart:** implicit heart curve `h³ - x²y³` beating (`beat=0.5+0.5·|sin(tf*2.2)|`, scale `1/(20+beat*6)`), hue 0.
- **84 rainUp:** reverse of rain2 — droplets rising with trails below.
- **85 starsFlying:** 55 stars drifting every direction, wrapping ±60, twinkling.
- **86 firework2:** 4 shells rise then burst into 24 gravity-arced sparks (`period=2.2+rand*1.5`, gravity `-14·e²`).
- **87 hStripesFlow:** crisp horizontal bands flowing up (`period=18`, duty 55%, edge ramp 1.2, scroll `tf*22`), hue `150+8·sin(y*0.05)`.
- **88 vuColumns:** each of 12 X-strips a VU bar (green→red), jumping on a beat (`beat=floor(tf*2.4)`, decay resets each beat), `lvl` from sine + `fxhash(col,beat)`.
- **89 vuGrid:** vertical bars (X strips, green→red) AND horizontal bars (Y strips, cyan→blue) simultaneously.

---

# PART 7 — MODULE FIRMWARE (`module/module.ino` + `module_config.h` + `module_effects.h`)

Pure ESP-NOW **receiver** (transmits only one hello/sec). Mirrors the grid wholesale; applies no steps.

**Config globals (NVS namespace `"module"`):** `gGroup`(=TOGA_GROUP_MODULES=2), `gEffectPin`(=EFFECT_FOLLOW=255), `gGain`(=100), `cfgW`(32), `cfgH`(8), `cfgPin`(4). Keys: `grp`, `w`, `h`, `pin`, `eff`, `gain`. Clamps: geometry falls back to 32×8 if `w<1||h<1||w*h>512`; `gEffectPin` → FOLLOW if `>=9`; `gGain` 10..200. Live geometry `gWidth/gHeight/gNumLeds`. **Geometry & pin are staged** (need `save`+reboot); effect & gain apply live.

**Two buffers:** `CRGB fx[512]` (effects paint NATIVE hues), `CRGB leds[512]` (driven). `showModule()` renders fx→leds rotating each lit pixel by `gHueBase+gHueAuto` (so trails don't smear). `gTrash` scratch pixel for OOB.

**XY (serpentine by column):**
```c
XY(x,y) = (x&1) ? x*gHeight + (gHeight-1-y) : x*gHeight + y;   // odd columns reversed
ledXY(x,y): bounds-checked → fx[XY] or gTrash
```
**addLedsForPin(pin):** switch over 4/5/16/17/18/21 → `FastLED.addLeds<WS2812B, pin, GRB>(leds, gNumLeds)`; `setCorrection(0xffe0b0)`; false on unknown.

**setup():** Serial 115200; `pinMode(LED_BLUE=2,OUTPUT)`; `loadConfig()`; `addLedsForPin(cfgPin)` (fallback 4); `setBrightness(gBri)`; clear both buffers + show; `initStarfield()`; seed glimmer; `togaWifiBegin(togaOtaModuleHost())`; `WiFi.macAddress(gMyMac)` **before** callback; `esp_now_init()`/`register_recv_cb(onReceive)` (fatal on fail); `togaAddPeer(TOGA_BCAST)` (non-fatal); `togaOtaSetup(togaOtaModuleHost())`.

**onReceive() (WiFi task, buffers only):** `TogaCmdMsg` (group-filtered) → set 3 strobe flags + `espLastHeartbeat` immediately; `TogaSyncMsg` → `syncBuf; syncPending=true` (deferred); `TogaModCfgMsg` (MAC-matched) → `cfgBuf; cfgPending=true` (deferred, writes NVS).

**applySync(s):** copy every field wholesale (no clamp): `gBri, gAnimSpeed, gHueBase, gHueAuto, gHueSpeed, gStrobeHue, gStrobeGap, gBeatPeriod=s.beatMs, gBeatPhase=s.beatPhase/255, gSingleColor=CRGB(tint), gAnimIndex=s.animIndex`.

**applyModCfg(c):** if `effectPin != TOGA_CFG_KEEP(254)` and valid → `gEffectPin`; if `gain != 0` → `gGain=constrain(gain,10,200)`; persist only keys `eff`/`gain` (live, deliberately persistent since set from a phone).

**activeEffect():** `gEffectPin<9 ? gEffectPin : (gAnimIndex % 8)` — following maps the grid's mode onto 8 lit effects; the 9th (powersave/off, id 8) is EXCLUDED from rotation (reachable only by pinning `effect 8`).

**loop():** `serialPoll(); togaOtaHandle(); togaWifiTick();` take pending sync/cfg; `senderAlive = espLastHeartbeat>0 && (t-espLastHeartbeat)<400`; `digitalWrite(LED_BLUE, senderAlive)`; `sendHello(t,senderAlive)`; `togaOtaMarkValidWhenHealthy(t,senderAlive)`; strobe edge tracking. FPS cap `if(!strobeActive && t-lastFrame<FRAME_MS(20)) return`. Beat-glow (real time): `gBeatPhase += frameDt/gBeatPeriod; gBeatGlow=togaBeatFactor(gBeatPhase)`. Hue rotate (real time) `hueAcc += togaHueRate(gHueSpeed)*frameDt; gHueAuto=(uint8_t)hueAcc`. Priority: (1) button-15 strobe overrides all (`flashSolid(white/CHSV(strobeHue), STROBE_BRI=255, cap STROBE_MAX_BRI=178)`); (2) castle-strobe → flash tint `CHSV(gHueBase+gHueAuto,255,220)` on/off 40 ms (no font); (3) mode-strobe → blink current effect brighter (×7/4), on/off 40 ms; (4) normal effect. Effect stepping via **integer virtual clock**: `gStepAcc += frameDt*gAnimSpeed/FRAME_MS; steps=(int)gStepAcc` (cap 4); each step `gVirtMs+=20; gTimebase=t-gVirtMs; gPatterns[activeEffect()]()`. `showModule(modeStrobeHeld ? gBri*7/4 : gBri)` every frame.

**showModule(baseBri):** rotate fx→leds by `gHueBase+gHueAuto` (memcpy fast path if off); `FastLED.setBrightness(constrain(round(baseBri * gGain/100 * gBeatGlow), 1, 128))`; show.

**sendHello():** every `HELLO_MS=1000` ms, `TogaHelloMsg` with mac, group, effectPin, `effectNow=activeEffect()`, gain, `pin=cfgPin`, linkAlive, width, height, `uptimeS=t/1000`.

**Serial console (115200):** `show`/`help`/`?`, `group <0-2>`, `w <n>`(1..512), `h <n>`(1..255), `leds <n>`(sets w, h=1), `pin <n>`(validated on save), `effect <0-8>` / `effect -1` or `follow`, `gain <10-200>`, `bmon <0/1>` (Li-Ion undervoltage lockout, default off, persists immediately), `save`, `reboot`. effect/gain/bmon live; w/h/pin need save+reboot.

**Battery monitoring (opt-in, `module_battery.h`):** single Li-Ion via 50:50 divider on **GPIO34** (ADC1_CHANNEL_6, esp_adc_cal, no analogRead). `readBattMv()` averages 8 raw samples (2 ms apart), one IIR low-pass (`IIR_ALPHA 0.15`, ~61 s tau). Every 10 s the loop compares the *filtered* pin mV to `BATT_THRESHOLD_MV 1650` (=3.30 V cell); `BATT_CONFIRM 3` consecutive breaches → UVLO: print, blank strip, tri-state `cfgPin`, `esp_deep_sleep_start()`. `RTC_DATA_ATTR batteryDead` re-sleeps on every boot until a real power cycle clears it (no wake source). Default **OFF** (`battMonEn`, NVS `bmon`) so a floating pin on the USB bench can't false-trip. `battVolt` (cell V) is a global, sent in `TogaHelloMsg.battMv`/`battFlags` and shown/toggled in the webserver UI.

**Effects (`module_effects.h`, `gPatterns[]` index = effect id, VERBATIM key parts):**
```c
// 0 singleColor: fill_solid(fx, gNumLeds, gSingleColor);
// 2 glimmer: per-pixel gGlimmerVal += gGlimmerState; fx[i]=gGlimmerBase[i] % gGlimmerVal[i];
//    at>=250 -> state=-1-random8(3); at<=70 -> base=CHSV(30+random8(20),128+random8(128),255), state=1+random8(3)
// 3 rainbowWithGlitter: fill_rainbow(fx,gNumLeds,0,7); addGlitter(80): if(random8()<80) fx[random16(n)]+=White
// 4 confetti: fadeToBlackBy(fx,n,10); fx[random16(n)] += CHSV(random8(64),200,255)
// 5 sinelon: fadeToBlackBy(fx,n,20); fx[beatsin16(13,0,n-1,gTimebase)] += CHSV(0,255,192)
// 6 juggle: fadeToBlackBy(fx,n,20); for i<8: fx[beatsin16(i+7,0,n-1,gTimebase)] |= CHSV(dothue,200,255); dothue+=32
// 7 bpm: beat=beatsin8(62,64,255,gTimebase); fx[i]=ColorFromPalette(PartyColors_p, i*2, beat + i*10)
// 8 powersave: fill_solid(fx,n,Black)
// 1 starfield: struct Star{int16 x,y; int8 z; uint8 hue;} _stars[24]; MAX_DEPTH=127;
//    fadeToBlackBy(fx,n,64); per star: k=MAX_DEPTH/(z+w); px=x*k+halfW; py=y*k+halfH;
//    z-=1; brightness=1-z/MAX_DEPTH; CHSV(hue, 60, 255*brightness) via ledXY; respawn when z<30
gPatterns = { singleColor, starfield, glimmer, rainbowWithGlitter, confetti, sinelon, juggle, bpm, powersave };
```
beatsin effects run on `gTimebase` (integer virtual clock, wrap-safe — a float would freeze effects after ~6 days uptime).

---

# PART 8 — GRID FONTS & DRAWING PRIMITIVES (`grid_text.h`, `grid_draw.h`, VERBATIM essentials)

**12-segment stroke font** (`FONTW[37]`, 2 cells wide × 2 tall on a 3×3 node grid) — real letters A–Z + digits. Segment bits: `sTL=1,sTR=2,sML=4,sMR=8,sBL=16,sBR=32,sLU=64,sLL=128,sCU=256,sCL=512,sRU=1024,sRL=2048`. `glyphIndex(c)`: space=0, A–Z=1..26, 0–9=27..36. `drawTextCells(s,leftC,topR,col)` — 3 node-cols per char.

**1-cell 7-segment micro-font** (`seg7(c)`, 1 cell wide × 2 tall) for CASTLE/2026: bits `g7a=1,g7b=2,g7c=4,g7d=8,g7e=16,g7f=32,g7g=64`. `drawText1Centered(s,topR,col)` — 6 chars fit across 11 columns.

**Mode-number overlay:** `triggerOverlay(t,idx)` shows the mode number top-right ~500 ms (white, `NUM_WHITE=70`); `triggerOverlayMood(t,mood)` shows "A1".."A4" ~1000 ms (`AUTO_MS 1000`) in the mood's colour `CHSV(TOGA_AUTO_HUE[mood-1]+gHueBase+gHueAuto,255,AUTO_V=160)`. Drawn inside `showGrid()`'s single show (after AGC/hue-rotate).

**Segment drawing:** `drawHSeg(R,C,col)` → Y-strip R+1 between X-cols C..C+1; `drawVSeg(R,C,col)` → X-strip C+1 between Y-rows R..R+1 (corner LEDs left dark).

**Cell canvas:** `fillCell(row0..10,col0..10,c)=drawGridSquare(row+1,col+1,row+2,col+2,c)`. `drawGridSquare(r1,c1,r2,c2,col)` draws 4 border strip-segments via `INTS[]`. `clearFrame()=FastLED.clear() + black spacer LEDs 255/256/257 on all 6 elec X&Y`.

**Soft-field additive primitives (float anti-aliased):**
- `fxAdd(isX,strip,led,c,f)` — additive (`qadd8`) with fraction f, using the exact setLED index mapping (elec/odd/forward/bufIdx).
- `fxSplat(px,py,rad,aa,col)` — round point: solid to `rad`, ramp to 0 over `aa`; fraction `(rad+aa-d)/aa`.
- `fxStreak(x0,y0,x1,y1,rad,aa,col)` — line of splats, `steps=(int)len+1`.
- `fxRing(cx,cy,rad,thick,aa,col)` — circle outline: fraction `(thick+aa-|d-rad|)/aa`.
- `fxNebula(at,hue,sat,scale,tspd,radius,gain)` — `inoise8((x+80)*scale,(y+80)*scale,at*tspd)`, radial envelope `(radius-d)/radius`, `v=(nz/255)²·env²·gain`.
- `fxPlanet(cx,cy,r,hue,sat,lx,ly,rot)` — lit sphere: `mask=(r+1.3-d)/1.3`, `lit=0.12+0.88·clamp(0.5+0.5·(dx·lx+dy·ly)/r)`, texture `0.72+0.28·(inoise8(...)/255)`.
- `smArc(cy,cx,rad,col,quad)` — thin arc where `|d-rad|<0.62`; quad 0=full/1=lower(dy>0.1)/2=upper(dy<-0.1).

---

# PART 9 — BEAT DETECTOR (`grid/grid_beat.h`, grid-only, full algorithm)

Tap-tempo → **brightness** sawtooth glow (never speed). The controller forwards each button-11 press as a raw `TOGA_CMD_BEAT_TAP` the instant it happens; all logic lives here, on the node drawing frames (the tap is the phase anchor).

**Constants:** `BEAT_DEBOUNCE_MS 100`, `BEAT_WINDOW 4`, `BEAT_LOCK_IVS 3` (lock after 3 valid intervals = 4 taps), `BEAT_TOL 0.15` (±15%), `BEAT_ADJUST 0.05` (5% period nudge), `BEAT_PULL 0.30` (30% phase pull), `BEAT_LOSE 6` (misfits before forget), `BEAT_SOLO_MS 2000` (lone-tap-off silence). Plus proto's `BEAT_MIN_MS 200`, `BEAT_MAX_MS 2000`, `BEAT_LATENCY_MS 70`.

**State:** `float bPeriod=0` (0=off); `bool bLocked=false`; `uint32_t bAnchor=0; float bAnchorFrac=0` (anchor split whole+fraction to survive millis() past 2²⁴); `uint32_t bLastTap=0; float bIvs[4]; uint8_t bIvCount=0, bMiss=0, bSeqTaps=0; bool bSoloArmed=false`.

**Functions:**
- `beatAnchorShift(ms)`: `na=bAnchorFrac+ms; w=floor(na); bAnchor+=(int32)w; bAnchorFrac=na-w`.
- `beatPhase(now)`: if `bPeriod<=0` return 0; `tt=now+70; el=(int32)(tt-bAnchor)-bAnchorFrac`; if `el≥bPeriod||el<0` advance anchor by whole periods; return `clamp(el/bPeriod,0,1)`.
- `beatFactor(now)`: `1.0` if no tempo, else `togaBeatFactor(beatPhase(now))` → sawtooth 1.0→0.25.
- `beatWindowTempo(out)`: true iff last 3 intervals agree within ±15% of their mean (the even-taps override detector; ignores the locked tempo).
- `beatPullPhase(now)`: nudge phase 30% toward nearest beat.
- `beatOnTap(now)`: debounce <100 ms; sequence bookkeeping (gap >2000 ms starts new sequence, `bSeqTaps=1`); reject intervals outside 200..2000 (but keep a locked tempo); push interval. **Locked:** first test override (even 4 taps differing >15% → hard re-anchor `beatSetPeriod`-style); else on-tempo (`|iv-bPeriod|≤0.15·bPeriod` → nudge 5% + pull phase), harmonic (`|iv-2·bPeriod|` → pull phase), else `++bMiss≥6 → beatForget`. **Learning:** at ≥3 intervals lock to their mean (no evenness test), hard-anchor.
- `beatTick(now)`: fires true once when `bSeqTaps==1` + 2000 ms silence + a tempo was running → `beatForget` (the lone-tap off-switch).
- `beatSetPeriod(ms,now)`: `bPeriod=constrain(ms,200,2000); bLocked=true; …; beatAnchorSet(now)`.

Serial (German): "Tempo verworfen (%s) — tippe neu ein", "Takt passt nicht mehr", "einzelner Tap", plus LOCK/Tap lines.

---

# PART 10 — WEBSERVER (`webserver/webserver.ino` + `togaws_config.h`)

Listen-only module manager. Config: `togaws_config.h` only defines `#define WS_HOSTNAME "toga"` (mDNS `http://toga.local/` + OTA host); WLAN creds inherited from `toga_proto.h`.

**Includes:** `Arduino.h`, `toga_proto.h`, `toga_ota.h`, `WebServer.h`, `ESPmDNS.h`, `togaws_config.h`. `WebServer server(80);`

**Constants:** `MAX_MODULES 16`, `OFFLINE_MS 5000`, `FORGET_MS 600000`, hello ring `HQ_LEN 8`.
**`struct ModEntry { bool used; uint8_t mac[6]; uint8_t group,effectPin,effectNow,gain,pin,linkAlive; uint16_t w,h; uint32_t uptimeS,lastSeen; };`**

**setup():** `togaWifiBegin(WS_HOSTNAME)`; wait up to 40×500 ms for WL_CONNECTED; on connect `MDNS.begin(WS_HOSTNAME)`, `addService("http","tcp",80)`, `enableArduino(TOGA_OTA_PORT,true)`; `esp_now_init()` (fatal on fail); `register_recv_cb(onReceive)`; **peer add with channel 0** (use interface's current channel — this node inherits the router's channel, unlike `togaAddPeer`); `togaOtaSetup(WS_HOSTNAME)`; routes `/`, `/api/modules`, `/api/set`, 404; `server.begin()`.

**onReceive():** `togaParseHello` → ring `helloQ[HQ_LEN]`; `togaParseSync` → single `syncBuf`, `syncSeen`, `syncAt`.
**upsert(mac):** find used row by MAC; else first free; else recycle stalest (min lastSeen); never null. **drainHellos(t):** merge ring into table, copy all fields, `lastSeen=t`; drop rows older than FORGET_MS.

**Endpoints:**
- `GET /` → serves `PAGE[]` (PROGMEM single page).
- `GET /api/modules` → JSON `{ch, chWant, chOk, gridSeen (sync <2000ms), gridMode (animIndex+1 or 0), effects[9 names], follow(255), mods:[{mac, online (age<5000), age(sec), grp, eff, effNow, gain, pin, w, h, link, up}]}`.
- `GET /api/set?mac=&effect=&gain=` → validate (effect 0..8 or 255, gain 10..200; absent effect→CFG_KEEP, absent gain→0=keep) → `sendModCfg` → `{"sent":true}`. **No optimistic echo** — the row updates only when the module's next hello confirms.
**sendModCfg(mac,eff,gain):** `TogaModCfgMsg`, broadcast **3×** (idempotent absolute values), addressed by MAC.

**Web UI (`PAGE[]`):** zero external JS libs (vanilla `fetch`, inline CSS, dark theme `#111`/`#eee`, **German** text, title "TOGA Module"). Per-module card: MAC, online/offline pill ("weg seit Ns"), controller-link pill, meta (`w×h · Pin · Gruppe · läuft: <effNow> · up Ns`), **Effekt** dropdown (effect names + "folgt Grid"=255), **Gain** slider 10–200%. `tick()` fetches `/api/modules`; `setInterval(tick,1500)` but **paused while dragging a slider** (`touching`). Effect change → `set(mac,'effect='+v)`; gain release → `set(mac,'gain='+v)`. Serial mirrors the module list every 5 s (fallback).

**Replaces** the deleted `module/esp32_webserver_toga/` (still in git history) — that was itself a module with a bootstrap/iro.js/jQuery UI, used FastLED 3.10 + core 3.x, incompatible with the pinned stack, so rewritten dependency-free.

---

# PART 11 — DOCS, TODOS, CRITICAL CONSTRAINTS

## Controller command reference (`BEFEHLE.md`) — highlights
Full 16-key map is in Part 4. Parameters: Brightness ±5 (1–128, saved per mode), Speed ±0.1 (0.1–4.0), Color/Strobe-color ±6 (wrap), Hue-Speed ±1 (0–100 log), Strobe-gap ±5 ms (30–250). Auto moods: Calm 45 s / Space 30 s / Party 12 s / Show 20 s (dwell ÷ Speed). Strobe frequency = `STROBE_FLASH_MS + gap` = 40–260 ms (3.8–25 Hz), strip-length-independent (absolute-time modulo). Strobe brightness capped 70% (178/255).

## Outstanding TODOs (`TODO.md` + `licht todos 16jul.rtf`)
- [ ] Reset-button key combination.
- [ ] A button for global strobe (incl. the module lights).
- [ ] A color-switching mode.
- [ ] Check the brightness-settings default helper.
- [ ] Verify beat-match on the real installation; tune `BEAT_LATENCY_MS` (too late → raise, too early → lower).
- Ideas: 3 buttons for modules ("send a dot through"); exclude modules from certain functions; module modes that depend on strip length.
- Note: `TODO.md` still says `TOGA_VERSION`=2 — **stale**; the code is at **4**.

## Critical constraints (read before changing anything)
1. **Pinned stack enforced by `#error`:** FastLED **3.6.0** + arduino-esp32 **2.0.17**. Do not upgrade.
2. **Partition scheme is `min_spiffs`** (two app slots for OTA), NOT `huge_app`.
3. **`TOGA_VERSION` = 4.** Any wire-format change → reflash **all four node types together**.
4. **Never `erase_flash`** (wipes per-device NVS).
5. **Everything on WiFi channel 1 / no router.** The controller's AP handles this.
6. `libraries/TogaProto` must be symlinked into the Arduino libraries dir before building.
7. The grid is the **only** node that applies command steps and the **only** sync source; the controller is the **only** command source and the AP host. Preserve those roles.
8. Working-tree note: `module/`, `webserver/`, `libraries/`, `flash-ota.sh` are currently untracked; old `module/esp32_webserver_toga/` is deleted (still in git history).

---

## Deliverable
This document is the reproduction spec itself. Proposed action on approval: save it into the repo (e.g. `PROJECT_SPEC.md`) so it can be handed to another AI. No code changes are involved. For byte-exact reproduction of the 90 procedural animations, the original `grid/animations/anim_*.h` files remain the ground truth; every constant and formula needed to re-derive them is above.
