// ═══════════════════════════════════════════════════════
//  NeoTrellis 4x4 Sender
//  Col0=yellow/bri  Col1=pink/speed  Col2=orange/mode  Col3=special
//   0 AnimBri+   1 AnimSpd+   2 NextMode   3 unused
//   4 AnimBri-   5 AnimSpd-   6 PrevMode   7 unused
//   8 StrobBri+  9 StrobFreq+ 10 StrobSq+  11 Blackout
//  12 StrobBri- 13 StrobFreq- 14 StrobSq-  15 Strobe(hold)
// ═══════════════════════════════════════════════════════

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include "Adafruit_NeoTrellis.h"
#include <Preferences.h>

uint8_t receiverMAC[] = {0xF4, 0x65, 0x0B, 0xE8, 0x98, 0x60};
Adafruit_NeoTrellis trellis;

typedef struct {
  uint16_t heldMask;
  uint8_t  eventBtn;
  bool     eventPressed;
  bool     isEvent;
} GridMsg;

typedef struct {
  uint8_t brightness;
  uint8_t strobeBrightness;
  uint8_t strobeOnMs;
  uint8_t strobeOffMs;
  uint8_t strobeSquares;
  int     animIndex;
  float   animSpeed;
  bool    blackoutActive;
} SyncMsg;

#define TOTAL_ANIMS      55
#define BRI_STEP         5
#define STROBE_BRI_STEP  10
#define STROBE_FREQ_STEP 2
#define STROBE_SQ_STEP   1
#define REPEAT_DELAY     500
#define REPEAT_RATE      80

struct SenderSettings {
  int     animIndex;
  uint8_t brightness;
  uint8_t strobeBrightness;
  uint8_t strobeOnMs;
  uint8_t strobeOffMs;
  uint8_t strobeSquares;
  float   animSpeed;
  bool    blackoutActive;
} ss;

void loadSenderSettings() {
  Preferences p; p.begin("grid",true);
  ss.animIndex        = p.getInt("anim",0);
  ss.brightness       = p.getUChar("bri",40);
  ss.strobeBrightness = p.getUChar("sbri",200);
  ss.strobeOnMs       = p.getUChar("son",40);
  ss.strobeOffMs      = p.getUChar("soff",40);
  ss.strobeSquares    = p.getUChar("ssq",30);
  ss.animSpeed        = p.getFloat("spd",1.0f);
  ss.blackoutActive   = false;
  p.end();
  ss.animIndex        = constrain(ss.animIndex,0,54);
  ss.brightness       = constrain(ss.brightness,1,255);
  ss.strobeBrightness = constrain(ss.strobeBrightness,1,255);
  ss.strobeOnMs       = constrain(ss.strobeOnMs,5,200);
  ss.strobeOffMs      = constrain(ss.strobeOffMs,5,200);
  ss.strobeSquares    = constrain(ss.strobeSquares,1,122);
  ss.animSpeed        = constrain(ss.animSpeed,0.1f,4.0f);
}

// ── Button state ──────────────────────────────────────
uint16_t heldMask         = 0;
bool     btnHeld[16]      = {};
uint32_t btnPressTime[16] = {};
uint32_t btnLastRepeat[16]= {};

// ── Limits ────────────────────────────────────────────
bool atLimit(int btn) {
  switch(btn){
    case 0:  return ss.brightness>=255;
    case 4:  return ss.brightness<=1;
    case 1:  return ss.animSpeed>=4.0f;
    case 5:  return ss.animSpeed<=0.1f;
    case 8:  return ss.strobeBrightness>=255;
    case 12: return ss.strobeBrightness<=1;
    case 9:  return ss.strobeOnMs<=5;
    case 13: return ss.strobeOnMs>=200;
    case 10: return ss.strobeSquares>=122;
    case 14: return ss.strobeSquares<=1;
    default: return false;
  }
}

// ── Colours ───────────────────────────────────────────
uint32_t C(uint8_t r,uint8_t g,uint8_t b){ return trellis.pixels.Color(r,g,b); }

uint32_t idleColour(int btn){
  // At limit: show bright version
  if(atLimit(btn)){
    switch(btn){
      case 0: case 4:   return C(220,220,0);
      case 1: case 5:   return C(220,0,130);
      case 8: case 12:  return C(220,220,0);
      case 9: case 13:  return C(220,0,130);
      case 10: case 14: return C(220,70,0);
      default: break;
    }
  }
  // Normal idle
  switch(btn){
    case 0: case 4: case 8: case 12: return C(25,25,0);   // yellow
    case 1: case 5: case 9: case 13: return C(25,0,15);   // pink
    case 2: case 6: case 10: case 14: return C(25,8,0);   // orange
    case 11: return C(15,0,0);   // dim red
    case 15: return C(80,80,80); // bright white
    default: return C(0,0,0);    // unused off
  }
}

uint32_t pressColour(int btn){
  if(btn==15) return C(255,255,255);
  if(btn==11) return C(255,0,0);
  return C(60,0,0); // dim red press feedback
}

// ── ESP-NOW ───────────────────────────────────────────
bool peerAdded=false;
void ensurePeer(){
  if(peerAdded) return;
  esp_now_peer_info_t peer={};
  memcpy(peer.peer_addr,receiverMAC,6);
  peer.channel=0; peer.encrypt=false;
  if(esp_now_add_peer(&peer)==ESP_OK) peerAdded=true;
}
void onSent(const uint8_t* mac, esp_now_send_status_t s){
  if(s!=ESP_NOW_SEND_SUCCESS){ esp_now_del_peer(receiverMAC); peerAdded=false; }
}
void sendMsg(bool isEvent, uint8_t btn, bool pressed){
  ensurePeer();
  GridMsg msg;
  msg.heldMask=heldMask; msg.eventBtn=btn; msg.eventPressed=pressed; msg.isEvent=isEvent;
  esp_now_send(receiverMAC,(uint8_t*)&msg,sizeof(msg));
}
void onReceive(const uint8_t* mac, const uint8_t* data, int len){
  if(len!=sizeof(SyncMsg)) return;
  SyncMsg sync;
  memcpy(&sync,data,sizeof(sync));
  ss.brightness       = sync.brightness;
  ss.strobeBrightness = sync.strobeBrightness;
  ss.strobeOnMs       = sync.strobeOnMs;
  ss.strobeOffMs      = sync.strobeOffMs;
  ss.strobeSquares    = sync.strobeSquares;
  ss.animIndex        = sync.animIndex;
  ss.animSpeed        = sync.animSpeed;
  ss.blackoutActive   = sync.blackoutActive;
}

// ── Actions ───────────────────────────────────────────
void applyAction(uint8_t btn){
  switch(btn){
    case 0:  ss.brightness=(uint8_t)min((int)ss.brightness+BRI_STEP,255); break;
    case 4:  ss.brightness=(uint8_t)max((int)ss.brightness-BRI_STEP,1);   break;
    case 1:  ss.animSpeed=constrain(ss.animSpeed+0.1f,0.1f,4.0f);         break;
    case 5:  ss.animSpeed=constrain(ss.animSpeed-0.1f,0.1f,4.0f);         break;
    case 2:  ss.animIndex=(ss.animIndex+1)%TOTAL_ANIMS;                   break;
    case 6:  ss.animIndex=(ss.animIndex-1+TOTAL_ANIMS)%TOTAL_ANIMS;       break;
    case 8:  ss.strobeBrightness=(uint8_t)min((int)ss.strobeBrightness+STROBE_BRI_STEP,255); break;
    case 12: ss.strobeBrightness=(uint8_t)max((int)ss.strobeBrightness-STROBE_BRI_STEP,1);   break;
    case 9:  ss.strobeOnMs=(uint8_t)max((int)ss.strobeOnMs-STROBE_FREQ_STEP,5); ss.strobeOffMs=ss.strobeOnMs; break;
    case 13: ss.strobeOnMs=(uint8_t)min((int)ss.strobeOnMs+STROBE_FREQ_STEP,200); ss.strobeOffMs=ss.strobeOnMs; break;
    case 10: ss.strobeSquares=(uint8_t)min((int)ss.strobeSquares+STROBE_SQ_STEP,122); break;
    case 14: ss.strobeSquares=(uint8_t)max((int)ss.strobeSquares-STROBE_SQ_STEP,1);   break;
    case 11: ss.blackoutActive=!ss.blackoutActive;                         break;
  }
}

// ── NeoTrellis callback ───────────────────────────────
TrellisCallback handleKey(keyEvent evt){
  uint8_t btn=evt.bit.NUM;
  bool pressed=(evt.bit.EDGE==SEESAW_KEYPAD_EDGE_RISING);
  btnHeld[btn]=pressed;
  if(pressed){
    heldMask|=(1<<btn);
    btnPressTime[btn]=millis();
    btnLastRepeat[btn]=millis();
    applyAction(btn);
  } else {
    heldMask&=~(1<<btn);
  }
  sendMsg(true,btn,pressed);
  return 0;
}

// ─────────────────────────────────────────────────────
void setup(){
  Wire.begin();
  if(!trellis.begin()){ while(1) delay(1000); } // halt if trellis not found
  for(int i=0;i<16;i++){
    trellis.activateKey(i,SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i,SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i,handleKey);
  }
  loadSenderSettings();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if(esp_now_init()!=ESP_OK){ while(1) delay(1000); }
  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onReceive);
  ensurePeer();
}

void loop(){
  static uint32_t lastHB=0, lastTrellis=0;
  uint32_t t=millis();

  // ── Poll + render all pixels every 20ms ──────────
  if(t-lastTrellis>=20){
    lastTrellis=t;
    trellis.read();  // poll keys; may internally overwrite pixels

    bool blink=((t/167)%2)==0;  // ~6Hz blink

    for(int i=0;i<16;i++){
      uint32_t col;
      if(btnHeld[i]){
        if(atLimit(i))
          col=blink?C(255,0,0):C(25,0,0);  // blink at limit
        else
          col=pressColour(i);
      } else if(i==11 && ss.blackoutActive){
        col=blink?C(180,0,0):C(20,0,0);    // blink blackout
      } else {
        col=idleColour(i);
      }
      trellis.pixels.setPixelColor(i,col);
    }
    trellis.pixels.show();
  }

  // ── Hold-to-repeat with acceleration ─────────────
  for(int i=0;i<16;i++){
    if(!btnHeld[i]||i==15||i==11||i==2||i==6) continue;
    uint32_t held=t-btnPressTime[i];
    if(held<REPEAT_DELAY) continue;
    uint32_t rate=held<2000?REPEAT_RATE:held<4000?REPEAT_RATE/2:REPEAT_RATE/5;
    if(t-btnLastRepeat[i]>=rate){
      btnLastRepeat[i]=t;
      applyAction(i);
      sendMsg(true,i,true);
    }
  }

  // ── Heartbeat every 100ms ─────────────────────────
  if(t-lastHB>=100){ lastHB=t; sendMsg(false,0,false); }
}
