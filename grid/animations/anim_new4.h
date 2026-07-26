// Showpiece pack #4 — 15 more self-contained modes (grid 87..101). Same contract
// as anim_new3.h (which is #included first, so n3hash/n3dts are already visible).
// void anim_<name>(uint32_t at,float dt); ends with showGrid().
#pragma once

// ═══════════════════════════════════════════════════════════════════════════
// 87 — INK BLOOM
// Drops of coloured ink hit water and bloom — a soft core that swells while six
// arms curl outward and thin — fading as they diffuse, new drops landing over top.
// ═══════════════════════════════════════════════════════════════════════════
#define IK_N 6
struct Ik{ float x,y,r,mr; uint8_t hue; bool live; };
static Ik ikP[IK_N]; static bool ikInit=false; static float ikAcc=0;
void anim_inkBloom(uint32_t at,float dt){
  float dts=n3dts(dt,0.1f); (void)at;
  if(!ikInit){ for(int i=0;i<IK_N;i++) ikP[i].live=false; ikInit=true; }
  ikAcc-=dts;
  if(ikAcc<=0.0f){ for(int i=0;i<IK_N;i++) if(!ikP[i].live){
      ikP[i]=(Ik){(frand()-0.5f)*80,(frand()-0.5f)*80,1.0f,20.0f+frand()*20.0f,(uint8_t)(frand()*255),true}; break; }
    ikAcc=0.5f+frand()*0.6f;
  }
  clearFrame();
  for(int i=0;i<IK_N;i++) if(ikP[i].live){
    ikP[i].r+=8.0f*dts;
    if(ikP[i].r>ikP[i].mr){ ikP[i].live=false; continue; }
    float age=ikP[i].r/ikP[i].mr, fade=1.0f-age;
    fxSplat(ikP[i].x,ikP[i].y,ikP[i].r*0.5f,ikP[i].r, CRGB(CHSV(ikP[i].hue,220,(uint8_t)(fade*120))));
    for(int a=0;a<6;a++){                              // curling arms
      float ang=a*(6.2832f/6.0f)+ikP[i].r*0.08f;
      fxSplat(ikP[i].x+cosf(ang)*ikP[i].r,ikP[i].y+sinf(ang)*ikP[i].r,1.0f+age*2.0f,2.0f,
              CRGB(CHSV(ikP[i].hue,200,(uint8_t)(fade*160))));
    }
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 88 — ELECTRIC EEL
// A serpentine body swims across on a travelling sine, glowing blue-green, firing
// crackling white discharges off random points along its length. Sleek and menacing.
// ═══════════════════════════════════════════════════════════════════════════
void anim_electricEel(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float hx=-70.0f+fmodf(tf*26.0f,160.0f);              // head sweeps L→R
  const int seg=18; float px=hx,py=0; float bodyY[19];
  for(int s=0;s<=seg;s++){
    float u=(float)s/seg, x=hx-s*7.0f;
    float y=sinf(x*0.06f-tf*4.0f)*16.0f+sinf(tf*0.7f)*6.0f; bodyY[s]=y;
    float b=0.4f+0.6f*(1.0f-u);
    if(s>0) fxStreak(px,py,x,y,0.7f,1.2f, CRGB(CHSV(140,200,(uint8_t)(b*200))));
    else    fxSplat(x,y,1.8f,1.4f, CRGB(CHSV(150,150,240)));
    px=x; py=y;
  }
  for(int e=0;e<3;e++){                                // electric discharges
    int s=(int)(n3hash(floorf(tf*8.0f)+e*3.7f)*seg); s=constrain(s,0,seg);
    float sx=hx-s*7.0f, sy=bodyY[s];
    float ex=sx+(n3hash(tf*13.0f+e)-0.5f)*30.0f, ey=sy+(n3hash(tf*17.0f+e*2.0f)-0.5f)*30.0f;
    fxStreak(sx,sy,ex,ey,0.4f,1.0f, CRGB(CHSV(160,80,220)));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 89 — GALAXY COLLISION
// Two spiral galaxies whose cores ORBIT their common centre (so they visibly
// swing around each other, not just spin in place), arms winding up and unwinding,
// THREE arms each so the rotation reads (a 2-arm spiral is 180°-symmetric and looks
// frozen). They swoop close — cores flare, a wobbling tidal bridge of stars is torn
// between them — then separate. Nothing holds still.
// ═══════════════════════════════════════════════════════════════════════════
void anim_galaxyCollision(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float orbit=tf*0.5f;                                 // the pair revolves about its centre
  float app  =0.5f+0.5f*sinf(tf*0.22f);                // 0 far .. 1 close
  float sep  =8.0f+(1.0f-app)*30.0f;                   // core separation breathes
  float c0x=cosf(orbit)*sep,          c0y=sinf(orbit)*sep;
  float c1x=cosf(orbit+3.14159f)*sep, c1y=sinf(orbit+3.14159f)*sep;
  float wind=3.0f+2.0f*sinf(tf*0.35f);                 // arms wind tight / unwind
  for(int gx=0;gx<2;gx++){
    float cx=gx?c1x:c0x, cy=gx?c1y:c0y;
    float rot=tf*(gx?1.1f:-1.4f);                       // different spin rates
    uint8_t base=gx?200:28;
    const int ARMS=3;                                   // 3 arms → spin is unmistakable
    for(int a=0;a<ARMS;a++){
      float a0=rot+a*(6.2832f/ARMS);
      float armLen=22.0f*(0.7f+0.3f*sinf(tf*0.6f+a*1.7f+gx));   // unequal, breathing arms
      float prevx=cx,prevy=cy;
      for(int s=1;s<=14;s++){
        float u=(float)s/14, rr=u*armLen, ang=a0+u*wind;
        float x=cx+cosf(ang)*rr, y=cy+sinf(ang)*rr;
        fxStreak(prevx,prevy,x,y,0.5f,1.1f, CRGB(CHSV((uint8_t)(base+u*30+tf*10),200,(uint8_t)((1.0f-u*0.55f)*180))));
        prevx=x; prevy=y;
      }
    }
    float burst=app*app;                                // cores flare on close approach
    fxSplat(cx,cy,2.2f+burst*2.0f,2.0f, CRGB(CHSV(base,(uint8_t)(140-burst*90),(uint8_t)(200+burst*55))));
  }
  if(app>0.4f){                                         // tidal bridge torn between the cores
    float k=(app-0.4f)/0.6f; const int n=10;
    for(int i=1;i<n;i++){ float u=(float)i/n;
      float bx=c0x+(c1x-c0x)*u+sinf(u*3.14159f)*sinf(tf*2.0f)*8.0f;
      float by=c0y+(c1y-c0y)*u+sinf(u*3.14159f)*cosf(tf*2.0f)*8.0f;
      fxSplat(bx,by,0.7f,1.2f, CRGB(CHSV((uint8_t)(120+u*80),160,(uint8_t)(k*160))));
    }
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 90 — GYROSCOPE
// Three nested rings spinning on different tilted axes — ellipses whose squash
// and rotation shift so the whole gimbal appears to tumble in 3D. Precise, kinetic.
// ═══════════════════════════════════════════════════════════════════════════
void anim_gyroscope(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  for(int r=0;r<3;r++){
    float rr=16.0f+r*13.0f;
    float minor=fabsf(sinf(tf*(0.5f+r*0.3f)));          // axis tilt → squash
    float phi=tf*(0.4f+r*0.2f)*((r%2)?-1.0f:1.0f);
    uint8_t hue=(uint8_t)(r*60+tf*20); float px=0,py=0;
    for(int s=0;s<=28;s++){
      float a=s/28.0f*6.2832f, ex=cosf(a)*rr, ey=sinf(a)*rr*minor;
      float x=ex*cosf(phi)-ey*sinf(phi), y=ex*sinf(phi)+ey*cosf(phi);
      if(s>0) fxStreak(px,py,x,y,0.5f,1.0f, CRGB(CHSV(hue,200,180)));
      px=x; py=y;
    }
  }
  fxSplat(0,0,2.0f,1.6f, CRGB(255,255,255));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 91 — DOUBLE PENDULUM
// A real chaotic double pendulum swings from a pivot, its tip painting a long
// glowing trail that never once repeats. Physics you can watch forever.
// ═══════════════════════════════════════════════════════════════════════════
static bool dpInit=false; static float dpA1,dpA2,dpV1,dpV2;
#define DP_TRAIL 64
static float dpTx[DP_TRAIL], dpTy[DP_TRAIL]; static int dpHead=0; static bool dpFull=false;
void anim_doublePendulum(uint32_t at,float dt){
  float dts=n3dts(dt,0.05f); (void)at;
  if(!dpInit){ dpA1=1.9f; dpA2=2.6f; dpV1=dpV2=0; dpHead=0; dpFull=false; dpInit=true; }
  const float L1=20,L2=20,m1=1,m2=1,g=30;
  for(int it=0;it<4;it++){                              // sub-step the integrator
    float d=dts/4.0f, a1=dpA1,a2=dpA2,w1=dpV1,w2=dpV2, del=a1-a2;
    float den=(2*m1+m2-m2*cosf(2*del));
    float da1=(-g*(2*m1+m2)*sinf(a1)-m2*g*sinf(a1-2*a2)-2*sinf(del)*m2*(w2*w2*L2+w1*w1*L1*cosf(del)))/(L1*den);
    float da2=(2*sinf(del)*(w1*w1*L1*(m1+m2)+g*(m1+m2)*cosf(a1)+w2*w2*L2*m2*cosf(del)))/(L2*den);
    dpV1+=da1*d; dpV2+=da2*d;
    dpV1=constrain(dpV1,-18.0f,18.0f); dpV2=constrain(dpV2,-18.0f,18.0f);   // keep bounded
    dpA1+=dpV1*d; dpA2+=dpV2*d;
  }
  const float pvy=-24.0f;
  float x1=sinf(dpA1)*L1, y1=cosf(dpA1)*L1+pvy;
  float x2=x1+sinf(dpA2)*L2, y2=y1+cosf(dpA2)*L2;
  dpTx[dpHead]=x2; dpTy[dpHead]=y2; dpHead=(dpHead+1)%DP_TRAIL; if(dpHead==0)dpFull=true;
  clearFrame();
  int cnt=dpFull?DP_TRAIL:dpHead;
  for(int i=0;i<cnt;i++){ int idx=(dpHead-1-i+DP_TRAIL)%DP_TRAIL; float f=1.0f-(float)i/cnt;
    fxSplat(dpTx[idx],dpTy[idx],0.5f,1.2f, CRGB(CHSV((uint8_t)(160+i*2),200,(uint8_t)(f*f*200)))); }
  fxStreak(0,pvy,x1,y1,0.6f,1.1f, CRGB(180,180,180));
  fxStreak(x1,y1,x2,y2,0.6f,1.1f, CRGB(180,180,180));
  fxSplat(x1,y1,1.2f,1.2f, CRGB(CHSV(40,200,220)));
  fxSplat(x2,y2,1.6f,1.4f, CRGB(CHSV(200,220,240)));
  fxSplat(0,pvy,1.0f,1.0f, CRGB(120,120,120));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 92 — PULSAR BEACON
// A neutron star spinning its twin lighthouse beams around the panel while shells
// of radiation ripple outward on every turn, the core flickering. Rhythmic, stark.
// ═══════════════════════════════════════════════════════════════════════════
#define PB_SH 5
struct PbSh{ float r; bool live; };
static PbSh pbSh[PB_SH]; static bool pbInit=false; static float pbAcc=0;
void anim_pulsarBeacon(uint32_t at,float dt){
  float dts=n3dts(dt,0.1f); float tf=at*0.001f;
  if(!pbInit){ for(int i=0;i<PB_SH;i++) pbSh[i].live=false; pbInit=true; }
  clearFrame();
  float a=tf*2.2f;
  for(int b=0;b<2;b++){ float ang=a+b*3.14159f;        // twin beams
    for(int f=-1;f<=1;f++){ float aa=ang+f*0.06f;
      fxStreak(0,0,cosf(aa)*72.0f,sinf(aa)*72.0f,0.5f,2.0f, CRGB(CHSV(150,120,(uint8_t)(120-fabsf((float)f)*40)))); } }
  pbAcc-=dts; if(pbAcc<=0.0f){ for(int i=0;i<PB_SH;i++) if(!pbSh[i].live){ pbSh[i]=(PbSh){2.0f,true}; break; } pbAcc=0.7f; }
  for(int i=0;i<PB_SH;i++) if(pbSh[i].live){ pbSh[i].r+=26.0f*dts;
    if(pbSh[i].r>72.0f){ pbSh[i].live=false; continue; }
    fxRing(0,0,pbSh[i].r,0.7f,1.2f, CRGB(CHSV(160,180,(uint8_t)((1.0f-pbSh[i].r/72.0f)*160)))); }
  float fl=0.6f+0.4f*sinf(tf*18.0f);
  fxSplat(0,0,2.0f+fl,1.6f, CRGB(220,230,255));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 93 — WORMHOLE THROAT
// Nested rings streaming inward and bunching toward a leaning bright throat, the
// mouth swaying — a tunnel you fall down forever. Deep, dizzying.
// ═══════════════════════════════════════════════════════════════════════════
void anim_wormholeThroat(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  const int R=9;
  for(int i=0;i<R;i++){
    float u=fmodf((float)i/R+tf*0.25f,1.0f);
    float rad=4.0f+(1.0f-u)*(1.0f-u)*60.0f;             // perspective bunching
    float ox=sinf(tf*0.5f)*6.0f*(1.0f-u), oy=cosf(tf*0.4f)*6.0f*(1.0f-u);
    fxRing(ox,oy,rad,0.7f,1.3f, CRGB(CHSV((uint8_t)(160+u*60),200,(uint8_t)((0.3f+0.7f*u)*180))));
  }
  fxSplat(sinf(tf*0.5f)*3.0f,cosf(tf*0.4f)*3.0f,2.5f,3.0f, CRGB(CHSV(200,150,200)));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 94 — SPIROGRAPH ARMS
// Chains of arms rotating at different rates draw a wandering point that leaves a
// comet trail — three chains at once, weaving those looping spirograph curves. A
// direct cousin of the radar-arms mode: arms overtaking arms.
// ═══════════════════════════════════════════════════════════════════════════
void anim_spirographArms(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  const float w[3]={0.7f,-1.6f,2.4f}, len[3]={22.0f,14.0f,8.0f};
  for(int c=0;c<3;c++){
    float base=c*2.094f, k=1.0f+c*0.2f; uint8_t hue=(uint8_t)(c*80+tf*20);
    float x=0,y=0;
    for(int l=0;l<3;l++){ float a=base+tf*w[l]*k;       // draw the arm chain
      float nx=x+cosf(a)*len[l], ny=y+sinf(a)*len[l];
      fxStreak(x,y,nx,ny,0.4f,1.0f, CRGB(CHSV(hue,180,120))); x=nx; y=ny; }
    fxSplat(x,y,1.4f,1.4f, CRGB(CHSV((uint8_t)(hue+40),220,240)));   // tracing tip
    for(int b=1;b<=16;b++){                             // analytic comet trail
      float tb=tf-b*0.03f, xx=0,yy=0;
      for(int l=0;l<3;l++){ float a=base+tb*w[l]*k; xx+=cosf(a)*len[l]; yy+=sinf(a)*len[l]; }
      fxSplat(xx,yy,0.5f,1.0f, CRGB(CHSV((uint8_t)(hue+40),200,(uint8_t)((1.0f-b/16.0f)*160))));
    }
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 95 — HONEYCOMB PULSE
// The whole lattice becomes a tessellated field of cells, a radial wave of colour
// and light sweeping out through them from the centre. Geometric, blocky, alive.
// ═══════════════════════════════════════════════════════════════════════════
void anim_honeycombPulse(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  for(int r=0;r<11;r++) for(int cc=0;cc<11;cc++){
    float cx=(cc-5)*10.0f, cy=(r-5)*10.0f, d=sqrtf(cx*cx+cy*cy);
    float v=sinf(d*0.12f-tf*3.0f)*0.5f+0.5f; v*=v;
    if(v>0.15f) fillCell(r,cc, CRGB(CHSV((uint8_t)(d*1.5f+tf*20),220,(uint8_t)(v*220))));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 96 — TURBINE BLADES
// A great curved-blade turbine spinning up and winding down, blades trailing a
// motion-blur ghost. Mechanical power, felt as speed.
// ═══════════════════════════════════════════════════════════════════════════
static float tbA=0;
void anim_turbineBlades(uint32_t at,float dt){
  float dts=n3dts(dt,0.1f); float tf=at*0.001f;
  float spd=1.5f+1.2f*sinf(tf*0.4f); tbA+=spd*dts*4.0f;
  clearFrame();
  const int blades=6; uint8_t hue=(uint8_t)(tf*30);
  for(int b=0;b<blades;b++){
    float a=tbA+b*(6.2832f/blades), px=0,py=0;
    for(int s=1;s<=6;s++){ float u=(float)s/6, rr=8.0f+u*40.0f, aa=a-u*0.6f;
      float x=cosf(aa)*rr, y=sinf(aa)*rr;
      fxStreak(px,py,x,y,0.7f,1.4f, CRGB(CHSV(hue,180,(uint8_t)((0.4f+0.6f*u)*200)))); px=x; py=y; }
    float ag=a-0.25f;                                   // blur ghost trailing
    fxStreak(cosf(ag)*8.0f,sinf(ag)*8.0f,cosf(ag-0.6f)*48.0f,sinf(ag-0.6f)*48.0f,0.5f,2.0f, CRGB(CHSV(hue,150,60)));
  }
  fxSplat(0,0,3.0f,2.0f, CRGB(CHSV(hue,120,200)));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 97 — MANTA GLIDE
// A great ray banks and glides through dark water, wings undulating, a string of
// bioluminescent spots trailing from its tail. Serene, weightless.
// ═══════════════════════════════════════════════════════════════════════════
void anim_mantaGlide(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  float cx=sinf(tf*0.35f)*30.0f, cy=sinf(tf*0.27f+1.0f)*22.0f;
  float vx=cosf(tf*0.35f)*30.0f*0.35f, vy=cosf(tf*0.27f+1.0f)*22.0f*0.27f;
  float head=atan2f(vy,vx), ca=cosf(head), sa=sinf(head);
  float flap=sinf(tf*2.2f); const uint8_t hue=180;
  for(int side=-1;side<=1;side+=2){                     // two undulating wings
    float px=cx,py=cy;
    for(int s=1;s<=7;s++){ float u=(float)s/7;
      float wx=cx+(-sa)*side*u*30.0f-ca*u*u*10.0f, wy=cy+ca*side*u*30.0f-sa*u*u*10.0f;
      float b=constrain(0.4f+0.6f*(1.0f-u)+0.2f*flap*u,0.0f,1.0f);
      fxStreak(px,py,wx,wy,0.7f,1.3f, CRGB(CHSV(hue,180,(uint8_t)(b*200)))); px=wx; py=wy; }
  }
  fxSplat(cx,cy,2.2f,1.8f, CRGB(CHSV(hue,140,220)));
  fxSplat(cx+ca*6.0f,cy+sa*6.0f,1.4f,1.2f, CRGB(CHSV((uint8_t)(hue+20),120,200)));
  fxStreak(cx-ca*4.0f,cy-sa*4.0f,cx-ca*30.0f,cy-sa*30.0f,0.4f,1.0f, CRGB(CHSV(hue,200,120)));
  for(int i=0;i<6;i++){ float u=(float)i/6;             // bioluminescent tail spots
    fxSplat(cx-ca*(8.0f+u*24.0f),cy-sa*(8.0f+u*24.0f),0.5f,1.0f, CRGB(CHSV(140,220,(uint8_t)((1.0f-u)*180)))); }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 98 — PLASMA GLOBE
// A glass sphere with electric tendrils streaming from the core to a wandering
// "touch" point, the strongest filament bending to follow it, all crackling and
// forking. A cousin of the octopus-in-a-sphere — but pure lightning.
// ═══════════════════════════════════════════════════════════════════════════
void anim_plasmaGlobe(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  fxRing(0,0,48.0f,0.8f,2.0f, CRGB(CHSV(200,120,40)));   // glass shell
  float ta=tf*0.7f, tx=cosf(ta)*46.0f, ty=sinf(ta*1.3f)*46.0f;   // touch point
  const int fil=5; float frame=floorf(tf*10.0f);
  for(int f=0;f<fil;f++){
    float dirA=tf*0.3f+f*(6.2832f/fil);
    float tgx=cosf(dirA)*46.0f, tgy=sinf(dirA)*46.0f;
    if(f==0){ tgx=tx; tgy=ty; }                          // hottest filament follows touch
    float px=0,py=0, flick=0.5f+0.5f*sinf(tf*20.0f+f*3.0f); uint8_t hue=(uint8_t)(190+f*6);
    for(int s=1;s<=7;s++){ float u=(float)s/7;
      float jx=(n3hash(f*3.1f+s+frame)-0.5f)*10.0f*(1.0f-u), jy=(n3hash(f*5.7f+s+frame+9.0f)-0.5f)*10.0f*(1.0f-u);
      float x=tgx*u+jx, y=tgy*u+jy;
      float b=(f==0?1.0f:0.5f)*flick*(0.5f+0.5f*u);
      fxStreak(px,py,x,y,0.4f,1.0f, CRGB(CHSV(hue,180,(uint8_t)(b*220)))); px=x; py=y; }
  }
  fxSplat(0,0,2.4f,1.8f, CRGB(CHSV(24,120,240)));
  fxSplat(tx,ty,1.6f,1.6f, CRGB(CHSV(210,120,200)));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 99 — SUPERNOVA SHELL
// A star detonates — a white flash, then a filamentary shell blasting outward
// (lumpy, noisy edge) with an inner reverse-shock ring, fading to a remnant before
// it all happens again. Explosive, cyclic.
// ═══════════════════════════════════════════════════════════════════════════
static float snbAge=0; static bool snbInit=false;
void anim_supernovaShell(uint32_t at,float dt){
  float dts=n3dts(dt,0.1f); (void)at;
  if(!snbInit){ snbAge=0; snbInit=true; }
  snbAge+=dts;
  float p=fmodf(snbAge,6.0f)/6.0f;                      // 0..1 blast cycle
  clearFrame();
  if(p<0.05f){ float kk=1.0f-p/0.05f; fxSplat(0,0,6.0f+kk*30.0f,6.0f, CRGB(255,255,255)); }
  float rad=p*66.0f, fade=1.0f-p;
  for(int a=0;a<48;a++){                                // lumpy filamentary shell
    float ang=a/48.0f*6.2832f, nz=n3hash(a*1.3f);
    float rr=rad*(0.85f+nz*0.3f), b=fade*(0.4f+0.6f*nz);
    fxSplat(cosf(ang)*rr,sinf(ang)*rr,0.8f,1.3f, hotColor(b,(uint8_t)(nz*40+p*30),30,60));
  }
  fxRing(0,0,rad*0.5f,0.6f,1.2f, CRGB(CHSV(180,180,(uint8_t)(fade*120))));   // reverse shock
  fxSplat(0,0,1.6f,1.4f, CRGB(CHSV(30,120,(uint8_t)(fade*200))));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 100 — COMET SWARM
// A blazing star with a flock of comets on elliptical orbits, each tail always
// swept away from the star, flaring longer as they whip past perihelion. Elegant
// celestial mechanics.
// ═══════════════════════════════════════════════════════════════════════════
#define CS_N 7
struct Cs{ float a,ecc,phase,spd; uint8_t hue; };
static Cs csP[CS_N]; static bool csInit=false;
void anim_cometSwarm(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  if(!csInit){ for(int i=0;i<CS_N;i++)
      csP[i]=(Cs){18.0f+frand()*34.0f,0.3f+frand()*0.4f,frand()*6.2832f,0.4f+frand()*0.9f,(uint8_t)(140+frand()*80)}; csInit=true; }
  clearFrame();
  float fl=0.7f+0.3f*sinf(tf*6.0f);
  fxSplat(0,0,3.0f+fl,2.4f, CRGB(255,240,200));         // central star
  for(int i=0;i<CS_N;i++){
    float M=tf*csP[i].spd+csP[i].phase, rot=i*0.9f;
    float ex=cosf(M)*csP[i].a, ey=sinf(M)*csP[i].a*(1.0f-csP[i].ecc);
    float x=ex*cosf(rot)-ey*sinf(rot), y=ex*sinf(rot)+ey*cosf(rot);
    float d=sqrtf(x*x+y*y)+0.01f, tx=x/d, ty=y/d;
    float tl=10.0f+(1.0f-d/csP[i].a)*20.0f;             // longer tail near the star
    fxStreak(x,y,x+tx*tl,y+ty*tl,0.5f,1.4f, CRGB(CHSV(csP[i].hue,180,140)));
    fxSplat(x,y,1.2f,1.2f, CRGB(CHSV(csP[i].hue,120,230)));
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 101 — ROSE WINDOW
// A rotating stained-glass rose: radiating spokes and concentric rings of petals,
// adjacent rings counter-turning, the whole thing breathing and cycling colour.
// A cathedral finale.
// ═══════════════════════════════════════════════════════════════════════════
void anim_roseWindow(uint32_t at,float dt){
  (void)dt; float tf=at*0.001f;
  clearFrame();
  const int seg=12; float rot=tf*0.15f, breath=0.5f+0.5f*sinf(tf*0.5f);
  for(int s=0;s<seg;s++){ float a=rot+s*(6.2832f/seg);   // radiating spokes
    fxStreak(0,0,cosf(a)*56.0f,sinf(a)*56.0f,0.4f,1.0f, CRGB(CHSV((uint8_t)(s*21),200,80))); }
  for(int r=1;r<=4;r++){
    float rr=12.0f*r+breath*6.0f;
    for(int s=0;s<seg;s++){
      float a=rot*((r%2)?1.0f:-1.0f)+s*(6.2832f/seg)+r*0.26f;
      uint8_t hue=(uint8_t)(s*21+r*30+tf*20);
      fxSplat(cosf(a)*rr,sinf(a)*rr,1.6f,1.6f, CRGB(CHSV(hue,220,180)));
    }
    fxRing(0,0,rr,0.4f,1.0f, CRGB(CHSV((uint8_t)(r*40+tf*20),160,70)));
  }
  fxSplat(0,0,3.0f,2.0f, CRGB(255,240,255));
  showGrid();
}
