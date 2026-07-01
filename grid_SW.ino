#include <FastLED.h>
#include <math.h>

#define LEDS_ACTIVE    121
#define LEDS_SPACER     15
#define LEDS_PER_STRIP 258
#define NUM_ELEC        6
#define BRIGHTNESS     40
#define MAX_BRIGHTNESS 128   // cap grid max brightness at ~50% (white/yellow etc)
#define ANIM_DURATION  10000

CRGB ledsX[NUM_ELEC][LEDS_PER_STRIP];
CRGB ledsY[NUM_ELEC][LEDS_PER_STRIP];

const float CENTER   = 61.0f;
const float MAX_DIST = 72.0f;
const float X_POS[12] = { -51,-42,-32,-23,-14,-5, 5,14,23,32,42,51 };
const float Y_POS[12] = { -51,-42,-32,-23,-14,-5, 5,14,23,32,42,51 };

void setLED(bool isX, int strip, int led, CRGB colour) {
  int elec=(strip-1)/2; bool isOdd=(strip%2==1); bool forward; int bufIdx;
  if(isX)forward=!isOdd; else forward=isOdd;
  if(isOdd)bufIdx=forward?(led-1):(121-led);
  else bufIdx=forward?(135+led):(258-led);
  if(isX)ledsX[elec][bufIdx]=colour; else ledsY[elec][bufIdx]=colour;
}

void clearAll() {
  FastLED.clear(true);delay(10);FastLED.clear(true);
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
  FastLED.show();
}

// ══════════════════════════════════════════════
// ORIGINAL ANIMATIONS (minus 4, 6, 15)
// ══════════════════════════════════════════════

// 1. HEARTBEAT PULSE
void anim_heartbeat(uint32_t t) {
  uint32_t cycle=t%1000; float speed=0.055f;
  float p1=(cycle<600)?cycle*speed:0.0f;
  float p2=(cycle>200&&cycle<800)?(cycle-200)*speed:0.0f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y)/MAX_DIST;float bri=0.0f;float d1=fabsf(dist-p1);if(d1<0.06f)bri+=(1.0f-d1/0.06f)*(1.0f-p1);float d2=fabsf(dist-p2);if(d2<0.06f)bri+=(1.0f-d2/0.06f)*(1.0f-p2)*0.7f;bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*255.0f);uint8_t sat=(uint8_t)(255.0f-bri*180.0f);setLED(true,xi+1,n,CRGB(CHSV(0,sat,b)));}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y)/MAX_DIST;float bri=0.0f;float d1=fabsf(dist-p1);if(d1<0.06f)bri+=(1.0f-d1/0.06f)*(1.0f-p1);float d2=fabsf(dist-p2);if(d2<0.06f)bri+=(1.0f-d2/0.06f)*(1.0f-p2)*0.7f;bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*255.0f);uint8_t sat=(uint8_t)(255.0f-bri*180.0f);setLED(false,yi+1,n,CRGB(CHSV(0,sat,b)));}}
  FastLED.show();
}

// 2. BINARY RAIN
static float rainPos[12];static float rainSpeed[12];static uint8_t rainLen[12];static bool rainInit=false;
void anim_binaryRain(uint32_t t,float dt) {
  if(!rainInit){for(int i=0;i<12;i++){rainPos[i]=random(121);rainSpeed[i]=0.03f+random(70)*0.001f;rainLen[i]=15+random(30);}rainInit=true;}
  for(int xi=0;xi<12;xi++){rainPos[xi]+=rainSpeed[xi]*dt;if(rainPos[xi]>121.0f+rainLen[xi])rainPos[xi]=-rainLen[xi];for(int n=1;n<=121;n++){float trail=rainPos[xi]-n;uint8_t b=0;if(trail>0&&trail<rainLen[xi]){float fade=1.0f-(trail/rainLen[xi]);b=(trail<2.0f)?255:(uint8_t)(fade*fade*180.0f);if(random(20)==0)b=min((int)b+60,255);}uint8_t hue=96+(n%3)*5;setLED(true,xi+1,n,b>0?CRGB(CHSV(hue,220,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){for(int n=1;n<=121;n++){uint8_t b=(random(100)<3)?random(30):0;setLED(false,yi+1,n,b>0?CRGB(CHSV(96,220,b)):CRGB::Black);}}
  FastLED.show();
}

// 3. TECTONIC PLATES
void anim_tectonic(uint32_t t) {
  float fX=CENTER+sinf(t*0.0003f)*20.0f;float fY=CENTER+cosf(t*0.0002f)*20.0f;float fw=4.0f+sinf(t*0.0007f)*2.0f;
  for(int xi=0;xi<12;xi++){float sp=X_POS[xi]+CENTER;for(int n=1;n<=121;n++){float dX=fabsf((float)n-fY);float dY=fabsf(sp-fX);float fb=0.0f;if(dX<fw)fb=max(fb,1.0f-dX/fw);if(dY<fw)fb=max(fb,1.0f-dY/fw);float rumble=sinf(t*0.001f+xi*0.7f+n*0.05f)*0.5f+0.5f;uint8_t pb=(uint8_t)(rumble*25.0f);if(fb>0.01f){uint8_t b=(uint8_t)(fb*255.0f);uint8_t sat=(uint8_t)(200.0f-fb*200.0f);setLED(true,xi+1,n,CRGB(CHSV(20,sat,b)));}else setLED(true,xi+1,n,CRGB(pb,pb,pb));}}
  for(int yi=0;yi<12;yi++){float sp=Y_POS[yi]+CENTER;for(int n=1;n<=121;n++){float dX=fabsf((float)n-fX);float dY=fabsf(sp-fY);float fb=0.0f;if(dX<fw)fb=max(fb,1.0f-dX/fw);if(dY<fw)fb=max(fb,1.0f-dY/fw);float rumble=sinf(t*0.001f+yi*0.9f+n*0.05f)*0.5f+0.5f;uint8_t pb=(uint8_t)(rumble*25.0f);if(fb>0.01f){uint8_t b=(uint8_t)(fb*255.0f);uint8_t sat=(uint8_t)(200.0f-fb*200.0f);setLED(false,yi+1,n,CRGB(CHSV(20,sat,b)));}else setLED(false,yi+1,n,CRGB(pb,pb,pb));}}
  FastLED.show();
}

// 4. VORTEX SPIRAL (the fan favourite)
void anim_vortex(uint32_t t) {
  float tf=t*0.001f;float tight=0.15f+sinf(tf*0.3f)*0.08f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float sp=fmod(angle*3.0f/(2.0f*M_PI)-dist*tight-tf*0.5f,1.0f);if(sp<0.0f)sp+=1.0f;float bri=powf(sinf(sp*M_PI),2.5f)*sinf(constrain(dist/MAX_DIST,0.0f,1.0f)*M_PI);uint8_t b=(uint8_t)(bri*220.0f);uint8_t hue=(uint8_t)((int)(180.0f+dist*0.8f+tf*20.0f)%255);setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,220,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float sp=fmod(angle*3.0f/(2.0f*M_PI)-dist*tight-tf*0.5f,1.0f);if(sp<0.0f)sp+=1.0f;float bri=powf(sinf(sp*M_PI),2.5f)*sinf(constrain(dist/MAX_DIST,0.0f,1.0f)*M_PI);uint8_t b=(uint8_t)(bri*220.0f);uint8_t hue=(uint8_t)((int)(180.0f+dist*0.8f+tf*20.0f)%255);setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,220,b)):CRGB::Black);}}
  FastLED.show();
}

// 5. EARTHQUAKE
static float epiX=0.0f,epiY=0.0f;static uint32_t strikeTime=0;
void anim_earthquake(uint32_t t) {
  if(t-strikeTime>3000){epiX=(random(80)-40);epiY=(random(80)-40);strikeTime=t;}
  float elapsed=(t-strikeTime)*0.001f;float wp=elapsed*30.0f;float decay=expf(-elapsed*0.8f);
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf((x-epiX)*(x-epiX)+(y-epiY)*(y-epiY));float w=max(sinf((dist-wp)*0.3f)*decay,0.0f)+max(sinf((dist-wp*0.6f)*0.2f)*decay*0.4f,0.0f);float total=constrain(w,0.0f,1.0f);uint8_t b=(uint8_t)(total*230.0f);uint8_t sat=(uint8_t)(200.0f-total*100.0f);setLED(true,xi+1,n,b>3?CRGB(CHSV(25,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf((x-epiX)*(x-epiX)+(y-epiY)*(y-epiY));float w=max(sinf((dist-wp)*0.3f)*decay,0.0f)+max(sinf((dist-wp*0.6f)*0.2f)*decay*0.4f,0.0f);float total=constrain(w,0.0f,1.0f);uint8_t b=(uint8_t)(total*230.0f);uint8_t sat=(uint8_t)(200.0f-total*100.0f);setLED(false,yi+1,n,b>3?CRGB(CHSV(25,sat,b)):CRGB::Black);}}
  FastLED.show();
}

// 6. DNA HELIX
void anim_dnaHelix(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float w=8.0f+sinf(tf*0.7f)*3.0f;float s1y=sinf(x*0.12f-tf*2.0f)*40.0f;float s2y=sinf(x*0.12f+tf*2.0f+M_PI)*40.0f;float b1=fabsf(y-s1y)<w?powf(1.0f-fabsf(y-s1y)/w,2.0f):0.0f;float b2=fabsf(y-s2y)<w?powf(1.0f-fabsf(y-s2y)/w,2.0f):0.0f;CRGB c=CRGB::Black;if(b1>0){CRGB s=CRGB(CHSV(130,240,(uint8_t)(b1*220.0f)));c.r=qadd8(c.r,s.r);c.g=qadd8(c.g,s.g);c.b=qadd8(c.b,s.b);}if(b2>0){CRGB s=CRGB(CHSV(300,240,(uint8_t)(b2*220.0f)));c.r=qadd8(c.r,s.r);c.g=qadd8(c.g,s.g);c.b=qadd8(c.b,s.b);}setLED(true,xi+1,n,c);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float w=8.0f+sinf(tf*0.7f)*3.0f;float s1x=sinf(y*0.12f-tf*2.0f)*40.0f;float s2x=sinf(y*0.12f+tf*2.0f+M_PI)*40.0f;float b1=fabsf(x-s1x)<w?powf(1.0f-fabsf(x-s1x)/w,2.0f):0.0f;float b2=fabsf(x-s2x)<w?powf(1.0f-fabsf(x-s2x)/w,2.0f):0.0f;CRGB c=CRGB::Black;if(b1>0){CRGB s=CRGB(CHSV(130,240,(uint8_t)(b1*220.0f)));c.r=qadd8(c.r,s.r);c.g=qadd8(c.g,s.g);c.b=qadd8(c.b,s.b);}if(b2>0){CRGB s=CRGB(CHSV(300,240,(uint8_t)(b2*220.0f)));c.r=qadd8(c.r,s.r);c.g=qadd8(c.g,s.g);c.b=qadd8(c.b,s.b);}setLED(false,yi+1,n,c);}}
  FastLED.show();
}

// 7. SUPERNOVA
static uint32_t novaStrike=0;static bool novaInit=false;
static float sparkAngle[20];static float sparkSpeed[20];
void anim_supernova(uint32_t at) {
  if(!novaInit){novaStrike=0;for(int i=0;i<20;i++){sparkAngle[i]=random(360)*M_PI/180.0f;sparkSpeed[i]=0.02f+random(40)*0.001f;}novaInit=true;}
  uint32_t t=at; if(t-novaStrike>4000){novaStrike=t;for(int i=0;i<20;i++){sparkAngle[i]=random(360)*M_PI/180.0f;sparkSpeed[i]=0.02f+random(40)*0.001f;}}
  float elapsed=(t-novaStrike)*0.001f;float shockwave=elapsed*55.0f;float coreBri=max(0.0f,1.0f-elapsed*0.8f);
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float sw=fabsf(dist-shockwave);float swBri=(sw<5.0f)?(1.0f-sw/5.0f)*max(0.0f,1.0f-elapsed*0.3f):0.0f;float cBri=coreBri*expf(-dist*0.04f);float spBri=0.0f;float angle=atan2f(y,x);for(int i=0;i<20;i++){float sd=fabsf(dist-sparkSpeed[i]*elapsed*MAX_DIST);float sa=fabsf(angle-sparkAngle[i]);if(sa>M_PI)sa=2.0f*M_PI-sa;if(sd<3.0f&&sa<0.15f)spBri=max(spBri,(1.0f-sd/3.0f)*max(0.0f,1.0f-elapsed*0.4f));}float total=constrain(swBri+cBri+spBri,0.0f,1.0f);uint8_t b=(uint8_t)(total*255.0f);uint8_t hue=(uint8_t)(swBri>cBri?20:0);uint8_t sat=(uint8_t)(200.0f-cBri*200.0f);setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float sw=fabsf(dist-shockwave);float swBri=(sw<5.0f)?(1.0f-sw/5.0f)*max(0.0f,1.0f-elapsed*0.3f):0.0f;float cBri=coreBri*expf(-dist*0.04f);float spBri=0.0f;float angle=atan2f(y,x);for(int i=0;i<20;i++){float sd=fabsf(dist-sparkSpeed[i]*elapsed*MAX_DIST);float sa=fabsf(angle-sparkAngle[i]);if(sa>M_PI)sa=2.0f*M_PI-sa;if(sd<3.0f&&sa<0.15f)spBri=max(spBri,(1.0f-sd/3.0f)*max(0.0f,1.0f-elapsed*0.4f));}float total=constrain(swBri+cBri+spBri,0.0f,1.0f);uint8_t b=(uint8_t)(total*255.0f);uint8_t hue=(uint8_t)(swBri>cBri?20:0);uint8_t sat=(uint8_t)(200.0f-cBri*200.0f);setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  FastLED.show();
}

// 8. ACID TRIP (dimmed 35%)
void anim_acidTrip(uint32_t t) {
  float tf=t*0.001f;float cx[5],cy[5];
  for(int i=0;i<5;i++){cx[i]=sinf(tf*0.3f+i*1.3f)*45.0f;cy[i]=cosf(tf*0.4f+i*0.9f)*45.0f;}
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float v=0.0f;for(int i=0;i<5;i++){float d=sqrtf((x-cx[i])*(x-cx[i])+(y-cy[i])*(y-cy[i]));v+=sinf(d*0.15f-tf*(1.0f+i*0.3f));}v=(v+5.0f)/10.0f;uint8_t hue=(uint8_t)(v*255.0f+(uint8_t)(tf*50.0f));uint8_t bri=(uint8_t)((180.0f+sinf(v*M_PI*2.0f)*75.0f)*0.65f);setLED(true,xi+1,n,CRGB(CHSV(hue,240,bri)));}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float v=0.0f;for(int i=0;i<5;i++){float d=sqrtf((x-cx[i])*(x-cx[i])+(y-cy[i])*(y-cy[i]));v+=sinf(d*0.15f-tf*(1.0f+i*0.3f));}v=(v+5.0f)/10.0f;uint8_t hue=(uint8_t)(v*255.0f+(uint8_t)(tf*50.0f));uint8_t bri=(uint8_t)((180.0f+sinf(v*M_PI*2.0f)*75.0f)*0.65f);setLED(false,yi+1,n,CRGB(CHSV(hue,240,bri)));}}
  FastLED.show();
}

// 9. BLACK HOLE
void anim_blackHole(uint32_t t) {
  float tf=t*0.001f;float eh=8.0f+sinf(tf*0.5f)*2.0f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);if(dist<eh){setLED(true,xi+1,n,CRGB::Black);continue;}float diskBri=expf(-(dist-eh)*0.08f);float diskSpin=fmod(angle-tf*2.0f+dist*0.05f,2.0f*M_PI)/(2.0f*M_PI);float diskPat=powf(fabsf(sinf(diskSpin*M_PI*6.0f)),3.0f);float jetBri=0.0f;float absA=fabsf(fabsf(angle)-M_PI/2.0f);if(absA<0.15f)jetBri=(1.0f-absA/0.15f)*min(1.0f,dist/30.0f)*max(0.0f,1.0f-dist/MAX_DIST)*1.5f;float total=constrain(diskBri*diskPat*0.8f+jetBri,0.0f,1.0f);uint8_t b=(uint8_t)(total*240.0f);uint8_t hue=jetBri>0.1f?150:(uint8_t)(20.0f+diskBri*30.0f);uint8_t sat=jetBri>0.1f?200:(uint8_t)(255.0f-diskBri*150.0f);setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);if(dist<eh){setLED(false,yi+1,n,CRGB::Black);continue;}float diskBri=expf(-(dist-eh)*0.08f);float diskSpin=fmod(angle-tf*2.0f+dist*0.05f,2.0f*M_PI)/(2.0f*M_PI);float diskPat=powf(fabsf(sinf(diskSpin*M_PI*6.0f)),3.0f);float jetBri=0.0f;float absA=fabsf(fabsf(angle)-M_PI/2.0f);if(absA<0.15f)jetBri=(1.0f-absA/0.15f)*min(1.0f,dist/30.0f)*max(0.0f,1.0f-dist/MAX_DIST)*1.5f;float total=constrain(diskBri*diskPat*0.8f+jetBri,0.0f,1.0f);uint8_t b=(uint8_t)(total*240.0f);uint8_t hue=jetBri>0.1f?150:(uint8_t)(20.0f+diskBri*30.0f);uint8_t sat=jetBri>0.1f?200:(uint8_t)(255.0f-diskBri*150.0f);setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  FastLED.show();
}

// 10. WORMHOLE
void anim_wormhole(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y)/MAX_DIST;float lD=logf(1.0f+dist*9.0f)/logf(10.0f);float ring=fmod(lD*8.0f-tf*2.0f,1.0f);if(ring<0.0f)ring+=1.0f;float angle=atan2f(y,x);float twist=sinf(angle*3.0f+dist*5.0f-tf*1.5f)*0.3f+0.7f;float bri=powf(sinf(ring*M_PI),3.0f)*twist*(1.0f-dist*0.3f);uint8_t b=(uint8_t)(bri*230.0f);uint8_t hue=(uint8_t)(190.0f+dist*40.0f+ring*30.0f);setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,240,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y)/MAX_DIST;float lD=logf(1.0f+dist*9.0f)/logf(10.0f);float ring=fmod(lD*8.0f-tf*2.0f,1.0f);if(ring<0.0f)ring+=1.0f;float angle=atan2f(y,x);float twist=sinf(angle*3.0f+dist*5.0f-tf*1.5f)*0.3f+0.7f;float bri=powf(sinf(ring*M_PI),3.0f)*twist*(1.0f-dist*0.3f);uint8_t b=(uint8_t)(bri*230.0f);uint8_t hue=(uint8_t)(190.0f+dist*40.0f+ring*30.0f);setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,240,b)):CRGB::Black);}}
  FastLED.show();
}

// 11. CELLULAR MITOSIS
void anim_mitosis(uint32_t t) {
  float tf=t*0.001f;float split=sinf(tf*0.6f)*35.0f;float bx1=split,by1=cosf(tf*0.4f)*20.0f;float bx2=-split,by2=-cosf(tf*0.4f)*20.0f;float br1=15.0f+sinf(tf*1.1f)*5.0f;float br2=15.0f+sinf(tf*0.9f+1.0f)*5.0f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float d1=sqrtf((x-bx1)*(x-bx1)+(y-by1)*(y-by1));float d2=sqrtf((x-bx2)*(x-bx2)+(y-by2)*(y-by2));float v1=max(0.0f,1.0f-d1/br1);float v2=max(0.0f,1.0f-d2/br2);float total=constrain(v1+v2,0.0f,1.0f);float membrane=max(powf(v1*(1.0f-v1)*4.0f,2.0f),powf(v2*(1.0f-v2)*4.0f,2.0f));float bri=total*0.4f+membrane*0.8f;uint8_t b=(uint8_t)(constrain(bri,0.0f,1.0f)*230.0f);uint8_t hue=(uint8_t)(85.0f+total*30.0f);uint8_t sat=(uint8_t)(200.0f-membrane*100.0f);setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float d1=sqrtf((x-bx1)*(x-bx1)+(y-by1)*(y-by1));float d2=sqrtf((x-bx2)*(x-bx2)+(y-by2)*(y-by2));float v1=max(0.0f,1.0f-d1/br1);float v2=max(0.0f,1.0f-d2/br2);float total=constrain(v1+v2,0.0f,1.0f);float membrane=max(powf(v1*(1.0f-v1)*4.0f,2.0f),powf(v2*(1.0f-v2)*4.0f,2.0f));float bri=total*0.4f+membrane*0.8f;uint8_t b=(uint8_t)(constrain(bri,0.0f,1.0f)*230.0f);uint8_t hue=(uint8_t)(85.0f+total*30.0f);uint8_t sat=(uint8_t)(200.0f-membrane*100.0f);setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  FastLED.show();
}

// 12. SOLAR FLARES
void anim_solarFlares(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float bri=0.0f;for(int f=0;f<4;f++){float fA=tf*0.7f+f*M_PI/2.0f;float aP=fmod(tf*0.5f+f*0.25f,1.0f);float aD=sinf(aP*M_PI)*60.0f;float aAng=fA+sinf(aP*M_PI)*0.8f;float ax=cosf(aAng)*aD;float ay=sinf(aAng)*aD;float d=sqrtf((x-ax)*(x-ax)+(y-ay)*(y-ay));bri+=max(0.0f,(1.0f-d/12.0f)*(1.0f-aP*0.5f));}bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*240.0f);uint8_t hue=(uint8_t)(15.0f+bri*25.0f);uint8_t sat=(uint8_t)(255.0f-bri*100.0f);setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float bri=0.0f;for(int f=0;f<4;f++){float fA=tf*0.7f+f*M_PI/2.0f;float aP=fmod(tf*0.5f+f*0.25f,1.0f);float aD=sinf(aP*M_PI)*60.0f;float aAng=fA+sinf(aP*M_PI)*0.8f;float ax=cosf(aAng)*aD;float ay=sinf(aAng)*aD;float d=sqrtf((x-ax)*(x-ax)+(y-ay)*(y-ay));bri+=max(0.0f,(1.0f-d/12.0f)*(1.0f-aP*0.5f));}bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*240.0f);uint8_t hue=(uint8_t)(15.0f+bri*25.0f);uint8_t sat=(uint8_t)(255.0f-bri*100.0f);setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  FastLED.show();
}

// 13. PARTICLE COLLIDER
static uint32_t collideStrike=0;
void anim_collider(uint32_t t) {
  if(t-collideStrike>3500)collideStrike=t;
  float phase=(t-collideStrike)*0.001f/3.5f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float bri=0.0f;if(phase<0.7f){float inward=phase/0.7f;float bD=MAX_DIST*(1.0f-inward);float bW=4.0f;float d1=fabsf(dist-bD);float a1=fmod(fabsf(angle-inward*M_PI*3.0f),M_PI*2.0f);if(d1<bW&&a1<0.3f)bri+=(1.0f-d1/bW)*(1.0f-a1/0.3f);float a2=fmod(fabsf(angle+M_PI-inward*M_PI*3.0f),M_PI*2.0f);if(d1<bW&&a2<0.3f)bri+=(1.0f-d1/bW)*(1.0f-a2/0.3f);}else{float burst=(phase-0.7f)/0.3f;float sR=burst*MAX_DIST*1.5f;float sw=fabsf(dist-sR);bri=max(0.0f,(1.0f-sw/8.0f)*(1.0f-burst));bri+=max(0.0f,(1.0f-dist/15.0f)*(1.0f-burst*2.0f));}bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*255.0f);uint8_t hue=phase<0.7f?(uint8_t)(130.0f+phase*100.0f):(uint8_t)(30.0f+(phase-0.7f)*200.0f);uint8_t sat=(uint8_t)(255.0f-bri*100.0f);setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float bri=0.0f;if(phase<0.7f){float inward=phase/0.7f;float bD=MAX_DIST*(1.0f-inward);float bW=4.0f;float d1=fabsf(dist-bD);float a1=fmod(fabsf(angle-inward*M_PI*3.0f),M_PI*2.0f);if(d1<bW&&a1<0.3f)bri+=(1.0f-d1/bW)*(1.0f-a1/0.3f);float a2=fmod(fabsf(angle+M_PI-inward*M_PI*3.0f),M_PI*2.0f);if(d1<bW&&a2<0.3f)bri+=(1.0f-d1/bW)*(1.0f-a2/0.3f);}else{float burst=(phase-0.7f)/0.3f;float sR=burst*MAX_DIST*1.5f;float sw=fabsf(dist-sR);bri=max(0.0f,(1.0f-sw/8.0f)*(1.0f-burst));bri+=max(0.0f,(1.0f-dist/15.0f)*(1.0f-burst*2.0f));}bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*255.0f);uint8_t hue=phase<0.7f?(uint8_t)(130.0f+phase*100.0f):(uint8_t)(30.0f+(phase-0.7f)*200.0f);uint8_t sat=(uint8_t)(255.0f-bri*100.0f);setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  FastLED.show();
}

// 14. BREATHING MANDALA (dimmed 35%)
void anim_mandala(uint32_t t) {
  float tf=t*0.001f;
  // breath: keep it lower, never fully peaking — reduces the "too bright too long" issue
  float breath=0.45f+sinf(tf*0.8f)*0.25f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=dist/MAX_DIST;float sA=fmod(fabsf(fmod(angle,M_PI/4.0f)-(M_PI/8.0f)),M_PI/4.0f);float petal=sinf(sA*8.0f+tf*0.5f)*sinf(nD*M_PI*4.0f-tf*1.2f);float ring=sinf(nD*M_PI*6.0f-tf*0.8f);float radial=sinf(angle*6.0f+tf*0.3f);float v=(petal+ring+radial+3.0f)/6.0f*breath;v=constrain(v,0.0f,1.0f);uint8_t hue=(uint8_t)(v*180.0f+tf*15.0f+nD*60.0f);uint8_t bri=(uint8_t)(v*v*180.0f);setLED(true,xi+1,n,bri>3?CRGB(CHSV(hue,230,bri)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=dist/MAX_DIST;float sA=fmod(fabsf(fmod(angle,M_PI/4.0f)-(M_PI/8.0f)),M_PI/4.0f);float petal=sinf(sA*8.0f+tf*0.5f)*sinf(nD*M_PI*4.0f-tf*1.2f);float ring=sinf(nD*M_PI*6.0f-tf*0.8f);float radial=sinf(angle*6.0f+tf*0.3f);float v=(petal+ring+radial+3.0f)/6.0f*breath;v=constrain(v,0.0f,1.0f);uint8_t hue=(uint8_t)(v*180.0f+tf*15.0f+nD*60.0f);uint8_t bri=(uint8_t)(v*v*180.0f);setLED(false,yi+1,n,bri>3?CRGB(CHSV(hue,230,bri)):CRGB::Black);}}
  FastLED.show();
}

// 15. SHOCKWAVE CHAIN (dimmed 35%)
#define NUM_SHOCKS 4
static float shockX[NUM_SHOCKS],shockY[NUM_SHOCKS];static uint32_t shockT[NUM_SHOCKS];static bool shockInit=false;
void anim_shockwaveChain(uint32_t t) {
  if(!shockInit){for(int i=0;i<NUM_SHOCKS;i++){shockX[i]=(random(80)-40);shockY[i]=(random(80)-40);shockT[i]=t-random(2000);}shockInit=true;}
  for(int i=0;i<NUM_SHOCKS;i++){if(t-shockT[i]>2000){shockX[i]=(random(80)-40);shockY[i]=(random(80)-40);shockT[i]=t;}}
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float total=0.0f;uint8_t bestHue=200;
    for(int i=0;i<NUM_SHOCKS;i++){
      float el=(t-shockT[i])*0.001f;
      float d=sqrtf((x-shockX[i])*(x-shockX[i])+(y-shockY[i])*(y-shockY[i]));
      float waveFront=d-el*45.0f;
      // Fast Gaussian ring instead of expf — just a quadratic falloff
      float ring=max(0.0f,1.0f-fabsf(waveFront)/8.0f);
      float decay=max(0.0f,1.0f-el*0.5f);
      total+=ring*ring*decay;
      if(ring>0.1f) bestHue=(uint8_t)(200.0f+i*20.0f);
    }
    total=constrain(total,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*210.0f);
    setLED(true,xi+1,n,b>4?CRGB(CHSV(bestHue,220,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float total=0.0f;uint8_t bestHue=200;
    for(int i=0;i<NUM_SHOCKS;i++){
      float el=(t-shockT[i])*0.001f;float d=sqrtf((x-shockX[i])*(x-shockX[i])+(y-shockY[i])*(y-shockY[i]));
      float ring=max(0.0f,1.0f-fabsf(d-el*45.0f)/8.0f);float decay=max(0.0f,1.0f-el*0.5f);
      total+=ring*ring*decay;if(ring>0.1f)bestHue=(uint8_t)(200.0f+i*20.0f);
    }
    total=constrain(total,0.0f,1.0f);uint8_t b=(uint8_t)(total*210.0f);
    setLED(false,yi+1,n,b>4?CRGB(CHSV(bestHue,220,b)):CRGB::Black);
  }}
  FastLED.show();
}

// ══════════════════════════════════════════════
// 10 NEW BLUE SPIRAL ANIMATIONS
// All blue palette (hue ~140-170), vortex vibe, very different from each other
// ══════════════════════════════════════════════

// Shared blue hue range: 140=cyan-blue, 160=pure blue, 170=deep blue

// 16. DUAL COUNTER-VORTEX
// Two spiral sets rotating in opposite directions, interfering where they cross.
void anim_dualVortex(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    // Spiral A: 3 arms clockwise
    float spA=fmod(angle*3.0f/(2.0f*M_PI)-dist*0.12f-tf*0.7f,1.0f);if(spA<0.0f)spA+=1.0f;
    float briA=powf(sinf(spA*M_PI),3.0f)*sinf(nD*M_PI);
    // Spiral B: 2 arms counter-clockwise, different tightness
    float spB=fmod(angle*2.0f/(2.0f*M_PI)+dist*0.09f+tf*0.5f,1.0f);if(spB<0.0f)spB+=1.0f;
    float briB=powf(sinf(spB*M_PI),3.0f)*sinf(nD*M_PI);
    // Interference: where both bright = flash white-blue
    float interference=briA*briB*2.0f;
    float total=constrain(briA*0.6f+briB*0.6f+interference,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*220.0f);
    uint8_t hue=interference>0.3f?145:(uint8_t)(160.0f+briA*10.0f-briB*10.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,220,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float spA=fmod(angle*3.0f/(2.0f*M_PI)-dist*0.12f-tf*0.7f,1.0f);if(spA<0.0f)spA+=1.0f;float briA=powf(sinf(spA*M_PI),3.0f)*sinf(nD*M_PI);
    float spB=fmod(angle*2.0f/(2.0f*M_PI)+dist*0.09f+tf*0.5f,1.0f);if(spB<0.0f)spB+=1.0f;float briB=powf(sinf(spB*M_PI),3.0f)*sinf(nD*M_PI);
    float interference=briA*briB*2.0f;float total=constrain(briA*0.6f+briB*0.6f+interference,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*220.0f);uint8_t hue=interference>0.3f?145:(uint8_t)(160.0f+briA*10.0f-briB*10.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,220,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 17. COLLAPSING FUNNEL
// Spiral tightens inward to a point then explodes outward, looping.
void anim_collapsingFunnel(uint32_t t) {
  float tf=t*0.001f;
  float cycle=fmod(tf*0.4f,1.0f); // 0..1 one full collapse/expand per 2.5s
  float tight=0.05f+cycle*0.4f;   // tightness grows during collapse
  float direction=(cycle<0.7f)?-1.0f:1.0f; // inward then snap outward
  float brightness=cycle<0.7f?1.0f:max(0.0f,1.0f-(cycle-0.7f)/0.3f)*1.5f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float sp=fmod(angle*4.0f/(2.0f*M_PI)+direction*dist*tight-tf*1.2f,1.0f);if(sp<0.0f)sp+=1.0f;
    float bri=powf(sinf(sp*M_PI),2.0f)*sinf(nD*M_PI)*brightness;
    bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*230.0f);
    uint8_t hue=(uint8_t)(155.0f+cycle*20.0f); uint8_t sat=(uint8_t)(240.0f-cycle*80.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float sp=fmod(angle*4.0f/(2.0f*M_PI)+direction*dist*tight-tf*1.2f,1.0f);if(sp<0.0f)sp+=1.0f;
    float bri=powf(sinf(sp*M_PI),2.0f)*sinf(nD*M_PI)*brightness;bri=constrain(bri,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*230.0f);uint8_t hue=(uint8_t)(155.0f+cycle*20.0f);uint8_t sat=(uint8_t)(240.0f-cycle*80.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 18. PULSAR
// Single ultra-tight fast arm with very long decay trail. Like a lighthouse beam.
void anim_pulsar(uint32_t t) {
  float tf=t*0.001f;float sweep=fmod(tf*3.0f,1.0f)*2.0f*M_PI; // fast single arm
  float trailLen=200.0f; // degrees of trail
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);if(angle<0.0f)angle+=2.0f*M_PI;
    float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float diff=sweep-angle;if(diff<0.0f)diff+=2.0f*M_PI;
    float trailDeg=diff*180.0f/M_PI;
    if(trailDeg>trailLen){setLED(true,xi+1,n,CRGB::Black);continue;}
    float trailFade=1.0f-(trailDeg/trailLen);trailFade=trailFade*trailFade;
    float distFade=sinf(nD*M_PI);
    float bri=trailFade*distFade;
    uint8_t b=(uint8_t)(bri*240.0f);
    uint8_t hue=(uint8_t)(150.0f+trailDeg*0.05f); // cyan at tip, deeper blue in trail
    uint8_t sat=(uint8_t)(180.0f+trailFade*75.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);if(angle<0.0f)angle+=2.0f*M_PI;
    float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float diff=sweep-angle;if(diff<0.0f)diff+=2.0f*M_PI;float trailDeg=diff*180.0f/M_PI;
    if(trailDeg>trailLen){setLED(false,yi+1,n,CRGB::Black);continue;}
    float trailFade=1.0f-(trailDeg/trailLen);trailFade=trailFade*trailFade;
    float bri=trailFade*sinf(nD*M_PI);uint8_t b=(uint8_t)(bri*240.0f);
    uint8_t hue=(uint8_t)(150.0f+trailDeg*0.05f);uint8_t sat=(uint8_t)(180.0f+trailFade*75.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 19. LIQUID METAL
// Slow viscous spiral. Arms thick and flowing like mercury. Very smooth.
void anim_liquidMetal(uint32_t t) {
  float tf=t*0.001f;
  // Hue drifts slowly through warm metals: gold→copper→silver→chrome
  float hueBase=tf*4.0f; // slow hue cycle
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float sp=fmod(angle*2.0f/(2.0f*M_PI)-dist*0.06f-tf*0.15f,1.0f);if(sp<0.0f)sp+=1.0f;
    float bri=powf(0.5f+0.5f*sinf(sp*2.0f*M_PI),6.0f);
    float radBand=0.5f+0.5f*sinf(nD*M_PI*3.0f-tf*0.3f);
    bri*=radBand*sinf(nD*M_PI);
    uint8_t b=(uint8_t)(bri*230.0f);
    // Colour: cycles through gold(25), copper(10), rose(245), silver(160)
    uint8_t hue=(uint8_t)(hueBase+nD*30.0f+sp*20.0f);
    uint8_t sat=(uint8_t)(140.0f+bri*100.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float sp=fmod(angle*2.0f/(2.0f*M_PI)-dist*0.06f-tf*0.15f,1.0f);if(sp<0.0f)sp+=1.0f;
    float bri=powf(0.5f+0.5f*sinf(sp*2.0f*M_PI),6.0f)*(0.5f+0.5f*sinf(nD*M_PI*3.0f-tf*0.3f))*sinf(nD*M_PI);
    uint8_t b=(uint8_t)(bri*230.0f);uint8_t hue=(uint8_t)(hueBase+nD*30.0f+sp*20.0f);uint8_t sat=(uint8_t)(140.0f+bri*100.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 20. QUANTUM FOAM
// Many thin rapid arms flickering in and out. Chaotic blue static field.
void anim_quantumFoam(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float bri=0.0f;
    // 9 thin arms at wildly different speeds
    for(int a=0;a<9;a++){
      float arms=(float)(a+2);float speed=0.3f+a*0.4f;float tightness=0.05f+a*0.03f;
      float sp=fmod(angle*arms/(2.0f*M_PI)-dist*tightness-tf*speed*(a%2?1.0f:-0.7f),1.0f);if(sp<0.0f)sp+=1.0f;
      float armBri=powf(sinf(sp*M_PI),8.0f); // very sharp thin arms
      // Flicker: each arm independently flickers
      float flicker=0.5f+0.5f*sinf(tf*(5.0f+a*3.7f)+a*1.3f);
      bri+=armBri*flicker*(1.0f/(a+2));
    }
    bri=constrain(bri*sinf(nD*M_PI),0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*210.0f);
    uint8_t hue=(uint8_t)(148.0f+sinf(angle+tf)*8.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,230,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float bri=0.0f;
    for(int a=0;a<9;a++){float arms=(float)(a+2);float speed=0.3f+a*0.4f;float tightness=0.05f+a*0.03f;float sp=fmod(angle*arms/(2.0f*M_PI)-dist*tightness-tf*speed*(a%2?1.0f:-0.7f),1.0f);if(sp<0.0f)sp+=1.0f;float flicker=0.5f+0.5f*sinf(tf*(5.0f+a*3.7f)+a*1.3f);bri+=powf(sinf(sp*M_PI),8.0f)*flicker*(1.0f/(a+2));}
    bri=constrain(bri*sinf(nD*M_PI),0.0f,1.0f);uint8_t b=(uint8_t)(bri*210.0f);uint8_t hue=(uint8_t)(148.0f+sinf(angle+tf)*8.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,230,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 21. ICE CROWN
// Sharp radial spikes of varying length rotating slowly. Crystalline, hard edges.
void anim_iceCrown(uint32_t t) {
  float tf=t*0.001f;float rotSpeed=0.08f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x)-tf*rotSpeed;float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    // 12 spikes, alternating long and short, irregular widths
    float spikeCount=12.0f;float spikeAngle=fmod(angle+M_PI,2.0f*M_PI/spikeCount*spikeCount)/(2.0f*M_PI)*spikeCount;
    float spikePhase=fmod(spikeAngle,1.0f);float spikeIndex=floorf(spikeAngle);
    // Spike length varies by index
    float spikeLen=0.4f+0.5f*fabsf(sinf(spikeIndex*1.7f+tf*0.2f));
    // Angular sharpness — very hard edge like a crystal
    float angularBri=(spikePhase<0.08f||spikePhase>0.92f)?powf(1.0f-min(spikePhase,1.0f-spikePhase)/0.08f,3.0f):0.0f;
    // Only light within spike length
    float radialBri=(nD<spikeLen)?1.0f-nD/spikeLen:0.0f;
    float bri=angularBri*radialBri;
    uint8_t b=(uint8_t)(bri*240.0f);
    uint8_t sat=(uint8_t)(150.0f+nD*90.0f); // icy white at center, blue at tips
    uint8_t hue=160;
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x)-tf*rotSpeed;float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float spikeAngle=fmod(angle+M_PI,2.0f*M_PI/12.0f*12.0f)/(2.0f*M_PI)*12.0f;float spikePhase=fmod(spikeAngle,1.0f);float spikeIndex=floorf(spikeAngle);
    float spikeLen=0.4f+0.5f*fabsf(sinf(spikeIndex*1.7f+tf*0.2f));
    float angularBri=(spikePhase<0.08f||spikePhase>0.92f)?powf(1.0f-min(spikePhase,1.0f-spikePhase)/0.08f,3.0f):0.0f;
    float bri=angularBri*((nD<spikeLen)?1.0f-nD/spikeLen:0.0f);
    uint8_t b=(uint8_t)(bri*240.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(160,(uint8_t)(150.0f+nD*90.0f),b)):CRGB::Black);
  }}
  FastLED.show();
}

// 22. DEEP ABYSS
// Extremely slow dark blue rotation. Nearly invisible. Builds to full then fades.
void anim_deepAbyss(uint32_t t) {
  float tf=t*0.001f;
  float breathCycle=fmod(tf*0.15f,1.0f);
  float masterBri=powf(sinf(breathCycle*M_PI),1.5f)*0.55f; // reduced brightness
  // Slow hue drift for colour variety
  float hueShift=tf*8.0f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float sp=fmod(angle*5.0f/(2.0f*M_PI)-dist*0.07f-tf*0.08f,1.0f);if(sp<0.0f)sp+=1.0f;
    float bri=powf(sinf(sp*M_PI),4.0f)*sinf(nD*M_PI)*masterBri;
    float sp2=fmod(angle*3.0f/(2.0f*M_PI)+dist*0.05f+tf*0.05f,1.0f);if(sp2<0.0f)sp2+=1.0f;
    float bri2=powf(sinf(sp2*M_PI),6.0f)*sinf(nD*M_PI)*masterBri*0.4f;
    float total=constrain(bri+bri2,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*160.0f);
    // Colour: deep teal→purple→midnight blue
    uint8_t hue=(uint8_t)(hueShift+nD*40.0f+sp*30.0f);
    uint8_t sat=(uint8_t)(200.0f+total*55.0f);
    setLED(true,xi+1,n,b>1?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float sp=fmod(angle*5.0f/(2.0f*M_PI)-dist*0.07f-tf*0.08f,1.0f);if(sp<0.0f)sp+=1.0f;
    float bri=powf(sinf(sp*M_PI),4.0f)*sinf(nD*M_PI)*masterBri;
    float sp2=fmod(angle*3.0f/(2.0f*M_PI)+dist*0.05f+tf*0.05f,1.0f);if(sp2<0.0f)sp2+=1.0f;
    float bri2=powf(sinf(sp2*M_PI),6.0f)*sinf(nD*M_PI)*masterBri*0.4f;
    float total=constrain(bri+bri2,0.0f,1.0f);uint8_t b=(uint8_t)(total*160.0f);
    uint8_t hue=(uint8_t)(hueShift+nD*40.0f+sp*30.0f);uint8_t sat=(uint8_t)(200.0f+total*55.0f);
    setLED(false,yi+1,n,b>1?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 23. CORIOLIS STORM
// Hurricane structure: calm eye at center, tight eyewall, spiral bands outward.
void anim_coriolisStorm(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    // Rotation speed decreases with distance (differential rotation)
    float localSpeed=tf*(1.5f-nD*1.2f);
    float sp=fmod((angle+localSpeed)/(2.0f*M_PI)*5.0f,1.0f);if(sp<0.0f)sp+=1.0f;
    float armBri=powf(sinf(sp*M_PI),2.5f);
    // Eye: dark center
    float eyeRadius=0.12f;float eyeWall=0.20f;
    float eyeFade=nD<eyeRadius?0.0f:(nD<eyeWall?powf((nD-eyeRadius)/(eyeWall-eyeRadius),2.0f):1.0f);
    // Banding: outer spiral bands
    float band=0.6f+0.4f*sinf(nD*M_PI*5.0f-tf*0.4f);
    float bri=armBri*eyeFade*band;
    uint8_t b=(uint8_t)(bri*230.0f);
    uint8_t hue=(uint8_t)(148.0f+nD*15.0f);
    uint8_t sat=(uint8_t)(200.0f+nD*55.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float localSpeed=tf*(1.5f-nD*1.2f);float sp=fmod((angle+localSpeed)/(2.0f*M_PI)*5.0f,1.0f);if(sp<0.0f)sp+=1.0f;
    float armBri=powf(sinf(sp*M_PI),2.5f);float eyeRadius=0.12f;float eyeWall=0.20f;
    float eyeFade=nD<eyeRadius?0.0f:(nD<eyeWall?powf((nD-eyeRadius)/(eyeWall-eyeRadius),2.0f):1.0f);
    float band=0.6f+0.4f*sinf(nD*M_PI*5.0f-tf*0.4f);float bri=armBri*eyeFade*band;
    uint8_t b=(uint8_t)(bri*230.0f);uint8_t hue=(uint8_t)(148.0f+nD*15.0f);uint8_t sat=(uint8_t)(200.0f+nD*55.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 24. FIBONACCI SPIRAL
// Golden ratio arm spacing. Organic and mathematical. Different from uniform spirals.
void anim_fibonacci(uint32_t t) {
  float tf=t*0.001f;
  const float PHI=1.6180339887f;
  const float GOLDEN_ANGLE=2.0f*M_PI*(1.0f-1.0f/PHI); // ~137.5 degrees
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float bri=0.0f;
    // Sample 8 fibonacci arms
    for(int a=0;a<8;a++){
      float armAngle=fmod(a*GOLDEN_ANGLE+tf*0.4f,2.0f*M_PI);
      float armTight=0.08f+a*0.01f;
      float sp=fmod((angle-armAngle)/(2.0f*M_PI)-dist*armTight,1.0f);if(sp<0.0f)sp+=1.0f;if(sp>0.5f)sp=1.0f-sp;
      bri+=powf(1.0f-sp*2.0f,5.0f)*(1.0f-a*0.08f);
    }
    bri=constrain(bri*sinf(nD*M_PI),0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*220.0f);
    uint8_t hue=(uint8_t)(142.0f+nD*18.0f+tf*3.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,235,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float bri=0.0f;
    for(int a=0;a<8;a++){float armAngle=fmod(a*GOLDEN_ANGLE+tf*0.4f,2.0f*M_PI);float sp=fmod((angle-armAngle)/(2.0f*M_PI)-dist*(0.08f+a*0.01f),1.0f);if(sp<0.0f)sp+=1.0f;if(sp>0.5f)sp=1.0f-sp;bri+=powf(1.0f-sp*2.0f,5.0f)*(1.0f-a*0.08f);}
    bri=constrain(bri*sinf(nD*M_PI),0.0f,1.0f);uint8_t b=(uint8_t)(bri*220.0f);uint8_t hue=(uint8_t)(142.0f+nD*18.0f+tf*3.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,235,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 25. CRYSTAL FRACTURE
// Hard angular arms with sharp edges, like cracked ice sheet. No soft fading.
void anim_crystalFracture(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float bri=0.0f;
    // 6 arms with hard binary edge (above threshold = on, below = off)
    for(int a=0;a<6;a++){
      float armAng=fmod(angle*6.0f/(2.0f*M_PI)-dist*0.11f-tf*0.35f+(float)a,1.0f);if(armAng<0.0f)armAng+=1.0f;
      float threshold=0.07f+0.04f*sinf((float)a*1.3f+dist*0.2f); // jagged threshold = fracture lines
      if(armAng<threshold||armAng>(1.0f-threshold)){
        // Hard edge with a tiny anti-alias
        float edge=min(armAng,1.0f-armAng)/threshold;
        bri+=constrain(edge,0.0f,1.0f)*(1.0f-a*0.12f);
      }
    }
    bri=constrain(bri*sinf(nD*M_PI),0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*240.0f);
    // Pure icy: high value, varying saturation with distance
    uint8_t sat=(uint8_t)(80.0f+nD*170.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(158,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=constrain(dist/MAX_DIST,0.0f,1.0f);
    float bri=0.0f;
    for(int a=0;a<6;a++){float armAng=fmod(angle*6.0f/(2.0f*M_PI)-dist*0.11f-tf*0.35f+(float)a,1.0f);if(armAng<0.0f)armAng+=1.0f;float threshold=0.07f+0.04f*sinf((float)a*1.3f+dist*0.2f);if(armAng<threshold||armAng>(1.0f-threshold)){float edge=min(armAng,1.0f-armAng)/threshold;bri+=constrain(edge,0.0f,1.0f)*(1.0f-a*0.12f);}}
    bri=constrain(bri*sinf(nD*M_PI),0.0f,1.0f);uint8_t b=(uint8_t)(bri*240.0f);uint8_t sat=(uint8_t)(80.0f+nD*170.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(158,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// ──────────────────────────────────────────────
// BUTTONS
// ── Button control now via ESP-NOW from NeoTrellis ────
// GPIO pins 32/33/34 no longer used

// ══════════════════════════════════════════════
// 10 NEW ANIMATIONS (25-34)
// ══════════════════════════════════════════════

// 25. AURORA BOREALIS
// Slow undulating curtains of light, like northern lights.
// Tall vertical bands drift horizontally, shimmer organically.
void anim_aurora(uint32_t t) {
  float tf=t*0.001f;
  // Wider hue range: green→cyan→magenta→red→orange→back
  float palShift=tf*12.0f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nY=(float)n/121.0f;
    float band=sinf(x*0.04f+tf*0.12f)+sinf(x*0.07f-tf*0.08f+1.3f);
    band=(band+2.0f)/4.0f;
    float shimmer=sinf(nY*M_PI*6.0f+tf*2.0f+x*0.1f)*0.3f+0.7f;
    float curtain=sinf(nY*M_PI);
    float bri=band*shimmer*curtain;bri=constrain(bri,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*200.0f);
    // Wide hue sweep: green(85)→teal→magenta→red→orange, shifts over time
    uint8_t hue=(uint8_t)(palShift+band*160.0f+sinf(tf*0.2f+x*0.03f)*50.0f);
    uint8_t sat=(uint8_t)(180.0f+curtain*75.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=(float)n/121.0f;
    float band=sinf(y*0.04f+tf*0.12f)+sinf(y*0.07f-tf*0.08f+1.3f);band=(band+2.0f)/4.0f;
    float shimmer=sinf(nX*M_PI*6.0f+tf*2.0f+y*0.1f)*0.3f+0.7f;
    float curtain=sinf(nX*M_PI);float bri=constrain(band*shimmer*curtain,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*200.0f);uint8_t hue=(uint8_t)(palShift+band*160.0f+sinf(tf*0.2f+y*0.03f)*50.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,(uint8_t)(180.0f+curtain*75.0f),b)):CRGB::Black);
  }}
  FastLED.show();
}

// 26. LAVA LAMP
// Large slow blobs of colour rise and fall. Thick, viscous, hypnotic.
static float blobX[5],blobY[5],blobVX[5],blobVY[5],blobR[5];static uint8_t blobHue[5];static bool blobInit=false;
void anim_lavaLamp(uint32_t t,float dt) {
  if(!blobInit){for(int i=0;i<5;i++){blobX[i]=(random(80)-40);blobY[i]=(random(80)-40);blobVX[i]=(random(20)-10)*0.003f;blobVY[i]=(random(20)-10)*0.003f;blobR[i]=15.0f+random(20);blobHue[i]=random(255);}blobInit=true;}
  for(int i=0;i<5;i++){blobX[i]+=blobVX[i]*dt;blobY[i]+=blobVY[i]*dt;if(fabsf(blobX[i])>55.0f)blobVX[i]*=-1.0f;if(fabsf(blobY[i])>55.0f)blobVY[i]*=-1.0f;blobR[i]=15.0f+10.0f*sinf(t*0.0005f+i*1.3f);}
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float total=0.0f;uint8_t domHue=0;float domBri=0.0f;
    for(int i=0;i<5;i++){float d=sqrtf((x-blobX[i])*(x-blobX[i])+(y-blobY[i])*(y-blobY[i]));float v=max(0.0f,1.0f-d/blobR[i]);v=v*v*v;total+=v;if(v>domBri){domBri=v;domHue=blobHue[i];}}
    total=constrain(total,0.0f,1.0f);uint8_t b=(uint8_t)(total*220.0f);
    uint8_t sat=(uint8_t)(200.0f+total*55.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(domHue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float total=0.0f;uint8_t domHue=0;float domBri=0.0f;
    for(int i=0;i<5;i++){float d=sqrtf((x-blobX[i])*(x-blobX[i])+(y-blobY[i])*(y-blobY[i]));float v=max(0.0f,1.0f-d/blobR[i]);v=v*v*v;total+=v;if(v>domBri){domBri=v;domHue=blobHue[i];}}
    total=constrain(total,0.0f,1.0f);uint8_t b=(uint8_t)(total*220.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(domHue,(uint8_t)(200.0f+total*55.0f),b)):CRGB::Black);
  }}
  FastLED.show();
}

// 27. ELECTRIC NERVOUS SYSTEM
// Random sparks shoot along strips like nerve impulses firing.
// Short bright pulses travel at random speeds, leave dim trail.
static float nervePos[24];static float nerveSpeed[24];static uint8_t nerveHue[24];static bool nerveDir[24];static bool nerveInit=false;
void anim_nerveSystem(uint32_t t,float dt) {
  if(!nerveInit){for(int i=0;i<24;i++){nervePos[i]=random(121);nerveSpeed[i]=0.05f+random(150)*0.001f;nerveHue[i]=random(255);nerveDir[i]=random(2);}nerveInit=true;}
  for(int i=0;i<24;i++){nervePos[i]+=(nerveDir[i]?1:-1)*nerveSpeed[i]*dt;if(nervePos[i]>125.0f){nervePos[i]=125.0f;nerveDir[i]=false;nerveSpeed[i]=0.05f+random(150)*0.001f;nerveHue[i]=random(255);}if(nervePos[i]<-4.0f){nervePos[i]=-4.0f;nerveDir[i]=true;nerveSpeed[i]=0.05f+random(150)*0.001f;nerveHue[i]=random(255);}}
  for(int xi=0;xi<12;xi++){for(int n=1;n<=121;n++){
    float trail=nervePos[xi]-(float)n;float trailLen=10.0f;
    uint8_t b=0;
    if(trail>-1.0f&&trail<trailLen){float fade=trail<0?0.4f:(1.0f-trail/trailLen);b=(trail<1.0f)?255:(uint8_t)(fade*fade*220.0f);}
    setLED(true,xi+1,n,b>0?CRGB(CHSV(nerveHue[xi],220,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){int idx=12+yi;for(int n=1;n<=121;n++){
    float trail=nervePos[idx]-(float)n;float trailLen=10.0f;
    uint8_t b=0;
    if(trail>-1.0f&&trail<trailLen){float fade=trail<0?0.4f:(1.0f-trail/trailLen);b=(trail<1.0f)?255:(uint8_t)(fade*fade*220.0f);}
    setLED(false,yi+1,n,b>0?CRGB(CHSV(nerveHue[idx],220,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 28. GRAVITY LENS
// Simulates gravitational lensing. A massive object warps the
// brightness field around it, creating arcs and rings. Moves slowly.
void anim_gravityLens(uint32_t t) {
  float tf=t*0.001f;
  // Lens center slowly orbits
  float lx=sinf(tf*0.2f)*30.0f;float ly=cosf(tf*0.15f)*30.0f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    // Vector from this point to lens
    float dx=x-lx;float dy=y-ly;float dl=sqrtf(dx*dx+dy*dy)+0.1f;
    // Lensing deflection: bend rays toward lens
    float deflect=300.0f/(dl*dl+100.0f);
    float lensedX=x+dx/dl*deflect;float lensedY=y+dy/dl*deflect;
    // Sample a background pattern at lensed position
    float dist=sqrtf(lensedX*lensedX+lensedY*lensedY);
    float angle=atan2f(lensedY,lensedX);
    float pattern=sinf(dist*0.15f-tf*0.5f)*sinf(angle*4.0f+tf*0.3f);
    pattern=(pattern+1.0f)*0.5f;
    // Einstein ring: very bright circle at lens radius
    float ringDist=fabsf(dl-20.0f);
    float ring=max(0.0f,1.0f-ringDist/4.0f)*1.5f;
    float total=constrain(pattern*0.6f+ring,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*230.0f);
    uint8_t hue=(uint8_t)(180.0f+pattern*60.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,220,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dx=x-lx;float dy=y-ly;float dl=sqrtf(dx*dx+dy*dy)+0.1f;
    float deflect=300.0f/(dl*dl+100.0f);float lensedX=x+dx/dl*deflect;float lensedY=y+dy/dl*deflect;
    float dist=sqrtf(lensedX*lensedX+lensedY*lensedY);float angle=atan2f(lensedY,lensedX);
    float pattern=(sinf(dist*0.15f-tf*0.5f)*sinf(angle*4.0f+tf*0.3f)+1.0f)*0.5f;
    float ring=max(0.0f,1.0f-fabsf(dl-20.0f)/4.0f)*1.5f;
    float total=constrain(pattern*0.6f+ring,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*230.0f);uint8_t hue=(uint8_t)(180.0f+pattern*60.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,220,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 29. TIDAL WAVE
// Massive slow waves roll across the entire grid from one side to the other.
// Then reverse. Deep ocean colours. Foam crests bright white.
void anim_tidalWave(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    // Multiple waves at different speeds and sizes
    float w1=sinf(x*0.05f-tf*0.4f+y*0.01f);
    float w2=sinf(x*0.08f-tf*0.6f+1.5f+y*0.02f)*0.6f;
    float w3=sinf(x*0.03f-tf*0.25f+3.0f)*0.4f;
    float wave=(w1+w2+w3+2.0f)/4.0f; // 0..1
    // Foam crest: sharp bright peak when wave is near top
    float foam=max(0.0f,wave-0.75f)*4.0f;
    float bri=wave*0.7f+foam*0.5f;bri=constrain(bri,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*220.0f);
    // Deep blue→cyan→white at crest
    uint8_t hue=foam>0.3f?140:(uint8_t)(160.0f-wave*20.0f);
    uint8_t sat=foam>0.3f?(uint8_t)(255.0f-foam*200.0f):230;
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float w1=sinf(y*0.05f-tf*0.4f+x*0.01f);float w2=sinf(y*0.08f-tf*0.6f+1.5f+x*0.02f)*0.6f;float w3=sinf(y*0.03f-tf*0.25f+3.0f)*0.4f;
    float wave=(w1+w2+w3+2.0f)/4.0f;float foam=max(0.0f,wave-0.75f)*4.0f;float bri=constrain(wave*0.7f+foam*0.5f,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*220.0f);uint8_t hue=foam>0.3f?140:(uint8_t)(160.0f-wave*20.0f);uint8_t sat=foam>0.3f?(uint8_t)(255.0f-foam*200.0f):230;
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 30. CYMATICS
// Standing wave patterns like Chladni figures. Nodes and antinodes
// create complex geometric shapes that morph between modes.
void anim_cymatics(uint32_t t) {
  float tf=t*0.001f;
  // Mode numbers change faster at higher speed — more responsive
  float modeX=2.0f+sinf(tf*0.15f)*1.5f;float modeY=3.0f+sinf(tf*0.11f+1.0f)*1.5f;
  float modeX2=4.0f+sinf(tf*0.09f+2.0f)*2.0f;float modeY2=2.0f+sinf(tf*0.13f+0.5f)*1.5f;
  // Accent colour pulses at nodal intersections
  float accentPhase=fmod(tf*0.4f,1.0f);
  uint8_t accentHue=(uint8_t)(accentPhase*255.0f);
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nx=x/MAX_DIST;float ny=y/MAX_DIST;
    float v1=fabsf(cosf(modeX*nx*M_PI)*cosf(modeY*ny*M_PI))-fabsf(cosf(modeX2*nx*M_PI)*cosf(modeY2*ny*M_PI));
    float v=constrain(fabsf(v1),0.0f,1.0f);
    float bri=powf(1.0f-v,3.0f);
    // Accent: near nodal crossings (where two different modes nearly intersect)
    float cross=fabsf(fabsf(cosf(modeX*nx*M_PI)*cosf(modeY*ny*M_PI))-0.5f)<0.08f&&
                fabsf(fabsf(cosf(modeX2*nx*M_PI)*cosf(modeY2*ny*M_PI))-0.5f)<0.08f?1.0f:0.0f;
    uint8_t b=(uint8_t)((bri*0.7f+cross*0.5f)*195.0f);
    uint8_t hue=cross>0.5f?accentHue:(uint8_t)(modeX*30.0f+tf*10.0f);
    uint8_t sat=cross>0.5f?255:220;
    setLED(true,xi+1,n,b>5?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nx=x/MAX_DIST;float ny=y/MAX_DIST;
    float v1=fabsf(cosf(modeX*nx*M_PI)*cosf(modeY*ny*M_PI))-fabsf(cosf(modeX2*nx*M_PI)*cosf(modeY2*ny*M_PI));
    float v=constrain(fabsf(v1),0.0f,1.0f);float bri=powf(1.0f-v,3.0f);
    float cross=fabsf(fabsf(cosf(modeX*nx*M_PI)*cosf(modeY*ny*M_PI))-0.5f)<0.08f&&fabsf(fabsf(cosf(modeX2*nx*M_PI)*cosf(modeY2*ny*M_PI))-0.5f)<0.08f?1.0f:0.0f;
    uint8_t b=(uint8_t)((bri*0.7f+cross*0.5f)*195.0f);
    uint8_t hue=cross>0.5f?accentHue:(uint8_t)(modeX*30.0f+tf*10.0f);
    setLED(false,yi+1,n,b>5?CRGB(CHSV(hue,cross>0.5f?255:220,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 31. STARFIELD WARP
// Stars rushing past as if in warp drive. Each strip is a streak.
// Stars appear at center and shoot outward at high speed.
static float starDist[24];static float starSpeed[24];static float starAngle[24];static uint8_t starBri[24];static bool starInit=false;
void anim_starfieldWarp(uint32_t t,float dt) {
  if(!starInit){for(int i=0;i<24;i++){starDist[i]=random(100)*0.7f;starSpeed[i]=0.03f+random(80)*0.001f;starAngle[i]=random(360)*M_PI/180.0f;starBri[i]=100+random(155);}starInit=true;}
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
  for(int i=0;i<24;i++){
    starDist[i]+=starSpeed[i]*dt;
    if(starDist[i]>MAX_DIST+10.0f){starDist[i]=0.5f;starSpeed[i]=0.03f+random(80)*0.001f;starAngle[i]=random(360)*M_PI/180.0f;starBri[i]=100+random(155);}
    // Project star onto nearest strip
    float sx=cosf(starAngle[i])*starDist[i];float sy=sinf(starAngle[i])*starDist[i];
    // Find closest X strip
    float bestDX=999.0f;int bestXi=-1;for(int xi=0;xi<12;xi++){float d=fabsf(X_POS[xi]-sx);if(d<bestDX){bestDX=d;bestXi=xi;}}
    if(bestDX<8.0f&&bestXi>=0){int led=(int)(sy+CENTER);if(led>=1&&led<=121){float warp=constrain(starDist[i]/MAX_DIST,0.0f,1.0f);uint8_t b=(uint8_t)(starBri[i]*warp);setLED(true,bestXi+1,led,CRGB(CHSV(200+i*3,180,b)));}}
    // Find closest Y strip
    float bestDY=999.0f;int bestYi=-1;for(int yi=0;yi<12;yi++){float d=fabsf(Y_POS[yi]-sy);if(d<bestDY){bestDY=d;bestYi=yi;}}
    if(bestDY<8.0f&&bestYi>=0){int led=(int)(sx+CENTER);if(led>=1&&led<=121){float warp=constrain(starDist[i]/MAX_DIST,0.0f,1.0f);uint8_t b=(uint8_t)(starBri[i]*warp);setLED(false,bestYi+1,led,CRGB(CHSV(200+i*3,180,b)));}}
  }
  FastLED.show();
}

// 32. MAGNETIC FIELD LINES
// Visualises magnetic field lines of a dipole rotating slowly.
// Clean, scientific, beautiful.
void anim_magneticField(uint32_t t) {
  float tf=t*0.001f;
  float dipoleAngle=tf*0.3f;
  float dx=cosf(dipoleAngle);float dy=sinf(dipoleAngle);
  // Second dipole orbits slower, opposite polarity
  float dx2=cosf(-tf*0.18f+1.0f);float dy2=sinf(-tf*0.18f+1.0f);
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y)+0.01f;
    float mdotr=(dx*x+dy*y)/(dist*dist);
    float bx=(3.0f*mdotr*x/dist/dist-dx)/(dist*dist*dist);
    float by=(3.0f*mdotr*y/dist/dist-dy)/(dist*dist*dist);
    float bmag=sqrtf(bx*bx+by*by);
    float bangle=atan2f(by,bx);
    float stripAngle=atan2f(y,x);
    float alignment=fabsf(cosf(bangle-stripAngle));
    float bri=alignment*min(1.0f,bmag*500.0f)*sinf(constrain(dist/MAX_DIST,0.0f,1.0f)*M_PI);
    // Second dipole adds complementary field
    float mdotr2=(dx2*x+dy2*y)/(dist*dist);
    float bx2=(3.0f*mdotr2*x/dist/dist-dx2)/(dist*dist*dist);
    float by2=(3.0f*mdotr2*y/dist/dist-dy2)/(dist*dist*dist);
    float bmag2=sqrtf(bx2*bx2+by2*by2);
    float bangle2=atan2f(by2,bx2);
    float align2=fabsf(cosf(bangle2-stripAngle));
    float bri2=align2*min(1.0f,bmag2*500.0f)*sinf(constrain(dist/MAX_DIST,0.0f,1.0f)*M_PI)*0.6f;
    bri=constrain(bri+bri2,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*255.0f);
    // N pole=warm red/orange, S pole=blue, field lines=blend
    uint8_t hue=mdotr>0?(uint8_t)(10.0f+bri*20.0f):(uint8_t)(160.0f+bri*20.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,220,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y)+0.01f;float mdotr=(dx*x+dy*y)/(dist*dist);float bx=(3.0f*mdotr*x/dist/dist-dx)/(dist*dist*dist);float by=(3.0f*mdotr*y/dist/dist-dy)/(dist*dist*dist);float bmag=sqrtf(bx*bx+by*by);float bangle=atan2f(by,bx);float stripAngle=atan2f(y,x);float alignment=fabsf(cosf(bangle-stripAngle));float bri=alignment*min(1.0f,bmag*500.0f)*sinf(constrain(dist/MAX_DIST,0.0f,1.0f)*M_PI);
    float mdotr2=(dx2*x+dy2*y)/(dist*dist);float bx2=(3.0f*mdotr2*x/dist/dist-dx2)/(dist*dist*dist);float by2=(3.0f*mdotr2*y/dist/dist-dy2)/(dist*dist*dist);float bmag2=sqrtf(bx2*bx2+by2*by2);float align2=fabsf(cosf(atan2f(by2,bx2)-stripAngle));float bri2=align2*min(1.0f,bmag2*500.0f)*sinf(constrain(dist/MAX_DIST,0.0f,1.0f)*M_PI)*0.6f;
    bri=constrain(bri+bri2,0.0f,1.0f);uint8_t b=(uint8_t)(bri*255.0f);uint8_t hue=mdotr>0?(uint8_t)(10.0f+bri*20.0f):(uint8_t)(160.0f+bri*20.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,220,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 33. PENDULUM CHAOS
// Double pendulum traces chaotic paths across the grid.
// The path glows and fades — beautiful deterministic chaos.
static float p1Angle=0.5f,p2Angle=1.2f,p1Vel=0.0f,p2Vel=0.0f;static bool pendInit=false;
static float pendTrailX[60],pendTrailY[60];static int pendTrailIdx=0;
void anim_pendulumChaos(uint32_t t,float dt) {
  if(!pendInit){p1Angle=0.5f+random(100)*0.01f;p2Angle=1.2f+random(100)*0.01f;p1Vel=0.0f;p2Vel=0.0f;pendInit=true;}
  float g=9.8f;float L=0.6f;float dtS=dt*0.001f*2.0f; // speed up time
  // Double pendulum physics (simplified)
  float denom=2.0f-cosf(2.0f*(p1Angle-p2Angle));
  float a1=(-g*(2.0f)*sinf(p1Angle)-sinf(p1Angle-p2Angle)*(2.0f*(p2Vel*p2Vel+p1Vel*p1Vel*cosf(p1Angle-p2Angle))))/(L*denom);
  float a2=(2.0f*sinf(p1Angle-p2Angle)*(2.0f*p1Vel*p1Vel+g*cosf(p1Angle)+p2Vel*p2Vel*cosf(p1Angle-p2Angle)))/(L*denom);
  p1Vel+=a1*dtS;p2Vel+=a2*dtS;p1Angle+=p1Vel*dtS;p2Angle+=p2Vel*dtS;
  float x1=sinf(p1Angle)*35.0f;float y1=-cosf(p1Angle)*35.0f;
  float x2=x1+sinf(p2Angle)*35.0f;float y2=y1-cosf(p2Angle)*35.0f;
  pendTrailX[pendTrailIdx]=x2;pendTrailY[pendTrailIdx]=y2;pendTrailIdx=(pendTrailIdx+1)%60;

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  for(int ti=0;ti<60;ti++){
    int idx=(pendTrailIdx+ti)%60;float age=(float)ti/60.0f;
    float px=pendTrailX[idx];float py=pendTrailY[idx];
    // Paint onto nearest strips
    for(int xi=0;xi<12;xi++){float d=fabsf(X_POS[xi]-px);if(d<6.0f){int led=(int)(py+CENTER);if(led>=1&&led<=121){uint8_t b=(uint8_t)(age*age*200.0f*(1.0f-d/6.0f));setLED(true,xi+1,led,b>3?CRGB(CHSV((uint8_t)(age*200),230,b)):CRGB::Black);}}}
    for(int yi=0;yi<12;yi++){float d=fabsf(Y_POS[yi]-py);if(d<6.0f){int led=(int)(px+CENTER);if(led>=1&&led<=121){uint8_t b=(uint8_t)(age*age*200.0f*(1.0f-d/6.0f));setLED(false,yi+1,led,b>3?CRGB(CHSV((uint8_t)(age*200),230,b)):CRGB::Black);}}}
  }
  FastLED.show();
}

// 34. DARK MATTER
// Almost everything is dark. Faint tendrils of light trace invisible
// structure. Occasionally a region brightens revealing hidden geometry.
// Eerily beautiful and minimal.
void anim_darkMatter(uint32_t t) {
  float tf=t*0.001f;
  // Slow moving filaments of dark matter
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=dist/MAX_DIST;
    // Very faint underlying structure
    float structure=sinf(x*0.06f+tf*0.05f)*sinf(y*0.06f-tf*0.04f)+sinf(dist*0.08f-tf*0.03f)*0.5f;
    structure=(structure+1.5f)/3.0f; // 0..1
    // Rare bright reveal: slow moving spotlight
    float revealX=sinf(tf*0.17f)*50.0f;float revealY=cosf(tf*0.13f)*50.0f;
    float revealDist=sqrtf((x-revealX)*(x-revealX)+(y-revealY)*(y-revealY));
    float reveal=max(0.0f,1.0f-revealDist/25.0f);reveal=reveal*reveal;
    float bri=structure*0.08f+structure*reveal*0.9f; // mostly dark, revealed = bright
    bri=constrain(bri,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*200.0f);
    uint8_t hue=(uint8_t)(200.0f+nD*30.0f+tf*5.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,240,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);float nD=dist/MAX_DIST;
    float structure=sinf(x*0.06f+tf*0.05f)*sinf(y*0.06f-tf*0.04f)+sinf(dist*0.08f-tf*0.03f)*0.5f;structure=(structure+1.5f)/3.0f;
    float revealX=sinf(tf*0.17f)*50.0f;float revealY=cosf(tf*0.13f)*50.0f;
    float revealDist=sqrtf((x-revealX)*(x-revealX)+(y-revealY)*(y-revealY));float reveal=powf(max(0.0f,1.0f-revealDist/25.0f),2.0f);
    float bri=constrain(structure*0.08f+structure*reveal*0.9f,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*200.0f);uint8_t hue=(uint8_t)(200.0f+nD*30.0f+tf*5.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,240,b)):CRGB::Black);
  }}
  FastLED.show();
}

// ══════════════════════════════════════════════
// 10 NEW ANIMATIONS (35-44) — SQUARES, SYMMETRY, TUNNEL
// ══════════════════════════════════════════════

// Intersection positions for square drawing
const int INTS[12] = {10,19,29,38,47,56,66,75,84,93,103,112};

// Draw a square outline defined by grid coordinates (r1,c1)→(r2,c2)
// r,c are strip indices 1..12. Draws the 4 border strips between them.
void drawGridSquare(int r1,int c1,int r2,int c2,CRGB colour) {
  // top border: Y strip r1, LEDs between X c1 and X c2
  for(int led=INTS[c1-1]+1;led<INTS[c2-1];led++) setLED(false,r1,led,colour);
  // bottom border: Y strip r2
  for(int led=INTS[c1-1]+1;led<INTS[c2-1];led++) setLED(false,r2,led,colour);
  // left border: X strip c1, LEDs between Y r1 and Y r2
  for(int led=INTS[r1-1]+1;led<INTS[r2-1];led++) setLED(true,c1,led,colour);
  // right border: X strip c2
  for(int led=INTS[r1-1]+1;led<INTS[r2-1];led++) setLED(true,c2,led,colour);
}

// 35. SQUARE TUNNEL
// Concentric square frames zoom toward viewer like flying through a tunnel.
// Perspective: outer frames = far, inner = near. Smooth continuous motion.
void anim_squareTunnel(uint32_t t) {
  float tf=t*0.001f;
  float offset=fmod(tf*0.8f,1.0f);
  // Tunnel slowly rotates/shifts off-center for parallax
  float shiftX=sinf(tf*0.13f)*1.5f;
  float shiftY=cosf(tf*0.09f)*1.5f;

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  for(int frame=0;frame<8;frame++){
    float depth=(float)frame+offset;
    float fade=1.0f-depth/8.0f;if(fade<=0.0f)continue;
    float size=(7.0f-depth)/7.0f;
    int half=(int)(size*5.5f);if(half<1)continue;
    // Shift center slightly for parallax
    int cx=6+(int)(shiftX*(1.0f-size));
    int cy=6+(int)(shiftY*(1.0f-size));
    int r1=constrain(cy-half,1,12);int r2=constrain(cy+half+1,1,12);
    int c1=constrain(cx-half,1,12);int c2=constrain(cx+half+1,1,12);
    if(r1>=r2||c1>=c2)continue;
    uint8_t b=(uint8_t)(fade*fade*230.0f);
    // Each frame gets a hue that rotates — makes tunnel feel alive
    uint8_t hue=(uint8_t)(depth*32.0f+tf*25.0f);
    uint8_t sat=(uint8_t)(180.0f+fade*75.0f);
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,sat,b)));
  }
  FastLED.show();
}

// 36. CONCENTRIC SQUARE PULSE
// Square frames expand outward from center like sonar pings.
// Multiple overlapping pulses, smooth brightness wave.
void anim_concentricSquares(uint32_t t) {
  float tf=t*0.001f;float pulseRate=0.5f;

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  // For each possible square size (1..5 half-width = sizes 2..11)
  for(int half=1;half<=5;half++){
    int r1=6-half; int r2=7+half;
    int c1=6-half; int c2=7+half;
    // Pulse phase for this size: outer squares = earlier phase
    float phase=fmod(tf*pulseRate-(float)half*0.15f,1.0f);if(phase<0.0f)phase+=1.0f;
    // Smooth sine brightness
    float bri=powf(sinf(phase*M_PI),2.0f);
    uint8_t b=(uint8_t)(bri*220.0f);if(b<3)continue;
    uint8_t hue=(uint8_t)(tf*30.0f+half*20.0f);
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,220,b)));
  }
  FastLED.show();
}

// 37. SQUARE RAIN
// Random squares of varying sizes (1x1 to 4x4) flash on and fade out.
// Like raindrops hitting a grid surface. Each drop independent.
#define SQRAIN_COUNT 30
static int sqR[SQRAIN_COUNT],sqC[SQRAIN_COUNT],sqSize[SQRAIN_COUNT];
static uint32_t sqBirth[SQRAIN_COUNT];static uint8_t sqHue[SQRAIN_COUNT];
static bool sqRainInit=false;
void anim_squareRain(uint32_t t) {
  if(!sqRainInit){for(int i=0;i<SQRAIN_COUNT;i++){sqBirth[i]=t-random(2000);sqR[i]=1+random(11);sqC[i]=1+random(11);sqSize[i]=1+random(4);sqHue[i]=random(255);}sqRainInit=true;}
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
  uint32_t dropLife=600;
  for(int i=0;i<SQRAIN_COUNT;i++){
    uint32_t age=t-sqBirth[i];
    if(age>dropLife){sqBirth[i]=t;sqR[i]=1+random(11);sqC[i]=1+random(11);sqSize[i]=1+random(4);sqHue[i]=random(255);age=0;}
    float phase=(float)age/dropLife;
    // Fast rise, slow fade
    float bri=phase<0.1f?phase/0.1f:1.0f-(phase-0.1f)/0.9f;
    uint8_t b=(uint8_t)(bri*230.0f);if(b<3)continue;
    int r1=sqR[i]; int c1=sqC[i];
    int r2=constrain(r1+sqSize[i],1,12);
    int c2=constrain(c1+sqSize[i],1,12);
    if(r1>=r2||c1>=c2)continue;
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(sqHue[i],220,b)));
  }
  FastLED.show();
}

// 38. FOURFOLD SYMMETRY
// One quadrant generates a random organic animation.
// The result is mirrored into all 4 quadrants simultaneously.
// Geometric and hypnotic. Uses plasma-style source.
void anim_fourfoldSymmetry(uint32_t t) {
  float tf=t*0.001f;
  // Generate source quadrant (top-left: X1-6, Y1-6)
  // Then mirror to all 4 quadrants
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    // Fold into top-left quadrant
    float fx=fabsf(x); float fy=fabsf(y);
    // Source pattern on folded coords
    float v=sinf(fx*0.1f+tf*0.7f)*sinf(fy*0.09f-tf*0.5f)
           +sinf((fx+fy)*0.06f+tf*0.4f)*0.5f
           +sinf(sqrtf(fx*fx+fy*fy)*0.08f-tf*0.6f)*0.5f;
    v=(v+2.0f)/4.0f;
    uint8_t hue=(uint8_t)(v*180.0f+tf*20.0f);
    uint8_t bri=(uint8_t)(sinf(v*M_PI)*220.0f);
    setLED(true,xi+1,n,bri>3?CRGB(CHSV(hue,230,bri)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float fx=fabsf(x);float fy=fabsf(y);
    float v=sinf(fx*0.1f+tf*0.7f)*sinf(fy*0.09f-tf*0.5f)+sinf((fx+fy)*0.06f+tf*0.4f)*0.5f+sinf(sqrtf(fx*fx+fy*fy)*0.08f-tf*0.6f)*0.5f;
    v=(v+2.0f)/4.0f;uint8_t hue=(uint8_t)(v*180.0f+tf*20.0f);uint8_t bri=(uint8_t)(sinf(v*M_PI)*220.0f);
    setLED(false,yi+1,n,bri>3?CRGB(CHSV(hue,230,bri)):CRGB::Black);
  }}
  FastLED.show();
}

// 39. DIAMOND PULSE
// Concentric diamonds (45° squares) expand from center.
// Distance measured as Manhattan distance = perfect diamond shape.
void anim_diamondPulse(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    // Manhattan distance from center = diamond metric
    float manhattan=(fabsf(x)+fabsf(y))/MAX_DIST;
    // Multiple expanding rings
    float ring=fmod(manhattan-tf*0.4f,0.25f)/0.25f;
    float bri=powf(sinf(ring*M_PI),3.0f);
    // Rotation: slowly rotate the diamond
    float rotAngle=tf*0.1f;float rx=x*cosf(rotAngle)-y*sinf(rotAngle);float ry=x*sinf(rotAngle)+y*cosf(rotAngle);
    float rotManhattan=(fabsf(rx)+fabsf(ry))/MAX_DIST;
    float ring2=fmod(rotManhattan-tf*0.3f+0.5f,0.25f)/0.25f;
    float bri2=powf(sinf(ring2*M_PI),3.0f)*0.5f;
    float total=constrain(bri+bri2,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*230.0f);
    uint8_t hue=(uint8_t)(manhattan*200.0f+tf*25.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,230,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float manhattan=(fabsf(x)+fabsf(y))/MAX_DIST;float ring=fmod(manhattan-tf*0.4f,0.25f)/0.25f;float bri=powf(sinf(ring*M_PI),3.0f);
    float rotAngle=tf*0.1f;float rx=x*cosf(rotAngle)-y*sinf(rotAngle);float ry=x*sinf(rotAngle)+y*cosf(rotAngle);
    float ring2=fmod((fabsf(rx)+fabsf(ry))/MAX_DIST-tf*0.3f+0.5f,0.25f)/0.25f;
    float total=constrain(bri+powf(sinf(ring2*M_PI),3.0f)*0.5f,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*230.0f);uint8_t hue=(uint8_t)(manhattan*200.0f+tf*25.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,230,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 40. CHECKERBOARD MELT
// 121 squares in checkerboard pattern. Colours slowly melt and shift
// between squares. Neighbouring squares bleed into each other.
void anim_checkerboardMelt(uint32_t t) {
  float tf=t*0.001f;
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  for(int row=0;row<11;row++){for(int col=0;col<11;col++){
    bool checker=((row+col)%2==0);
    // Slowly shifting hue per square, neighbours offset
    float hueBase=(float)(row*11+col)*2.0f;
    float hueShift=sinf(tf*0.3f+hueBase*0.05f)*40.0f;
    uint8_t hue=(uint8_t)(hueBase+hueShift+tf*15.0f);
    float bri=0.6f+0.4f*sinf(tf*0.5f+hueBase*0.1f);
    uint8_t b=(uint8_t)(bri*(checker?200.0f:80.0f));
    // Draw filled square: all 4 borders
    int r1=row+1;int c1=col+1;int r2=row+2;int c2=col+2;
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,230,b)));
  }}
  FastLED.show();
}

// 41. GAME OF LIFE
// Conway's Game of Life on the 11x11 grid of squares.
// Each square = 1 cell. Evolves every 400ms. Full colour by age.
static bool golGrid[11][11]; static bool golNext[11][11];
static uint8_t golAge[11][11]; // age of living cells
static uint32_t golLastStep=0; static bool golInit=false;
void anim_gameOfLife(uint32_t t) {
  if(!golInit){
    for(int r=0;r<11;r++)for(int c=0;c<11;c++){golGrid[r][c]=(random(3)==0);golAge[r][c]=0;}
    golInit=true;golLastStep=t;
  }
  if(t-golLastStep>350){
    // Step
    for(int r=0;r<11;r++)for(int c=0;c<11;c++){
      int n=0;
      for(int dr=-1;dr<=1;dr++)for(int dc=-1;dc<=1;dc++){if(dr==0&&dc==0)continue;int nr=(r+dr+11)%11;int nc=(c+dc+11)%11;if(golGrid[nr][nc])n++;}
      golNext[r][c]=(golGrid[r][c]?(n==2||n==3):(n==3));
    }
    // Check if static (copy and count changes)
    int changes=0;
    for(int r=0;r<11;r++)for(int c=0;c<11;c++){if(golNext[r][c]!=golGrid[r][c])changes++;golGrid[r][c]=golNext[r][c];if(golGrid[r][c])golAge[r][c]++;else golAge[r][c]=0;}
    // If stuck, randomize
    if(changes==0){for(int r=0;r<11;r++)for(int c=0;c<11;c++){golGrid[r][c]=(random(3)==0);golAge[r][c]=0;}}
    golLastStep=t;
  }
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
  for(int row=0;row<11;row++)for(int col=0;col<11;col++){
    if(!golGrid[row][col])continue;
    // Colour by age: young=white, old=deep colour
    uint8_t age=min((int)golAge[row][col],60);
    uint8_t hue=(uint8_t)(age*4);
    uint8_t sat=(uint8_t)(min((int)age*4,255));
    uint8_t bri=200;
    drawGridSquare(row+1,col+1,row+2,col+2,CRGB(CHSV(hue,sat,bri)));
  }
  FastLED.show();
}

// 42. ROTATING CUBE
// Isometric projection of a rotating cube. Cube faces drawn using
// grid squares. 3 visible faces with different brightness/colour.
void anim_rotatingCube(uint32_t t) {
  float tf=t*0.001f;
  float rotX=tf*0.4f; float rotY=tf*0.6f;

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  // Cube vertices in 3D (centered, size=4 strips)
  float S=4.0f;
  float verts[8][3]={{-S,-S,-S},{S,-S,-S},{S,S,-S},{-S,S,-S},{-S,-S,S},{S,-S,S},{S,S,S},{-S,S,S}};

  // Rotate all vertices
  float proj[8][2];
  for(int v=0;v<8;v++){
    float x=verts[v][0],y=verts[v][1],z=verts[v][2];
    // Rotate around Y
    float x2=x*cosf(rotY)-z*sinf(rotY); float z2=x*sinf(rotY)+z*cosf(rotY);
    // Rotate around X
    float y3=y*cosf(rotX)-z2*sinf(rotX); float z3=y*sinf(rotX)+z2*cosf(rotX);
    // Isometric projection → grid coords (6=center)
    float iso_x=x2*0.866f-y3*0.866f; float iso_y=x2*0.5f+y3*0.5f-z3;
    proj[v][0]=iso_x+6.5f; proj[v][1]=iso_y+6.5f; // offset to grid center
  }

  // Draw 3 visible faces as square outlines
  // Face indices: front(4,5,6,7), top(3,2,6,7), right(1,5,6,2)
  int faces[3][4]={{4,5,6,7},{3,2,6,7},{1,5,6,2}};
  uint8_t faceHue[3]={(uint8_t)(tf*20),(uint8_t)(tf*20+60),(uint8_t)(tf*20+120)};
  uint8_t faceBri[3]={220,160,100};

  for(int f=0;f<3;f++){
    // Project each face vertex to nearest grid square corner
    for(int e=0;e<4;e++){
      int v1=faces[f][e]; int v2=faces[f][(e+1)%4];
      int r1=constrain((int)proj[v1][1],1,12); int c1=constrain((int)proj[v1][0],1,12);
      int r2=constrain((int)proj[v2][1],1,12); int c2=constrain((int)proj[v2][0],1,12);
      if(r1==r2&&c1==c2)continue;
      // Draw line between projected points on strips
      int rMin=min(r1,r2);int rMax=max(r1,r2);int cMin=min(c1,c2);int cMax=max(c1,c2);
      if(rMin==rMax){// horizontal edge → Y strip
        for(int led=INTS[cMin-1];led<=INTS[cMax-1];led++)setLED(false,rMin,led,CRGB(CHSV(faceHue[f],220,faceBri[f])));
      }else if(cMin==cMax){// vertical edge → X strip
        for(int led=INTS[rMin-1];led<=INTS[rMax-1];led++)setLED(true,cMin,led,CRGB(CHSV(faceHue[f],220,faceBri[f])));
      }
    }
  }
  FastLED.show();
}

// 43. SQUARE BREATHING
// 11x11 outermost square contracts to 1x1 center, then expands back.
// Smooth, meditative. Multiple nested squares all breathing together.
void anim_squareBreathing(uint32_t t) {
  float tf=t*0.001f;
  float breath=0.5f+0.5f*sinf(tf*0.5f); // 0..1 expand/contract

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  // Draw multiple nested squares all breathing in sync with phase offset
  for(int layer=0;layer<5;layer++){
    float layerPhase=fmod(breath+(float)layer*0.15f,1.0f);
    float size=layerPhase; // 0=1x1 center, 1=11x11 outer
    int half=(int)(size*5.0f+0.5f);half=constrain(half,1,5);
    int r1=6-half;int r2=7+half;int c1=6-half;int c2=7+half;
    float bri=sinf(layerPhase*M_PI);
    uint8_t b=(uint8_t)(bri*bri*230.0f);if(b<3)continue;
    uint8_t hue=(uint8_t)(tf*20.0f+layer*35.0f);
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,220,b)));
  }
  FastLED.show();
}

// 44. FRACTAL SQUARES
// Squares subdivide recursively. Zooms into center subdivision endlessly.
// Mandelbrot-set-zoom vibe using only the square grid geometry.
void anim_fractalSquares(uint32_t t) {
  float tf=t*0.001f;
  float zoom=fmod(tf*0.3f,1.0f); // 0..1 one zoom cycle
  float zoomFactor=powf(2.0f,zoom); // 1..2 then resets

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  // Draw grid squares at multiple scales that zoom inward
  for(int level=0;level<4;level++){
    float scale=powf(0.5f,level)/zoomFactor;
    float fade=(float)level/4.0f;
    float bri=1.0f-fade;bri*=(1.0f-zoom*0.3f);
    uint8_t b=(uint8_t)(bri*200.0f);if(b<5)continue;
    uint8_t hue=(uint8_t)(level*60.0f+tf*30.0f);

    // Draw subdivided squares at this scale
    int divisions=(int)(1.0f/scale);
    if(divisions<1)divisions=1;if(divisions>5)divisions=5;
    int step=11/divisions;if(step<1)step=1;
    for(int r=0;r<11;r+=step)for(int c=0;c<11;c+=step){
      int r1=r+1;int c1=c+1;int r2=min(r+step+1,12);int c2=min(c+step+1,12);
      if(r1>=r2||c1>=c2)continue;
      drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,220,b)));
    }
  }
  FastLED.show();
}

// ══════════════════════════════════════════════
// 10 NEW ANIMATIONS (45-54) — GRADIENT, BRIGHTNESS, SMOOTH, SLOW
// ══════════════════════════════════════════════

// 45. SILK
// Soft flowing waves of colour drift across the grid like silk in wind.
// Very smooth, multi-layered, slow.
void anim_silk(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST; float nY=y/MAX_DIST;
    // Multiple very slow sine layers
    float v = sinf(nX*1.2f + nY*0.8f + tf*0.18f)
            + sinf(nX*0.7f - nY*1.1f + tf*0.13f + 1.5f)*0.7f
            + sinf((nX+nY)*0.9f + tf*0.09f + 3.0f)*0.5f;
    v=(v+2.2f)/4.4f; // 0..1
    // Smooth brightness: never fully dark, never fully bright
    float bri=0.2f+0.7f*powf(sinf(v*M_PI),1.5f);
    uint8_t b=(uint8_t)(bri*210.0f);
    // Slowly drifting hue across the surface
    uint8_t hue=(uint8_t)(v*120.0f + tf*8.0f + nX*20.0f);
    uint8_t sat=(uint8_t)(160.0f+v*80.0f);
    setLED(true,xi+1,n,CRGB(CHSV(hue,sat,b)));
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float v=sinf(nX*1.2f+nY*0.8f+tf*0.18f)+sinf(nX*0.7f-nY*1.1f+tf*0.13f+1.5f)*0.7f+sinf((nX+nY)*0.9f+tf*0.09f+3.0f)*0.5f;
    v=(v+2.2f)/4.4f;float bri=0.2f+0.7f*powf(sinf(v*M_PI),1.5f);
    uint8_t b=(uint8_t)(bri*210.0f);uint8_t hue=(uint8_t)(v*120.0f+tf*8.0f+nX*20.0f);uint8_t sat=(uint8_t)(160.0f+v*80.0f);
    setLED(false,yi+1,n,CRGB(CHSV(hue,sat,b)));
  }}
  FastLED.show();
}

// 46. DEEP BREATH
// The entire grid breathes as one organism. Brightness pulses from
// near-zero to full on a long slow cycle. Colour shifts simultaneously.
void anim_deepBreath(uint32_t t) {
  float tf=t*0.001f;
  // Master breath: one full cycle every ~8 seconds
  float breath=0.5f+0.5f*sinf(tf*0.25f*M_PI*0.5f);
  // Breath is not linear — ease in/out (smoothstep)
  breath=breath*breath*(3.0f-2.0f*breath);
  // Colour slowly cycles through warm→cool→warm
  uint8_t hue=(uint8_t)(tf*6.0f);
  // Distance from center adds subtle gradient
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y)/MAX_DIST;
    // Outer LEDs slightly delayed in phase
    float localBreath=0.5f+0.5f*sinf((tf-dist*0.8f)*0.25f*M_PI*0.5f);
    localBreath=localBreath*localBreath*(3.0f-2.0f*localBreath);
    float bri=0.05f+0.9f*localBreath;
    uint8_t b=(uint8_t)(bri*220.0f);
    uint8_t localHue=(uint8_t)(hue+dist*20.0f);
    uint8_t sat=(uint8_t)(180.0f+localBreath*75.0f);
    setLED(true,xi+1,n,CRGB(CHSV(localHue,sat,b)));
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y)/MAX_DIST;
    float localBreath=0.5f+0.5f*sinf((tf-dist*0.8f)*0.25f*M_PI*0.5f);localBreath=localBreath*localBreath*(3.0f-2.0f*localBreath);
    float bri=0.05f+0.9f*localBreath;uint8_t b=(uint8_t)(bri*220.0f);
    uint8_t localHue=(uint8_t)(hue+dist*20.0f);uint8_t sat=(uint8_t)(180.0f+localBreath*75.0f);
    setLED(false,yi+1,n,CRGB(CHSV(localHue,sat,b)));
  }}
  FastLED.show();
}

// 47. SUNSET HORIZON
// Colour gradient flows from top to bottom. Slowly rotates through
// all sunset colours — deep blue → indigo → purple → red → orange → gold.
// The horizon line drifts slowly.
void anim_sunsetHorizon(uint32_t t) {
  float tf=t*0.001f;
  // Horizon position drifts slowly
  float horizon=sinf(tf*0.07f)*0.3f; // -0.3..+0.3 normalised
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nY=y/MAX_DIST; // -1..1
    // Position relative to horizon
    float pos=nY-horizon; // -1.3..1.3
    float posN=(pos+1.3f)/2.6f; // 0..1
    // Colour gradient: sky (top, posN=0) → horizon (posN=0.5) → ground (posN=1)
    // Sky: deep blue-purple
    // Horizon: orange-red glow
    // Add a soft atmospheric glow at the horizon line
    float horizonGlow=expf(-fabsf(pos)*6.0f)*0.6f;
    // Main gradient hue: 180(blue) at top → 0(red) at horizon → 25(orange) below
    uint8_t hue;
    if(posN<0.5f) hue=(uint8_t)(200.0f-posN*2.0f*200.0f+(tf*5.0f));
    else          hue=(uint8_t)(0.0f+(posN-0.5f)*2.0f*25.0f+(tf*5.0f));
    float bri=0.3f+0.6f*(1.0f-fabsf(posN-0.5f)*1.5f)+horizonGlow;
    bri=constrain(bri,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*220.0f);
    uint8_t sat=(uint8_t)(180.0f+horizonGlow*75.0f);
    setLED(true,xi+1,n,CRGB(CHSV(hue,sat,b)));
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nY=y/MAX_DIST;float pos=nY-horizon;float posN=(pos+1.3f)/2.6f;
    float horizonGlow=expf(-fabsf(pos)*6.0f)*0.6f;
    uint8_t hue=posN<0.5f?(uint8_t)(200.0f-posN*2.0f*200.0f+(tf*5.0f)):(uint8_t)((posN-0.5f)*2.0f*25.0f+(tf*5.0f));
    float bri=constrain(0.3f+0.6f*(1.0f-fabsf(posN-0.5f)*1.5f)+horizonGlow,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*220.0f);uint8_t sat=(uint8_t)(180.0f+horizonGlow*75.0f);
    setLED(false,yi+1,n,CRGB(CHSV(hue,sat,b)));
  }}
  FastLED.show();
}

// 48. OIL SLICK
// Iridescent interference colours like oil on water.
// Thin-film colour shifting, slow flowing motion.
void anim_oilSlick(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    // Thin film interference: path length difference creates colour
    // Multiple slow flowing waves of "film thickness"
    float thickness = sinf(nX*2.1f+nY*1.3f+tf*0.12f)*0.5f
                    + sinf(nX*0.8f-nY*2.4f+tf*0.09f+2.0f)*0.3f
                    + sinf((nX*nX+nY*nY)*3.0f-tf*0.07f)*0.2f;
    thickness=(thickness+1.0f)*0.5f; // 0..1
    // Interference hue cycles rapidly with thickness
    uint8_t hue=(uint8_t)(thickness*255.0f+tf*4.0f);
    // High saturation, moderate brightness — iridescent look
    float bri=0.4f+0.5f*powf(sinf(thickness*M_PI*2.0f+tf*0.3f)*0.5f+0.5f,1.5f);
    uint8_t b=(uint8_t)(bri*200.0f);
    setLED(true,xi+1,n,CRGB(CHSV(hue,255,b)));
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float thickness=sinf(nX*2.1f+nY*1.3f+tf*0.12f)*0.5f+sinf(nX*0.8f-nY*2.4f+tf*0.09f+2.0f)*0.3f+sinf((nX*nX+nY*nY)*3.0f-tf*0.07f)*0.2f;
    thickness=(thickness+1.0f)*0.5f;uint8_t hue=(uint8_t)(thickness*255.0f+tf*4.0f);
    float bri=0.4f+0.5f*powf(sinf(thickness*M_PI*2.0f+tf*0.3f)*0.5f+0.5f,1.5f);
    setLED(false,yi+1,n,CRGB(CHSV(hue,255,(uint8_t)(bri*200.0f))));
  }}
  FastLED.show();
}

// 49. EMBER GLOW
// Glowing coal aesthetic. Brightness pulses organically like a fire
// breathing. Deep reds and oranges, dark areas very dark.
void anim_emberGlow(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    // Slow organic turbulence
    float heat = sinf(nX*2.3f+nY*1.7f+tf*0.22f)
               * sinf(nX*1.1f-nY*2.1f+tf*0.17f+1.0f);
    heat=(heat+1.0f)*0.5f; // 0..1
    // Secondary slow pulse
    float pulse=0.5f+0.5f*sinf(tf*0.4f+nX*1.5f+nY*1.2f);
    float total=heat*0.7f+pulse*0.3f;
    // Power curve: dark areas very dark, bright areas very bright
    total=powf(total,2.2f);
    float bri=constrain(total,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*240.0f);
    // Colour: deep red → orange → yellow-white at peak
    uint8_t hue=(uint8_t)(bri<0.3f?0:bri<0.7f?(bri-0.3f)/0.4f*25.0f:25.0f+(bri-0.7f)/0.3f*15.0f);
    uint8_t sat=(uint8_t)(255.0f-bri*120.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float heat=sinf(nX*2.3f+nY*1.7f+tf*0.22f)*sinf(nX*1.1f-nY*2.1f+tf*0.17f+1.0f);heat=(heat+1.0f)*0.5f;
    float pulse=0.5f+0.5f*sinf(tf*0.4f+nX*1.5f+nY*1.2f);
    float total=powf(heat*0.7f+pulse*0.3f,2.2f);float bri=constrain(total,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*240.0f);uint8_t hue=(uint8_t)(bri<0.3f?0:bri<0.7f?(bri-0.3f)/0.4f*25.0f:25.0f+(bri-0.7f)/0.3f*15.0f);
    uint8_t sat=(uint8_t)(255.0f-bri*120.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 50. DEPTH MAP
// Radial gradient: centre bright and warm, edges dark and cool.
// Slowly rotates and breathes. Smooth concentric colour bands.
void anim_depthMap(uint32_t t) {
  float tf=t*0.001f;
  float breath=0.7f+0.3f*sinf(tf*0.35f);
  float rotSpeed=tf*0.06f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    // Slowly rotating coordinate
    float rx=x*cosf(rotSpeed)-y*sinf(rotSpeed);
    float ry=x*sinf(rotSpeed)+y*cosf(rotSpeed);
    float dist=sqrtf(rx*rx+ry*ry)/MAX_DIST; // 0..1
    // Smooth radial gradient with soft banding
    float banding=0.8f+0.2f*sinf(dist*M_PI*5.0f-tf*0.3f);
    float bri=(1.0f-powf(dist,1.4f))*banding*breath;
    bri=constrain(bri,0.0f,1.0f);
    // Colour: warm (low hue) at centre, cool (high hue) at edge
    uint8_t hue=(uint8_t)(dist*160.0f+tf*4.0f);
    uint8_t sat=(uint8_t)(150.0f+dist*105.0f);
    uint8_t b=(uint8_t)(bri*230.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float rx=x*cosf(rotSpeed)-y*sinf(rotSpeed);float ry=x*sinf(rotSpeed)+y*cosf(rotSpeed);
    float dist=sqrtf(rx*rx+ry*ry)/MAX_DIST;float banding=0.8f+0.2f*sinf(dist*M_PI*5.0f-tf*0.3f);
    float bri=constrain((1.0f-powf(dist,1.4f))*banding*breath,0.0f,1.0f);
    uint8_t hue=(uint8_t)(dist*160.0f+tf*4.0f);uint8_t sat=(uint8_t)(150.0f+dist*105.0f);uint8_t b=(uint8_t)(bri*230.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 51. NORTHERN MIST
// Extremely delicate wisps of colour float across the grid.
// Very low brightness. Blue/green/white. Ghostly and minimal.
void anim_northernMist(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    // Very slow multi-layer mist
    float mist = sinf(nX*1.5f+nY*0.6f+tf*0.08f)*0.5f+0.5f;
    float mist2= sinf(nX*0.4f-nY*1.8f+tf*0.06f+2.5f)*0.5f+0.5f;
    float mist3= sinf((nX+nY)*1.1f+tf*0.05f+5.0f)*0.5f+0.5f;
    // Multiply layers for wispy look (most of grid near-zero)
    float total=mist*mist2*mist3;
    total=powf(total,1.5f); // darken midtones
    float bri=total*0.55f; // max brightness very low — mist is subtle
    uint8_t b=(uint8_t)(bri*180.0f);
    // Colour: shifts between icy blue, mint green, and near-white
    uint8_t hue=(uint8_t)(140.0f+mist*40.0f+tf*3.0f);
    uint8_t sat=(uint8_t)(100.0f+mist2*80.0f);
    setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float mist=sinf(nX*1.5f+nY*0.6f+tf*0.08f)*0.5f+0.5f;float mist2=sinf(nX*0.4f-nY*1.8f+tf*0.06f+2.5f)*0.5f+0.5f;float mist3=sinf((nX+nY)*1.1f+tf*0.05f+5.0f)*0.5f+0.5f;
    float total=powf(mist*mist2*mist3,1.5f);float bri=total*0.55f;uint8_t b=(uint8_t)(bri*180.0f);
    uint8_t hue=(uint8_t)(140.0f+mist*40.0f+tf*3.0f);uint8_t sat=(uint8_t)(100.0f+mist2*80.0f);
    setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 52. MOLTEN GOLD
// Heavy slow viscous waves of gold and amber. High brightness at peaks,
// very dark in troughs. Like liquid metal cooling and flowing.
void anim_moltenGold(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    // Slow heavy waves — low frequency, high amplitude
    float flow = sinf(nX*1.3f+nY*0.9f+tf*0.14f)
               + sinf(nX*0.6f-nY*1.5f+tf*0.11f+2.0f)*0.6f
               + sinf(sqrtf(nX*nX+nY*nY)*2.0f-tf*0.08f)*0.4f;
    flow=(flow+2.0f)/4.0f; // 0..1
    // Viscous: power curve makes bright peaks sharp, dark troughs wide
    float bri=powf(flow,1.8f);
    uint8_t b=(uint8_t)(bri*245.0f);
    // Gold palette: dark amber→gold→bright warm white at peaks
    uint8_t hue=(uint8_t)(25.0f-bri*10.0f); // 25(orange)→15(warm gold)
    uint8_t sat=(uint8_t)(255.0f-bri*160.0f); // desaturates to near-white at peaks
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float flow=sinf(nX*1.3f+nY*0.9f+tf*0.14f)+sinf(nX*0.6f-nY*1.5f+tf*0.11f+2.0f)*0.6f+sinf(sqrtf(nX*nX+nY*nY)*2.0f-tf*0.08f)*0.4f;
    flow=(flow+2.0f)/4.0f;float bri=powf(flow,1.8f);uint8_t b=(uint8_t)(bri*245.0f);
    uint8_t hue=(uint8_t)(25.0f-bri*10.0f);uint8_t sat=(uint8_t)(255.0f-bri*160.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// 53. COSMIC DUST
// Nebula-like colour clouds drifting very slowly. Purple/blue/pink.
// Soft and dreamlike. No hard edges anywhere.
void anim_cosmicDust(uint32_t t) {
  float tf=t*0.001f;
  // 3 slowly moving colour clouds
  float c1x=sinf(tf*0.07f)*40.0f;float c1y=cosf(tf*0.05f)*40.0f;
  float c2x=sinf(tf*0.06f+2.0f)*45.0f;float c2y=cosf(tf*0.08f+1.0f)*35.0f;
  float c3x=sinf(tf*0.09f+4.0f)*38.0f;float c3y=cosf(tf*0.06f+3.0f)*42.0f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float d1=sqrtf((x-c1x)*(x-c1x)+(y-c1y)*(y-c1y));
    float d2=sqrtf((x-c2x)*(x-c2x)+(y-c2y)*(y-c2y));
    float d3=sqrtf((x-c3x)*(x-c3x)+(y-c3y)*(y-c3y));
    // Soft Gaussian clouds
    float v1=expf(-d1*d1*0.0008f)*0.8f;
    float v2=expf(-d2*d2*0.0007f)*0.7f;
    float v3=expf(-d3*d3*0.0009f)*0.6f;
    // Each cloud has its own hue
    CRGB col=CRGB::Black;
    if(v1>0.01f){CRGB c=CRGB(CHSV(200,(uint8_t)(220),(uint8_t)(v1*200.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    if(v2>0.01f){CRGB c=CRGB(CHSV(240,(uint8_t)(200),(uint8_t)(v2*180.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    if(v3>0.01f){CRGB c=CRGB(CHSV(170,(uint8_t)(210),(uint8_t)(v3*160.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    setLED(true,xi+1,n,col);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float d1=sqrtf((x-c1x)*(x-c1x)+(y-c1y)*(y-c1y));float d2=sqrtf((x-c2x)*(x-c2x)+(y-c2y)*(y-c2y));float d3=sqrtf((x-c3x)*(x-c3x)+(y-c3y)*(y-c3y));
    float v1=expf(-d1*d1*0.0008f)*0.8f;float v2=expf(-d2*d2*0.0007f)*0.7f;float v3=expf(-d3*d3*0.0009f)*0.6f;
    CRGB col=CRGB::Black;
    if(v1>0.01f){CRGB c=CRGB(CHSV(200,220,(uint8_t)(v1*200.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    if(v2>0.01f){CRGB c=CRGB(CHSV(240,200,(uint8_t)(v2*180.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    if(v3>0.01f){CRGB c=CRGB(CHSV(170,210,(uint8_t)(v3*160.0f)));col.r=qadd8(col.r,c.r);col.g=qadd8(col.g,c.g);col.b=qadd8(col.b,c.b);}
    setLED(false,yi+1,n,col);
  }}
  FastLED.show();
}

// 54. HEARTGLOW
// One single very slow warm pulse radiates from the center.
// Long pause between beats. Deeply warm and intimate.
// The brightest possible version of "one thing done perfectly".
void anim_heartglow(uint32_t t) {
  float tf=t*0.001f;
  // Heartbeat timing: beat every ~4s, very slow expansion
  float cycle=fmod(tf*0.25f,1.0f); // one cycle per 4s
  // Shape: fast rise (0→0.15), slow glow (0.15→0.6), long pause (0.6→1.0)
  float pulse;
  if(cycle<0.15f)      pulse=cycle/0.15f;                          // rise
  else if(cycle<0.6f)  pulse=1.0f-(cycle-0.15f)/0.45f*0.7f;       // glow decay
  else                 pulse=0.3f*(1.0f-(cycle-0.6f)/0.4f);        // tail to silence
  pulse=pulse*pulse*(3.0f-2.0f*pulse); // smoothstep

  // Pulse radius expands outward
  float radius=cycle*MAX_DIST*1.3f;

  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);
    // Warm ambient glow always present (very dim)
    float ambient=expf(-dist*0.03f)*0.08f;
    // Expanding pulse ring
    float ring=max(0.0f,1.0f-fabsf(dist-radius)/12.0f)*pulse;
    // Core glow at center during peak
    float core=expf(-dist*0.07f)*pulse*0.8f;
    float total=constrain(ambient+ring+core,0.0f,1.0f);
    uint8_t b=(uint8_t)(total*235.0f);
    // Warm: deep red→warm orange→soft white at peak
    uint8_t hue=(uint8_t)(5.0f+pulse*15.0f);
    uint8_t sat=(uint8_t)(255.0f-total*180.0f);
    setLED(true,xi+1,n,b>1?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float dist=sqrtf(x*x+y*y);
    float ambient=expf(-dist*0.03f)*0.08f;float ring=max(0.0f,1.0f-fabsf(dist-radius)/12.0f)*pulse;float core=expf(-dist*0.07f)*pulse*0.8f;
    float total=constrain(ambient+ring+core,0.0f,1.0f);uint8_t b=(uint8_t)(total*235.0f);
    uint8_t hue=(uint8_t)(5.0f+pulse*15.0f);uint8_t sat=(uint8_t)(255.0f-total*180.0f);
    setLED(false,yi+1,n,b>1?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

// ══════════════════════════════════════════════
// ANIMATION 55: EVOLUTION
// A self-developing generative system. Never the same twice.
// ══════════════════════════════════════════════

struct Gene {
  float value;
  float target;
  float rate;
  float baseRate;
  float memory;
};

static Gene evo[16];
static float evoMood       = 0.0f;
static float evoMoodTarget = 0.3f;
static float evoPalette    = 0.0f;
static float evoPaletteSpd = 0.003f; // slow continuous hue drift
static uint32_t evoLastMutate = 0;
static uint32_t evoNextMutate = 8000;
static bool evoInit = false;
static uint32_t evoLastT = 0;

void evoInit_fn(uint32_t t) {
  // Seed genes with safe, visually reasonable starting values
  // Gene ranges are pre-constrained so random init always looks ok
  float seeds[16] = {
    0.3f,  // 0: arm count → ~4 arms
    0.2f,  // 1: rotation speed → slow
    0.3f,  // 2: tightness → medium
    0.4f,  // 3: arm width → medium
    0.5f,  // 4: brightness curve → medium
    0.3f,  // 5: layer2 weight → subtle
    0.4f,  // 6: layer2 arm count
    0.2f,  // 7: layer2 counter-rotation → slow
    0.2f,  // 8: organic layer weight → subtle
    0.3f,  // 9: organic frequency
    0.4f,  // 10: colour spread
    0.6f,  // 11: saturation → good colour
    0.05f, // 12: brightness floor → very low
    0.3f,  // 13: symmetry
    0.3f,  // 14: time warp
    0.1f   // 15: chaos → LOW to avoid flicker
  };
  for(int i=0;i<16;i++){
    evo[i].value    = seeds[i] + (random(20)-10)*0.01f;
    evo[i].value    = constrain(evo[i].value, 0.05f, 0.95f);
    evo[i].target   = evo[i].value + (random(30)-15)*0.01f;
    evo[i].target   = constrain(evo[i].target, 0.05f, 0.95f);
    evo[i].baseRate = 0.0001f + i*0.00001f; // very slow drift
    evo[i].rate     = evo[i].baseRate;
    evo[i].memory   = evo[i].value;
  }
  // Force chaos gene to stay low
  evo[15].value  = 0.05f;
  evo[15].target = 0.08f;
  evo[15].baseRate = 0.00005f;

  evoMood         = 0.2f;
  evoMoodTarget   = 0.3f;
  evoPalette      = random(255);
  evoLastMutate   = t;
  evoNextMutate   = 6000 + random(8000);
  evoLastT        = t;
  evoInit         = true;
}

void evoStep(uint32_t t) {
  // Use real wall-clock delta, clamped to sane range
  float dt = constrain((float)(t - evoLastT), 1.0f, 50.0f);
  evoLastT = t;

  // Update rates with gentle coupling
  for(int i=0;i<16;i++){
    float coupling = 1.0f + evo[(i+2)%16].value*0.5f - evo[(i+5)%16].value*0.3f;
    coupling = constrain(coupling, 0.3f, 2.5f);
    evo[i].rate = evo[i].baseRate * coupling;
  }

  // Drift genes toward targets
  for(int i=0;i<16;i++){
    float diff = evo[i].target - evo[i].value;
    evo[i].value += diff * evo[i].rate * dt;
    evo[i].memory = evo[i].memory*0.9995f + evo[i].value*0.0005f;
    // New target when close — biased away from memory
    if(fabsf(diff) < 0.003f){
      float away = 1.0f - evo[i].memory;
      evo[i].target = constrain(away*0.4f + random(40)*0.01f - 0.2f*evo[i].memory, 0.05f, 0.95f);
      // Gene 15 (chaos) stays locked low
      if(i==15) evo[i].target = 0.03f + random(8)*0.01f;
      // Gene 12 (floor) stays low
      if(i==12) evo[i].target = 0.02f + random(5)*0.01f;
    }
  }

  // Mood drift — very slow
  evoMood += (evoMoodTarget - evoMood) * 0.00005f * dt;
  if(fabsf(evoMoodTarget - evoMood) < 0.02f)
    evoMoodTarget = constrain(random(60)*0.01f, 0.0f, 0.6f); // cap at 0.6, never fully chaotic

  // Palette: continuous slow hue drift + occasional jump
  evoPalette += evoPaletteSpd * dt;
  if(evoPalette > 255.0f) evoPalette -= 255.0f;

  // Mutations: affect 1-2 genes, never chaos/floor genes
  if(t - evoLastMutate > evoNextMutate){
    int n = 1 + random(2);
    for(int m=0;m<n;m++){
      int g = random(14); // skip 14,15
      evo[g].target = constrain(random(80)*0.01f + 0.05f, 0.05f, 0.85f);
    }
    // Also occasionally shift palette speed
    evoPaletteSpd = 0.001f + random(6)*0.001f;
    evoLastMutate  = t;
    evoNextMutate  = 5000 + random(10000);
  }
}

CRGB evoColour(float x, float y, uint32_t t) {
  float tf   = t * 0.001f;
  float dist = sqrtf(x*x + y*y);           // unavoidable
  float nD   = dist * (1.0f/MAX_DIST);
  if(nD > 1.0f) nD = 1.0f;
  float angle = atan2f(y, x);              // unavoidable for spiral

  float armCount  = 2.0f  + evo[0].value * 6.0f;
  float rotSpd    = 0.04f + evo[1].value * 0.35f;
  float tight     = 0.04f + evo[2].value * 0.18f;
  float halfWidth = 0.25f + evo[3].value * 0.45f;
  float briExp    = 1.2f  + evo[4].value * 2.5f;
  float l2w       = evo[5].value * 0.5f;
  float arm2      = 1.0f  + evo[6].value * 4.0f;
  float rot2      = -(0.02f + evo[7].value * 0.2f);
  float l3w       = evo[8].value * 0.35f;
  float oFreq     = 0.8f  + evo[9].value * 2.5f;
  float colSpread = 20.0f + evo[10].value * 60.0f;
  float satBase   = 180.0f+ evo[11].value * 75.0f;
  float briFloor  = evo[12].value * 0.08f;
  float timeWarp  = evo[14].value * 0.4f;
  float chaos     = evo[15].value * 0.10f;

  float sAngle = angle;
  if(evo[13].value > 0.6f) sAngle = fabsf(fmod(angle + M_PI, M_PI)) - M_PI*0.5f;

  float lt = tf * (1.0f + timeWarp*(nD - 0.5f));

  // Fast edge fade: parabola approximation of sinf(nD*PI)
  // 4*x*(1-x) matches sine within ~1.5% — no trig needed
  float edgeFade = 4.0f * nD * (1.0f - nD);

  // ── Layer 1: primary spiral ──
  // Fast fmod: subtract integer part
  float sp1 = sAngle*armCount*(1.0f/(2.0f*M_PI)) - dist*tight - lt*rotSpd;
  sp1 -= (int)sp1; if(sp1<0.0f) sp1+=1.0f;
  float ad1 = sp1 < 0.5f ? sp1 : 1.0f - sp1;
  float t1  = 1.0f - ad1/halfWidth;
  // Fast power: replace powf with integer multiplies
  // briExp is 1.2..3.7 — approximate with x*x or x*x*x
  float bri1 = 0.0f;
  if(ad1 < halfWidth) {
    float p = t1 * t1;                         // x^2 base
    if(briExp > 2.5f) p *= t1;                 // x^3 for high exp
    if(briExp > 3.5f) p *= t1;                 // x^4 for very high
    bri1 = p * edgeFade;
  }

  // ── Layer 2: counter-rotating ──
  float sp2 = sAngle*arm2*(1.0f/(2.0f*M_PI)) - dist*tight*0.6f - lt*rot2;
  sp2 -= (int)sp2; if(sp2<0.0f) sp2+=1.0f;
  float ad2 = sp2 < 0.5f ? sp2 : 1.0f - sp2;
  float bri2 = 0.0f;
  if(ad2 < 0.35f) {
    float t2 = 1.0f - ad2/0.35f;
    bri2 = t2*t2 * l2w * edgeFade;             // always x^2 for layer 2
  }

  // ── Layer 3: organic — triangle waves instead of sinf ──
  // Triangle wave: cheap, no trig, visually similar feel
  float ox = x*oFreq*0.07f + tf*0.15f;
  float oy = y*oFreq*0.06f - tf*0.12f;
  ox -= (int)ox; if(ox<0.0f)ox+=1.0f;  // fmod 1
  oy -= (int)oy; if(oy<0.0f)oy+=1.0f;
  float tx = ox<0.5f ? ox*2.0f : 2.0f-ox*2.0f;  // triangle 0..1
  float ty = oy<0.5f ? oy*2.0f : 2.0f-oy*2.0f;
  float bri3 = tx * ty * l3w * edgeFade;

  // ── Noise: hash-style, zero trig ──
  float nx = x*0.031f + y*0.019f + tf*0.04f;
  nx -= (int)nx; if(nx<0.0f)nx+=1.0f;
  float noise = (nx<0.5f ? nx*2.0f : 2.0f-nx*2.0f) * chaos;

  // ── Combine ──
  float total = briFloor
              + bri1*(1.0f - evoMood*0.4f)
              + bri2*(1.0f - evoMood*0.2f)
              + bri3*evoMood
              + noise;
  if(total > 1.0f) total = 1.0f;
  if(total < 0.02f) return CRGB::Black;

  // ── Colour ──
  float hueShift = (bri1-bri2) * colSpread + nD * colSpread * 0.5f;
  uint8_t hue = (uint8_t)(evoPalette + hueShift);
  // Fast saturation mod: skip powf — use simple quadratic
  float sm = fabsf(total - 0.5f)*2.0f;
  uint8_t sat = (uint8_t)(satBase * (1.0f - sm*sm*0.4f));
  uint8_t bri = (uint8_t)(total * 215.0f);

  return CRGB(CHSV(hue, sat, bri));
}

void anim_evolution(uint32_t t) {
  if(!evoInit) evoInit_fn(t);
  evoStep(t);
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;setLED(true,xi+1,n,evoColour(x,y,t));}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;setLED(false,yi+1,n,evoColour(x,y,t));}}
  FastLED.show();
}

// ══════════════════════════════════════════════
// ANIMATION 56: MORPH
// Multiple fundamentally different visual topologies computed
// simultaneously and blended with slowly drifting weights.
// No hard switches — everything flows into everything else.
//
// 6 base topologies:
//   A: spiral arms      B: concentric rings
//   C: linear waves     D: standing interference
//   E: rotating sectors F: organic blobs
//
// Weights for all 6 drift independently and slowly.
// Mutations every 8-20s shift targets dramatically.
// ══════════════════════════════════════════════

struct MorphWeight { float w,target,speed,mem; };
struct MorphParam  { float v,target,speed; };

static MorphWeight mw[6];
static MorphParam  mp[12];
static float morphPal    = 0.0f;
static float morphPalSpd = 0.004f;
static uint32_t morphLastT      = 0;
static uint32_t morphLastMutate = 0;
static uint32_t morphNextMutate = 0;
static bool morphInit = false;

void morphInit_fn(uint32_t t) {
  float wSp[6]={0.00003f,0.00003f,0.00004f,0.00003f,0.00004f,0.00003f};
  // Start with ONE dominant topology, all others near zero
  int dominant = random(6);
  for(int i=0;i<6;i++){
    mw[i].w      = (i==dominant) ? 0.7f+random(20)*0.01f : 0.02f+random(6)*0.01f;
    mw[i].target = mw[i].w;
    mw[i].speed  = wSp[i];
    mw[i].mem    = mw[i].w;
  }
  float pSp[12]={0.00008f,0.00006f,0.00005f,0.00010f,0.00009f,0.00006f,0.00008f,0.00010f,0.00007f,0.00008f,0.00005f,0.00004f};
  float pIn[12]={0.25f,0.3f,0.4f,0.35f,0.3f,0.5f,0.4f,0.2f,0.25f,0.3f,0.65f,0.03f};
  for(int i=0;i<12;i++){
    mp[i].v      = pIn[i]+(random(20)-10)*0.01f;
    mp[i].target = mp[i].v;
    mp[i].speed  = pSp[i];
  }
  morphPal     = random(255);
  morphPalSpd  = 0.0008f;  // much slower palette drift
  morphLastT   = t;
  morphLastMutate = t;
  morphNextMutate = 10000+random(10000); // first mutation after 10-20s
  morphInit    = true;
}

void morphStep(uint32_t t) {
  float dt=constrain((float)(t-morphLastT),1.0f,50.0f);morphLastT=t;
  for(int i=0;i<6;i++){
    mw[i].w+=(mw[i].target-mw[i].w)*mw[i].speed*dt;
    mw[i].mem=mw[i].mem*0.9997f+mw[i].w*0.0003f;
    if(fabsf(mw[i].target-mw[i].w)<0.005f){float away=1.0f-mw[i].mem;mw[i].target=constrain(away*(0.1f+random(50)*0.01f),0.0f,0.9f);}
    mw[i].w=constrain(mw[i].w,0.0f,1.0f);
  }
  for(int i=0;i<12;i++){
    mp[i].v+=(mp[i].target-mp[i].v)*mp[i].speed*dt;mp[i].v=constrain(mp[i].v,0.02f,0.98f);
    if(fabsf(mp[i].target-mp[i].v)<0.004f)mp[i].target=constrain(0.05f+random(80)*0.01f,0.05f,0.92f);
  }
  morphPal+=morphPalSpd*dt;if(morphPal>255.0f)morphPal-=255.0f;
  if(t-morphLastMutate>morphNextMutate){
    // Pick a NEW dominant topology — different from current dominant
    int curDom=0; float curMax=0.0f;
    for(int i=0;i<6;i++) if(mw[i].w>curMax){curMax=mw[i].w;curDom=i;}
    int newDom=curDom;
    while(newDom==curDom) newDom=random(6);

    // Sink current dominant, boost new one, sink all others
    for(int i=0;i<6;i++){
      if(i==newDom)      mw[i].target=0.65f+random(25)*0.01f;  // new dominant
      else if(i==curDom) mw[i].target=0.02f+random(5)*0.01f;   // sink old
      else               mw[i].target=0.02f+random(8)*0.01f;   // keep others low
    }
    // Also mutate 1-2 parameters for variety within the new topology
    int p1=random(10); mp[p1].target=constrain(0.1f+random(75)*0.01f,0.1f,0.88f);
    if(random(2)) { int p2=random(10); mp[p2].target=constrain(0.1f+random(75)*0.01f,0.1f,0.88f); }

    // Slow palette speed changes only — no hue jumps
    morphPalSpd=0.0005f+random(15)*0.0001f;

    morphLastMutate=t;
    morphNextMutate=12000+random(10000); // next change in 12-22s
  }
}

// Precomputed blob centres updated once per frame (not per LED!)
static float morphC1x=0,morphC1y=0,morphC2x=0,morphC2y=0;

CRGB morphColour(float x, float y, uint32_t t) {
  float tf=t*0.001f;

  // dist and nD
  float dist=sqrtf(x*x+y*y);
  float nD=dist*(1.0f/MAX_DIST); if(nD>1.0f)nD=1.0f;

  // Fast edge fade — parabola instead of sinf(nD*PI)
  float ef=4.0f*nD*(1.0f-nD);

  float rotSpd=0.03f+mp[0].v*0.4f;
  float tight =0.03f+mp[1].v*0.18f;
  float arms  =2.0f +mp[2].v*6.0f;
  float freqX =0.5f +mp[3].v*2.5f;
  float freqY =0.5f +mp[4].v*2.5f;
  float briExp=1.5f +mp[5].v*2.0f;  // 1.5..3.5
  float hueSpr=20.0f+mp[6].v*70.0f;
  float rot2  =-(0.02f+mp[7].v*0.25f);
  float twarp =mp[8].v*0.35f;
  float satBase=165.0f+mp[10].v*80.0f;
  // briFlr kept very low — main brightness fix
  float briFlr=mp[11].v*0.03f;      // max 3% floor

  float lt=tf*(1.0f+twarp*(nD-0.5f)*2.0f);

  // ── A: Spiral — needs angle, unavoidable ──
  float angle=atan2f(y,x);
  float hw=0.18f+mp[2].v*0.25f;
  float sp=angle*arms*(1.0f/(2.0f*M_PI))-dist*tight-lt*rotSpd;
  sp-=(int)sp; if(sp<0.0f)sp+=1.0f;
  float ad=sp<0.5f?sp:1.0f-sp;
  float briA=0.0f;
  if(ad<hw){ float tv=1.0f-ad/hw; briA=tv*tv*tv*ef; }  // x^3, no powf

  // ── B: Rings — no angle needed ──
  float rFreq=1.5f+mp[2].v*3.5f;
  float ring=nD*rFreq-lt*rotSpd*0.5f;
  ring-=(int)ring; if(ring<0.0f)ring+=1.0f;
  float rd=ring<0.5f?ring:1.0f-ring;
  float tv=1.0f-rd*2.0f; if(tv<0.0f)tv=0.0f;
  float briB=tv*tv*tv*0.85f;        // x^3, no powf, no sinf

  // ── C: Linear waves — triangle waves, no sinf ──
  float wx=x*freqX*0.08f+lt*rotSpd*0.6f;
  float wy=y*freqY*0.07f-lt*rotSpd*0.5f;
  wx-=(int)wx; if(wx<0.0f)wx+=1.0f;
  wy-=(int)wy; if(wy<0.0f)wy+=1.0f;
  float twX=wx<0.5f?wx*2.0f:2.0f-wx*2.0f;
  float twY=wy<0.5f?wy*2.0f:2.0f-wy*2.0f;
  float briC=(twX*0.7f+twY*0.3f)*ef;

  // ── D: Interference — triangle waves only ──
  float ix=x*freqX*0.09f+lt*rotSpd*0.35f;
  float iy=y*freqY*0.09f-lt*rotSpd*0.3f;
  ix-=(int)ix; if(ix<0.0f)ix+=1.0f;
  iy-=(int)iy; if(iy<0.0f)iy+=1.0f;
  float tix=ix<0.5f?ix*2.0f:2.0f-ix*2.0f;
  float tiy=iy<0.5f?iy*2.0f:2.0f-iy*2.0f;
  float briD=tix*tiy*ef;

  // ── E: Sectors — reuse angle ──
  float secA=angle-lt*rotSpd+M_PI;
  secA-=(int)(secA/(2.0f*M_PI))*(2.0f*M_PI); if(secA<0.0f)secA+=2.0f*M_PI;
  float secP=secA/(2.0f*M_PI)*(arms*0.5f);
  secP-=(int)secP;
  float secD=secP<0.5f?secP:1.0f-secP;
  float briE=(secD<0.28f)?(1.0f-secD/0.28f)*(1.0f-secD/0.28f)*ef:0.0f;

  // ── F: Blobs — replace expf with cheap 1/(1+d) falloff ──
  // Centres precomputed once per frame in anim_morph
  float dx1=x-morphC1x, dy1=y-morphC1y;
  float dx2=x-morphC2x, dy2=y-morphC2y;
  float d1sq=dx1*dx1+dy1*dy1;
  float d2sq=dx2*dx2+dy2*dy2;
  float blobScl=1.5f+mp[9].v*3.0f;
  // Soft falloff without expf: 1/(1 + k*d^2)
  float briF=(1.0f/(1.0f+d1sq*blobScl*0.0006f))*0.7f
            +(1.0f/(1.0f+d2sq*blobScl*0.0005f))*0.55f;
  if(briF>1.0f)briF=1.0f;

  // ── Blend — scale each layer weight down so sum stays ≤ 1 ──
  // Max weight per layer 0.9, divide by ~3 expected active layers
  float total = briFlr
    + briA * mw[0].w * 0.7f
    + briB * mw[1].w * 0.7f
    + briC * mw[2].w * 0.6f
    + briD * mw[3].w * 0.6f
    + briE * mw[4].w * 0.7f
    + briF * mw[5].w * 0.5f;
  if(total>1.0f)total=1.0f;
  if(total<0.015f)return CRGB::Black;

  // ── Colour ──
  float wSum=mw[0].w+mw[1].w+mw[2].w+mw[3].w+mw[4].w+mw[5].w+0.001f;
  float hC=(briA*mw[0].w*(angle*(1.0f/(2.0f*M_PI))*hueSpr)
           +briB*mw[1].w*(nD*hueSpr)
           +briC*mw[2].w*(twX*hueSpr*0.5f)
           +briD*mw[3].w*(tix*tiy*hueSpr*0.7f)
           +briE*mw[4].w*(secP*hueSpr)
           +briF*mw[5].w*(d1sq/(MAX_DIST*MAX_DIST*4.0f)*hueSpr))/wSum;

  uint8_t hue=(uint8_t)(morphPal+hC+mp[6].v*25.0f);
  // Saturation moderate — not too high to avoid garish
  uint8_t sat=(uint8_t)(satBase*(0.7f+total*0.3f));
  uint8_t bri=(uint8_t)(total*205.0f);  // cap at 205, not 255

  return CRGB(CHSV(hue,sat,bri));
}

void anim_morph(uint32_t t) {
  if(!morphInit)morphInit_fn(t);
  morphStep(t);

  // Precompute blob centres ONCE per frame (not per LED)
  float lt=t*0.001f;
  morphC1x=sinf(lt*0.13f)*40.0f; morphC1y=cosf(lt*0.09f)*40.0f;
  morphC2x=sinf(lt*0.11f+2.0f)*35.0f; morphC2y=cosf(lt*0.14f+1.0f)*35.0f;

  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;setLED(true,xi+1,n,morphColour(x,y,t));}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;setLED(false,yi+1,n,morphColour(x,y,t));}}
  FastLED.show();
}

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
static float wvU[WV_W][WV_H];     // displacement
static float wvV[WV_W][WV_H];     // velocity
static bool  wvInit=false;
static uint32_t wvLastStep=0;
static uint32_t wvNextDrop=0;
static float wvDamping=0.998f;    // slowly drifts
static float wvDampingT=0.998f;
static float wvSpeed=0.35f;       // wave speed, drifts
static float wvSpeedT=0.35f;

void wvInitFn(){
  for(int x=0;x<WV_W;x++)for(int y=0;y<WV_H;y++){wvU[x][y]=0.0f;wvV[x][y]=0.0f;}
  wvInit=true;wvLastStep=millis();wvNextDrop=millis()+500;
}

void wvDrop(int cx,int cy,float amplitude){
  // Drop a stone — disturb a region
  for(int dx=-3;dx<=3;dx++)for(int dy=-3;dy<=3;dy++){
    int x=constrain(cx+dx,0,WV_W-1);int y=constrain(cy+dy,0,WV_H-1);
    float d=sqrtf((float)(dx*dx+dy*dy));
    wvU[x][y]+=amplitude*(1.0f-d/4.0f);
  }
}

void wvStep(){
  float c2=wvSpeed*wvSpeed;
  for(int x=1;x<WV_W-1;x++)for(int y=1;y<WV_H-1;y++){
    float laplacian=wvU[x+1][y]+wvU[x-1][y]+wvU[x][y+1]+wvU[x][y-1]-4.0f*wvU[x][y];
    wvV[x][y]=(wvV[x][y]+c2*laplacian)*wvDamping;
  }
  for(int x=0;x<WV_W;x++)for(int y=0;y<WV_H;y++){
    wvU[x][y]+=wvV[x][y];
    wvU[x][y]=constrain(wvU[x][y],-1.5f,1.5f);
  }
  // Reflecting boundaries
  for(int x=0;x<WV_W;x++){wvU[x][0]=wvU[x][1];wvU[x][WV_H-1]=wvU[x][WV_H-2];}
  for(int y=0;y<WV_H;y++){wvU[0][y]=wvU[1][y];wvU[WV_W-1][y]=wvU[WV_W-2][y];}
  // Parameter drift
  wvDamping+=(wvDampingT-wvDamping)*0.001f;
  wvSpeed  +=(wvSpeedT  -wvSpeed  )*0.001f;
}

void anim_wavePhysics(uint32_t t){
  if(!wvInit)wvInitFn();
  // Drop stones at random intervals
  if(t>wvNextDrop){
    int cx=3+random(WV_W-6);int cy=3+random(WV_H-6);
    float amp=0.4f+random(60)*0.01f;
    wvDrop(cx,cy,amp);
    // Multiple simultaneous drops occasionally
    if(random(3)==0){wvDrop(3+random(WV_W-6),3+random(WV_H-6),0.3f+random(40)*0.01f);}
    wvNextDrop=t+1500+random(3000);
    // Also drift parameters at each drop
    wvDampingT=0.990f+random(9)*0.001f;  // 0.990..0.999
    wvSpeedT  =0.25f+random(20)*0.01f;   // 0.25..0.45
  }
  // Run multiple physics steps per frame
  if(t-wvLastStep>15){for(int i=0;i<4;i++)wvStep();wvLastStep=t;}

  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    // Map to grid coords
    float gx=(x+MAX_DIST)/(2.0f*MAX_DIST)*(WV_W-1);
    float gy=(y+MAX_DIST)/(2.0f*MAX_DIST)*(WV_H-1);
    int ix=constrain((int)gx,0,WV_W-2);int iy=constrain((int)gy,0,WV_H-2);
    float fx=gx-ix;float fy=gy-iy;
    float u=wvU[ix][iy]*(1-fx)*(1-fy)+wvU[ix+1][iy]*fx*(1-fy)+wvU[ix][iy+1]*(1-fx)*fy+wvU[ix+1][iy+1]*fx*fy;
    // u is -1.5..1.5 — map to brightness and colour
    float bri=u*u*0.6f;  // both + and - waves are bright
    bri=constrain(bri,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*230.0f);
    // Positive displacement = warm, negative = cool
    uint8_t hue=u>0?(uint8_t)(40.0f-u*30.0f):(uint8_t)(160.0f+u*(-40.0f));
    uint8_t sat=(uint8_t)(180.0f+bri*75.0f);
    setLED(true,xi+1,n,b>4?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float gx=(x+MAX_DIST)/(2.0f*MAX_DIST)*(WV_W-1);float gy=(y+MAX_DIST)/(2.0f*MAX_DIST)*(WV_H-1);
    int ix=constrain((int)gx,0,WV_W-2);int iy=constrain((int)gy,0,WV_H-2);float fx=gx-ix;float fy=gy-iy;
    float u=wvU[ix][iy]*(1-fx)*(1-fy)+wvU[ix+1][iy]*fx*(1-fy)+wvU[ix][iy+1]*(1-fx)*fy+wvU[ix+1][iy+1]*fx*fy;
    float bri=constrain(u*u*0.6f,0.0f,1.0f);uint8_t b=(uint8_t)(bri*230.0f);
    uint8_t hue=u>0?(uint8_t)(40.0f-u*30.0f):(uint8_t)(160.0f+u*(-40.0f));
    setLED(false,yi+1,n,b>4?CRGB(CHSV(hue,(uint8_t)(180.0f+bri*75.0f),b)):CRGB::Black);
  }}
  FastLED.show();
}

void anim_reactionDiffusion(uint32_t t){ anim_wavePhysics(t); }

// ══════════════════════════════════════════════
// ANIMATION 58: STRANGE ATTRACTOR
// Lorenz attractor runs in 3D. Trajectory projected
// onto strips as a glowing trail. Parameter nudges
// slowly change the attractor shape over time.
// ══════════════════════════════════════════════

static float lorX=0.1f,lorY=0.0f,lorZ=0.0f;
static float lorSigma=10.0f,lorRho=28.0f,lorBeta=2.667f;
static float lorSigmaT=10.0f,lorRhoT=28.0f;
// Trail: store recent positions
#define LOR_TRAIL 80
static float lorTX[LOR_TRAIL],lorTY[LOR_TRAIL],lorTZ[LOR_TRAIL];
static int lorTIdx=0;
static bool lorInit=false;
static uint32_t lorLastMutate=0;

void anim_strangeAttractor(uint32_t t,float dt){
  if(!lorInit){
    lorX=0.1f+random(20)*0.1f;lorY=random(20)*0.1f;lorZ=random(20)*0.1f;
    for(int i=0;i<LOR_TRAIL;i++){lorTX[i]=lorX;lorTY[i]=lorY;lorTZ[i]=lorZ;}
    lorInit=true;lorLastMutate=t;
  }
  // Run attractor steps (fast, just float math)
  float dtS=dt*0.001f*8.0f; // speed up time
  int steps=5;
  for(int s=0;s<steps;s++){
    float dx=lorSigma*(lorY-lorX);
    float dy=lorX*(lorRho-lorZ)-lorY;
    float dz=lorX*lorY-lorBeta*lorZ;
    lorX+=dx*dtS/steps; lorY+=dy*dtS/steps; lorZ+=dz*dtS/steps;
    // Prevent explosion
    lorX=constrain(lorX,-40.0f,40.0f);lorY=constrain(lorY,-40.0f,40.0f);lorZ=constrain(lorZ,0.0f,60.0f);
  }
  lorTX[lorTIdx]=lorX; lorTY[lorTIdx]=lorY; lorTZ[lorTIdx]=lorZ;
  lorTIdx=(lorTIdx+1)%LOR_TRAIL;

  // Drift parameters slowly — changes attractor shape
  lorSigma+=(lorSigmaT-lorSigma)*0.0001f*dt;
  lorRho  +=(lorRhoT  -lorRho  )*0.0001f*dt;
  if(t-lorLastMutate>15000+random(10000)){
    lorSigmaT=8.0f+random(8)*0.5f;   // 8..12
    lorRhoT  =20.0f+random(20)*0.5f; // 20..30
    lorLastMutate=t;
  }

  // Render trail onto strips
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  for(int i=0;i<LOR_TRAIL;i++){
    int idx=(lorTIdx+i)%LOR_TRAIL;
    float age=(float)i/LOR_TRAIL; // 0=oldest, 1=newest
    float px=lorTX[idx]*1.1f;    // X position → X strip space (-51..51)
    float py=lorTY[idx]*1.1f;    // Y position → LED position
    // pz drives colour
    float pz=lorTZ[idx]/60.0f;   // 0..1

    uint8_t bri=(uint8_t)(age*age*200.0f);
    uint8_t hue=(uint8_t)(pz*180.0f+age*40.0f);

    // Project onto nearest X strip
    for(int xi=0;xi<12;xi++){
      float d=fabsf(X_POS[xi]-px);
      if(d<8.0f){
        int led=(int)(py+CENTER); if(led<1||led>121)continue;
        uint8_t b2=(uint8_t)(bri*(1.0f-d/8.0f));
        setLED(true,xi+1,led,b2>5?CRGB(CHSV(hue,220,b2)):CRGB::Black);
      }
    }
    // Project onto nearest Y strip
    for(int yi=0;yi<12;yi++){
      float d=fabsf(Y_POS[yi]-py);
      if(d<8.0f){
        int led=(int)(px+CENTER); if(led<1||led>121)continue;
        uint8_t b2=(uint8_t)(bri*(1.0f-d/8.0f));
        setLED(false,yi+1,led,b2>5?CRGB(CHSV(hue,220,b2)):CRGB::Black);
      }
    }
  }
  FastLED.show();
}

// ══════════════════════════════════════════════
// ANIMATION 59: HARMONIC RESONANCE
// Multiple oscillators with drifting frequencies.
// When frequencies align → constructive interference → bright.
// When they diverge → destructive → dark.
// The system breathes between resonance and silence.
// ══════════════════════════════════════════════

#define HR_OSC 5
static float hrFreq[HR_OSC];     // current frequencies
static float hrFreqT[HR_OSC];    // target frequencies
static float hrPhase[HR_OSC];    // running phase accumulators
static float hrAmp[HR_OSC];      // amplitude (drifts too)
static float hrAmpT[HR_OSC];
static bool  hrInit=false;

void anim_harmonicResonance(uint32_t t, float dt){
  if(!hrInit){
    for(int i=0;i<HR_OSC;i++){
      hrFreq[i] =0.5f+i*0.6f+random(30)*0.01f;
      hrFreqT[i]=hrFreq[i];
      hrPhase[i]=random(628)*0.01f;
      hrAmp[i]  =0.5f+random(50)*0.01f;
      hrAmpT[i] =hrAmp[i];
    }
    hrInit=true;
  }
  float dtS=dt*0.001f;

  for(int i=0;i<HR_OSC;i++){
    hrPhase[i]+=hrFreq[i]*dtS;
    if(hrPhase[i]>2.0f*M_PI)hrPhase[i]-=2.0f*M_PI;
    hrFreq[i]+=(hrFreqT[i]-hrFreq[i])*0.0002f*dt;
    hrAmp[i] +=(hrAmpT[i] -hrAmp[i] )*0.00015f*dt;

    if(fabsf(hrFreqT[i]-hrFreq[i])<0.008f){
      // Prevent lock-in: always pick a NEW unique target, not based on other oscillators
      // Use irrational multiples to prevent harmonic lock
      float base=0.3f+random(50)*0.04f;
      float mult=1.0f+(float)(i*7+random(5))*0.13f; // incommensurable ratios
      hrFreqT[i]=constrain(base*mult,0.1f,8.0f);
      hrAmpT[i]=0.4f+random(60)*0.01f;
    }
  }

  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;
    float sum=0.0f;
    for(int i=0;i<HR_OSC;i++){
      float spatial;
      switch(i%5){
        case 0: spatial=sinf(nX*(hrFreq[i]*2.5f)+hrPhase[i]); break;
        case 1: spatial=sinf(nY*(hrFreq[i]*2.5f)+hrPhase[i]); break;
        case 2: spatial=sinf((nX+nY)*(hrFreq[i]*1.8f)+hrPhase[i]); break;
        case 3: spatial=sinf(sqrtf(nX*nX+nY*nY)*hrFreq[i]*4.0f-hrPhase[i]); break;
        default:spatial=sinf((nX*nX-nY*nY)*(hrFreq[i]*2.0f)+hrPhase[i]); break;
      }
      sum+=spatial*hrAmp[i];
    }
    sum=(sum+3.5f)/7.0f;
    // Power: bright peaks, dark troughs — more contrast
    float bri=sum*sum*sum;
    bri=constrain(bri,0.0f,1.0f);
    uint8_t b=(uint8_t)(bri*235.0f);
    // Colour: hue sweeps with sum value AND drifts over time
    uint8_t hue=(uint8_t)(sum*200.0f+tf*8.0f+nX*30.0f);
    uint8_t sat=(uint8_t)(190.0f+bri*65.0f);
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float nX=x/MAX_DIST;float nY=y/MAX_DIST;float sum=0.0f;
    for(int i=0;i<HR_OSC;i++){float spatial;switch(i%5){case 0:spatial=sinf(nX*(hrFreq[i]*2.5f)+hrPhase[i]);break;case 1:spatial=sinf(nY*(hrFreq[i]*2.5f)+hrPhase[i]);break;case 2:spatial=sinf((nX+nY)*(hrFreq[i]*1.8f)+hrPhase[i]);break;case 3:spatial=sinf(sqrtf(nX*nX+nY*nY)*hrFreq[i]*4.0f-hrPhase[i]);break;default:spatial=sinf((nX*nX-nY*nY)*(hrFreq[i]*2.0f)+hrPhase[i]);break;}sum+=spatial*hrAmp[i];}
    sum=(sum+3.5f)/7.0f;float bri=constrain(sum*sum*sum,0.0f,1.0f);uint8_t b=(uint8_t)(bri*235.0f);
    uint8_t hue=(uint8_t)(sum*200.0f+tf*8.0f+nY*30.0f);uint8_t sat=(uint8_t)(190.0f+bri*65.0f);
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

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
static uint8_t nfState[NF_W][NF_H];
static uint8_t nfTimer[NF_W][NF_H];   // time remaining in current state
static float   nfGlow[NF_W][NF_H];    // visual brightness (decays)
static bool    nfInit=false;
static uint32_t nfLastStep=0;
static uint8_t nfRefractTime=8;        // refractory period, drifts
static uint8_t nfFireTime=2;           // how long it fires
static float   nfExciteThresh=0.4f;   // fraction of neighbours needed to excite
static float   nfExciteT=0.4f;
static uint32_t nfLastParam=0;

void nfInitFn(){
  for(int x=0;x<NF_W;x++)for(int y=0;y<NF_H;y++){
    nfState[x][y]=NF_REST;nfTimer[x][y]=0;nfGlow[x][y]=0.0f;
  }
  // Seed a few random firing cells to start waves
  for(int i=0;i<5;i++){nfState[random(NF_W)][random(NF_H)]=NF_FIRED;nfTimer[random(NF_W)][random(NF_H)]=nfFireTime;}
  nfInit=true;nfLastStep=millis();nfLastParam=millis();
}

void nfStep(){
  static uint8_t nextState[NF_W][NF_H];
  static uint8_t nextTimer[NF_W][NF_H];
  for(int x=0;x<NF_W;x++)for(int y=0;y<NF_H;y++){
    nextState[x][y]=nfState[x][y];nextTimer[x][y]=nfTimer[x][y];
    if(nfState[x][y]==NF_FIRED){
      if(nfTimer[x][y]>0)nextTimer[x][y]--;
      else{nextState[x][y]=NF_REFRACT;nextTimer[x][y]=nfRefractTime;}
    } else if(nfState[x][y]==NF_REFRACT){
      if(nfTimer[x][y]>0)nextTimer[x][y]--;
      else nextState[x][y]=NF_REST;
    } else { // REST
      // Count fired neighbours
      int fired=0,total=0;
      for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++){
        if(dx==0&&dy==0)continue;
        int nx=(x+dx+NF_W)%NF_W;int ny=(y+dy+NF_H)%NF_H;
        if(nfState[nx][ny]==NF_FIRED)fired++;total++;
      }
      if(total>0&&(float)fired/total>=nfExciteThresh){
        nextState[x][y]=NF_FIRED;nextTimer[x][y]=nfFireTime;
      }
    }
    // Update glow
    nfGlow[x][y]*=0.75f;
    if(nfState[x][y]==NF_FIRED)nfGlow[x][y]=1.0f;
  }
  for(int x=0;x<NF_W;x++)for(int y=0;y<NF_H;y++){nfState[x][y]=nextState[x][y];nfTimer[x][y]=nextTimer[x][y];}
  // Random spontaneous firing to keep system alive
  if(random(60)==0){int x=random(NF_W);int y=random(NF_H);if(nfState[x][y]==NF_REST){nfState[x][y]=NF_FIRED;nfTimer[x][y]=nfFireTime;}}
}

void anim_neuralFire(uint32_t t){
  if(!nfInit)nfInitFn();
  // Parameter drift — shifts between wave regimes
  if(t-nfLastParam>8000+random(8000)){
    nfExciteT=0.2f+random(4)*0.1f;  // 0.2(easy excite)..0.5(hard)
    nfRefractTime=4+random(12);      // 4..16 steps
    nfLastParam=t;
  }
  nfExciteThresh+=(nfExciteT-nfExciteThresh)*0.005f;
  if(t-nfLastStep>35){nfStep();nfLastStep=t;}

  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    float gx=(x+MAX_DIST)/(2.0f*MAX_DIST)*(NF_W-1);float gy=(y+MAX_DIST)/(2.0f*MAX_DIST)*(NF_H-1);
    int ix=constrain((int)gx,0,NF_W-2);int iy=constrain((int)gy,0,NF_H-2);float fx=gx-ix;float fy=gy-iy;
    float g=nfGlow[ix][iy]*(1-fx)*(1-fy)+nfGlow[ix+1][iy]*fx*(1-fy)+nfGlow[ix][iy+1]*(1-fx)*fy+nfGlow[ix+1][iy+1]*fx*fy;
    // State colour: fired=white-blue, refractory=dark purple, rest=black
    float rState=(float)nfState[ix][iy]/2.0f;
    uint8_t b=(uint8_t)(g*230.0f);
    uint8_t hue=g>0.5f?150:(uint8_t)(200.0f+rState*30.0f);
    uint8_t sat=g>0.5f?(uint8_t)(255.0f-g*180.0f):240;
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float gx=(x+MAX_DIST)/(2.0f*MAX_DIST)*(NF_W-1);float gy=(y+MAX_DIST)/(2.0f*MAX_DIST)*(NF_H-1);
    int ix=constrain((int)gx,0,NF_W-2);int iy=constrain((int)gy,0,NF_H-2);float fx=gx-ix;float fy=gy-iy;
    float g=nfGlow[ix][iy]*(1-fx)*(1-fy)+nfGlow[ix+1][iy]*fx*(1-fy)+nfGlow[ix][iy+1]*(1-fx)*fy+nfGlow[ix+1][iy+1]*fx*fy;
    float rState=(float)nfState[ix][iy]/2.0f;uint8_t b=(uint8_t)(g*230.0f);
    uint8_t hue=g>0.5f?150:(uint8_t)(200.0f+rState*30.0f);uint8_t sat=g>0.5f?(uint8_t)(255.0f-g*180.0f):240;
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);
  }}
  FastLED.show();
}

void anim_continuousCA(uint32_t t){ anim_neuralFire(t); }

// ══════════════════════════════════════════════
// ANIMATION 61: PLASMA LIGHTNING
// Energy field builds up across the grid.
// When threshold exceeded, Lichtenberg-figure lightning
// branches through the strips — real branching discharge.
// Uses the arm strobe system from the beginning + energy field.
// ══════════════════════════════════════════════

#define PL_N 12
static float plEnergy[PL_N][PL_N];  // energy at each intersection
static float plGlow[PL_N][PL_N];    // afterglow
static bool  plInit=false;
static uint32_t plLastStep=0;
static float plChargeRate=0.003f;
static float plChargeRateT=0.003f;

// Lightning bolt: a branching path through the intersection grid
#define PL_BOLT_MAX 30
struct PlBolt {
  bool  active;
  int   pathX[PL_BOLT_MAX];
  int   pathY[PL_BOLT_MAX];
  int   pathLen;
  uint8_t ttl;     // frames to live
  uint8_t maxTTL;
  uint8_t hue;
};
#define PL_MAX_BOLTS 4
static PlBolt plBolts[PL_MAX_BOLTS];

void plInitFn(){
  for(int x=0;x<PL_N;x++)for(int y=0;y<PL_N;y++){plEnergy[x][y]=random(50)*0.01f;plGlow[x][y]=0.0f;}
  for(int i=0;i<PL_MAX_BOLTS;i++)plBolts[i].active=false;
  plInit=true;plLastStep=millis();
}

void plStrikeBolt(int startX,int startY){
  // Find free bolt slot
  for(int i=0;i<PL_MAX_BOLTS;i++){
    if(plBolts[i].active)continue;
    PlBolt& b=plBolts[i];
    b.active=true;b.pathLen=0;
    b.ttl=8+random(12);b.maxTTL=b.ttl;
    b.hue=140+random(40); // blue-white
    // Grow lightning bolt using random walk biased toward high energy
    int cx=startX,cy=startY;
    for(int step=0;step<PL_BOLT_MAX&&b.pathLen<PL_BOLT_MAX;step++){
      b.pathX[b.pathLen]=cx;b.pathY[b.pathLen]=cy;b.pathLen++;
      plEnergy[cx][cy]*=0.1f; // discharge along path
      plGlow[cx][cy]=1.0f;
      // Choose next cell: biased toward highest energy neighbour
      // with some randomness (50% random, 50% greedy)
      int nx=cx,ny=cy;
      if(random(2)){
        // Random step
        int dir=random(4);
        if(dir==0&&cx>0)nx=cx-1;
        else if(dir==1&&cx<PL_N-1)nx=cx+1;
        else if(dir==2&&cy>0)ny=cy-1;
        else if(cy<PL_N-1)ny=cy+1;
      } else {
        // Greedy: go to highest energy neighbour
        float best=-1.0f;
        if(cx>0&&plEnergy[cx-1][cy]>best){best=plEnergy[cx-1][cy];nx=cx-1;ny=cy;}
        if(cx<PL_N-1&&plEnergy[cx+1][cy]>best){best=plEnergy[cx+1][cy];nx=cx+1;ny=cy;}
        if(cy>0&&plEnergy[cx][cy-1]>best){best=plEnergy[cx][cy-1];nx=cx;ny=cy-1;}
        if(cy<PL_N-1&&plEnergy[cx][cy+1]>best){best=plEnergy[cx][cy+1];nx=cx;ny=cy+1;}
      }
      if(nx==cx&&ny==cy)break; // stuck
      cx=nx;cy=ny;
    }
    return;
  }
}

void plStep(float dt){
  float dtS=dt*0.001f;
  // Charge all cells slowly
  plChargeRate+=(plChargeRateT-plChargeRate)*0.001f*dt;
  if(fabsf(plChargeRateT-plChargeRate)<0.0005f)plChargeRateT=0.001f+random(6)*0.001f;

  for(int x=0;x<PL_N;x++)for(int y=0;y<PL_N;y++){
    plEnergy[x][y]+=plChargeRate*dtS*10.0f;
    plEnergy[x][y]=constrain(plEnergy[x][y],0.0f,1.0f);
    plGlow[x][y]*=max(0.0f,1.0f-4.0f*dtS);
    // Check discharge
    if(plEnergy[x][y]>0.7f+random(30)*0.01f){
      plStrikeBolt(x,y);
      plEnergy[x][y]*=0.05f;
    }
  }
  // Decay bolts
  for(int i=0;i<PL_MAX_BOLTS;i++){
    if(!plBolts[i].active)continue;
    if(plBolts[i].ttl>0)plBolts[i].ttl--;
    else plBolts[i].active=false;
  }
}

void anim_plasmaLightning(uint32_t t,float dt){
  if(!plInit)plInitFn();
  if(t-plLastStep>20){plStep(dt);plLastStep=t;}

  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}

  // Render energy field as dim background glow on strips
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;
    // Sample energy at nearest intersection
    float gx=(x+MAX_DIST)/(2.0f*MAX_DIST)*(PL_N-1);float gy=(y+MAX_DIST)/(2.0f*MAX_DIST)*(PL_N-1);
    int ix=constrain((int)(gx+0.5f),0,PL_N-1);int iy=constrain((int)(gy+0.5f),0,PL_N-1);
    float e=plEnergy[ix][iy];float g=plGlow[ix][iy];
    float bri=e*0.15f+g*0.4f; // energy=dim blue glow, discharge=bright
    uint8_t b=(uint8_t)(constrain(bri,0.0f,1.0f)*200.0f);
    uint8_t hue=g>0.2f?150:180;
    setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,(uint8_t)(255.0f-g*150.0f),b)):CRGB::Black);
  }}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;
    float gx=(x+MAX_DIST)/(2.0f*MAX_DIST)*(PL_N-1);float gy=(y+MAX_DIST)/(2.0f*MAX_DIST)*(PL_N-1);
    int ix=constrain((int)(gx+0.5f),0,PL_N-1);int iy=constrain((int)(gy+0.5f),0,PL_N-1);
    float e=plEnergy[ix][iy];float g=plGlow[ix][iy];
    float bri=e*0.15f+g*0.4f;uint8_t b=(uint8_t)(constrain(bri,0.0f,1.0f)*200.0f);
    uint8_t hue=g>0.2f?150:180;
    setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,(uint8_t)(255.0f-g*150.0f),b)):CRGB::Black);
  }}

  // Render active lightning bolts — bright flashes on the strips between intersections
  for(int i=0;i<PL_MAX_BOLTS;i++){
    if(!plBolts[i].active)continue;
    PlBolt& bolt=plBolts[i];
    float age=(float)bolt.ttl/(float)bolt.maxTTL; // 1=fresh 0=fading
    uint8_t boltBri=(uint8_t)(age*age*255.0f);

    // Draw each segment of the bolt path
    for(int s=0;s<bolt.pathLen-1;s++){
      int x1=bolt.pathX[s],y1=bolt.pathY[s];
      int x2=bolt.pathX[s+1],y2=bolt.pathY[s+1];
      float segAge=age*(1.0f-(float)s/bolt.pathLen*0.3f);
      uint8_t segBri=(uint8_t)(segAge*segAge*255.0f);

      // Horizontal segment: same row, different column → Y strip between them
      if(y1==y2){
        int col1=min(x1,x2);int col2=max(x1,x2);
        // Y strip at row y1+1
        int yStrip=y1+1;if(yStrip<1||yStrip>12)continue;
        int led1=INTS[col1];int led2=INTS[col2];
        for(int led=led1;led<=led2;led++)setLED(false,yStrip,led,CRGB(CHSV(bolt.hue,(uint8_t)(80.0f+segBri*0.7f),segBri)));
      }
      // Vertical segment: same column, different row → X strip between them
      if(x1==x2){
        int row1=min(y1,y2);int row2=max(y1,y2);
        int xStrip=x1+1;if(xStrip<1||xStrip>12)continue;
        int led1=INTS[row1];int led2=INTS[row2];
        for(int led=led1;led<=led2;led++)setLED(true,xStrip,led,CRGB(CHSV(bolt.hue,(uint8_t)(80.0f+segBri*0.7f),segBri)));
      }
    }
  }
  FastLED.show();
}

void anim_agentField(uint32_t t,float dt){ anim_plasmaLightning(t,dt); }

// ═══════════════════════════════════════════════════════
//  LED Grid Receiver — ESP-NOW from NeoTrellis Sender
//  Board: ESP32 (MAC: f4:65:0b:e8:98:60)
//  Sender NeoTrellis ESP32 MAC: 44:1d:64:fa:b0:f8
// ═══════════════════════════════════════════════════════

#include <esp_now.h>
#include <WiFi.h>

// ── Messages — must match sender ──────────────────────
typedef struct {
  uint16_t heldMask;
  uint8_t  eventBtn;
  bool     eventPressed;
  bool     isEvent;
} GridMsg;

// Receiver → Sender sync: current settings state
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

// ── Volatile state updated by ESP-NOW callback ────────
volatile uint16_t espHeldMask      = 0;
volatile uint32_t espLastHeartbeat = 0;
static uint8_t    senderMAC[6]     = {};
static bool       senderKnown      = false;
static bool       syncPeerAdded    = false;

void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  if(len != sizeof(GridMsg)) return;
  GridMsg msg;
  memcpy(&msg, data, sizeof(msg));
  espHeldMask      = msg.heldMask;
  espLastHeartbeat = millis();
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
  uint8_t brightness;
  uint8_t strobeBrightness;
  uint8_t strobeOnMs;
  uint8_t strobeOffMs;
  uint8_t strobeSquares;
  float   animSpeed;      // 0.1..4.0, 1.0=normal
};
Settings cfg;

void saveSettings() {
  prefs.begin("grid", false);
  prefs.putInt("anim",   cfg.animIndex);
  prefs.putUChar("bri",  cfg.brightness);
  prefs.putUChar("sbri", cfg.strobeBrightness);
  prefs.putUChar("son",  cfg.strobeOnMs);
  prefs.putUChar("soff", cfg.strobeOffMs);
  prefs.putUChar("ssq",  cfg.strobeSquares);
  prefs.putFloat("spd",  cfg.animSpeed);
  prefs.end();
}

void loadSettings() {
  prefs.begin("grid", true);
  cfg.animIndex        = prefs.getInt("anim",   0);
  cfg.brightness       = prefs.getUChar("bri",  40);
  cfg.strobeBrightness = prefs.getUChar("sbri", 200);
  cfg.strobeOnMs       = prefs.getUChar("son",  40);
  cfg.strobeOffMs      = prefs.getUChar("soff", 40);
  cfg.strobeSquares    = prefs.getUChar("ssq",  30);
  cfg.animSpeed        = prefs.getFloat("spd",  1.0f);
  prefs.end();
  cfg.animIndex        = constrain(cfg.animIndex, 0, 54);
  cfg.brightness       = constrain(cfg.brightness, 1, MAX_BRIGHTNESS);
  cfg.strobeBrightness = constrain(cfg.strobeBrightness, 1, 255);
  cfg.strobeOnMs       = constrain(cfg.strobeOnMs, 5, 200);
  cfg.strobeOffMs      = constrain(cfg.strobeOffMs, 5, 200);
  cfg.strobeSquares    = constrain(cfg.strobeSquares, 1, 122);
  cfg.animSpeed        = constrain(cfg.animSpeed, 0.1f, 4.0f);
}

// ═══════════════════════════════════════════════════════
//  EXTRA ANIMATION PACK (cases 39..50)
//  Reuses setLED / drawGridSquare. "Cell canvas" = 11x11
//  grid of squares (row,col 0..10); one cell == one grid
//  square outline, used as a low-res pixel for figures,
//  Tetris and other block-based effects.
// ═══════════════════════════════════════════════════════

static inline int iabs(int v){ return v<0?-v:v; }
static inline float frand(){ return random(1001)*0.001f; }

// Light one cell of the 11x11 canvas (row,col 0..10).
void fillCell(int row,int col,CRGB c){
  if(row<0||row>10||col<0||col>10) return;
  drawGridSquare(row+1,col+1,row+2,col+2,c);
}

// Lightweight per-frame clear (keeps spacer LEDs black).
void clearFrame(){
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){
    ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;
    ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;
  }
}

// ── Icons: 11 rows, 11 bits each (col0 = MSB / bit 10) ──
void drawIcon(const uint16_t rows[11],CRGB c){
  for(int r=0;r<11;r++)
    for(int col=0;col<11;col++)
      if(rows[r]&(1<<(10-col))) fillCell(r,col,c);
}
const uint16_t ICON_HEART[11]={
  0b00000000000,0b01100011000,0b11110111100,0b11111111110,0b11111111110,
  0b11111111110,0b01111111100,0b00111111000,0b00011110000,0b00001100000,0b00000000000};
const uint16_t ICON_SMILEY[11]={
  0b00011111000,0b00111111100,0b01111111110,0b01101110110,0b01101110110,
  0b01111111110,0b01111111110,0b01100000110,0b01111111110,0b00111111100,0b00011111000};
const uint16_t ICON_STAR[11]={
  0b00000100000,0b00000100000,0b00001110000,0b11111111111,0b01111111110,
  0b00111111100,0b00111111100,0b01110001110,0b01100000110,0b01000000010,0b00000000000};
const uint16_t ICON_ARROW[11]={
  0b00000100000,0b00001110000,0b00011111000,0b00111111100,0b01111111110,
  0b11101110111,0b00001110000,0b00001110000,0b00001110000,0b00001110000,0b00001110000};
const uint16_t ICON_INVADER[11]={
  0b00100000100,0b00010001000,0b00111111100,0b01101110110,0b11111111111,
  0b10111111101,0b10100000101,0b00011011000,0b00000000000,0b00000000000,0b00000000000};
const uint16_t ICON_NOTE[11]={
  0b00000011110,0b00000011110,0b00000010010,0b00000010010,0b00000010010,
  0b00000010010,0b00011010010,0b00111011100,0b00111011100,0b00011000000,0b00000000000};
const uint16_t* const ICONS[6]={ICON_HEART,ICON_SMILEY,ICON_STAR,ICON_ARROW,ICON_INVADER,ICON_NOTE};
const int NUM_ICONS=6;

// ═══════════ 39. TETRIS ═══════════
bool tetInit=false;
static uint8_t tetB[11][11];
static int8_t  tetP[4][2];
static int     tetR,tetC;
static uint8_t tetHue;
static uint32_t tetStep;
const int8_t TET_SHAPES[7][4][2]={
  {{0,-1},{0,0},{0,1},{0,2}}, // I
  {{0,0},{0,1},{1,0},{1,1}},  // O
  {{0,-1},{0,0},{0,1},{1,0}}, // T
  {{0,0},{0,1},{1,-1},{1,0}}, // S
  {{0,-1},{0,0},{1,0},{1,1}}, // Z
  {{0,-1},{0,0},{0,1},{1,1}}, // J
  {{0,-1},{0,0},{0,1},{1,-1}} // L
};
bool tetFits(int r,int c){
  for(int i=0;i<4;i++){
    int rr=r+tetP[i][0], cc=c+tetP[i][1];
    if(cc<0||cc>10||rr>10) return false;
    if(rr>=0 && tetB[rr][cc]) return false;
  }
  return true;
}
void tetNew(){
  int s=random(7);
  for(int i=0;i<4;i++){ tetP[i][0]=TET_SHAPES[s][i][0]; tetP[i][1]=TET_SHAPES[s][i][1]; }
  int rot=random(4);
  for(int k=0;k<rot;k++) for(int i=0;i<4;i++){ int a=tetP[i][0],b=tetP[i][1]; tetP[i][0]=b; tetP[i][1]=-a; }
  tetHue=random(256); if(tetHue==0) tetHue=1;
  tetR=0; tetC=3+random(5);
  if(!tetFits(tetR,tetC)) for(int d=-4;d<=4;d++) if(tetFits(0,tetC+d)){ tetC+=d; break; }
  if(!tetFits(tetR,tetC)) for(int a=0;a<11;a++) for(int b=0;b<11;b++) tetB[a][b]=0; // board full -> reset
}
void anim_tetris(uint32_t at,float dt){
  if(!tetInit){ for(int a=0;a<11;a++) for(int b=0;b<11;b++) tetB[a][b]=0; tetStep=at; tetNew(); tetInit=true; }
  if(at-tetStep>=180){
    tetStep=at;
    if(tetFits(tetR+1,tetC)) tetR++;
    else {
      for(int i=0;i<4;i++){ int rr=tetR+tetP[i][0], cc=tetC+tetP[i][1]; if(rr>=0&&rr<=10&&cc>=0&&cc<=10) tetB[rr][cc]=tetHue; }
      // clear full rows (top-down copy)
      for(int r=10;r>=0;r--){
        bool full=true; for(int c=0;c<11;c++) if(!tetB[r][c]) full=false;
        if(full){ for(int rr=r;rr>0;rr--) for(int c=0;c<11;c++) tetB[rr][c]=tetB[rr-1][c];
                  for(int c=0;c<11;c++) tetB[0][c]=0; r++; }
      }
      tetNew();
    }
  }
  clearFrame();
  for(int r=0;r<11;r++) for(int c=0;c<11;c++) if(tetB[r][c]) fillCell(r,c,CRGB(CHSV(tetB[r][c],220,160)));
  for(int i=0;i<4;i++){ int rr=tetR+tetP[i][0], cc=tetC+tetP[i][1]; if(rr>=0) fillCell(rr,cc,CRGB(CHSV(tetHue,255,235))); }
  FastLED.show();
}

// ═══════════ 40. SNAKE (self-playing) ═══════════
bool snakeInit=false;
static int8_t snR[121],snC[121];
static int    snLen,snDir;
static int8_t sfR,sfC;
static uint32_t snStep;
const int8_t SN_DR[4]={-1,0,1,0}, SN_DC[4]={0,1,0,-1};
bool snOccupied(int r,int c,int upto){
  for(int i=0;i<upto;i++) if(snR[i]==r&&snC[i]==c) return true;
  return false;
}
void snakeFood(){
  for(int tries=0;tries<200;tries++){
    int r=random(11),c=random(11);
    if(!snOccupied(r,c,snLen)){ sfR=r; sfC=c; return; }
  }
}
void snakeReset(){
  snLen=3; snDir=1;
  for(int i=0;i<snLen;i++){ snR[i]=5; snC[i]=5-i; }
  snakeFood();
}
void anim_snake(uint32_t at,float dt){
  if(!snakeInit){ snakeReset(); snStep=at; snakeInit=true; }
  if(at-snStep>=140){
    snStep=at;
    int best=-1,bestDist=999;
    for(int d=0;d<4;d++){
      if(d==(snDir+2)%4) continue;              // no reversing
      int nr=snR[0]+SN_DR[d], nc=snC[0]+SN_DC[d];
      if(nr<0||nr>10||nc<0||nc>10) continue;
      if(snOccupied(nr,nc,snLen-1)) continue;    // tail cell will vacate
      int dist=iabs(nr-sfR)+iabs(nc-sfC);
      if(dist<bestDist){ bestDist=dist; best=d; }
    }
    if(best<0){ snakeReset(); }
    else {
      snDir=best;
      int nr=snR[0]+SN_DR[best], nc=snC[0]+SN_DC[best];
      bool ate=(nr==sfR&&nc==sfC);
      if(ate&&snLen<120) snLen++;
      for(int i=snLen-1;i>0;i--){ snR[i]=snR[i-1]; snC[i]=snC[i-1]; }
      snR[0]=nr; snC[0]=nc;
      if(ate) snakeFood();
    }
  }
  clearFrame();
  fillCell(sfR,sfC,CRGB(CHSV(0,255,220)));                 // food = red
  for(int i=0;i<snLen;i++){
    uint8_t v=(i==0)?255:(uint8_t)(90+120.0f*(1.0f-(float)i/snLen));
    fillCell(snR[i],snC[i],CRGB(CHSV(96,i==0?160:220,v)));  // body = green
  }
  FastLED.show();
}

// ═══════════ 41. ICON SHOW ═══════════
void anim_iconShow(uint32_t at){
  const uint32_t per=2600;
  int idx=(at/per)%NUM_ICONS;
  uint32_t ph=at%per;
  float env = (ph<400)?ph/400.0f : (ph>per-500)?(per-ph)/500.0f : 1.0f;
  uint8_t val=(uint8_t)constrain(env*230.0f,0.0f,230.0f);
  uint8_t hue=(uint8_t)(at/22);
  clearFrame();
  if(val>4) drawIcon(ICONS[idx],CRGB(CHSV(hue,235,val)));
  FastLED.show();
}

// ═══════════ 42. BOUNCING LOGO ═══════════
bool logoInit=false;
static float logoX,logoY,logoVX,logoVY;
static uint8_t logoHue;
const uint8_t LOGO[5]={0b01010,0b11111,0b11111,0b01110,0b00100}; // 5x5 heart
void anim_bouncingLogo(uint32_t at,float dt){
  if(!logoInit){ logoX=3;logoY=2;logoVX=0.017f;logoVY=0.012f;logoHue=random(256);logoInit=true; }
  logoX+=logoVX*dt; logoY+=logoVY*dt;
  const int w=5;
  if(logoX<0){ logoX=0; logoVX=-logoVX; logoHue+=43; }
  if(logoX>11-w){ logoX=11-w; logoVX=-logoVX; logoHue+=43; }
  if(logoY<0){ logoY=0; logoVY=-logoVY; logoHue+=43; }
  if(logoY>11-w){ logoY=11-w; logoVY=-logoVY; logoHue+=43; }
  clearFrame();
  int ox=(int)(logoX+0.5f), oy=(int)(logoY+0.5f);
  for(int r=0;r<5;r++) for(int c=0;c<5;c++)
    if(LOGO[r]&(1<<(4-c))) fillCell(oy+r,ox+c,CRGB(CHSV(logoHue,255,215)));
  FastLED.show();
}

// ═══════════ 43. STAR TUNNEL 3D ═══════════
bool stunInit=false;
#define STUN_N 18
static float stA[STUN_N],stRad[STUN_N];
void anim_starTunnel3D(uint32_t at,float dt){
  if(!stunInit){ for(int i=0;i<STUN_N;i++){ stA[i]=frand()*6.2832f; stRad[i]=frand(); } stunInit=true; }
  clearFrame();
  for(int i=0;i<STUN_N;i++){
    stRad[i]+=dt*0.00016f*(stRad[i]*2.5f+0.25f);       // accelerate outward
    if(stRad[i]>1.05f){ stRad[i]=0.04f; stA[i]=frand()*6.2832f; }
    float rr=stRad[i], ca=cosf(stA[i]), sa=sinf(stA[i]);
    int cx=(int)(5.0f+ca*rr*5.6f+0.5f), cy=(int)(5.0f+sa*rr*5.6f+0.5f);
    uint8_t val=(uint8_t)(30+rr*225), hue=(uint8_t)(160+rr*80);
    int tx=(int)(5.0f+ca*(rr-0.13f)*5.6f+0.5f), ty=(int)(5.0f+sa*(rr-0.13f)*5.6f+0.5f);
    fillCell(ty,tx,CRGB(CHSV(hue,150,val/3)));           // dim trail
    fillCell(cy,cx,CRGB(CHSV(hue,120,val)));             // bright head
  }
  FastLED.show();
}

// ═══════════ 44. RIPPLE RAIN ═══════════
bool rripInit=false;
#define RRIP_N 7
static int8_t rpR[RRIP_N],rpC[RRIP_N];
static float  rpRad[RRIP_N];
static bool   rpAct[RRIP_N];
static uint8_t rpHue[RRIP_N];
static uint32_t rripSpawn;
void anim_rippleRain(uint32_t at,float dt){
  if(!rripInit){ for(int i=0;i<RRIP_N;i++) rpAct[i]=false; rripSpawn=at; rripInit=true; }
  if(at-rripSpawn>=340){
    rripSpawn=at;
    for(int i=0;i<RRIP_N;i++) if(!rpAct[i]){ rpAct[i]=true; rpR[i]=1+random(9); rpC[i]=1+random(9); rpRad[i]=0; rpHue[i]=random(256); break; }
  }
  clearFrame();
  for(int i=0;i<RRIP_N;i++){
    if(!rpAct[i]) continue;
    rpRad[i]+=dt*0.006f;
    if(rpRad[i]>8.0f){ rpAct[i]=false; continue; }
    int R=(int)(rpRad[i]+0.5f);
    uint8_t val=(uint8_t)constrain(220.0f*(1.0f-rpRad[i]/8.0f),0.0f,220.0f);
    for(int r=0;r<11;r++) for(int c=0;c<11;c++)
      if(max(iabs(r-rpR[i]),iabs(c-rpC[i]))==R) fillCell(r,c,CRGB(CHSV(rpHue[i],200,val)));
  }
  FastLED.show();
}

// ═══════════ 45. SPIRAL GALAXY ═══════════
void anim_spiralGalaxy(uint32_t t){
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){
    float x=X_POS[xi];
    for(int n=1;n<=121;n++){
      float y=(float)n-CENTER, r=sqrtf(x*x+y*y)/MAX_DIST, ang=atan2f(y,x);
      float arm=sinf(ang*2.0f - r*7.5f + tf*1.6f)*0.5f+0.5f;
      float v=arm*(1.0f-r*0.55f); if(r<0.14f) v+=(0.14f-r)*5.0f; // core glow
      v=constrain(v,0.0f,1.0f); v*=v;
      uint8_t b=(uint8_t)(v*230.0f);
      uint8_t hue=(uint8_t)(165+r*70.0f+tf*9.0f);
      setLED(true,xi+1,n, b>3?CRGB(CHSV(hue,r<0.14f?90:210,b)):CRGB::Black);
    }
  }
  for(int yi=0;yi<12;yi++){
    float yy=Y_POS[yi];
    for(int n=1;n<=121;n++){
      float x=(float)n-CENTER, r=sqrtf(x*x+yy*yy)/MAX_DIST, ang=atan2f(yy,x);
      float arm=sinf(ang*2.0f - r*7.5f + tf*1.6f)*0.5f+0.5f;
      float v=arm*(1.0f-r*0.55f); if(r<0.14f) v+=(0.14f-r)*5.0f;
      v=constrain(v,0.0f,1.0f); v*=v;
      uint8_t b=(uint8_t)(v*230.0f);
      uint8_t hue=(uint8_t)(165+r*70.0f+tf*9.0f);
      setLED(false,yi+1,n, b>3?CRGB(CHSV(hue,r<0.14f?90:210,b)):CRGB::Black);
    }
  }
  FastLED.show();
}

// ═══════════ 46. PLASMA FIELD ═══════════
void anim_plasmaField(uint32_t t){
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){
    float x=X_POS[xi];
    for(int n=1;n<=121;n++){
      float y=(float)n-CENTER;
      float v=sinf(x*0.09f+tf)+sinf(y*0.09f+tf*1.1f)
             +sinf((x+y)*0.07f+tf*0.8f)+sinf(sqrtf(x*x+y*y)*0.10f-tf*1.3f);
      float nv=(v+4.0f)/8.0f;
      uint8_t hue=(uint8_t)(nv*255.0f+tf*20.0f);
      setLED(true,xi+1,n,CRGB(CHSV(hue,230,200)));
    }
  }
  for(int yi=0;yi<12;yi++){
    float yy=Y_POS[yi];
    for(int n=1;n<=121;n++){
      float x=(float)n-CENTER;
      float v=sinf(x*0.09f+tf)+sinf(yy*0.09f+tf*1.1f)
             +sinf((x+yy)*0.07f+tf*0.8f)+sinf(sqrtf(x*x+yy*yy)*0.10f-tf*1.3f);
      float nv=(v+4.0f)/8.0f;
      uint8_t hue=(uint8_t)(nv*255.0f+tf*20.0f);
      setLED(false,yi+1,n,CRGB(CHSV(hue,230,200)));
    }
  }
  FastLED.show();
}

// ═══════════ 47. BREATHE GRADIENT (calm) ═══════════
void anim_breatheGradient(uint32_t t){
  float tf=t*0.001f;
  float breath=0.45f+0.55f*(0.5f+0.5f*sinf(tf*0.6f));
  uint8_t base=(uint8_t)(tf*6.0f);
  for(int xi=0;xi<12;xi++){
    float x=X_POS[xi];
    for(int n=1;n<=121;n++){
      float y=(float)n-CENTER;
      uint8_t hue=(uint8_t)(base+(x+y)*0.9f);
      uint8_t b=(uint8_t)(breath*200.0f);
      setLED(true,xi+1,n,CRGB(CHSV(hue,190,b)));
    }
  }
  for(int yi=0;yi<12;yi++){
    float yy=Y_POS[yi];
    for(int n=1;n<=121;n++){
      float x=(float)n-CENTER;
      uint8_t hue=(uint8_t)(base+(x+yy)*0.9f);
      uint8_t b=(uint8_t)(breath*200.0f);
      setLED(false,yi+1,n,CRGB(CHSV(hue,190,b)));
    }
  }
  FastLED.show();
}

// ═══════════ 48. BOIDS (flocking) ═══════════
bool boidInit=false;
#define BOID_N 10
static float bx[BOID_N],by[BOID_N],bvx[BOID_N],bvy[BOID_N];
void anim_boids(uint32_t at,float dt){
  if(!boidInit){ for(int i=0;i<BOID_N;i++){ bx[i]=frand()*100-50; by[i]=frand()*100-50; bvx[i]=frand()*0.04f-0.02f; bvy[i]=frand()*0.04f-0.02f; } boidInit=true; }
  for(int i=0;i<BOID_N;i++){
    float cx=0,cy=0,vx=0,vy=0,sx=0,sy=0; int cnt=0;
    for(int j=0;j<BOID_N;j++){
      if(j==i) continue;
      float ddx=bx[j]-bx[i], ddy=by[j]-by[i], d2=ddx*ddx+ddy*ddy;
      cx+=bx[j]; cy+=by[j]; vx+=bvx[j]; vy+=bvy[j]; cnt++;
      if(d2<90.0f){ sx-=ddx; sy-=ddy; }
    }
    if(cnt){ cx/=cnt; cy/=cnt; vx/=cnt; vy/=cnt;
      bvx[i]+=(cx-bx[i])*0.0006f+(vx-bvx[i])*0.03f+sx*0.0018f;
      bvy[i]+=(cy-by[i])*0.0006f+(vy-bvy[i])*0.03f+sy*0.0018f;
    }
    float sp=sqrtf(bvx[i]*bvx[i]+bvy[i]*bvy[i]);
    if(sp>0.05f){ bvx[i]=bvx[i]/sp*0.05f; bvy[i]=bvy[i]/sp*0.05f; }
    if(sp<0.012f&&sp>0){ bvx[i]=bvx[i]/sp*0.012f; bvy[i]=bvy[i]/sp*0.012f; }
    bx[i]+=bvx[i]*dt; by[i]+=bvy[i]*dt;
    if(bx[i]<-55) bx[i]=55; if(bx[i]>55) bx[i]=-55;
    if(by[i]<-55) by[i]=55; if(by[i]>55) by[i]=-55;
  }
  clearFrame();
  for(int i=0;i<BOID_N;i++){
    int col=(int)((bx[i]+55.0f)/110.0f*10.0f+0.5f);
    int row=(int)((by[i]+55.0f)/110.0f*10.0f+0.5f);
    uint8_t hue=(uint8_t)((atan2f(bvy[i],bvx[i])+3.1416f)*40.0f);
    fillCell(row,col,CRGB(CHSV(hue,220,220)));
  }
  FastLED.show();
}

// ═══════════ 49. FIREWORKS ═══════════
bool fwInit=false;
#define FW_N 48
static float fpx[FW_N],fpy[FW_N],fpvx[FW_N],fpvy[FW_N],fpl[FW_N];
static uint8_t fphue[FW_N];
static uint32_t fwSpawn;
void anim_fireworks(uint32_t at,float dt){
  if(!fwInit){ for(int i=0;i<FW_N;i++) fpl[i]=0; fwSpawn=at; fwInit=true; }
  if(at-fwSpawn>=800){
    fwSpawn=at;
    float bx0=2+frand()*7, by0=1+frand()*4; uint8_t h=random(256);
    int made=0;
    for(int i=0;i<FW_N&&made<14;i++) if(fpl[i]<=0){
      float ang=frand()*6.2832f, spd=0.006f+frand()*0.014f;
      fpx[i]=bx0; fpy[i]=by0; fpvx[i]=cosf(ang)*spd; fpvy[i]=sinf(ang)*spd;
      fpl[i]=1.0f; fphue[i]=h+random(20); made++;
    }
  }
  clearFrame();
  for(int i=0;i<FW_N;i++){
    if(fpl[i]<=0) continue;
    fpvy[i]+=0.00003f*dt;               // gravity (downward = +row)
    fpx[i]+=fpvx[i]*dt; fpy[i]+=fpvy[i]*dt;
    fpl[i]-=dt*0.0009f;
    int col=(int)(fpx[i]+0.5f), row=(int)(fpy[i]+0.5f);
    uint8_t val=(uint8_t)constrain(fpl[i]*235.0f,0.0f,235.0f);
    fillCell(row,col,CRGB(CHSV(fphue[i],230,val)));
  }
  FastLED.show();
}

// ═══════════ 50. REACTION-DIFFUSION (Gray-Scott lite) ═══════════
bool rdInit=false;
static float rdA[11][11],rdB[11][11],rdNA[11][11],rdNB[11][11];
void anim_reactionDiff(uint32_t at,float dt){
  if(!rdInit){
    for(int r=0;r<11;r++) for(int c=0;c<11;c++){ rdA[r][c]=1.0f; rdB[r][c]=0.0f; }
    for(int r=4;r<=6;r++) for(int c=4;c<=6;c++){ rdA[r][c]=0.0f; rdB[r][c]=0.9f; }
    rdInit=true;
  }
  const float Da=1.0f,Db=0.5f,f=0.055f,k=0.062f;
  for(int iter=0;iter<2;iter++){
    for(int r=0;r<11;r++) for(int c=0;c<11;c++){
      int rm=(r+10)%11, rp=(r+1)%11, cm=(c+10)%11, cp=(c+1)%11; // toroidal
      float lapA=rdA[rm][c]+rdA[rp][c]+rdA[r][cm]+rdA[r][cp]-4.0f*rdA[r][c];
      float lapB=rdB[rm][c]+rdB[rp][c]+rdB[r][cm]+rdB[r][cp]-4.0f*rdB[r][c];
      float ab=rdA[r][c]*rdB[r][c]*rdB[r][c];
      rdNA[r][c]=constrain(rdA[r][c]+(Da*lapA-ab+f*(1.0f-rdA[r][c])),0.0f,1.0f);
      rdNB[r][c]=constrain(rdB[r][c]+(Db*lapB+ab-(k+f)*rdB[r][c]),0.0f,1.0f);
    }
    for(int r=0;r<11;r++) for(int c=0;c<11;c++){ rdA[r][c]=rdNA[r][c]; rdB[r][c]=rdNB[r][c]; }
  }
  clearFrame();
  for(int r=0;r<11;r++) for(int c=0;c<11;c++){
    uint8_t val=(uint8_t)constrain(rdB[r][c]*640.0f,0.0f,225.0f);
    if(val>4) fillCell(r,c,CRGB(CHSV((uint8_t)(150+rdB[r][c]*260.0f),210,val)));
  }
  FastLED.show();
}

// ═══════════════════════════════════════════════════════
//  TEXT / FONT SYSTEM  (5x7 glyphs on the 11x11 cell canvas)
//  Used by the banner modes and the mode-change overlay.
// ═══════════════════════════════════════════════════════
// FONT3x5[glyph][row], 5 bits/row (bit4 = leftmost column), 3 rows tall.
// Very short (3 cells → ~4 strip-lines) but still distinguishable.
// glyph index: 0=space, 1..26 = A..Z, 27..36 = 0..9
const uint8_t FONT3x5[37][3]={
  {0,0,0},                       // ' '
  {0b01110,0b11111,0b10001},     // A
  {0b11110,0b11110,0b11111},     // B
  {0b01111,0b10000,0b01111},     // C
  {0b11110,0b10001,0b11110},     // D
  {0b11111,0b11110,0b11111},     // E
  {0b11111,0b11110,0b10000},     // F
  {0b01110,0b10011,0b01111},     // G
  {0b10001,0b11111,0b10001},     // H
  {0b11111,0b00100,0b11111},     // I
  {0b00111,0b00010,0b11110},     // J
  {0b10010,0b11100,0b10010},     // K
  {0b10000,0b10000,0b11111},     // L
  {0b10001,0b11011,0b10101},     // M
  {0b11001,0b10101,0b10011},     // N
  {0b01110,0b10001,0b01110},     // O
  {0b11110,0b11110,0b10000},     // P
  {0b01110,0b10101,0b01111},     // Q
  {0b11110,0b11110,0b10011},     // R
  {0b01111,0b01110,0b11110},     // S
  {0b11111,0b00100,0b00100},     // T
  {0b10001,0b10001,0b01110},     // U
  {0b10001,0b01010,0b00100},     // V
  {0b10001,0b10101,0b01110},     // W
  {0b10001,0b01110,0b10001},     // X
  {0b10001,0b01110,0b00100},     // Y
  {0b11111,0b00110,0b11111},     // Z
  {0b11111,0b10001,0b11111},     // 0
  {0b01100,0b00100,0b01110},     // 1
  {0b11110,0b00100,0b01111},     // 2
  {0b11110,0b01110,0b11110},     // 3
  {0b10010,0b11111,0b00010},     // 4
  {0b11111,0b11110,0b01111},     // 5
  {0b11110,0b11111,0b01111},     // 6
  {0b11111,0b00010,0b00100},     // 7
  {0b11111,0b01110,0b11111},     // 8
  {0b11111,0b01111,0b00011}      // 9
};
int glyphIndex(char c){
  if(c>='a'&&c<='z') c-=32;
  if(c>='A'&&c<='Z') return c-'A'+1;
  if(c>='0'&&c<='9') return c-'0'+27;
  return 0; // space / unknown
}
int textPixelWidth(const char* s){ int n=0; while(*s++) n++; return n*6-1; } // 5 wide + 1 gap
// Draw string on the cell canvas; left edge at cell column xStart (may be off-screen for scrolling).
void drawTextCells(const char* s,int xStart,int topRow,CRGB col){
  int x=xStart;
  for(const char* p=s;*p;p++){
    int gi=glyphIndex(*p);
    for(int row=0;row<3;row++){
      uint8_t bits=FONT3x5[gi][row];
      for(int c=0;c<5;c++) if(bits&(1<<(4-c))){ int cx=x+c; if(cx>=0&&cx<=10) fillCell(topRow+row,cx,col); }
    }
    x+=6;
  }
}

// ── Mode names (index matches the dispatch switch below) ──
const char* const ANIM_NAMES[]={
  "SETUP","HEARTBEAT","BINARY RAIN","TECTONIC","VORTEX","EARTHQUAKE","DNA HELIX",
  "SUPERNOVA","BLACK HOLE","WORMHOLE","MITOSIS","COLLIDER","SHOCKWAVE","PULSAR",
  "LIQUID METAL","ICE CROWN","CORIOLIS","CRYSTAL","AURORA","LAVA LAMP","NERVE",
  "GRAVITY LENS","TIDAL WAVE","CYMATICS","STARFIELD","DARK MATTER","SQUARES",
  "SQUARE RAIN","DIAMOND","CHECKER","CUBE","SUNSET","DEPTH","COSMIC DUST",
  "HEARTGLOW","EVOLUTION","MORPH","ATTRACTOR","HARMONIC","AGENT FIELD","TETRIS",
  "SNAKE","ICONS","LOGO","STAR TUNNEL","RIPPLE","GALAXY","PLASMA","BREATHE",
  "BOIDS","FIREWORKS","REACTION","TOGA","CASTLE","2026"
};

// ── Mode-change overlay: mode NUMBER held static & centered for a
//    moment, then the NAME scrolls past slowly. Text is centred at
//    row 3 (5-tall font → rows 3..7).
uint32_t overlayUntil=0, overlayStart=0;
char     overlayNum[4];    // "40"
char     overlayName[24];  // "REACTION"
#define OVL_HOLD 1000      // ms the number stays fixed in the middle
#define OVL_STEP 100       // ms per cell while the name scrolls (slow)
#define TEXT_ROW 4         // top row for the 3-tall font (centres it: rows 4..6)
void triggerOverlay(uint32_t t,int idx){
  int n=idx+1; char* p=overlayNum;
  if(n>=10) *p++=(char)('0'+n/10);
  *p++=(char)('0'+n%10); *p=0;
  char* q=overlayName;
  for(const char* s=ANIM_NAMES[idx]; *s && (q-overlayName)<22; ) *q++=*s++;
  *q=0;
  overlayStart=t;
  overlayUntil=t+OVL_HOLD+(uint32_t)((11+textPixelWidth(overlayName)+2)*OVL_STEP);
}
void renderOverlay(uint32_t t){
  clearFrame();
  uint32_t el=t-overlayStart;
  CRGB col=CRGB(CHSV((uint8_t)(t/16),230,220));
  if(el<OVL_HOLD){
    int w=textPixelWidth(overlayNum);
    drawTextCells(overlayNum,(11-w)/2,TEXT_ROW,col);   // number fixed & centred
  } else {
    int x=11-(int)((el-OVL_HOLD)/OVL_STEP);             // name scrolls slowly
    drawTextCells(overlayName,x,TEXT_ROW,col);
  }
  FastLED.show();
}

// ═══════════ 0. SETUP / CALIBRATION GRID ═══════════
// Lights every strip's crossing LEDs so each strip can be aligned:
// X-strip crossings = red, Y-strip crossings = green, origin = white.
void anim_setupGrid(uint32_t t){
  clearFrame();
  for(int s=1;s<=12;s++)
    for(int k=0;k<12;k++){
      setLED(true, s,INTS[k],CRGB(255,0,0));   // X-strip crossings = red
      setLED(false,s,INTS[k],CRGB(0,255,0));   // Y-strip crossings = green
    }
  setLED(true, 1,INTS[0],CRGB(255,255,255));   // origin (X1,Y1) = white
  setLED(false,1,INTS[0],CRGB(255,255,255));
  FastLED.show();
}

// ═══════════ 52-54. SCROLLING TEXT BANNERS ═══════════
void anim_scrollBanner(const char* s,uint32_t at,uint8_t baseHue){
  clearFrame();
  int span=textPixelWidth(s)+11+6;
  int pos=(int)((at/150)%span);   // slower scroll
  drawTextCells(s,11-pos,TEXT_ROW,CRGB(CHSV((uint8_t)(baseHue+at/60),255,220)));
  FastLED.show();
}
void anim_textTOGA(uint32_t at){   anim_scrollBanner("TOGA",  at, 40); }
void anim_textCASTLE(uint32_t at){ anim_scrollBanner("CASTLE",at,140); }
void anim_text2026(uint32_t at){   anim_scrollBanner("2026",  at,210); }

#define TOTAL_ANIMS 55
#define BRI_STEP         5
#define STROBE_BRI_STEP  10
#define STROBE_FREQ_STEP 2
#define STROBE_SQ_STEP   1
#define HEARTBEAT_TIMEOUT 400
#define REPEAT_DELAY     500
#define REPEAT_RATE      80

void resetAnimState(uint32_t t) {
  rainInit=false;shockInit=false;novaInit=false;blobInit=false;
  nerveInit=false;starInit=false;pendInit=false;sqRainInit=false;golInit=false;
  evoInit=false;morphInit=false;wvInit=false;lorInit=false;hrInit=false;nfInit=false;plInit=false;
  tetInit=false;snakeInit=false;logoInit=false;stunInit=false;rripInit=false;boidInit=false;fwInit=false;rdInit=false;
  strikeTime=t;collideStrike=t;
  epiX=(random(80)-40);epiY=(random(80)-40);
  clearAll();delay(100);
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  loadSettings();
  Serial.printf("Loaded: anim=%d bri=%d strobeBri=%d on=%dms off=%dms\n",
    cfg.animIndex+1,cfg.brightness,cfg.strobeBrightness,cfg.strobeOnMs,cfg.strobeOffMs);

  FastLED.addLeds<WS2812B,14,GRB>(ledsX[0],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,15,GRB>(ledsX[1],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,17,GRB>(ledsX[2],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,16,GRB>(ledsX[3],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,13,GRB>(ledsX[4],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,18,GRB>(ledsX[5],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,21,GRB>(ledsY[0],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,22,GRB>(ledsY[1],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,23,GRB>(ledsY[2],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,25,GRB>(ledsY[3],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,26,GRB>(ledsY[4],LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B,19,GRB>(ledsY[5],LEDS_PER_STRIP);
  FastLED.setBrightness(cfg.brightness);
  clearAll();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.printf("Receiver MAC: %s\n", WiFi.macAddress().c_str());
  if(esp_now_init() != ESP_OK) { Serial.println("ESP-NOW init failed!"); while(1) delay(1000); }
  esp_now_register_recv_cb(onReceive);
  Serial.printf("Grid ready. %d animations.\n", TOTAL_ANIMS);
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

  // ── Timeout → treat all as released ──────────────
  // No heartbeat for 400ms = sender disconnected or reset
  bool senderAlive = (espLastHeartbeat > 0) &&
                     ((t - (uint32_t)espLastHeartbeat) < (uint32_t)HEARTBEAT_TIMEOUT);
  uint16_t held = senderAlive ? (uint16_t)espHeldMask : 0;

  // ── Strobe: purely level-driven by bit 15 ─────────
  bool strobeHeld = (held >> 15) & 1;
  if(strobeHeld && !strobeActive) {
    preStrobeAnim = cfg.animIndex;
    preStrobeBri  = cfg.brightness;
    strobeActive  = true;
    strobeOn      = false;
    strobeFlip    = t;
    Serial.println("  STROBE ON");
  } else if(!strobeHeld && strobeActive) {
    strobeActive  = false;
    strobeOn      = false;
    cfg.animIndex = preStrobeAnim;
    cfg.brightness= preStrobeBri;
    FastLED.setBrightness(cfg.brightness);
    FastLED.clear();
    for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
    FastLED.show();
    Serial.printf("  STROBE OFF → Anim %d Bri %d\n", cfg.animIndex+1, cfg.brightness);
  }

  static uint32_t recvPressTime[16]   = {};
  static uint32_t recvLastRepeat[16]  = {};
  static uint16_t prevHeld            = 0;
  static bool     blackoutActive      = false;
  static int      preBlackoutAnim     = 0;

  uint16_t rising  = held & ~prevHeld;
  prevHeld = held;

  // Record press times
  if(rising)
    for(int i=0;i<16;i++) if((rising>>i)&1){ recvPressTime[i]=t; recvLastRepeat[i]=t; }

  // ── Blackout toggle (button 11, rising edge only) ──
  if((rising>>11)&1) {
    if(!blackoutActive) {
      blackoutActive   = true;
      preBlackoutAnim  = cfg.animIndex;
      FastLED.clear();
      for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
      FastLED.show();
      Serial.println("  BLACKOUT ON");
    } else {
      blackoutActive   = false;
      cfg.animIndex    = preBlackoutAnim;
      Serial.println("  BLACKOUT OFF");
    }
  }

  // ── Strobe overrides blackout while held ──
  // (handled below — strobe check uses strobeHeld regardless of blackout)

  // ── Mode buttons: rising edge only, blocked in blackout ──
  if((rising>>2)&1&&!strobeActive&&!blackoutActive){cfg.animIndex=(cfg.animIndex+1)%TOTAL_ANIMS;resetAnimState(t);saveSettings();triggerOverlay(t,cfg.animIndex);Serial.printf("  →Anim%d\n",cfg.animIndex+1);}
  if((rising>>6)&1&&!strobeActive&&!blackoutActive){cfg.animIndex=(cfg.animIndex-1+TOTAL_ANIMS)%TOTAL_ANIMS;resetAnimState(t);saveSettings();triggerOverlay(t,cfg.animIndex);Serial.printf("  ←Anim%d\n",cfg.animIndex+1);}

  // ── Value buttons: hold-repeat with acceleration ──
  int valueBtns[] = {0,4,1,5,8,12,9,13,10,14};
  for(int vi=0;vi<10;vi++){
    int i=valueBtns[vi];
    if(!((held>>i)&1)) continue;
    uint32_t heldFor = t - recvPressTime[i];
    uint32_t rate;
    if(heldFor==0)                  rate=0;
    else if(heldFor < REPEAT_DELAY) continue;
    else if(heldFor < 2000)         rate=REPEAT_RATE;
    else if(heldFor < 4000)         rate=REPEAT_RATE/2;
    else                            rate=REPEAT_RATE/5;
    if(heldFor>0 && t-recvLastRepeat[i]<rate) continue;
    recvLastRepeat[i]=t;
    switch(i){
      case 0:  cfg.brightness=(uint8_t)min((int)cfg.brightness+BRI_STEP,MAX_BRIGHTNESS);if(!strobeActive&&!blackoutActive)FastLED.setBrightness(cfg.brightness);saveSettings();break;
      case 4:  cfg.brightness=(uint8_t)max((int)cfg.brightness-BRI_STEP,1);if(!strobeActive&&!blackoutActive)FastLED.setBrightness(cfg.brightness);saveSettings();break;
      case 1:  cfg.animSpeed=constrain(cfg.animSpeed+0.1f,0.1f,4.0f);saveSettings();Serial.printf("  Spd→%.2f\n",cfg.animSpeed);break;
      case 5:  cfg.animSpeed=constrain(cfg.animSpeed-0.1f,0.1f,4.0f);saveSettings();Serial.printf("  Spd→%.2f\n",cfg.animSpeed);break;
      case 8:  cfg.strobeBrightness=(uint8_t)min((int)cfg.strobeBrightness+STROBE_BRI_STEP,255);saveSettings();break;
      case 12: cfg.strobeBrightness=(uint8_t)max((int)cfg.strobeBrightness-STROBE_BRI_STEP,1);saveSettings();break;
      case 9:  cfg.strobeOnMs=(uint8_t)max((int)cfg.strobeOnMs-STROBE_FREQ_STEP,5);cfg.strobeOffMs=cfg.strobeOnMs;saveSettings();break;
      case 13: cfg.strobeOnMs=(uint8_t)min((int)cfg.strobeOnMs+STROBE_FREQ_STEP,200);cfg.strobeOffMs=cfg.strobeOnMs;saveSettings();break;
      case 10: cfg.strobeSquares=(uint8_t)min((int)cfg.strobeSquares+STROBE_SQ_STEP,122);saveSettings();break;
      case 14: cfg.strobeSquares=(uint8_t)max((int)cfg.strobeSquares-STROBE_SQ_STEP,1);saveSettings();break;
    }
  }

  if(t-lastFrame<20)return;
  lastFrame=t;

  // Accumulate scaled animation time
  // Use absolute time scaled by speed for animations
  static uint32_t lastAnimT = 0;
  if(lastAnimT==0) lastAnimT=t;
  float frameDt = (float)(t - lastAnimT);
  lastAnimT = t;
  static float virtualT = 0.0f;
  virtualT += frameDt * cfg.animSpeed;
  uint32_t at = (uint32_t)virtualT;
  float    dt = frameDt * cfg.animSpeed;

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
      sync.animIndex        = cfg.animIndex;
      sync.animSpeed        = cfg.animSpeed;
      sync.blackoutActive   = blackoutActive;
      esp_now_send(senderMAC,(uint8_t*)&sync,sizeof(sync));
    }
  }

  // ── Strobe rendering (overrides blackout while held) ─
  if(strobeActive){
    if(t>strobeFlip){
      strobeOn=!strobeOn;
      strobeFlip=t+(uint32_t)(strobeOn?cfg.strobeOnMs:cfg.strobeOffMs);
      if(strobeOn){
        FastLED.setBrightness(cfg.strobeBrightness);
        FastLED.clear();
        if(cfg.strobeSquares >= 122) {
          for(int e=0;e<NUM_ELEC;e++){
            for(int i=0;i<LEDS_PER_STRIP;i++){ledsX[e][i]=CRGB::White;ledsY[e][i]=CRGB::White;}
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
            for(int led=INTS[col]+1;led<INTS[col+1];led++){setLED(false,r1,led,CRGB::White);setLED(false,r2,led,CRGB::White);}
            for(int led=INTS[row]+1;led<INTS[row+1];led++){setLED(true,c1,led,CRGB::White);setLED(true,c2,led,CRGB::White);}
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

  // ── Blackout: grid stays dark ─────────────────────
  if(blackoutActive){
    FastLED.clear();
    for(int e=0;e<NUM_ELEC;e++){ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;}
    FastLED.show();
    return;
  }

  // ── Mode-change overlay: show number + name briefly ──
  if(t < overlayUntil){
    FastLED.setBrightness(cfg.brightness);
    renderOverlay(t);
    return;
  }

  // ── Normal animation ──────────────────────────────
  FastLED.setBrightness(cfg.brightness);
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
  }
}
