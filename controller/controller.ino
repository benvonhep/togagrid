// ═══════════════════════════════════════════════════════
//  NeoTrellis 4x4 Sender — parameter-adjust interaction model
//
//  Broadcasts every command (ESP-NOW, group 0 = all) so the grid and the
//  LED modules act on the same packet. The grid broadcasts its absolute
//  state back; that is what drives the meters on these buttons.
//
//  PAD ROTATED 90° CCW ("nach links"). The physical pad is turned, so every
//  function sits on a rotated key. All button numbers in THIS comment and in the
//  code below are LOGICAL; PHYS2LOG/LOG2PHYS translate at the two I/O edges.
//  Physical layout after the rotation (what the operator actually presses):
//     phys 0 Mode+   1 Mode−   2 Beat    3 Strobe
//          4 SpeedSweep 5 Spot 6 Blackout 7 Mode-Strobe
//          8 Color   9 Hue-spd 10 StrobeSpeed 11 Castle-Strobe
//         12 Auto   13 Helligkeit 14 Speed 15 Strobe-colour
//
//  DEFAULT:  + (3) / − (7) change MODE.  0 = Auto-Modus.   (logical numbers)
//  Hold a param button 2s → ADJUST that param (it blinks, all others
//    black except +/−; +/− then change only that param w/ accel).
//  3s of no +/− activity → back to DEFAULT.
//  Helligkeit (4) is its own 3-level machine: tap = GLOBAL master (slow pulse),
//    hold 2s = current mode's FACTOR (fast pulse), hold 10s = this CONTROLLER's
//    pad brightness (solid white). Effective = global × factor; the global also
//    dims the pad. Tap again to exit. Everything persists across power-off.
//  Beat match (11): just tap the beat, ~4 taps lock it. Each tap is forwarded
//    raw and the GRID does the tempo detection → glow pulse. Speed is untouched.
//  Strobe (15): momentary while held. Strobe(15)/Mode-strobe(14)/Castle(13) all
//    flash on ONE global strobe speed. Hold any of them + press +/- → tune that
//    speed live; or TAP StrobeSpeed (phys 10 / logical 9) to enter its adjust mode
//    and use +/-. Faster = shorter period; on:off ratio is a fixed 1:2 (typical strobe).
//  Auto (0): tap to cycle the four moods 1→2→3→4→1 (button blinks in the
//    mood's colour). Any manual mode change with +/- turns it off again.
//  PROFILE (v11): hold phys 0+1+2 (Mode+ / Mode− / Beat) 5s → pad blinks red,
//    phys 12 = Togalicht, phys 13 = Drahthaus. Drahthaus: +/- walk the SETLIST
//    (edited on the phone page), no mode numbers on the wall, and phys 11 (Castle)
//    TAPS open the text-strobe layer: phys 0..5 = RAVE! · ALLE GEGEN ALLE · Elefant ·
//    DHA/RTU/AHS · DRAHT/HAUS (hold = strobe), 11 again = close.
//
//  Params (logical):  4 Helligkeit (own machine) · 1 Color · 12 Strobe-colour · 8 Speed · 5 Hue-speed · 9 Strobe-speed (tap-toggle)
// ═══════════════════════════════════════════════════════
#include <Arduino.h>
#include <toga_proto.h>          // the wire format, shared with grid/ and module/
#include <Wire.h>
#include <Preferences.h>          // NVS: persist this controller's own pad-brightness factor
#include <WebServer.h>           // the phone module-manager UI (was standalone webserver/, now folded in here — that sketch is deleted)
#include <ESPmDNS.h>             // advertise the page over mDNS as toga-ctl.local
#include "Adafruit_NeoTrellis.h"

// ── Module manager (formerly the standalone webserver/ sketch, now here) ──────────
// We host the AP, so the phone page that lists every module and configures it
// now lives here too — no separate board. Modules announce themselves once a
// second with a TogaHelloMsg; we keep a table of who is out there and send a
// TogaModCfgMsg addressed at one MAC when the operator changes something. This
// is a pure listener/config path — it never touches the show or the meters.
// Declared above the first function definition (below) so arduino-cli's hoisted
// prototype for upsert() — which returns ModEntry* — lands after the type, not
// before it.
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
  uint16_t battMv;               // cell mV, 0 = unknown/none
  uint8_t  battFlags;            // TOGA_BATT_F_ENABLED / _LOW
  uint8_t  followMask;           // TOGA_FOLLOW_* — which grid functions the module slaves to
  uint8_t  gap;                  // output gap: 1 = every LED, N = light every Nth
  uint8_t  floor;                // floor-strip flow: 0 off, 1 fwd, 2 rev
  uint16_t fwBuild;              // firmware fingerprint (TOGA_FW_BUILD) — spot a stale module
  uint8_t  localHue;             // independent-mode colour (0 = native) (v10)
  char     name[16];             // user label (module-side, echoed in its hello)
  uint32_t lastSeen;             // millis() on THIS node
};
ModEntry mods[MAX_MODULES];

// The recv callback runs in the WiFi task; HTTP handlers read the table from
// loop(). Rather than lock, the callback only parks the hello in a ring and
// loop() merges it — the same split the modules themselves use for sync/cfg.
#define HQ_LEN 8
volatile TogaHelloMsg helloQ[HQ_LEN];
volatile uint8_t hqHead = 0, hqTail = 0;

Adafruit_NeoTrellis trellis;

#define HOLD_TO_SELECT     2000   // hold btn 4 this long → per-mode factor
#define HOLD_TO_CTRL      10000   // hold btn 4 this long → controller pad brightness

// ── Synced state from the grid (for at-limit display) ──
struct { uint8_t brightness; uint8_t globalBrightness; float animSpeed; uint8_t hueBase,hueAuto,hueSpeed,strobeHue;
         uint8_t tintR,tintG,tintB, sColR,sColG,sColB;
         uint16_t beatMs; uint8_t beatPhase; uint32_t beatAt;
         int16_t animIndex; uint8_t modeFactor; uint8_t strobeSpeed; uint16_t autoDwellS; uint16_t sweepPeriodS; } gs = {40,BRI_DEFAULT,1.0f,0,0,0,0, 255,0,0, 200,200,200, 0,0,0, 0, BRICOEF_MAX, STROBE_SPEED_DEF, AUTO_INT_DEF, SWEEP_INT_DEF};

// ── Brightness edit mode (button 4 / Helligkeit) ──
// tap 4 → GLOBAL (slow pulse) · hold 2s → MODE factor (fast pulse) · hold 10s →
// CONTROLLER pad (solid white). A tap while in any of them exits to NONE.
enum { BRI_NONE, BRI_GLOBAL, BRI_MODE, BRI_CTRL };
int briMode=BRI_NONE;
uint8_t ctrlBri=CTRL_BRI_DEFAULT;      // this pad's own dimming factor (0..255), scaled further by globalBrightness
Preferences ctrlPrefs;
bool ctrlBriDirty=false; uint32_t ctrlBriDirtyAt=0;   // debounce NVS writes (flash wear)

// ── Pad physically mounted rotated 90° clockwise ──
// The NeoTrellis is mounted turned 90° CW, so the key the chip calls NUM 12 is at
// the operator's TOP-LEFT, NUM 0 at the top-right, etc. These two tables cancel
// that rotation so the VISUAL layout the operator sees and presses is the standard
// one — Auto top-left, Color, +/- on the right, … Strobe bottom-right — while the
// whole interaction logic below keeps its plain LOGICAL numbering (0=Auto, 3/7=+/-,
// 15=Strobe …). Translate only at the two I/O edges: physical→logical in handleKey(),
// logical→physical when writing the LEDs in renderLEDs(). PHYS2LOG and LOG2PHYS are
// inverses, so a pressed key's own LED lands under it. (PHYS2LOG = the 90° CW map
// (r,c)→(c,3-r); LOG2PHYS its inverse. Verified against the wall: top-left→Auto,
// top-right→Mode+.)
static const uint8_t PHYS2LOG[16] = { 3, 7,11,15,  2, 6,10,14,  1, 5, 9,13,  0, 4, 8,12 };
static const uint8_t LOG2PHYS[16] = {12, 8, 4, 0, 13, 9, 5, 1, 14,10, 6, 2, 15,11, 7, 3 };

// ── Button map ── (all indices below are LOGICAL — see the rotation note above)
// param button → target id
// Button 4 (Helligkeit) is NOT a generic param — it drives its own brightness
// edit-mode state machine (global / per-mode / controller), so it is absent here.
int paramTarget(int b){ switch(b){case 8:return TOGA_P_SPEED;
                                 case 1:return TOGA_P_COLOR;case 12:return TOGA_P_STROBEHUE;
                                 case 5:return TOGA_P_HUESPEED;
                                 case 9:return TOGA_P_STROBESPEED;}  // phys 10: global strobe speed (tap to toggle)
                        return -1; }
bool isParamBtn(int b){ return paramTarget(b)>=0; }

bool     btnDown[16]={}; uint32_t btnDownT[16]={};
bool     plusHit=false, minusHit=false;   // +/- press latched in handleKey (see there)

// ── Auto mood (button 0) ──
// Tap cycles 1→2→3→4→1; there is no "off" in the cycle. It turns off only when
// the mode is taken over by hand (+/-), which is the moment the operator has
// stopped wanting the automatic. Rides out on every packet — see toga_proto.h.
uint8_t autoMode=TOGA_AUTO_OFF;

// ── Blackout (button 10) ──
// A LATCH: each press flips it, and it rides every packet in the flags (like the
// strobes). RAM only, never persisted — a reboot or a controller that goes away
// must bring the wall back, never boot into black. The grid/modules gate it on
// senderAlive, so switching this unit off also releases it.
bool gBlackout=false;

// ── Speed sweep (phys key 4 / logical 2) ──
// A LATCH like blackout: each press flips it, rides every packet in the flags. While
// on, the GRID sweeps the animation speed min→max→min on an absolute clock (see
// TOGA_F_SWEEP / togaSweepSpeed in toga_proto.h). RAM only; the grid gates it on
// senderAlive, so a controller that goes away releases the sweep.
bool gSweep=false;

// ── Profile: Togalicht / Drahthaus (v11) ──
// Owned HERE (NVS) and sent on every packet; the grid and modules only follow.
// Togalicht = everything as before. Drahthaus = the concert: +/- walk the setlist
// below (absolute TOGA_CMD_MODESET jumps), the wall shows no mode numbers, and
// key 13 (phys 11, Castle) opens the text-strobe layer instead of CASTLE/2026.
// Every other key is identical in both profiles.
uint8_t  gProfile=TOGA_PROFILE_TOGA;
int16_t  togaModeBefore=-1;          // mode the wall was on when we left Togalicht → restored on return
// Profile-select gesture: the top row's three left keys (phys 0/1/2 = logical
// 3/7/11: Mode+ / Mode- / Beat) held together for PROFILE_HOLD_MS. The pad then
// blinks red and the bottom row offers phys 12 (logical 0) = Togalicht, phys 13
// (logical 4) = Drahthaus. Side effect, accepted: before the 5s are up, the
// first press already stepped +1/-1 (net zero) and sent one beat tap.
#define PROFILE_HOLD_MS     5000
#define PROFILE_SEL_TIMEOUT 10000
bool     profSel=false; uint32_t profSelT=0;
// Text-strobe layer (Drahthaus only): tap 13 → only the six text keys light;
// HOLD one → that text strobes on the wall (textStrobe rides every packet);
// tap 13 again → back. No timeout: a song may want several texts in a row.
// TEXT_KEYS[i] = logical key for text id i+1. They sit in the BOTTOM TWO ROWS
// (phys 2,6,14 / 3,7,15), so the top half stays fully live inside the layer:
// Mode± (phys 0/1), Colour (8), Hue-speed (9), Brightness (13), Auto (12),
// Sweep (4), Spot (5) — plus Strobe-speed (phys 10) and the layer key (11).
// Layer view (physical):   14 Elefant · 10 StrSpd · 6 ALLE · 2 RAVE!
//                          15 StFarbe · 11 zu   · 7 DRAHT/HAUS · 3 DHA/RTU/AHS
bool     textLayer=false; uint8_t textHeld=TOGA_TEXT_NONE;
static const uint8_t TEXT_KEYS[TOGA_TEXT_COUNT]={11,10,8,15,14};     // phys 2 6 14 3 7 (RAVE ALLE Elefant Quadrat DRAHT/HAUS); phys 15 stays Strobe-colour
static inline bool isTextKey(int b){ if(!textLayer) return false; for(uint8_t i=0;i<TOGA_TEXT_COUNT;i++) if(TEXT_KEYS[i]==b) return true; return false; }
// Setlist: the Drahthaus mode order, editable on the phone page, kept in NVS.
// Default order: 0 = the calibration view (setupGrid, mode 0), 1 = the marker
// (the neon-lit submarine = "top of list"), then the songs, then the DJ modes.
#define SETLIST_MAX 32
uint8_t  setlist[SETLIST_MAX]; uint8_t setN=0; uint8_t setPos=0;
bool     setDirty=false; uint32_t setDirtyAt=0;
static const uint8_t SETLIST_DEFAULT[]={0,TOGA_MODE_SCHWARZ,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,34,57,66,102,73};

// ── Interaction state machine ──
enum { ST_DEFAULT, ST_ADJUST };
int  state=ST_DEFAULT, activeBtn=-1, activeTarget=-1;
int  suppressBtn=-1;                 // button that just deactivated a param (ignore its hold until released)
uint32_t plusPressT=0,minusPressT=0,plusRepT=0,minusRepT=0;

uint32_t C(uint8_t r,uint8_t g,uint8_t b){ return trellis.pixels.Color(r,g,b); }

// ── ESP-NOW ──
// Commands go out as a broadcast so the grid and every LED module hear the
// same packet. The grid unicasts nothing back any more — it broadcasts its
// absolute state, which is what keeps the meters below honest.
volatile bool gsSyncSeen=false;
uint8_t  gModeHue[128]={}; uint8_t gModeHueN=0;
uint8_t  gTxOn=0, gTxHue[8]={}, gTxSat[8]={};       // text-strobe colours (from the same table)    // grid's per-mode colour table (TOGA_MSG_HUETAB), for the colour pickers                // have we ever heard the grid? drives the page's grid-freshness pill
volatile uint32_t gsSyncAt=0;                  // last sync arrival (millis) — the page's grid-freshness
void onReceive(const uint8_t*mac,const uint8_t*data,int len){
  // A module announcing itself — park it in the ring, loop() merges it into the
  // table. Returns here so a hello never falls through into the sync path.
  TogaHelloMsg hm;
  if(togaParseHello(data,len,&hm)){
    uint8_t nh=(uint8_t)((hqHead+1)%HQ_LEN);
    if(nh!=hqTail){ memcpy((void*)&helloQ[hqHead],&hm,sizeof(hm)); hqHead=nh; }
    return;
  }
  TogaHueTabMsg ht;
  if(togaParseHueTab(data,len,&ht)){ memcpy(gModeHue,ht.hue,sizeof(gModeHue)); gModeHueN=ht.count;
    gTxOn=ht.txOn; memcpy(gTxHue,ht.txHue,8); memcpy(gTxSat,ht.txSat,8); return; }
  TogaSyncMsg m;
  if(!togaParseSync(data,len,&m)) return;
  gsSyncSeen=true; gsSyncAt=millis();
  gs.brightness=m.brightness; gs.globalBrightness=m.globalBrightness; gs.animSpeed=m.animSpeed; gs.hueBase=m.hueBase; gs.hueSpeed=m.hueSpeed; gs.strobeHue=m.strobeHue;
  gs.hueAuto=m.hueAuto;                       // the grid's live rotation — button 0 turns with it
  gs.tintR=m.tintR; gs.tintG=m.tintG; gs.tintB=m.tintB; gs.sColR=m.sColR; gs.sColG=m.sColG; gs.sColB=m.sColB;
  // Beat: the grid owns the tempo, we only mirror it so button 11 can pulse on
  // it — that blink is the operator's proof the grid actually locked on.
  gs.beatMs=m.beatMs; gs.beatPhase=m.beatPhase; gs.beatAt=millis();
  gs.animIndex=m.animIndex;                    // for the module page's grid-mode readout
  gs.modeFactor=m.modeFactor;                  // current mode's brightness factor — drives the +/- at-limit blink
  gs.strobeSpeed=m.strobeSpeed;                // global strobe speed — button-9 meter + at-limit blink
  gs.autoDwellS=m.autoDwellS; gs.sweepPeriodS=m.sweepPeriodS;   // web-UI timing header (echoed for display)
}
uint16_t txSeq=0;
// The live pad state every packet carries (flags, auto, profile, held text).
// Filled at SEND time — also for a delayed repeat, so a repeat can never
// re-assert a strobe that was released in the meantime.
void fillLive(TogaCmdMsg& m){
  // Inside the text layer the strobe keys are text keys, so their flags stay off;
  // the castle flag exists only in the Togalicht profile (Drahthaus: 13 = text layer).
  // A key that is a TEXT key in the open layer never sends its normal flag.
  auto live=[&](int k){ return !profSel && btnDown[k] && !isTextKey(k); };
  m.flags=(live(15)?TOGA_F_STROBE:0)|(live(14)?TOGA_F_MODE_STROBE:0)
         |((live(13)&&gProfile==TOGA_PROFILE_TOGA)?TOGA_F_CASTLE_STROBE:0)
         |(gBlackout?TOGA_F_BLACKOUT:0)|(live(6)?TOGA_F_SPOT:0)|(gSweep?TOGA_F_SWEEP:0); // btn6 (phys 5): spot · logical 2 (phys 4): speed-sweep latch
  m.autoMode=autoMode;                                       // state, carried by every packet
  m.profile=gProfile;                                        // v11: profile + held text, every packet
  m.textStrobe=textLayer?textHeld:TOGA_TEXT_NONE;
}
// Unicast retried in hardware; broadcast is unacked and never retried. The
// heartbeat re-sends the strobe flags 10x/s anyway, but mode/param/beat are
// one-shots and would be silently swallowed. Send them 3x under ONE seq —
// receivers apply the first and drop the repeats. The repeats used to go out
// back-to-back within microseconds, so one burst of interference (our own AP
// talking to the phone, a beacon) ate all three and the press was simply lost.
// Now: copy 1 at once (no added latency), copies 2/3 at +REP1/+REP2 ms from a
// small queue serviced at the top of loop(). The grid dedupes the last 8 seqs,
// so interleaved copies of two quick presses are still applied exactly once.
#define REP1_MS 8
#define REP2_MS 25
#define REPQ_N  12
struct RepSlot { TogaCmdMsg m; uint32_t due; uint8_t left; };
RepSlot repQ[REPQ_N]={};
void sendCmd(uint8_t cmd,uint8_t target,int16_t arg){
  TogaCmdMsg m={};
  togaInitHeader(m.h,TOGA_MSG_CMD,TOGA_GROUP_ALL,++txSeq);   // group 0 = everyone; targeting UI comes later
  m.cmd=cmd; m.target=target; m.arg=arg;
  fillLive(m);
  esp_now_send(TOGA_BCAST,(const uint8_t*)&m,sizeof(m));
  if(cmd==TOGA_CMD_HEARTBEAT) return;
  RepSlot* s=nullptr;                                        // free slot, else the one due soonest (queue full = a burst; oldest repeat is the least needed)
  for(int i=0;i<REPQ_N;i++) if(!repQ[i].left){ s=&repQ[i]; break; }
  if(!s){ s=&repQ[0]; for(int i=1;i<REPQ_N;i++) if((int32_t)(repQ[i].due-s->due)<0) s=&repQ[i]; }
  s->m=m; s->due=millis()+REP1_MS; s->left=2;
}
void serviceRepeats(uint32_t t){
  for(int i=0;i<REPQ_N;i++){ RepSlot& s=repQ[i];
    if(!s.left || (int32_t)(t-s.due)<0) continue;
    fillLive(s.m);                                           // same seq/cmd/arg, CURRENT flags
    esp_now_send(TOGA_BCAST,(const uint8_t*)&s.m,sizeof(s.m));
    if(--s.left) s.due=t+(REP2_MS-REP1_MS);
  }
}

// ── Module manager: table maintenance + config send ─────
// Find this MAC, or claim a row for it. A full table means more boards than
// MAX_MODULES have ever been heard, so the stalest row is the one nobody will
// miss. Never returns null: there is always a row to recycle.
ModEntry* upsert(const uint8_t mac[6]){
  for(int i=0;i<MAX_MODULES;i++)
    if(mods[i].used && memcmp(mods[i].mac,mac,6)==0) return &mods[i];
  ModEntry* slot=nullptr;
  for(int i=0;i<MAX_MODULES;i++) if(!mods[i].used){ slot=&mods[i]; break; }
  if(!slot){ slot=&mods[0]; for(int i=1;i<MAX_MODULES;i++) if(mods[i].lastSeen<slot->lastSeen) slot=&mods[i]; }
  memset(slot,0,sizeof(*slot));
  slot->used=true; memcpy(slot->mac,mac,6);
  return slot;
}
void drainHellos(uint32_t t){
  while(hqTail!=hqHead){
    TogaHelloMsg m; memcpy(&m,(const void*)&helloQ[hqTail],sizeof(m));
    hqTail=(uint8_t)((hqTail+1)%HQ_LEN);
    ModEntry* e=upsert(m.mac);
    e->group=m.group; e->effectPin=m.effectPin; e->effectNow=m.effectNow;
    e->gain=m.gain; e->pin=m.pin; e->linkAlive=m.linkAlive;
    e->w=m.width; e->h=m.height; e->uptimeS=m.uptimeS;
    e->battMv=m.battMv; e->battFlags=m.battFlags;
    e->followMask=m.followMask;
    e->gap=m.gap;
    e->floor=m.floorFlow;
    e->fwBuild=m.fwBuild;
    e->localHue=m.localHue;
    memcpy(e->name,m.name,sizeof(e->name)); e->name[sizeof(e->name)-1]=0;
    e->lastSeen=t;
  }
  for(int i=0;i<MAX_MODULES;i++)                        // long-gone rows expire
    if(mods[i].used && (t-mods[i].lastSeen)>FORGET_MS) mods[i].used=false;
}
String macStr(const uint8_t m[6]){
  char b[18]; snprintf(b,sizeof(b),"%02X:%02X:%02X:%02X:%02X:%02X",m[0],m[1],m[2],m[3],m[4],m[5]);
  return String(b);
}
bool parseMac(const String& s,uint8_t out[6]){
  int v[6];
  if(sscanf(s.c_str(),"%x:%x:%x:%x:%x:%x",&v[0],&v[1],&v[2],&v[3],&v[4],&v[5])!=6) return false;
  for(int i=0;i<6;i++){ if(v[i]<0||v[i]>255) return false; out[i]=(uint8_t)v[i]; }
  return true;
}
// Settings for ONE module, addressed by MAC. Broadcast is unacked/never retried,
// so send 3x — these are absolute values, so applying one twice is a no-op and
// needs no dedup on the module side. Reuses the shared broadcast peer already
// registered on TOGA_CHANNEL (no channel-0 hack: we ARE the AP on that channel).
void sendModCfg(const uint8_t mac[6],uint8_t effectPin,uint8_t gain,uint8_t battMon=TOGA_CFG_KEEP,
                uint8_t followMask=TOGA_FOLLOW_KEEP,uint16_t ledCount=0,const char* name=nullptr,
                uint8_t gap=TOGA_GAP_KEEP,uint8_t floorFlow=TOGA_FLOOR_KEEP,
                uint8_t localHue=TOGA_LOCALHUE_KEEP){
  TogaModCfgMsg m={};
  togaInitHeader(m.h,TOGA_MSG_MODCFG,TOGA_GROUP_ALL,++txSeq);
  memcpy(m.mac,mac,6);
  m.effectPin=effectPin; m.gain=gain; m.battMon=battMon;
  m.followMask=followMask; m.ledCount=ledCount; m.gap=gap; m.floorFlow=floorFlow;
  m.localHue=localHue;   // KEEP by default — m is zero-init and 0 means "native", not "leave alone"
  if(name && name[0]) strncpy(m.name,name,sizeof(m.name)-1);   // m zero-init → NUL kept; "" = keep
  for(int i=0;i<3;i++) esp_now_send(TOGA_BCAST,(const uint8_t*)&m,sizeof(m));
}
// Minimal JSON string escaper for the user-set name (handleSet already strips it).
String jsonEsc(const char* s){ String o; for(const char* p=s;*p;p++){ char c=*p;
  if(c=='"'||c=='\\'){ o+='\\'; o+=c; } else if((uint8_t)c>=0x20) o+=c; } return o; }

// ── Module manager: HTTP ────────────────────────────────
// We host the AP, so WiFi.channel() is always TOGA_CHANNEL; the chOk field is
// kept for page compatibility but the mismatch warning simply never fires.
bool channelOk(){ return WiFi.channel()==TOGA_CHANNEL; }
void handleModules(){
  uint32_t t=millis();
  String j="{\"ch\":"+String(WiFi.channel())+
           ",\"chWant\":"+String(TOGA_CHANNEL)+
           ",\"chOk\":"+String(channelOk()?"true":"false")+
           ",\"gridSeen\":"+String((gsSyncSeen && (t-gsSyncAt)<2000)?"true":"false")+
           ",\"gridMode\":"+String(gsSyncSeen?gs.animIndex+1:0)+
           ",\"gridIdx\":"+String(gsSyncSeen?gs.animIndex:-1)+
           ",\"profile\":"+String(gProfile)+",\"setPos\":"+String(setPos)+",\"setlist\":"+setlistJson()+
           ",\"mhue\":"+modeHueJson()+",\"nhue\":"+nativeHueJson()+",\"tcol\":"+textColJson()+
           ",\"autoInt\":"+String(gs.autoDwellS)+
           ",\"sweepInt\":"+String(gs.sweepPeriodS)+
           ",\"effects\":[";
  for(uint8_t e=0;e<TOGA_EFFECT_COUNT;e++) j+=(e?",\"":"\"")+String(togaEffectName(e))+"\"";
  j+="],\"locals\":[";
  for(uint8_t e=0;e<TOGA_LOCAL_COUNT;e++) j+=(e?",\"":"\"")+String(togaLocalEffectName(e))+"\"";
  j+="],\"autos\":[";
  for(uint8_t e=0;e<TOGA_LOCAL_AUTO_COUNT;e++) j+=(e?",\"":"\"")+String(togaAutoProgName(e))+"\"";
  j+="],\"localBase\":"+String(TOGA_EFFECT_LOCAL_BASE)+
     ",\"autoBase\":"+String(TOGA_EFFECT_AUTO_BASE)+
     ",\"follow\":"+String(TOGA_EFFECT_FOLLOW)+",\"mods\":[";
  bool first=true;
  for(int i=0;i<MAX_MODULES;i++){
    if(!mods[i].used) continue;
    if(!first) j+=","; first=false;
    uint32_t age=t-mods[i].lastSeen;
    j+="{\"mac\":\""+macStr(mods[i].mac)+"\""+
       ",\"online\":"+String(age<OFFLINE_MS?"true":"false")+
       ",\"age\":"+String(age/1000)+
       ",\"grp\":"+String(mods[i].group)+
       ",\"eff\":"+String(mods[i].effectPin)+
       ",\"effNow\":"+String(mods[i].effectNow)+
       ",\"gain\":"+String(mods[i].gain)+
       ",\"pin\":"+String(mods[i].pin)+
       ",\"w\":"+String(mods[i].w)+",\"h\":"+String(mods[i].h)+
       ",\"name\":\""+jsonEsc(mods[i].name)+"\""+
       ",\"follow\":"+String(mods[i].followMask)+
       ",\"gap\":"+String(mods[i].gap)+
       ",\"floor\":"+String(mods[i].floor)+
       ",\"lhue\":"+String(mods[i].localHue)+
       ",\"batt\":"+String(mods[i].battMv/1000.0,2)+
       ",\"bmon\":"+String((mods[i].battFlags&TOGA_BATT_F_ENABLED)?"true":"false")+
       ",\"blow\":"+String((mods[i].battFlags&TOGA_BATT_F_LOW)?"true":"false")+
       ",\"fw\":"+String(mods[i].fwBuild)+
       ",\"link\":"+String(mods[i].linkAlive?"true":"false")+
       ",\"up\":"+String(mods[i].uptimeS)+"}";
  }
  j+="]}";
  server.send(200,"application/json",j);
}
void handleSet(){
  uint8_t mac[6];
  if(!server.hasArg("mac")||!parseMac(server.arg("mac"),mac)){ server.send(400,"text/plain","bad mac"); return; }
  uint8_t eff=TOGA_CFG_KEEP, gain=0;               // 0 / KEEP = "leave this one alone"
  if(server.hasArg("effect")){
    int e=server.arg("effect").toInt();
    bool ok = e==TOGA_EFFECT_FOLLOW || e==TOGA_EFFECT_OFF || e==TOGA_EFFECT_IDENTIFY
           || (e>=0 && e<TOGA_EFFECT_COUNT)
           || togaIsLocalEffect((uint8_t)e) || togaIsAutoEffect((uint8_t)e);
    if(!ok){ server.send(400,"text/plain","bad effect"); return; }
    eff=(uint8_t)e;
  }
  uint8_t lhue=TOGA_LOCALHUE_KEEP;
  if(server.hasArg("color")){                      // independent-mode colour: 0 = native, 1..255 = hue
    int h=server.arg("color").toInt();
    if(h<0||h>255||h==TOGA_LOCALHUE_KEEP){ server.send(400,"text/plain","bad color"); return; }
    lhue=(uint8_t)h;
  }
  if(server.hasArg("gain")){
    int g=server.arg("gain").toInt();
    if(g<10||g>200){ server.send(400,"text/plain","bad gain"); return; }
    gain=(uint8_t)g;
  }
  uint8_t follow=TOGA_FOLLOW_KEEP; uint16_t leds=0; char name[16]={0};
  if(server.hasArg("follow")){
    int f=server.arg("follow").toInt();
    if(f<0||f>TOGA_FOLLOW_ALL){ server.send(400,"text/plain","bad follow"); return; }
    follow=(uint8_t)f;
  }
  if(server.hasArg("leds")){
    int n=server.arg("leds").toInt();
    if(n<1||n>512){ server.send(400,"text/plain","bad leds"); return; }   // module re-clamps to its MAX_LEDS
    leds=(uint16_t)n;
  }
  uint8_t gap=TOGA_GAP_KEEP;
  if(server.hasArg("gap")){                          // 1 = every LED (off), N = light every Nth
    int gp=server.arg("gap").toInt();
    if(gp<TOGA_GAP_OFF||gp>TOGA_GAP_MAX){ server.send(400,"text/plain","bad gap"); return; }
    gap=(uint8_t)gp;
  }
  uint8_t floor=TOGA_FLOOR_KEEP;
  if(server.hasArg("floor")){                        // 0 off, 1 fwd, 2 rev
    int f=server.arg("floor").toInt();
    if(f<TOGA_FLOOR_OFF||f>TOGA_FLOOR_MAX){ server.send(400,"text/plain","bad floor"); return; }
    floor=(uint8_t)f;
  }
  if(server.hasArg("name")){
    String s=server.arg("name"); int k=0;
    for(int i=0;i<(int)s.length()&&k<15;i++){ char c=s[i];
      if(isalnum((int)c)||c==' '||c=='-'||c=='_') name[k++]=c; }
    name[k]=0;
    if(k==0){ server.send(400,"text/plain","bad name"); return; }
  }
  uint8_t battMon=TOGA_CFG_KEEP;
  if(server.hasArg("bmon")) battMon=server.arg("bmon").toInt()?1:0;
  sendModCfg(mac,eff,gain,battMon,follow,leds,name,gap,floor,lhue);
  // No optimistic echo: the row updates when the module's next hello confirms it
  // actually took. The page shows what IS, not what we hope.
  server.send(200,"application/json","{\"sent\":true}");
}
// ── Drahthaus setlist / profile (v11) ──────────────────────────────────────
// [[name,own,hue,sat],…] for text ids 1..TOGA_TEXT_COUNT
String textColJson(){ String j="["; for(int k=1;k<=TOGA_TEXT_COUNT;k++){ if(k>1) j+=",";
  j+="[\""+String(togaTextName(k))+"\","+String((gTxOn>>k)&1)+","+String(gTxHue[k])+","+String(gTxSat[k])+"]"; } return j+"]"; }
// /api/textcol?id=1..5&h=0..255&s=0..255   or   &reset=1 (back to the strobe colour)
void handleTextCol(){
  if(!server.hasArg("id")){ server.send(400,"text/plain","id"); return; }
  int id=server.arg("id").toInt(); if(id<1||id>TOGA_TEXT_COUNT){ server.send(400,"text/plain","range"); return; }
  if(server.hasArg("reset")){ sendCmd(TOGA_CMD_TEXTCOL,(uint8_t)(id|0x80),0); gTxOn&=~(1<<id); }
  else { int h=constrain(server.arg("h").toInt(),0,255), s=constrain(server.arg("s").toInt(),0,255);
         sendCmd(TOGA_CMD_TEXTCOL,(uint8_t)id,(int16_t)((h<<8)|s)); gTxOn|=(1<<id); gTxHue[id]=h; gTxSat[id]=s; }
  server.send(200,"application/json","{\"sent\":true}");
}
String modeHueJson(){ String j="["; for(int i=0;i<gModeHueN && i<TOGA_EFFECT_COUNT;i++){ if(i) j+=","; j+=String(gModeHue[i]); } return j+"]"; }
String nativeHueJson(){ String j="["; for(int i=0;i<TOGA_EFFECT_COUNT;i++){ if(i) j+=","; j+=String(TOGA_MODE_NATIVE_HUE[i]); } return j+"]"; }
// /api/modehue?m=<mode>&h=<0..255 hue offset> → the grid stores it as that mode's colour
void handleModeHue(){
  if(!server.hasArg("m")||!server.hasArg("h")){ server.send(400,"text/plain","m,h"); return; }
  int md=server.arg("m").toInt(), h=server.arg("h").toInt();
  if(md<0||md>=TOGA_EFFECT_COUNT||md>127||h<0||h>255){ server.send(400,"text/plain","range"); return; }
  sendCmd(TOGA_CMD_PARAM,TOGA_P_MODEHUE,(int16_t)(md*256+h));
  if(md<gModeHueN) gModeHue[md]=(uint8_t)h;                   // optimistic, the next table confirms
  server.send(200,"application/json","{\"sent\":true}");
}
String setlistJson(){ String j="["; for(uint8_t i=0;i<setN;i++){ if(i) j+=","; j+=String(setlist[i]); } return j+"]"; }
// GET /api/setlist            → the list   ·   /api/setlist?list=107,108,… → replace it
void handleSetlist(){
  if(server.hasArg("list")){
    String l=server.arg("list"); uint8_t tmp[SETLIST_MAX]; uint8_t n=0; int from=0;
    while(from<=(int)l.length() && n<SETLIST_MAX){
      int to=l.indexOf(',',from); if(to<0) to=l.length();
      String tok=l.substring(from,to); tok.trim(); from=to+1;
      if(!tok.length()) continue;
      int v=tok.toInt(); if(v<0||v>=TOGA_EFFECT_COUNT){ server.send(400,"text/plain","bad mode"); return; }
      tmp[n++]=(uint8_t)v;
    }
    if(n==0){ server.send(400,"text/plain","empty"); return; }
    uint8_t cur=setN?setlist[setPos]:255;
    memcpy(setlist,tmp,n); setN=n;
    setPos=0; for(uint8_t i=0;i<setN;i++) if(setlist[i]==cur){ setPos=i; break; }   // keep standing on the same song if it survived
    saveSetlist();
  }
  server.send(200,"application/json","{\"profile\":"+String(gProfile)+",\"pos\":"+String(setPos)+",\"list\":"+setlistJson()+"}");
}
void handleJump(){
  if(!server.hasArg("pos")){ server.send(400,"text/plain","no pos"); return; }
  int p=server.arg("pos").toInt(); if(p<0||p>=setN){ server.send(400,"text/plain","bad pos"); return; }
  setlistGoto((uint8_t)p);
  server.send(200,"application/json","{\"sent\":true}");
}
void handleProfile(){
  if(!server.hasArg("p")){ server.send(400,"text/plain","no p"); return; }
  int p=server.arg("p").toInt(); if(p!=TOGA_PROFILE_TOGA && p!=TOGA_PROFILE_DRAHTHAUS){ server.send(400,"text/plain","bad p"); return; }
  setProfile((uint8_t)p);
  server.send(200,"application/json","{\"profile\":"+String(gProfile)+"}");
}
// Global timing header: auto-mode dwell and sweep period, in seconds. These live
// on the GRID (it owns the show clock); we forward ABSOLUTE-value PARAM commands
// and the grid echoes the applied values back in its sync for the page to show.
void handleTiming(){
  bool any=false;
  if(server.hasArg("auto")){
    int s=constrain(server.arg("auto").toInt(),TIMING_INT_MIN,TIMING_INT_MAX);
    sendCmd(TOGA_CMD_PARAM,TOGA_P_AUTOINT,(int16_t)s); gs.autoDwellS=(uint16_t)s; any=true;
  }
  if(server.hasArg("sweep")){
    int s=constrain(server.arg("sweep").toInt(),TIMING_INT_MIN,TIMING_INT_MAX);
    sendCmd(TOGA_CMD_PARAM,TOGA_P_SWEEPINT,(int16_t)s); gs.sweepPeriodS=(uint16_t)s; any=true;
  }
  if(!any){ server.send(400,"text/plain","no field"); return; }
  server.send(200,"application/json","{\"sent\":true}");
}
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>TOGA Module</title><style>
body{font:15px system-ui,sans-serif;margin:0;padding:12px;background:#111;color:#eee}
h1{font-size:17px;margin:0 0 10px}
.warn{background:#7a1717;padding:10px;border-radius:6px;margin-bottom:10px;line-height:1.45}
.bar{color:#8a8a8a;font-size:13px;margin-bottom:10px}
.timing{background:#1c1c1c;border-radius:8px;padding:10px;margin-bottom:8px;display:flex;gap:16px;align-items:center;flex-wrap:wrap}
.timing .tl{font-weight:600;color:#eee}
.timing label{min-width:auto;color:#cfcfcf;display:inline-flex;align-items:center;gap:6px;font-size:13px}
.timing input{width:68px}
.m{background:#1c1c1c;border-radius:8px;padding:10px;margin-bottom:8px;border-left:4px solid #2c2c2c}
.m.on{border-left-color:#2ea043}.m.off{border-left-color:#7a1717;opacity:.65}
.mac{font-family:ui-monospace,monospace;font-size:13px}
.meta{color:#8a8a8a;font-size:12px;margin:3px 0 8px}
.row{display:flex;gap:8px;align-items:center;margin-top:6px;flex-wrap:wrap}
select,input[type=range],input[type=text],input[type=number]{background:#2a2a2a;color:#eee;border:1px solid #3a3a3a;border-radius:5px;padding:5px}
input[type=range]{padding:0;flex:1;min-width:120px}
input[type=number]{width:74px}
.nm{font-size:15px;font-weight:600;min-width:110px;flex:1}
.slave{gap:14px}
.slave label{min-width:auto;color:#cfcfcf;display:inline-flex;align-items:center;gap:4px}
.hint{font-size:11px;color:#7a7a7a}
label{font-size:12px;color:#8a8a8a;min-width:52px}
.pill{font-size:11px;padding:2px 6px;border-radius:9px;background:#2c2c2c;color:#9a9a9a}
.pill.bad{background:#7a1717;color:#fff}
.sws{display:inline-flex;gap:6px;flex-wrap:wrap}
.sw{width:22px;height:22px;border-radius:50%;border:2px solid #333;cursor:pointer;display:inline-block}
.sw.sel{border-color:#fff;box-shadow:0 0 0 2px rgba(255,255,255,.2)}
.empty{color:#8a8a8a;padding:20px 0;line-height:1.5}
button{background:#2a2a2a;color:#eee;border:1px solid #3a3a3a;border-radius:5px;padding:4px 9px;font:inherit;font-size:12px;cursor:pointer}
button:hover{background:#3a3a3a}
.idf{margin-left:auto}
.kz{background:#1c1c1c;border-radius:8px;padding:10px;margin-bottom:8px}
.kz .tl{font-weight:600;color:#eee;margin-right:8px}
.pb{padding:5px 12px;border-radius:14px}
.pb.sel{background:#2ea043;border-color:#2ea043;color:#fff}
.pb.dh.sel{background:#d85a00;border-color:#d85a00}
.sr{display:flex;gap:6px;align-items:center;padding:4px 6px;border-radius:5px;border-left:3px solid transparent}
.sr.cur{background:#263326}
.sr.live{border-left-color:#2ea043}
.sn{width:22px;color:#8a8a8a;font-size:12px;text-align:right}
.sm{flex:1;font-size:14px}
.sr button{padding:3px 7px}
#sl{margin-top:6px}
.kzadd{margin-top:8px;display:flex;gap:6px;align-items:center;flex-wrap:wrap}
.kzadd select{max-width:220px}
.mh{display:flex;gap:8px;align-items:center;padding:4px 6px;flex-wrap:wrap}
.mh .sm{flex:1;min-width:150px}
.mh input[type=color]{width:42px;height:28px;padding:0;border:1px solid #3a3a3a;background:#2a2a2a;border-radius:5px}
.mh input.hx{width:78px;font-family:ui-monospace,monospace}
.fold>summary{cursor:pointer;list-style:none;display:flex;gap:8px;align-items:center;flex-wrap:wrap;user-select:none}
.fold>summary::-webkit-details-marker{display:none}
.fold .ar{color:#8a8a8a;display:inline-block;transition:transform .15s}
.fold[open] .ar{transform:rotate(90deg)}
.fold .fh{margin:8px 0 4px}
</style></head><body>
<h1>TOGA Module</h1>
<div id="warn"></div><div class="bar" id="bar">…</div>
<div class="timing">
  <span class="tl">Intervalle</span>
  <label>Auto-Modus <input type="number" id="autoInt" min="5" max="600"> s</label>
  <label>Sweep <input type="number" id="sweepInt" min="5" max="600"> s</label>
  <span class="hint">Modus-Wechsel im Auto-Modus &middot; Speed-Sweep min&rarr;max&rarr;min</span>
</div>
<div class="kz">
  <div class="row"><span class="tl">Profil</span>
    <button class="pb" id="pt" onclick="setProfile(0)">Togalicht</button>
    <button class="pb dh" id="pd" onclick="setProfile(1)">Drahthaus</button>
    <span class="hint">am Pad: Mod+ &middot; Mod&minus; &middot; Beat 5&nbsp;s halten</span></div>
  <div class="row" style="margin-top:10px"><span class="tl">Setlist</span><span class="hint" id="slh"></span></div>
  <div id="sl"></div>
  <div class="kzadd"><select id="sladd"></select><button onclick="slAdd()">+ Hinzuf&uuml;gen</button>
    <span class="hint">&#9654; springt hin &middot; Reihenfolge mit &#9650;&#9660; &middot; Mod+/&minus; am Pad steppen hier durch (Drahthaus)</span></div>
</div>
<details class="kz fold" id="fmh">
  <summary><span class="ar">&#9656;</span><span class="tl">Farben der Konzertmodi</span><span class="hint" id="mhn"></span></summary>
  <div class="hint fh">Farbe w&auml;hlen (Picker oder Hex) &middot; &bdquo;Nativ&ldquo; = Originalfarben &middot; gilt pro Modus, wie Taste 8</div>
  <div id="mh"></div>
</details>
<details class="kz fold" id="ftc">
  <summary><span class="ar">&#9656;</span><span class="tl">Farben der Text-Strobes</span><span class="hint" id="tcn"></span></summary>
  <div class="hint fh">pro Text eine eigene Farbe &middot; &bdquo;Strobe&ldquo; = wie die Strobe-Farbe (Taste 15) &middot; Module blitzen in derselben Farbe</div>
  <div id="tc"></div>
</details>
<div id="list"></div>
<script>
let EFF=[],LOCALS=[],AUTOS=[],LBASE=100,ABASE=120,OPTS='',FOLLOW=255;
const F_STROBE=1,F_BRIGHT=2,F_MODE=4,F_BEAT=8,F_BLACK=16;   // mirror toga_proto.h
const IDENT=201;   // TOGA_EFFECT_IDENTIFY — momentary "blink to find this strip"
const macid=m=>m.replace(/:/g,'');
const fwtag=v=>(v||0).toString(16).padStart(4,'0');   // firmware build → 4-hex tag
function set(mac,q){fetch('/api/set?mac='+encodeURIComponent(mac)+'&'+q)}
// Global timing header (auto-mode dwell + sweep period, seconds). pend() locks the
// field the instant it is changed so the 1.5s poll cannot yank it mid-edit (same as
// the module fields); tick() writes the grid's echoed values back via put().
const autoIntEl=document.getElementById('autoInt'), sweepIntEl=document.getElementById('sweepInt');
autoIntEl.onchange=e=>{pend(e.target);fetch('/api/timing?auto='+encodeURIComponent(e.target.value))};
sweepIntEl.onchange=e=>{pend(e.target);fetch('/api/timing?sweep='+encodeURIComponent(e.target.value))};
// Independent-mode colour palette. h = FastLED hue sent as ?color= (0 = "Nativ",
// no tint); c = an approximate CSS colour for the swatch only. The KEEP sentinel
// (254) is never in this list, so choosing a colour never trips it.
const PAL=[{n:'Nativ',h:0,c:'#666'},{n:'Rot',h:2,c:'#ff0000'},{n:'Orange',h:24,c:'#ff7a00'},
  {n:'Gelb',h:64,c:'#ffe000'},{n:'Grün',h:96,c:'#12d100'},{n:'Türkis',h:128,c:'#00d0b0'},
  {n:'Blau',h:160,c:'#0060ff'},{n:'Violett',h:192,c:'#8a2be2'},{n:'Magenta',h:224,c:'#ff30c0'},{n:'Pink',h:244,c:'#ff5090'}];
// Grouped effect list: dance autos + standalone effects (both make the module
// independent of the grid) first, then "folgt Grid", then the raw grid modes.
function optsHTML(){ let h='<optgroup label="Auto-Tanz">';
  for(let i=0;i<AUTOS.length;i++)h+='<option value="'+(ABASE+i)+'">Auto: '+AUTOS[i]+'</option>';
  h+='</optgroup><optgroup label="Standalone">';
  for(let i=0;i<LOCALS.length;i++)h+='<option value="'+(LBASE+i)+'">'+LOCALS[i]+'</option>';
  h+='</optgroup><option value="'+FOLLOW+'">folgt Grid</option><optgroup label="Grid-Modi">';
  for(let i=0;i<EFF.length;i++)h+='<option value="'+i+'">'+i+' '+EFF[i]+'</option>';
  return h+'</optgroup>'; }
// Name of whatever effectPin is actually on screen (grid mode / local / auto).
const runName=e=>e>=ABASE?('Auto: '+(AUTOS[e-ABASE]||'?')):(e>=LBASE&&e<LBASE+LOCALS.length?(LOCALS[e-LBASE]||'?'):(EFF[e]||'?'));
function selSw(r,h){ r.sws.querySelectorAll('.sw').forEach(sw=>sw.classList.toggle('sel',(+sw.dataset.h)===h)); }
// ── Per-control confirm lock (fixes the two-phone glitch) ──────────────────
// pend() marks a control the moment the user changes it; put() then refuses to
// overwrite it until the module's own hello confirms the value (or ~4s pass),
// and never touches a focused control. So the poll cannot yank a dropdown, name
// field or checkbox out from under a second phone, and edits never snap back.
const valOf=el=>el.type==='checkbox'?(el.checked?'1':'0'):(''+el.value);
function pend(el){el._p=valOf(el);el._t=Date.now()}
function put(el,rep){ rep=''+rep;
  if(el===document.activeElement)return;
  if(el._p!=null){ if(el._p===rep){el._p=null} else if(Date.now()-el._t>4000){el._p=null} else return; }
  if(el.type==='checkbox')el.checked=(rep==='1');else el.value=rep;
}
// ── Drahthaus setlist / profile ──
let SL=[],SLKEY='';
function setProfile(p){fetch('/api/profile?p='+p);SLKEY=''}
function slSend(){fetch('/api/setlist?list='+SL.join(','));SLKEY=''}
function slMove(i,d){const j=i+d;if(j<0||j>=SL.length)return;const t=SL[i];SL[i]=SL[j];SL[j]=t;slSend()}
function slDel(i){if(SL.length<=1)return;SL.splice(i,1);slSend()}
function slAdd(){if(SL.length>=32)return;SL.push(+document.getElementById('sladd').value);slSend()}
function slJump(i){fetch('/api/jump?pos='+i);SLKEY=''}
function slRender(d){
  const key=JSON.stringify([d.profile,d.setPos,d.setlist,d.gridIdx]); if(key===SLKEY)return; SLKEY=key;
  document.getElementById('pt').className='pb'+(d.profile?'':' sel');
  document.getElementById('pd').className='pb dh'+(d.profile?' sel':'');
  document.getElementById('slh').textContent=d.profile?('Position '+d.setPos+' von '+d.setlist.length):'(nur im Profil Drahthaus aktiv)';
  SL=d.setlist.slice(); let h='';
  SL.forEach((m,i)=>{ const cur=(d.profile&&i==d.setPos), live=(m===d.gridIdx);
    h+='<div class="sr'+(cur?' cur':'')+(live?' live':'')+'"><span class="sn">'+i+'</span><span class="sm">'+(EFF[m]||m)+'</span>'+
       '<button onclick="slJump('+i+')" title="springen">&#9654;</button><button onclick="slMove('+i+',-1)">&#9650;</button>'+
       '<button onclick="slMove('+i+',1)">&#9660;</button><button onclick="slDel('+i+')" title="entfernen">&#10005;</button></div>'; });
  document.getElementById('sl').innerHTML=h;
}
// ── Per-mode colour pickers ──
// The grid stores a HUE OFFSET per mode (added to the mode's native colours). A
// picked colour is turned into a FastLED hue and the offset = picked − native.
// FastLED's rainbow hue is not CSS hue: map through its anchor points.
const FLH=[0,32,64,96,128,160,192,224,256], CSSH=[0,30,60,120,180,240,270,330,360];
function cssToFl(d){ d=((d%360)+360)%360; for(let i=0;i<8;i++) if(d<=CSSH[i+1]) return Math.round(FLH[i]+(d-CSSH[i])/(CSSH[i+1]-CSSH[i])*(FLH[i+1]-FLH[i]))&255; return 0; }
function flToCss(h){ h&=255; for(let i=0;i<8;i++) if(h<=FLH[i+1]) return CSSH[i]+(h-FLH[i])/(FLH[i+1]-FLH[i])*(CSSH[i+1]-CSSH[i]); return 0; }
function hslHex(hd){ const f=n=>{const k=(n+hd/30)%12, a=0.5; const v=0.5-a*Math.max(-1,Math.min(k-3,9-k,1)); return Math.round(v*255).toString(16).padStart(2,'0');}; return '#'+f(0)+f(8)+f(4); }
function hexHue(hx){ const m=/^#?([0-9a-f]{6})$/i.exec(hx.trim()); if(!m) return null; const n=parseInt(m[1],16), r=(n>>16&255)/255,g=(n>>8&255)/255,b=(n&255)/255;
  const mx=Math.max(r,g,b), mn=Math.min(r,g,b), d=mx-mn; if(d===0) return 0; let h;
  if(mx===r) h=((g-b)/d)%6; else if(mx===g) h=(b-r)/d+2; else h=(r-g)/d+4; return (h*60+360)%360; }
let MH={}, NH=[], MHKEY='';
function mhSet(m,off){ fetch('/api/modehue?m='+m+'&h='+(off&255)); MH[m]={off:off&255,t:Date.now()}; }
function mhNative(m){ mhSet(m,0); MHKEY=''; }
function mhPick(m,hx){ const d=hexHue(hx); if(d===null) return; mhSet(m,(cssToFl(d)-(NH[m]||0)+512)&255); MHKEY=''; }
function mhRender(d){
  if(!d.mhue||!d.mhue.length) { document.getElementById('mh').innerHTML='<span class="hint">warte auf das Grid &hellip;</span>'; return; }
  NH=d.nhue||[];
  const list=[]; d.setlist.forEach(m=>{ if(list.indexOf(m)<0) list.push(m); });
  const now=Date.now(); const val=m=>{ const p=MH[m]; if(p && now-p.t<4000 && p.off!==d.mhue[m]) return p.off; return d.mhue[m]; };
  const key=JSON.stringify([list,list.map(val)]); if(key===MHKEY) return;
  const own=list.filter(m=>val(m)).length;   // closed-fold summary: how many modes carry their own colour
  document.getElementById('mhn').textContent=list.length+' Modi'+(own?(' · '+own+' angepasst'):'');
  if(document.activeElement && document.activeElement.closest && document.activeElement.closest('#mh')) return;   // don't yank a field mid-edit
  MHKEY=key; let h='';
  list.forEach(m=>{ const off=val(m)||0, hx=hslHex(flToCss((NH[m]||0)+off));
    h+='<div class="mh"><span class="sm">'+(EFF[m]||m)+'</span>'+
       '<input type="color" value="'+hx+'" onchange="mhPick('+m+',this.value)">'+
       '<input class="hx" type="text" value="'+hx+'" maxlength="7" onchange="mhPick('+m+',this.value)">'+
       '<button onclick="mhNative('+m+')">Nativ</button>'+
       '<span class="hint">'+(off?('+'+off):'nativ')+'</span></div>'; });
  document.getElementById('mh').innerHTML=h;
}
// ── Text-strobe colours: here the picked colour IS the colour (hue + saturation) ──
let TCKEY='', TCP={};
function hexHS(hx){ const m=/^#?([0-9a-f]{6})$/i.exec(hx.trim()); if(!m) return null; const n=parseInt(m[1],16), r=(n>>16&255)/255,g=(n>>8&255)/255,b=(n&255)/255;
  const mx=Math.max(r,g,b), mn=Math.min(r,g,b); const s=mx?Math.round((mx-mn)/mx*255):0; return {h:cssToFl(hexHue(hx)), s:s}; }
function hsHex(h,s){ const d=flToCss(h), sv=s/255; const f=n=>{ const k=(n+d/60)%6; const v=1-sv*Math.max(0,Math.min(k,4-k,1)); return Math.round(v*255).toString(16).padStart(2,'0'); }; return '#'+f(5)+f(3)+f(1); }
function tcPick(id,hx){ const v=hexHS(hx); if(!v) return; fetch('/api/textcol?id='+id+'&h='+v.h+'&s='+v.s); TCP[id]={t:Date.now(),on:1,h:v.h,s:v.s}; TCKEY=''; }
function tcReset(id){ fetch('/api/textcol?id='+id+'&reset=1'); TCP[id]={t:Date.now(),on:0}; TCKEY=''; }
function tcRender(d){
  if(!d.tcol) return;
  const now=Date.now(); const rowsT=d.tcol.map((r,i)=>{ const id=i+1, p=TCP[id]; if(p && now-p.t<4000) return [r[0],p.on,p.on?p.h:r[2],p.on?p.s:r[3]]; return r; });
  const key=JSON.stringify(rowsT); if(key===TCKEY) return;
  const own=rowsT.filter(r=>r[1]).length;     // closed-fold summary
  document.getElementById('tcn').textContent=rowsT.length+' Texte'+(own?(' · '+own+' eigene Farbe'):' · alle wie Strobe-Farbe');
  if(document.activeElement && document.activeElement.closest && document.activeElement.closest('#tc')) return;
  TCKEY=key; let h='';
  rowsT.forEach((r,i)=>{ const id=i+1, hx=r[1]?hsHex(r[2],r[3]):'#ffffff';
    h+='<div class="mh"><span class="sm">'+r[0]+'</span>'+
       '<input type="color" value="'+hx+'" onchange="tcPick('+id+',this.value)">'+
       '<input class="hx" type="text" value="'+(r[1]?hx:'')+'" placeholder="Strobe" maxlength="7" onchange="tcPick('+id+',this.value)">'+
       '<button onclick="tcReset('+id+')">Strobe</button>'+
       '<span class="hint">'+(r[1]?'eigene Farbe':'wie Strobe-Farbe')+'</span></div>'; });
  document.getElementById('tc').innerHTML=h;
}
// Colour folds start closed; each phone remembers which ones it had open.
['fmh','ftc'].forEach(id=>{ const f=document.getElementById(id);
  try{ f.open=localStorage.getItem('fold.'+id)==='1'; }catch(e){}
  f.addEventListener('toggle',()=>{ try{ localStorage.setItem('fold.'+id,f.open?'1':'0'); }catch(e){} }); });
const rows={};
function makeCard(mac){
  const c=document.createElement('div'); c.className='m';
  c.innerHTML=
    '<div class="row"><input class="nm" type="text" maxlength="15"> <span class="mac"></span>'+
      ' <span class="pill st"></span> <span class="pill lk"></span> <span class="pill fw"></span>'+
      '<button class="idf" title="Streifen blinken lassen">&#128161; Finden</button></div>'+
    '<div class="meta"></div>'+
    '<div class="row"><label>Effekt</label><select class="ef">'+OPTS+'</select></div>'+
    '<div class="row"><label>Farbe</label><span class="sws"></span>'+
      '<span class="hint">nur eigenst&auml;ndig (nicht bei &bdquo;folgt Grid&ldquo;)</span></div>'+
    '<div class="row slave"><label>Slave</label>'+
      '<label><input type="checkbox" class="fm"> Grid-Modus</label>'+
      '<label><input type="checkbox" class="fs"> Strobe</label>'+
      '<label><input type="checkbox" class="fb"> Helligkeit</label>'+
      '<label><input type="checkbox" class="fe"> Beat</label>'+
      '<label><input type="checkbox" class="fk"> Blackout</label></div>'+
    '<div class="row"><label>Gain</label><input type="range" min="10" max="200" class="gn"><span class="gv"></span></div>'+
    '<div class="row"><label>LEDs</label><input type="number" min="1" max="512" class="ld">'+
      '<span class="hint">Enter = speichern &amp; Neustart</span></div>'+
    '<div class="row"><label>Gap</label><input type="number" min="1" max="255" class="gp">'+
      '<span class="hint">1 = jede LED · N = jede N-te leuchtet, N-1 dunkel</span></div>'+
    '<div class="row"><label>Boden</label><select class="fl">'+
      '<option value="0">Aus</option><option value="1">Fluss &#9654;</option><option value="2">Fluss &#9664;</option>'+
      '</select><span class="hint">Bodenstreifen: Taste 5 l&auml;sst ihn flie&szlig;en</span></div>'+
    '<div class="row"><label>Akku</label><span class="bt"></span>'+
      '<label style="min-width:auto"><input type="checkbox" class="bm" checked disabled> Unterspannungs-Schutz (immer an)</label></div>';
  const q=s=>c.querySelector(s);
  const r={card:c,nm:q('.nm'),mac:q('.mac'),st:q('.st'),lk:q('.lk'),fw:q('.fw'),meta:q('.meta'),ef:q('.ef'),fm:q('.fm'),idf:q('.idf'),
           fs:q('.fs'),fb:q('.fb'),fe:q('.fe'),fk:q('.fk'),gn:q('.gn'),gv:q('.gv'),ld:q('.ld'),gp:q('.gp'),fl:q('.fl'),bt:q('.bt'),bm:q('.bm'),
           sws:q('.sws'),follow:0,effNow:255,_cp:null,_ct:0};
  // Colour swatches: independent-mode tint. Built once; a click sends ?color= and
  // holds its highlight (r._cp) until the module's hello echoes it back, or ~4s,
  // so the 1.5s poll can't snap the selection back mid-change (same idea as put()).
  r.sws.innerHTML=PAL.map(p=>'<span class="sw" data-h="'+p.h+'" title="'+p.n+'" style="background:'+p.c+'"></span>').join('');
  r.sws.querySelectorAll('.sw').forEach(sw=>sw.onclick=()=>{ r._cp=+sw.dataset.h; r._ct=Date.now(); selSw(r,r._cp); set(mac,'color='+sw.dataset.h); });
  // Identify: momentary blink so you can find this strip. Not an effect change — sent raw, no pending.
  r.idf.onclick=()=>set(mac,'effect='+IDENT);
  const sendFollow=()=>{ let mk=(r.follow&F_MODE);
    if(r.fs.checked)mk|=F_STROBE; if(r.fb.checked)mk|=F_BRIGHT; if(r.fe.checked)mk|=F_BEAT; if(r.fk.checked)mk|=F_BLACK;
    set(mac,'follow='+mk); };
  r.nm.onchange=e=>{pend(e.target);set(mac,'name='+encodeURIComponent(e.target.value))};
  r.ef.onchange=e=>{pend(e.target);set(mac,'effect='+e.target.value)};
  // Grid-Modus = a view of effectPin: checked → follow grid (255); unchecked → pin the mode showing now.
  r.fm.onchange=e=>{pend(e.target);set(mac,'effect='+(e.target.checked?FOLLOW:r.effNow))};
  [r.fs,r.fb,r.fe,r.fk].forEach(cb=>cb.onchange=e=>{pend(e.target);sendFollow()});
  r.gn.oninput=e=>{r.gv.textContent=e.target.value+'%';pend(e.target)};
  r.gn.onchange=e=>set(mac,'gain='+e.target.value);
  r.ld.onchange=e=>{pend(e.target);set(mac,'leds='+e.target.value)};
  r.gp.onchange=e=>{pend(e.target);set(mac,'gap='+e.target.value)};
  r.fl.onchange=e=>{pend(e.target);set(mac,'floor='+e.target.value)};
  // Unterspannungs-Schutz ist fest an — kein onchange (Checkbox nur zur Anzeige, disabled).
  return r;
}
async function tick(){
  let d; try{d=await(await fetch('/api/modules')).json()}catch(e){return}
  if(!OPTS){ EFF=d.effects;LOCALS=d.locals;AUTOS=d.autos;LBASE=d.localBase;ABASE=d.autoBase;FOLLOW=d.follow;OPTS=optsHTML();
    document.getElementById('sladd').innerHTML=EFF.map((n,i)=>'<option value="'+i+'"'+(i==108?' selected':'')+'>'+i+' '+n+'</option>').join(''); }
  slRender(d); mhRender(d); tcRender(d);
  document.getElementById('warn').innerHTML = d.chOk?'':
    '<div class="warn"><b>Kanal '+d.ch+' statt '+d.chWant+'.</b> Dieser Knoten h&auml;ngt am Router und'+
    ' funkt deshalb auf dessen Kanal &mdash; die Module senden auf '+d.chWant+' und sind hier'+
    ' unsichtbar. Router auf Kanal '+d.chWant+' stellen.</div>';
  // Firmware spread among the modules we hear: >1 distinct build = some strip
  // missed the last OTA. Most common build is "current"; odd ones get a red pill.
  const on=d.mods.filter(m=>m.online), cnt={};
  on.forEach(m=>cnt[m.fw]=(cnt[m.fw]||0)+1);
  const builds=Object.keys(cnt); let common=null,best=-1;
  for(const b in cnt) if(cnt[b]>best){best=cnt[b];common=b}
  const mixed=builds.length>1;
  document.getElementById('bar').textContent =
    (d.profile?'Drahthaus':'Togalicht')+' · Kanal '+d.ch+' · Grid '+(d.gridSeen?('sichtbar, Modus '+d.gridIdx+' '+(EFF[d.gridIdx]||'')):'nicht gesehen')+' · '+d.mods.length+' Module'+
    (mixed?' · ⚠ '+builds.length+' verschiedene Versionen!':(builds.length==1?' · alle Version '+fwtag(builds[0]):''));
  put(autoIntEl,d.autoInt); put(sweepIntEl,d.sweepInt);   // grid's echoed interval values (put() protects an in-flight edit)
  const L=document.getElementById('list'); const seen={};
  if(!d.mods.length){ L.innerHTML='<div class="empty">Noch kein Modul geh&ouml;rt.'+
    (d.chOk?' Modul eingeschaltet? Es meldet sich einmal pro Sekunde.':'')+'</div>';
    for(const k in rows)delete rows[k]; return; }
  if(L.querySelector('.empty'))L.innerHTML='';
  d.mods.forEach(m=>{
    const id=macid(m.mac); seen[id]=1;
    let r=rows[id];
    if(!r){ r=rows[id]=makeCard(m.mac); L.appendChild(r.card); }
    r.card.className='m '+(m.online?'on':'off');
    r.mac.textContent=m.mac;
    r.st.textContent=m.online?'online':('weg seit '+m.age+'s');
    r.lk.textContent=m.link?'Controller ok':'kein Controller';
    r.fw.textContent='Version '+fwtag(m.fw);
    r.fw.className='pill fw'+(m.online&&mixed&&(''+m.fw)!==common?' bad':'');   // red = not the common build
    r.meta.innerHTML=m.w+'&times;'+m.h+' · Pin '+m.pin+' · Gruppe '+m.grp+' · l&auml;uft: '+runName(m.effNow)+' · up '+m.up+'s';
    put(r.nm,m.name);
    put(r.ef,m.eff);
    // Colour swatch highlight, with the same pending-lock idea as put(): honour a
    // recent click until the module confirms it (or ~4s), else show reported lhue.
    if(r._cp!=null){ if(r._cp===m.lhue){r._cp=null} else if(Date.now()-r._ct>4000){r._cp=null} }
    if(r._cp==null) selSw(r,m.lhue);
    r.effNow=m.effNow;                          // remembered so unchecking Grid-Modus can pin it
    put(r.fm,(m.eff==FOLLOW)?1:0);
    r.follow=m.follow;
    put(r.fs,(m.follow&F_STROBE)?1:0); put(r.fb,(m.follow&F_BRIGHT)?1:0);
    put(r.fe,(m.follow&F_BEAT)?1:0);   put(r.fk,(m.follow&F_BLACK)?1:0);
    put(r.gn,m.gain); r.gv.textContent=r.gn.value+'%';
    put(r.ld,m.w*m.h);
    put(r.gp,m.gap);
    put(r.fl,m.floor);
    // Spannung: always visible per module. battVolt is sampled every 10s on the module.
    r.bt.innerHTML='<span class="pill">&#128267; '+(m.batt>0?m.batt.toFixed(2)+'&nbsp;V':'&ndash;')+(m.blow?' &#9888;':'')+'</span>';
    // Schutz ist immer an → Checkbox bleibt gesetzt (disabled); Modul meldet stets enabled.
  });
  for(const id in rows) if(!seen[id]){ rows[id].card.remove(); delete rows[id]; }
}
tick();setInterval(tick,1500);          // poll always: put() protects in-flight edits
</script></body></html>)HTML";
void handleRoot(){ server.send_P(200,"text/html",PAGE); }

void enterDefault(){ state=ST_DEFAULT; activeBtn=-1; activeTarget=-1; }
void enterAdjust(int b){ state=ST_ADJUST; activeBtn=b; activeTarget=paramTarget(b); briMode=BRI_NONE; } // param adjust and brightness edit are exclusive

// Apply one +/- step to whichever brightness the button-4 edit-mode is on.
void briAdjust(int dir){
  switch(briMode){
    case BRI_GLOBAL: sendCmd(TOGA_CMD_PARAM,TOGA_P_BRI,dir);    break;   // grid global master
    case BRI_MODE:   sendCmd(TOGA_CMD_PARAM,TOGA_P_BRICOEF,dir); break;  // current mode's factor
    case BRI_CTRL:   ctrlBri=(uint8_t)constrain((int)ctrlBri+dir*CTRL_BRI_STEP,CTRL_BRI_MIN,CTRL_BRI_MAX);
                     ctrlBriDirty=true; ctrlBriDirtyAt=millis(); break;  // local pad dimming
  }
}
void saveCtrlBri(){ ctrlPrefs.begin("ctrl",false); ctrlPrefs.putUChar("cbri",ctrlBri); ctrlPrefs.end(); }

// ── Profile + setlist persistence (v11) ──
void saveProfile(){ ctrlPrefs.begin("ctrl",false); ctrlPrefs.putUChar("prof",gProfile); ctrlPrefs.end(); }
void saveSetlist(){ ctrlPrefs.begin("ctrl",false); ctrlPrefs.putBytes("setl",setlist,setN); ctrlPrefs.putUChar("setn",setN); ctrlPrefs.putUChar("spos",setPos); ctrlPrefs.end(); }
void loadProfileSetlist(){
  ctrlPrefs.begin("ctrl",true);
  gProfile=ctrlPrefs.getUChar("prof",TOGA_PROFILE_TOGA);
  setN=ctrlPrefs.getUChar("setn",0);
  if(setN>SETLIST_MAX) setN=0;
  if(setN && ctrlPrefs.getBytes("setl",setlist,setN)!=setN) setN=0;
  setPos=ctrlPrefs.getUChar("spos",0);
  ctrlPrefs.end();
  if(gProfile!=TOGA_PROFILE_DRAHTHAUS) gProfile=TOGA_PROFILE_TOGA;
  bool ok=setN>0; for(uint8_t i=0;i<setN;i++) if(setlist[i]>=TOGA_EFFECT_COUNT) ok=false;
  if(!ok){ setN=sizeof(SETLIST_DEFAULT); memcpy(setlist,SETLIST_DEFAULT,setN); setPos=0; }
  if(setPos>=setN) setPos=0;
  // One-time migration (2026-10): a setlist saved before mode 127 existed gets
  // "Schwarz" put in front of the submarine (107), or after the calibration view.
  // Flagged in NVS, so deleting it on the phone page later sticks.
  ctrlPrefs.begin("ctrl",false);
  if(!ctrlPrefs.getUChar("s127",0)){
    bool has=false; for(uint8_t i=0;i<setN;i++) if(setlist[i]==TOGA_MODE_SCHWARZ) has=true;
    if(!has && setN<SETLIST_MAX){
      uint8_t at=(setN>1)?1:setN; for(uint8_t i=0;i<setN;i++) if(setlist[i]==107){ at=i; break; }
      memmove(setlist+at+1,setlist+at,setN-at); setlist[at]=TOGA_MODE_SCHWARZ; setN++;
      if(setPos>=at && setPos+1<setN) setPos++;               // keep standing on the same song
      ctrlPrefs.putBytes("setl",setlist,setN); ctrlPrefs.putUChar("setn",setN); ctrlPrefs.putUChar("spos",setPos);
    }
    ctrlPrefs.putUChar("s127",1);
  }
  ctrlPrefs.end();
}
// Jump the wall to setlist position `pos` (absolute mode), in either profile.
// ── Mode change, closed loop ──
// A mode change is sent as an ABSOLUTE target (TOGA_CMD_MODESET) and then
// confirmed against the grid's own sync (200 ms). If the grid does not report
// the target within MODE_RETRY_MS of a send, it is sent again (new seq), up to
// MODE_TRIES times. Before this, a lost packet looked like "the press was
// ignored", and because the controller had already counted it, the NEXT press
// jumped two modes. Resending is harmless: the grid ignores a MODESET to the
// mode it already shows. Further presses before the confirm step from the
// pending target, not from the lagging sync, so fast stepping never loses one.
#define MODE_RETRY_MS 350
#define MODE_TRIES    6
int16_t  modeTarget=-1; uint32_t modeSentAt=0; uint8_t modeTries=0;
void sendModeTarget(int16_t idx){
  if(idx<0||idx>=TOGA_EFFECT_COUNT) return;
  modeTarget=idx; modeTries=0; modeSentAt=millis();
  sendCmd(TOGA_CMD_MODESET,0,idx);
  Serial.printf("mode -> %d\n", idx);
}
void serviceModeTarget(uint32_t t){
  if(modeTarget<0) return;
  if(autoMode!=TOGA_AUTO_OFF){ modeTarget=-1; return; }               // the automatic owns the mode now
  if(gsSyncSeen && (int32_t)(gsSyncAt-modeSentAt)>0 && gs.animIndex==modeTarget){ modeTarget=-1; return; }   // confirmed
  if((int32_t)(t-modeSentAt)<MODE_RETRY_MS) return;
  if(modeTries>=MODE_TRIES){ Serial.printf("mode %d: no confirm, giving up\n", modeTarget); modeTarget=-1; return; }
  modeTries++; modeSentAt=t;
  sendCmd(TOGA_CMD_MODESET,0,modeTarget);
  Serial.printf("mode -> %d (retry %u)\n", modeTarget, modeTries);
}
void setlistGoto(uint8_t pos){
  if(setN==0) return;
  setPos=pos%setN; setDirty=true; setDirtyAt=millis();
  autoMode=TOGA_AUTO_OFF;                                     // a chosen song ends the automatic, like a manual step
  sendModeTarget(setlist[setPos]);
}
// Mode +/- : Drahthaus walks the setlist, Togalicht steps the full list as before.
void stepMode(int dir){
  if(gProfile==TOGA_PROFILE_DRAHTHAUS && setN>0) setlistGoto((uint8_t)((setPos+dir+setN)%setN));
  else {
    // Togalicht: step from the pending target if one is in flight, else from what
    // the grid last reported. Without a sync yet, fall back to the relative step.
    int base = modeTarget>=0 ? modeTarget : (gsSyncSeen ? gs.animIndex : -1);
    if(base<0) sendCmd(TOGA_CMD_MODE,0,dir);
    else sendModeTarget((int16_t)((base+dir+TOGA_EFFECT_COUNT)%TOGA_EFFECT_COUNT));
  }
}
void setProfile(uint8_t p){
  profSel=false; textLayer=false; textHeld=TOGA_TEXT_NONE;
  if(p!=gProfile){
    if(p==TOGA_PROFILE_DRAHTHAUS && gsSyncSeen) togaModeBefore=gs.animIndex;   // remember where Togalicht was
    gProfile=p; saveProfile();
    autoMode=TOGA_AUTO_OFF;
    if(gProfile==TOGA_PROFILE_DRAHTHAUS){ if(setN) setlistGoto(setPos); else sendCmd(0,0,0); }
    else { if(togaModeBefore>=0 && togaModeBefore<TOGA_EFFECT_COUNT) sendModeTarget(togaModeBefore); else sendCmd(0,0,0); }
  } else sendCmd(0,0,0);
}

// ── Beat match (button 11) ──
// Tap it in time with the music; roughly 4 taps lock the tempo. There is no
// arming step and no timeout: the pulse then runs forever on that tempo until
// new taps set a new one.
//
// This node does NO tempo maths at all. Every tap is forwarded raw, the instant
// the key callback sees it, and the GRID averages, locks and phases it. That is
// deliberate: the tap is the phase anchor for the brightness peak, and an anchor
// only means something on the node that draws the frames. Averaging here and
// shipping a number over (what this used to do) threw the phase away — which is
// exactly why the flash never sat on the beat.
//
// The tempo does NOT touch the animation speed; it only pulses brightness.

// ── Trellis key callback ──
TrellisCallback handleKey(keyEvent evt){
  uint8_t b=PHYS2LOG[evt.bit.NUM]; bool pressed=(evt.bit.EDGE==SEESAW_KEYPAD_EDGE_RISING); // pad 90° CCW: physical→logical
  btnDown[b]=pressed; if(pressed) btnDownT[b]=millis();
  // +/- taps are LATCHED: a quick tap can press AND release inside one 20 ms poll,
  // and the level-edge test in loop() would then never see it.
  if(pressed && b==3) plusHit=true;
  if(pressed && b==7) minusHit=true;
  // ── Profile selection (pad blinking red): only the two choice keys do anything ──
  if(profSel){
    if(pressed){ if(b==0) setProfile(TOGA_PROFILE_TOGA); else if(b==4) setProfile(TOGA_PROFILE_DRAHTHAUS); }
    return 0;
  }
  // ── Drahthaus: key 13 toggles the text-strobe layer; inside it only text keys count ──
  if(gProfile==TOGA_PROFILE_DRAHTHAUS && b==13){
    if(pressed){ textLayer=!textLayer; textHeld=TOGA_TEXT_NONE; if(state==ST_ADJUST && isTextKey(activeBtn)) enterDefault(); sendCmd(0,0,0); }
    return 0;
  }
  if(isTextKey(b)){                                    // a text key in the open layer
    for(uint8_t i=0;i<TOGA_TEXT_COUNT;i++) if(TEXT_KEYS[i]==b){
      if(pressed) textHeld=(uint8_t)(i+1); else if(textHeld==i+1) textHeld=TOGA_TEXT_NONE;
      break;
    }
    sendCmd(0,0,0);                                    // edge → notify now, don't wait for the heartbeat
    return 0;
  }                                                    // every other key works as usual, layer or not
  if(pressed){
    if(isParamBtn(b) && state==ST_ADJUST && activeBtn==b){ enterDefault(); suppressBtn=b; } // tap active param → off
    // Strobe-speed key (phys 10): a plain TAP toggles its adjust mode (no 2s hold),
    // so "button 10 activates strobe speed" is one press. The line above already
    // handled the toggle-OFF (and set suppressBtn), so only enter here on a fresh tap.
    if(b==9 && !(state==ST_ADJUST && activeBtn==9) && suppressBtn!=9) enterAdjust(9);
    if(b==11) sendCmd(TOGA_CMD_BEAT_TAP,0,0);      // send NOW: every ms of delay is phase error
    if(b==0){ autoMode=(uint8_t)(autoMode%TOGA_AUTO_COUNT+1); sendCmd(TOGA_CMD_HEARTBEAT,0,0); } // next mood, wraps 4→1
    if(b==10){ gBlackout=!gBlackout; sendCmd(0,0,0); }  // blackout latch: flip + notify now (don't wait for the 100ms heartbeat)
    if(b==2){ gSweep=!gSweep; sendCmd(0,0,0); }          // speed-sweep latch (phys 4): flip + notify now
  } else if(b==suppressBtn) suppressBtn=-1;       // released → hold-to-activate allowed again
  if(b==15||b==14||b==13||b==6) sendCmd(0,0,0);  // strobe / mode-strobe / castle-strobe / spot edge → notify immediately
  return 0;
}

// true if the active param is at its limit in direction dir
bool atLimit(int dir){
  switch(activeTarget){
    case TOGA_P_SPEED:      return dir>0? gs.animSpeed>=4.0f : gs.animSpeed<=0.1f;
    case TOGA_P_HUESPEED:   return dir>0? gs.hueSpeed>=HUE_SPEED_MAX : gs.hueSpeed<=0;
    case TOGA_P_STROBESPEED:return dir>0? gs.strobeSpeed>=STROBE_SPEED_MAX : gs.strobeSpeed<=STROBE_SPEED_MIN;
    default: return false;                       // COLOR / STROBEHUE wrap
  }
}
// Is whatever the +/- keys are adjusting RIGHT NOW pinned at its limit in
// direction dir? Covers the three brightness edit-machines and the param adjust;
// plain mode-switching and the colour params wrap, so they never report a limit.
// Used only to blink the +/- key when a held press no longer changes anything.
bool adjustAtLimit(int dir){
  if(btnDown[15]||btnDown[14]||(btnDown[13]&&gProfile==TOGA_PROFILE_TOGA)||textHeld!=TOGA_TEXT_NONE)   // strobe/text held → tuning global strobe speed
    return dir>0? gs.strobeSpeed>=STROBE_SPEED_MAX : gs.strobeSpeed<=STROBE_SPEED_MIN;
  switch(briMode){
    case BRI_GLOBAL: return dir>0? gs.globalBrightness>=MAX_BRIGHTNESS : gs.globalBrightness<=BRI_STEP;  // floor = one step above off
    case BRI_MODE:   return dir>0? gs.modeFactor>=BRICOEF_MAX          : gs.modeFactor<=BRICOEF_MIN;
    case BRI_CTRL:   return dir>0? ctrlBri>=CTRL_BRI_MAX               : ctrlBri<=CTRL_BRI_MIN;
    default: break;
  }
  if(state==ST_ADJUST) return atLimit(dir);
  return false;
}

// HSV → trellis colour (h,s,v all 0..255).
uint32_t hsvC(uint8_t h,uint8_t s,uint8_t v){
  uint8_t region=h/43, rem=(uint8_t)((h-region*43)*6);
  uint8_t p=(uint8_t)((v*(255-s))/255), q=(uint8_t)((v*(255-((s*rem)/255)))/255), tt=(uint8_t)((v*(255-((s*(255-rem))/255)))/255);
  uint8_t r,g,b;
  switch(region){case 0:r=v;g=tt;b=p;break;case 1:r=q;g=v;b=p;break;case 2:r=p;g=v;b=tt;break;
                 case 3:r=p;g=q;b=v;break;case 4:r=tt;g=p;b=v;break;default:r=v;g=p;b=q;break;}
  return C(r,g,b);
}
// Cold→hot meter colour for a 0..1 fraction (blue=low, red=high).
uint32_t heatCol(float frac,uint8_t v){ if(frac<0)frac=0; if(frac>1)frac=1; return hsvC((uint8_t)(170.0f*(1.0f-frac)),255,v); }
// Scale an RGB triple to brightness v.
uint32_t rgbScale(uint8_t r,uint8_t g,uint8_t b,uint8_t v){ return C((uint8_t)((uint16_t)r*v/255),(uint8_t)((uint16_t)g*v/255),(uint8_t)((uint16_t)b*v/255)); }
// Actual strobe colour as reported by the grid.
uint32_t strobeCol(uint8_t v){ return rgbScale(gs.sColR,gs.sColG,gs.sColB,v); }
// Param button colour: value params show a cold→hot meter; colour params show
// the actual colour they are set to.
uint32_t paramCol(int b,uint8_t v){
  float f;
  switch(b){
    case 4:  f=(gs.brightness-1)/127.0f; break;      // Helligkeit 1..128
    case 8:  f=(gs.animSpeed-0.1f)/3.9f; break;       // Speed 0.1..4.0
    case 5:  f=gs.hueSpeed/(float)HUE_SPEED_MAX; break;   // Hue-speed 0..HUE_SPEED_MAX
    case 9:  f=gs.strobeSpeed/(float)STROBE_SPEED_MAX; break;  // Strobe-speed 0..MAX (cold→hot meter)
    case 1:  return rgbScale(gs.tintR,gs.tintG,gs.tintB,v);  // Color → exact grid tint
    case 12: return strobeCol(v);                            // Strobe-colour → actual strobe colour
    default: return C(v,v,v);
  }
  return heatCol(f,v);
}
// Auto-mood colour, for the moods in grid.ino: calm · space · party · show.
// The hues live in TOGA_AUTO_HUE (<toga_proto.h>) because the grid flashes the
// same mood as "A1".."A4" in the same colour — a second table here would drift,
// and the button and the wall would then disagree about which mood is running.
// The grid's tint (knob + auto-rotation) is added on top, exactly as the grid
// rotates the modes themselves — so the button always wears the colour the wall
// is actually wearing, and the four moods stay apart from each other while it
// turns. Same reason the strobe and colour buttons show real values, not labels.
uint32_t autoCol(uint8_t m,uint8_t v){
  if(m<1||m>TOGA_AUTO_COUNT) return C(0,0,0);
  return hsvC((uint8_t)(TOGA_AUTO_HUE[m-1]+gs.hueBase+gs.hueAuto),255,v);
}
// Button 11's colour: dark red at rest, pulsing on the grid's beat once a tempo
// is locked. Free-runs from the last sync's phase, which is close enough for a
// key LED — the grid re-snaps us every 200ms.
uint32_t beatCol(uint32_t t){
  if(!gs.beatMs) return C(40,0,0);                // no tempo → just "the beat button"
  float ph=gs.beatPhase/255.0f + (float)(t-gs.beatAt)/(float)gs.beatMs;
  ph-=floorf(ph);
  uint8_t v=(uint8_t)(togaBeatFactor(ph)*200.0f);
  return C(v,0,0);
}

// Button 4 while editing brightness: slow breathe=GLOBAL, fast breathe=MODE
// factor, solid white=CONTROLLER pad. A triangle breathe reads as "adjusting".
uint32_t briBtnCol(uint32_t t){
  if(briMode==BRI_CTRL) return C(255,255,255);
  uint32_t period=(briMode==BRI_MODE)?350:1400;   // fast vs slow
  float f=(float)(t%period)/period;               // 0..1
  float tri=f<0.5f?f*2.0f:(1.0f-f)*2.0f;          // 0..1..0
  uint8_t v=(uint8_t)(25+tri*230);
  return C(v,v,v);
}

// Scale a packed colour by s/255 (used to dim the pad per global brightness).
uint32_t scaleCol(uint32_t c,uint8_t s){
  uint8_t r=(uint8_t)(((c>>16&0xFF)*s)>>8), g=(uint8_t)(((c>>8&0xFF)*s)>>8), b=(uint8_t)((c&0xFF)*s>>8);
  return C(r,g,b);
}

void renderLEDs(uint32_t t){
  bool blink=((t/125)%2)==0;                      // ~4 Hz
  bool slowBlink=((t/250)%2)==0;                  // ~2 Hz — auto mood
  trellis.pixels.setBrightness(255);
  // ── Profile selection: the whole pad blinks red, the bottom row offers the two
  //    profiles (phys 12 = Togalicht in teal, phys 13 = Drahthaus in orange); the
  //    active one is bright, the other dim. Not dimmed by the pad brightness. ──
  if(profSel){
    for(int i=0;i<16;i++){
      uint32_t col = slowBlink ? C(90,0,0) : C(0,0,0);
      if(i==0) col = (gProfile==TOGA_PROFILE_TOGA)      ? C(0,170,210) : C(0,40,50);
      if(i==4) col = (gProfile==TOGA_PROFILE_DRAHTHAUS) ? C(230,70,0)  : C(55,16,0);
      trellis.pixels.setPixelColor(LOG2PHYS[i],col);
    }
    trellis.pixels.show(); return;
  }
  static const uint32_t TC[TOGA_TEXT_COUNT]={ C(230,20,120), C(230,60,0), C(120,120,120), C(0,150,200), C(0,170,90) };
  // Dim the pad by the grid's global brightness AND this pad's own factor — but do
  // the scaling PER PIXEL (scaleCol) instead of the hardware setBrightness, so the
  // Helligkeit key (button 4) can be exempted below and stay clearly visible while
  // you operate it. Fall back to BRI_DEFAULT when we have never seen a sync (an old
  // grid, or before the first packet) so the pad is never blacked out.
  trellis.pixels.setBrightness(255);              // no hardware scaling; we scale manually
  uint8_t gb=gs.globalBrightness?gs.globalBrightness:BRI_DEFAULT;
  uint16_t g255=(uint16_t)gb*2; if(g255>255)g255=255;   // 1..MAX_BRIGHTNESS → 0..255
  uint8_t padScale=(uint8_t)constrain((int)((uint32_t)g255*ctrlBri/255),30,255);   // floor keeps meters readable
  // The "exempt" keys (Helligkeit 4, Speed-Sweep 2) used to ignore padScale entirely
  // and blazed in a dark room when active. Track the pad dimming, but with a higher
  // floor than the params so they stay findable — visible, never blinding.
  uint8_t visScale=padScale<60?60:padScale;
  for(int i=0;i<16;i++){
    uint32_t col=C(0,0,0);
    if(state==ST_ADJUST){
      if(i==activeBtn)      col=blink?paramCol(i,170):paramCol(i,22);  // flash in its own colour
      else if(i==3||i==7)   col=C(0,60,80);
      else if(i==15 && activeTarget==3) col=strobeCol(90);   // live strobe-colour preview
    } else { // DEFAULT (+/- select the mode when no param is active)
      if(i==3||i==7)        col=C(0,60,80);        // +/-
      else if(isParamBtn(i)) col=paramCol(i,80);   // each param in its own colour (bright enough to stay visible when the pad is dimmed)
      else if(i==11)        col=beatCol(t);        // Beat tap — pulses on the locked tempo
      else if(i==13)        col=(gProfile==TOGA_PROFILE_DRAHTHAUS)?C(90,50,0):C(0,45,55);   // Drahthaus: text layer (amber) · Toga: castle-strobe
      else if(i==14)        col=C(45,20,45);       // Mode-strobe
      else if(i==15)        col=strobeCol(70);     // Strobe shows its set colour
    }
    // Auto mood (0): blinks in its mood's colour, dark when off. Deliberately
    // outside the state branches — adjusting a param or tapping a beat does not
    // stop the automatic, so the light must not lie about it either.
    if(i==0 && autoMode!=TOGA_AUTO_OFF) col=slowBlink?autoCol(autoMode,200):C(0,0,0);
    // press feedback
    if(btnDown[i]){
      if(i==15)                     col=strobeCol(255);
      else if(i==14)                col=rgbScale(gs.tintR,gs.tintG,gs.tintB,255);  // mode-strobe flash colour
      else if(i==13)                col=(gProfile==TOGA_PROFILE_DRAHTHAUS)?C(200,110,0):C(0,140,160);   // castle-strobe held / text layer opening
      else if(i!=4 && !(state==ST_ADJUST && i==activeBtn)) col=C(70,20,0);
    }
    // +/- at the end of the setting being adjusted: while the key is held and the
    // value can no longer move, blink it so "you've hit the limit" is unmistakable.
    // Overrides the press-feedback above; a held +/- that is NOT at a limit keeps it.
    if(i==3 && btnDown[3] && adjustAtLimit(+1)) col = blink?C(0,110,140):C(0,0,0);
    if(i==7 && btnDown[7] && adjustAtLimit(-1)) col = blink?C(0,110,140):C(0,0,0);
    // Everything except the Helligkeit key dims with the global brightness.
    if(i!=4) col=scaleCol(col,padScale);
    // Button 4 (Helligkeit) owns its own indicator, above the param/press logic, and
    // is NOT dimmed — it must stay clearly visible while you operate it:
    // slow breathe = GLOBAL, fast breathe = MODE factor, solid white = CONTROLLER,
    // a steady medium white at rest.
    if(i==4){
      if(briMode!=BRI_NONE) col=scaleCol(briBtnCol(t),visScale);  // while editing: dimmed with the pad (was full 255 → blinding in the dark)
      else col=scaleCol(C(70,70,70),padScale);       // at rest: same brightness as the other keys (dimmed with the pad)
    }
    // Button 10 (Blackout): bright while active, a dim marker at rest so the key is
    // findable. Not dimmed by the global brightness — it must stay visible in the dark.
    if(i==10) col = gBlackout ? C(200,200,200) : C(25,25,25);
    // Speed-sweep (logical 2 / phys 4): blinks while active, dimmed-down marker at
    // rest so it is findable. Not dimmed by the global brightness, like blackout.
    if(i==2)  col = gSweep ? (blink?scaleCol(C(110,70,0),visScale):C(0,0,0)) : C(18,12,0);   // active blink now dims with the pad (was fixed 70,44,0 — too bright in the dark)
    // While blacked out, kill every pad LED except the ones the operator still uses:
    // blackout(10), strobe(15), mode-strobe(14), castle-strobe(13) and auto(0).
    if(gBlackout && i!=0 && i!=10 && i!=13 && i!=14 && i!=15) col=C(0,0,0);
    // Text layer: the six text keys wear their own colours (held = white), the
    // layer key blinks amber; every other key keeps its normal light and function.
    if(textLayer){
      for(uint8_t k=0;k<TOGA_TEXT_COUNT;k++) if(TEXT_KEYS[k]==i) col=(textHeld==k+1)?C(255,255,255):scaleCol(TC[k],visScale);
      if(i==13) col = blink ? C(90,50,0) : C(40,22,0);
    }
    trellis.pixels.setPixelColor(LOG2PHYS[i],col);   // pad 90° CW: logical→physical
  }
  trellis.pixels.show();
}

void setup(){
  // This node hosts the WLAN the whole fleet lives on, so it has to be able to
  // say so. It ran mute for a long time, which was fine while it only pushed
  // buttons — it is not fine for the board everything else depends on.
  Serial.begin(115200);
  delay(200);
  Serial.println(F("\n─── TOGA CONTROL UNIT ───"));

  // This pad's own brightness factor, restored across power-off.
  ctrlPrefs.begin("ctrl",true);
  ctrlBri=ctrlPrefs.getUChar("cbri",CTRL_BRI_DEFAULT);
  ctrlPrefs.end();
  ctrlBri=constrain(ctrlBri,CTRL_BRI_MIN,CTRL_BRI_MAX);
  loadProfileSetlist();                            // profile + Drahthaus setlist (v11)
  Serial.printf("profile  %s, setlist %u entries, pos %u\n", gProfile?"Drahthaus":"Togalicht", setN, setPos);

  Wire.begin();
  if(!trellis.begin()){ Serial.println(F("NeoTrellis not found — check I2C")); while(1) delay(1000); }
  for(int i=0;i<16;i++){
    trellis.activateKey(i,SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i,SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i,handleKey);
  }
  // Diagnostics: log every station (dis)connect + IP lease on the AP, so a
  // reconnect that fails can be pinned to the auth phase (no STACONNECTED) vs the
  // DHCP phase (STACONNECTED but no IPASSIGNED). Registered BEFORE togaApBegin().
  WiFi.onEvent([](arduino_event_id_t e, arduino_event_info_t info){
    const uint8_t* m;
    switch(e){
      case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        m=info.wifi_ap_staconnected.mac;
        Serial.printf("AP + station %02X:%02X:%02X:%02X:%02X:%02X (aid %d)\n",m[0],m[1],m[2],m[3],m[4],m[5],info.wifi_ap_staconnected.aid); break;
      case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        m=info.wifi_ap_stadisconnected.mac;
        Serial.printf("AP - station %02X:%02X:%02X:%02X:%02X:%02X (aid %d)\n",m[0],m[1],m[2],m[3],m[4],m[5],info.wifi_ap_stadisconnected.aid); break;
      case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        Serial.printf("AP = IP assigned %s\n", IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str()); break;
      default: break;
    }
  });
  // We host the WLAN. Pinned to TOGA_CHANNEL, so associating to it costs nobody
  // their place on the bus — that is what makes every node flashable mid-show.
  togaApBegin();
  Serial.printf("AP       %s  ch=%d  ip=%s\n", TOGA_AP_SSID, TOGA_CHANNEL,
                WiFi.softAPIP().toString().c_str());
  Serial.printf("mac      %s\n", WiFi.macAddress().c_str());

  if(esp_now_init()!=ESP_OK){ Serial.println(F("ESP-NOW init failed!")); while(1) delay(1000); }
  esp_now_register_recv_cb(onReceive);
  // No send callback: broadcast is never acked, so the status is always
  // SUCCESS and there is nothing useful to do with it.
  if(!togaAddPeer(TOGA_BCAST)){ Serial.println(F("Broadcast peer failed!")); while(1) delay(1000); }

  // Module-manager web UI, served on our own AP. Reach it from a phone joined to
  // togacontroller at http://192.168.4.1/ (the SoftAP gateway). mDNS is best-effort:
  // we advertise HTTP as toga-ctl.local — handy on a laptop, flaky over SoftAP on
  // iOS, hence the IP is the sure path.
  MDNS.begin("toga-ctl");
  MDNS.addService("http", "tcp", 80);
  server.on("/", handleRoot);
  server.on("/api/modules", handleModules);
  server.on("/api/set", handleSet);
  server.on("/api/timing", handleTiming);
  server.on("/api/setlist", handleSetlist);         // Drahthaus setlist: read / replace (v11)
  server.on("/api/jump", handleJump);               // jump to a setlist position
  server.on("/api/profile", handleProfile);         // Togalicht / Drahthaus
  server.on("/api/modehue", handleModeHue);
  server.on("/api/textcol", handleTextCol);         // per-text strobe colour         // per-mode colour from the colour pickers
  server.onNotFound([](){ server.send(404,"text/plain","404"); });
  server.begin();
  Serial.println(F("http ready on :80  (http://192.168.4.1/)"));
}

// Who is on our AP? The fleet's nodes join it for the bus/web UI, so an empty
// list while a board is powered means that board is not on the network — the one
// fact this node is uniquely able to report.
void reportApClients(uint32_t t){
  static uint32_t last = 0;
  static int lastN = -1;
  if(t - last < 5000) return;
  last = t;
  int n = WiFi.softAPgetStationNum();
  if(n == lastN) return;                     // only on change: this is a console, not a log
  lastN = n;
  Serial.printf("AP clients: %d\n", n);
}

void loop(){
  static uint32_t lastPoll=0,lastHB=0;
  static bool prevPlus=false, prevMinus=false;
  uint32_t t=millis();

  // Above the 20ms poll gate below: that returns on most iterations, and the web
  // UI must not be starved by it.
  server.handleClient();                            // the module-manager page
  drainHellos(t);                                   // merge parked hellos into the table

  serviceRepeats(t);                                // spread repeats of one-shot commands (above the poll gate: ms-accurate)
  if(t-lastHB>=100){ lastHB=t; sendCmd(0,0,0); }   // heartbeat (carries strobe state)

  reportApClients(t);

  if(t-lastPoll<20) return;
  lastPoll=t;
  trellis.read();                                  // fires handleKey
  // handleKey stamps btnDownT with millis() DURING the I2C read (1-2ms), i.e.
  // possibly later than `t`. With the old `t`, t-btnDownT wrapped to ~4e9 and a
  // short TAP on a param key jumped straight into ADJUST — from then on +/-
  // changed that param instead of the mode. Re-read the clock after the read.
  t=millis();

  // ── Profile gesture: Mode+ / Mode- / Beat (phys 0/1/2) held together 5s → select ──
  static uint32_t triDownAt=0; static bool triPrev=false, triFired=false;
  bool tri=btnDown[3]&&btnDown[7]&&btnDown[11];
  if(tri && !triPrev){ triDownAt=t; triFired=false; }
  triPrev=tri;
  if(tri && !triFired && !profSel && (t-triDownAt)>=PROFILE_HOLD_MS){   // fires once per hold: release to re-arm
    triFired=true;
    profSel=true; profSelT=t; textLayer=false; textHeld=TOGA_TEXT_NONE; enterDefault(); briMode=BRI_NONE; sendCmd(0,0,0);
  }
  if(profSel && (t-profSelT)>=PROFILE_SEL_TIMEOUT) profSel=false;   // nobody chose → back, unchanged
  // In the Drahthaus profile the wall must be on the setlist entry we think it is:
  // after boot (or a grid reboot) re-send the position once the grid is heard.
  static bool resynced=false;
  if(gProfile==TOGA_PROFILE_DRAHTHAUS && gsSyncSeen && !resynced && setN){
    resynced=true; if(gs.animIndex!=setlist[setPos]) sendModeTarget(setlist[setPos]);
  }
  bool layered=profSel;                            // the profile picker owns the keys; the text layer does NOT

  // ── Hold a param button 2s → activate ADJUST (it starts blinking) ──
  const int pbs[4]={8,12,1,5};                   // button 4 has its own machine below
  for(int k=0;k<4;k++){ int b=pbs[k];
    if(layered) break;                           // keys belong to the layer
    if(b==suppressBtn) continue;                 // don't re-arm the button we just tapped off
    if(isTextKey(b)) continue;                   // Speed / Strobe-colour are text keys while the layer is open
    if(btnDown[b] && (t-btnDownT[b])>=HOLD_TO_SELECT && !(state==ST_ADJUST && activeBtn==b)) enterAdjust(b);
  }

  // ── Button 4 (Helligkeit): tap → GLOBAL, hold 2s → MODE factor, hold 10s → CONTROLLER pad ──
  // The LED (rendered below) escalates live while held, so the operator sees the
  // mode they'll land in on release. A tap while already in a mode exits it.
  // Debounce the raw key: the seesaw can bounce/repeat, which would otherwise read
  // as a burst of taps and rattle briMode. Only accept a level that has held steady
  // for B4_DEBOUNCE_MS, and additionally ignore any tap within B4_COOLDOWN_MS of the
  // last briMode change (belt and suspenders against faster-than-debounce chatter).
  #define B4_DEBOUNCE_MS  50
  #define B4_COOLDOWN_MS 300
  static bool     b4Raw=false, b4=false;
  static uint32_t b4RawT=0;
  if(btnDown[4]!=b4Raw){ b4Raw=btnDown[4]; b4RawT=t; }     // raw level changed → restart the stability clock
  if(b4!=b4Raw && (t-b4RawT)>=B4_DEBOUNCE_MS) b4=b4Raw;    // adopt it only once it has settled

  static bool     prevB4=false;
  static bool     b4Escalated=false;             // this hold already crossed a threshold → release is not a tap
  static uint32_t b4DownAt=0;                     // OUR own press timestamp — immune to handleKey re-stamping btnDownT
  static uint32_t briChangeT=0;                   // last time briMode changed here (cooldown anchor)
  if(b4 && !prevB4){ b4Escalated=false; b4DownAt=t; }   // fresh press: reset escalation + start our hold timer
  if(layered) b4Escalated=true;                   // key 4 picks a profile / is inert in a layer: never a brightness tap
  if(b4 && !layered){
    uint32_t held=t-b4DownAt;
    if(held>=HOLD_TO_CTRL){ if(briMode!=BRI_CTRL){ briMode=BRI_CTRL; b4Escalated=true; briChangeT=t; enterDefault(); } }
    else if(held>=HOLD_TO_SELECT){ if(briMode!=BRI_MODE && briMode!=BRI_CTRL){ briMode=BRI_MODE; b4Escalated=true; briChangeT=t; enterDefault(); } }
  }
  if(!b4 && prevB4){                              // released
    if(!b4Escalated && (t-briChangeT)>=B4_COOLDOWN_MS){    // a genuine tap: toggle into GLOBAL, or out of any mode
      if(briMode==BRI_NONE){ briMode=BRI_GLOBAL; enterDefault(); }
      else briMode=BRI_NONE;
      briChangeT=t;
    }
  }
  prevB4=b4;

  // Beat match needs nothing here: taps are sent straight from handleKey and the
  // grid owns everything downstream of that.

  // ── +/- ──
  bool plus=btnDown[3], minus=btnDown[7];
  bool plusEdge =(plus && !prevPlus)  || plusHit;  plusHit=false;
  bool minusEdge=(minus && !prevMinus)|| minusHit; minusHit=false;
  if(layered){ /* +/- are layer keys (or inert) — nothing to step */ }
  else if(btnDown[15]||btnDown[14]||(btnDown[13]&&gProfile==TOGA_PROFILE_TOGA)||textHeld!=TOGA_TEXT_NONE){   // + a held TEXT key   // any strobe held → +/- tune the ONE global strobe speed (+ = faster)
    if(plusEdge){ plusPressT=t; plusRepT=t; sendCmd(2,TOGA_P_STROBESPEED,+1); }
    else if(plus){ if(t-plusPressT>=400 && t-plusRepT>=60){ plusRepT=t; sendCmd(2,TOGA_P_STROBESPEED,+1); } }
    if(minusEdge){ minusPressT=t; minusRepT=t; sendCmd(2,TOGA_P_STROBESPEED,-1); }
    else if(minus){ if(t-minusPressT>=400 && t-minusRepT>=60){ minusRepT=t; sendCmd(2,TOGA_P_STROBESPEED,-1); } }
  } else if(briMode!=BRI_NONE){                     // brightness edit-mode → +/- adjust the selected brightness
    if(plusEdge){ plusPressT=t; plusRepT=t; briAdjust(+1); }
    else if(plus){ uint32_t h=t-plusPressT; if(h>=500){ uint32_t rate=h<2000?80:(h<4000?40:16); if(t-plusRepT>=rate){ plusRepT=t; briAdjust(+1); } } }
    if(minusEdge){ minusPressT=t; minusRepT=t; briAdjust(-1); }
    else if(minus){ uint32_t h=t-minusPressT; if(h>=500){ uint32_t rate=h<2000?80:(h<4000?40:16); if(t-minusRepT>=rate){ minusRepT=t; briAdjust(-1); } } }
  } else if(state==ST_DEFAULT){
    // In auto-mode the FIRST +/- press ONLY ends the automatic and keeps the mode
    // that is showing — it does NOT step. autoMode rides every packet, so notifying
    // once (sendCmd) propagates the off; the grid then persists the current mode.
    // Once auto is off, +/- steps the mode as before.
    bool edge = (plusEdge)||(minusEdge);
    if(edge && autoMode!=TOGA_AUTO_OFF){
      autoMode=TOGA_AUTO_OFF; sendCmd(0,0,0);        // end auto, hold current mode (no step)
    } else {
      if(plusEdge)  stepMode(+1);           // next mode (Drahthaus: next setlist entry)
      if(minusEdge) stepMode(-1);           // prev mode
    }
  } else if(state==ST_ADJUST){
    if(plusEdge){ plusPressT=t; plusRepT=t; sendCmd(2,activeTarget,+1); }
    else if(plus){ uint32_t h=t-plusPressT; if(h>=500){ uint32_t rate=h<2000?80:(h<4000?40:16); if(t-plusRepT>=rate){ plusRepT=t; sendCmd(2,activeTarget,+1); } } }
    if(minusEdge){ minusPressT=t; minusRepT=t; sendCmd(2,activeTarget,-1); }
    else if(minus){ uint32_t h=t-minusPressT; if(h>=500){ uint32_t rate=h<2000?80:(h<4000?40:16); if(t-minusRepT>=rate){ minusRepT=t; sendCmd(2,activeTarget,-1); } } }
  }
  prevPlus=plus; prevMinus=minus;
  serviceModeTarget(t);                             // resend an unconfirmed mode change

  // ADJUST stays until the active param button is tapped again (no timeout).

  // Persist the controller pad factor lazily (flash wear): 1.5s after the last edit.
  // Signed: both stamps are taken with millis() later in this same pass than `t`.
  if(ctrlBriDirty && (int32_t)(t-ctrlBriDirtyAt)>=1500){ ctrlBriDirty=false; saveCtrlBri(); }
  if(setDirty && (int32_t)(t-setDirtyAt)>=1500){ setDirty=false; saveSetlist(); }   // setlist / position, debounced (flash wear)

  renderLEDs(t);
}
