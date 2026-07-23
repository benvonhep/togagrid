// ═══════════════════════════════════════════════════════
//  Toga Webserver — lists every module on the bus and configures them one
//  by one from a phone.
//
//  It drives no LEDs and commands no show. It listens for TogaHelloMsg (every
//  module announces itself once a second), keeps a table of who is out there,
//  and sends TogaModCfgMsg addressed at a single MAC when you change something.
//  Nothing depends on this node: unplug it and the installation runs on.
//
//  It joins the controller's AP (togalights) like every other node. That AP is
//  pinned to TOGA_CHANNEL, so being on the network costs it nothing on the bus.
//  This used to be the fleet's one exception — the only board on a WLAN, and
//  that WLAN was a router on whatever channel it felt like, which could leave
//  this node deaf to the very bus it exists to watch. The page still shows the
//  channel and warns on a mismatch; it should now never fire.
//
//  Board: ESP32 Dev Module · Partition: min_spiffs (OTA needs 2 app slots) · core 2.0.17
// ═══════════════════════════════════════════════════════
#include <Arduino.h>
#include <toga_proto.h>          // the wire format, shared with grid/ and module/
#include <toga_ota.h>            // always-on firmware update, same as every node
#include <WebServer.h>
#include <ESPmDNS.h>
#include "togaws_config.h"

WebServer server(80);

// ── The module table ──────────────────────────────────
// Modules are listed as they announce themselves; an entry that stops being
// heard goes `offline` but STAYS on the page. A module that dropped out is
// exactly what you want to see — removing the row would hide the failure.
#define MAX_MODULES  16
#define OFFLINE_MS   5000        // 5 missed hellos → offline
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
  char     name[16];             // user label (module-side, echoed in its hello)
  uint32_t lastSeen;             // millis() on THIS node
};
ModEntry mods[MAX_MODULES];

// ── Inbound, buffered ─────────────────────────────────
// The recv callback runs in the WiFi task while HTTP handlers read the table
// from loop(). Rather than lock the table, the callback only parks the packet
// in a ring and loop() merges it — the same "buffer in the callback, apply in
// the main loop" split the modules use.
#define HQ_LEN 8
volatile TogaHelloMsg helloQ[HQ_LEN];
volatile uint8_t hqHead = 0, hqTail = 0;
volatile TogaSyncMsg  syncBuf;
volatile bool     syncSeen = false;
volatile uint32_t syncAt   = 0;

void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  TogaHelloMsg hm;
  if (togaParseHello(data, len, &hm)) {
    uint8_t nh = (uint8_t)((hqHead + 1) % HQ_LEN);
    if (nh != hqTail) { memcpy((void*)&helloQ[hqHead], &hm, sizeof(hm)); hqHead = nh; }
    return;
  }
  TogaSyncMsg s;
  if (togaParseSync(data, len, &s)) {
    memcpy((void*)&syncBuf, &s, sizeof(s));
    syncSeen = true; syncAt = millis();
  }
}

// Find this MAC, or claim a row for it. A full table means more boards than
// MAX_MODULES have ever been heard, so the stalest row is the one nobody will
// miss. Never returns null: there is always a row to recycle.
ModEntry* upsert(const uint8_t mac[6]) {
  for (int i = 0; i < MAX_MODULES; i++)
    if (mods[i].used && memcmp(mods[i].mac, mac, 6) == 0) return &mods[i];

  ModEntry* slot = nullptr;
  for (int i = 0; i < MAX_MODULES; i++)                    // a free row first
    if (!mods[i].used) { slot = &mods[i]; break; }
  if (!slot) {                                             // else the stalest one
    slot = &mods[0];
    for (int i = 1; i < MAX_MODULES; i++)
      if (mods[i].lastSeen < slot->lastSeen) slot = &mods[i];
  }
  memset(slot, 0, sizeof(*slot));
  slot->used = true;
  memcpy(slot->mac, mac, 6);
  return slot;
}

void drainHellos(uint32_t t) {
  while (hqTail != hqHead) {
    TogaHelloMsg m; memcpy(&m, (const void*)&helloQ[hqTail], sizeof(m));
    hqTail = (uint8_t)((hqTail + 1) % HQ_LEN);
    ModEntry* e = upsert(m.mac);
    e->group = m.group; e->effectPin = m.effectPin; e->effectNow = m.effectNow;
    e->gain  = m.gain;  e->pin = m.pin; e->linkAlive = m.linkAlive;
    e->w = m.width; e->h = m.height; e->uptimeS = m.uptimeS;
    e->battMv = m.battMv; e->battFlags = m.battFlags;
    e->followMask = m.followMask;
    e->gap = m.gap;
    e->floor = m.floorFlow;
    memcpy(e->name, m.name, sizeof(e->name)); e->name[sizeof(e->name) - 1] = 0;
    e->lastSeen = t;
  }
  for (int i = 0; i < MAX_MODULES; i++)                       // long-gone rows expire
    if (mods[i].used && (t - mods[i].lastSeen) > FORGET_MS) mods[i].used = false;
}

String macStr(const uint8_t m[6]) {
  char b[18]; snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", m[0],m[1],m[2],m[3],m[4],m[5]);
  return String(b);
}
bool parseMac(const String& s, uint8_t out[6]) {
  int v[6];
  if (sscanf(s.c_str(), "%x:%x:%x:%x:%x:%x", &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]) != 6) return false;
  for (int i = 0; i < 6; i++) { if (v[i] < 0 || v[i] > 255) return false; out[i] = (uint8_t)v[i]; }
  return true;
}

// ── Send settings to ONE module ───────────────────────
uint16_t txSeq = 0;
void sendModCfg(const uint8_t mac[6], uint8_t effectPin, uint8_t gain, uint8_t battMon = TOGA_CFG_KEEP,
                uint8_t followMask = TOGA_FOLLOW_KEEP, uint16_t ledCount = 0, const char* name = nullptr,
                uint8_t gap = TOGA_GAP_KEEP, uint8_t floorFlow = TOGA_FLOOR_KEEP) {
  TogaModCfgMsg m = {};
  togaInitHeader(m.h, TOGA_MSG_MODCFG, TOGA_GROUP_ALL, ++txSeq);
  memcpy(m.mac, mac, 6);
  m.effectPin = effectPin; m.gain = gain; m.battMon = battMon;
  m.followMask = followMask; m.ledCount = ledCount; m.gap = gap; m.floorFlow = floorFlow;
  if (name && name[0]) strncpy(m.name, name, sizeof(m.name) - 1);   // m is zero-init → NUL kept; "" = keep
  // Broadcast is unacked and never retried, so send it 3x. Unlike a +1 step
  // these are absolute values — applying the same one twice is a no-op, which
  // is why the repeats need no dedup on the module side.
  for (int i = 0; i < 3; i++) esp_now_send(TOGA_BCAST, (const uint8_t*)&m, sizeof(m));
}

// ── HTTP ──────────────────────────────────────────────
bool channelOk() { return WiFi.channel() == TOGA_CHANNEL; }

// Minimal JSON string escaper for the user-set name. handleSet() already strips
// names to a safe charset, so this only guards against a module reporting junk.
String jsonEsc(const char* s) {
  String o;
  for (const char* p = s; *p; p++) {
    char c = *p;
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c >= 0x20) o += c;   // drop control chars
  }
  return o;
}

void handleModules() {
  uint32_t t = millis();
  String j = "{\"ch\":" + String(WiFi.channel()) +
             ",\"chWant\":" + String(TOGA_CHANNEL) +
             ",\"chOk\":" + String(channelOk() ? "true" : "false") +
             ",\"gridSeen\":" + String((syncSeen && (t - syncAt) < 2000) ? "true" : "false") +
             ",\"gridMode\":" + String(syncSeen ? syncBuf.animIndex + 1 : 0) +
             ",\"mods\":[";   // 90 mode names come from /api/effects (static, cached)
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
         ",\"name\":\"" + jsonEsc(mods[i].name) + "\"" +
         ",\"follow\":" + String(mods[i].followMask) +
         ",\"gap\":" + String(mods[i].gap) +
         ",\"floor\":" + String(mods[i].floor) +
         ",\"link\":" + String(mods[i].linkAlive ? "true" : "false") +
         ",\"batt\":" + String(mods[i].battMv / 1000.0, 2) +
         ",\"bmon\":" + String((mods[i].battFlags & TOGA_BATT_F_ENABLED) ? "true" : "false") +
         ",\"blow\":" + String((mods[i].battFlags & TOGA_BATT_F_LOW) ? "true" : "false") +
         ",\"up\":" + String(mods[i].uptimeS) + "}";
  }
  j += "]}";
  server.send(200, "application/json", j);
}

void handleSet() {
  uint8_t mac[6];
  if (!server.hasArg("mac") || !parseMac(server.arg("mac"), mac)) {
    server.send(400, "text/plain", "bad mac"); return;
  }
  uint8_t eff = TOGA_CFG_KEEP, gain = 0, battMon = TOGA_CFG_KEEP;  // 0 / KEEP = "leave this one alone"
  if (server.hasArg("effect")) {
    int e = server.arg("effect").toInt();
    bool ok = (e >= 0 && e < TOGA_EFFECT_COUNT) || e == TOGA_EFFECT_OFF || e == TOGA_EFFECT_FOLLOW || e == TOGA_EFFECT_IDENTIFY;
    if (!ok) { server.send(400, "text/plain", "bad effect"); return; }
    eff = (uint8_t)e;
  }
  if (server.hasArg("gain")) {
    int g = server.arg("gain").toInt();
    if (g < 10 || g > 200) { server.send(400, "text/plain", "bad gain"); return; }
    gain = (uint8_t)g;
  }
  if (server.hasArg("bmon")) battMon = server.arg("bmon").toInt() ? 1 : 0;

  uint8_t  follow = TOGA_FOLLOW_KEEP;
  uint16_t leds   = 0;
  char     name[16] = {0};
  if (server.hasArg("follow")) {
    int f = server.arg("follow").toInt();
    if (f < 0 || f > TOGA_FOLLOW_ALL) { server.send(400, "text/plain", "bad follow"); return; }
    follow = (uint8_t)f;
  }
  if (server.hasArg("leds")) {
    int n = server.arg("leds").toInt();
    if (n < 1 || n > 512) { server.send(400, "text/plain", "bad leds"); return; }   // module re-clamps to its MAX_LEDS
    leds = (uint16_t)n;
  }
  uint8_t gap = TOGA_GAP_KEEP;
  if (server.hasArg("gap")) {                          // 1 = every LED (off), N = light every Nth
    int g = server.arg("gap").toInt();
    if (g < TOGA_GAP_OFF || g > TOGA_GAP_MAX) { server.send(400, "text/plain", "bad gap"); return; }
    gap = (uint8_t)g;
  }
  uint8_t floor = TOGA_FLOOR_KEEP;
  if (server.hasArg("floor")) {                        // 0 off, 1 fwd, 2 rev
    int f = server.arg("floor").toInt();
    if (f < TOGA_FLOOR_OFF || f > TOGA_FLOOR_MAX) { server.send(400, "text/plain", "bad floor"); return; }
    floor = (uint8_t)f;
  }
  if (server.hasArg("name")) {                       // sanitize to a safe, JSON-trivial charset
    String s = server.arg("name"); int j = 0;
    for (int i = 0; i < (int)s.length() && j < 15; i++) {
      char c = s[i];
      if (isalnum((int)c) || c == ' ' || c == '-' || c == '_') name[j++] = c;
    }
    name[j] = 0;
    if (j == 0) { server.send(400, "text/plain", "bad name"); return; }
  }
  sendModCfg(mac, eff, gain, battMon, follow, leds, name, gap, floor);
  // Deliberately no optimistic echo: the row updates when the module's next
  // hello confirms it actually took. The UI must show what IS, not what we hope.
  server.send(200, "application/json", "{\"sent\":true}");
}

// The 90 mode names, served once and cached. Split off /api/modules (which polls
// every 1.5s) so 90 strings do not ride every poll — the list is static.
void handleEffects() {
  String j = "{\"names\":[";
  for (uint8_t e = 0; e < TOGA_EFFECT_COUNT; e++) j += (e ? ",\"" : "\"") + String(togaEffectName(e)) + "\"";
  j += "],\"follow\":" + String(TOGA_EFFECT_FOLLOW) + ",\"off\":" + String(TOGA_EFFECT_OFF) + "}";
  server.sendHeader("Cache-Control", "max-age=3600");
  server.send(200, "application/json", j);
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
select,input[type=range],input[type=text],input[type=number]{background:#2a2a2a;color:#eee;border:1px solid #3a3a3a;border-radius:5px;padding:5px}
input[type=range]{padding:0;flex:1;min-width:120px}
input[type=number]{width:74px}
.nm{font-size:15px;font-weight:600;min-width:110px;flex:1}
.slave{gap:14px}
.slave label{min-width:auto;color:#cfcfcf;display:inline-flex;align-items:center;gap:4px}
.hint{font-size:11px;color:#7a7a7a}
label{font-size:12px;color:#8a8a8a;min-width:52px}
.pill{font-size:11px;padding:2px 6px;border-radius:9px;background:#2c2c2c;color:#9a9a9a}
.empty{color:#8a8a8a;padding:20px 0;line-height:1.5}
button{background:#2a2a2a;color:#eee;border:1px solid #3a3a3a;border-radius:5px;padding:5px 9px;font:inherit;font-size:12px;cursor:pointer}
button:hover{background:#3a3a3a}
.ota{margin-left:auto}
.idf{margin-left:auto}
.tools{display:flex;gap:8px;margin-bottom:10px}
</style></head><body>
<h1>TOGA Module</h1>
<div id="warn"></div><div class="bar" id="bar">…</div>
<div id="list"></div>
<script>
let EFF=[],ready=false,OPTS='';
const FOLLOW=255,OFF=200;
// Follow-mask bits (mirror toga_proto.h). MODE (0x04) is reserved — the effect
// dropdown already expresses mode-follow — so the UI preserves it untouched.
const F_STROBE=1,F_BRIGHT=2,F_MODE=4,F_BEAT=8,F_BLACK=16;
const IDENT=201;   // TOGA_EFFECT_IDENTIFY — momentary "blink to find this strip"
const GROUPS=[[0,25,'Feld'],[26,34,'Geometrie'],[35,39,'Leben'],[40,51,'Spiele'],[52,58,'Gesichter'],[59,89,'Weltraum']];
const macid=m=>m.replace(/:/g,'');
const nameOf=v=>v==FOLLOW?'folgt Grid':(v==OFF?'aus':(EFF[v]||('#'+v)));
function set(mac,q){fetch('/api/set?mac='+encodeURIComponent(mac)+'&'+q)}
function optsHTML(){
  let h='';
  for(const gr of GROUPS){h+='<optgroup label="'+gr[2]+'">';
    for(let i=gr[0];i<=gr[1];i++)h+='<option value="'+i+'">'+i+' '+(EFF[i]||'')+'</option>';
    h+='</optgroup>';}
  h+='<option value="'+FOLLOW+'">folgt Grid</option><option value="'+OFF+'">aus</option>';
  return h;
}
// ── Per-control confirm lock ──────────────────────────────────────────────
// The poll must never yank a control out from under a finger — that is the
// two-phone glitch. valOf/pend mark a control "pending" the moment the user
// changes it; put() then refuses to overwrite it until the module's own hello
// confirms the new value (pend matches) or ~4s pass (assume the packet was
// lost). A focused control is never touched either. So phone B's poll can no
// longer reset phone A's open dropdown, and your own pick never snaps back.
const valOf=el=>el.type==='checkbox'?(el.checked?'1':'0'):(''+el.value);
function pend(el){el._p=valOf(el);el._t=Date.now()}
function put(el,rep){
  rep=''+rep;
  if(el===document.activeElement)return;
  if(el._p!=null){ if(el._p===rep){el._p=null} else if(Date.now()-el._t>4000){el._p=null} else return; }
  if(el.type==='checkbox')el.checked=(rep==='1');else el.value=rep;
}
const rows={};                                   // mac -> refs, built once, patched in place
function makeCard(mac){
  const c=document.createElement('div'); c.className='m';
  c.innerHTML=
    '<div class="row"><input class="nm" type="text" maxlength="15"> <span class="mac"></span>'+
      ' <span class="pill st"></span> <span class="pill lk"></span> <span class="pill vt"></span>'+
      '<button class="idf" title="Streifen blinken lassen">&#128161; Finden</button></div>'+
    '<div class="meta"></div>'+
    '<div class="row"><label>Modus</label><select class="ef">'+OPTS+'</select></div>'+
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
      '<label style="min-width:auto"><input type="checkbox" class="bm"> Unterspannungs-Schutz</label></div>';
  const q=s=>c.querySelector(s);
  const r={card:c,nm:q('.nm'),mac:q('.mac'),st:q('.st'),lk:q('.lk'),vt:q('.vt'),meta:q('.meta'),ef:q('.ef'),fm:q('.fm'),idf:q('.idf'),
           fs:q('.fs'),fb:q('.fb'),fe:q('.fe'),fk:q('.fk'),gn:q('.gn'),gv:q('.gv'),ld:q('.ld'),gp:q('.gp'),fl:q('.fl'),bt:q('.bt'),bm:q('.bm'),follow:0,effNow:255};
  // Identify: momentary blink so you can find this strip. Not an effect change — sent raw, no pending.
  r.idf.onclick=()=>set(mac,'effect='+IDENT);
  const sendFollow=()=>{ let mk=(r.follow&F_MODE);              // keep the reserved mode bit as-is
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
  r.bm.onchange=e=>{pend(e.target);set(mac,'bmon='+(e.target.checked?1:0))};
  return r;
}
async function loadEffects(){try{const r=await(await fetch('/api/effects')).json();EFF=r.names;OPTS=optsHTML();ready=true}catch(e){}}
async function tick(){
  if(!ready){await loadEffects(); if(!ready)return}
  let d; try{d=await(await fetch('/api/modules')).json()}catch(e){return}
  document.getElementById('warn').innerHTML = d.chOk?'':
    '<div class="warn"><b>Kanal '+d.ch+' statt '+d.chWant+'.</b> Dieser Knoten h&auml;ngt am Router und'+
    ' funkt deshalb auf dessen Kanal &mdash; die Module senden auf '+d.chWant+' und sind hier'+
    ' unsichtbar. Router auf Kanal '+d.chWant+' stellen.</div>';
  document.getElementById('bar').textContent =
    'Kanal '+d.ch+' · Grid '+(d.gridSeen?('sichtbar, Modus '+d.gridMode):'nicht gesehen')+' · '+d.mods.length+' Module';
  const L=document.getElementById('list');
  const seen={};
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
    // Spannung: always visible per module. battVolt is sampled every 10s on the
    // module regardless of the undervoltage monitor, so this shows even with bmon off.
    r.vt.innerHTML='&#9889; '+(m.batt>0?m.batt.toFixed(2)+'&nbsp;V':'&ndash;')+(m.blow?' &#9888;':'');
    r.meta.innerHTML=m.w+'&times;'+m.h+' · Pin '+m.pin+' · Gruppe '+m.grp+' · l&auml;uft: '+nameOf(m.effNow)+' · up '+m.up+'s';
    put(r.nm,m.name);
    put(r.ef,m.eff);
    r.effNow=m.effNow;                          // remembered so unchecking Grid-Modus can pin it
    put(r.fm,(m.eff==FOLLOW)?1:0);
    r.follow=m.follow;
    put(r.fs,(m.follow&F_STROBE)?1:0); put(r.fb,(m.follow&F_BRIGHT)?1:0);
    put(r.fe,(m.follow&F_BEAT)?1:0);   put(r.fk,(m.follow&F_BLACK)?1:0);
    put(r.gn,m.gain); r.gv.textContent=r.gn.value+'%';
    put(r.ld,m.w*m.h);
    put(r.gp,m.gap);
    put(r.fl,m.floor);
    r.bt.innerHTML=(m.batt>0||m.bmon)?('<span class="pill">&#128267; '+m.batt.toFixed(2)+' V'+(m.blow?' &#9888;':'')+'</span>'):'';
    put(r.bm,m.bmon?1:0);
  });
  for(const id in rows) if(!seen[id]){ rows[id].card.remove(); delete rows[id]; }
}
tick();setInterval(tick,1500);          // poll always: put() protects in-flight edits, no global pause needed
</script></body></html>)HTML";

void handleRoot() { server.send_P(200, "text/html", PAGE); }

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("\n─── TOGA WEBSERVER ───"));

  // Joins the controller's AP like every other node now. This used to be the
  // fleet's one exception — the only board on a WLAN — and that WLAN was a
  // router on an unknown channel, which is exactly why it could end up deaf to
  // the very bus it exists to watch. The AP is ours and pinned to TOGA_CHANNEL,
  // so associating costs nobody their place on the bus and that whole failure
  // mode is gone.
  togaWifiBegin(WS_HOSTNAME);
  Serial.printf("joining %s", TOGA_AP_SSID);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) { delay(500); Serial.print("."); }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("ip       %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("hostname http://%s.local/\n", WS_HOSTNAME);
    if (MDNS.begin(WS_HOSTNAME)) {
      MDNS.addService("http", "tcp", 80);
      MDNS.enableArduino(TOGA_OTA_PORT, true);   // advertise OTA on OUR mDNS instance
    }
  } else {
    // No reboot loop: ESP-NOW still works, and the serial console still shows
    // every module. Losing the WLAN must not cost us the diagnostics.
    Serial.println(F("WLAN: no connection — check togaws_config.h. Listening anyway."));
  }

  Serial.printf("channel  %d (want %d)\n", WiFi.channel(), TOGA_CHANNEL);
  if (!channelOk()) {
    Serial.println(F("*** CHANNEL MISMATCH — no module will ever be heard. ***"));
    Serial.println(F("*** The router must be on TOGA_CHANNEL. See togaws_config.h. ***"));
  }

  if (esp_now_init() != ESP_OK) { Serial.println(F("ESP-NOW init failed!")); while (1) delay(1000); }
  esp_now_register_recv_cb(onReceive);
  // NOT togaAddPeer(): that pins the peer to TOGA_CHANNEL, and we are on
  // whatever channel the router put us on. Channel 0 = "use the interface's
  // current channel", which is the only honest answer for this node.
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, TOGA_BCAST, 6);
  p.channel = 0;
  p.ifidx   = WIFI_IF_STA;
  p.encrypt = false;
  esp_err_t e = esp_now_add_peer(&p);
  if (e != ESP_OK && e != ESP_ERR_ESPNOW_EXIST) Serial.println(F("broadcast peer failed — cannot configure modules"));

  togaOtaSetup(WS_HOSTNAME);   // same path as every other node now

  server.on("/", handleRoot);
  server.on("/api/modules", handleModules);
  server.on("/api/effects", handleEffects);
  server.on("/api/set", handleSet);
  server.onNotFound([]() { server.send(404, "text/plain", "404"); });
  server.begin();
  Serial.println(F("http ready on :80"));
}

void loop() {
  uint32_t t = millis();
  drainHellos(t);
  server.handleClient();
  togaOtaHandle();
  togaWifiTick();      // the AP may come up after us, or come back

  // Serial mirror of the page, once every 5s — the fallback when the WLAN is
  // the thing that is broken.
  static uint32_t lastDump = 0;
  if (t - lastDump > 5000) {
    lastDump = t;
    int n = 0;
    for (int i = 0; i < MAX_MODULES; i++) if (mods[i].used) n++;
    Serial.printf("[ch=%d %s] %d module(s)\n", WiFi.channel(), channelOk() ? "ok" : "MISMATCH", n);
    for (int i = 0; i < MAX_MODULES; i++) {
      if (!mods[i].used) continue;
      uint32_t age = (t - mods[i].lastSeen) / 1000;
      char batt[24] = "";
      if (mods[i].battFlags & TOGA_BATT_F_ENABLED)
        snprintf(batt, sizeof(batt), " batt=%.2fV%s", mods[i].battMv / 1000.0,
                 (mods[i].battFlags & TOGA_BATT_F_LOW) ? " LOW" : "");
      Serial.printf("  %s %ux%u pin=%u grp=%u eff=%s now=%s gain=%u%% up=%us %s%s\n",
                    macStr(mods[i].mac).c_str(), mods[i].w, mods[i].h, mods[i].pin, mods[i].group,
                    togaEffectName(mods[i].effectPin), togaEffectName(mods[i].effectNow),
                    mods[i].gain, mods[i].uptimeS,
                    age < (OFFLINE_MS / 1000) ? "online" : "OFFLINE", batt);
    }
  }
}
