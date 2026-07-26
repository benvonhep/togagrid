// Showpiece pack #3 — 15 brand-new, self-contained modes (grid 72..86), each a
// DISTINCT vibe. Same rules as anim_new2.h: #included into the main sketch AFTER
// the globals; every function ends with showGrid() so the shared hue-rotation /
// AGC / coverage pipeline runs. Signature: void anim_<name>(uint32_t at,float dt)
//   at = speed-scaled time (ms) — drives motion.  dt = per-frame delta (ms).
// The panel is a 12x12 LATTICE of light strips (world coords ~ -60..+60): only the
// strips light, so broad radial/rotational strokes read best. Toolkit lives in
// grid_draw.h: fxSplat/fxStreak/fxRing/fxNebula/fxPlanet/hotColor + fxAdd/fillCell.
#pragma once

// Local deterministic hash (own name, no clash with n2hash). Returns 0..1.
static inline float n3hash(float n){ float s = sinf(n*12.9898f)*43758.5453f; return s - floorf(s); }
// Clamp a mode-entry dt jump into a safe integration step.
static inline float n3dts(float dt,float cap){ float d=dt*0.001f; if(d<0)d=0; if(d>cap)d=cap; return d; }

// ═══════════════════════════════════════════════════════════════════════════
// 72 — JELLYFISH BLOOM
// A translucent bell that pulses (contract → jet), drifting slowly, trailing a
// fan of tentacles that undulate — each on its OWN phase & speed, so they ripple
// past one another. Soft, alive, hypnotic.
// ═══════════════════════════════════════════════════════════════════════════
void anim_jellyfishBloom(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  float cx    = sinf(tf*0.30f)*18.0f;
  float cy    = sinf(tf*0.23f+1.0f)*14.0f - 6.0f;   // slow drift, biased up
  float pulse = 0.5f-0.5f*cosf(tf*1.6f);            // 0 relaxed .. 1 contracted
  float bellR = 12.0f + pulse*8.0f;
  clearFrame();
  // translucent dome glow + bright rim
  fxSplat(cx,cy-bellR*0.25f, bellR*0.55f, 3.0f, CRGB(CHSV(150,150,(uint8_t)(70+pulse*70))));
  fxRing (cx,cy,bellR,1.3f,1.6f, CRGB(CHSV(150,190,220)));
  // undulating tentacles hanging below, differential speed per strand
  const int NT=7;
  for(int i=0;i<NT;i++){
    float baseA = (i-(NT-1)/2.0f)*0.30f;             // fan angle around straight-down
    float px=cx+sinf(baseA)*bellR*0.7f, py=cy+bellR*0.45f;
    float ph=tf*(1.3f+i*0.14f)+i*1.7f;               // each ripples at its own rate
    const int seg=6; float len=18.0f+pulse*10.0f;
    for(int s=1;s<=seg;s++){
      float u=(float)s/seg;
      float sway = sinf(ph+u*4.2f)*(3.0f+u*7.0f);
      float nx=cx+sinf(baseA)*(bellR*0.7f+u*6.0f)+sway;
      float ny=py+u*len;
      float b=1.0f-u*0.65f;
      fxStreak(px,py,nx,ny,0.5f,1.1f, CRGB(CHSV((uint8_t)(150+u*40),210,(uint8_t)(b*165))));
      px=nx; py=ny;
    }
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 73 — KALEIDO WHEEL
// A true kaleidoscope: coloured gems repeated with 6-fold symmetry AND a mirror
// image, each orbit ring spinning at a different rate so the facets slide through
// one another. Sharp, jewelled, endlessly reconfiguring.
// ═══════════════════════════════════════════════════════════════════════════
void anim_kaleidoWheel(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  const int S=6; float rot=tf*0.5f;
  for(int g=0;g<5;g++){
    float gr = 9.0f+g*9.5f + sinf(tf*0.8f+g)*4.0f;
    float ga = tf*(0.30f+g*0.20f);                   // differential spin per ring
    uint8_t hue=(uint8_t)(g*42+tf*20);
    float sz = 1.1f + (g==0?1.4f:0.0f);
    for(int s=0;s<S;s++){
      float base=s*(6.2832f/S);
      float a1=rot+ga+base;  fxSplat(cosf(a1)*gr,sinf(a1)*gr,sz,1.3f, CRGB(CHSV(hue,225,205)));
      float a2=rot-ga-base;  fxSplat(cosf(a2)*gr,sinf(a2)*gr,sz,1.3f, CRGB(CHSV((uint8_t)(hue+80),225,175)));
    }
    if(g<4) fxRing(0,0,gr,0.35f,0.9f, CRGB(CHSV(hue,180,45)));
  }
  fxSplat(0,0,2.0f+sinf(tf*2.0f),1.6f, CRGB(255,255,255));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 74 — LIGHTNING TREE
// A fractal bolt cracks across the panel — a jagged trunk that throws random
// branches — strikes white-bright, then flickers out. Rebuilt fresh every strike
// so it NEVER repeats.
// ═══════════════════════════════════════════════════════════════════════════
#define LT_MAX 42
struct LtSeg{ float x0,y0,x1,y1; };
static LtSeg ltSeg[LT_MAX]; static int ltN=0;
static float ltAge=0, ltLife=0.5f; static bool ltInit=false;
static void ltBuild(){
  ltN=0;
  float x=(frand()-0.5f)*46.0f, y=-58.0f, dirx=(frand()-0.5f)*0.4f;
  while(y<56.0f && ltN<LT_MAX){
    float nx=x+dirx*8.0f+(frand()-0.5f)*11.0f, ny=y+7.0f+frand()*5.0f;
    ltSeg[ltN++]=(LtSeg){x,y,nx,ny};
    if(frand()<0.42f && ltN<LT_MAX){                 // throw a branch
      float bx=nx+(frand()-0.5f)*24.0f, by=ny+7.0f+frand()*11.0f;
      ltSeg[ltN++]=(LtSeg){nx,ny,bx,by};
    }
    x=nx; y=ny; dirx+=(frand()-0.5f)*0.25f;
  }
}
void anim_lightningTree(uint32_t at,float dt){
  float dts=n3dts(dt,0.1f);
  if(!ltInit){ ltBuild(); ltAge=0; ltInit=true; }
  ltAge+=dts;
  if(ltAge>ltLife){ ltBuild(); ltAge=0; ltLife=0.35f+frand()*0.65f; }
  clearFrame();
  float f=1.0f-ltAge/ltLife;
  float fl=(ltAge<0.06f)?1.0f:(0.25f+0.75f*f)*(0.7f+0.3f*sinf(at*0.05f));
  uint8_t bri=(uint8_t)(constrain(fl,0.0f,1.0f)*255.0f);
  for(int i=0;i<ltN;i++)
    fxStreak(ltSeg[i].x0,ltSeg[i].y0,ltSeg[i].x1,ltSeg[i].y1,0.5f,1.2f, CRGB(CHSV(160,110,bri)));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 75 — DNA HELIX
// Two phosphorescent strands twisting in 3D with rungs between them; depth is read
// as brightness/size so the coils appear to rotate toward and away from you.
// ═══════════════════════════════════════════════════════════════════════════
void anim_dnaHelix(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float phase=tf*1.2f; const int N=22; const float r=22.0f;
  for(int i=0;i<=N;i++){
    float u=(float)i/N, y=-56.0f+u*112.0f, ang=phase+u*7.0f;
    float z1=sinf(ang), z2=sinf(ang+3.14159f);
    float x1=cosf(ang)*r, x2=cosf(ang+3.14159f)*r;
    float b1=0.35f+0.65f*(0.5f+0.5f*z1), b2=0.35f+0.65f*(0.5f+0.5f*z2);
    fxSplat(x1,y,0.7f+0.6f*(0.5f+0.5f*z1),1.1f, CRGB(CHSV(170,220,(uint8_t)(b1*255))));
    fxSplat(x2,y,0.7f+0.6f*(0.5f+0.5f*z2),1.1f, CRGB(CHSV( 20,220,(uint8_t)(b2*255))));
    if(i%2==0)
      fxStreak(x1,y,x2,y,0.4f,1.0f, CRGB(CHSV((uint8_t)(90+40*sinf(tf+u*6.0f)),200,90)));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 76 — FIREFLIES
// A warm swarm that wanders and glows out of phase, then slowly slides into
// synchrony — the whole field blinking as one — and drifts apart again. Alive.
// ═══════════════════════════════════════════════════════════════════════════
#define FF_N 26
struct Ff{ float x,y,vx,vy,ph,fr; };
static Ff ffP[FF_N]; static bool ffInit=false;
void anim_fireflies(uint32_t at,float dt){
  float dts=n3dts(dt,0.1f); float tf=at*0.001f;
  if(!ffInit){
    for(int i=0;i<FF_N;i++){ float a=frand()*6.2832f,s=6.0f+frand()*8.0f;
      ffP[i]=(Ff){(frand()-0.5f)*100,(frand()-0.5f)*100,cosf(a)*s,sinf(a)*s,frand()*6.2832f,1.6f+frand()*0.8f}; }
    ffInit=true;
  }
  float sync=0.5f+0.5f*sinf(tf*0.15f);               // global coherence rises & falls
  clearFrame();
  for(int i=0;i<FF_N;i++){
    ffP[i].vx += (frand()-0.5f)*8.0f*dts - ffP[i].x*0.02f*dts;
    ffP[i].vy += (frand()-0.5f)*8.0f*dts - ffP[i].y*0.02f*dts;
    float sp=sqrtf(ffP[i].vx*ffP[i].vx+ffP[i].vy*ffP[i].vy); if(sp>16.0f){ ffP[i].vx*=16.0f/sp; ffP[i].vy*=16.0f/sp; }
    ffP[i].x+=ffP[i].vx*dts; ffP[i].y+=ffP[i].vy*dts;
    if(ffP[i].x<-58||ffP[i].x>58) ffP[i].vx=-ffP[i].vx;
    if(ffP[i].y<-58||ffP[i].y>58) ffP[i].vy=-ffP[i].vy;
    ffP[i].ph += ffP[i].fr*dts*6.2832f;
    float glow=0.5f+0.5f*sinf(ffP[i].ph);
    float g=glow*(1.0f-sync)+(0.5f+0.5f*sinf(tf*3.0f))*sync; g*=g;
    fxSplat(ffP[i].x,ffP[i].y,0.5f+g*0.6f,1.0f, CRGB(CHSV(50,200,(uint8_t)(g*230))));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 77 — BLACK-HOLE ACCRETION
// A tilted disk of matter spiralling in — inner orbits whirl faster (Keplerian),
// the approaching side is Doppler-brightened, a thin photon ring circles the dark
// shadow at the centre. Rotational, physical, ominous.
// ═══════════════════════════════════════════════════════════════════════════
#define BA_N 60
struct BaP{ float ang,rad; uint8_t hue; };
static BaP baP[BA_N]; static bool baInit=false;
void anim_blackHoleAccretion(uint32_t at,float dt){
  float dts=n3dts(dt,0.1f);
  if(!baInit){ for(int i=0;i<BA_N;i++) baP[i]=(BaP){frand()*6.2832f,10.0f+frand()*40.0f,(uint8_t)(frand()*40)}; baInit=true; }
  clearFrame();
  for(int i=0;i<BA_N;i++){
    float w=2.6f/(baP[i].rad*0.02f+0.4f);            // inner faster
    baP[i].ang+=w*dts; baP[i].rad-=2.0f*dts;
    if(baP[i].rad<5.0f){ baP[i].rad=44.0f+frand()*10.0f; baP[i].ang=frand()*6.2832f; }
    float ca=cosf(baP[i].ang), sa=sinf(baP[i].ang);
    float x=ca*baP[i].rad, y=sa*baP[i].rad*0.4f;      // disk squashed for perspective
    float dopp=0.5f+0.5f*ca;                          // one side brightened
    float heat=constrain(1.0f-baP[i].rad/50.0f,0.0f,1.0f);
    float b=(0.3f+0.7f*dopp)*(0.4f+0.6f*heat);
    fxSplat(x,y,0.6f+heat*0.7f,1.1f, hotColor(b,(uint8_t)(baP[i].hue+heat*20),30,60));
  }
  fxRing(0,0,8.0f,0.6f,1.0f, CRGB(255,220,180));       // photon ring around the dark shadow
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 78 — OCEAN CAUSTICS
// The rippling net of sunlight on a pool floor: two drifting noise fields
// multiplied and sharpened into bright liquid veins that crawl and shimmer.
// ═══════════════════════════════════════════════════════════════════════════
void anim_oceanCaustics(uint32_t at,float dt){
  (void)dt;
  clearFrame();
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=pass?other:base,y=pass?base:other;
      uint8_t n1=inoise8((uint16_t)((x+80)*140),(uint16_t)((y+80)*140),(uint16_t)(at*10));
      uint8_t n2=inoise8((uint16_t)((x+80)*150+3000),(uint16_t)((y+80)*150),(uint16_t)(at*13));
      float v=(n1/255.0f)*(n2/255.0f); v=v*v*v*5.0f; if(v>1.0f)v=1.0f;   // sharpen into veins
      if(v>0.03f) fxAdd(pass?false:true,s+1,n, CRGB(CHSV((uint8_t)(120+30*(n1/255.0f)),200,255)), v);
    } }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 79 — PHOENIX RISE
// A firebird beating molten wings, body glowing white-hot, tail feathers of flame
// streaming and flickering behind it. Warm, fierce, mythic.
// ═══════════════════════════════════════════════════════════════════════════
void anim_phoenixRise(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float cx=sinf(tf*0.4f)*8.0f, cy=sinf(tf*0.7f)*6.0f;
  float flap=sinf(tf*1.4f);                            // wing beat
  fxSplat(cx,cy,3.0f,2.0f, hotColor(0.9f,20,30,40));   // body
  fxSplat(cx,cy-8.0f,1.6f,1.4f, hotColor(0.8f,24,20,50)); // head
  const int seg=8;
  for(int side=-1;side<=1;side+=2){                    // two wings
    float px=cx,py=cy;
    for(int s=1;s<=seg;s++){
      float u=(float)s/seg;
      float wx=cx+side*u*34.0f, wy=cy-flap*u*20.0f+u*u*8.0f;
      fxStreak(px,py,wx,wy,0.7f,1.3f, hotColor(1.0f-u*0.5f,(uint8_t)(18+u*20),30,50));
      px=wx; py=wy;
    }
  }
  for(int i=-1;i<=1;i++){                              // flaming tail feathers
    float tx=cx+i*6.0f+sinf(tf*3.0f+i)*3.0f, ty=cy+10.0f;
    float ex=cx+i*10.0f+sinf(tf*4.0f+i)*5.0f, ey=cy+34.0f+sinf(tf*2.0f+i)*4.0f;
    fxStreak(tx,ty,ex,ey,0.6f,1.2f, hotColor(0.7f,14,26,40));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 80 — CLOCKWORK GEARS
// Interlocking brass gears, each turning at its own rate (meshing pairs spin
// opposite ways), teeth and spokes wheeling. Mechanical, precise, satisfying.
// ═══════════════════════════════════════════════════════════════════════════
void anim_clockworkGears(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  struct G{ float cx,cy,r,w; int teeth; uint8_t hue; };
  static const G g[5]={ {0,0,20,0.8f,10,30},{-34,-10,13,-1.4f,7,150},{30,14,16,-1.0f,8,90},
                        {8,-34,11,1.6f,6,200},{-20,32,12,1.2f,7,60} };
  for(int i=0;i<5;i++){
    float a=tf*g[i].w;
    fxRing(g[i].cx,g[i].cy,g[i].r,1.0f,1.3f, CRGB(CHSV(g[i].hue,200,150)));
    fxRing(g[i].cx,g[i].cy,g[i].r*0.35f,0.7f,1.1f, CRGB(CHSV(g[i].hue,180,120)));
    for(int t=0;t<g[i].teeth;t++){                     // teeth
      float ta=a+t*(6.2832f/g[i].teeth), c=cosf(ta), s=sinf(ta);
      fxStreak(g[i].cx+c*g[i].r,g[i].cy+s*g[i].r, g[i].cx+c*(g[i].r+3.0f),g[i].cy+s*(g[i].r+3.0f),0.6f,1.0f, CRGB(CHSV(g[i].hue,160,200)));
    }
    for(int sp=0;sp<3;sp++){                            // spokes
      float sa=a+sp*2.094f;
      fxStreak(g[i].cx,g[i].cy, g[i].cx+cosf(sa)*g[i].r,g[i].cy+sinf(sa)*g[i].r,0.4f,1.0f, CRGB(CHSV(g[i].hue,180,80)));
    }
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 81 — MAGNETIC DIPOLE
// The looping field of a bar magnet — nested lines arcing from a red pole to a
// blue one — with the whole dipole slowly rotating and the lines shimmering as if
// charged. Elegant physics.
// ═══════════════════════════════════════════════════════════════════════════
void anim_magneticDipole(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float sep=26.0f, rot=tf*0.4f;
  float ax=cosf(rot)*sep, ay=sinf(rot)*sep, bx=-ax, by=-ay;
  float px=bx-ax, py=by-ay, pl=sqrtf(px*px+py*py)+0.01f, nx=-py/pl, ny=px/pl;
  for(int side=-1;side<=1;side+=2) for(int k=1;k<=5;k++){
    float bulge=k*10.0f*side;
    float qx=(ax+bx)*0.5f+nx*bulge, qy=(ay+by)*0.5f+ny*bulge;   // bezier control point
    float prevx=ax,prevy=ay; float shim=0.6f+0.4f*sinf(tf*3.0f-k*0.6f);
    for(int s=1;s<=10;s++){ float u=(float)s/10, um=1.0f-u;
      float x=um*um*ax+2*um*u*qx+u*u*bx, y=um*um*ay+2*um*u*qy+u*u*by;
      fxStreak(prevx,prevy,x,y,0.4f,1.0f, CRGB(CHSV((uint8_t)(160-k*8),180,(uint8_t)(shim*120))));
      prevx=x; prevy=y;
    }
  }
  fxSplat(ax,ay,2.0f,1.6f, CRGB(CHSV(0,220,220)));      // N (red)
  fxSplat(bx,by,2.0f,1.6f, CRGB(CHSV(160,220,220)));    // S (blue)
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 82 — QUANTUM ORBITALS
// Electron probability clouds morphing between shells — one lobe (s) → two (p) →
// four (d) — the whole thing rotating, the cloud shimmering with quantum
// uncertainty. Scientific and strange.
// ═══════════════════════════════════════════════════════════════════════════
void anim_quantumOrbitals(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float morph=fmodf(tf*0.15f,3.0f);
  int lobes=(morph<1.0f)?1:(morph<2.0f?2:4);
  float rot=tf*0.5f, pulse=0.6f+0.4f*sinf(tf*2.0f);
  for(int l=0;l<lobes;l++){
    float a=rot+l*(6.2832f/lobes);
    float dist=(lobes==1)?0.0f:24.0f;
    float lx=cosf(a)*dist, ly=sinf(a)*dist;
    uint8_t hue=(uint8_t)(180+l*30);
    for(int p=0;p<10;p++){                             // fuzzy probability cloud
      float rr=frand(), pa=frand()*6.2832f, rad=rr*12.0f*pulse;
      fxSplat(lx+cosf(pa)*rad,ly+sinf(pa)*rad,0.8f,2.0f, CRGB(CHSV(hue,200,(uint8_t)((1.0f-rr)*140))));
    }
  }
  fxSplat(0,0,1.4f,1.4f, CRGB(255,255,255));            // nucleus
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 83 — SERPENT NEST
// A tangle of snakes gliding back and forth at different heights and speeds,
// bodies rippling with a travelling wave, bright heads leading. They slide across
// and past each other — a living knot.
// ═══════════════════════════════════════════════════════════════════════════
#define SN_N 5
void anim_serpentNest(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  for(int k=0;k<SN_N;k++){
    float spd=0.5f+k*0.18f; uint8_t hue=(uint8_t)(k*45+60);
    float ph=tf*spd*0.5f+k*1.3f, m=fmodf(ph,2.0f), tri=fabsf(m-1.0f);
    float hx=-58.0f+(1.0f-tri)*116.0f, dir=(m<1.0f)?-1.0f:1.0f;
    float baseY=-38.0f+k*19.0f; const int seg=14; float px=hx,py=baseY;
    for(int s=0;s<=seg;s++){
      float x=hx+dir*s*6.0f, y=baseY+sinf(s*0.6f-tf*spd*4.0f)*13.0f;
      float b=0.3f+0.7f*(1.0f-(float)s/seg);
      if(s>0) fxStreak(px,py,x,y,0.6f,1.1f, CRGB(CHSV(hue,220,(uint8_t)(b*210))));
      else    fxSplat(x,y,1.5f,1.2f, CRGB(CHSV(hue,170,235)));
      px=x; py=y;
    }
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 84 — CRYSTAL GROWTH
// Seed points nucleate and shoot faceted spikes outward, glinting at the tips,
// growing to full bloom then dissolving as a new set nucleates elsewhere.
// ═══════════════════════════════════════════════════════════════════════════
#define CG_N 6
struct Cg{ float cx,cy,seed; uint8_t hue; };
static Cg cgP[CG_N]; static bool cgInit=false; static float cgAge=0;
static void cgSeed(){ for(int i=0;i<CG_N;i++) cgP[i]=(Cg){(frand()-0.5f)*80,(frand()-0.5f)*80,frand()*10.0f,(uint8_t)(frand()*255)}; }
void anim_crystalGrowth(uint32_t at,float dt){
  float dts=n3dts(dt,0.1f); (void)at;
  if(!cgInit){ cgSeed(); cgAge=0; cgInit=true; }
  cgAge+=dts*0.3f;
  float grow=0.5f-0.5f*cosf(cgAge*3.14159f);
  if(cgAge>2.0f){ cgAge=0; cgSeed(); }
  clearFrame();
  for(int i=0;i<CG_N;i++){
    const int spikes=6;
    for(int s=0;s<spikes;s++){
      float a=cgP[i].seed+s*(6.2832f/spikes);
      float len=grow*(10.0f+n3hash(cgP[i].seed+s)*14.0f);
      float ex=cgP[i].cx+cosf(a)*len, ey=cgP[i].cy+sinf(a)*len;
      fxStreak(cgP[i].cx,cgP[i].cy,ex,ey,0.5f,1.0f, CRGB(CHSV(cgP[i].hue,180,(uint8_t)(grow*200))));
      fxSplat(ex,ey,0.6f,1.0f, CRGB(CHSV(cgP[i].hue,110,(uint8_t)(grow*235))));
    }
    fxSplat(cgP[i].cx,cgP[i].cy,1.0f*grow,1.2f, CRGB(CHSV(cgP[i].hue,80,(uint8_t)(grow*255))));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 85 — SOLAR FLARE
// A churning star with a coronal loop that arcs off the surface, swells, and
// collapses back — anchored to a slowly rotating spot. Grand and hot.
// ═══════════════════════════════════════════════════════════════════════════
void anim_solarFlare(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  fxNebula(at,20,230,200.0f,30.0f,30.0f,1.1f);         // churning surface
  fxSplat(0,0,14.0f,4.0f, hotColor(0.8f,24,20,40));
  for(int i=0;i<10;i++){ float a=tf*0.5f+i, r=8.0f+n3hash(i*2.1f)*18.0f;   // granulation
    fxSplat(cosf(a)*r,sinf(a)*r,1.2f,1.3f, hotColor(0.9f,30,20,30)); }
  float amp=sinf(fmodf(tf*0.25f,1.0f)*3.14159f);       // loop grows then collapses
  float rootA=tf*0.2f, r0=14.0f;
  float ax=cosf(rootA)*r0, ay=sinf(rootA)*r0, bx=cosf(rootA+0.6f)*r0, by=sinf(rootA+0.6f)*r0;
  float mx=(ax+bx)*0.5f, my=(ay+by)*0.5f, ml=sqrtf(mx*mx+my*my)+0.01f;
  float apx=mx/ml*(r0+amp*34.0f), apy=my/ml*(r0+amp*34.0f);
  float prevx=ax,prevy=ay;
  for(int s=1;s<=12;s++){ float u=(float)s/12, um=1.0f-u;
    float x=um*um*ax+2*um*u*apx+u*u*bx, y=um*um*ay+2*um*u*apy+u*u*by;
    fxStreak(prevx,prevy,x,y,0.7f,1.3f, hotColor(0.7f+0.3f*amp,28,26,45)); prevx=x; prevy=y;
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 86 — MANDALA BLOOM
// Concentric rings of petals unfurl and breathe, adjacent layers counter-rotating
// so the pattern is forever reweaving itself. Hypnotic, symmetrical, meditative.
// ═══════════════════════════════════════════════════════════════════════════
void anim_mandalaBloom(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float breath=0.5f-0.5f*cosf(tf*0.6f);
  for(int ring=0;ring<4;ring++){
    int petals=6+ring*2;
    float rr=8.0f+ring*11.0f+breath*6.0f;
    float rot=tf*(0.3f+ring*0.15f)*((ring%2)?-1.0f:1.0f);
    uint8_t hue=(uint8_t)(ring*40+tf*15);
    for(int p=0;p<petals;p++){
      float a=rot+p*(6.2832f/petals), c=cosf(a), s=sinf(a);
      fxStreak(c*(rr-4.0f),s*(rr-4.0f),c*rr,s*rr,0.5f,1.0f, CRGB(CHSV(hue,220,180)));
      fxSplat(c*rr,s*rr,0.9f,1.1f, CRGB(CHSV((uint8_t)(hue+20),180,200)));
    }
    fxRing(0,0,rr-4.0f,0.4f,0.9f, CRGB(CHSV(hue,200,60)));
  }
  fxSplat(0,0,2.0f+breath*2.0f,1.6f, CRGB(255,240,255));
  showGrid();
}
