// Calibration, text banners, runner, faces
// (Auto-split from grid_SW.ino; #included into the main sketch — all shared
//  globals/functions are visible from grid_SW.ino. Do not compile standalone.)
#pragma once

// ── Mode names (index matches the dispatch switch below) ──
const char* const ANIM_NAMES[]={
  "SETUP","HEARTBEAT","BINARY RAIN","TECTONIC","VORTEX","EARTHQUAKE","DNA HELIX",
  "SUPERNOVA","BLACK HOLE","WORMHOLE","MITOSIS","COLLIDER","SHOCKWAVE","PULSAR",
  "LIQUID METAL","ICE CROWN","CORIOLIS","CRYSTAL","AURORA","LAVA LAMP","NERVE",
  "GRAVITY LENS","TIDAL WAVE","CYMATICS","STARFIELD","DARK MATTER","SQUARES",
  "SQUARE RAIN","DIAMOND","CHECKER","CUBE","SUNSET","DEPTH","COSMIC DUST",
  "HEARTGLOW","EVOLUTION","MORPH","ATTRACTOR","HARMONIC","AGENT FIELD","TETRIS",
  "SNAKE","ICONS","LOGO","STAR TUNNEL","RIPPLE","GALAXY","PLASMA","BREATHE",
  "BOIDS","FIREWORKS","REACTION","TOGA","CASTLE","2026","STICKMAN","BREAKOUT","SMILEY","EYE",
  "STARFIELD2","WARP","ASTEROIDS","SHOOTING","BLACKHOLE2","PULSAR2","PWNEBULA","ORION",
  "HORSEHEAD","TWINSTARS","PLANETS","EARTH","PANEL","RADAR","SINEWAVE","ROCKET","WARNLIGHT",
  "ALIENS","INVADERS","RAIN2","LEVITATE","CONFETTI","SPIRALEJ","CLOUDS3D","HEART2",
  "RAINUP","STARSFLY","FIREWORK2","HSTRIPES","VUBARS","VUGRID","ARMS"
};

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
  FastLED.show();                              // calibration must not be recoloured
}

// ═══════════ 52-54. SCROLLING TEXT BANNERS ═══════════
void anim_scrollBanner(const char* s,uint32_t at,uint8_t baseHue){
  clearFrame();
  int w=textLenNodes(s)-1;   // glyph width in node-columns
  CRGB col=CRGB(CHSV((uint8_t)(baseHue+at/60),255,220));
  if(w<=11){
    drawTextCells(s,(11-w)/2,TEXT_TOP,col);          // fits → static, centred
  } else {
    int span=w+12+3, pos=(int)((at/700)%span);        // too wide → slow scroll
    drawTextCells(s,12-pos,TEXT_TOP,col);
  }
  showGrid();
}
// TOGA banner + occasional LIGHTNING: a burst of 1..5 very short flashes (with
// short variable gaps between them) lighting a RANDOM single letter of "TOGA"
// electric blue-white, then a longer random pause before the next burst.
void anim_textTOGA(uint32_t at){
  clearFrame();
  CRGB col=CRGB(CHSV((uint8_t)(40+at/60),255,220));
  drawTextCells("TOGA",0,TEXT_TOP,col);        // base render ("TOGA" is 11 nodes → static, leftC 0)

  static const char* TW="TOGA";
  static uint32_t nextBurst=0, evt=0;          // evt = time of next flash/gap transition
  static int flashesLeft=0;                    // flashes still to fire in this burst
  static int litLetter=-1;                     // 0..3 = letter currently flashing, -1 = none
  static bool inFlash=false;                   // true while a letter is lit
  if(nextBurst==0) nextBurst=at+700;           // first strike shortly after start

  if(inFlash || flashesLeft>0){
    if(at>=evt){
      if(inFlash){                             // a flash just ended
        inFlash=false; litLetter=-1;
        if(flashesLeft>0) evt=at+25+random(120);         // short variable gap inside the burst
        else nextBurst=at+1500+random(4000);             // long random pause before next burst
      } else {                                 // gap ended → fire the next flash
        inFlash=true; litLetter=random(4);
        evt=at+20+random(55); flashesLeft--;             // very short flash
      }
    }
  } else if(at>=nextBurst){                     // start a fresh burst
    flashesLeft=1+random(5);                    // 1..5 flashes
    inFlash=true; litLetter=random(4);
    evt=at+20+random(55); flashesLeft--;
  }

  if(inFlash && litLetter>=0)                   // over-draw the struck letter, near-white electric blue
    drawGlyphW(FONTW[glyphIndex(TW[litLetter])],TEXT_TOP,litLetter*3,CRGB(205,230,255));
  showGrid();
}

// ═══════════ 55. RUNNER (Strichmännchen jump'n'run, Mario-style) ═══════════
bool stickInit=false;
static float rnObsX[RUN_OBS]; static uint8_t rnObsType[RUN_OBS]; static bool rnObsOn[RUN_OBS];
static float rnJumpU; static bool rnJumping; static uint32_t rnSpawn;   // rnJumpU = hop progress 0..1
// Obstacle shapes sitting on the ground (g = top ground cell row = GND-1).
void rnDrawObstacle(int t,int ox,int GND){
  int g=GND-1;
  if(t==0){                                   // rock / bush (low, wide)
    CRGB rk=CRGB(CHSV(0,0,110));
    fillCell(g,ox,rk); fillCell(g,ox+1,rk);
  } else if(t==1){                            // box
    CRGB bx=CRGB(CHSV(18,210,190));
    fillCell(g,ox,bx); fillCell(g-1,ox,bx);
  } else if(t==2){                            // cactus
    CRGB ca=CRGB(CHSV(80,230,160));
    fillCell(g,ox,ca); fillCell(g-1,ox,ca); fillCell(g-2,ox,ca); fillCell(g-1,ox+1,ca);
  } else {                                    // tree (trunk + canopy)
    CRGB tr=CRGB(CHSV(22,220,130)), lf=CRGB(CHSV(90,230,150));
    fillCell(g,ox,tr);
    fillCell(g-1,ox-1,lf); fillCell(g-1,ox,lf); fillCell(g-1,ox+1,lf);
    fillCell(g-2,ox,lf);
  }
}
void anim_stickman(uint32_t at,float dt){
  const int RX=2, GND=9;                       // runner column / ground node row
  if(!stickInit){ for(int i=0;i<RUN_OBS;i++) rnObsOn[i]=false; rnJumpU=1.0f;rnJumping=false; rnSpawn=at; stickInit=true; }
  // world scrolls: obstacles move left toward the runner
  for(int i=0;i<RUN_OBS;i++) if(rnObsOn[i]){ rnObsX[i]-=0.006f*dt; if(rnObsX[i]<-2.0f) rnObsOn[i]=false; }
  if(at-rnSpawn>1500){ rnSpawn=at;
    for(int i=0;i<RUN_OBS;i++) if(!rnObsOn[i]){ rnObsX[i]=12.0f; rnObsType[i]=random(4); rnObsOn[i]=true; break; } }
  // ── parabolic HOP that clears the obstacle and lands centred in the GAP ──
  //    Obstacles move 0.006 cells/ms. Trigger distance ≈ speed*JUMP_DUR/2 so the
  //    apex coincides with the obstacle passing the runner, and the taller/longer
  //    arc keeps a clear margin from the obstacle: at touch-down the obstacle is
  //    already >2 cells to the left (well inside the free space between two).
  const float JUMP_DUR=700.0f, JUMP_PEAK=5.2f;                     // ms / cells cleared
  if(!rnJumping) for(int i=0;i<RUN_OBS;i++) if(rnObsOn[i]){ float d=rnObsX[i]-RX; if(d>1.6f&&d<2.3f){ rnJumping=true; rnJumpU=0.0f; break; } }
  if(rnJumping){ rnJumpU+=dt/JUMP_DUR; if(rnJumpU>=1.0f){ rnJumpU=1.0f; rnJumping=false; } }
  float jh=JUMP_PEAK*4.0f*rnJumpU*(1.0f-rnJumpU);                  // 0 at ends, JUMP_PEAK at apex
  int F=GND-(int)(jh+0.5f);                     // feet node (whole figure rises with the hop)
  bool f=((at/140)&1);                          // running leg frame
  CRGB col =CRGB(CHSV(35,180,235));             // runner
  CRGB gcol=CRGB(CHSV(95,200,110));             // ground
  clearFrame();
  for(int c=0;c<11;c++) drawHSeg(GND,c,gcol);   // ground line
  for(int i=0;i<RUN_OBS;i++) if(rnObsOn[i]) rnDrawObstacle(rnObsType[i],(int)(rnObsX[i]+0.5f),GND);
  // ── runner ──
  int C=RX;
  fillCell(F-5,C-1,col); fillCell(F-5,C,col);   // head (2-cell box)
  drawVSeg(F-4,C,col); drawVSeg(F-3,C,col);      // torso (no arms)
  if(rnJumping){ drawVSeg(F-2,C-1,col); drawVSeg(F-2,C+1,col); }    // legs spread (airborne)
  else if(f){ drawVSeg(F-2,C-1,col); drawVSeg(F-1,C-1,col); drawVSeg(F-2,C+1,col); } // stride A
  else      { drawVSeg(F-2,C+1,col); drawVSeg(F-1,C+1,col); drawVSeg(F-2,C-1,col); } // stride B
  showGrid();
}

// ═══════════ 56. PADDLE & BALL (keep the ball up / Breakout) ═══════════
bool padInit=false;
static float pbBx,pbBy,pbVx,pbVy,pbPx;
static bool pbBrick[3][11];
void anim_paddleBall(uint32_t at,float dt){
  if(!padInit){
    for(int r=0;r<3;r++) for(int c=0;c<11;c++) pbBrick[r][c]=true;
    pbBx=5; pbBy=6; pbVx=0.006f; pbVy=-0.006f; pbPx=5; padInit=true;
  }
  pbBx+=pbVx*dt; pbBy+=pbVy*dt;
  if(pbBx<0){ pbBx=0; pbVx=-pbVx; }
  if(pbBx>10){ pbBx=10; pbVx=-pbVx; }
  if(pbBy<0){ pbBy=0; pbVy=-pbVy; }
  // paddle auto-tracks the ball
  if(pbPx<pbBx-0.1f) pbPx+=0.006f*dt; else if(pbPx>pbBx+0.1f) pbPx-=0.006f*dt;
  pbPx=constrain(pbPx,1.0f,9.0f);
  int bc=(int)(pbBx+0.5f), br=(int)(pbBy+0.5f), pc=(int)(pbPx+0.5f);
  if(br>=0&&br<3&&bc>=0&&bc<11&&pbBrick[br][bc]){ pbBrick[br][bc]=false; pbVy=-pbVy; } // break brick
  if(pbBy>=9.0f){                                  // paddle line
    if(bc>=pc-1&&bc<=pc+1){ pbBy=9.0f; pbVy=-fabsf(pbVy); }   // bounce off paddle
    else if(pbBy>11.0f){ pbBx=5; pbBy=6; pbVx=(random(2)?0.006f:-0.006f); pbVy=-0.006f; } // missed → respawn
  }
  bool any=false; for(int r=0;r<3;r++) for(int c=0;c<11;c++) if(pbBrick[r][c]) any=true;
  if(!any) for(int r=0;r<3;r++) for(int c=0;c<11;c++) pbBrick[r][c]=true;   // refill
  clearFrame();
  for(int r=0;r<3;r++) for(int c=0;c<11;c++) if(pbBrick[r][c]) fillCell(r,c,CRGB(CHSV((uint8_t)(c*18+r*50),220,150)));
  fillCell(10,pc-1,CRGB(CHSV(140,220,200))); fillCell(10,pc,CRGB(CHSV(140,220,200))); fillCell(10,pc+1,CRGB(CHSV(140,220,200)));
  fillCell(br,bc,CRGB(255,255,255));
  showGrid();
}
// Each eye is a 2x2 cell block with top-left cell (r,c). It can render four
// shapes so a cross / horizontal / vertical eye line is possible (Task 3).
enum { EYE_SOLID, EYE_HLINE, EYE_VLINE, EYE_CROSS };
void drawEye(int r,int c,int shape,CRGB col){
  switch(shape){
    case EYE_SOLID: fillCell(r,c,col);   fillCell(r,c+1,col);
                    fillCell(r+1,c,col); fillCell(r+1,c+1,col); break;
    case EYE_HLINE: drawHSeg(r+1,c,col); drawHSeg(r+1,c+1,col); break;      // bar across block centre
    case EYE_VLINE: drawVSeg(r,c+1,col); drawVSeg(r+1,c+1,col); break;      // bar down block centre
    case EYE_CROSS: drawHSeg(r+1,c,col); drawHSeg(r+1,c+1,col);
                    drawVSeg(r,c+1,col); drawVSeg(r+1,c+1,col); break;      // "+"
  }
}
// Smooth, anti-aliased friendly face: a warm-yellow ring, two eyes that glide
// and blink, and a gently-breathing smile — all drawn in world coords with the
// soft fx* helpers (no boxy cells). Restrained warm palette (yellow/amber/coral).
// (world +y is "down" on the panel, matching the segment-node orientation.)
void anim_smiley(uint32_t at){
  static bool smI=false;
  static float gx=0,gy=0,tx=0,ty=0;              // gaze offset: current / target
  static uint32_t moveT=0,blinkT=0,lastT=0;
  static bool blinking=false;
  if(!smI){ smI=true; moveT=blinkT=lastT=at; }
  float dt=(float)(at-lastT); lastT=at; if(dt>60.0f) dt=60.0f;   // clamp scheduling gaps

  // ── gaze: pick a new target now and then, glide smoothly toward it ──
  if(at-moveT>1700){ moveT=at; tx=(frand()*2.0f-1.0f)*7.0f; ty=(frand()*2.0f-1.0f)*4.5f; }
  float k=dt*0.0045f; if(k>1.0f) k=1.0f;
  gx+=(tx-gx)*k; gy+=(ty-gy)*k;

  // ── occasional blink (smooth close→open over ~300ms) ──
  if(!blinking && at-blinkT>3200){ blinking=true; blinkT=at; }
  float blink=0.0f;                              // 0 = open, 1 = shut
  if(blinking){ uint32_t e=at-blinkT; if(e>=300) blinking=false;
                else blink=1.0f-fabsf((float)e-150.0f)/150.0f; }
  float eyeOpen=1.0f-blink;

  clearFrame();
  CRGB faceCol = CRGB(CHSV(40,235,200));         // warm friendly yellow
  CRGB eyeCol  = CRGB(255,238,205);              // soft warm white (the moving pupil)
  CRGB socket  = CRGB(CHSV(38,240,70));          // faint amber eye-socket glow
  CRGB cheek   = CRGB(CHSV(20,235,55));          // subtle warm blush accent
  CRGB mthCol  = CRGB(CHSV(0,240,215));           // bright RED mouth — clearly distinct from the amber face

  // face outline — smooth glowing ring
  fxRing(0.0f,0.0f,42.0f,2.4f,3.2f,faceCol);

  // subtle cheeks
  fxSplat(-27.0f, 8.0f, 3.0f,4.0f,cheek);
  fxSplat( 27.0f, 8.0f, 3.0f,4.0f,cheek);

  // eyes: faint socket glow + a bright pupil that glides with the gaze; blink
  // squashes the pupil and draws a warm lid line across it.
  const float EX=16.0f, EY=-15.0f;
  for(int s=-1;s<=1;s+=2){ float ex=s*EX;
    fxSplat(ex, EY, 5.5f,3.0f,socket);                              // socket
    CRGB p=eyeCol; p.nscale8_video((uint8_t)(eyeOpen*eyeOpen*255.0f));
    fxSplat(ex+gx, EY+gy*0.7f, 3.1f,2.2f,p);                        // pupil (fades as it closes)
    if(blink>0.05f){ CRGB lid=faceCol; lid.nscale8_video((uint8_t)(blink*255.0f));
      fxStreak(ex-5.5f,EY, ex+5.5f,EY, 1.3f,2.0f,lid); }            // closing lid
  }

  // mouth: a smooth smile whose depth breathes gently (never a flat/sad line)
  float depth=11.0f+3.0f*sinf(at*0.0011f);       // gentle expression change
  const float MW=22.0f, MY=7.0f;
  int ms=20;
  for(int i=0;i<=ms;i++){ float u=(float)i/ms*2.0f-1.0f;           // -1..1 across the smile
    fxSplat(u*MW, MY+depth*(1.0f-u*u), 2.0f,2.2f,mthCol); }        // ∪ shape (middle lowest)
  showGrid();
}

// ═══════════ 58. EYE (looks around & blinks) ═══════════
// Rendered per-LED as a smooth field (soft, anti-aliased edges — like the
// older radial animations) rather than hard strip segments. Horizontal
// almond sclera + moving iris/pupil; upper lid sweeps down to blink.
bool eyeInit=false;
static float eyeGx,eyeGy,eyeTx,eyeTy; static uint32_t eyeMoveT,eyeBlinkT; static bool eyeBlk;
// Eye colour at world point (x,y). Returns false if this LED stays dark.
bool eyePixel(float x,float y,float gx,float gy,float bp,CRGB &out){
  const float A=56.0f,B=27.0f,RI=13.5f,RP=6.5f,SOFT=5.0f;       // larger almond, bigger pupil
  float ax=x/A, qx=1.0f-ax*ax; if(qx<=-0.12f) return false;      // wider margin → room for a THICKER outline
  float sq=sqrtf(qx>0.0f?qx:0.0f);
  float e=ax*ax+(y/B)*(y/B);
  float outF=constrain(1.0f-fabsf(e-1.0f)/0.26f,0.0f,1.0f);      // MUCH thicker almond outline
  float openTop=(2.0f*bp-1.0f)*B*sq, openBot=B*sq;               // upper lid sweeps down to close
  float vis=constrain((y-openTop)/SOFT,0.0f,1.0f)*constrain((openBot-y)/SOFT,0.0f,1.0f);
  float scl=constrain((0.84f-e)/0.14f,0.0f,1.0f)*vis;           // sclera fill sits INSIDE the thick rim
  // ── 3D sphere shading: bright domed centre, dim rim + fixed upper-left catch-light ──
  float dome=0.18f+0.82f*sqrtf(constrain(1.0f-e,0.0f,1.0f));    // stronger centre→rim falloff = rounder ball
  float hx=x+14.0f, hy=y+12.0f;                                 // specular near the upper-left
  float spec=constrain(1.0f-sqrtf(hx*hx+hy*hy)/13.0f,0.0f,1.0f); spec*=spec;
  float dg=sqrtf((x-gx)*(x-gx)+(y-gy)*(y-gy));
  float irisF=constrain((RI-dg)/2.0f,0.0f,1.0f);               // crisp iris edge → more contrast
  float pupF =constrain((RP-dg)/2.0f,0.0f,1.0f);
  float wF=fmaxf(outF, scl);
  if(wF<0.02f && vis<0.02f) return false;
  // ── sclera WHITE: brightest at the domed CENTRE (dome≈1), dimming toward the
  //    contour; an extra ambient-occlusion darkening where it meets the rim gives
  //    the eyeball a rounded, sunken-into-the-lid 3D look ──
  float ao=constrain((e-0.55f)/0.30f,0.0f,1.0f);               // 0 at centre → 1 near the outline
  float scv=constrain(scl*dome*(1.0f-0.55f*ao),0.0f,1.0f);
  CRGB c((uint8_t)(scv*190.0f),(uint8_t)(scv*187.0f),(uint8_t)(scv*165.0f));   // warm white
  c.r=qadd8(c.r,(uint8_t)(scl*spec*85.0f)); c.g=qadd8(c.g,(uint8_t)(scl*spec*85.0f)); c.b=qadd8(c.b,(uint8_t)(scl*spec*65.0f));
  // ── CONTOUR: bold luminous VIOLET rim — a clearly different hue from the warm
  //    white AND the teal iris, thick and the brightest element so it frames ──
  float of=constrain(outF,0.0f,1.0f);
  CRGB rim(150,40,240);
  c.r=(uint8_t)(c.r*(1.0f-of)+rim.r*of); c.g=(uint8_t)(c.g*(1.0f-of)+rim.g*of); c.b=(uint8_t)(c.b*(1.0f-of)+rim.b*of);
  // Iris REPLACES the sclera (lerp, not additive) so its saturated colour keeps
  // full contrast against the white instead of washing out; pupil darkens centre.
  if(irisF>0.001f && vis>0.001f){
    float ir=constrain(dg/RI,0.0f,1.0f);
    float ishade=0.50f+0.50f*(1.0f-ir*ir);                      // domed iris: bright centre, shaded edge (3D)
    CRGB iris=CRGB(15,225,185); iris.nscale8_video((uint8_t)(ishade*255.0f));  // bright emerald-teal (>white)
    float mix=irisF*vis;
    c.r=(uint8_t)(c.r*(1.0f-mix)+iris.r*mix);
    c.g=(uint8_t)(c.g*(1.0f-mix)+iris.g*mix);
    c.b=(uint8_t)(c.b*(1.0f-mix)+iris.b*mix);
    float limbal=constrain(1.0f-fabsf(dg-RI)/2.2f,0.0f,1.0f)*vis;   // DARK limbal ring at the iris edge → pops off white
    c.r=(uint8_t)(c.r*(1.0f-0.80f*limbal)); c.g=(uint8_t)(c.g*(1.0f-0.80f*limbal)); c.b=(uint8_t)(c.b*(1.0f-0.80f*limbal));
    float pm=pupF*vis;                                          // near-black pupil
    c.r=(uint8_t)(c.r*(1.0f-pm)); c.g=(uint8_t)(c.g*(1.0f-pm)); c.b=(uint8_t)(c.b*(1.0f-pm));
    float cl=constrain(1.0f-sqrtf((x-gx+4.0f)*(x-gx+4.0f)+(y-gy+4.0f)*(y-gy+4.0f))/3.0f,0.0f,1.0f)*vis;
    c.r=qadd8(c.r,(uint8_t)(cl*220)); c.g=qadd8(c.g,(uint8_t)(cl*225)); c.b=qadd8(c.b,(uint8_t)(cl*210));  // wet catch-light
  }
  out=c;
  return true;
}
void anim_eye(uint32_t at,float dt){
  if(!eyeInit){ eyeGx=eyeGy=eyeTx=eyeTy=0.0f; eyeMoveT=at; eyeBlinkT=at; eyeBlk=false; eyeInit=true; }
  if(at-eyeMoveT>1200){ eyeMoveT=at; eyeTx=frand()*52.0f-26.0f; eyeTy=frand()*16.0f-8.0f; }  // new gaze
  float k=dt*0.005f; if(k>1) k=1;
  eyeGx+=(eyeTx-eyeGx)*k; eyeGy+=(eyeTy-eyeGy)*k;                 // smooth eye movement
  if(!eyeBlk && at-eyeBlinkT>2800){ eyeBlk=true; eyeBlinkT=at; }
  float bp=0; if(eyeBlk){ uint32_t e=at-eyeBlinkT; if(e>=460) eyeBlk=false; else bp=1.0f-fabsf((float)e-230.0f)/230.0f; }  // slower lid
  clearFrame();
  CRGB c;
  for(int xi=0;xi<12;xi++){ float x=X_POS[xi];
    for(int n=1;n<=121;n++){ float y=(float)n-CENTER; if(eyePixel(x,y,eyeGx,eyeGy,bp,c)) setLED(true,xi+1,n,c); } }
  for(int yi=0;yi<12;yi++){ float y=Y_POS[yi];
    for(int n=1;n<=121;n++){ float x=(float)n-CENTER; if(eyePixel(x,y,eyeGx,eyeGy,bp,c)) setLED(false,yi+1,n,c); } }
  showGrid();
}

// ══════════════════════════════════════════════════════════════════════
//  NEW SHARP-EDGED MODES (Task 4)  — indices 59..83
//  All procedural (no per-mode state → nothing to reset). Rendered per-LED
//  in float world-coords with a NARROW anti-alias band for crisp edges;
//  additive (qadd8) so overlapping light blends. Helpers first.
// ══════════════════════════════════════════════════════════════════════
static inline float fxfract(float x){ return x-floorf(x); }
