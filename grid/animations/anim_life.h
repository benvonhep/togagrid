// Simulation / generative animations
// (Auto-split from grid_SW.ino; #included into the main sketch — all shared
//  globals/functions are visible from grid_SW.ino. Do not compile standalone.)
#pragma once

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
  showGrid();
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
  showGrid();
}
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
  showGrid();
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
  showGrid();
}
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
  showGrid();
}
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
  showGrid();
}

void anim_continuousCA(uint32_t t){ anim_neuralFire(t); }
static float plEnergy[PL_N][PL_N];  // energy at each intersection
static float plGlow[PL_N][PL_N];    // afterglow
static bool  plInit=false;
static uint32_t plLastStep=0;
static float plChargeRate=0.003f;
static float plChargeRateT=0.003f;
struct PlBolt {
  bool  active;
  int   pathX[PL_BOLT_MAX];
  int   pathY[PL_BOLT_MAX];
  int   pathLen;
  uint8_t ttl;     // frames to live
  uint8_t maxTTL;
  uint8_t hue;
};
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
  showGrid();
}

void anim_agentField(uint32_t t,float dt){ anim_plasmaLightning(t,dt); }
