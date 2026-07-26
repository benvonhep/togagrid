// New sharp-edged space & particle modes
// (Auto-split from grid_SW.ino; #included into the main sketch — all shared
//  globals/functions are visible from grid_SW.ino. Do not compile standalone.)
#pragma once

static inline float fxhash(float n){ return fxfract(sinf(n)*43758.5453f); }

// 59 — STARFIELD: stars streaming out from the centre, accelerating.
void anim_starfield2(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<70;i++){ float ang=fxhash(i*1.7f)*6.2832f; float ph=fxfract(tf*0.25f+fxhash(i*3.1f));
    float r=ph*ph*62.0f; float br=ph; if(br>1)br=1;
    fxSplat(cosf(ang)*r,sinf(ang)*r,0.4f+ph*1.5f,1.0f,hotColor(br,150+(int)(fxhash(i)*40),30)); }
  showGrid();
}
// 60 — WARP: starfield with elongated motion-blur streaks.
void anim_warp(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<55;i++){ float ang=fxhash(i*1.7f)*6.2832f; float ph=fxfract(tf*0.6f+fxhash(i*3.1f));
    float r=ph*ph*64.0f; float r0=r-6.0f-ph*10.0f; if(r0<0)r0=0; float c=cosf(ang),s=sinf(ang); float br=ph; if(br>1)br=1;
    fxStreak(c*r0,s*r0,c*r,s*r,0.4f,1.0f,hotColor(br,160,28)); }
  showGrid();
}
// 62 — SHOOTING STARS: faint twinkling sky + ONE fast comet in a random direction.
void anim_shootingStars(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<45;i++){ float x=(fxhash(i*1.3f)-0.5f)*120,y=(fxhash(i*2.7f)-0.5f)*120;
    float tw=0.4f+0.6f*fabsf(sinf(tf*2.0f+i)); fxSplat(x,y,0.35f,0.9f,hotColor(tw*0.35f,150,22)); }
  // Exactly ONE shooting star at a time — WAY faster, fresh random direction each spawn.
  static int curCycle=-1; static float ang=0.0f, sx=0.0f, sy=0.0f, hue=150.0f;
  const float speed=2.6f;                    // cycles/sec (was ~0.4) → much faster streaks
  float pcont=tf*speed; int cycle=(int)pcont; float ph=pcont-cycle;
  if(cycle!=curCycle){ curCycle=cycle;
    ang=frand()*6.2832f;                     // brand-new random heading
    float c=cosf(ang),s=sinf(ang);
    sx=-c*82.0f + (frand()-0.5f)*36.0f;      // start off-panel, opposite the travel dir
    sy=-s*82.0f + (frand()-0.5f)*36.0f;
    hue=140.0f+frand()*40.0f;
  }
  float c=cosf(ang),s=sinf(ang);
  float head=ph*210.0f, tail=head-26.0f; if(tail<0)tail=0;
  float br=(1.0f-ph); if(ph<0.1f)br=ph*10.0f;
  float hx=sx+c*head, hy=sy+s*head, tx=sx+c*tail, ty=sy+s*tail;
  fxStreak(tx,ty,hx,hy,0.5f,1.2f,hotColor(br*0.8f,(uint8_t)hue,20));   // tail behind travel dir
  fxSplat(hx,hy,1.2f,1.3f,hotColor(br,(uint8_t)hue,30));               // bright comet head
  showGrid();
}
// 63 — HYPER VORTEX: a swirling kaleidoscopic plasma tunnel of flowing colour.
//      (Rewritten wholesale — mesmerizing showpiece, not the old accretion ring.)
void anim_blackHole2(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER;
      float x=pass?other:base, y=pass?base:other;
      float r=sqrtf(x*x+y*y); if(r>72.0f) continue;
      float th=atan2f(y,x);
      // spiralling arms: angle winds up with radius and time
      float swirl = th*5.0f + r*0.30f - tf*2.6f;
      float spiral= sinf(swirl);
      // interference rings pulsing outward, warped by a 6-fold kaleidoscope fold
      float rings = sinf(r*0.52f - tf*3.4f + 2.0f*sinf(th*3.0f + tf));
      float plasma= 0.5f*spiral + 0.5f*rings;
      float v = 0.5f + 0.5f*plasma; v*=v;                    // punchier contrast
      float env = constrain(1.0f - r/72.0f, 0.0f, 1.0f);
      env = env*env*(3.0f-2.0f*env);                         // smooth radial falloff
      v *= env;
      // hues flow around the wheel with angle + radius + time + the plasma itself
      uint8_t hue = (uint8_t)(th*40.7f + r*3.0f + tf*46.0f + 120.0f*spiral);
      uint8_t sat = (uint8_t)(210.0f - 90.0f*v);             // hot cores bleach toward white
      if(v>0.02f) fxAdd(pass?false:true,s+1,n,CRGB(CHSV(hue,sat,255)),v);
    } }
  // pulsing vortex eye + a colour-cycling shockwave ring
  float pulse=0.6f+0.4f*sinf(tf*3.0f);
  fxSplat(0,0,2.0f+pulse*2.2f,3.0f,CRGB(255,244,255));
  fxRing(0,0,9.0f+4.0f*sinf(tf*1.7f),1.4f,1.8f,CRGB(CHSV((uint8_t)(tf*60.0f),200,255)));
  showGrid();
}
// 64 — PULSAR: rotating twin beams + a fanned trail of dimmer shadow echoes.
void anim_pulsar2(uint32_t at){
  clearFrame(); float tf=at*0.001f; float base=tf*2.4f;
  fxSplat(0,0,2.5f,1.2f,CRGB(200,220,255));
  // several trailing "shadow" strings, each at greater angular lag and dimmer
  const int NSHAD=5;
  for(int sh=NSHAD;sh>=1;sh--){ float lag=0.15f*sh; float dim=1.0f-(float)sh/(NSHAD+1);
    for(int b=0;b<2;b++){ float a=base-lag+b*3.1416f; for(int k=3;k<=14;k++){ float rr=k*4.4f;
      fxSplat(cosf(a)*rr,sinf(a)*rr,0.5f,1.0f,CRGB(CHSV(160,150,(uint8_t)(60.0f*dim)))); } } }
  // main beams on top (brightest)
  for(int b=0;b<2;b++){ float a=base+b*3.1416f; for(int k=1;k<=14;k++){ float rr=k*4.4f; float br=(1.0f-k/15.0f);
    fxSplat(cosf(a)*rr,sinf(a)*rr,0.7f,1.0f,hotColor(br,160,26)); } }
  showGrid();
}
// 67 — HORSEHEAD: dark silhouette galloping against a glowing backdrop.
void anim_horsehead(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  // galloping gait: two-beat vertical bob (fabsf doubles the cadence) + fore/aft sway + head nod
  float bob =2.8f*fabsf(sinf(tf*6.0f)) - 1.2f;
  float sway=1.6f*sinf(tf*3.0f);
  float tilt=0.10f*sinf(tf*6.0f+1.0f);
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=pass?other:base,y=pass?base:other;
      uint8_t nz=inoise8((uint16_t)((x+80)*160),(uint16_t)((y+80)*160),(uint16_t)(at*25));
      float glow=(nz/255.0f); glow=glow*glow; float grad=constrain((y+50)/100.0f,0.0f,1.0f);
      float v=glow*(0.35f+0.65f*grad)*0.9f;
      // move the silhouette each frame so the horse looks like it is running
      float hx=x-sway, hy=y-bob - tilt*(x+4.0f);
      float headTop=-6.0f-10.0f*expf(-((hx+4)*(hx+4))/60.0f);
      if(hx>-16 && hx<8 && hy>headTop) v*=0.04f;              // silhouette stays near-black
      if(v>0.02f) fxAdd(pass?false:true,s+1,n,hotTint(v,24,20),v);
    } }
  showGrid();
}
// 70 — ROTATING EARTH through a window: shaded globe with scrolling continents.
void anim_earthWindow(uint32_t at){
  clearFrame(); float r=26.0f;
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=pass?other:base,y=pass?base:other;
      float d=sqrtf(x*x+y*y); if(d>r+1.4f)continue; float mask=constrain((r+1.4f-d)/1.4f,0.0f,1.0f);
      float lit=0.12f+0.88f*constrain(0.5f+0.5f*(x*0.7f-y*0.2f)/r,0.0f,1.0f);
      uint8_t nz=inoise8((uint16_t)((x+80)*260+(uint16_t)(at*30)),(uint16_t)((y+80)*260),3000);
      bool land=nz>140; uint8_t hue=land?85:150,sat=land?200:230; float v=mask*lit*(land?1.0f:0.85f);
      if(v>0.02f) fxAdd(pass?false:true,s+1,n,CRGB(CHSV(hue,sat,255)),v);
    } }
  fxRing(0,0,40.0f,3.0f,1.4f,CRGB(40,40,48));      // window frame vignette
  showGrid();
}
// 72 — RADAR: sweeping beam (soft leading edge) + ships that flash when swept.
void anim_radar(uint32_t at){
  clearFrame(); float tf=at*0.001f; float a=fxfract(tf*0.25f)*6.2832f;
  fxRing(0,0,20,1.0f,1.4f,CRGB(0,60,0)); fxRing(0,0,40,1.0f,1.4f,CRGB(0,45,0));
  for(int k=1;k<=15;k++){ float rr=k*3.4f;
    fxSplat(cosf(a)*rr,sinf(a)*rr,0.7f,1.8f,CRGB(0,170,0));            // soft front (no sharp edge)
    fxSplat(cosf(a-0.35f)*rr,sinf(a-0.35f)*rr,0.6f,1.0f,CRGB(0,110,0));
    fxSplat(cosf(a-0.7f)*rr,sinf(a-0.7f)*rr,0.5f,1.0f,CRGB(0,55,0)); }
  fxSplat(0,0,1.5f,1.0f,CRGB(0,220,0));
  for(int i=0;i<4;i++){ float ba=fxhash(i*3.7f)*6.2832f; float rr=12+fxhash(i*5.9f)*28;
    float rel=fmodf(a-ba+6.2832f,6.2832f); float br=expf(-rel*2.2f);
    if(br>0.03f) fxSplat(cosf(ba)*rr,sinf(ba)*rr,1.1f,1.0f,CRGB(0,(uint8_t)(120+br*135),0)); }
  showGrid();
}
// 73 — SINE WAVES: smooth traveling, interfering bands.
void anim_sineWaves(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=pass?other:base,y=pass?base:other;
      float w=sinf(x*0.14f+tf*1.5f)+0.7f*sinf((x*0.09f-y*0.06f)+tf*1.0f);
      float f=constrain(1.0f-fabsf(y*0.08f-w)*1.4f,0.0f,1.0f);
      if(f>0.02f) fxAdd(pass?false:true,s+1,n,hotTint(f,140+(uint8_t)(w*30),20),f);
    } }
  showGrid();
}
// 74 — ROCKET FIRE: a wide rocket riding a big flame plume with billowing smoke.
void anim_rocketFire(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  // ── smoke: clouds billowing out below the exhaust (world +y renders DOWNWARD,
  //    so the rocket flies UP → flame/exhaust live at POSITIVE y, below it) ──
  for(int i=0;i<44;i++){ float ph=fxfract(tf*0.5f+fxhash(i*5.5f));
    float x=(fxhash(i*1.7f)-0.5f)*48.0f*(0.4f+ph); float y=28.0f+ph*30.0f;
    uint8_t g=(uint8_t)((70.0f+fxhash(i*8.3f)*60.0f)*(1.0f-ph));
    if(g>4) fxSplat(x,y,2.0f+ph*4.2f,2.4f,CRGB(g,g,g)); }
  // ── fire: big hot plume shooting down (on-wall) from the throat ──
  for(int i=0;i<90;i++){ float ph=fxfract(tf*1.3f+fxhash(i*2.7f));
    float x=(fxhash(i*3.1f)-0.5f)*20.0f*(0.3f+ph); float y=6.0f+ph*54.0f; float life=1.0f-ph;
    uint8_t hue=(uint8_t)(45*life),val=(uint8_t)(255*life),sat=ph<0.2f?110:255;
    fxSplat(x,y,1.0f+ph*2.2f,1.2f,CRGB(CHSV(hue,sat,val))); }
  for(int j=0;j<6;j++) fxSplat((frand()-0.5f)*6.0f,6.0f+frand()*3.0f,2.6f,1.4f,CRGB(255,245,220)); // bright throat
  // ── wide rocket body + nose cone (y negated → nose points UP on the wall) ──
  const float W=10.0f;                         // half-width — noticeably wider
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=pass?other:base,y=pass?base:other;
      y=-y;                                                                            // 180° vertical mirror
      float v=0.0f; uint8_t hue=160,sat=30;
      if(y>-2.0f && y<34.0f && fabsf(x)<W){ v=1.0f-0.5f*fabsf(x)/W; }                  // cylinder body
      else if(y>=34.0f && y<50.0f){ float w=W*(1.0f-(y-34.0f)/16.0f);                 // nose cone
        if(fabsf(x)<w){ v=0.9f*(1.0f-0.5f*fabsf(x)/(w+0.01f)); hue=0; sat=200; } }
      if(v>0.02f) fxAdd(pass?false:true,s+1,n,CRGB(CHSV(hue,sat,(uint8_t)(v*255.0f))),v);
    } }
  fxStreak(-W,2,-W-6,8,0.6f,1.2f,CRGB(200,20,20));   // fins (mirrored to the base)
  fxStreak( W,2, W+6,8,0.6f,1.2f,CRGB(200,20,20));
  fxSplat(0,-20,2.4f,1.2f,CRGB(120,200,255));         // window (up near the nose)
  showGrid();
}
// 75 — LIGHTHOUSE: a bright narrow beam sweeping steadily around from the lamp.
//      (Rewritten wholesale — was the amber warning beacon.)
void anim_warningLight(uint32_t at){
  clearFrame(); float tf=at*0.001f; float beam=tf*1.6f;      // beam heading, steady rotation
  fxSplat(0,0,3.0f,1.6f,CRGB(255,250,230));                  // lamp housing
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=pass?other:base,y=pass?base:other;
      float r=sqrtf(x*x+y*y); if(r<2.0f||r>72.0f) continue;
      float th=atan2f(y,x); float d=th-beam;
      while(d>3.1416f)d-=6.2832f; while(d<-3.1416f)d+=6.2832f;
      const float half=0.22f;                                // narrow beam wedge (radians)
      float wedge=1.0f-fabsf(d)/half; if(wedge<=0.0f) continue; wedge*=wedge;
      float reach=constrain(1.0f-r/72.0f,0.0f,1.0f);
      float v=wedge*(0.35f+0.65f*reach);                     // illuminate what the beam passes
      if(v>0.02f) fxAdd(pass?false:true,s+1,n,CRGB(CHSV(38,110,255)),v);
    } }
  showGrid();
}
// 76 — ALIENS: cheeky hopping alien heads that wobble, blink, dart their eyes and zap.
void anim_aliens(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<3;i++){
    float hop   = fabsf(sinf(tf*2.2f+i*2.1f));            // springy bounce
    float cx    = -34+i*34.0f + sinf(tf*0.9f+i)*4.0f;     // wander side to side
    float cy    = -17.0f + hop*36.0f;                     // hop up — much bigger vertical travel (~2.5x)
    float squash= 1.0f+0.22f*sinf(tf*4.4f+i);             // wobble/squash
    uint8_t hue = 90+i*22;
    fxSplat(cx,cy,9.0f*squash,3.0f,CRGB(CHSV(hue,220,80)));
    fxSplat(cx,cy+3,7.0f,2.0f,CRGB(CHSV(hue,220,55)));
    // wiggly antennae with glowing tips
    fxSplat(cx-3.0f+sinf(tf*5.0f+i)*1.2f,cy+9.5f,0.8f,1.0f,CRGB(CHSV(40,255,210)));
    fxSplat(cx+3.0f-sinf(tf*5.0f+i)*1.2f,cy+9.5f,0.8f,1.0f,CRGB(CHSV(40,255,210)));
    // blinking eyes that dart left/right
    float blink=fabsf(sinf(tf*1.3f+i*1.7f))>0.12f?1.0f:0.1f;
    float look =sinf(tf*1.7f+i)*1.3f;
    fxSplat(cx-3.0f+look,cy-1.0f,1.6f,1.0f,CRGB(CHSV(0,200,(uint8_t)(240*blink))));
    fxSplat(cx+3.0f+look,cy-1.0f,1.6f,1.0f,CRGB(CHSV(0,200,(uint8_t)(240*blink))));
    // cheeky grin
    fxSplat(cx,cy-4.0f,1.0f,1.2f,CRGB(CHSV(0,150,(uint8_t)(150*blink))));
    // occasional zap bolt from an alien
    int znt=(int)(tf*0.6f)+i*7;
    if(fxhash(znt*1.7f)>0.66f){ float zp=fxfract(tf*2.5f+i);
      float zx=cx+(fxhash(znt*3.3f)-0.5f)*44.0f;
      fxStreak(cx,cy,zx,cy+2.0f+zp*6.0f,0.4f,1.0f,CRGB(CHSV(150,120,(uint8_t)(255*(1.0f-zp*0.6f))))); }
  }
  showGrid();
}
// 77 — SPACE INVADERS: smooth glow-cluster invaders marching (not blocky).
void fxInvader(float cx,float cy,uint8_t hue,int frame){
  CRGB c=CRGB(CHSV(hue,210,255));
  fxSplat(cx,cy,2.0f,1.1f,c); fxSplat(cx-3,cy,1.3f,1.0f,c); fxSplat(cx+3,cy,1.3f,1.0f,c);
  float ay=frame?-2.5f:2.5f; fxSplat(cx-4,cy+ay,1.1f,1.0f,c); fxSplat(cx+4,cy+ay,1.1f,1.0f,c);
}
// 78 — RAIN: smooth falling droplets with short soft trails.
void anim_rain2(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<40;i++){ float x=(fxhash(i*1.9f)-0.5f)*116; float sp=0.5f+fxhash(i*3.3f)*0.6f;
    float ph=fxfract(tf*sp+fxhash(i*7.7f)); float y=54.0f-ph*114.0f;
    fxStreak(x,y+5.0f,x,y,0.35f,1.0f,CRGB(CHSV(150,120,220))); }
  showGrid();
}
// 79 — LEVITATING POINTS: soft motes drifting upward, slowly flickering.
void anim_levitate(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<45;i++){ float x=(fxhash(i*2.2f)-0.5f)*116+sinf(tf*0.6f+i)*4.0f;
    float sp=0.2f+fxhash(i*4.1f)*0.25f; float ph=fxfract(tf*sp+fxhash(i*6.3f)); float y=-56.0f+ph*112.0f;
    float fl=0.5f+0.5f*sinf(tf*3.0f+i*1.3f); float fade=constrain(1.4f-fabsf(ph-0.5f)*2.0f,0.0f,1.0f);
    fxSplat(x,y,0.5f+0.4f*fl,1.0f,CRGB(CHSV(40,90,(uint8_t)(fade*(120+fl*135))))); }
  showGrid();
}
// 80 — CONFETTI: multicolor points popping and fading, drifting down.
void anim_confetti(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<60;i++){ float ph=fxfract(tf*0.7f+fxhash(i*9.1f));
    float x=(fxhash(i*1.3f)-0.5f)*116+ph*6.0f*(fxhash(i*2.9f)-0.5f); float y=(fxhash(i*4.7f)-0.5f)*116-ph*16.0f;
    float br=sinf(ph*3.14159f); if(br>0.02f) fxSplat(x,y,0.7f,1.0f,CRGB(CHSV((uint8_t)(fxhash(i*5.5f)*255),230,(uint8_t)(br*255)))); }
  showGrid();
}
// 81 — SPIRAL EJECT: a wide spinning spiral flinging glowing particles outward.
void anim_spiralEject(uint32_t at){
  clearFrame(); float tf=at*0.001f; fxSplat(0,0,2.4f,1.2f,CRGB(255,255,255));
  for(int i=0;i<70;i++){ float ph=fxfract(tf*0.5f+fxhash(i*3.7f)); float r=ph*76.0f;   // wider reach
    float ang=fxhash(i*3.7f)*6.2832f+tf*2.0f+ph*2.2f; float br=1.0f-ph;                // arms spread more
    fxSplat(cosf(ang)*r,sinf(ang)*r,0.6f+ph*1.4f,1.0f,hotColor(br,(uint8_t)(180+ph*80),0)); }
  showGrid();
}
// 82 — 3D-NOISE CLOUDS: drifting layered value-noise.
void anim_clouds3d(uint32_t at){
  clearFrame();
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=pass?other:base,y=pass?base:other;
      uint8_t nz=inoise8((uint16_t)((x+80)*130+(uint16_t)(at*20)),(uint16_t)((y+80)*130),(uint16_t)(at*12));
      uint8_t nz2=inoise8((uint16_t)((x+80)*300),(uint16_t)((y+80)*300+(uint16_t)(at*15)),(uint16_t)(at*8));
      float v=(nz/255.0f)*0.7f+(nz2/255.0f)*0.3f; v=constrain((v-0.35f)/0.65f,0.0f,1.0f); v=v*v;
      if(v>0.03f) fxAdd(pass?false:true,s+1,n,hotTint(v,150+(uint8_t)((nz2-128)/6),18),v);
    } }
  showGrid();
}
// 83 — PULSING HEART: a clear heart silhouette (two top lobes, pointed bottom) beating.
void anim_pulseHeart(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  // lub-dub double beat that briefly swells the heart — eased for a smooth pulse
  float b=fmodf(tf*1.4f,1.0f);
  float beat=constrain(expf(-b*7.0f)+0.6f*expf(-fabsf(b-0.22f)*7.0f),0.0f,1.0f);
  beat=beat*beat*(3.0f-2.0f*beat);                         // smoothstep ease — no harsh steps
  float sc=1.0f/(22.0f - beat*3.0f);                        // grow on the beat
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER;
      // world +y renders DOWNWARD on the wall, so negate y to flip the classic
      // heart curve upright: two lobes on TOP, pointed cusp at the BOTTOM.
      float x=(pass?other:base)*sc, y=-((pass?base:other)*sc) - 0.2f;
      float xx=x*x,yy=y*y; float h=(xx+yy-1.0f); float f3=h*h*h-1.6f*xx*y*yy;  // <0 inside; 1.6 = deeper top notch → the two lobes read
      float fill=f3<0?1.0f:constrain(1.0f-f3*5.0f,0.0f,1.0f);              // soft anti-aliased body
      float rim=f3<0?constrain(1.0f+f3/0.55f,0.0f,1.0f):0.0f;             // bright band hugging the INNER edge (keeps the top notch crisp)
      if(fill>0.03f||rim>0.03f){
        float bri=fill*(70.0f+beat*45.0f) + rim*rim*(150.0f+beat*95.0f);  // dim body + sharp bright outline traces the whole silhouette
        bri=constrain(bri,0.0f,255.0f);
        uint8_t sat=(uint8_t)(255.0f-90.0f*rim*rim);                      // outline blushes pinker so the shape pops
        fxAdd(pass?false:true,s+1,n,CRGB(CHSV(0,sat,(uint8_t)bri)),1.0f); }
    } }
  showGrid();
}
// 84 — RAIN UP: droplets flying from bottom to top with soft trails (reverse of RAIN).
void anim_rainUp(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<40;i++){ float x=(fxhash(i*1.9f)-0.5f)*116; float sp=0.5f+fxhash(i*3.3f)*0.6f;
    float ph=fxfract(tf*sp+fxhash(i*7.7f)); float y=-54.0f+ph*114.0f;      // rise
    fxStreak(x,y-5.0f,x,y,0.35f,1.0f,CRGB(CHSV(150,120,220))); }           // trail below the head
  showGrid();
}
// 85 — STARS FLYING: stars drifting across in every direction, wrapping, twinkling.
void anim_starsFlying(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<55;i++){ float ang=fxhash(i*1.3f)*6.2832f; float sp=6.0f+fxhash(i*2.7f)*14.0f;
    float rx=fxhash(i*4.1f)*120.0f-60.0f + cosf(ang)*sp*tf;
    float ry=fxhash(i*5.9f)*120.0f-60.0f + sinf(ang)*sp*tf;
    float x=rx-120.0f*floorf((rx+60.0f)/120.0f);                          // wrap into [-60,60)
    float y=ry-120.0f*floorf((ry+60.0f)/120.0f);
    float tw=0.5f+0.5f*sinf(tf*3.0f+i);
    fxSplat(x,y,0.4f+tw*0.35f,1.0f,hotColor(0.43f+tw*0.57f,150,20)); }
  showGrid();
}
// 86 — FIREWORK: shells launch, burst into radial sparks that arc down and fade.
void anim_firework2(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int s=0;s<4;s++){
    float period=2.2f+fxhash(s*3.1f)*1.5f;
    float ph=fxfract((tf+fxhash(s*7.3f)*period)/period);
    float bx=(fxhash(s*2.1f)-0.5f)*80.0f, by=10.0f+fxhash(s*5.5f)*30.0f;  // burst point (+y = up)
    uint8_t hue=(uint8_t)(fxhash(s*9.9f)*255);
    if(ph<0.30f){                                                        // rocket rising
      float ry=-55.0f+(ph/0.30f)*(by+55.0f);
      fxSplat(bx,ry,0.7f,1.0f,CRGB(CHSV(40,120,255)));
      fxStreak(bx,ry-6.0f,bx,ry,0.4f,1.0f,CRGB(CHSV(30,180,140)));
    } else {                                                             // explosion
      float e=(ph-0.30f)/0.70f, fade=1.0f-e;
      for(int p=0;p<24;p++){ float pa=p*0.2618f+fxhash(s*13.0f);
        float spd=18.0f+fxhash(s*100.0f+p*1.0f)*10.0f;
        float px=bx+cosf(pa)*spd*e, py=by+sinf(pa)*spd*e-14.0f*e*e;      // gravity
        fxSplat(px,py,0.5f,1.0f,hotColor(fade*fade,hue,0)); }
    }
  }
  showGrid();
}
// 88 — VU BARS: each vertical strip is a music level bar (green→red), jumping to a beat.
void anim_vuColumns(uint32_t at){
  clearFrame(); float tf=at*0.001f; int beat=(int)floorf(tf*2.4f); float decay=1.0f-fxfract(tf*2.4f);
  for(int xi=0;xi<12;xi++){
    float lvl=constrain(0.2f+0.32f*(0.5f+0.5f*sinf(tf*2.5f+xi*0.8f))+0.6f*fxhash(xi*2.3f+beat*1.7f)*decay,0.05f,1.0f);
    float topY=-60.0f+lvl*120.0f;
    for(int n=1;n<=121;n++){ float y=(float)n-CENTER; if(y>topY)continue; float frac=(y+60.0f)/120.0f;
      float a=(topY-y<5.0f)?1.0f:0.7f;
      fxAdd(true,xi+1,n,hotTint(a,(uint8_t)(96*(1.0f-frac)),6,150),a); }
  }
  showGrid();
}
// 89 — VU GRID: vertical bars (rise from bottom) + horizontal bars (grow from left) together.
void anim_vuGrid(uint32_t at){
  clearFrame(); float tf=at*0.001f; int beat=(int)floorf(tf*2.4f); float decay=1.0f-fxfract(tf*2.4f);
  for(int xi=0;xi<12;xi++){
    float lvl=constrain(0.2f+0.3f*(0.5f+0.5f*sinf(tf*2.5f+xi*0.8f))+0.6f*fxhash(xi*2.3f+beat*1.7f)*decay,0.05f,1.0f);
    float topY=-60.0f+lvl*120.0f;
    for(int n=1;n<=121;n++){ float y=(float)n-CENTER; if(y>topY)continue; float frac=(y+60.0f)/120.0f;
      float a=(topY-y<5.0f)?1.0f:0.65f;
      fxAdd(true,xi+1,n,hotTint(a,(uint8_t)(96*(1.0f-frac)),6,150),a); }
  }
  for(int yi=0;yi<12;yi++){
    float lvl=constrain(0.2f+0.3f*(0.5f+0.5f*cosf(tf*2.3f+yi*0.7f))+0.6f*fxhash(yi*3.1f+beat*2.9f)*decay,0.05f,1.0f);
    float rightX=-60.0f+lvl*120.0f;
    for(int n=1;n<=121;n++){ float x=(float)n-CENTER; if(x>rightX)continue; float frac=(x+60.0f)/120.0f;
      float a=(rightX-x<5.0f)?1.0f:0.65f;
      fxAdd(false,yi+1,n,hotTint(a,(uint8_t)(160+64*(1.0f-frac)),6,150),a); }
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════
// 91 — SUPER HEXAGON: the game, imitated. Hexagonal walls are born at the rim
// and rush the centre, each missing one of its six sides (the gap). A little
// dart orbits the central hexagon and AUTO-DODGES — it steers into the gap of
// the nearest incoming wall. The whole field spins and, Super-Hexagon style,
// periodically reverses; the six sectors alternate two shades and pulse. Fast
// and dizzying. Time is `at`, so the Speed knob scales the whole game;
// showGrid() applies the Colour knob + AGC.
//
// Walls use a HEXAGON metric (level set of d*cos(angle-to-nearest-side-normal))
// rather than a plain radius, so the rings read as rotating hexagons, not
// circles — the one detail that makes it "super hexagon" and not "tunnel".
// ═══════════════════════════════════════════════════════
#define SHX_WALLS 6
struct ShxWall { float r; uint8_t gap; uint8_t hue; bool live; };
static ShxWall shxW[SHX_WALLS];
static bool    shxInit=false;
static float   shxSpin, shxSpinVel, shxPlayer, shxLastAt, shxSpawnAcc, shxFlipAcc;

void anim_superHexagon(uint32_t at){
  float tf=at*0.001f;
  if(!shxInit){
    for(int i=0;i<SHX_WALLS;i++) shxW[i].live=false;
    shxSpin=0; shxSpinVel=1.4f; shxPlayer=0; shxLastAt=tf; shxSpawnAcc=0; shxFlipAcc=0;
    shxInit=true;
  }
  float dt=tf-shxLastAt; shxLastAt=tf; if(dt<0)dt=0; if(dt>0.10f)dt=0.10f;   // guard mode-entry / wrap jumps

  // ── Global spin with occasional reversal (the signature Super Hexagon move) ──
  shxFlipAcc-=dt;
  if(shxFlipAcc<=0){ shxSpinVel=(frand()<0.5f?-1.0f:1.0f)*(1.0f+frand()*1.6f); shxFlipAcc=2.5f+frand()*3.0f; }
  shxSpin+=shxSpinVel*dt;

  // ── Spawn walls from the rim on a steady cadence, advance them inward ──
  const float OUTER=66.0f, WALLSPD=42.0f, THICK=2.6f, PLR=11.0f, CORE=6.5f;
  shxSpawnAcc-=dt;
  if(shxSpawnAcc<=0){
    for(int i=0;i<SHX_WALLS;i++) if(!shxW[i].live){
      shxW[i].live=true; shxW[i].r=OUTER; shxW[i].gap=(uint8_t)random(6); shxW[i].hue=(uint8_t)(tf*22.0f);
      break;
    }
    shxSpawnAcc=0.55f;   // seconds between walls
  }
  for(int i=0;i<SHX_WALLS;i++) if(shxW[i].live){ shxW[i].r-=WALLSPD*dt; if(shxW[i].r<3.0f) shxW[i].live=false; }

  // ── Auto-player: aim at the gap of the nearest wall still outside the player ──
  float targetSect=-1.0f, bestR=1e9f;
  for(int i=0;i<SHX_WALLS;i++) if(shxW[i].live && shxW[i].r>PLR-1.0f && shxW[i].r<bestR){
    bestR=shxW[i].r; targetSect=shxW[i].gap+0.5f;   // centre of the gap sector, in field space
  }
  if(targetSect>=0.0f){
    float worldTarget=(targetSect/6.0f)*6.2832f + shxSpin;     // field angle → world angle
    float da=worldTarget-shxPlayer; while(da>3.14159f)da-=6.2832f; while(da<-3.14159f)da+=6.2832f;
    float step=7.0f*dt; if(da>step)da=step; if(da<-step)da=-step; shxPlayer+=da;
  }

  clearFrame();
  // ── Field render: one pass per strip pixel, hexagon walls composited over the
  //    alternating-sector background (max-blend keeps walls/core on top). ──
  float pulse=0.55f+0.45f*sinf(tf*3.0f);
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float bpos=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=pass?other:bpos, y=pass?bpos:other;
      float d=sqrtf(x*x+y*y);
      float ang=atan2f(y,x)-shxSpin;                         // into the spinning field frame
      float sf=ang*0.15915f; sf-=floorf(sf);                 // /(2pi) → 0..1
      float within=sf*6.0f; int sect=(int)within; if(sect>5)sect=5;
      float haf=(within-(float)sect)-0.5f;                   // -0.5..0.5 across the sector
      float hexval=d*cosf(haf*1.0472f);                      // pi/3: hexagon level set (not a circle)
      // background — two shades, radial falloff, whole-field pulse
      float radial=constrain(1.0f-d/72.0f,0.0f,1.0f);
      float v=((sect&1)?0.040f:0.075f)*pulse*radial;       // dim glow — let the hexagon rings dominate
      CRGB col=CRGB(CHSV((uint8_t)(tf*18.0f),210,255));    // a touch more saturated so it reads as colour, not wash
      // central hexagon
      float ce=1.0f-fabsf(hexval-CORE)/1.3f; if(ce>v){ v=ce; col=CRGB(CHSV((uint8_t)(tf*18.0f+20),150,255)); }
      // walls (every side lit except the gap sector)
      for(int i=0;i<SHX_WALLS;i++) if(shxW[i].live && sect!=shxW[i].gap){
        float e=1.0f-fabsf(hexval-shxW[i].r)/THICK; if(e>v){ v=e; col=hotTint(e,shxW[i].hue,18); }
      }
      if(v>0.03f) fxAdd(pass?false:true,s+1,n,col,v);
    } }
  // ── The dart orbiting the core ──
  fxSplat(cosf(shxPlayer)*PLR, sinf(shxPlayer)*PLR, 1.4f, 1.1f, CRGB(255,255,255));
  showGrid();
}

// ═══════════════════════════════════════════════════════
// 92 — SQUARE IMPLODE: the exact inverse of CONCENTRIC SQUARES (wall mode #27).
// There the square ping-rings expand OUTWARD from the centre; here they are born
// at the rim and collapse INWARD, brightening and whitening as they shrink, with
// a white core flash on every collapse. Same drawGridSquare()/showGrid() path;
// only the phase term's sign is flipped so the wavefront travels the other way.
// ═══════════════════════════════════════════════════════
void anim_squareImplode(uint32_t at){
  float tf=at*0.001f; const float pulseRate=0.5f;
  clearFrame();
  for(int half=5;half>=1;half--){
    int r1=6-half, r2=7+half, c1=6-half, c2=7+half;
    // +half*0.15 (concentricSquares used -half*0.15): the bright band now moves
    // from the outer rings toward the centre as time advances.
    float phase=fmodf(tf*pulseRate+(float)half*0.15f,1.0f); if(phase<0.0f)phase+=1.0f;
    float bri=powf(sinf(phase*M_PI),2.0f);
    uint8_t b=(uint8_t)(bri*(150.0f+(5-half)*20.0f)); if(b<3)continue;    // inner rings peak brighter
    uint8_t hue=(uint8_t)(tf*30.0f-half*20.0f);
    uint8_t sat=(uint8_t)constrain(220-(5-half)*28,90,255);              // whiter toward the core
    drawGridSquare(r1,c1,r2,c2,CRGB(CHSV(hue,sat,b)));
  }
  // Core flash timed to the collapse — a spike as a wavefront reaches the middle.
  float cf=powf(sinf(fmodf(tf*pulseRate,1.0f)*M_PI),6.0f);
  uint8_t cb=(uint8_t)(cf*255.0f);
  if(cb>3) fillCell(5,5,CRGB(cb,cb,cb));
  showGrid();
}
