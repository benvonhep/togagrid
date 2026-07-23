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
//          4 free    5 free    6 Blackout 7 Mode-Strobe
//          8 Color   9 Hue-spd 10 free   11 Castle-Strobe
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
//  Strobe (15): momentary while held. Hold 15 + press +/- → tune the gap
//    between flashes (the flash itself is fixed at its shortest).
//  Auto (0): tap to cycle the four moods 1→2→3→4→1 (button blinks in the
//    mood's colour). Any manual mode change with +/- turns it off again.
//
//  Params (logical):  4 Helligkeit (own machine) · 1 Color · 12 Strobe-colour · 8 Speed · 5 Hue-speed
// ═══════════════════════════════════════════════════════
#include <Arduino.h>
#include <toga_proto.h>          // the wire format, shared with grid/ and module/
#include <toga_ota.h>            // always-on firmware update; we host the AP
#include <Wire.h>
#include <Preferences.h>          // NVS: persist this controller's own pad-brightness factor
#include <WebServer.h>           // the phone module-manager UI (folded in from webserver/)
#include <ESPmDNS.h>             // advertise the page over mDNS on top of the OTA instance
#include "Adafruit_NeoTrellis.h"

// ── Module manager (folded in from webserver/) ──────────
// We host the AP, so the phone page that lists every module and configures it
// now lives here too — no separate board. Modules announce themselves once a
// second with a TogaHelloMsg; we keep a table of who is out there and send a
// TogaModCfgMsg addressed at one MAC when the operator changes something. This
// is a pure listener/config path — it never touches the show or the meters.
// Declared above the first function definition (verifyRollbackLater, below) so
// arduino-cli's hoisted prototype for upsert() — which returns ModEntry* —
// lands after the type, not before it.
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
  uint32_t lastSeen;             // millis() on THIS node
};
ModEntry mods[MAX_MODULES];

// The recv callback runs in the WiFi task; HTTP handlers read the table from
// loop(). Rather than lock, the callback only parks the hello in a ring and
// loop() merges it — the same split the modules themselves use for sync/cfg.
#define HQ_LEN 8
volatile TogaHelloMsg helloQ[HQ_LEN];
volatile uint8_t hqHead = 0, hqTail = 0;

// Must stay BELOW every include: this is a function DEFINITION, and arduino-cli
// injects its generated prototypes just above the first one it finds. Put it any
// higher and the prototype for handleKey() lands above Adafruit_NeoTrellis.h,
// which is where TrellisCallback comes from. (The module-manager type above is
// declarations only — no function — so the injection point is still right here.)
//
// Keep a freshly OTA'd image on probation until it has proven itself, instead
// of letting initArduino() bless it before setup() runs. extern "C" is
// mandatory — see the long note in toga_ota.h.
extern "C" bool verifyRollbackLater() { return true; }

Adafruit_NeoTrellis trellis;

#define HOLD_TO_SELECT     2000   // hold btn 4 this long → per-mode factor
#define HOLD_TO_CTRL      10000   // hold btn 4 this long → controller pad brightness

// ── Synced state from the grid (for at-limit display) ──
struct { uint8_t brightness; uint8_t globalBrightness; float animSpeed; uint8_t hueBase,hueAuto,hueSpeed,strobeHue;
         uint8_t tintR,tintG,tintB, sColR,sColG,sColB;
         uint16_t beatMs; uint8_t beatPhase; uint32_t beatAt;
         int16_t animIndex; } gs = {40,BRI_DEFAULT,1.0f,0,0,0,0, 255,0,0, 200,200,200, 0,0,0, 0};

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
                                 case 5:return TOGA_P_HUESPEED;} return -1; }
bool isParamBtn(int b){ return paramTarget(b)>=0; }

bool     btnDown[16]={}; uint32_t btnDownT[16]={};

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
volatile bool gsSyncSeen=false;                // our proof the radio works, for the OTA commit
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
}
uint16_t txSeq=0;
void sendCmd(uint8_t cmd,uint8_t target,int16_t arg){
  TogaCmdMsg m={};
  togaInitHeader(m.h,TOGA_MSG_CMD,TOGA_GROUP_ALL,++txSeq);   // group 0 = everyone; targeting UI comes later
  m.cmd=cmd; m.target=target; m.arg=arg;
  m.flags=(btnDown[15]?TOGA_F_STROBE:0)|(btnDown[14]?TOGA_F_MODE_STROBE:0)|(btnDown[13]?TOGA_F_CASTLE_STROBE:0)|(gBlackout?TOGA_F_BLACKOUT:0);
  m.autoMode=autoMode;                                       // state, carried by every packet
  // Unicast retried in hardware; broadcast is unacked and never retried. The
  // heartbeat re-sends the strobe flags 10x/s anyway, but mode/param/beat are
  // one-shots and would be silently swallowed. Send them 3x under ONE seq —
  // receivers apply the first and drop the repeats.
  const int reps=(cmd==TOGA_CMD_HEARTBEAT)?1:3;
  for(int i=0;i<reps;i++) esp_now_send(TOGA_BCAST,(const uint8_t*)&m,sizeof(m));
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
void sendModCfg(const uint8_t mac[6],uint8_t effectPin,uint8_t gain,uint8_t battMon=TOGA_CFG_KEEP){
  TogaModCfgMsg m={};
  togaInitHeader(m.h,TOGA_MSG_MODCFG,TOGA_GROUP_ALL,++txSeq);
  memcpy(m.mac,mac,6);
  m.effectPin=effectPin; m.gain=gain; m.battMon=battMon;
  for(int i=0;i<3;i++) esp_now_send(TOGA_BCAST,(const uint8_t*)&m,sizeof(m));
}

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
           ",\"effects\":[";
  for(uint8_t e=0;e<TOGA_EFFECT_COUNT;e++) j+=(e?",\"":"\"")+String(togaEffectName(e))+"\"";
  j+="],\"follow\":"+String(TOGA_EFFECT_FOLLOW)+",\"mods\":[";
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
    if(e!=TOGA_EFFECT_FOLLOW && (e<0||e>=TOGA_EFFECT_COUNT)){ server.send(400,"text/plain","bad effect"); return; }
    eff=(uint8_t)e;
  }
  if(server.hasArg("gain")){
    int g=server.arg("gain").toInt();
    if(g<10||g>200){ server.send(400,"text/plain","bad gain"); return; }
    gain=(uint8_t)g;
  }
  sendModCfg(mac,eff,gain);
  // No optimistic echo: the row updates when the module's next hello confirms it
  // actually took. The page shows what IS, not what we hope.
  server.send(200,"application/json","{\"sent\":true}");
}
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>TOGA Module</title><style>
body{font:15px system-ui,sans-serif;margin:0;padding:12px;background:#111;color:#eee}
h1{font-size:17px;margin:0 0 10px}
.warn{background:#7a1717;padding:10px;border-radius:6px;margin-bottom:10px;line-height:1.45}
.bar{color:#8a8a8a;font-size:13px;margin-bottom:10px}
.m{background:#1c1c1c;border-radius:8px;padding:10px;margin-bottom:8px;border-left:4px solid #2c2c2c}
.m.on{border-left-color:#2ea043}.m.off{border-left-color:#7a1717;opacity:.65}
.mac{font-family:ui-monospace,monospace;font-size:13px}
.meta{color:#8a8a8a;font-size:12px;margin:3px 0 8px}
.row{display:flex;gap:8px;align-items:center;margin-top:6px;flex-wrap:wrap}
select,input[type=range]{background:#2a2a2a;color:#eee;border:1px solid #3a3a3a;border-radius:5px;padding:5px}
input[type=range]{padding:0;flex:1;min-width:120px}
label{font-size:12px;color:#8a8a8a;min-width:52px}
.pill{font-size:11px;padding:2px 6px;border-radius:9px;background:#2c2c2c;color:#9a9a9a}
.empty{color:#8a8a8a;padding:20px 0;line-height:1.5}
</style></head><body>
<h1>TOGA Module</h1>
<div id="warn"></div><div class="bar" id="bar">…</div>
<div id="list"></div>
<script>
let EFF=[],FOLLOW=255,touching=null;
const macid=m=>m.replace(/:/g,'');
function set(mac,q){fetch('/api/set?mac='+encodeURIComponent(mac)+'&'+q)}
async function tick(){
  let d; try{d=await(await fetch('/api/modules')).json()}catch(e){return}
  EFF=d.effects;FOLLOW=d.follow;
  document.getElementById('warn').innerHTML = d.chOk?'':
    '<div class="warn"><b>Kanal '+d.ch+' statt '+d.chWant+'.</b> Dieser Knoten h&auml;ngt am Router und'+
    ' funkt deshalb auf dessen Kanal &mdash; die Module senden auf '+d.chWant+' und sind hier'+
    ' unsichtbar. Router auf Kanal '+d.chWant+' stellen.</div>';
  document.getElementById('bar').textContent =
    'Kanal '+d.ch+' · Grid '+(d.gridSeen?('sichtbar, Modus '+d.gridMode):'nicht gesehen')+' · '+d.mods.length+' Module';
  const L=document.getElementById('list');
  if(!d.mods.length){L.innerHTML='<div class="empty">Noch kein Modul geh&ouml;rt.'+
    (d.chOk?' Modul eingeschaltet? Es meldet sich einmal pro Sekunde.':'')+'</div>';return}
  L.innerHTML=d.mods.map(m=>{
    const id=macid(m.mac);
    const opts=EFF.map((n,i)=>'<option value="'+i+'"'+(m.eff==i?' selected':'')+'>'+n+'</option>').join('')
      +'<option value="'+FOLLOW+'"'+(m.eff==FOLLOW?' selected':'')+'>folgt Grid</option>';
    return '<div class="m '+(m.online?'on':'off')+'">'+
      '<span class="mac">'+m.mac+'</span> '+
      '<span class="pill">'+(m.online?'online':('weg seit '+m.age+'s'))+'</span> '+
      '<span class="pill">'+(m.link?'Controller ok':'kein Controller')+'</span>'+
      '<div class="meta">'+m.w+'&times;'+m.h+' · Pin '+m.pin+' · Gruppe '+m.grp+
        ' · l&auml;uft: '+(EFF[m.effNow]||'?')+' · up '+m.up+'s</div>'+
      '<div class="row"><label>Effekt</label><select id="e'+id+'">'+opts+'</select></div>'+
      '<div class="row"><label>Gain</label><input type="range" min="10" max="200" value="'+m.gain+
        '" id="g'+id+'"><span id="gv'+id+'">'+m.gain+'%</span></div></div>';
  }).join('');
  d.mods.forEach(m=>{
    const id=macid(m.mac);
    document.getElementById('e'+id).onchange=e=>set(m.mac,'effect='+e.target.value);
    const g=document.getElementById('g'+id);
    g.oninput=e=>{document.getElementById('gv'+id).textContent=e.target.value+'%';touching=id};
    g.onchange=e=>{set(m.mac,'gain='+e.target.value);touching=null};
  });
}
tick();setInterval(()=>{if(!touching)tick()},1500);
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
  if(pressed){
    if(isParamBtn(b) && state==ST_ADJUST && activeBtn==b){ enterDefault(); suppressBtn=b; } // tap active param → off
    if(b==11) sendCmd(TOGA_CMD_BEAT_TAP,0,0);      // send NOW: every ms of delay is phase error
    if(b==0){ autoMode=(uint8_t)(autoMode%TOGA_AUTO_COUNT+1); sendCmd(TOGA_CMD_HEARTBEAT,0,0); } // next mood, wraps 4→1
    if(b==10){ gBlackout=!gBlackout; sendCmd(0,0,0); }  // blackout latch: flip + notify now (don't wait for the 100ms heartbeat)
  } else if(b==suppressBtn) suppressBtn=-1;       // released → hold-to-activate allowed again
  if(b==15||b==14||b==13) sendCmd(0,0,0);        // strobe / mode-strobe / castle-strobe edge → notify immediately
  return 0;
}

// true if the active param is at its limit in direction dir
bool atLimit(int dir){
  switch(activeTarget){
    case 0: return dir>0? gs.brightness>=128 : gs.brightness<=1;
    case 1: return dir>0? gs.animSpeed>=4.0f : gs.animSpeed<=0.1f;
    case 4: return dir>0? gs.hueSpeed>=HUE_SPEED_MAX : gs.hueSpeed<=0;
    default: return false;                       // COLOR / STROBEHUE wrap
  }
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
  // Dim the pad by the grid's global brightness AND this pad's own factor — but do
  // the scaling PER PIXEL (scaleCol) instead of the hardware setBrightness, so the
  // Helligkeit key (button 4) can be exempted below and stay clearly visible while
  // you operate it. Fall back to BRI_DEFAULT when we have never seen a sync (an old
  // grid, or before the first packet) so the pad is never blacked out.
  trellis.pixels.setBrightness(255);              // no hardware scaling; we scale manually
  uint8_t gb=gs.globalBrightness?gs.globalBrightness:BRI_DEFAULT;
  uint16_t g255=(uint16_t)gb*2; if(g255>255)g255=255;   // 1..MAX_BRIGHTNESS → 0..255
  uint8_t padScale=(uint8_t)constrain((int)((uint32_t)g255*ctrlBri/255),30,255);   // floor keeps meters readable
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
      else if(i==13)        col=C(0,45,55);        // Castle-strobe (2nd strobe)
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
      else if(i==13)                col=C(0,140,160);   // castle-strobe held
      else if(i!=4 && !(state==ST_ADJUST && i==activeBtn)) col=C(70,20,0);
    }
    // Everything except the Helligkeit key dims with the global brightness.
    if(i!=4) col=scaleCol(col,padScale);
    // Button 4 (Helligkeit) owns its own indicator, above the param/press logic, and
    // is NOT dimmed — it must stay clearly visible while you operate it:
    // slow breathe = GLOBAL, fast breathe = MODE factor, solid white = CONTROLLER,
    // a steady medium white at rest.
    if(i==4){
      if(briMode!=BRI_NONE) col=briBtnCol(t);
      else col=C(70,70,70);                          // clearly visible at rest
    }
    // Button 10 (Blackout): bright while active, a dim marker at rest so the key is
    // findable. Not dimmed by the global brightness — it must stay visible in the dark.
    if(i==10) col = gBlackout ? C(200,200,200) : C(25,25,25);
    // While blacked out, kill every pad LED except the ones the operator still uses:
    // blackout(10), strobe(15), mode-strobe(14), castle-strobe(13) and auto(0).
    if(gBlackout && i!=0 && i!=10 && i!=13 && i!=14 && i!=15) col=C(0,0,0);
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

  Wire.begin();
  if(!trellis.begin()){ Serial.println(F("NeoTrellis not found — check I2C")); while(1) delay(1000); }
  for(int i=0;i<16;i++){
    trellis.activateKey(i,SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i,SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i,handleKey);
  }
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
  togaOtaSetup("toga-ctl");                          // flashable from now on, no trigger

  // Module-manager web UI, served on our own AP. Reach it from a phone joined to
  // togalights at http://192.168.4.1/ (the SoftAP gateway). mDNS is best-effort:
  // togaOtaSetup already began an mDNS instance, we just advertise HTTP on it —
  // handy on a laptop, flaky over SoftAP on iOS, hence the IP is the sure path.
  MDNS.addService("http", "tcp", 80);
  server.on("/", handleRoot);
  server.on("/api/modules", handleModules);
  server.on("/api/set", handleSet);
  server.onNotFound([](){ server.send(404,"text/plain","404"); });
  server.begin();
  Serial.println(F("http ready on :80  (http://192.168.4.1/)"));
}

// Who is on our AP? The fleet's nodes join it to be flashable, so an empty list
// while a board is powered means that board is not reachable for OTA — the one
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

  // Above the 20ms poll gate below: that returns on most iterations, and OTA
  // and the web UI must not be starved by it.
  togaOtaHandle();
  server.handleClient();                            // the module-manager page
  drainHellos(t);                                   // merge parked hellos into the table

  if(t-lastHB>=100){ lastHB=t; sendCmd(0,0,0); }   // heartbeat (carries strobe state)

  // Our bus evidence is the grid's sync — we transmit, the grid answers. Without
  // this call the image stays PENDING_VERIFY forever and the NEXT reset of any
  // kind quietly reverts to the old firmware.
  togaOtaMarkValidWhenHealthy(t, gsSyncSeen);
  reportApClients(t);

  if(t-lastPoll<20) return;
  lastPoll=t;
  trellis.read();                                  // fires handleKey

  // ── Hold a param button 2s → activate ADJUST (it starts blinking) ──
  const int pbs[4]={8,12,1,5};                   // button 4 has its own machine below
  for(int k=0;k<4;k++){ int b=pbs[k];
    if(b==suppressBtn) continue;                 // don't re-arm the button we just tapped off
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
  if(b4){
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
  if(btnDown[15]){                                  // strobe held → +/- tune the gap between flashes
    if(plus && !prevPlus){ plusPressT=t; plusRepT=t; sendCmd(2,TOGA_P_STROBEGAP,+1); }
    else if(plus){ if(t-plusPressT>=400 && t-plusRepT>=60){ plusRepT=t; sendCmd(2,TOGA_P_STROBEGAP,+1); } }
    if(minus && !prevMinus){ minusPressT=t; minusRepT=t; sendCmd(2,TOGA_P_STROBEGAP,-1); }
    else if(minus){ if(t-minusPressT>=400 && t-minusRepT>=60){ minusRepT=t; sendCmd(2,TOGA_P_STROBEGAP,-1); } }
  } else if(briMode!=BRI_NONE){                     // brightness edit-mode → +/- adjust the selected brightness
    if(plus && !prevPlus){ plusPressT=t; plusRepT=t; briAdjust(+1); }
    else if(plus){ uint32_t h=t-plusPressT; if(h>=500){ uint32_t rate=h<2000?80:(h<4000?40:16); if(t-plusRepT>=rate){ plusRepT=t; briAdjust(+1); } } }
    if(minus && !prevMinus){ minusPressT=t; minusRepT=t; briAdjust(-1); }
    else if(minus){ uint32_t h=t-minusPressT; if(h>=500){ uint32_t rate=h<2000?80:(h<4000?40:16); if(t-minusRepT>=rate){ minusRepT=t; briAdjust(-1); } } }
  } else if(state==ST_DEFAULT){
    // Taking the mode over by hand ends the automatic — clear it BEFORE the
    // send, so the very packet that steps the mode also carries autoMode=0.
    if((plus && !prevPlus)||(minus && !prevMinus)) autoMode=TOGA_AUTO_OFF;
    if(plus  && !prevPlus)  sendCmd(1,0,+1);        // next mode
    if(minus && !prevMinus) sendCmd(1,0,-1);        // prev mode
  } else if(state==ST_ADJUST){
    if(plus && !prevPlus){ plusPressT=t; plusRepT=t; sendCmd(2,activeTarget,+1); }
    else if(plus){ uint32_t h=t-plusPressT; if(h>=500){ uint32_t rate=h<2000?80:(h<4000?40:16); if(t-plusRepT>=rate){ plusRepT=t; sendCmd(2,activeTarget,+1); } } }
    if(minus && !prevMinus){ minusPressT=t; minusRepT=t; sendCmd(2,activeTarget,-1); }
    else if(minus){ uint32_t h=t-minusPressT; if(h>=500){ uint32_t rate=h<2000?80:(h<4000?40:16); if(t-minusRepT>=rate){ minusRepT=t; sendCmd(2,activeTarget,-1); } } }
  }
  prevPlus=plus; prevMinus=minus;

  // ADJUST stays until the active param button is tapped again (no timeout).

  // Persist the controller pad factor lazily (flash wear): 1.5s after the last edit.
  if(ctrlBriDirty && (t-ctrlBriDirtyAt)>=1500){ ctrlBriDirty=false; saveCtrlBri(); }

  renderLEDs(t);
}
