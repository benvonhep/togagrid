// Energy / plasma showpiece pack #2 — five brand-new, self-contained modes in the
// spirit of anim_plasmaLightning (grid mode 67), but each a DISTINCT vibe.
// (Auto-split style: #included into the main sketch AFTER the globals, exactly like
//  the other animations/anim_*.h files. Do not compile standalone.)
//
// Signature convention matches anim_plasmaLightning / anim_strangeAttractor:
//   void anim_<name>(uint32_t at, float dt)
//     at = speed-scaled time in ms (drives motion → Speed knob works)
//     dt = per-frame delta in ms (drives integration → Speed knob scales physics)
// Every function ends with showGrid() so the shared hue-rotation / AGC pipeline runs.
#pragma once

// Local deterministic hash (own name so we never clash with anim_new.h's fxhash,
// regardless of include order). Returns 0..1.
static inline float n2hash(float n){ float s = sinf(n)*43758.5453f; return s - floorf(s); }

// ═══════════════════════════════════════════════════════════════════════════
// 1 — AURORA CURTAINS
// Flowing vertical sheets of light that shear and ripple sideways like a real
// aurora, with an upward-streaming shimmer and colour bands (green→teal→magenta)
// that slowly wander across the wheel. Nothing like the blue bolts of plasma —
// this is soft, drifting, continuous. A handful of stacked "curtain" layers
// (own random phase/speed/colour seeds) are summed per pixel into thin ridges.
// ═══════════════════════════════════════════════════════════════════════════
#define AU_CURTAINS 3
static bool  auInit=false;
static float auXoff[AU_CURTAINS];   // horizontal seed position of each curtain
static float auSpd[AU_CURTAINS];    // sideways drift speed
static float auHueOff[AU_CURTAINS]; // per-curtain colour offset
static float auHueDrift=0.0f;       // slow global palette wander

void anim_auroraCurtains(uint32_t at, float dt){
  if(!auInit){
    for(int i=0;i<AU_CURTAINS;i++){
      auXoff[i]   = frand()*120.0f - 60.0f;
      auSpd[i]    = (frand()<0.5f?-1.0f:1.0f)*(3.0f + frand()*6.0f);
      auHueOff[i] = frand()*60.0f;
    }
    auHueDrift = frand()*255.0f;
    auInit=true;
  }
  float tf=at*0.001f;
  auHueDrift += dt*0.003f;                       // slow, continuous palette drift

  clearFrame();
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER;
      float x=pass?other:base, y=pass?base:other;
      // wavy vertical shear — this is what makes the curtains "wave" as they rise
      float xs = x + sinf(y*0.055f + tf*0.5f)*7.0f;
      // stack a few sharpened sine ridges into thin, bright curtains
      float v=0.0f, hueMix=0.0f;
      for(int i=0;i<AU_CURTAINS;i++){
        float band = sinf((xs - auXoff[i] - tf*auSpd[i])*0.15f);
        float b = band*band; b*=b;               // square twice → thin ridge
        v      += b*0.5f;
        hueMix += b*auHueOff[i];
      }
      // upward-streaming shimmer + soft fade toward top/bottom edges
      float shim = 0.55f + 0.45f*sinf(y*0.22f - tf*3.0f + xs*0.08f);
      float vy   = constrain(1.0f - fabsf(y-8.0f)/74.0f, 0.0f, 1.0f);
      v *= shim*vy;
      if(v>0.02f){
        uint8_t hue = (uint8_t)(auHueDrift + 96.0f + y*1.1f + hueMix);
        fxAdd(pass?false:true, s+1, n, CRGB(CHSV(hue,220,255)), v>1.0f?1.0f:v);
      }
    } }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 2 — NEON SHOCKWAVES
// Crisp neon rings born from random emitters all over the panel, expanding and
// fading, colliding and adding light where they overlap. Each ring is a coloured
// body with a whiter, thinner leading edge, plus a bright spark at its birthplace.
// Sharp, punchy, geometric — the opposite of the soft aurora.
// ═══════════════════════════════════════════════════════════════════════════
#define SW_MAX 7
struct SwWave { float cx,cy,r,speed,maxR; uint8_t hue; bool live; };
static SwWave swW[SW_MAX];
static bool   swInit=false;
static float  swSpawnAcc=0.0f;

void anim_neonShockwaves(uint32_t at, float dt){
  if(!swInit){ for(int i=0;i<SW_MAX;i++) swW[i].live=false; swSpawnAcc=0.0f; swInit=true; }
  float dts=dt*0.001f; if(dts<0)dts=0; if(dts>0.1f)dts=0.1f;   // guard mode-entry jumps
  float tf=at*0.001f;

  // spawn a fresh ring on a jittered cadence
  swSpawnAcc -= dts;
  if(swSpawnAcc<=0.0f){
    for(int i=0;i<SW_MAX;i++) if(!swW[i].live){
      swW[i].live  = true;
      swW[i].cx    = (frand()-0.5f)*70.0f;
      swW[i].cy    = (frand()-0.5f)*70.0f;
      swW[i].r     = 1.0f;
      swW[i].speed = 34.0f + frand()*26.0f;
      swW[i].maxR  = 60.0f + frand()*24.0f;
      swW[i].hue   = (uint8_t)(tf*40.0f + frand()*80.0f);
      break;
    }
    swSpawnAcc = 0.32f + frand()*0.35f;
  }
  // advance & retire
  for(int i=0;i<SW_MAX;i++) if(swW[i].live){
    swW[i].r += swW[i].speed*dts;
    if(swW[i].r > swW[i].maxR) swW[i].live=false;
  }

  clearFrame();
  for(int i=0;i<SW_MAX;i++) if(swW[i].live){
    float age  = swW[i].r/swW[i].maxR;           // 0..1
    float fade = 1.0f-age; fade*=fade;
    uint8_t bri=(uint8_t)(fade*255.0f);
    if(bri<4) continue;
    // coloured shock body
    fxRing(swW[i].cx,swW[i].cy,swW[i].r,1.6f,1.6f,CRGB(CHSV(swW[i].hue,230,bri)));
    // whiter, thinner leading edge just ahead of the body
    fxRing(swW[i].cx,swW[i].cy,swW[i].r+1.2f,0.5f,1.2f,CRGB(CHSV(swW[i].hue,90,(uint8_t)(fade*200.0f))));
    // bright spark at the emitter for the first moment of life
    if(age<0.25f){
      float k=(0.25f-age)*4.0f;                  // 1..0
      fxSplat(swW[i].cx,swW[i].cy,1.0f+k*7.0f,1.4f,CRGB(CHSV(swW[i].hue,120,(uint8_t)(k*220.0f))));
    }
  }
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 3 — EMBER VORTEX
// A swirling fire tornado: warm turbulent haze for depth (cheap integer noise)
// with dozens of glowing embers spiralling inward, spinning FASTER as they near
// the white-hot flickering core (angular speed ∝ 1/radius). Embers whiten toward
// yellow as they heat up, then die and respawn at the rim. Hot, organic, rotational.
// ═══════════════════════════════════════════════════════════════════════════
#define EV_N 46
struct Ember { float ang,rad,radV,angV,life,maxLife,size; uint8_t hue; };
static Ember evP[EV_N];
static bool  evInit=false;

static void evSpawn(int i){
  evP[i].ang     = frand()*6.2832f;
  evP[i].rad     = 50.0f + frand()*18.0f;        // born at the rim
  evP[i].radV    = -(8.0f + frand()*14.0f);      // fall inward
  evP[i].angV    = 1.2f + frand()*1.6f;          // base swirl rate
  evP[i].maxLife = 2.0f + frand()*2.5f;
  evP[i].life    = evP[i].maxLife;
  evP[i].size    = 0.6f + frand()*1.1f;
  evP[i].hue     = (uint8_t)(frand()*38.0f);     // deep red → orange → yellow
}

void anim_emberVortex(uint32_t at, float dt){
  if(!evInit){ for(int i=0;i<EV_N;i++) evSpawn(i); evInit=true; }
  float dts=dt*0.001f; if(dts<0)dts=0; if(dts>0.12f)dts=0.12f;
  float tf=at*0.001f;

  clearFrame();
  // warm turbulent depth haze (integer noise → cheap), slowly churning
  fxNebula(at, 8, 255, 150.0f, 26.0f, 58.0f, 0.9f);

  for(int i=0;i<EV_N;i++){
    // vortex law: whirl accelerates toward the centre
    float w = evP[i].angV*(1.0f + 40.0f/(evP[i].rad+6.0f));
    evP[i].ang += w*dts;
    evP[i].rad += evP[i].radV*dts;
    evP[i].life -= dts;
    if(evP[i].life<=0.0f || evP[i].rad<3.0f){ evSpawn(i); continue; }
    float lf    = evP[i].life/evP[i].maxLife;                    // 1 fresh .. 0 spent
    float heat  = constrain(1.0f - evP[i].rad/56.0f, 0.0f, 1.0f);// hotter near core
    float bri   = constrain(0.25f + heat*0.9f, 0.0f, 1.0f)*(0.4f + 0.6f*lf);
    float ex    = cosf(evP[i].ang)*evP[i].rad;
    float ey    = sinf(evP[i].ang)*evP[i].rad;
    uint8_t hue = (uint8_t)(evP[i].hue + heat*18.0f);            // whiten/yellow when hot
    fxSplat(ex, ey, evP[i].size*(0.7f + heat*0.9f), 1.3f, hotColor(bri,hue,26,40));
  }

  // white-hot flickering core
  float flick = 0.7f + 0.3f*sinf(tf*11.0f);
  fxSplat(0,0, 2.6f + flick*1.4f, 2.2f, CRGB(255,240,210));
  fxSplat(0,0, 5.5f, 3.0f, CRGB(CHSV(20,220,(uint8_t)(120*flick))));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 4 — PRISM SHARDS
// Rainbow crystal blades that EXPLODE outward and REASSEMBLE into a tight spinning
// star, on a slow breathing cycle. When flung out they lengthen and desaturate to
// pure hue; when they snap back together they whiten and the core flashes bright.
// Sharp, faceted, kaleidoscopic — a crystalline counterpoint to the fire vortex.
// ═══════════════════════════════════════════════════════════════════════════
#define PS_N 16
struct Shard { float ang,len,spin,jitter; uint8_t hue; };
static Shard psS[PS_N];
static bool  psInit=false;

void anim_prismShards(uint32_t at, float dt){
  (void)dt;
  if(!psInit){
    for(int i=0;i<PS_N;i++){
      psS[i].ang    = (float)i/PS_N*6.2832f;
      psS[i].len    = 10.0f + frand()*8.0f;
      psS[i].spin   = (frand()-0.5f)*2.0f;
      psS[i].jitter = 0.6f + frand()*0.8f;
      psS[i].hue    = (uint8_t)((float)i/PS_N*255.0f);
    }
    psInit=true;
  }
  float tf=at*0.001f;
  float p     = 0.5f - 0.5f*cosf(tf*0.9f);       // 0 assembled .. 1 fully exploded
  float twist = tf*0.6f;

  clearFrame();
  for(int i=0;i<PS_N;i++){
    float a     = psS[i].ang + twist + psS[i].spin*p*2.4f;
    float inner = 3.0f + p*(46.0f*psS[i].jitter);
    float outer = inner + psS[i].len*(0.6f + p*1.8f);
    float ca=cosf(a), sa=sinf(a);
    uint8_t hue = (uint8_t)(psS[i].hue + tf*30.0f);
    float assemble = 1.0f - p;                    // 1 when gathered at the core
    float bri = 0.45f + 0.55f*p;
    uint8_t sat = (uint8_t)(255.0f - assemble*assemble*160.0f);   // whiten on reassembly
    fxStreak(ca*inner,sa*inner, ca*outer,sa*outer, 0.6f,1.2f,
             CRGB(CHSV(hue,sat,(uint8_t)(bri*255.0f))));
    // glinting tip
    fxSplat(ca*outer,sa*outer, 0.7f,1.1f, CRGB(CHSV(hue,200,(uint8_t)(bri*230.0f))));
  }
  // central prism flashes when the shards slam back together
  float core=(1.0f-p); core*=core; core*=core;
  fxSplat(0,0, 2.0f + core*3.0f, 1.8f, CRGB(255,255,255));
  showGrid();
}

// ═══════════════════════════════════════════════════════════════════════════
// 5 — NEBULA BREATH
// A deep-space cloud that inhales and exhales: three layered noise clouds
// (independent slow hue drift) whose radii breathe in and out, wavering tendrils
// reaching from the glowing heart, and a field of twinkling stars over the top.
// Vast, slow, and dreamy — the calm cosmic member of the pack.
// ═══════════════════════════════════════════════════════════════════════════
static bool  nbInit=false;
static float nbHue1,nbHue2,nbHue3;   // per-layer drifting hues
static float nbStarSeed;

void anim_nebulaBreath(uint32_t at, float dt){
  if(!nbInit){ nbHue1=160.0f; nbHue2=210.0f; nbHue3=120.0f; nbStarSeed=frand()*100.0f; nbInit=true; }
  float tf=at*0.001f;
  nbHue1 += dt*0.0016f; nbHue2 += dt*0.0011f; nbHue3 += dt*0.0022f;   // slow palette wander
  float breath = 0.5f + 0.5f*sinf(tf*0.5f);      // 0..1 breathing envelope

  clearFrame();
  // two drifting outer clouds + a brighter, tighter inner core cloud
  fxNebula(at,        (uint8_t)nbHue1, 210, 120.0f, 10.0f, 44.0f + breath*22.0f, 0.85f);
  fxNebula(at+9000,   (uint8_t)nbHue2, 235, 220.0f, 14.0f, 30.0f + breath*20.0f, 0.75f);
  fxNebula(at+3000,   (uint8_t)nbHue3, 200, 320.0f,  8.0f, 16.0f + breath*10.0f, 0.90f);

  // wavering tendrils reaching out from the heart
  for(int k=0;k<5;k++){
    float a   = k*1.2566f + tf*0.2f + sinf(tf*0.7f + k)*0.4f;
    float len = 20.0f + 18.0f*breath + 6.0f*sinf(tf*1.3f + k*2.0f);
    fxStreak(0,0, cosf(a)*len, sinf(a)*len, 1.3f, 2.0f,
             CRGB(CHSV((uint8_t)(nbHue3+40.0f),180,60)));
  }
  // twinkling stars scattered across the field
  for(int i=0;i<34;i++){
    float sx = (n2hash(i*1.7f + nbStarSeed)-0.5f)*120.0f;
    float sy = (n2hash(i*3.3f + nbStarSeed)-0.5f)*120.0f;
    float tw = 0.4f + 0.6f*fabsf(sinf(tf*2.2f + i*1.3f));
    fxSplat(sx,sy, 0.35f + tw*0.3f, 1.0f, CRGB(CHSV(200,60,(uint8_t)(tw*200.0f))));
  }
  // gently pulsing glowing heart
  float pulse = 0.6f + 0.4f*breath;
  fxSplat(0,0, 2.0f + pulse*2.0f, 2.4f, CRGB(CHSV((uint8_t)(nbHue2+20.0f),150,(uint8_t)(150*pulse))));
  showGrid();
}
