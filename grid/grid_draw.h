// Low-level drawing primitives
// (Auto-split from grid_SW.ino; #included into the main sketch — all shared
//  globals/functions are visible from grid_SW.ino. Do not compile standalone.)
#pragma once

// INTS[] (strip-crossing LED indices) and the grid geometry now live in grid_config.h.

// Draw a square outline defined by grid coordinates (r1,c1)→(r2,c2)
// r,c are strip indices 1..12. Draws the 4 border strips between them.
void drawGridSquare(int r1,int c1,int r2,int c2,CRGB colour) {
  // top border: Y strip r1, LEDs between X c1 and X c2
  for(int led=INTS[c1-1]+1;led<INTS[c2-1];led++) setLED(false,r1,led,colour);
  // bottom border: Y strip r2
  for(int led=INTS[c1-1]+1;led<INTS[c2-1];led++) setLED(false,r2,led,colour);
  // left border: X strip c1, LEDs between Y r1 and Y r2
  for(int led=INTS[r1-1]+1;led<INTS[r2-1];led++) setLED(true,c1,led,colour);
  // right border: X strip c2
  for(int led=INTS[r1-1]+1;led<INTS[r2-1];led++) setLED(true,c2,led,colour);
}

// ═══════════════════════════════════════════════════════
//  TEXT / FONT SYSTEM  (5x7 glyphs on the 11x11 cell canvas)
//  Used by the banner modes and the mode-change overlay.
// ═══════════════════════════════════════════════════════
// ── 7-segment (DSEG7-style) text, drawn with STRIP SEGMENTS ──
// Each glyph is ONE cell wide x TWO cells tall. A "segment" is a single
// strip-segment between two adjacent intersections — i.e. one LED strip
// wide — so "1"/"I" is a thin vertical line, not a filled cell.
// Node (R,C), R,C in 0..11: horizontal segment = piece of Y strip R+1
// between X-cols C..C+1; vertical segment = piece of X strip C+1 between
// Y-rows R..R+1. (Corner LEDs at intersections are left dark, like a
// real 7-seg display where segments don't touch.)
void drawHSeg(int R,int C,CRGB col){        // horizontal → Y strip R+1
  if(R<0||R>11||C<0||C>10) return;
  for(int led=INTS[C]+1; led<INTS[C+1]; led++) setLED(false,R+1,led,col);
}
void drawVSeg(int R,int C,CRGB col){        // vertical → X strip C+1
  if(R<0||R>10||C<0||C>11) return;
  for(int led=INTS[R]+1; led<INTS[R+1]; led++) setLED(true,C+1,led,col);
}

// ═══════════════════════════════════════════════════════
//  EXTRA ANIMATION PACK (cases 39..50)
//  Reuses setLED / drawGridSquare. "Cell canvas" = 11x11
//  grid of squares (row,col 0..10); one cell == one grid
//  square outline, used as a low-res pixel for figures,
//  Tetris and other block-based effects.
// ═══════════════════════════════════════════════════════

static inline int iabs(int v){ return v<0?-v:v; }
static inline float frand(){ return random(1001)*0.001f; }

// Light one cell of the 11x11 canvas (row,col 0..10).
void fillCell(int row,int col,CRGB c){
  if(row<0||row>10||col<0||col>10) return;
  drawGridSquare(row+1,col+1,row+2,col+2,c);
}

// Lightweight per-frame clear (keeps spacer LEDs black).
void clearFrame(){
  FastLED.clear();
  for(int e=0;e<NUM_ELEC;e++){
    ledsX[e][255]=ledsX[e][256]=ledsX[e][257]=CRGB::Black;
    ledsY[e][255]=ledsY[e][256]=ledsY[e][257]=CRGB::Black;
  }
}

// ═══════════ 57. SMILEY (changing expressions) ═══════════
// Face + mouth drawn as smooth arcs of thin strip segments (not boxy cells).
// quad: 0=full circle, 1=lower arc (smile), 2=upper arc (frown).
void smArc(float cy,float cx,float rad,CRGB col,int quad){
  for(int R=0;R<=11;R++) for(int C=0;C<11;C++){
    float dy=R-cy, dx=(C+0.5f)-cx, d=sqrtf(dy*dy+dx*dx);
    if(fabsf(d-rad)<0.62f && (quad==0||(quad==1&&dy>0.1f)||(quad==2&&dy<-0.1f))) drawHSeg(R,C,col);
  }
  for(int R=0;R<11;R++) for(int C=0;C<=11;C++){
    float dy=(R+0.5f)-cy, dx=C-cx, d=sqrtf(dy*dy+dx*dx);
    if(fabsf(d-rad)<0.62f && (quad==0||(quad==1&&dy>0.1f)||(quad==2&&dy<-0.1f))) drawVSeg(R,C,col);
  }
}
// Additive pixel write using the exact setLED index mapping.
void fxAdd(bool isX,int strip,int led,CRGB c,float f){
  if(led<1||led>121||f<=0.0f) return; if(f>1.0f) f=1.0f;
  int elec=(strip-1)/2; bool isOdd=(strip%2==1); bool forward; int bufIdx;
  if(isX)forward=!isOdd; else forward=isOdd;
  if(isOdd)bufIdx=forward?(led-1):(121-led); else bufIdx=forward?(135+led):(257-led);
  CRGB* p=isX?&ledsX[elec][bufIdx]:&ledsY[elec][bufIdx];
  p->r=qadd8(p->r,(uint8_t)(c.r*f)); p->g=qadd8(p->g,(uint8_t)(c.g*f)); p->b=qadd8(p->b,(uint8_t)(c.b*f));
}
// Crisp round point of light at world (px,py): solid to 'rad', ramps out over 'aa'.
void fxSplat(float px,float py,float rad,float aa,CRGB col){
  float R=rad+aa;
  for(int xi=0;xi<12;xi++){ float dx=X_POS[xi]-px; if(fabsf(dx)>R)continue;
    int n0=(int)(py+CENTER-R),n1=(int)(py+CENTER+R); if(n0<1)n0=1; if(n1>121)n1=121;
    for(int n=n0;n<=n1;n++){ float dy=(float)n-CENTER-py; float d=sqrtf(dx*dx+dy*dy);
      fxAdd(true,xi+1,n,col,(rad+aa-d)/aa); } }
  for(int yi=0;yi<12;yi++){ float dy=Y_POS[yi]-py; if(fabsf(dy)>R)continue;
    int n0=(int)(px+CENTER-R),n1=(int)(px+CENTER+R); if(n0<1)n0=1; if(n1>121)n1=121;
    for(int n=n0;n<=n1;n++){ float dx=(float)n-CENTER-px; float d=sqrtf(dx*dx+dy*dy);
      fxAdd(false,yi+1,n,col,(rad+aa-d)/aa); } }
}
// Sharp line/streak between two world points (sampled splats).
void fxStreak(float x0,float y0,float x1,float y1,float rad,float aa,CRGB col){
  float dx=x1-x0,dy=y1-y0; int steps=(int)sqrtf(dx*dx+dy*dy)+1;
  for(int i=0;i<=steps;i++){ float u=(float)i/steps; fxSplat(x0+dx*u,y0+dy*u,rad,aa,col); }
}
// Crisp circle outline of radius 'rad', half-thickness 'thick', edge softness 'aa'.
void fxRing(float cx,float cy,float rad,float thick,float aa,CRGB col){
  float R=rad+thick+aa;
  for(int xi=0;xi<12;xi++){ float dx=X_POS[xi]-cx; if(fabsf(dx)>R)continue;
    for(int n=1;n<=121;n++){ float dy=(float)n-CENTER-cy; float d=sqrtf(dx*dx+dy*dy);
      fxAdd(true,xi+1,n,col,(thick+aa-fabsf(d-rad))/aa); } }
  for(int yi=0;yi<12;yi++){ float dy=Y_POS[yi]-cy; if(fabsf(dy)>R)continue;
    for(int n=1;n<=121;n++){ float dx=(float)n-CENTER-cx; float d=sqrtf(dx*dx+dy*dy);
      fxAdd(false,yi+1,n,col,(thick+aa-fabsf(d-rad))/aa); } }
}
// Soft noise cloud inside a radial envelope (used by the nebula modes).
void fxNebula(uint32_t at,uint8_t hue,uint8_t sat,float scale,float tspd,float radius,float gain){
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER;
      float x=pass?other:base, y=pass?base:other; float d=sqrtf(x*x+y*y);
      if(d>radius) continue;
      uint8_t nz=inoise8((uint16_t)((x+80)*scale),(uint16_t)((y+80)*scale),(uint16_t)(at*tspd));
      float env=constrain((radius-d)/radius,0.0f,1.0f);
      float v=(nz/255.0f); v=v*v*env*env*gain;
      if(v>0.02f) fxAdd(pass?false:true,s+1,n,CRGB(CHSV(hue,sat,255)),v);
    } }
}
// Value-driven "hot" colour — restores the early field-mode look for later modes.
//   bright → white-hot core (saturation ramps down), dim → fully saturated;
//   hue spreads with the value so one mode drifts across a band of hues.
// v in 0..1 (intensity), h0 = base/identity hue, span = hue spread across the range,
// satFloor = saturation at full brightness (lower = whiter core). Mirrors the inline
// `sat = 255 - bri*K` trick used all over anim_field.h / anim_geometry.h.
static inline CRGB hotColor(float v,uint8_t h0,uint8_t span=32,uint8_t satFloor=50){
  v = v<0.0f?0.0f:(v>1.0f?1.0f:v);
  uint8_t hue = h0 + (uint8_t)(v*span);
  uint8_t sat = (uint8_t)(255.0f - v*(255.0f-satFloor));   // white-hot core
  return CRGB(CHSV(hue,sat,(uint8_t)(v*255.0f)));
}
// Same white-hot ramp + hue drift, but full value (255) — for use as the colour arg to
// fxAdd(), where intensity is applied separately by fxAdd's own factor. Prevents the
// brightness being squared, while still bleaching the hot samples toward white.
static inline CRGB hotTint(float v,uint8_t h0,uint8_t span=32,uint8_t satFloor=50){
  v = v<0.0f?0.0f:(v>1.0f?1.0f:v);
  return CRGB(CHSV(h0+(uint8_t)(v*span),(uint8_t)(255.0f - v*(255.0f-satFloor)),255));
}

// Soft-shaded lit sphere with a light direction and faint surface texture.
void fxPlanet(float cx,float cy,float r,uint8_t hue,uint8_t sat,float lx,float ly,uint16_t rot){
  float R=r+1.3f;
  for(int pass=0;pass<2;pass++) for(int s=0;s<12;s++){ float base=pass?Y_POS[s]:X_POS[s];
    for(int n=1;n<=121;n++){ float other=(float)n-CENTER; float x=pass?other:base,y=pass?base:other;
      float dx=x-cx,dy=y-cy,d=sqrtf(dx*dx+dy*dy); if(d>R)continue;
      float mask=constrain((r+1.3f-d)/1.3f,0.0f,1.0f);
      float lit=0.12f+0.88f*constrain(0.5f+0.5f*(dx*lx+dy*ly)/r,0.0f,1.0f);
      uint8_t nz=inoise8((uint16_t)((x-cx+40)*380+rot),(uint16_t)((y-cy+40)*380),1234);
      float v=mask*lit*(0.72f+0.28f*(nz/255.0f));
      if(v>0.02f) fxAdd(pass?false:true,s+1,n,CRGB(CHSV(hue,sat,255)),v);
    } }
}
