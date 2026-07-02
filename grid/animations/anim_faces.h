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
  "ALIENS","INVADERS","RAIN2","LEVITATE","CONFETTI","SPIRALEJ","CLOUDS3D","HEART2"
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
void anim_textTOGA(uint32_t at){   anim_scrollBanner("TOGA",  at, 40); }
void anim_textCASTLE(uint32_t at){ anim_scrollBanner("CASTLE",at,140); }
void anim_text2026(uint32_t at){   anim_scrollBanner("2026",  at,210); }

// ═══════════ 55. RUNNER (Strichmännchen jump'n'run, Mario-style) ═══════════
bool stickInit=false;
static float rnObsX[RUN_OBS]; static uint8_t rnObsType[RUN_OBS]; static bool rnObsOn[RUN_OBS];
static float rnJumpH,rnJumpV; static bool rnJumping; static uint32_t rnSpawn;
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
  if(!stickInit){ for(int i=0;i<RUN_OBS;i++) rnObsOn[i]=false; rnJumpH=0;rnJumpV=0;rnJumping=false; rnSpawn=at; stickInit=true; }
  // world scrolls: obstacles move left toward the runner
  for(int i=0;i<RUN_OBS;i++) if(rnObsOn[i]){ rnObsX[i]-=0.006f*dt; if(rnObsX[i]<-2.0f) rnObsOn[i]=false; }
  if(at-rnSpawn>1500){ rnSpawn=at;
    for(int i=0;i<RUN_OBS;i++) if(!rnObsOn[i]){ rnObsX[i]=12.0f; rnObsType[i]=random(4); rnObsOn[i]=true; break; } }
  // auto-jump when an obstacle approaches (jump higher to clear trees)
  if(!rnJumping) for(int i=0;i<RUN_OBS;i++) if(rnObsOn[i]){ float d=rnObsX[i]-RX; if(d>0.3f&&d<3.4f){ rnJumping=true; rnJumpV=0.020f; rnJumpH=0.01f; break; } }
  if(rnJumping){ rnJumpH+=rnJumpV*dt; rnJumpV-=0.00005f*dt; if(rnJumpH<=0){ rnJumpH=0; rnJumpV=0; rnJumping=false; } }
  int F=GND-(int)(rnJumpH+0.5f);                // feet node (rises when jumping)
  bool f=((at/140)&1);                          // running leg frame
  CRGB col =CRGB(CHSV(35,180,235));             // runner
  CRGB gcol=CRGB(CHSV(95,200,110));             // ground
  clearFrame();
  for(int c=0;c<11;c++) drawHSeg(GND,c,gcol);   // ground line
  for(int i=0;i<RUN_OBS;i++) if(rnObsOn[i]) rnDrawObstacle(rnObsType[i],(int)(rnObsX[i]+0.5f),GND);
  // ── runner ──
  int C=RX;
  fillCell(F-5,C-1,col); fillCell(F-5,C,col);   // head (2-cell box)
  drawVSeg(F-4,C,col); drawVSeg(F-3,C,col);      // torso
  if(f) drawHSeg(F-4,C,col); else drawHSeg(F-4,C-1,col);            // pumping arm
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
void anim_smiley(uint32_t at){
  int expr=(int)((at/1600)%4);                  // 4 upbeat expressions, ~1.6s each
  CRGB face=CRGB(CHSV(45,235,205));
  CRGB eye =CRGB(CHSV(150,220,235));
  CRGB mth =CRGB(CHSV(0,230,235));
  clearFrame();
  smArc(5.5f,5.5f,4.3f,face,0);                 // smooth round face
  // eyes: 2x2 blocks at (2,2) and (2,6), symmetric on the round face
  int le,re;
  switch(expr){
    case 0:  le=re=EYE_SOLID; break;                  // happy   → wide eyes
    case 1:  le=EYE_SOLID; re=EYE_HLINE; break;       // wink    → one eye a line
    case 2:  le=re=EYE_VLINE; break;                  // surprised → alert vertical eyes
    default: le=re=EYE_CROSS; break;                  // playful → cross eyes
  }
  drawEye(2,2,le,eye); drawEye(2,6,re,eye);
  // mouth: always upbeat — smile, small O, or a straight line (never sad/angry)
  switch(expr){
    case 0: case 1: smArc(3.7f,5.5f,3.7f,mth,1); break;                    // smile (lower arc)
    case 2: smArc(7.4f,5.5f,1.4f,mth,0); break;                           // surprised O
    default: drawHSeg(8,4,mth); drawHSeg(8,5,mth); drawHSeg(8,6,mth); break; // neutral line
  }
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
  float ax=x/A, qx=1.0f-ax*ax; if(qx<=-0.06f) return false;      // outside (margin keeps the outline)
  float sq=sqrtf(qx>0.0f?qx:0.0f);
  float e=ax*ax+(y/B)*(y/B);
  float outF=constrain(1.0f-fabsf(e-1.0f)/0.16f,0.0f,1.0f);      // almond outline — always visible
  float openTop=(2.0f*bp-1.0f)*B*sq, openBot=B*sq;               // upper lid sweeps down to close
  float vis=constrain((y-openTop)/SOFT,0.0f,1.0f)*constrain((openBot-y)/SOFT,0.0f,1.0f);
  float scl=constrain((0.94f-e)/0.12f,0.0f,1.0f)*vis;           // sclera fill (hidden by the lid)
  float dg=sqrtf((x-gx)*(x-gx)+(y-gy)*(y-gy));
  float irisF=constrain((RI-dg)/SOFT,0.0f,1.0f);
  float pupF =constrain((RP-dg)/SOFT,0.0f,1.0f);
  float wF=fmaxf(outF*0.5f, scl*(1.0f-irisF));                   // dim contour, or bright fill
  float iF=irisF*(1.0f-pupF)*vis;                               // iris ring (dark pupil), hidden by lid
  if(wF<0.02f && iF<0.02f) return false;
  uint8_t wv=(uint8_t)(wF*160.0f);
  CRGB c(wv,wv,(uint8_t)(wv*0.85f));                            // warm-white sclera / contour
  CRGB ir=CRGB(CHSV(140,225,(uint8_t)(iF*235.0f)));            // teal iris
  out=CRGB(qadd8(c.r,ir.r),qadd8(c.g,ir.g),qadd8(c.b,ir.b));
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
