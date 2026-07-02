// Field / space classic animations
// (Auto-split from grid_SW.ino; #included into the main sketch — all shared
//  globals/functions are visible from grid_SW.ino. Do not compile standalone.)
#pragma once

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
  showGrid();
}

// 2. BINARY RAIN
static float rainPos[12];static float rainSpeed[12];static uint8_t rainLen[12];static bool rainInit=false;
void anim_binaryRain(uint32_t t,float dt) {
  if(!rainInit){for(int i=0;i<12;i++){rainPos[i]=random(121);rainSpeed[i]=0.03f+random(70)*0.001f;rainLen[i]=15+random(30);}rainInit=true;}
  for(int xi=0;xi<12;xi++){rainPos[xi]+=rainSpeed[xi]*dt;if(rainPos[xi]>121.0f+rainLen[xi])rainPos[xi]=-rainLen[xi];for(int n=1;n<=121;n++){float trail=rainPos[xi]-n;uint8_t b=0;if(trail>0&&trail<rainLen[xi]){float fade=1.0f-(trail/rainLen[xi]);b=(trail<2.0f)?255:(uint8_t)(fade*fade*180.0f);if(random(20)==0)b=min((int)b+60,255);}uint8_t hue=96+(n%3)*5;setLED(true,xi+1,n,b>0?CRGB(CHSV(hue,220,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){for(int n=1;n<=121;n++){uint8_t b=(random(100)<3)?random(30):0;setLED(false,yi+1,n,b>0?CRGB(CHSV(96,220,b)):CRGB::Black);}}
  showGrid();
}

// 3. TECTONIC PLATES
void anim_tectonic(uint32_t t) {
  float fX=CENTER+sinf(t*0.0003f)*20.0f;float fY=CENTER+cosf(t*0.0002f)*20.0f;float fw=4.0f+sinf(t*0.0007f)*2.0f;
  for(int xi=0;xi<12;xi++){float sp=X_POS[xi]+CENTER;for(int n=1;n<=121;n++){float dX=fabsf((float)n-fY);float dY=fabsf(sp-fX);float fb=0.0f;if(dX<fw)fb=max(fb,1.0f-dX/fw);if(dY<fw)fb=max(fb,1.0f-dY/fw);float rumble=sinf(t*0.001f+xi*0.7f+n*0.05f)*0.5f+0.5f;uint8_t pb=(uint8_t)(rumble*25.0f);if(fb>0.01f){uint8_t b=(uint8_t)(fb*255.0f);uint8_t sat=(uint8_t)(200.0f-fb*200.0f);setLED(true,xi+1,n,CRGB(CHSV(20,sat,b)));}else setLED(true,xi+1,n,CRGB(pb,pb,pb));}}
  for(int yi=0;yi<12;yi++){float sp=Y_POS[yi]+CENTER;for(int n=1;n<=121;n++){float dX=fabsf((float)n-fX);float dY=fabsf(sp-fY);float fb=0.0f;if(dX<fw)fb=max(fb,1.0f-dX/fw);if(dY<fw)fb=max(fb,1.0f-dY/fw);float rumble=sinf(t*0.001f+yi*0.9f+n*0.05f)*0.5f+0.5f;uint8_t pb=(uint8_t)(rumble*25.0f);if(fb>0.01f){uint8_t b=(uint8_t)(fb*255.0f);uint8_t sat=(uint8_t)(200.0f-fb*200.0f);setLED(false,yi+1,n,CRGB(CHSV(20,sat,b)));}else setLED(false,yi+1,n,CRGB(pb,pb,pb));}}
  showGrid();
}

// 4. VORTEX SPIRAL (the fan favourite)
void anim_vortex(uint32_t t) {
  float tf=t*0.001f;float tight=0.15f+sinf(tf*0.3f)*0.08f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float sp=fmod(angle*3.0f/(2.0f*M_PI)-dist*tight-tf*0.5f,1.0f);if(sp<0.0f)sp+=1.0f;float bri=powf(sinf(sp*M_PI),2.5f)*sinf(constrain(dist/MAX_DIST,0.0f,1.0f)*M_PI);uint8_t b=(uint8_t)(bri*220.0f);uint8_t hue=(uint8_t)((int)(180.0f+dist*0.8f+tf*20.0f)%255);setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,220,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float sp=fmod(angle*3.0f/(2.0f*M_PI)-dist*tight-tf*0.5f,1.0f);if(sp<0.0f)sp+=1.0f;float bri=powf(sinf(sp*M_PI),2.5f)*sinf(constrain(dist/MAX_DIST,0.0f,1.0f)*M_PI);uint8_t b=(uint8_t)(bri*220.0f);uint8_t hue=(uint8_t)((int)(180.0f+dist*0.8f+tf*20.0f)%255);setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,220,b)):CRGB::Black);}}
  showGrid();
}

// 5. EARTHQUAKE
static float epiX=0.0f,epiY=0.0f;static uint32_t strikeTime=0;
void anim_earthquake(uint32_t t) {
  if(t-strikeTime>3000){epiX=(random(80)-40);epiY=(random(80)-40);strikeTime=t;}
  float elapsed=(t-strikeTime)*0.001f;float wp=elapsed*30.0f;float decay=expf(-elapsed*0.8f);
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf((x-epiX)*(x-epiX)+(y-epiY)*(y-epiY));float w=max(sinf((dist-wp)*0.3f)*decay,0.0f)+max(sinf((dist-wp*0.6f)*0.2f)*decay*0.4f,0.0f);float total=constrain(w,0.0f,1.0f);uint8_t b=(uint8_t)(total*230.0f);uint8_t sat=(uint8_t)(200.0f-total*100.0f);setLED(true,xi+1,n,b>3?CRGB(CHSV(25,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf((x-epiX)*(x-epiX)+(y-epiY)*(y-epiY));float w=max(sinf((dist-wp)*0.3f)*decay,0.0f)+max(sinf((dist-wp*0.6f)*0.2f)*decay*0.4f,0.0f);float total=constrain(w,0.0f,1.0f);uint8_t b=(uint8_t)(total*230.0f);uint8_t sat=(uint8_t)(200.0f-total*100.0f);setLED(false,yi+1,n,b>3?CRGB(CHSV(25,sat,b)):CRGB::Black);}}
  showGrid();
}

// 6. DNA HELIX
void anim_dnaHelix(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float w=8.0f+sinf(tf*0.7f)*3.0f;float s1y=sinf(x*0.12f-tf*2.0f)*40.0f;float s2y=sinf(x*0.12f+tf*2.0f+M_PI)*40.0f;float b1=fabsf(y-s1y)<w?powf(1.0f-fabsf(y-s1y)/w,2.0f):0.0f;float b2=fabsf(y-s2y)<w?powf(1.0f-fabsf(y-s2y)/w,2.0f):0.0f;CRGB c=CRGB::Black;if(b1>0){CRGB s=CRGB(CHSV(130,240,(uint8_t)(b1*220.0f)));c.r=qadd8(c.r,s.r);c.g=qadd8(c.g,s.g);c.b=qadd8(c.b,s.b);}if(b2>0){CRGB s=CRGB(CHSV(300,240,(uint8_t)(b2*220.0f)));c.r=qadd8(c.r,s.r);c.g=qadd8(c.g,s.g);c.b=qadd8(c.b,s.b);}setLED(true,xi+1,n,c);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float w=8.0f+sinf(tf*0.7f)*3.0f;float s1x=sinf(y*0.12f-tf*2.0f)*40.0f;float s2x=sinf(y*0.12f+tf*2.0f+M_PI)*40.0f;float b1=fabsf(x-s1x)<w?powf(1.0f-fabsf(x-s1x)/w,2.0f):0.0f;float b2=fabsf(x-s2x)<w?powf(1.0f-fabsf(x-s2x)/w,2.0f):0.0f;CRGB c=CRGB::Black;if(b1>0){CRGB s=CRGB(CHSV(130,240,(uint8_t)(b1*220.0f)));c.r=qadd8(c.r,s.r);c.g=qadd8(c.g,s.g);c.b=qadd8(c.b,s.b);}if(b2>0){CRGB s=CRGB(CHSV(300,240,(uint8_t)(b2*220.0f)));c.r=qadd8(c.r,s.r);c.g=qadd8(c.g,s.g);c.b=qadd8(c.b,s.b);}setLED(false,yi+1,n,c);}}
  showGrid();
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
  showGrid();
}

// 8. ACID TRIP (dimmed 35%)
void anim_acidTrip(uint32_t t) {
  float tf=t*0.001f;float cx[5],cy[5];
  for(int i=0;i<5;i++){cx[i]=sinf(tf*0.3f+i*1.3f)*45.0f;cy[i]=cosf(tf*0.4f+i*0.9f)*45.0f;}
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float v=0.0f;for(int i=0;i<5;i++){float d=sqrtf((x-cx[i])*(x-cx[i])+(y-cy[i])*(y-cy[i]));v+=sinf(d*0.15f-tf*(1.0f+i*0.3f));}v=(v+5.0f)/10.0f;uint8_t hue=(uint8_t)(v*255.0f+(uint8_t)(tf*50.0f));uint8_t bri=(uint8_t)((180.0f+sinf(v*M_PI*2.0f)*75.0f)*0.65f);setLED(true,xi+1,n,CRGB(CHSV(hue,240,bri)));}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float v=0.0f;for(int i=0;i<5;i++){float d=sqrtf((x-cx[i])*(x-cx[i])+(y-cy[i])*(y-cy[i]));v+=sinf(d*0.15f-tf*(1.0f+i*0.3f));}v=(v+5.0f)/10.0f;uint8_t hue=(uint8_t)(v*255.0f+(uint8_t)(tf*50.0f));uint8_t bri=(uint8_t)((180.0f+sinf(v*M_PI*2.0f)*75.0f)*0.65f);setLED(false,yi+1,n,CRGB(CHSV(hue,240,bri)));}}
  showGrid();
}

// 9. BLACK HOLE
void anim_blackHole(uint32_t t) {
  float tf=t*0.001f;float eh=8.0f+sinf(tf*0.5f)*2.0f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);if(dist<eh){setLED(true,xi+1,n,CRGB::Black);continue;}float diskBri=expf(-(dist-eh)*0.08f);float diskSpin=fmod(angle-tf*2.0f+dist*0.05f,2.0f*M_PI)/(2.0f*M_PI);float diskPat=powf(fabsf(sinf(diskSpin*M_PI*6.0f)),3.0f);float jetBri=0.0f;float absA=fabsf(fabsf(angle)-M_PI/2.0f);if(absA<0.15f)jetBri=(1.0f-absA/0.15f)*min(1.0f,dist/30.0f)*max(0.0f,1.0f-dist/MAX_DIST)*1.5f;float total=constrain(diskBri*diskPat*0.8f+jetBri,0.0f,1.0f);uint8_t b=(uint8_t)(total*240.0f);uint8_t hue=jetBri>0.1f?150:(uint8_t)(20.0f+diskBri*30.0f);uint8_t sat=jetBri>0.1f?200:(uint8_t)(255.0f-diskBri*150.0f);setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);if(dist<eh){setLED(false,yi+1,n,CRGB::Black);continue;}float diskBri=expf(-(dist-eh)*0.08f);float diskSpin=fmod(angle-tf*2.0f+dist*0.05f,2.0f*M_PI)/(2.0f*M_PI);float diskPat=powf(fabsf(sinf(diskSpin*M_PI*6.0f)),3.0f);float jetBri=0.0f;float absA=fabsf(fabsf(angle)-M_PI/2.0f);if(absA<0.15f)jetBri=(1.0f-absA/0.15f)*min(1.0f,dist/30.0f)*max(0.0f,1.0f-dist/MAX_DIST)*1.5f;float total=constrain(diskBri*diskPat*0.8f+jetBri,0.0f,1.0f);uint8_t b=(uint8_t)(total*240.0f);uint8_t hue=jetBri>0.1f?150:(uint8_t)(20.0f+diskBri*30.0f);uint8_t sat=jetBri>0.1f?200:(uint8_t)(255.0f-diskBri*150.0f);setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  showGrid();
}

// 10. WORMHOLE
void anim_wormhole(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y)/MAX_DIST;float lD=logf(1.0f+dist*9.0f)/logf(10.0f);float ring=fmod(lD*8.0f-tf*2.0f,1.0f);if(ring<0.0f)ring+=1.0f;float angle=atan2f(y,x);float twist=sinf(angle*3.0f+dist*5.0f-tf*1.5f)*0.3f+0.7f;float bri=powf(sinf(ring*M_PI),3.0f)*twist*(1.0f-dist*0.3f);uint8_t b=(uint8_t)(bri*230.0f);uint8_t hue=(uint8_t)(190.0f+dist*40.0f+ring*30.0f);setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,240,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y)/MAX_DIST;float lD=logf(1.0f+dist*9.0f)/logf(10.0f);float ring=fmod(lD*8.0f-tf*2.0f,1.0f);if(ring<0.0f)ring+=1.0f;float angle=atan2f(y,x);float twist=sinf(angle*3.0f+dist*5.0f-tf*1.5f)*0.3f+0.7f;float bri=powf(sinf(ring*M_PI),3.0f)*twist*(1.0f-dist*0.3f);uint8_t b=(uint8_t)(bri*230.0f);uint8_t hue=(uint8_t)(190.0f+dist*40.0f+ring*30.0f);setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,240,b)):CRGB::Black);}}
  showGrid();
}

// 11. CELLULAR MITOSIS
void anim_mitosis(uint32_t t) {
  float tf=t*0.001f;float split=sinf(tf*0.6f)*35.0f;float bx1=split,by1=cosf(tf*0.4f)*20.0f;float bx2=-split,by2=-cosf(tf*0.4f)*20.0f;float br1=15.0f+sinf(tf*1.1f)*5.0f;float br2=15.0f+sinf(tf*0.9f+1.0f)*5.0f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float d1=sqrtf((x-bx1)*(x-bx1)+(y-by1)*(y-by1));float d2=sqrtf((x-bx2)*(x-bx2)+(y-by2)*(y-by2));float v1=max(0.0f,1.0f-d1/br1);float v2=max(0.0f,1.0f-d2/br2);float total=constrain(v1+v2,0.0f,1.0f);float membrane=max(powf(v1*(1.0f-v1)*4.0f,2.0f),powf(v2*(1.0f-v2)*4.0f,2.0f));float bri=total*0.4f+membrane*0.8f;uint8_t b=(uint8_t)(constrain(bri,0.0f,1.0f)*230.0f);uint8_t hue=(uint8_t)(85.0f+total*30.0f);uint8_t sat=(uint8_t)(200.0f-membrane*100.0f);setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float d1=sqrtf((x-bx1)*(x-bx1)+(y-by1)*(y-by1));float d2=sqrtf((x-bx2)*(x-bx2)+(y-by2)*(y-by2));float v1=max(0.0f,1.0f-d1/br1);float v2=max(0.0f,1.0f-d2/br2);float total=constrain(v1+v2,0.0f,1.0f);float membrane=max(powf(v1*(1.0f-v1)*4.0f,2.0f),powf(v2*(1.0f-v2)*4.0f,2.0f));float bri=total*0.4f+membrane*0.8f;uint8_t b=(uint8_t)(constrain(bri,0.0f,1.0f)*230.0f);uint8_t hue=(uint8_t)(85.0f+total*30.0f);uint8_t sat=(uint8_t)(200.0f-membrane*100.0f);setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  showGrid();
}

// 12. SOLAR FLARES
void anim_solarFlares(uint32_t t) {
  float tf=t*0.001f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float bri=0.0f;for(int f=0;f<4;f++){float fA=tf*0.7f+f*M_PI/2.0f;float aP=fmod(tf*0.5f+f*0.25f,1.0f);float aD=sinf(aP*M_PI)*60.0f;float aAng=fA+sinf(aP*M_PI)*0.8f;float ax=cosf(aAng)*aD;float ay=sinf(aAng)*aD;float d=sqrtf((x-ax)*(x-ax)+(y-ay)*(y-ay));bri+=max(0.0f,(1.0f-d/12.0f)*(1.0f-aP*0.5f));}bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*240.0f);uint8_t hue=(uint8_t)(15.0f+bri*25.0f);uint8_t sat=(uint8_t)(255.0f-bri*100.0f);setLED(true,xi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float bri=0.0f;for(int f=0;f<4;f++){float fA=tf*0.7f+f*M_PI/2.0f;float aP=fmod(tf*0.5f+f*0.25f,1.0f);float aD=sinf(aP*M_PI)*60.0f;float aAng=fA+sinf(aP*M_PI)*0.8f;float ax=cosf(aAng)*aD;float ay=sinf(aAng)*aD;float d=sqrtf((x-ax)*(x-ax)+(y-ay)*(y-ay));bri+=max(0.0f,(1.0f-d/12.0f)*(1.0f-aP*0.5f));}bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*240.0f);uint8_t hue=(uint8_t)(15.0f+bri*25.0f);uint8_t sat=(uint8_t)(255.0f-bri*100.0f);setLED(false,yi+1,n,b>3?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  showGrid();
}

// 13. PARTICLE COLLIDER
static uint32_t collideStrike=0;
void anim_collider(uint32_t t) {
  if(t-collideStrike>3500)collideStrike=t;
  float phase=(t-collideStrike)*0.001f/3.5f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float bri=0.0f;if(phase<0.7f){float inward=phase/0.7f;float bD=MAX_DIST*(1.0f-inward);float bW=4.0f;float d1=fabsf(dist-bD);float a1=fmod(fabsf(angle-inward*M_PI*3.0f),M_PI*2.0f);if(d1<bW&&a1<0.3f)bri+=(1.0f-d1/bW)*(1.0f-a1/0.3f);float a2=fmod(fabsf(angle+M_PI-inward*M_PI*3.0f),M_PI*2.0f);if(d1<bW&&a2<0.3f)bri+=(1.0f-d1/bW)*(1.0f-a2/0.3f);}else{float burst=(phase-0.7f)/0.3f;float sR=burst*MAX_DIST*1.5f;float sw=fabsf(dist-sR);bri=max(0.0f,(1.0f-sw/8.0f)*(1.0f-burst));bri+=max(0.0f,(1.0f-dist/15.0f)*(1.0f-burst*2.0f));}bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*255.0f);uint8_t hue=phase<0.7f?(uint8_t)(130.0f+phase*100.0f):(uint8_t)(30.0f+(phase-0.7f)*200.0f);uint8_t sat=(uint8_t)(255.0f-bri*100.0f);setLED(true,xi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float bri=0.0f;if(phase<0.7f){float inward=phase/0.7f;float bD=MAX_DIST*(1.0f-inward);float bW=4.0f;float d1=fabsf(dist-bD);float a1=fmod(fabsf(angle-inward*M_PI*3.0f),M_PI*2.0f);if(d1<bW&&a1<0.3f)bri+=(1.0f-d1/bW)*(1.0f-a1/0.3f);float a2=fmod(fabsf(angle+M_PI-inward*M_PI*3.0f),M_PI*2.0f);if(d1<bW&&a2<0.3f)bri+=(1.0f-d1/bW)*(1.0f-a2/0.3f);}else{float burst=(phase-0.7f)/0.3f;float sR=burst*MAX_DIST*1.5f;float sw=fabsf(dist-sR);bri=max(0.0f,(1.0f-sw/8.0f)*(1.0f-burst));bri+=max(0.0f,(1.0f-dist/15.0f)*(1.0f-burst*2.0f));}bri=constrain(bri,0.0f,1.0f);uint8_t b=(uint8_t)(bri*255.0f);uint8_t hue=phase<0.7f?(uint8_t)(130.0f+phase*100.0f):(uint8_t)(30.0f+(phase-0.7f)*200.0f);uint8_t sat=(uint8_t)(255.0f-bri*100.0f);setLED(false,yi+1,n,b>2?CRGB(CHSV(hue,sat,b)):CRGB::Black);}}
  showGrid();
}

// 14. BREATHING MANDALA (dimmed 35%)
void anim_mandala(uint32_t t) {
  float tf=t*0.001f;
  // breath: keep it lower, never fully peaking — reduces the "too bright too long" issue
  float breath=0.45f+sinf(tf*0.8f)*0.25f;
  for(int xi=0;xi<12;xi++){float x=X_POS[xi];for(int n=1;n<=121;n++){float y=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=dist/MAX_DIST;float sA=fmod(fabsf(fmod(angle,M_PI/4.0f)-(M_PI/8.0f)),M_PI/4.0f);float petal=sinf(sA*8.0f+tf*0.5f)*sinf(nD*M_PI*4.0f-tf*1.2f);float ring=sinf(nD*M_PI*6.0f-tf*0.8f);float radial=sinf(angle*6.0f+tf*0.3f);float v=(petal+ring+radial+3.0f)/6.0f*breath;v=constrain(v,0.0f,1.0f);uint8_t hue=(uint8_t)(v*180.0f+tf*15.0f+nD*60.0f);uint8_t bri=(uint8_t)(v*v*180.0f);setLED(true,xi+1,n,bri>3?CRGB(CHSV(hue,230,bri)):CRGB::Black);}}
  for(int yi=0;yi<12;yi++){float y=Y_POS[yi];for(int n=1;n<=121;n++){float x=(float)n-CENTER;float dist=sqrtf(x*x+y*y);float angle=atan2f(y,x);float nD=dist/MAX_DIST;float sA=fmod(fabsf(fmod(angle,M_PI/4.0f)-(M_PI/8.0f)),M_PI/4.0f);float petal=sinf(sA*8.0f+tf*0.5f)*sinf(nD*M_PI*4.0f-tf*1.2f);float ring=sinf(nD*M_PI*6.0f-tf*0.8f);float radial=sinf(angle*6.0f+tf*0.3f);float v=(petal+ring+radial+3.0f)/6.0f*breath;v=constrain(v,0.0f,1.0f);uint8_t hue=(uint8_t)(v*180.0f+tf*15.0f+nD*60.0f);uint8_t bri=(uint8_t)(v*v*180.0f);setLED(false,yi+1,n,bri>3?CRGB(CHSV(hue,230,bri)):CRGB::Black);}}
  showGrid();
}
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
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
  showGrid();
}
