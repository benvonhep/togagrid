// Showpiece pack #5 — the next tier (grid 102..106). These reach past the earlier
// packs with techniques not used before: real 3D/4D projection, shape-morphing
// swarms, gravitational coordinate-warping, and a depth-shaded 3D creature.
// Same contract: void anim_<name>(uint32_t at,float dt); ends with showGrid().
// (anim_new3.h is included first, so n3hash/n3dts are visible.)
#pragma once

// ═══════════════════════════════════════════════════════════════════════════
// 102 — TESSERACT
// A 4-dimensional hypercube rotating through TWO 4D planes at once, projected
// 4D→3D→2D. The inner cube swells through the outer as the fourth axis turns —
// the "impossible" motion the eye can't quite resolve. Pure structure, hypnotic.
// ═══════════════════════════════════════════════════════════════════════════
void anim_tesseract(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float A=tf*0.5f, B=tf*0.37f, Cc=tf*0.23f;
  float px[16],py[16],pz[16];
  for(int i=0;i<16;i++){
    float x=(i&1)?1.0f:-1.0f, y=(i&2)?1.0f:-1.0f, z=(i&4)?1.0f:-1.0f, w=(i&8)?1.0f:-1.0f;
    float x1=x*cosf(A)-w*sinf(A), w1=x*sinf(A)+w*cosf(A);   // rotate X-W plane
    float y1=y*cosf(B)-z*sinf(B), z1=y*sinf(B)+z*cosf(B);   // rotate Y-Z plane
    float x2=x1*cosf(Cc)-y1*sinf(Cc), y2=x1*sinf(Cc)+y1*cosf(Cc);  // rotate X-Y
    float kw=2.4f/(3.0f-w1);                                // 4D→3D perspective
    float X=x2*kw, Y=y2*kw, Z=z1*kw;
    float kz=2.2f/(3.0f-Z);                                 // 3D→2D perspective
    px[i]=X*kz*26.0f; py[i]=Y*kz*26.0f; pz[i]=Z;
  }
  for(int i=0;i<16;i++) for(int j=i+1;j<16;j++){
    int d=i^j; if(d&(d-1)) continue;                        // edge = differ in exactly one axis
    float depth=(pz[i]+pz[j])*0.5f, b=0.35f+0.65f*(0.5f+0.5f*depth);
    fxStreak(px[i],py[i],px[j],py[j],0.45f,1.0f, CRGB(CHSV((uint8_t)(150+depth*30+tf*20),200,(uint8_t)(b*220))));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 103 — TORUS KNOT
// A glowing (2,3) torus knot — a single closed strand that loops through itself
// in 3D — turning slowly on two axes, its hue running along the cord and its
// brightness reading depth. Sculptural, clean, mesmerising.
// ═══════════════════════════════════════════════════════════════════════════
void anim_torusKnot(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float ra=tf*0.4f, rb=tf*0.3f; const int N=112;
  float px=0,py=0; bool first=true;
  for(int i=0;i<=N;i++){
    float u=(float)i/N*6.2832f, r=2.0f+cosf(3.0f*u);
    float x=r*cosf(2.0f*u), y=r*sinf(2.0f*u), z=sinf(3.0f*u);
    float x1=x*cosf(ra)-z*sinf(ra), z1=x*sinf(ra)+z*cosf(ra);        // yaw
    float y1=y*cosf(rb)-z1*sinf(rb), z2=y*sinf(rb)+z1*cosf(rb);      // pitch
    float kz=2.4f/(4.6f-z2);                                         // depth
    float sx=x1*kz*10.0f, sy=y1*kz*10.0f;
    if(!first){
      float b=constrain(0.3f+0.7f*(kz-0.55f)*2.2f,0.0f,1.0f);
      fxStreak(px,py,sx,sy,0.5f,1.0f, CRGB(CHSV((uint8_t)(u*40+tf*20),220,(uint8_t)(b*230))));
    }
    px=sx; py=sy; first=false;
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 104 — MORPH SWARM
// A cloud of points that never rests — it eases from one recognisable figure to
// the next (ring → heart → star → spiral → knot) and back, each point flaring
// brighter the faster it flies. Watching it guess the next shape is the hook.
// ═══════════════════════════════════════════════════════════════════════════
#define MS_N 64
static float msx[MS_N], msy[MS_N]; static bool msInit=false;
static void msTarget(int shape,int i,float& tx,float& ty){
  float u=(float)i/MS_N;
  if(shape==0){ float a=u*6.2832f; tx=cosf(a)*42.0f; ty=sinf(a)*42.0f; }               // ring
  else if(shape==1){ float a=u*6.2832f;                                                 // heart
    tx=16.0f*sinf(a)*sinf(a)*sinf(a)*2.6f;
    ty=-(13.0f*cosf(a)-5.0f*cosf(2*a)-2.0f*cosf(3*a)-cosf(4*a))*2.6f; }
  else if(shape==2){ float a=u*6.2832f*2.0f; float r=(i&1)?18.0f:44.0f; tx=cosf(a)*r; ty=sinf(a)*r; } // star
  else if(shape==3){ float a=u*6.2832f*3.0f, r=u*46.0f; tx=cosf(a)*r; ty=sinf(a)*r; }   // spiral
  else { float a=u*6.2832f; tx=cosf(3.0f*a)*44.0f; ty=sinf(2.0f*a)*44.0f; }             // lissajous knot
}
void anim_morphSwarm(uint32_t at,float dt){
  float dts=n3dts(dt,0.1f); float tf=at*0.001f;
  if(!msInit){ for(int i=0;i<MS_N;i++) msTarget(0,i,msx[i],msy[i]); msInit=true; }
  float cyc=tf*0.25f; int sA=((int)cyc)%5, sB=(sA+1)%5;
  float f=cyc-floorf(cyc), e=f<0.5f?4.0f*f*f*f:1.0f-powf(-2.0f*f+2.0f,3.0f)*0.5f;   // ease in/out
  float lerp=constrain(6.0f*dts,0.0f,1.0f);
  clearFrame();
  for(int i=0;i<MS_N;i++){
    float ax,ay,bx,by; msTarget(sA,i,ax,ay); msTarget(sB,i,bx,by);
    float tx=ax+(bx-ax)*e, ty=ay+(by-ay)*e;
    float mvx=tx-msx[i], mvy=ty-msy[i];
    msx[i]+=mvx*lerp; msy[i]+=mvy*lerp;
    float b=0.5f+constrain((fabsf(mvx)+fabsf(mvy))*0.05f,0.0f,0.5f);   // flare while moving
    fxSplat(msx[i],msy[i],0.7f,1.2f, CRGB(CHSV((uint8_t)(i*4+tf*30),200,(uint8_t)(b*230))));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 105 — GRAVITATIONAL LENS
// A drifting black hole bends a field of background stars around itself — stars
// near it smear outward into a shimmering Einstein ring, a thin photon ring circles
// the dark shadow. Real light-bending, warping the whole starfield as it passes.
// ═══════════════════════════════════════════════════════════════════════════
#define GL_N 72
void anim_gravitationalLens(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float hx=sinf(tf*0.30f)*30.0f, hy=cosf(tf*0.23f)*24.0f; const float Rs=10.0f;
  for(int i=0;i<GL_N;i++){
    float sx=(n3hash(i*1.7f)-0.5f)*130.0f, sy=(n3hash(i*3.1f+5.0f)-0.5f)*130.0f;
    float dx=sx-hx, dy=sy-hy, r=sqrtf(dx*dx+dy*dy)+0.001f;
    if(r<Rs*0.6f) continue;                                    // swallowed → the dark shadow
    float defl=Rs*Rs/r;                                        // deflection ∝ 1/r
    float ax=hx+dx/r*(r+defl), ay=hy+dy/r*(r+defl);
    float tw=0.5f+0.5f*sinf(tf*3.0f+i);
    fxSplat(ax,ay,0.5f,1.0f, CRGB(CHSV(200,80,(uint8_t)(constrain(tw*(0.4f+defl*0.05f),0.0f,1.0f)*200.0f))));
  }
  fxRing(hx,hy,Rs*1.6f,0.5f,1.4f, CRGB(CHSV(40,80,90)));       // Einstein-ring shimmer
  fxRing(hx,hy,Rs,0.5f,1.0f, CRGB(255,220,180));               // photon ring
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 106 — CELESTIAL DRAGON
// A long serpent-dragon swims a 3D path through the dark — its body a follow-the-
// leader chain, depth read as thickness and brightness so it coils toward and away
// from you, spine glinting, glowing head leading. The octopus's big cousin.
// ═══════════════════════════════════════════════════════════════════════════
#define DR_TRAIL 84
static float drx[DR_TRAIL], dry[DR_TRAIL], drz[DR_TRAIL]; static int drHead=0; static bool drInit=false;
void anim_celestialDragon(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  if(!drInit){ for(int i=0;i<DR_TRAIL;i++){ drx[i]=dry[i]=drz[i]=0.0f; } drInit=true; }
  drHead=(drHead+1)%DR_TRAIL;                                   // push a new head sample
  drx[drHead]=sinf(tf*0.7f)*38.0f+sinf(tf*1.7f)*10.0f;
  dry[drHead]=cosf(tf*0.9f)*30.0f+cosf(tf*2.1f)*8.0f;
  drz[drHead]=sinf(tf*0.5f)*1.2f;
  clearFrame();
  float ppx=0,ppy=0; bool first=true;
  for(int k=DR_TRAIL-1;k>=0;k--){                               // tail → head
    int idx=(drHead-k+DR_TRAIL*2)%DR_TRAIL;
    float z=drz[idx], kz=2.6f/(3.6f-z);                         // depth
    float sx=drx[idx]*kz, sy=dry[idx]*kz, u=(float)k/DR_TRAIL;  // u: 0 head .. 1 tail
    float thick=0.4f+(1.0f-u)*1.4f;
    float b=constrain((0.3f+0.7f*(1.0f-u))*(0.6f+0.4f*(0.5f+0.5f*z)),0.0f,1.0f);
    if(!first) fxStreak(ppx,ppy,sx,sy,thick,1.2f, CRGB(CHSV((uint8_t)(150+u*60+tf*15),200,(uint8_t)(b*220))));
    if(k%6==0) fxSplat(sx,sy,thick*0.6f,1.0f, CRGB(CHSV((uint8_t)(190+u*60),120,180)));   // spine glints
    ppx=sx; ppy=sy; first=false;
  }
  float kz=2.6f/(3.6f-drz[drHead]);
  fxSplat(drx[drHead]*kz,dry[drHead]*kz,2.4f,1.6f, CRGB(CHSV(150,120,240)));   // head
  showGrid();
}
