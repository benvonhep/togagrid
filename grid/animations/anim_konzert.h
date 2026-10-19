// ═══════════════════════════════════════════════════════════════════════════
//  Drahthaus concert pack — "Elephant in the Room" (grid modes 107..122)
//
//  107 konzertMarker       setlist marker: cover scene — neon submarine + 4 divers over the desert, tubes striking on
//  108 dh100Elefanten      pink elephants (100 of them, all different) crossing a prairie
//  109 (→ anim_mitosis)    Where Did You Sleep Last Night — Togalicht mode 8
//  110 dhEcholot           sonar: square pings, a turning beam, echo blips
//  111 dhCryMeARiver       a river streaming down, tears falling and splashing
//  112 dhNichtErschrecken  thunderstorm: random forked lightning, white/blue/violet/UV
//  113 (→ anim_shockwaveChain) Alle gegen alle — Togalicht mode 10
//  114 dhSabbia1           sandstorm in orange / yellow / red
//  115 (→ anim_vortex)     Sabbia II — Togalicht mode 3
//  116 dhMuecke            one mosquito; the swat kills it, the next one flies in
//  117 dhRoentgenauge      x-ray eye: lids that really close, amber iris, hot pupil
//  118 (→ anim_quadMirror) Dienstag (noch immer dieser Rave)
//  119 dhSong12            placeholder: square tunnel (swap once song 12 is known)
//  120 quadMirror          DJ: 4-fold mirrored random blocks, crossfading
//  121 squareSpiral        DJ: a square spiral drawing itself in, then erasing
//  122 gridPulse           DJ: checkerboard pulse, parity flips per beat
//
//  Same contract as every other pack: void anim_<name>(uint32_t at,float dt), `at`
//  is speed-scaled virtual time, `dt` the scaled frame delta; end with showGrid().
//  All modes share ONE init flag (kzInit) that resetAnimState() clears on every
//  mode change, so each mode re-seeds its own statics on entry. Everything here
//  runs endlessly without a visible restart: processions spawn as they go, wars
//  never reset wholesale, storms are random.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once

static bool kzInit=false;   // cleared by resetAnimState() — every mode below re-seeds when false

// ── shared helpers ───────────────────────────────────────────────────────────
// Per-pixel field pass over all 24 strips in world coords (x,y ≈ ±60). The
// functor fills `c` and returns true to light the pixel.
template<typename F> static inline void kzField(F f){
  CRGB c;
  for(int xi=0;xi<12;xi++){ float x=X_POS[xi];
    for(int n=1;n<=121;n++){ float y=(float)n-CENTER; if(f(x,y,c)) setLED(true,xi+1,n,c); } }
  for(int yi=0;yi<12;yi++){ float y=Y_POS[yi];
    for(int n=1;n<=121;n++){ float x=(float)n-CENTER; if(f(x,y,c)) setLED(false,yi+1,n,c); } }
}
// Beat clock: phase 0..1 within the current beat, plus a one-frame edge flag and a
// running count. Locked to the tapped tempo (real time, like the glow) when one is
// set, else free-running at `hz` on virtual time so the Speed knob drives it.
struct KzBeat { float ph; bool edge; uint32_t n; };
static KzBeat kzBeat(uint32_t at,float hz){
  static float prev=0.0f; static uint32_t cnt=0;
  float ph = (bPeriod>0.0f) ? beatPhase(millis()) : fmodf(at*0.001f*hz,1.0f);
  bool edge = ph<prev; if(edge) cnt++;
  prev=ph;
  KzBeat b={ph,edge,cnt}; return b;
}
// Square ring of cells at Chebyshev radius `ring` (0 = centre cell, 5 = outer).
static inline void kzCellRing(int ring,CRGB col){
  for(int r=0;r<11;r++) for(int c=0;c<11;c++) if(max(abs(r-5),abs(c-5))==ring) fillCell(r,c,col);
}
// Square frame on the strip lattice at half-size `half` (0 = centre cell .. 5 = rim).
static inline void kzSquare(int half,CRGB col){
  half=constrain(half,0,5); drawGridSquare(6-half,6-half,7+half,7+half,col);
}
static inline CRGB kzScale(CRGB c,float f){ f=constrain(f,0.0f,1.0f); c.nscale8_video((uint8_t)(f*255.0f)); return c; }
// Small integer hash → 0..1, for "the same elephant looks the same every time".
static inline float kzHash(uint32_t k,uint32_t salt){ uint32_t h=(k*2654435761u)^(salt*40503u); h^=h>>13; h*=0x5bd1e995u; h^=h>>15; return (h&0xFFFF)/65535.0f; }
// Thin anti-aliased line in world coords.
static inline void kzLine(float x0,float y0,float x1,float y1,float rad,CRGB c){ fxStreak(x0,y0,x1,y1,rad,1.2f,c); }
static inline void kzWhiteout(uint8_t w){ for(int s=1;s<=12;s++) for(int led=1;led<=LEDS_ACTIVE;led++){ setLED(true,s,led,CRGB(w,w,w)); setLED(false,s,led,CRGB(w,w,w)); } }

// ═══════════════════════════════════════════════════════════════════════════
// Crisp shape helpers (signed distances in world units, negative = inside)
// ═══════════════════════════════════════════════════════════════════════════
static inline float kzSegDist(float px,float py,float ax,float ay,float bx,float by,float* tOut=nullptr){
  float dx=bx-ax, dy=by-ay, l2=dx*dx+dy*dy; float t=l2>0?((px-ax)*dx+(py-ay)*dy)/l2:0; t=constrain(t,0.0f,1.0f);
  if(tOut) *tOut=t; float qx=ax+t*dx-px, qy=ay+t*dy-py; return sqrtf(qx*qx+qy*qy);
}
// Ellipse, first-order distance (f/|grad f|): exact on the boundary, good enough for a 1-unit edge.
static inline float kzEllipseD(float px,float py,float cx,float cy,float a,float b){
  float x=(px-cx)/a, y=(py-cy)/b; float f=x*x+y*y-1.0f;
  float gx=2.0f*x/a, gy=2.0f*y/b; float g=sqrtf(gx*gx+gy*gy); return g>1e-4f? f/g : -fminf(a,b);
}
static inline float kzBoxD(float px,float py,float cx,float cy,float hw,float hh){
  float qx=fabsf(px-cx)-hw, qy=fabsf(py-cy)-hh;
  float ox=fmaxf(qx,0.0f), oy=fmaxf(qy,0.0f); return sqrtf(ox*ox+oy*oy)+fminf(fmaxf(qx,qy),0.0f);
}
// Coverage of a signed distance with an `aa`-wide edge (sharp: aa ≈ 1 world unit).
static inline float kzCov(float d,float aa){ return constrain(0.5f-d/aa,0.0f,1.0f); }
static inline CRGB kzMix(CRGB a,CRGB b,float t){ t=constrain(t,0.0f,1.0f);
  return CRGB((uint8_t)(a.r+(b.r-a.r)*t),(uint8_t)(a.g+(b.g-a.g)*t),(uint8_t)(a.b+(b.b-a.b)*t)); }

// ═══════════════════════════════════════════════════════════════════════════
// 107 — KONZERT MARKER: the submarine over the desert (album cover)
// Setlist position 1, right after the calibration view. The cover scene: a dark
// red sky that glows orange toward the horizon, two dunes FILLED in the cover's
// orange/ochre/copper, and the lens-shaped submarine as a neon OUTLINE (not
// filled) with its domed tower, antenna, stern mast and two-blade propeller;
// four divers as simple neon stick figures hanging well below it, never touching
// the hull. The portholes sit exactly on strips/crossings so they read as points.
//
// Every part is its own neon tube and STRIKES like a real one being switched on:
// first a single short flash, long dark pause, another one — then the strikes
// come closer together and turn into stuttering bursts of several 1–2-frame
// flashes with a phosphor afterglow, until the tube catches and stays on with a
// faint hum. The parts ignite one after another (hull, tower, propeller, windows,
// divers; fully lit after ~8 s). Afterwards one tube relapses now and then into a
// short nervous stutter, like a tired sign. The flicker runs on REAL time, so the
// Speed knob only changes the slow motion (divers sway, sand haze).
// ═══════════════════════════════════════════════════════════════════════════
// One neon tube. kzNeonTick() returns its light level 0..1 for this frame.
struct KzNeon { uint32_t start, dur, next, relapseAt, lastT; uint8_t flashes, relapse; bool on, lit; float inst, glow; };
static void kzNeonInit(KzNeon& n,uint32_t t,uint32_t delay,uint32_t dur){
  n={}; n.start=t+delay+random(250); n.dur=dur; n.next=n.start; n.lastT=t;
}
static float kzNeonTick(KzNeon& n,uint32_t t){
  float dms=(float)(t-n.lastT); n.lastT=t;
  if(t<n.start) return 0.0f;
  if(!n.lit){                                              // ── striking ──
    float p=constrain((float)(t-n.start)/(float)n.dur,0.0f,1.0f);
    if(t>=n.next){
      if(n.on){                                            // a flash ends
        n.on=false;
        if(n.flashes>0) n.next=t+12+random(15+(int)(35.0f*(1.0f-p)));   // stutter gap inside a strike: 1–2 frames
        else if(p>=1.0f){ n.lit=true; n.on=true; n.inst=1.0f; n.relapseAt=t+9000+random(22000); }   // it catches
        else { float g=(1.0f-p)*(1.0f-p)*1100.0f+70.0f;  // pause to the next strike: long at first, short at the end
               n.next=t+(uint32_t)(g*(0.45f+1.1f*frand())); }
      } else {                                             // a flash starts
        if(n.flashes==0) n.flashes=1+random(1+(int)(p*p*8.0f));   // a new strike: single flashes first, bursts later
        n.flashes--; n.on=true;
        n.inst=(frand()<0.3f+0.5f*p) ? 1.0f : 0.3f+0.6f*frand();   // weak flashes early, mostly full later
        n.next=t+14+random(16+(int)(90.0f*p*p));          // flashes grow longer as it warms up
      }
    }
  } else if(n.relapse>0){                                  // ── a relapse: nervous stutter ──
    if(t>=n.next){ n.on=!n.on;
      if(n.on){ n.inst=0.45f+0.55f*frand(); n.next=t+18+random(70); }
      else { n.relapse--; n.next=t+14+random(frand()<0.15f?400:60); }
      if(n.relapse==0){ n.on=true; n.inst=1.0f; n.relapseAt=t+9000+random(22000); } }
  } else {                                                 // ── lit: a faint hum ──
    n.on=true; n.inst=0.93f+0.07f*(inoise8((uint16_t)(t/3),(uint16_t)n.start)/255.0f);
    if(t>=n.relapseAt){ n.relapse=2+random(6); n.on=false; n.next=t+15; }
  }
  if(n.on) n.glow=n.inst;                                  // instant attack …
  else     n.glow*=expf(-dms/20.0f);                       // … short phosphor afterglow on release (hard flicker, no fade)
  return n.glow;
}
// Neon look for one tube at distance d from its centre line: near-white hot core,
// the glass in full colour, a soft halo on the dark around it.
static inline CRGB kzNeonPx(float d,uint8_t h,uint8_t s,float L){
  if(L<=0.004f || d>7.0f) return CRGB::Black;
  float core=constrain(1.0f-d/0.9f,0.0f,1.0f), tube=constrain(1.0f-d/1.9f,0.0f,1.0f), glow=expf(-d/2.6f)*0.30f;
  CRGB part=kzScale(CRGB(CHSV(h,s,255)),fmaxf(tube,glow)); part=kzMix(part,CRGB(CHSV(h,(uint8_t)(s/3),255)),core*0.7f);
  return kzScale(part,L);
}
// Scene geometry (world units, y down). Windows sit on the strip lines y=-14 / -23.
#define KZ_SUB_CY  (-10.0f)   // hull centre line
#define KZ_SUB_A    46.0f     // hull half-length
#define KZ_SUB_BT    9.0f     // hull half-height above the centre line (flatter deck)
#define KZ_SUB_BB   13.0f     // … below it (rounder keel)
#define KZ_TOW_CY  (-19.0f)   // tower dome: upper half of this ellipse
#define KZ_HORIZON  30.0f
static const float KZ_HWIN[8]={-32,-23,-14,-5,5,14,23,32};
static const float KZ_TWIN[4]={-9,-3,3,9};
static const float KZ_DIV[4][3]={{-27,22,0.30f},{-8,26,-0.12f},{12,25,0.25f},{31,21,-0.30f}};   // x, y, tilt — well below the keel (y≈+3), never touching it
// One diver in its own frame (head up, tank on the back, legs/fins apart, arms out):
// signed distance to the union of capsules.
static float kzDiverD(float x,float y,float px,float py,float c,float s,bool line=false){
  float dx=x-px, dy=y-py; if(fabsf(dx)>11.0f || fabsf(dy)>12.0f) return 1e9f;
  float lx=c*dx+s*dy, ly=-s*dx+c*dy;
  static const float CP[9][5]={{0,-6.5f,0,-6.5f,2.0f},{0,-4.5f,0,2,1.9f},{2,-4,2,1,1.3f},
    {0,2,-1.8f,7.5f,1.1f},{0,2,2.2f,7,1.1f},{-1.8f,7.5f,-3.6f,9.2f,0.9f},{2.2f,7,4,8.6f,0.9f},
    {0,-3.5f,-3.8f,-0.5f,0.9f},{0,-3.5f,3.6f,-1.5f,0.9f}};
  float d=1e9f; for(int i=0;i<9;i++) d=fminf(d,kzSegDist(lx,ly,CP[i][0],CP[i][1],CP[i][2],CP[i][3])-(line?0.0f:CP[i][4]));
  if(line){ float hd=fabsf(hypotf(lx,ly+6.5f)-1.6f); d=fminf(d,hd); }   // the head as a small ring
  return d;
}
// Moving bits drawn on top as soft splats: air bubbles rising from the divers'
// helmets, sand trickling out of the hull (as on the cover), wind-blown grains
// running along the dune crests. Each respawns on its own — no global restart.
struct KzMote { float x, y, vx, vy, life; };
#define KZ_NBUB  8
#define KZ_NSAND 12
#define KZ_NWIND 6
// 6 tubes: hull, tower (+antenna, mast), hull windows, tower windows, propeller, divers.
#define KZ_NT 6
void anim_konzertMarker(uint32_t at,float dt){
  static KzNeon tb[KZ_NT]; static uint32_t t0=0; static float duneA[121], duneB[121];
  static KzMote bub[KZ_NBUB], sand[KZ_NSAND], wind[KZ_NWIND];
  uint32_t t=millis();
  if(!kzInit){ t0=t;
    static const uint16_t DELAY[KZ_NT]={300,1300,3100,3700,2100,5200}, DUR[KZ_NT]={4600,4000,2600,2300,3400,2600};
    for(int i=0;i<KZ_NT;i++) kzNeonInit(tb[i],t,DELAY[i],DUR[i]);
    for(int i=0;i<121;i++){ float x=(float)(i-60);           // dune crests depend on x only (X_POS are integers)
      duneA[i]=38.0f+12.0f*tanhf((x+5.0f)/28.0f); duneB[i]=36.0f-6.0f*tanhf((x-30.0f)/14.0f); }
    for(int i=0;i<KZ_NBUB;i++)  bub[i].life=-frand()*3.0f;    // negative life = waiting to spawn
    for(int i=0;i<KZ_NSAND;i++) sand[i].life=-frand()*2.0f;
    for(int i=0;i<KZ_NWIND;i++) wind[i].life=-frand()*4.0f;
    kzInit=true; }
  float L[KZ_NT]; for(int i=0;i<KZ_NT;i++) L[i]=kzNeonTick(tb[i],t);
  float bg=constrain((float)(t-t0)/1800.0f,0.0f,1.0f);        // the desert fades in under the first strikes
  float s=dt*0.001f;                                         // seconds this frame (speed-scaled)
  // The sub floats: a slow sideways sway and a gentle bob. The bob stays small so
  // the portholes keep sitting on their strip lines.
  float subX=sinf(at*0.00021f)*4.5f, subY=sinf(at*0.00047f)*1.2f;
  float ph=at*0.0075f, tipX=1.6f*sinf(ph), tipY=6.5f*cosf(ph); // propeller: two blades turning about the shaft
  // The divers hang below the sub, each swaying and bobbing on its own.
  float DXp[4], DYp[4], DC[4], DS[4];
  for(int i=0;i<4;i++){ float a=KZ_DIV[i][2]+sinf(at*0.0006f+i*1.9f)*0.22f;
    DXp[i]=KZ_DIV[i][0]+subX*0.8f+sinf(at*0.00033f+i*2.3f)*2.0f; DYp[i]=KZ_DIV[i][1]+subY+sinf(at*0.0009f+i*1.7f)*1.6f;
    DC[i]=cosf(a); DS[i]=sinf(a); }
  uint16_t hz=(uint16_t)(at/9);                              // sand haze drifts across the sky
  clearFrame();
  kzField([&](float x,float y,CRGB& c)->bool{
    int xi=constrain((int)lroundf(x)+60,0,120);
    // ── desert ──
    CRGB col;
    if(y<KZ_HORIZON){ float k=(y+60.0f)/(KZ_HORIZON+60.0f); k=k*k*k;
      float haze=0.75f+0.5f*inoise8((uint16_t)(x*9.0f)+hz,(uint16_t)(y*9.0f))/255.0f;
      col=CHSV((uint8_t)(2+14*k),250,(uint8_t)constrain((4.0f+66.0f*k)*haze,0.0f,255.0f)); }
    else col=CHSV(10,245,40);
    // Dunes FILLED in the cover colours: the back dune deep orange-copper, the
    // front dune bright ochre-orange, both with soft wind ripples and getting
    // lighter toward their crest.
    if(x>-10.0f && y>duneB[xi]){ float k=constrain((y-duneB[xi])/24.0f,0.0f,1.0f);
      float rip=0.85f+0.15f*sinf(x*0.35f+y*0.9f);
      col=CHSV((uint8_t)(24-8*k),235,(uint8_t)((150.0f-70.0f*k)*rip)); }
    if(y>duneA[xi]){ float k=constrain((y-duneA[xi])/22.0f,0.0f,1.0f);
      float rip=0.85f+0.15f*sinf(x*0.31f-y*0.8f);
      col=CHSV((uint8_t)(30-12*k),240,(uint8_t)((185.0f-85.0f*k)*rip)); }
    col+=kzScale(CRGB(CHSV(20,190,200)),kzCov(fabsf(y-duneA[xi])-0.6f,1.4f));
    if(x>-10.0f) col+=kzScale(CRGB(CHSV(20,190,200)),kzCov(fabsf(y-duneB[xi])-0.6f,1.4f));
    col=kzScale(col,bg);
    // ── submarine (sub-local coords: the whole boat floats by subX/subY) ──
    float ox=x, oy=y; x-=subX; y-=subY;
    if(x>-58.0f && x<50.0f && y>-48.0f && y<6.0f){
      float hd=kzEllipseD(x,y,0,KZ_SUB_CY,KZ_SUB_A,y<KZ_SUB_CY?KZ_SUB_BT:KZ_SUB_BB);
      float td=(y<KZ_TOW_CY)?kzEllipseD(x,y,0,KZ_TOW_CY,15.0f,12.0f):1e9f;
      float body=fminf(hd,td);
      if(body<0.0f) col=kzScale(col,0.25f);                  // NOT filled: just the night sky dimmed a little behind the hull
      col+=kzNeonPx(fabsf(body),22,200,L[0]);                // hot rim around hull + dome
      if(hd<0.0f) col+=kzScale(kzNeonPx(fabsf(y-KZ_SUB_CY),16,240,L[0]),0.5f);   // the equator seam
      if(fabsf(y+14.0f)<6.0f){ float wd=1e9f; for(int i=0;i<8;i++) wd=fminf(wd,hypotf(x-KZ_HWIN[i],y+14.0f));
        col+=kzNeonPx(fmaxf(0.0f,wd-1.5f),44,170,L[2]); }
      if(fabsf(y+23.0f)<6.0f){ float wd=1e9f; for(int i=0;i<4;i++) wd=fminf(wd,hypotf(x-KZ_TWIN[i],y+23.0f));
        col+=kzNeonPx(fmaxf(0.0f,wd-1.3f),44,170,L[3]); }
      float ant=fminf(fminf(kzSegDist(x,y,4,-31,10,-45),kzSegDist(x,y,6,-37,12,-37)),kzSegDist(x,y,-40,-15,-40,-27));
      col+=kzNeonPx(ant,24,200,L[1]);                        // antenna + stern mast ride with the tower
      if(x<-40.0f){ float pr=fminf(kzSegDist(x,y,-46,KZ_SUB_CY,-53,KZ_SUB_CY),
          fmaxf(0.0f,kzSegDist(x,y,-53-tipX,KZ_SUB_CY-tipY,-53+tipX,KZ_SUB_CY+tipY)-1.3f));
        col+=kzNeonPx(pr,16,215,L[4]); }                     // shaft + the turning blade pair
    }
    x=ox; y=oy;
    // ── divers: pale filled figures, the last tube to strike ──
    if(y>8.0f && y<40.0f && L[5]>0.004f){
      float dd=1e9f; for(int i=0;i<4;i++) dd=fminf(dd,kzDiverD(x,y,DXp[i],DYp[i],DC[i],DS[i]));
      (void)dd; float dl=1e9f; for(int i=0;i<4;i++) dl=fminf(dl,kzDiverD(x,y,DXp[i],DYp[i],DC[i],DS[i],true));
      col+=kzScale(kzNeonPx(dl,24,120,L[5]),0.85f);           // a simple neon stick figure
    }
    if((col.r|col.g|col.b)==0) return false;
    c=col; return true;
  });
  // Bubbles: from a diver's helmet up to the hull, wobbling; only once the divers are lit.
  for(int i=0;i<KZ_NBUB;i++){ KzMote& m=bub[i];
    if(m.life<0.0f){ m.life+=s; if(m.life>=0.0f){ int d=random(4);
        m.x=DXp[d]+6.5f*DS[d]; m.y=DYp[d]-6.5f*DC[d]; m.vy=-(6.0f+frand()*5.0f); m.vx=frand()*6.28f; m.life=0.0f; }
      continue; }
    m.life+=s; m.y+=m.vy*s; float bx=m.x+sinf(m.vx+m.life*5.0f)*1.2f;
    float u=(bx-subX)/KZ_SUB_A;                               // pops when it reaches the hull's underside
    float lim=subY+KZ_SUB_CY+KZ_SUB_BB*sqrtf(fmaxf(0.0f,1.0f-u*u));
    if(m.y<lim){ m.life=-(0.3f+frand()*2.5f); continue; }
    if(L[5]>0.004f) fxSplat(bx,m.y,0.8f,1.0f,kzScale(CRGB(CHSV(30,70,190)),L[5]*constrain(m.life*3.0f,0.0f,1.0f)));
  }
  // Sand trickling out of the hull, falling and drifting with the wind, gone at the dune.
  for(int i=0;i<KZ_NSAND;i++){ KzMote& m=sand[i];
    if(m.life<0.0f){ m.life+=s; if(m.life>=0.0f){
        float u=(frand()*2.0f-1.0f)*0.6f; m.x=subX+u*KZ_SUB_A; m.y=subY+KZ_SUB_CY+KZ_SUB_BB*sqrtf(1.0f-u*u);
        m.vx=1.0f+frand()*2.0f; m.vy=3.0f+frand()*4.0f; m.life=0.0f; }
      continue; }
    m.life+=s; m.vy+=9.0f*s; m.x+=m.vx*s; m.y+=m.vy*s;
    int xi=constrain((int)lroundf(m.x)+60,0,120);
    if(m.y>duneA[xi] || m.y>58.0f){ m.life=-(0.1f+frand()*1.6f); continue; }
    fxSplat(m.x,m.y,0.6f,1.0f,kzScale(CRGB(CHSV(12,220,150)),(0.25f+0.75f*L[0])*constrain(1.0f-m.life*0.35f,0.2f,1.0f)));
  }
  // Wind: bright grains running along the two dune crests, fading in and out.
  for(int i=0;i<KZ_NWIND;i++){ KzMote& m=wind[i];
    if(m.life<0.0f){ m.life+=s; if(m.life>=0.0f){ m.vy=(float)random(2);   // which crest
        m.x=(m.vy>0.5f)?-10.0f+frand()*20.0f:-60.0f+frand()*30.0f; m.vx=14.0f+frand()*16.0f; m.life=0.0f; }
      continue; }
    m.life+=s; m.x+=m.vx*s;
    float dur=2.2f; if(m.life>dur || m.x>60.0f){ m.life=-(0.5f+frand()*3.0f); continue; }
    int xi=constrain((int)lroundf(m.x)+60,0,120); float y=(m.vy>0.5f)?duneB[xi]:duneA[xi];
    fxSplat(m.x,y-0.8f,1.0f,1.2f,kzScale(CRGB(CHSV(26,150,230)),bg*sinf(m.life/dur*3.14159f)));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 108 — 100 ELEFANTEN
// Pink elephants walk right → left over a prairie. Each is a SOLID, crisply-edged
// figure (signed-distance shapes, 1-unit edge): body, round head, a big darker
// ear, a yellow eye, a trunk in three pieces that swings with the stride, four
// legs that walk and lift, a tail. Every one of the 100 is its own animal — size
// (big to very big), shade of pink, ear, trunk curl, body length, spacing — and
// they come through as an endless procession, no restart.
// Green grass grows from the very bottom of the wall up to their knees: tapered
// blades that sway in a gusting wind and slowly wander sideways, drawn IN FRONT
// of the herd.
// ═══════════════════════════════════════════════════════════════════════════
#define KZ_EL_MAX 3
#define KZ_GROUND 48.0f          // where the feet stand (world y, +down); grass fills from here to the bottom
#define KZ_GRASS  30
struct KzElephant { bool used; float x; uint32_t k; float walk; };
struct KzElShape { float sc,bodyL,earS,curl,trumpet; uint8_t hue,sat; };
static inline KzElShape kzElShape(uint32_t k){
  KzElShape s;
  // Mostly BIG (1.8 .. 2.9, the biggest is taller than half the wall and as long as
  // it), but every ~4th is a small one (1.1 .. 1.4) walking in between.
  s.sc = kzHash(k,11)<0.25f ? 1.1f+0.3f*kzHash(k,1) : 1.8f+1.1f*kzHash(k,1);
  s.trumpet=kzHash(k,8)*6.28f;   // phase of the trunk-raise / water-spray cycle
  // Every elephant its own colour from a palette of BROKEN tones — pink, rose,
  // raspberry, coral, salmon, mauve, lilac, violet, plum — never a primary: the
  // saturation stays well below full, and each animal gets a small hue jitter of
  // its own on top. (hue/sat must be set here: they once slipped into a comment
  // and were uninitialised, so the colour flickered every frame.)
  static const uint8_t PAL[9][2]={ {232,150},{240,170},{246,195},{6,150},{12,125},
                                   {220,110},{200,100},{192,150},{210,165} };   // hue, sat (FastLED)
  const uint8_t* pc=PAL[(int)(kzHash(k,2)*8.999f)];
  s.hue=(uint8_t)(pc[0]+(int)(kzHash(k,12)*10.0f)-5);
  s.sat=(uint8_t)(pc[1]+(int)(kzHash(k,3)*20.0f)-10);
  s.earS=0.85f+0.40f*kzHash(k,4); s.curl=-3.0f+6.0f*kzHash(k,5); s.bodyL=0.92f+0.28f*kzHash(k,6); return s;
}
// How far the trunk is raised (0 = hanging and swinging, 1 = up, trumpeting).
static inline float kzTrunkRaise(const KzElephant& e,const KzElShape& s){
  float r=sinf(e.walk*0.11f+s.trumpet)*2.2f-1.3f; r=constrain(r,0.0f,1.0f); return r*r*(3.0f-2.0f*r);
}
// The three trunk joints in local coords, blended between hanging and raised.
static inline void kzTrunk(const KzElephant& e,const KzElShape& s,float& ax,float& ay,float& bx,float& by,float& cx,float& cy){
  float ts=sinf(e.walk*0.5f+1.0f)*2.0f, r=kzTrunkRaise(e,s);
  float hax=22+ts, hay=14, hbx=23+ts*1.5f, hby=7, hcx=22+s.curl+ts*2.0f, hcy=2;       // hanging
  float rax=25, ray=30, rbx=28, rby=37, rcx=26+s.curl*0.3f, rcy=43;                   // raised
  ax=hax+(rax-hax)*r; ay=hay+(ray-hay)*r; bx=hbx+(rbx-hbx)*r; by=hby+(rby-hby)*r; cx=hcx+(rcx-hcx)*r; cy=hcy+(rcy-hcy)*r;
}
// One elephant at world pixel (x,y): returns coverage 0..1 and its colour.
static float kzElephantPx(const KzElephant& e,const KzElShape& s,float x,float y,CRGB& col){
  float u=(e.x-x)/s.sc, v=(KZ_GROUND-y)/s.sc;                // local: u forward (to the left), v up
  if(u<-36.0f||u>32.0f||v<-1.0f||v>46.0f) return 0;   // v to 46: the raised trunk
  float L=s.bodyL;
  float dBody=kzEllipseD(u,v,-4.0f*L,21.0f,16.0f*L,9.5f);
  float dHead=sqrtf((u-14.0f)*(u-14.0f)+(v-25.0f)*(v-25.0f))-7.5f;
  float sw=sinf(e.walk)*3.0f, lift=fmaxf(sinf(e.walk),0.0f)*1.6f, lift2=fmaxf(-sinf(e.walk),0.0f)*1.6f;
  float dLeg=fminf(fminf(kzSegDist(u,v,-15.0f*L,15,-15.0f*L+sw,1.2f+lift),  kzSegDist(u,v,-9.0f*L,15,-9.0f*L-sw,1.2f+lift2)),
                   fminf(kzSegDist(u,v,  2.0f,  15,  2.0f-sw,   1.2f+lift2), kzSegDist(u,v, 7.5f,  15, 7.5f+sw,  1.2f+lift)))-2.4f;
  float ax,ay,bx,by,cx,cy; kzTrunk(e,s,ax,ay,bx,by,cx,cy);
  float dTrunk=fminf(fminf(kzSegDist(u,v,19,22,ax,ay)-2.4f, kzSegDist(u,v,ax,ay,bx,by)-1.9f),
                     kzSegDist(u,v,bx,by,cx,cy)-1.5f);
  float tailX=-4.0f-16.0f*L;
  float dTail=kzSegDist(u,v,tailX+1.5f,23,tailX-3.0f,14)-0.7f;
  float d=fminf(fminf(fminf(dBody,dHead),fminf(dLeg,dTrunk)),dTail)*s.sc;   // back to world units
  float cov=kzCov(d,1.0f); if(cov<=0.0f) return 0;
  float edge=constrain(1.0f-fabsf(d+0.6f)/1.1f,0.0f,1.0f);     // a bright rim just inside the outline
  CRGB body=CRGB(CHSV(s.hue,s.sat,255));
  col=kzScale(body,0.55f+0.45f*edge);
  float dEar=kzEllipseD(u,v,9.0f,24.0f,4.6f*s.earS,7.0f*s.earS)*s.sc;
  float ce=kzCov(dEar,1.0f);
  if(ce>0){ CRGB ear=kzScale(CRGB(CHSV((uint8_t)(s.hue+6),(uint8_t)min(255,s.sat+45),255)),0.32f+0.55f*constrain(1.0f-fabsf(dEar+0.6f)/1.1f,0.0f,1.0f));
            col=kzMix(col,ear,ce); }
  float dEye=(sqrtf((u-15.0f)*(u-15.0f)+(v-27.0f)*(v-27.0f))-0.85f)*s.sc;   // small, set back toward the ear
  float cEye=kzCov(dEye,0.9f); if(cEye>0) col=kzMix(col,CRGB(255,210,0),cEye);
  return cov;
}
void anim_dh100Elefanten(uint32_t at,float dt){
  static KzElephant el[KZ_EL_MAX]; static uint32_t nextK=0; static float sinceSpawn=0, spawnGap=0;
  static float gBase[KZ_GRASS], gPh[KZ_GRASS], gH[KZ_GRASS]; static uint8_t gHue[KZ_GRASS];
  if(!kzInit){
    for(int i=0;i<KZ_EL_MAX;i++) el[i].used=false;
    nextK=0; sinceSpawn=1e9f; spawnGap=0;
    for(int i=0;i<KZ_GRASS;i++){ gBase[i]=-64.0f+i*(128.0f/KZ_GRASS)+frand()*3.0f; gPh[i]=frand()*6.28f; gH[i]=10.0f+frand()*14.0f; gHue[i]=(uint8_t)(86+random(20)); }
    kzInit=true;
  }
  float v=0.018f*dt;                                        // world units per frame (speed-scaled)
  sinceSpawn+=v;
  if(sinceSpawn>=spawnGap){                                  // next animal enters from the right
    for(int i=0;i<KZ_EL_MAX;i++) if(!el[i].used){
      KzElShape s=kzElShape(nextK);
      el[i].used=true; el[i].k=nextK; el[i].x=62.0f+28.0f*s.sc; el[i].walk=frand()*6.28f;
      spawnGap=(30.0f+22.0f*s.bodyL)*s.sc+6.0f+kzHash(nextK,7)*30.0f;   // its own length + a gap of its own
      nextK=(nextK+1)%100; sinceSpawn=0; break; }
  }
  KzElShape sh[KZ_EL_MAX];
  for(int i=0;i<KZ_EL_MAX;i++) if(el[i].used){
    sh[i]=kzElShape(el[i].k);
    el[i].x-=v; el[i].walk+=dt*0.0070f;
    if(el[i].x<-62.0f-40.0f*sh[i].sc) el[i].used=false;
  }
  // grass blade geometry for this frame
  float wind=inoise8((uint16_t)(at/25))/255.0f-0.5f;
  float bx[KZ_GRASS], tx[KZ_GRASS];
  for(int i=0;i<KZ_GRASS;i++){ bx[i]=gBase[i]+sinf(at*0.00022f+gPh[i])*7.0f; tx[i]=bx[i]+sinf(at*0.0032f+gPh[i]*1.7f)*3.0f+wind*8.0f; }
  // showGrid() turns every colour by the mode's colour setting (Colour key / app) and
  // the hue-speed rotation. The elephants may turn with it, the GRASS must stay
  // green: pre-turn it backwards by the same amount so it lands on green again.
  uint8_t grassRot=(uint8_t)(gHueBase+gHueAuto);
  clearFrame();
  kzField([&](float x,float y,CRGB& c)->bool{
    // grass (front)
    float gc=0; CRGB gcol=CRGB::Black;
    if(y>61.0f-34.0f) for(int i=0;i<KZ_GRASS;i++){
      float top=61.0f-gH[i]; if(y<top-1.5f) continue;
      if(fabsf(x-bx[i])>12.0f) continue;
      float t; float d=kzSegDist(x,y,bx[i],61.5f,tx[i],top,&t)-(1.5f-1.1f*t);
      float cv=kzCov(d,0.9f); if(cv>gc){ gc=cv; gcol=CRGB(CHSV((uint8_t)(gHue[i]-grassRot),235,(uint8_t)(150+100*t))); }
    }
    if(gc>0.97f){ c=gcol; return true; }
    // the herd (behind the grass)
    float ec=0; CRGB ecol=CRGB::Black;
    for(int i=0;i<KZ_EL_MAX;i++) if(el[i].used){
      CRGB cc; float cv=kzElephantPx(el[i],sh[i],x,y,cc); if(cv>ec){ ec=cv; ecol=cc; } }
    if(gc<=0.0f && ec<=0.0f) return false;
    c=kzMix(kzScale(ecol,ec),gcol,gc);
    return (c.r|c.g|c.b)!=0;
  });
  // trunk up → a fountain of water out of the tip
  for(int i=0;i<KZ_EL_MAX;i++) if(el[i].used){
    float r=kzTrunkRaise(el[i],sh[i]); if(r<0.75f) continue;
    float ax,ay,bx,by,cx,cy; kzTrunk(el[i],sh[i],ax,ay,bx,by,cx,cy);
    float tx=el[i].x-cx*sh[i].sc, ty=KZ_GROUND-cy*sh[i].sc;
    for(int j=0;j<7;j++){ float ph=fmodf(at*0.0012f+j/7.0f,1.0f);
      float px=tx-(3.0f+14.0f*ph)*sh[i].sc*0.5f*(0.7f+0.1f*j), py=ty-20.0f*sh[i].sc*ph*(1.0f-ph)*2.0f+ph*ph*12.0f;
      fxSplat(px,py,0.9f,1.1f,kzScale(CRGB(170,215,255),(r-0.7f)*3.3f*(1.0f-ph))); }
  }
  showGrid();
}
// ═══════════════════════════════════════════════════════════════════════════
// 110 — ECHOLOT
// Sonar: square pings run from the centre to the rim and decay (one per beat),
// a radar beam turns with a fading wake, and three hidden targets light up as
// echo blips when the beam sweeps over them — then fade until the next pass.
// ═══════════════════════════════════════════════════════════════════════════
void anim_dhEcholot(uint32_t at,float dt){
  static float tx[3],ty[3],blip[3]; static uint32_t seedT=0;
  if(!kzInit){ for(int i=0;i<3;i++){ tx[i]=frand()*90-45; ty[i]=frand()*90-45; blip[i]=0; } seedT=at; kzInit=true; }
  if(at-seedT>14000){ seedT=at; int i=random(3); tx[i]=frand()*90-45; ty[i]=frand()*90-45; }
  float tf=at*0.001f;
  KzBeat b=kzBeat(at,0.5f);
  float ang=fmodf(tf*1.1f,(float)(2*M_PI));              // beam angle
  for(int i=0;i<3;i++){
    float ta=atan2f(ty[i],tx[i]); float d=fabsf(ta-ang); if(d>M_PI) d=2*M_PI-d;
    if(d<0.12f) blip[i]=1.0f;
    blip[i]*=powf(0.5f,dt/700.0f);
  }
  clearFrame();
  float ringR=b.ph*64.0f;                                  // ping radius (square, Chebyshev)
  kzField([&](float x,float y,CRGB& c)->bool{
    float cheb=fmaxf(fabsf(x),fabsf(y));
    float ring=constrain(1.0f-fabsf(cheb-ringR)/3.0f,0.0f,1.0f)*(1.0f-b.ph)*(1.0f-b.ph);
    float pa=atan2f(y,x); float d=ang-pa; while(d<0) d+=2*M_PI; while(d>=2*M_PI) d-=2*M_PI;   // wake behind the beam
    float beam=d<1.4f ? (1.0f-d/1.4f) : 0.0f; beam=beam*beam*0.55f;
    if(d<0.06f) beam=1.0f;
    float v=fmaxf(ring,beam);
    if(v<0.03f) return false;
    c=hotColor(v,120,20,110); return true;
  });
  for(int i=0;i<3;i++) if(blip[i]>0.04f){
    int col=constrain((int)((tx[i]+55.0f)/10.0f),0,10), row=constrain((int)((ty[i]+55.0f)/10.0f),0,10);
    fillCell(row,col,CRGB(CHSV(100,120,(uint8_t)(blip[i]*255))));
  }
  fillCell(5,5,CRGB(CHSV(120,200,(uint8_t)(60+120*(1.0f-b.ph)))));   // the sonar source
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 111 — CRY ME A RIVER
// Water streaming down every vertical strip (noise bands moving with the flow),
// faint cross-ripples on the horizontals, and tears: bright drops falling from
// the top that splash into a small ring where they land.
// ═══════════════════════════════════════════════════════════════════════════
void anim_dhCryMeARiver(uint32_t at,float dt){
  struct Drop { float x,y,vy; bool alive; float splash; float sx; };
  static Drop dr[4]; static uint32_t nextDrop=0;
  if(!kzInit){ for(int i=0;i<4;i++){ dr[i].alive=false; dr[i].splash=0; } nextDrop=at+500; kzInit=true; }
  if(at>=nextDrop){ nextDrop=at+900+random(1800);
    for(int i=0;i<4;i++) if(!dr[i].alive){ dr[i].alive=true; dr[i].x=X_POS[random(12)]; dr[i].y=-62; dr[i].vy=0.02f; break; } }
  for(int i=0;i<4;i++){
    if(dr[i].alive){ dr[i].vy+=dt*0.00012f; dr[i].y+=dr[i].vy*dt; if(dr[i].y>56){ dr[i].alive=false; dr[i].splash=1.0f; dr[i].sx=dr[i].x; } }
    dr[i].splash*=powf(0.5f,dt/220.0f);
  }
  clearFrame();
  uint32_t flow=(uint32_t)(at*0.09f);
  kzField([&](float x,float y,CRGB& c)->bool{
    uint8_t n1=inoise8((uint16_t)((x+80)*9),(uint16_t)((y+80)*5-flow));          // down-stream bands
    uint8_t n2=inoise8((uint16_t)((x+80)*22+flow/2),(uint16_t)((y+80)*22));       // fine turbulence
    float v=(n1/255.0f); v=v*v*(0.75f+0.25f*(n2/255.0f));
    v*=0.85f;
    if(v<0.04f) return false;
    c=hotColor(v,135,25,120); return true;
  });
  for(int i=0;i<4;i++){
    if(dr[i].alive){ fxSplat(dr[i].x,dr[i].y,1.1f,1.6f,CRGB(200,230,255)); fxSplat(dr[i].x,dr[i].y-4.0f,0.6f,1.4f,CRGB(90,140,200)); }
    if(dr[i].splash>0.05f) fxRing(dr[i].sx,56.0f,(1.0f-dr[i].splash)*12.0f,1.0f,1.5f,kzScale(CRGB(150,210,255),dr[i].splash));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 112 — NICHT ERSCHRECKEN: thunderstorm
// Built like real lightning, and frequent. Each strike is a SEQUENCE generated
// fresh: a dim stepped LEADER creeping down, the blinding RETURN STROKE (the
// whole wall lights up with it), then 1–6 RE-STRIKES with their own random gaps
// (15–300 ms), lengths and strengths, sometimes down a new channel, sometimes
// only the upper part — and the channel's afterglow fading in between. Every
// stroke flickers inside itself. Kinds of strike, picked at random:
//   · cloud-to-ground (a jagged trunk with 2–6 forks, forks of forks),
//   · a DOUBLE strike (two channels at once),
//   · cloud-to-cloud (a long branching crawler across the top),
//   · sheet lightning (only the clouds flare).
// Strikes come every 0.3–4 s, in clusters. Colour per strike between white,
// blue, violet and UV. A tapped tempo pulls the first stroke onto the beat.
// ═══════════════════════════════════════════════════════════════════════════
#define KZ_BOLT_N 34
#define KZ_BR_N   8
struct KzBolt { int n; float x[KZ_BOLT_N],y[KZ_BOLT_N]; int branches; int bn[KZ_BR_N]; float bx[KZ_BR_N][10],by[KZ_BR_N][10]; };
static void kzBranch(KzBolt& b,float x,float y,float dir,int len,float down){
  if(b.branches>=KZ_BR_N) return;
  int k=b.branches++; b.bn[k]=0;
  for(int i=0;i<len && b.bn[k]<10;i++){
    b.bx[k][b.bn[k]]=x; b.by[k][b.bn[k]]=y; b.bn[k]++;
    y+=down*(1.5f+frand()*3.5f); x+=dir*(1.5f+frand()*4.5f)+(frand()-0.5f)*3.5f;
    if(i==2 && frand()<0.35f) kzBranch(b,x,y,-dir,2+random(4),down);   // a fork of the fork
  }
}
// kind 0 = cloud-to-ground, 1 = cloud-to-cloud (horizontal across the top)
static void kzMakeBolt(KzBolt& b,int kind){
  b.n=0; b.branches=0;
  if(kind==0){
    float x=frand()*100.0f-50.0f, y=-62.0f, endY=20.0f+frand()*41.0f, drift=(frand()-0.5f)*2.5f;
    while(y<endY && b.n<KZ_BOLT_N){ b.x[b.n]=x; b.y[b.n]=y; b.n++; y+=2.0f+frand()*4.0f; x+=drift+(frand()-0.5f)*(frand()<0.18f?15.0f:6.0f); }
    int nb=2+random(5);
    for(int k=0;k<nb;k++){ int from=1+random(max(1,b.n-4)); kzBranch(b,b.x[from],b.y[from],frand()<0.5f?-1.0f:1.0f,3+random(7),1.0f); }
  } else {
    float dir=frand()<0.5f?-1.0f:1.0f, x=-dir*62.0f, y=-50.0f+frand()*22.0f;
    while(fabsf(x)<=62.0f && b.n<KZ_BOLT_N){ b.x[b.n]=x; b.y[b.n]=y; b.n++; x+=dir*(3.0f+frand()*4.5f); y+=(frand()-0.5f)*6.0f; y=constrain(y,-60.0f,-15.0f); }
    int nb=3+random(5);
    for(int k=0;k<nb;k++){ int from=random(max(1,b.n-1)); kzBranch(b,b.x[from],b.y[from],frand()<0.5f?-1.0f:1.0f,2+random(5),frand()<0.5f?0.6f:-0.4f); }
  }
}
static void kzDrawBolt(const KzBolt& b,float bri,float reach,uint8_t hue,uint8_t sat){
  if(bri<0.02f) return;
  CRGB halo=kzScale(CRGB(CHSV(hue,sat,255)),bri*0.55f), core=kzScale(CRGB(CHSV(hue,35,255)),bri);
  int nVis=max(2,(int)(b.n*reach));
  for(int i=0;i+1<nVis && i+1<b.n;i++){
    fxStreak(b.x[i],b.y[i],b.x[i+1],b.y[i+1],1.2f,1.1f,halo);
    fxStreak(b.x[i],b.y[i],b.x[i+1],b.y[i+1],0.3f,0.55f,core);
  }
  if(reach<1.0f) return;
  for(int k=0;k<b.branches;k++){ float f=0.75f-0.06f*k; if(f<0.3f) f=0.3f;
    for(int i=0;i+1<b.bn[k];i++){
      fxStreak(b.bx[k][i],b.by[k][i],b.bx[k][i+1],b.by[k][i+1],0.8f,1.0f,kzScale(halo,f*0.8f));
      fxStreak(b.bx[k][i],b.by[k][i],b.bx[k][i+1],b.by[k][i+1],0.22f,0.55f,kzScale(core,f)); } }
}
struct KzStroke { uint16_t start,dur; float inten; float reach; bool newPath; };
void anim_dhNichtErschrecken(uint32_t at,float dt){
  static KzBolt bolt, bolt2; static KzStroke st[8]; static int nSt=0, cur=-1, kind=0;
  static uint32_t strikeT=0, nextStrike=0; static uint16_t leaderDur=0; static bool sheet=false, dbl=false, active=false;
  static uint8_t hue=0, sat=0; static float glow=0, chan=0;
  if(!kzInit){ nextStrike=at+600+random(1500); active=false; glow=chan=0; kzInit=true; }
  KzBeat b=kzBeat(at,1.0f);
  if(!active && at>=nextStrike && (bPeriod<=0.0f || b.edge || (at-nextStrike)>1500)){
    active=true; strikeT=at; cur=-1;
    float r=frand(); sheet = r<0.15f; kind = (r>=0.15f && r<0.35f) ? 1 : 0; dbl = !sheet && kind==0 && frand()<0.22f;
    kzMakeBolt(bolt,kind); if(dbl) kzMakeBolt(bolt2,0);
    leaderDur = (sheet||kind==1) ? 0 : (uint16_t)(50+random(110));
    float pick=frand(); hue = pick<0.25f ? 150 : (pick<0.55f ? 168 : (pick<0.8f ? 190 : 210));
    sat = (frand()<0.35f) ? 25 : (uint8_t)(130+random(120));
    nSt=0; uint16_t t=leaderDur;
    int extra = sheet ? 2+random(5) : 1+random(6);
    for(int i=0;i<=extra && nSt<8;i++){
      KzStroke& s=st[nSt++];
      s.start=t; s.dur=(uint16_t)(i==0 ? 50+random(50) : 18+random(65));
      s.inten = i==0 ? 1.0f : 0.3f+0.65f*frand();
      s.reach = (i>0 && frand()<0.25f) ? 0.3f+0.4f*frand() : 1.0f;
      s.newPath = i>0 && frand()<0.25f;
      t += s.dur + (frand()<0.15f ? 180+random(160) : 15+random(110));
    }
  }
  float fl=0, reach=1.0f, leader=0;
  if(active){
    uint32_t e=at-strikeT;
    if(e<leaderDur) leader=(float)e/leaderDur;
    int now=-1; for(int i=0;i<nSt;i++) if(e>=st[i].start && e<(uint32_t)(st[i].start+st[i].dur)){ now=i; break; }
    if(now>=0){
      if(now!=cur){ cur=now; if(st[now].newPath) kzMakeBolt(bolt,kind); }
      const KzStroke& s=st[now]; float le=(float)(e-s.start);
      float shape = le<8.0f ? 1.0f : expf(-(le-8.0f)/(s.dur*0.45f));
      fl = s.inten*shape*(0.6f+0.4f*frand());              // flicker inside the stroke
      reach = s.reach;
    }
    const KzStroke& last=st[nSt-1];
    if(e>(uint32_t)(last.start+last.dur+30)){ active=false;
      nextStrike = at + (frand()<0.45f ? 300+random(900) : 1200+random(2800)); }
  }
  if(fl>chan) chan=fl; chan*=powf(0.5f,dt/45.0f);           // channel afterglow between strokes
  if(fl>glow) glow=fl; glow*=powf(0.5f,dt/220.0f);          // clouds stay lit a little longer
  clearFrame();
  // sky: dark clouds at the top, flaring with each stroke; the return stroke lights the whole wall
  kzField([&](float x,float y,CRGB& c)->bool{
    float top=constrain(1.0f-(y+62.0f)/(sheet?75.0f:50.0f),0.0f,1.0f);
    uint8_t nz=inoise8((uint16_t)((x+80)*5+at/45),(uint16_t)((y+80)*6));
    float v=(nz/255.0f)*top*(0.035f+(sheet?0.8f:0.4f)*glow);
    if(!sheet) v+=0.07f*fl*fl;                               // whole wall catches the strong strokes
    if(v<0.015f) return false;
    c=CRGB(CHSV(hue,(uint8_t)min(255,sat+50),(uint8_t)(constrain(v,0.0f,1.0f)*255))); return true;
  });
  if(!sheet){
    if(leader>0){ int nl=(int)(bolt.n*leader); for(int i=0;i+1<nl && i+1<bolt.n;i++)
        fxStreak(bolt.x[i],bolt.y[i],bolt.x[i+1],bolt.y[i+1],0.3f,0.55f,CRGB(CHSV(hue,sat,55))); }
    float bri=fmaxf(fl,chan*0.35f);
    kzDrawBolt(bolt,bri,reach,hue,sat);
    if(dbl) kzDrawBolt(bolt2,bri*0.8f,reach,hue,sat);
  }
  showGrid();
}
// ═══════════════════════════════════════════════════════════════════════════
// 114 — SABBIA I: sandstorm
// Two layers of noise drift sideways as dunes, gusts swell and fade, single
// grains glitter for a frame. Palette: red → orange → yellow.
// ═══════════════════════════════════════════════════════════════════════════
void anim_dhSabbia1(uint32_t at,float dt){
  (void)dt;
  uint32_t wind=(uint32_t)(at*0.16f);
  float gust=0.55f+0.45f*(inoise8((uint16_t)(at/40))/255.0f);
  clearFrame();
  kzField([&](float x,float y,CRGB& c)->bool{
    uint8_t n1=inoise8((uint16_t)((x+80)*6+wind),(uint16_t)((y+80)*4+wind/3));
    uint8_t n2=inoise8((uint16_t)((x+80)*20+wind*3),(uint16_t)((y+80)*20));
    float v=n1/255.0f; v=v*v*gust*(0.6f+0.4f*(n2/255.0f));
    if(random(1600)==0) v=1.0f;                               // a grain catching the light
    if(v<0.05f) return false;
    c=hotColor(v,8,36,150); return true;                      // red (8) → orange → yellow (44) with brightness
  });
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 116 — MÜCKE: the bug zapper
// One to three mosquitoes zig-zag with nervous random walks and fading trails,
// each in its own colour. Now and then the zapper gets some of them — one, two
// or all of them at once. Each one that gets caught BURNS: it freezes, glows
// orange, and the zapper crackles "tz-tz … tz-tz-tz": 2–4 bursts of 2–4 very
// short flashes each, with random gaps and strengths, and every burst a DIFFERENT
// size — a small square around the bug, a bigger one, now and then the whole
// wall. Blue-white, like the real thing. Then it drops: it tumbles all the way
// down in a different colour, lies on the floor as a fading ember, and only when
// every dead one has landed do new mosquitoes fly in from the edges.
// ═══════════════════════════════════════════════════════════════════════════
#define KZ_MQ 3
#define KZ_MQ_FLOOR 57.0f
struct KzMosquito {
  uint8_t st;                       // 0 unused · 1 flying · 2 burning · 3 falling · 4 lying on the floor
  float x,y,vx,vy,spin; uint8_t hue, deadHue;
  float tr[14][2]; int trN; uint32_t trT;
  // the zap: bursts of flashes
  uint32_t zT; uint8_t bursts, flashes; bool zOn; float zI; int zHalf; uint32_t lieT;
};
static void kzSpawnMosquito(KzMosquito& m,uint8_t avoidHue){
  int side=random(4); m.st=1; m.trN=0; m.trT=0; m.spin=0;
  m.hue=(uint8_t)(avoidHue+70+random(116));
  m.deadHue=(uint8_t)(m.hue+96+random(64));                  // falls in a clearly different colour
  if(side==0){ m.x=-58; m.y=frand()*80-45; m.vx=0.06f; m.vy=0; } else if(side==1){ m.x=58; m.y=frand()*80-45; m.vx=-0.06f; m.vy=0; }
  else if(side==2){ m.x=frand()*80-40; m.y=-58; m.vx=0; m.vy=0.06f; } else { m.x=frand()*80-40; m.y=45; m.vx=0; m.vy=-0.06f; }
}
static void kzZapBurstStart(KzMosquito& q,uint32_t at){
  q.flashes=2+random(3); q.zOn=true; q.zT=at+15+random(20);
  q.zI=0.5f+0.5f*frand();
  float r=frand(); q.zHalf = r<0.15f ? 99 : (r<0.5f ? 1+random(2) : 2+random(3));   // 99 = the whole wall
}
void anim_dhMuecke(uint32_t at,float dt){
  static KzMosquito m[KZ_MQ]; static uint32_t zapAt=0; static uint8_t lastHue=0;
  if(!kzInit){
    for(int i=0;i<KZ_MQ;i++) m[i].st=0;
    int n=1+random(KZ_MQ); for(int i=0;i<n;i++){ kzSpawnMosquito(m[i],lastHue); lastHue=m[i].hue; }
    zapAt=at+4000+random(6000); kzInit=true;
  }
  // ── the zapper strikes: catch 1..all of the flying ones ──
  int flying=0; for(int i=0;i<KZ_MQ;i++) if(m[i].st==1) flying++;
  if(flying>0 && at>=zapAt){
    int catchN=1+random(flying);
    for(int i=0;i<KZ_MQ && catchN>0;i++) if(m[i].st==1 && (random(2)||catchN>=flying)){
      m[i].st=2; m[i].bursts=2+random(3); kzZapBurstStart(m[i],at+random(120)); m[i].zOn=false; catchN--; flying--; }
    zapAt=at+5000+random(7000);
  }
  for(int i=0;i<KZ_MQ;i++){ KzMosquito& q=m[i];
    switch(q.st){
      case 1:                                                  // flying
        q.vx+=(frand()-0.5f)*0.012f*dt*0.06f; q.vy+=(frand()-0.5f)*0.012f*dt*0.06f;
        { float sp=sqrtf(q.vx*q.vx+q.vy*q.vy), maxSp=0.07f; if(sp>maxSp){ q.vx*=maxSp/sp; q.vy*=maxSp/sp; } }
        q.x+=q.vx*dt; q.y+=q.vy*dt;
        if(q.x<-55){ q.x=-55; q.vx=fabsf(q.vx); } if(q.x>55){ q.x=55; q.vx=-fabsf(q.vx); }
        if(q.y<-55){ q.y=-55; q.vy=fabsf(q.vy); } if(q.y>45){ q.y=45; q.vy=-fabsf(q.vy); }
        break;
      case 2:                                                  // burning: crackling bursts
        if(at>=q.zT){
          if(q.zOn){ q.zOn=false; q.flashes--;
            if(q.flashes>0) q.zT=at+12+random(30);                                // tz-tz inside a burst
            else if(--q.bursts>0){ q.zT=at+70+random(160); q.flashes=0; }        // pause, next burst
            else { q.st=3; q.vy=-0.01f; q.vx=(frand()-0.5f)*0.04f; q.trN=0; } }   // done → it drops
          else { if(q.flashes==0) kzZapBurstStart(q,at); q.zOn=true; q.zI=0.4f+0.6f*frand(); q.zT=at+14+random(22); }
        }
        break;
      case 3:                                                  // falling slowly, tumbling
        q.vy+=dt*0.00012f; if(q.vy>0.05f) q.vy=0.05f;
        q.vx*=powf(0.5f,dt/600.0f); q.spin+=dt*0.012f;
        q.x+=q.vx*dt+sinf(q.spin)*0.25f; q.y+=q.vy*dt;
        if(q.y>=KZ_MQ_FLOOR){ q.y=KZ_MQ_FLOOR; q.st=4; q.lieT=at; }
        break;
      case 4:                                                  // on the floor, fading
        if(at-q.lieT>1800) q.st=0;
        break;
    }
    if(q.st==1||q.st==3){ if(at-q.trT>40){ q.trT=at; for(int k=13;k>0;k--){ q.tr[k][0]=q.tr[k-1][0]; q.tr[k][1]=q.tr[k-1][1]; } q.tr[0][0]=q.x; q.tr[0][1]=q.y; if(q.trN<14) q.trN++; } }
  }
  // new ones fly in only when every caught one has landed (nothing burning or falling)
  bool busy=false; int alive=0;
  for(int i=0;i<KZ_MQ;i++){ if(m[i].st==2||m[i].st==3) busy=true; if(m[i].st==1) alive++; }
  if(!busy && alive==0){
    bool anyLying=false; for(int i=0;i<KZ_MQ;i++) if(m[i].st==4) anyLying=true;
    int n=1+random(KZ_MQ);
    for(int i=0;i<KZ_MQ && n>0;i++) if(m[i].st==0||(m[i].st==4 && !anyLying)){ kzSpawnMosquito(m[i],lastHue); lastHue=m[i].hue; n--; }
    if(n>0) for(int i=0;i<KZ_MQ && n>0;i++) if(m[i].st==4){ kzSpawnMosquito(m[i],lastHue); lastHue=m[i].hue; n--; }
  }
  clearFrame();
  // the zap flashes: each burning bug lights its own square (or the whole wall), flickering
  for(int i=0;i<KZ_MQ;i++){ KzMosquito& q=m[i]; if(q.st!=2 || !q.zOn) continue;
    uint8_t w=(uint8_t)(255*q.zI); CRGB zc=CRGB((uint8_t)(w*0.75f),(uint8_t)(w*0.85f),w);
    if(q.zHalf>=99){ kzWhiteout(w); continue; }
    int col=constrain((int)((q.x+55)/10),0,10), row=constrain((int)((q.y+55)/10),0,10);
    for(int r=row-q.zHalf;r<=row+q.zHalf;r++) for(int c=col-q.zHalf;c<=col+q.zHalf;c++) fillCell(r,c,zc);
  }
  for(int i=0;i<KZ_MQ;i++){ KzMosquito& q=m[i]; if(q.st==0) continue;
    CRGB dot, trail;
    if(q.st==1){ dot=CRGB(CHSV(q.hue,170,255)); trail=CRGB(CHSV(q.hue,230,200)); }
    else if(q.st==2){ dot=CRGB(CHSV(16,255,255)); trail=dot; }                       // glowing while it burns
    else if(q.st==3){ dot=CRGB(CHSV(q.deadHue,255,255)); trail=CRGB(CHSV(q.deadHue,240,170)); }
    else { float f=1.0f-(at-q.lieT)/1800.0f; dot=kzScale(CRGB(CHSV(q.deadHue,255,200)),f); trail=CRGB::Black; }
    if(q.st==1||q.st==3) for(int k=1;k<q.trN;k++){ float f=1.0f-(float)k/q.trN; fxSplat(q.tr[k][0],q.tr[k][1],0.5f,1.0f,kzScale(trail,f*f*0.7f)); }
    fxSplat(q.x,q.y,q.st==2?1.5f:1.0f,1.2f,dot);
  }
  showGrid();
}
// ═══════════════════════════════════════════════════════════════════════════
// 117 — RÖNTGENAUGE
// An eye as an x-ray plate, in HARD contrasts — no two parts share a hue family:
//   · lids / rim: bone white (the lid really comes down on a blink, the rim with it)
//   · eyeball:    deep blue, dim
//   · iris:       saturated orange-red with radial spokes, bright
//   · limbus:     a black ring between iris and eyeball, so they never blur
//   · pupil:      white-hot
//   · two green scan lines, the only green on the wall: one sweeping down,
//     one sweeping left → right (no lattice flash any more).
// ═══════════════════════════════════════════════════════════════════════════
void anim_dhRoentgenauge(uint32_t at,float dt){
  static float gx,gy,tx,ty; static uint32_t moveT,blinkT; static bool blk;
  if(!kzInit){ gx=gy=tx=ty=0; moveT=at; blinkT=at; blk=false; kzInit=true; }
  if(at-moveT>1400){ moveT=at; tx=frand()*40.0f-20.0f; ty=frand()*8.0f-4.0f; }   // the bigger iris has less room to look around
  float k=dt*0.004f; if(k>1) k=1; gx+=(tx-gx)*k; gy+=(ty-gy)*k;
  if(!blk && at-blinkT>3600){ blk=true; blinkT=at; }
  float bp=0; if(blk){ uint32_t e=at-blinkT; if(e>=460) blk=false; else bp=1.0f-fabsf((float)e-230.0f)/230.0f; }
  float scan =fmodf(at*0.001f/2.6f,1.0f)*130.0f-65.0f;      // y of the horizontal scan line (top → bottom)
  float scanX=fmodf(at*0.001f/3.4f,1.0f)*130.0f-65.0f;      // x of the vertical one (left → right), its own pace
  const float A=50.0f, B=24.0f, RI=19.0f, RP=7.0f;            // bigger iris
  const CRGB BALL=CRGB(CHSV(160,255,255)), IRIS=CRGB(CHSV(14,255,255)), PUPIL=CRGB(255,255,255), RIM=CRGB(245,245,255), SCAN=CRGB(CHSV(96,230,255));
  clearFrame();
  kzField([&](float x,float y,CRGB& c)->bool{
    float ax=fabsf(x)/A;
    float sl=fmaxf(constrain(1.0f-fabsf(y-scan)/2.0f,0.0f,1.0f),constrain(1.0f-fabsf(x-scanX)/2.0f,0.0f,1.0f))*0.5f;   // both scan lines
    if(ax>1.05f){ if(sl<0.03f) return false; c=kzScale(SCAN,sl); return true; }
    float halfH=B*sqrtf(fmaxf(1.0f-ax*ax,0.0f));
    float lidTop=(2.0f*bp-1.0f)*halfH, lidBot=halfH;       // upper lid comes down with the blink
    float vis=constrain((y-lidTop)/1.2f,0.0f,1.0f)*constrain((lidBot-y)/1.2f,0.0f,1.0f);
    float rim=fmaxf(constrain(1.0f-fabsf(y-lidTop)/1.5f,0.0f,1.0f),constrain(1.0f-fabsf(y-lidBot)/1.5f,0.0f,1.0f));
    if(ax>1.0f) rim=0;
    float dg=sqrtf((x-gx)*(x-gx)+(y-gy)*(y-gy));
    CRGB col=CRGB::Black;
    if(vis>0.01f){
      col=kzScale(BALL,0.30f);                                                    // deep blue eyeball
      if(dg<RI){ float ang=atan2f(y-gy,x-gx); float spoke=0.5f+0.5f*cosf(ang*9.0f+at*0.0015f);
                 col=kzScale(IRIS,0.60f+0.40f*spoke); }                          // orange-red iris
      float limb=constrain(1.0f-fabsf(dg-RI)/1.6f,0.0f,1.0f);
      col=kzMix(col,CRGB::Black,limb);                                            // black limbus ring
      col=kzMix(col,PUPIL,kzCov(dg-RP,1.0f));                                    // white-hot pupil
      col=kzScale(col,vis);
    }
    if(rim>0.01f) col=kzMix(col,RIM,rim);                                         // bone-white lids
    if(sl>0.03f) col=kzMix(col,SCAN,sl);
    if((col.r|col.g|col.b)==0) return false;
    c=col; return true;
  });
  showGrid();
}
// ═══════════════════════════════════════════════════════════════════════════
// 119 — SONG 12 (placeholder)
// A square tunnel: six nested frames with a brightness wave running outward,
// slow hue drift. Symmetric and neutral — to be replaced by a proper mode once
// the twelfth song is known.
// ═══════════════════════════════════════════════════════════════════════════
void anim_dhSong12(uint32_t at,float dt){
  (void)dt;
  float tf=at*0.001f;
  clearFrame();
  for(int half=0;half<=5;half++){
    float phase=fmodf(tf*0.6f-half*0.17f,1.0f); if(phase<0) phase+=1.0f;
    float br=powf(sinf(phase*M_PI),2.0f);
    uint8_t v=(uint8_t)(br*(120.0f+half*25.0f)); if(v<3) continue;
    kzSquare(half,CRGB(CHSV((uint8_t)(tf*18.0f+half*14),(uint8_t)(230-half*20),v)));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 120 — QUAD MIRROR  (DJ, and mode 118 "Dienstag")
// Random cells in one quadrant, mirrored into all four. A fresh pattern on
// every beat, crossfaded from the previous one so it rolls rather than cuts;
// each cell carries its own hue from a palette that rotates with the bars.
// ═══════════════════════════════════════════════════════════════════════════
void anim_quadMirror(uint32_t at,float dt){
  static uint8_t pa[6][6], pb[6][6]; static uint32_t beatN=0xFFFFFFFF; static uint8_t base=0;
  if(!kzInit){ beatN=0xFFFFFFFF; base=0; memset(pa,0,sizeof(pa)); memset(pb,0,sizeof(pb)); kzInit=true; }
  KzBeat b=kzBeat(at,1.8f);
  if(b.n!=beatN){ beatN=b.n; memcpy(pa,pb,sizeof(pa)); base+=23;
    for(int r=0;r<6;r++) for(int c=0;c<6;c++) pb[r][c]=(random(100)<40)?(uint8_t)(1+random(4)):0; }
  float x=b.ph<0.5f ? b.ph*2.0f : 1.0f; x=x*x*(3.0f-2.0f*x);   // smooth crossfade in the first half-beat
  (void)dt;
  clearFrame();
  for(int r=0;r<11;r++) for(int c=0;c<11;c++){
    int fr=r<6?r:10-r, fc=c<6?c:10-c;
    float va=pa[fr][fc]?1.0f:0.0f, vb=pb[fr][fc]?1.0f:0.0f;
    float v=va*(1.0f-x)+vb*x; if(v<0.03f) continue;
    uint8_t h=base+(uint8_t)((pb[fr][fc]?pb[fr][fc]:pa[fr][fc])*50);
    fillCell(r,c,CRGB(CHSV(h,220,(uint8_t)(v*230))));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 121 — SQUARE SPIRAL  (DJ)
// A square spiral path over all 121 cells, rim to centre. The head draws the
// path in, then a tail erases it from the rim, so the spiral is always being
// born or dying. Hue runs along the path; the head is white-hot.
// ═══════════════════════════════════════════════════════════════════════════
void anim_squareSpiral(uint32_t at,float dt){
  (void)dt;
  static uint8_t sr[121],sc[121]; static bool built=false;
  if(!built){ int k=0, top=0,bot=10,lef=0,rig=10;
    while(top<=bot && lef<=rig){
      for(int c=lef;c<=rig;c++){ sr[k]=top; sc[k]=c; k++; } top++;
      for(int r=top;r<=bot;r++){ sr[k]=r; sc[k]=rig; k++; } rig--;
      if(top<=bot){ for(int c=rig;c>=lef;c--){ sr[k]=bot; sc[k]=c; k++; } bot--; }
      if(lef<=rig){ for(int r=bot;r>=top;r--){ sr[k]=r; sc[k]=lef; k++; } lef++; }
    } built=true; }
  float tf=at*0.001f;
  float ph=fmodf(tf*0.22f,1.0f);                            // one full draw+erase per ~4.5s
  int head=(int)(ph*242.0f);                                // 0..241: draw to 121, then erase to 242
  int from=head>121?head-121:0, to=head>121?121:head;
  clearFrame();
  for(int i=from;i<to;i++){
    uint8_t h=(uint8_t)(tf*30.0f+i*2); float f=0.55f+0.45f*(float)(i-from)/fmaxf((float)(to-from),1.0f);
    fillCell(sr[i],sc[i],CRGB(CHSV(h,210,(uint8_t)(f*200))));
  }
  if(to>from && to<=121) fillCell(sr[to-1],sc[to-1],CRGB(255,255,255));   // the head
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 122 — GRID PULSE  (DJ)
// A checkerboard on the beat: one parity lights, decays over the beat, then the
// other parity takes the next hit. The hue shifts every bar; on every eighth
// beat the whole board flashes white, inverted.
// ═══════════════════════════════════════════════════════════════════════════
void anim_gridPulse(uint32_t at,float dt){
  (void)dt;
  static uint32_t beatN=0xFFFFFFFF; static uint8_t hue=0;
  if(!kzInit){ beatN=0xFFFFFFFF; hue=0; kzInit=true; }
  KzBeat b=kzBeat(at,2.0f);
  if(b.n!=beatN){ beatN=b.n; if((b.n&3)==0) hue+=48; }
  int parity=b.n&1; bool slam=(b.n&7)==0 && b.ph<0.12f;
  float decay=powf(1.0f-b.ph,1.4f);
  clearFrame();
  for(int r=0;r<11;r++) for(int c=0;c<11;c++){
    bool on=((r+c)&1)==parity;
    if(slam) on=!on;
    if(!on) continue;
    uint8_t v=slam?255:(uint8_t)(30+decay*225);
    fillCell(r,c,slam?CRGB(v,v,v):CRGB(CHSV((uint8_t)(hue+(r+c)*3),225,v)));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
//  DRAHTHAUS TEST PACK (123..126) — big lettering and word art
//  A bold 5x7 block font in WORLD units: one font pixel is a full grid square
//  (~11 units), so every stroke is a band a whole square wide and a letter is
//  7 of the 11 rows tall. Because it is drawn per LED in world coordinates (not
//  per cell), text can scroll and wave SMOOTHLY instead of jumping square by square.
// ═══════════════════════════════════════════════════════════════════════════
static uint8_t kzGlyphRow(char ch,int r){            // bit 4 = left column
  static const uint8_t F[][7]={
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, // D
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // I
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
  };
  static const char K[]="ADEHILMNOPRSTU";
  if(r<0||r>6) return 0;
  for(int i=0;K[i];i++) if(K[i]==ch) return F[i][r];
  return 0;                                            // space / unknown
}
#define KZ_CELL 11.1f      // one font pixel = one grid square
// Is text-space point (tx,ty) inside a lit font pixel? tx from the text start, ty from the glyph top.
// Glyph advance = 6 pixels (5 + 1 gap). Returns the letter index (or -1) via `li`.
static inline bool kzTextPx(const char* txt,int len,float tx,float ty,int& li,int& gr,int& gc){
  if(tx<0||ty<0) return false;
  int cx=(int)(tx/KZ_CELL), cy=(int)(ty/KZ_CELL); if(cy>6) return false;
  li=cx/6; int col=cx%6; if(li>=len||col==5) return false;
  gr=cy; gc=col;
  return (kzGlyphRow(txt[li],cy)>>(4-col))&1;
}

// ═══════════════════════════════════════════════════════════════════════════
// 123 — DRAHTHAUS RIESENSCHRIFT
// "DRAHTHAUS" in giant block letters scrolling smoothly right → left, endless.
// Album palette: each letter runs from deep orange at the bottom to yellow at the
// top, a white shine band sweeps across the word, and a copper drop shadow sits
// one half-square down-right — classic sign lettering.
// ═══════════════════════════════════════════════════════════════════════════
void anim_dhRiesenschrift(uint32_t at,float dt){
  (void)dt;
  static const char* TXT="DRAHTHAUS"; const int len=9;
  const float W=len*6*KZ_CELL, SPAN=W+124.0f;
  float scroll=fmodf(at*0.022f,SPAN);                 // units scrolled
  float top=-61.0f+2.0f*KZ_CELL;                      // glyphs on rows 2..8
  float shine=fmodf(at*0.045f,W+60.0f)-30.0f;
  clearFrame();
  kzField([&](float x,float y,CRGB& c)->bool{
    float tx=x+62.0f-124.0f+scroll, ty=y-top;          // text enters from the right edge
    int li,gr,gc;
    if(kzTextPx(TXT,len,tx,ty,li,gr,gc)){
      float g=1.0f-gr/6.0f;                            // 1 at the top row
      CRGB col=CRGB(CHSV((uint8_t)(14+g*30),240,(uint8_t)(170+g*85)));
      float sh=constrain(1.0f-fabsf(tx-shine-(6.0f-gr)*3.0f)/9.0f,0.0f,1.0f);
      c=kzMix(col,CRGB(255,250,220),sh*0.8f); return true;
    }
    if(kzTextPx(TXT,len,tx-KZ_CELL*0.5f,ty-KZ_CELL*0.5f,li,gr,gc)){ c=CRGB(CHSV(20,200,55)); return true; }   // drop shadow
    return false;
  });
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 124 — ELEPHANT IN THE ROOM (WordArt)
// The album title as 90s word art: every letter its own rainbow colour, a deep
// 3-step extrusion behind it (darker copies stepping down-right), and the whole
// line riding a sine wave that travels along the text — smooth, endless scroll.
// ═══════════════════════════════════════════════════════════════════════════
void anim_dhElephantRoom(uint32_t at,float dt){
  (void)dt;
  static const char* TXT="ELEPHANT IN THE ROOM"; const int len=20;
  const float W=len*6*KZ_CELL, SPAN=W+124.0f;
  float scroll=fmodf(at*0.026f,SPAN);
  float base=-61.0f+2.0f*KZ_CELL, tf=at*0.001f;
  clearFrame();
  kzField([&](float x,float y,CRGB& c)->bool{
    float tx=x+62.0f-124.0f+scroll;
    float wave=sinf(tx*0.018f-tf*2.4f)*KZ_CELL*0.9f;   // the line rides a travelling wave
    int li,gr,gc;
    if(kzTextPx(TXT,len,tx,y-base-wave,li,gr,gc)){
      uint8_t h=(uint8_t)(li*23+tf*20.0f);
      float g=1.0f-gr/6.0f;
      c=CRGB(CHSV(h,(uint8_t)(255-g*90),255)); return true;   // whiter toward the top: a gloss
    }
    for(int k=1;k<=3;k++){                             // extrusion, back to front
      float o=KZ_CELL*0.28f*k;
      if(kzTextPx(TXT,len,tx-o,y-base-wave-o,li,gr,gc)){
        uint8_t h=(uint8_t)(li*23+tf*20.0f);
        c=CRGB(CHSV(h,255,(uint8_t)(150-k*35))); return true; }
    }
    return false;
  });
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 125 — DH MONOGRAMM
// A giant "DH" filling the wall (two 5x7 letters, 11 squares wide). The letters
// glow orange (D) and yellow (H); a neon contour runs around them with a bright
// light chasing along it; every beat the letters punch brighter, and every 8th
// beat the picture inverts for one beat — the background lights, the letters go
// black.
// ═══════════════════════════════════════════════════════════════════════════
void anim_dhMonogramm(uint32_t at,float dt){
  (void)dt;
  static const char* TXT="DH";
  KzBeat b=kzBeat(at,1.6f);
  float punch=powf(1.0f-b.ph,2.0f);
  bool inv=(b.n%8)==0;
  float top=-61.0f+2.0f*KZ_CELL, left=-61.0f;
  float chase=at*0.0008f;
  clearFrame();
  kzField([&](float x,float y,CRGB& c)->bool{
    float tx=x-left, ty=y-top; int li,gr,gc;
    bool on=kzTextPx(TXT,2,tx,ty,li,gr,gc);
    // contour: inside a lit pixel but within 2.2 units of an unlit neighbour
    bool edge=false;
    if(on){ int l2,r2,c2; const float e=2.2f;
      edge=!kzTextPx(TXT,2,tx-e,ty,l2,r2,c2)||!kzTextPx(TXT,2,tx+e,ty,l2,r2,c2)||!kzTextPx(TXT,2,tx,ty-e,l2,r2,c2)||!kzTextPx(TXT,2,tx,ty+e,l2,r2,c2); }
    if(inv){ if(on) return false; c=CRGB(CHSV(28,230,(uint8_t)(90+120*punch))); return true; }
    if(!on) return false;
    uint8_t h = li==0 ? 18 : 42;
    CRGB col=CRGB(CHSV(h,235,(uint8_t)(110+100*punch)));
    if(edge){ float ang=atan2f(y-0.0f,x)/(2*M_PI)+0.5f; float ch=fmodf(ang-chase+1.0f,1.0f);
      float spot=ch<0.06f?1.0f-ch/0.06f:0.0f;
      col=kzMix(CRGB(CHSV(h,120,255)),CRGB(255,255,255),spot); }
    c=col; return true;
  });
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 126 — ELEFANTENKOPF
// One giant pink elephant head, front view, filling the wall: huge ears that flap
// slowly, two small eyes that blink, white tusks, and a long trunk swinging in
// the middle — every few seconds it rises and trumpets a spray of water.
// ═══════════════════════════════════════════════════════════════════════════
void anim_dhElefantenKopf(uint32_t at,float dt){
  (void)dt;
  static uint32_t blinkT=0; static bool blk=false;
  if(!kzInit){ blinkT=at; blk=false; kzInit=true; }
  float tf=at*0.001f;
  float flap=0.78f+0.22f*sinf(tf*1.6f);                          // ear width breathing
  if(!blk && at-blinkT>4200){ blk=true; blinkT=at; }
  float lid=0; if(blk){ uint32_t e=at-blinkT; if(e>=300) blk=false; else lid=1.0f-fabsf((float)e-150.0f)/150.0f; }
  float raise=constrain(sinf(tf*0.45f)*2.0f-1.2f,0.0f,1.0f); raise=raise*raise*(3-2*raise);
  float sw=sinf(tf*1.3f)*(1.0f-raise)*9.0f;
  // trunk joints from the face down (or up when raised)
  float j[5][2]={{0,8},{0,0},{0,0},{0,0},{0,0}};
  for(int k=1;k<5;k++){ float t=k/4.0f;
    float hx=sw*t*t, hy=8+t*50;                                    // hanging, swinging
    float rx=sinf(t*2.2f)*14*t, ry=8-t*44+t*t*10;                  // curled up beside the head
    j[k][0]=hx+(rx-hx)*raise; j[k][1]=hy+(ry-hy)*raise; }
  const CRGB PINK=CRGB(CHSV(236,170,255)), EAR=CRGB(CHSV(240,215,255)), TUSK=CRGB(240,235,215), EYE=CRGB(255,205,0);
  clearFrame();
  kzField([&](float x,float y,CRGB& c)->bool{
    float dHead=sqrtf(x*x+(y+6)*(y+6))-24.0f;
    float dEarL=kzEllipseD(x,y,-34,-6,20*flap,27), dEarR=kzEllipseD(x,y,34,-6,20*flap,27);
    float dTr=1e9f; for(int k=0;k<4;k++){ float r=5.0f-k*0.9f; dTr=fminf(dTr,kzSegDist(x,y,j[k][0],j[k][1],j[k+1][0],j[k+1][1])-r); }
    float dTusk=fminf(kzSegDist(x,y,-9,10,-13,24),kzSegDist(x,y,9,10,13,24))-1.6f;
    float eyeH=2.6f*(1.0f-lid)+0.4f;
    float dEye=fminf(kzEllipseD(x,y,-11,-12,2.6f,eyeH),kzEllipseD(x,y,11,-12,2.6f,eyeH));
    CRGB col=CRGB::Black; float cov=0;
    float ce=fmaxf(kzCov(dEarL,1.0f),kzCov(dEarR,1.0f));
    if(ce>0){ float rim=constrain(1.0f-fabsf(fminf(dEarL,dEarR)+0.6f)/1.2f,0.0f,1.0f);
      col=kzScale(EAR,0.35f+0.5f*rim); cov=ce; }
    float ch=kzCov(fminf(dHead,dTr),1.0f);
    if(ch>0){ float rim=constrain(1.0f-fabsf(fminf(dHead,dTr)+0.6f)/1.2f,0.0f,1.0f);
      col=kzMix(col,kzScale(PINK,0.55f+0.45f*rim),ch); cov=fmaxf(cov,ch); }
    float ct=kzCov(dTusk,1.0f); if(ct>0){ col=kzMix(col,TUSK,ct); cov=fmaxf(cov,ct); }
    float cy=kzCov(dEye,0.8f); if(cy>0) col=kzMix(col,EYE,cy);
    if(cov<=0) return false;
    c=kzScale(col,cov); return true;
  });
  if(raise>0.8f){                                                  // the trumpet: water out of the raised tip
    float tx=j[4][0], ty=j[4][1];
    for(int k=0;k<9;k++){ float ph=fmodf(tf*1.3f+k/9.0f,1.0f);
      fxSplat(tx+(k-4)*ph*3.0f, ty-ph*30.0f+ph*ph*26.0f, 1.0f,1.2f, kzScale(CRGB(170,215,255),(raise-0.75f)*4.0f*(1.0f-ph))); }
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 127 — SCHWARZ (TOGA_MODE_SCHWARZ): the dark standby
// Everything off except ONE small dot on the bottom-right crossing, breathing
// slowly in a soft cool colour, so you can see the wall is alive and on purpose
// dark (Drahthaus setlist: the position before the submarine). It bypasses
// showGrid(): the AGC would otherwise blow a single dot up to full brightness.
// The global brightness still scales it; the colour follows the mode's own
// colour (button 8 / web picker), native = teal. Modules go fully dark.
// ═══════════════════════════════════════════════════════════════════════════
void anim_schwarz(uint32_t at){
  clearFrame();
  float br=0.6f+0.4f*(0.5f+0.5f*sinf(at*0.0015f));               // slow breath, never off
  uint8_t hue=(uint8_t)(130+gHueBase+gHueAuto);
  fxSplat(X_POS[11],Y_POS[11],0.9f,1.0f,kzScale(CRGB(CHSV(hue,170,255)),br));
  gCoverage=0; gFrameHue=hue;                                     // modules: nothing lit (they render black anyway)
  FastLED.setBrightness(max((int)gBaseBri,12));                   // stays visible at the lowest master setting
  FastLED.show();
}
