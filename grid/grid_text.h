// Segment font, text & mode-number overlay
// (Auto-split from grid_SW.ino; #included into the main sketch — all shared
//  globals/functions are visible from grid_SW.ino. Do not compile standalone.)
#pragma once

// 12-segment stroke font: each glyph is 2 cells wide x 2 cells tall
// (a 3x3 node grid). The centre column + centre row let us draw proper
// letters (real G, R, K, M, N…) rather than calculator digits.
//   n--TL--n--TR--n
//   |LU     |CU    |RU
//   n--ML--n--MR--n
//   |LL     |CL    |RL
//   n--BL--n--BR--n
enum { sTL=1,sTR=2,sML=4,sMR=8,sBL=16,sBR=32,
       sLU=64,sLL=128,sCU=256,sCL=512,sRU=1024,sRL=2048 };
const uint16_t FONTW[37]={
  0,                                              // ' '
  sTL|sTR|sLU|sLL|sRU|sRL|sML|sMR,                // A
  sTL|sTR|sLU|sLL|sRU|sRL|sML|sMR|sBL|sBR,        // B (~8)
  sTL|sTR|sLU|sLL|sBL|sBR,                        // C
  sTL|sLU|sLL|sBL|sCU|sCL,                        // D (narrow box: 1 cell wide, so it is not another O)
  sTL|sTR|sLU|sLL|sML|sMR|sBL|sBR,                // E
  sTL|sTR|sLU|sLL|sML|sMR,                        // F
  sTL|sTR|sLU|sLL|sBL|sBR|sRL|sMR,                // G
  sLU|sLL|sRU|sRL|sML|sMR,                        // H
  sTL|sTR|sBL|sBR|sCU|sCL,                        // I
  sRU|sRL|sBL|sBR|sLL,                            // J
  sLU|sLL|sMR|sRU|sRL,                            // K
  sLU|sLL|sBL|sBR,                                // L
  sLU|sLL|sRU|sRL|sTL|sTR|sCU,                    // M
  sLU|sLL|sRU|sRL|sCU|sCL,                        // N
  sTL|sTR|sLU|sLL|sRU|sRL|sBL|sBR,                // O
  sTL|sTR|sLU|sLL|sML|sMR|sRU,                    // P
  sTL|sTR|sLU|sLL|sRU|sRL|sBL|sBR|sCL,            // Q
  sTL|sTR|sLU|sLL|sML|sMR|sRU|sRL,                // R
  sTL|sTR|sLU|sML|sMR|sRL|sBL|sBR,                // S
  sTL|sTR|sCU|sCL,                                // T
  sLU|sLL|sRU|sRL|sBL|sBR,                        // U
  sLU|sLL|sRU|sRL|sBL|sBR,                        // V (~U)
  sLU|sLL|sRU|sRL|sBL|sBR|sCL,                    // W
  sLU|sRU|sCU|sCL|sLL|sRL,                        // X (approx)
  sLU|sRU|sCU|sCL,                                // Y
  sTL|sTR|sMR|sCL|sBL|sBR,                        // Z (approx)
  sTL|sTR|sLU|sLL|sRU|sRL|sBL|sBR,                // 0
  sCU|sCL,                                        // 1
  sTL|sTR|sRU|sML|sMR|sLL|sBL|sBR,                // 2
  sTL|sTR|sRU|sML|sMR|sRL|sBL|sBR,                // 3
  sLU|sML|sMR|sRU|sRL,                            // 4
  sTL|sTR|sLU|sML|sMR|sRL|sBL|sBR,                // 5
  sTL|sTR|sLU|sLL|sML|sMR|sRL|sBL|sBR,            // 6
  sTL|sTR|sRU|sRL,                                // 7
  sTL|sTR|sLU|sLL|sRU|sRL|sML|sMR|sBL|sBR,        // 8
  sTL|sTR|sLU|sRU|sRL|sML|sMR|sBL|sBR             // 9
};
int glyphIndex(char c){
  if(c>='a'&&c<='z') c-=32;
  if(c>='A'&&c<='Z') return c-'A'+1;
  if(c>='0'&&c<='9') return c-'0'+27;
  return 0; // space / unknown
}
// Draw one glyph with top-left node (topR,C). Box spans nodes topR..topR+2, C..C+2.
void drawGlyphW(uint16_t s,int topR,int C,CRGB col){
  if(s&sTL) drawHSeg(topR,   C,   col);
  if(s&sTR) drawHSeg(topR,   C+1, col);
  if(s&sML) drawHSeg(topR+1, C,   col);
  if(s&sMR) drawHSeg(topR+1, C+1, col);
  if(s&sBL) drawHSeg(topR+2, C,   col);
  if(s&sBR) drawHSeg(topR+2, C+1, col);
  if(s&sLU) drawVSeg(topR,   C,   col);
  if(s&sLL) drawVSeg(topR+1, C,   col);
  if(s&sCU) drawVSeg(topR,   C+1, col);
  if(s&sCL) drawVSeg(topR+1, C+1, col);
  if(s&sRU) drawVSeg(topR,   C+2, col);
  if(s&sRL) drawVSeg(topR+1, C+2, col);
}
// Width / draw in node-columns (3 per char: 2 wide glyph + 1 gap).
int textLenNodes(const char* s){ int n=0; while(*s++) n++; return n*3; }
void drawTextCells(const char* s,int leftC,int topR,CRGB col){
  int C=leftC;
  for(const char* p=s;*p;p++){ drawGlyphW(FONTW[glyphIndex(*p)],topR,C,col); C+=3; }
}

// ── Condensed 1-cell-wide 7-segment micro-font (Task 5) ──
// Each glyph is ONE cell wide x TWO cells tall (nodes topR..topR+2, C..C+1), so
// 6 letters fit statically across the 11-column lattice (CASTLE / 2026, no scroll).
// Segments a=top g=mid d=bottom  f=top-left e=bottom-left  b=top-right c=bottom-right.
enum { g7a=1,g7b=2,g7c=4,g7d=8,g7e=16,g7f=32,g7g=64, g7EXCL=128 };   // g7EXCL: the '!' glyph (see drawGlyph1)
uint8_t seg7(char c){
  if(c>='a'&&c<='z') c-=32;
  switch(c){
    case '0': return g7a|g7b|g7c|g7d|g7e|g7f;
    case '1': return g7b|g7c;
    case '2': return g7a|g7b|g7g|g7e|g7d;
    case '3': return g7a|g7b|g7g|g7c|g7d;
    case '4': return g7f|g7g|g7b|g7c;
    case '5': return g7a|g7f|g7g|g7c|g7d;
    case '6': return g7a|g7f|g7g|g7e|g7c|g7d;
    case '7': return g7a|g7b|g7c;
    case '8': return g7a|g7b|g7c|g7d|g7e|g7f|g7g;
    case '9': return g7a|g7b|g7c|g7d|g7f|g7g;
    case 'A': return g7a|g7b|g7c|g7e|g7f|g7g;
    case 'C': return g7a|g7d|g7e|g7f;
    case 'E': return g7a|g7d|g7e|g7f|g7g;
    case 'F': return g7a|g7e|g7f|g7g;
    case 'G': return g7a|g7c|g7d|g7e|g7f;   // C-shape + bottom-right → reads as G
    case 'H': return g7b|g7c|g7e|g7f|g7g;
    case 'L': return g7d|g7e|g7f;
    case 'O': return g7a|g7b|g7c|g7d|g7e|g7f;
    case 'P': return g7a|g7b|g7e|g7f|g7g;
    case 'S': return g7a|g7c|g7d|g7f|g7g;
    case 'T': return g7d|g7e|g7f|g7g;   // 7-seg 't' (no centre column possible in 1 cell)
    case 'U': return g7b|g7c|g7d|g7e|g7f;
    // v11 (Drahthaus texts): lowercase 7-seg shapes where the capital has no 1-cell form
    case 'D': return g7b|g7c|g7d|g7e|g7g;   // 'd'
    case 'R': return g7e|g7g;               // 'r'
    case 'N': return g7c|g7e|g7g;           // 'n'
    case 'I': return g7b|g7c;               // like 1
    case 'V': return g7b|g7c|g7d|g7e|g7f;   // = U (no diagonal possible)
    case '!': return g7EXCL;                // special-cased in drawGlyph1: bar + dot
    default:  return 0;                 // space / unsupported
  }
}
void drawGlyph1(uint8_t s,int topR,int C,CRGB col){   // 1 cell wide, 2 cells tall
  if(s&g7EXCL){                                    // '!': upper bar on the right column + a 3-LED dot below it
    drawVSeg(topR, C+1, col);
    if(topR+1>=0 && topR+1<=10 && C+1>=0 && C+1<=11)
      for(int led=INTS[topR+2]-3; led<INTS[topR+2]; led++) setLED(true,C+2,led,col);
    return;
  }
  if(s&g7a) drawHSeg(topR,   C,   col);
  if(s&g7g) drawHSeg(topR+1, C,   col);
  if(s&g7d) drawHSeg(topR+2, C,   col);
  if(s&g7f) drawVSeg(topR,   C,   col);
  if(s&g7e) drawVSeg(topR+1, C,   col);
  if(s&g7b) drawVSeg(topR,   C+1, col);
  if(s&g7c) drawVSeg(topR+1, C+1, col);
}
// Draw a word centered horizontally at node-row topR (advances 2 node-cols/char).
void drawText1Centered(const char* s,int topR,CRGB col){
  int n=0; for(const char* p=s;*p;p++) n++;
  int leftC=(12-2*n)/2; if(leftC<0) leftC=0;
  int C=leftC; for(const char* p=s;*p;p++){ drawGlyph1(seg7(*p),topR,C,col); C+=2; }
}
#define TEXT_TOP 4          // top node row for banner text (glyphs 2 cells tall → rows 4..6)
// Same font, explicit left column (may be negative / past the edge → clipped per segment).
void drawText1At(const char* s,int leftC,int topR,CRGB col){
  int C=leftC; for(const char* p=s;*p;p++){ drawGlyph1(seg7(*p),topR,C,col); C+=2; }
}

// ── Segment sprites: pictograms on the 12x12 node lattice ──────────────────────
// A sprite is two bitmaps over its own node box (w cols x h rows): hRows[R] bit C =
// horizontal segment (R,C)->(R,C+1), vRows[R] bit C = vertical segment (R,C)->(R+1,C).
// Drawn at a node offset; `mirror` flips it left<->right inside its own width, so a
// right-facing figure can face left without a second table. Segments outside the
// lattice are dropped by drawHSeg/drawVSeg, so a sprite can slide in from the edge.
struct SegSprite { uint8_t w, h; uint16_t hRows[8]; uint16_t vRows[8]; };
void drawSegSprite(const SegSprite& sp,int offR,int offC,bool mirror,CRGB col){
  for(int R=0;R<sp.h;R++){
    for(int C=0;C<sp.w-1;C++) if(sp.hRows[R]&(1u<<C)) drawHSeg(offR+R, offC+(mirror? sp.w-2-C : C), col);
    for(int C=0;C<sp.w;C++)   if(sp.vRows[R]&(1u<<C)) drawVSeg(offR+R, offC+(mirror? sp.w-1-C : C), col);
  }
}
// Elephant, facing right, 11 cols x 8 rows: back + head on row 0, tail at the left,
// a boxed ear, two legs, and a trunk that either hangs with a curl (frame 0) or
// kicks forward (frame 1). Used by the "100 Elefanten" mode and the text strobe.
const SegSprite ELEFANT[2] = {
  { 11,8, {0x0FE,0x041,0x140,0x080,0x000,0x038,0x100,0x042}, {0x102,0x1C3,0x303,0x282,0x282,0x2C6,0x0C6,0x000} },
  { 11,8, {0x0FE,0x041,0x140,0x080,0x200,0x038,0x000,0x042}, {0x102,0x1C3,0x303,0x282,0x082,0x0C6,0x0C6,0x000} }
};

// ── Drahthaus text strobe looks (v11) ─────────────────────────────────────────
// One flash of text `id` (TOGA_TEXT_*), flash number `n` within the current press.
// Called with the frame already cleared; the caller shows it. Fonts: the 1-cell
// 7-seg font fits 6 glyphs across, the wide 12-seg font 4 — so the longer texts
// are sequenced over flashes rather than squeezed.
#define TX_ELEFANT_FLIP 4      // elephant: flashes before it turns around

// ALLE GEGEN ALLE as one picture, drawn from the hand sketch: three stacked words whose
// letters share strokes (the A's right leg is the first L's stem, GEGEN is one box).
// Lines in node coords relative to the top-left node; {horizontal?, fixed, from, to}.
// 10 cols x 8 rows; at AGA_C0/AGA_R0 it sits exactly where the sketch put it.
#define AGA_C0 1
#define AGA_R0 2
struct SegLine { bool h; int8_t at, a, b; };
const SegLine ALLE_LINES[] = {                 // one ALLE, rows 0..2
  {true,0,1,3},{true,1,1,3},{true,2,3,9},{true,0,7,9},{true,1,7,9},
  {false,1,0,2},{false,3,0,2},{false,5,0,2},{false,7,0,2} };
const SegLine GEGEN_LINES[] = {                // rows 3..5
  {true,3,0,10},{true,5,0,8},{true,4,1,3},{true,4,5,8},
  {false,0,3,5},{false,2,3,5},{false,4,3,5},{false,6,3,5},{false,8,3,5},{false,10,3,5} };   // N: left stem now full height (was missing its top half)
void drawSegLines(const SegLine* L,int n,int offR,int offC,CRGB col){
  for(int i=0;i<n;i++){
    if(L[i].h) for(int c=L[i].a;c<L[i].b;c++) drawHSeg(offR+L[i].at, offC+c, col);
    else       for(int r=L[i].a;r<L[i].b;r++) drawVSeg(offR+r, offC+L[i].at, col);
  }
}
void drawAlleGegenAlle(CRGB col){
  const int nA=sizeof(ALLE_LINES)/sizeof(ALLE_LINES[0]), nG=sizeof(GEGEN_LINES)/sizeof(GEGEN_LINES[0]);
  drawSegLines(ALLE_LINES, nA,AGA_R0,  AGA_C0,col);
  drawSegLines(GEGEN_LINES,nG,AGA_R0,  AGA_C0,col);
  drawSegLines(ALLE_LINES, nA,AGA_R0+6,AGA_C0,col);
}
// RAVE as bold block letters: "RA" over "VE", each letter 5x5 CELLS of the 11x11
// canvas (one cell = one grid square, all four edges lit), so every stroke is a
// full band one square wide and the word fills the whole wall.
static const uint8_t RAVE_BMP[4][5]={            // bit 4 = left column
  {0b11110,0b10001,0b11110,0b10010,0b10001},     // R
  {0b01110,0b10001,0b11111,0b10001,0b10001},     // A
  {0b10001,0b10001,0b10001,0b01010,0b00100},     // V
  {0b11111,0b10000,0b11110,0b10000,0b11111}};    // E
void drawRaveBlock(CRGB col){
  static const int8_t POS[4][2]={{0,0},{0,6},{6,0},{6,6}};   // R A / V E  (row, col of the letter's top-left cell)
  for(int L=0;L<4;L++) for(int r=0;r<5;r++) for(int c=0;c<5;c++)
    if(RAVE_BMP[L][r]&(0x10>>c)) fillCell(POS[L][0]+r,POS[L][1]+c,col);
}
// ── "Sketch" lettering for the DRAHTHAUS text strobes (drawn by the band) ──
// Square letters on the strip lattice. Two styles of the same letter shapes:
//   · WIDE   : 2x2 grid squares per letter (3x3 nodes), for DHA / RTU / AHS
//   · NARROW : 1 square wide, 2 tall (T is 2 wide so it can keep its centre stem),
//              for DRAHT / HAUS
// From the sketch: A = two full sides, top and middle bar, open bottom. R = the
// same, but its top and middle bar stick out HALF a square to the LEFT and (wide only)
// its lower leg stands in the middle column. T = top bar
// and a centre stem. D = a closed box whose top and bottom bar stick out to the
// left. H / U / S are not in the sketch and follow the same square style.
// The left overhang is drawn into the gap before the letter.
enum { SK_TOP=1, SK_MID=2, SK_BOT=4 };               // which bars overhang to the left
// The serif: only the RIGHT HALF of the horizontal segment (R,C) — half a square,
// touching the letter's stem at node C+1, the left half stays dark.
static void skSerif(int R,int C,CRGB col){
  if(R<0||R>11||C<0||C>10) return;
  int mid=(INTS[C]+INTS[C+1])/2;
  for(int led=mid; led<INTS[C+1]; led++) setLED(false,R+1,led,col);
}
static uint8_t skOver(char c){ return c=='R' ? (SK_TOP|SK_MID) : (c=='D' ? (SK_TOP|SK_BOT) : 0); }
static uint16_t skWide(char c){
  switch(c){
    case 'A': return sTL|sTR|sLU|sLL|sRU|sRL|sML|sMR;
    case 'R': return sTL|sTR|sLU|sLL|sRU|sML|sMR|sCL;      // the lower leg in the MIDDLE column, not at the right edge
    case 'D': return sTL|sTR|sLU|sLL|sRU|sRL|sBL|sBR;
    case 'T': return sTL|sTR|sCU|sCL;
    case 'H': return sLU|sLL|sRU|sRL|sML|sMR;
    case 'U': return sLU|sLL|sRU|sRL|sBL|sBR;
    case 'S': return sTL|sTR|sLU|sML|sMR|sRL|sBL|sBR;
    default:  return 0;
  }
}
void drawSketchWide(const char* s,int leftC,int topR,CRGB col){
  int C=leftC;
  for(const char* p=s;*p;p++){
    drawGlyphW(skWide(*p),topR,C,col);
    uint8_t o=skOver(*p);
    if(o&SK_TOP) skSerif(topR,  C-1,col);
    if(o&SK_MID) skSerif(topR+1,C-1,col);
    if(o&SK_BOT) skSerif(topR+2,C-1,col);
    C+=3;
  }
}
// One narrow letter at node (topR,C). Returns its width in squares (1, T = 2).
static int drawSketchNarrowGlyph(char ch,int topR,int C,CRGB col){
  if(ch=='T'){ drawHSeg(topR,C,col); drawHSeg(topR,C+1,col); drawVSeg(topR,C+1,col); drawVSeg(topR+1,C+1,col); return 2; }
  bool a=false,g=false,d=false,f=false,e=false,b=false,c=false;
  switch(ch){
    case 'A': case 'R': a=g=f=e=b=c=true; break;
    case 'D': a=d=f=e=b=c=true; break;
    case 'H': g=f=e=b=c=true; break;
    case 'U': d=f=e=b=c=true; break;
    case 'S': a=f=g=c=d=true; break;
    default: return 1;
  }
  if(a) drawHSeg(topR,C,col);   if(g) drawHSeg(topR+1,C,col);   if(d) drawHSeg(topR+2,C,col);
  if(f) drawVSeg(topR,C,col);   if(e) drawVSeg(topR+1,C,col);
  if(b) drawVSeg(topR,C+1,col); if(c) drawVSeg(topR+1,C+1,col);
  uint8_t o=skOver(ch);
  if(o&SK_TOP) skSerif(topR,  C-1,col);
  if(o&SK_MID) skSerif(topR+1,C-1,col);
  if(o&SK_BOT) skSerif(topR+2,C-1,col);
  return 1;
}
// A word of narrow letters, one free square between letters (room for the overhang), centred.
void drawSketchNarrow(const char* s,int topR,CRGB col){
  int w=0; for(const char* p=s;*p;p++){ w+=(*p=='T')?2:1; if(p[1]) w+=1; }
  int C=(11-w+1)/2;
  for(const char* p=s;*p;p++){ C+=drawSketchNarrowGlyph(*p,topR,C,col)+1; }
}
void drawTextStrobe(uint8_t id,uint16_t n,CRGB col){
  switch(id){
    case TOGA_TEXT_RAVE:
      drawRaveBlock(col);                                      // bold RA / VE filling the wall
      break;
    case TOGA_TEXT_ALLE:                                       // all three words at once, static
      drawAlleGegenAlle(col);
      break;
    case TOGA_TEXT_ELEFANT: {                                  // trunk swings every flash, turns every 4th
      const SegSprite& sp=ELEFANT[n&1];
      drawSegSprite(sp,(12-sp.h)/2,(12-sp.w)/2,((n/TX_ELEFANT_FLIP)&1)!=0,col);
      break; }
    case TOGA_TEXT_DH_QUADRAT:                                 // DHA / RTU / AHS: DRAHTHAUS read down the columns
      drawSketchWide("DHA",2,1,col);                           // the band's sketch letters, 2x2 squares each,
      drawSketchWide("RTU",2,4,col);                           // one column right of centre so the serifs have room
      drawSketchWide("AHS",2,7,col);
      break;
    case TOGA_TEXT_DRAHT_HAUS:                                 // two lines, like CASTLE / 2026
      drawSketchNarrow("DRAHT",2,col);                         // the band's sketch letters, narrow
      drawSketchNarrow("HAUS", 7,col);
      break;
    default: break;
  }
}


// ── Mode-change readout: the mode NUMBER in the top-right corner for ~0.5s,
//    drawn INTO the frame by showGrid() (see the forward-declares in grid.ino).
//    It used to be painted after the animation's show() and pushed with a second
//    FastLED.show(); at 10-15ms per show that left it dark for half of every
//    frame, which read as a hard strobe. It is a readout, not an event — it
//    should sit there quietly and go away. ──
uint32_t overlayUntil=0;
char     overlayNum[4];     // "40" (mode) or "A2" (auto mood)
uint8_t  overlayMood=0;     // 0 = mode readout (white), 1..4 = that mood's colour
void triggerOverlay(uint32_t t,int idx){
  if(gHideOverlay) return;   // Drahthaus profile: no numbers on the wall (v11)
  // 0-based, matching the anim switch / MODE_TABLE / MODE_NAMES.md: setupGrid is
  // index 0 and reads out as "0", heartbeat as "1", … (not idx+1).
  int n=idx; char* p=overlayNum;
  if(n>=10) *p++=(char)('0'+n/10);
  *p++=(char)('0'+n%10); *p=0;
  overlayMood=0;
  overlayUntil=t+500;        // number flashes ~0.5s, top-right, over the animation
}
// The auto mood (button 0) reads out as "A1".."A4" in that mood's own colour —
// the same colour button 0 is blinking in, so the wall and the controller can
// never name the same mood differently.
//
// The "A" carries the identity, not the colour: "A2" and mode 2 would otherwise
// differ by hue alone, and the tint walks the mood hues onto each other (mood 2
// at tint 150 lands on mood 4's own green). The colour is the link to the
// button; the letter is what makes the readout unambiguous.
//
// Held twice as long as the mode number: it answers "what did I just press" on
// a button that only ever counts up 1→2→3→4→1 and never shows where it stopped.
#define AUTO_MS 1000
void triggerOverlayMood(uint32_t t,uint8_t mood){
  if(gHideOverlay) return;   // Drahthaus profile: no readouts on the wall (v11)
  if(mood<1||mood>TOGA_AUTO_COUNT) return;
  overlayNum[0]='A'; overlayNum[1]=(char)('0'+mood); overlayNum[2]=0;
  overlayMood=mood;
  overlayUntil=t+AUTO_MS;
}
// Draw the mode number small in the top-right (rows 0..2), into the frame.
// Plain white, deliberately dim: it is drawn AFTER the AGC has measured the
// frame, so it never gets dimmed along with a bright mode — at 150 it was the
// brightest thing on the wall on every mode change. Tune NUM_WHITE alone if it
// reads too bright or too faint; it is the only knob for this.
//
// AUTO_V is the same knob for the mood readout, and it is higher for a reason:
// a fully saturated hue at NUM_WHITE lights one channel where white lights
// three, so at 70 it would read as a third of the brightness. It is not a
// second opinion about how bright the corner should be.
#define NUM_WHITE 70
#define AUTO_V   160
void drawNumberTopRight(uint32_t t){
  // Colour resolved HERE, not at trigger time: hueAuto rotates continuously, and
  // button 0 wears the tint live. Latching it would leave the wall on a stale
  // hue for the whole second while the button turned — at the top of the Hue-
  // Speed dial that is three full revolutions of disagreement.
  CRGB col(NUM_WHITE,NUM_WHITE,NUM_WHITE);
  if(overlayMood)
    col=CRGB(CHSV((uint8_t)(TOGA_AUTO_HUE[overlayMood-1]+gHueBase+gHueAuto),255,AUTO_V));
  int w=textLenNodes(overlayNum);           // node-cols (3 per digit incl. gap)
  int leftC=11-w; if(leftC<0) leftC=0;       // right-aligned, ~1 node margin
  drawTextCells(overlayNum,leftC,0,col);
}
