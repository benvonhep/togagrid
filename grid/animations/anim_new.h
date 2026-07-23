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
// 61 — ASTEROIDS: rocks drifting across on linear paths, with a spinning glint.
void anim_asteroids(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<5;i++){ float sp=0.05f+fxhash(i*2.1f)*0.05f; float ph=fxfract(tf*sp+fxhash(i*5.3f));
    float x=-64.0f+ph*128.0f; float y=(fxhash(i*7.7f)-0.5f)*90.0f; float rad=3.0f+fxhash(i*9.1f)*3.0f;
    fxSplat(x,y,rad,1.3f,CRGB(70,64,58));
    fxSplat(x+cosf(tf*2.0f+i)*rad*0.4f,y+sinf(tf*2.0f+i)*rad*0.4f,0.6f,1.0f,CRGB(155,155,165)); }
  showGrid();
}
// 62 — SHOOTING STARS: faint twinkling sky + occasional bright streaks.
void anim_shootingStars(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<45;i++){ float x=(fxhash(i*1.3f)-0.5f)*120,y=(fxhash(i*2.7f)-0.5f)*120;
    float tw=0.4f+0.6f*fabsf(sinf(tf*2.0f+i)); fxSplat(x,y,0.35f,0.9f,hotColor(tw*0.35f,150,22)); }
  for(int i=0;i<3;i++){ float ph=fxfract(tf*0.4f+fxhash(i*11.1f));
    float x0=-60+fxhash(i*4.4f)*30,y0=-50+fxhash(i*6.6f)*100; float ang=-0.5f+fxhash(i*8.8f)*0.4f; float c=cosf(ang),s=sinf(ang);
    float head=ph*150.0f,tail=head-22.0f; if(tail<0)tail=0; float br=(1.0f-ph); if(ph<0.1f)br=ph*10.0f;
    fxStreak(x0+c*tail,y0+s*tail,x0+c*head,y0+s*head,0.5f,1.1f,hotColor(br,150,20)); }
  showGrid();
}
// 63 — BLACK HOLE: dark core, bright swirling accretion ring.
void anim_blackHole2(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  fxRing(0,0,15.0f,2.0f,1.4f,CRGB(CHSV(28,255,255)));
  fxRing(0,0,20.0f,1.0f,1.4f,CRGB(CHSV(20,255,130)));
  for(int i=0;i<10;i++){ float a=tf*1.6f+i*0.628f; float r=15.0f+sinf(tf*3+i)*1.5f;
    fxSplat(cosf(a)*r,sinf(a)*r,1.0f,1.0f,CRGB(CHSV(40,220,255))); }
  showGrid();
}
// 64 — PULSAR: rotating twin beams from a bright core.
void anim_pulsar2(uint32_t at){
  clearFrame(); float tf=at*0.001f; float base=tf*2.4f;
  fxSplat(0,0,2.5f,1.2f,CRGB(200,220,255));
  for(int b=0;b<2;b++){ float a=base+b*3.1416f; for(int k=1;k<=14;k++){ float rr=k*4.4f; float br=(1.0f-k/15.0f);
    fxSplat(cosf(a)*rr,sinf(a)*rr,0.7f,1.0f,hotColor(br,160,26)); } }
  for(int b=0;b<2;b++){ float a=base-0.28f+b*3.1416f; for(int k=3;k<=14;k++){ float rr=k*4.4f;
    fxSplat(cosf(a)*rr,sinf(a)*rr,0.5f,1.0f,CRGB(CHSV(160,150,40))); } }
  showGrid();
}
// 65 — PULSAR WIND NEBULA: pulsar beams inside a diffuse glow cloud.
void anim_pulsarNebula(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  fxNebula(at,180,200,180,40,44,0.9f);
  float a=tf*2.0f; fxSplat(0,0,2.0f,1.0f,CRGB(220,230,255));
  for(int b=0;b<2;b++){ float aa=a+b*3.1416f; for(int k=1;k<=12;k++){ float rr=k*3.6f; float br=1.0f-k/13.0f;
    fxSplat(cosf(aa)*rr,sinf(aa)*rr,0.6f,1.0f,hotColor(br,150,26)); } }
  showGrid();
}
// 66 — ORION NEBULA: layered gas clouds + embedded stars.
void anim_orionNebula(uint32_t at){
  clearFrame();
  fxNebula(at,155,190,150,30,48,0.9f);
  fxNebula(at,215,160,220,45,30,0.8f);
  fxNebula(at,20,130,300,60,16,0.7f);
  for(int i=0;i<6;i++) fxSplat((fxhash(i*3.3f)-0.5f)*70,(fxhash(i*5.1f)-0.5f)*70,0.5f,0.9f,hotColor(0.88f,150,18));
  showGrid();
}
// 67 — HORSEHEAD: dark silhouette against a glowing backdrop.
void anim_horsehead(uint32_t at){
  clearFrame();
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=pass?other:base,y=pass?base:other;
      uint8_t nz=inoise8((uint16_t)((x+80)*160),(uint16_t)((y+80)*160),(uint16_t)(at*25));
      float glow=(nz/255.0f); glow=glow*glow; float grad=constrain((y+50)/100.0f,0.0f,1.0f);
      float v=glow*(0.35f+0.65f*grad)*0.9f;
      float headTop=-6.0f-10.0f*expf(-((x+4)*(x+4))/60.0f);
      if(x>-16 && x<8 && y>headTop) v*=0.04f;                 // silhouette stays near-black
      if(v>0.02f) fxAdd(pass?false:true,s+1,n,hotTint(v,24,20),v);
    } }
  showGrid();
}
// 68 — TWIN STARS: a binary pair orbiting, joined by a faint gas bridge.
void anim_twinStars(uint32_t at){
  clearFrame(); float tf=at*0.001f; float a=tf*1.3f; float R=22.0f;
  float x1=cosf(a)*R,y1=sinf(a)*R,x2=-x1,y2=-y1;
  fxStreak(x1,y1,x2,y2,0.8f,2.2f,CRGB(CHSV(280,150,40)));
  fxSplat(x1,y1,7.0f,4.0f,CRGB(CHSV(30,180,110))); fxSplat(x1,y1,2.2f,1.2f,CRGB(CHSV(35,120,255)));
  fxSplat(x2,y2,7.0f,4.0f,CRGB(CHSV(150,180,110))); fxSplat(x2,y2,2.2f,1.2f,CRGB(CHSV(150,120,255)));
  showGrid();
}
// 69 — PLANETS: soft-shaded worlds drifting past.
void anim_planets(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<20;i++) fxSplat((fxhash(i*1.7f)-0.5f)*120,(fxhash(i*3.9f)-0.5f)*120,0.3f,0.8f,CRGB(CHSV(150,20,90)));
  fxPlanet(-16+sinf(tf*0.2f)*6,-8+cosf(tf*0.15f)*5,12,20,210,0.8f,-0.3f,(uint16_t)(at*20));
  fxPlanet(20+sinf(tf*0.13f)*5,14+cosf(tf*0.17f)*6,8,150,220,0.6f,-0.5f,(uint16_t)(at*26));
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
// 71 — STARSHIP PANEL: a grid of indicator glows blinking at varied rates.
void anim_shipPanel(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int gy=0;gy<4;gy++)for(int gx=0;gx<5;gx++){ int i=gy*5+gx; float px=-40+gx*20.0f,py=-30+gy*20.0f;
    float rate=0.5f+fxhash(i*2.1f)*3.0f; float ph=fabsf(sinf(tf*rate+fxhash(i*4.4f)*6.28f));
    uint8_t hue=(i%3==0)?0:(i%3==1)?90:150;
    fxSplat(px,py,2.4f,1.1f,hotColor(0.157f+ph*0.843f,hue,16)); }
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
// 74 — ROCKET FIRE: an upward plume of hot particles cooling as they rise.
void anim_rocketFire(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<50;i++){ float ph=fxfract(tf*0.9f+fxhash(i*2.7f));
    float x=(fxhash(i*3.1f)-0.5f)*10.0f*(0.3f+ph); float y=-42.0f+ph*90.0f; float life=1.0f-ph;
    uint8_t hue=(uint8_t)(45*life),val=(uint8_t)(255*life),sat=ph<0.15f?120:255;
    fxSplat(x,y,0.7f+ph*1.4f,1.1f,CRGB(CHSV(hue,sat,val))); }
  fxSplat(0,-42,2.2f,1.3f,CRGB(255,240,210));
  showGrid();
}
// 75 — WARNING LIGHT: a rotating amber beacon beam with a bright core.
void anim_warningLight(uint32_t at){
  clearFrame(); float tf=at*0.001f; float a=tf*3.0f;
  fxSplat(0,0,3.5f,1.4f,CRGB(255,140,0));
  for(int k=1;k<=16;k++){ float rr=k*4.0f; float spread=0.05f*k; float br=(1.0f-k/17.0f);
    for(int j=-1;j<=1;j++){ float aa=a+j*spread; float b2=br*(j==0?1.0f:0.6f);
      fxSplat(cosf(aa)*rr,sinf(aa)*rr,0.8f,1.2f,hotColor(b2,20,0)); } }
  showGrid();
}
// 76 — ALIENS: glowing alien heads with blinking eyes, gently bobbing.
void anim_aliens(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  for(int i=0;i<3;i++){ float cx=-34+i*34.0f; float cy=sinf(tf*1.2f+i)*10.0f;
    fxSplat(cx,cy,9.0f,3.0f,CRGB(CHSV(95,220,70))); fxSplat(cx,cy+3,7.0f,2.0f,CRGB(CHSV(95,220,50)));
    float blink=fabsf(sinf(tf*0.8f+i*1.7f))>0.1f?1.0f:0.15f;
    fxSplat(cx-3.0f,cy-1.0f,1.4f,1.0f,CRGB(CHSV(140,180,(uint8_t)(230*blink))));
    fxSplat(cx+3.0f,cy-1.0f,1.4f,1.0f,CRGB(CHSV(140,180,(uint8_t)(230*blink)))); }
  showGrid();
}
// 77 — SPACE INVADERS: smooth glow-cluster invaders marching (not blocky).
void fxInvader(float cx,float cy,uint8_t hue,int frame){
  CRGB c=CRGB(CHSV(hue,210,255));
  fxSplat(cx,cy,2.0f,1.1f,c); fxSplat(cx-3,cy,1.3f,1.0f,c); fxSplat(cx+3,cy,1.3f,1.0f,c);
  float ay=frame?-2.5f:2.5f; fxSplat(cx-4,cy+ay,1.1f,1.0f,c); fxSplat(cx+4,cy+ay,1.1f,1.0f,c);
}
void anim_spaceInvaders(uint32_t at){
  clearFrame(); float tf=at*0.001f; int frame=((int)(tf*2.0f))&1;
  float march=sinf(tf*0.8f)*10.0f; float drop=fmodf(tf*3.0f,60.0f)-30.0f;
  for(int r=0;r<3;r++)for(int c=0;c<4;c++){ uint8_t hue=r==0?150:r==1?95:30;
    fxInvader(-30+c*20.0f+march,-34+r*13.0f+drop,hue,frame); }
  showGrid();
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
// 81 — SPIRAL EJECT: a spinning spiral flinging glowing particles outward.
void anim_spiralEject(uint32_t at){
  clearFrame(); float tf=at*0.001f; fxSplat(0,0,2.4f,1.2f,CRGB(255,255,255));
  for(int i=0;i<60;i++){ float ph=fxfract(tf*0.5f+fxhash(i*3.7f)); float r=ph*60.0f;
    float ang=fxhash(i*3.7f)*6.2832f+tf*2.0f+ph*3.0f; float br=1.0f-ph;
    fxSplat(cosf(ang)*r,sinf(ang)*r,0.5f+ph*1.0f,1.0f,hotColor(br,(uint8_t)(180+ph*80),0)); }
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
// 83 — PULSING HEART: a smooth heart shape beating.
void anim_pulseHeart(uint32_t at){
  clearFrame(); float tf=at*0.001f; float beat=0.5f+0.5f*fabsf(sinf(tf*2.2f)); float sc=1.0f/(20.0f+beat*6.0f);
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=(pass?other:base)*sc, y=-(pass?base:other)*sc;
      float xx=x*x,yy=y*y; float h=(xx+yy-1.0f); float f3=h*h*h-xx*y*yy;
      float v=f3<0?1.0f:constrain(1.0f-f3*6.0f,0.0f,1.0f);
      if(v>0.03f) fxAdd(pass?false:true,s+1,n,CRGB(CHSV(0,240,(uint8_t)(v*(150+beat*105)))),1.0f);
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
// 87 — HORIZONTAL STRIPES: crisp bands flowing upward across the whole grid.
void anim_hStripesFlow(uint32_t at){
  clearFrame(); float tf=at*0.001f;
  const float period=18.0f, aa=1.2f, on=period*0.55f;      // stripe repeat, ~1-LED edge, 55% duty
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float y=pass?base:other;
      float p=y-tf*22.0f; float m=p-period*floorf(p/period);          // position within one period (0..period)
      float f=constrain(m/aa,0.0f,1.0f)*constrain((on-m)/aa,0.0f,1.0f); // hard band, 1-LED ramp at both edges
      if(f>0.03f) fxAdd(pass?false:true,s+1,n,hotTint(f,150+(uint8_t)(8.0f*sinf(y*0.05f)),24),f);
    } }
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
