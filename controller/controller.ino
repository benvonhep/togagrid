// ═══════════════════════════════════════════════════════
//  NeoTrellis 4x4 Sender — parameter-adjust interaction model
//
//  DEFAULT:  + (3) / − (7) change MODE.  0 = Modus indicator.
//  Hold a param button 2s → ADJUST that param (it blinks, all others
//    black except +/−; +/− then change only that param w/ accel).
//  3s of no +/− activity → back to DEFAULT.
//  Beat tap (11): tap-tempo → global animSpeed. Exits on 3s idle.
//  Strobe (15): momentary while held.
//
//  Params:  4 Helligkeit · 8 Speed · 12 Color · 1 Strobe-colour · 5 Hue-speed
// ═══════════════════════════════════════════════════════
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include "Adafruit_NeoTrellis.h"

uint8_t receiverMAC[] = {0xF4,0x65,0x0B,0xE8,0x98,0x60};
Adafruit_NeoTrellis trellis;

// ── Messages (must match grid_SW.ino) ──
typedef struct { uint8_t cmd; uint8_t target; int16_t arg; bool strobe; bool modeStrobe; bool castleStrobe; } GridMsg;
typedef struct { uint8_t brightness,strobeBrightness,strobeOnMs,strobeOffMs,strobeSquares;
                 uint8_t hueBase,hueSpeed,strobeHue;
                 uint8_t tintR,tintG,tintB, sColR,sColG,sColB;
                 int animIndex; float animSpeed; } SyncMsg;

#define HOLD_TO_SELECT     2000
#define INACTIVITY_TIMEOUT 3000

// ── Synced state from the grid (for at-limit display) ──
struct { uint8_t brightness; float animSpeed; uint8_t hueBase,hueSpeed,strobeHue;
         uint8_t tintR,tintG,tintB, sColR,sColG,sColB; } gs = {40,1.0f,0,0,0, 255,0,0, 200,200,200};

// ── Button map ──
// param button → target id (0=BRI 1=SPEED 2=COLOR 3=STROBEHUE 4=HUESPEED)
int paramTarget(int b){ switch(b){case 4:return 0;case 8:return 1;case 12:return 2;case 1:return 3;case 5:return 4;} return -1; }
bool isParamBtn(int b){ return paramTarget(b)>=0; }

bool     btnDown[16]={}; uint32_t btnDownT[16]={};

// ── Interaction state machine ──
enum { ST_DEFAULT, ST_ADJUST, ST_TAP };
int  state=ST_DEFAULT, activeBtn=-1, activeTarget=-1;
int  suppressBtn=-1;                 // button that just deactivated a param (ignore its hold until released)
uint32_t lastActivity=0;
uint32_t plusPressT=0,minusPressT=0,plusRepT=0,minusRepT=0;

#define MAX_TAPS 5
uint32_t taps[MAX_TAPS]={}; int tapCount=0;

uint32_t C(uint8_t r,uint8_t g,uint8_t b){ return trellis.pixels.Color(r,g,b); }

// ── ESP-NOW ──
bool peerAdded=false;
void ensurePeer(){ if(peerAdded)return; esp_now_peer_info_t p={}; memcpy(p.peer_addr,receiverMAC,6); p.channel=0;p.encrypt=false; if(esp_now_add_peer(&p)==ESP_OK)peerAdded=true; }
void onSent(const uint8_t*mac,esp_now_send_status_t s){ if(s!=ESP_NOW_SEND_SUCCESS){ esp_now_del_peer(receiverMAC); peerAdded=false; } }
void onReceive(const uint8_t*mac,const uint8_t*data,int len){
  if(len!=sizeof(SyncMsg))return; SyncMsg m; memcpy(&m,data,sizeof(m));
  gs.brightness=m.brightness; gs.animSpeed=m.animSpeed; gs.hueBase=m.hueBase; gs.hueSpeed=m.hueSpeed; gs.strobeHue=m.strobeHue;
  gs.tintR=m.tintR; gs.tintG=m.tintG; gs.tintB=m.tintB; gs.sColR=m.sColR; gs.sColG=m.sColG; gs.sColB=m.sColB;
}
void sendCmd(uint8_t cmd,uint8_t target,int16_t arg){
  ensurePeer(); GridMsg m; m.cmd=cmd; m.target=target; m.arg=arg; m.strobe=btnDown[15]; m.modeStrobe=btnDown[14]; m.castleStrobe=btnDown[13];
  esp_now_send(receiverMAC,(uint8_t*)&m,sizeof(m));
}

void enterDefault(){ state=ST_DEFAULT; activeBtn=-1; activeTarget=-1; tapCount=0; }
void enterAdjust(int b){ state=ST_ADJUST; activeBtn=b; activeTarget=paramTarget(b); lastActivity=millis(); }
void enterTap(){ state=ST_TAP; tapCount=0; lastActivity=millis(); }

// ── Trellis key callback ──
TrellisCallback handleKey(keyEvent evt){
  uint8_t b=evt.bit.NUM; bool pressed=(evt.bit.EDGE==SEESAW_KEYPAD_EDGE_RISING);
  btnDown[b]=pressed; if(pressed) btnDownT[b]=millis();
  if(pressed){
    if(isParamBtn(b) && state==ST_ADJUST && activeBtn==b){ enterDefault(); suppressBtn=b; } // tap active param → off
  } else if(b==suppressBtn) suppressBtn=-1;       // released → hold-to-activate allowed again
  if(b==15||b==14||b==13) sendCmd(0,0,0);        // strobe / mode-strobe / castle-strobe edge → notify immediately
  return 0;
}

// true if the active param is at its limit in direction dir
bool atLimit(int dir){
  switch(activeTarget){
    case 0: return dir>0? gs.brightness>=128 : gs.brightness<=1;
    case 1: return dir>0? gs.animSpeed>=4.0f : gs.animSpeed<=0.1f;
    case 4: return dir>0? gs.hueSpeed>=40    : gs.hueSpeed<=0;
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
    case 5:  f=gs.hueSpeed/40.0f; break;              // Hue-speed 0..40
    case 12: return rgbScale(gs.tintR,gs.tintG,gs.tintB,v);  // Color → exact grid tint
    case 1:  return strobeCol(v);                            // Strobe-colour → actual strobe colour
    default: return C(v,v,v);
  }
  return heatCol(f,v);
}
void renderLEDs(uint32_t t){
  bool blink=((t/125)%2)==0;                      // ~4 Hz
  for(int i=0;i<16;i++){
    uint32_t col=C(0,0,0);
    if(state==ST_ADJUST){
      if(i==activeBtn)      col=blink?paramCol(i,170):paramCol(i,22);  // flash in its own colour
      else if(i==3||i==7)   col=C(0,60,80);
      else if(i==15 && activeTarget==3) col=strobeCol(90);   // live strobe-colour preview
    } else if(state==ST_TAP){
      if(i==11)             col=blink?C(160,0,0):C(15,0,0);
    } else { // DEFAULT (+/- select the mode when no param is active)
      if(i==3||i==7)        col=C(0,60,80);        // +/-
      else if(isParamBtn(i)) col=paramCol(i,32);   // each param in its own colour
      else if(i==11)        col=C(40,0,0);         // Beat tap
      else if(i==13)        col=C(0,45,55);        // Castle-strobe (2nd strobe)
      else if(i==14)        col=C(45,20,45);       // Mode-strobe
      else if(i==15)        col=strobeCol(70);     // Strobe shows its set colour
    }
    // press feedback
    if(btnDown[i]){
      if(i==15)                     col=strobeCol(255);
      else if(i==14)                col=rgbScale(gs.tintR,gs.tintG,gs.tintB,255);  // mode-strobe flash colour
      else if(i==13)                col=C(0,140,160);   // castle-strobe held
      else if(!(state==ST_ADJUST && i==activeBtn)) col=C(70,20,0);
    }
    trellis.pixels.setPixelColor(i,col);
  }
  trellis.pixels.show();
}

void setup(){
  Wire.begin();
  if(!trellis.begin()){ while(1) delay(1000); }
  for(int i=0;i<16;i++){
    trellis.activateKey(i,SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i,SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i,handleKey);
  }
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  if(esp_now_init()!=ESP_OK){ while(1) delay(1000); }
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);
  ensurePeer();
}

void loop(){
  static uint32_t lastPoll=0,lastHB=0;
  static bool prevTap=false, prevPlus=false, prevMinus=false;
  uint32_t t=millis();

  if(t-lastHB>=100){ lastHB=t; sendCmd(0,0,0); }   // heartbeat (carries strobe state)

  if(t-lastPoll<20) return;
  lastPoll=t;
  trellis.read();                                  // fires handleKey

  // ── Hold a param button 2s → activate ADJUST (it starts blinking) ──
  const int pbs[5]={4,8,12,1,5};
  for(int k=0;k<5;k++){ int b=pbs[k];
    if(b==suppressBtn) continue;                 // don't re-arm the button we just tapped off
    if(btnDown[b] && (t-btnDownT[b])>=HOLD_TO_SELECT && !(state==ST_ADJUST && activeBtn==b)) enterAdjust(b);
  }

  // ── Beat matcher: hold 11 for 3s to arm, then tap the beat ──
  if(btnDown[11] && (t-btnDownT[11])>=3000 && state!=ST_TAP){ enterTap(); lastActivity=t; }
  if(state==ST_TAP && btnDown[11] && !prevTap){        // a fresh tap (the arming hold isn't counted)
    if(tapCount<MAX_TAPS) taps[tapCount++]=t;
    else { for(int i=1;i<MAX_TAPS;i++) taps[i-1]=taps[i]; taps[MAX_TAPS-1]=t; }
    lastActivity=t;
    if(tapCount>=2){
      uint32_t sum=0; for(int i=1;i<tapCount;i++) sum+=taps[i]-taps[i-1];
      float avg=(float)sum/(tapCount-1);              // ms per beat
      float sp=(60000.0f/avg)/120.0f;                 // 120 BPM = speed 1.0
      if(sp<0.1f)sp=0.1f; if(sp>4.0f)sp=4.0f;
      sendCmd(3,0,(int16_t)(sp*1000));                // store tempo (persists; final one stays after 3s idle)
    }
  }
  prevTap=btnDown[11];

  // ── +/- ──
  bool plus=btnDown[3], minus=btnDown[7];
  if(state==ST_DEFAULT){
    if(plus  && !prevPlus)  sendCmd(1,0,+1);        // next mode
    if(minus && !prevMinus) sendCmd(1,0,-1);        // prev mode
  } else if(state==ST_ADJUST){
    if(plus && !prevPlus){ plusPressT=t; plusRepT=t; sendCmd(2,activeTarget,+1); lastActivity=t; }
    else if(plus){ uint32_t h=t-plusPressT; if(h>=500){ uint32_t rate=h<2000?80:(h<4000?40:16); if(t-plusRepT>=rate){ plusRepT=t; sendCmd(2,activeTarget,+1); lastActivity=t; } } }
    if(minus && !prevMinus){ minusPressT=t; minusRepT=t; sendCmd(2,activeTarget,-1); lastActivity=t; }
    else if(minus){ uint32_t h=t-minusPressT; if(h>=500){ uint32_t rate=h<2000?80:(h<4000?40:16); if(t-minusRepT>=rate){ minusRepT=t; sendCmd(2,activeTarget,-1); lastActivity=t; } } }
  }
  prevPlus=plus; prevMinus=minus;

  // ADJUST stays until the active param button is tapped again (no timeout).
  // The beat matcher exits (storing the tempo) after 3s of no taps.
  if(state==ST_TAP && (t-lastActivity)>INACTIVITY_TIMEOUT) enterDefault();

  renderLEDs(t);
}
